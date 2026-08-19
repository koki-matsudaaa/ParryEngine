#include "Engine/Core/pch.h"
// コンスタントバッファで行列を転送
#include "Engine/Core/Dx12Context.h"
#include "imgui_impl_win32.h"
#include <d3dcompiler.h>
#include <dxgidebug.h>
#include <cassert>

#ifdef _DEBUG
#include <iostream>
#endif

// ──────────────────────────────────────────
// グローバル変数の実体
// ──────────────────────────────────────────
const unsigned int window_width = 1280;
const unsigned int window_height = 720;

IDXGIFactory4* _dxgiFactory = nullptr;
ID3D12Device* _dev = nullptr;
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;
IDXGISwapChain4* _swapchain = nullptr;

map<string, LoadLambda_t>   loadLambdaTable;
map<string, ID3D12Resource*> _resourceTable;

// ──────────────────────────────────────────
// デバッグ出力
// ──────────────────────────────────────────
void DebugOutputFormatString(const char* format, ...) {
#ifdef _DEBUG
    va_list valist;
    va_start(valist, format);
    printf(format, valist);
    va_end(valist);
#endif
}

// ──────────────────────────────────────────
// ウィンドウ
// ──────────────────────────────────────────

// ImGuiのハンドラを前方宣言 (imgui_impl_win32.h が提供)
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

    // ImGuiに先に処理させる
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return true;

    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

void CreateGameWindow(HWND& hwnd, WNDCLASSEX& windowClass) {
    HINSTANCE hInst = GetModuleHandle(nullptr);
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.lpfnWndProc = (WNDPROC)WindowProcedure;
    windowClass.lpszClassName = _T("DirectXTest");
    windowClass.hInstance = GetModuleHandle(0);
    RegisterClassEx(&windowClass);

    RECT wrc = { 0, 0, (LONG)window_width, (LONG)window_height };
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
    hwnd = CreateWindow(
        windowClass.lpszClassName,
        _T("ParryEngine"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wrc.right - wrc.left,
        wrc.bottom - wrc.top,
        nullptr, nullptr,
        windowClass.hInstance,
        nullptr);
}

// ──────────────────────────────────────────
// DX12 初期化
// ──────────────────────────────────────────
void EnableDebugLayer() {
    ID3D12Debug* debugLayer = nullptr;
    D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
    debugLayer->EnableDebugLayer();
    debugLayer->Release();
}

HRESULT InitializeDXGIDevice() {
    UINT flagsDXGI = DXGI_CREATE_FACTORY_DEBUG;
    auto result = CreateDXGIFactory2(flagsDXGI, IID_PPV_ARGS(&_dxgiFactory));
    if (FAILED(result)) 
        return result;

    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    vector<IDXGIAdapter*> adapters;
    IDXGIAdapter* tmpAdapter = nullptr;
    for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
        adapters.push_back(tmpAdapter);

    for (auto adpt : adapters) {
        DXGI_ADAPTER_DESC adesc = {};
        adpt->GetDesc(&adesc);
        wstring strDesc = adesc.Description;
        if (strDesc.find(L"NVIDIA") != wstring::npos) {
            tmpAdapter = adpt;
            break;
        }
    }

    result = S_FALSE;
    D3D_FEATURE_LEVEL featureLevel;
    for (auto l : levels) {
        if (D3D12CreateDevice(tmpAdapter, l, IID_PPV_ARGS(&_dev)) == S_OK) {
            featureLevel = l;
            result = S_OK;
            break;
        }
    }
    return result;
}

HRESULT InitializeCommand() {
    auto result = _dev->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
    if (FAILED(result)) { 
        assert(0); return result;
    }

    result = _dev->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr,
        IID_PPV_ARGS(&_cmdList));
    if (FAILED(result)) { 
        assert(0); return result;
    }

    D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
    cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cmdQueueDesc.NodeMask = 0;
    cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));
    if (FAILED(result)) {
        assert(0);
    }
    return S_OK;
}

HRESULT CreateSwapChain(const HWND& hwnd, IDXGIFactory4*& dxgiFactory) {
    DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
    swapchainDesc.Width = window_width;
    swapchainDesc.Height = window_height;
    swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchainDesc.Stereo = false;
    swapchainDesc.SampleDesc.Count = 1;
    swapchainDesc.SampleDesc.Quality = 0;
    swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDesc.BufferCount = 2;
    swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    return dxgiFactory->CreateSwapChainForHwnd(
        _cmdQueue, hwnd, &swapchainDesc, nullptr, nullptr,
        (IDXGISwapChain1**)&_swapchain);
}

HRESULT CreateFinalRenderTarget(ID3D12DescriptorHeap*& rtvHeaps,
    vector<ID3D12Resource*>& backBuffers) {
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.NodeMask = 0;
    heapDesc.NumDescriptors = 2;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    auto result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));
    if (FAILED(result)) { assert(0); return result; }

    DXGI_SWAP_CHAIN_DESC swcDesc = {};
    _swapchain->GetDesc(&swcDesc);
    backBuffers.resize(swcDesc.BufferCount);

    D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    for (size_t i = 0; i < swcDesc.BufferCount; ++i) {
        result = _swapchain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&backBuffers[i]));
        assert(SUCCEEDED(result));
        rtvDesc.Format = backBuffers[i]->GetDesc().Format;
        _dev->CreateRenderTargetView(backBuffers[i], &rtvDesc, handle);
        handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }
    return S_OK;
}

// ──────────────────────────────────────────
// テクスチャユーティリティ
// ──────────────────────────────────────────
ID3D12Resource* CreateGrayGradationTexture() {
    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resDesc.Width = 4;
    resDesc.Height = 256;
    resDesc.DepthOrArraySize = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.SampleDesc.Quality = 0;
    resDesc.MipLevels = 1;
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES texHeapProp = {};
    texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
    texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
    texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
    texHeapProp.CreationNodeMask = 0;
    texHeapProp.VisibleNodeMask = 0;

    ID3D12Resource* gradBuff = nullptr;
    auto result = _dev->CreateCommittedResource(
        &texHeapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr, IID_PPV_ARGS(&gradBuff));
    if (FAILED(result)) return nullptr;

    vector<unsigned int> data(4 * 256);
    auto it = data.begin();
    unsigned int c = 0xff;
    for (; it != data.end(); it += 4) {
        auto col = (0xff << 24) | RGB(c, c, c);
        fill(it, it + 4, col);
        --c;
    }
    gradBuff->WriteToSubresource(0, nullptr, data.data(),
        4 * sizeof(unsigned int),
        sizeof(unsigned int) * static_cast<UINT>(data.size()));
    return gradBuff;
}

ID3D12Resource* CreateWhiteTexture() {
    D3D12_HEAP_PROPERTIES texHeapProp = {};
    texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
    texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
    texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
    texHeapProp.CreationNodeMask = 0;
    texHeapProp.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resDesc.Width = 4;
    resDesc.Height = 4;
    resDesc.DepthOrArraySize = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.SampleDesc.Quality = 0;
    resDesc.MipLevels = 1;
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ID3D12Resource* whiteBuff = nullptr;
    auto result = _dev->CreateCommittedResource(
        &texHeapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr, IID_PPV_ARGS(&whiteBuff));
    if (FAILED(result)) return nullptr;

    vector<unsigned char> data(4 * 4 * 4);
    fill(data.begin(), data.end(), 0xff);
    whiteBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4,
        static_cast<UINT>(data.size()));
    return whiteBuff;
}

ID3D12Resource* CreateBlackTexture() {
    D3D12_HEAP_PROPERTIES texHeapProp = {};
    texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
    texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
    texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
    texHeapProp.CreationNodeMask = 0;
    texHeapProp.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resDesc.Width = 4;
    resDesc.Height = 4;
    resDesc.DepthOrArraySize = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.SampleDesc.Quality = 0;
    resDesc.MipLevels = 1;
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ID3D12Resource* blackBuff = nullptr;
    auto result = _dev->CreateCommittedResource(
        &texHeapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr, IID_PPV_ARGS(&blackBuff));
    if (FAILED(result)) return nullptr;

    vector<unsigned char> data(4 * 4 * 4);
    fill(data.begin(), data.end(), 0x00);
    blackBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4,
        static_cast<UINT>(data.size()));
    return blackBuff;
}

ID3D12Resource* LoadTextureFromFile(std::string& texPath) {
    auto it = _resourceTable.find(texPath);
    if (it != _resourceTable.end())
        return _resourceTable[texPath];

    TexMetadata  metadata = {};
    ScratchImage scratchImg = {};
    auto wtexpath = GetWideStringFromString(texPath);
    auto ext = GetExtension(texPath);
    auto result = loadLambdaTable[ext](wtexpath, &metadata, scratchImg);
    if (FAILED(result)) return nullptr;

    auto img = scratchImg.GetImage(0, 0, 0);

    D3D12_HEAP_PROPERTIES texHeapProp = {};
    texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
    texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
    texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
    texHeapProp.CreationNodeMask = 0;
    texHeapProp.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Format = metadata.format;
    resDesc.Width = static_cast<UINT>(metadata.width);
    resDesc.Height = static_cast<UINT>(metadata.height);
    resDesc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);
    resDesc.SampleDesc.Count = 1;
    resDesc.SampleDesc.Quality = 0;
    resDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
    resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ID3D12Resource* texbuff = nullptr;
    result = _dev->CreateCommittedResource(
        &texHeapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr, IID_PPV_ARGS(&texbuff));
    if (FAILED(result)) return nullptr;

    result = texbuff->WriteToSubresource(0, nullptr,
        img->pixels,
        static_cast<UINT>(img->rowPitch),
        static_cast<UINT>(img->slicePitch));
    if (FAILED(result)) return nullptr;

    _resourceTable[texPath] = texbuff;
    return texbuff;
}

// ──────────────────────────────────────────
// 文字列ユーティリティ
// ──────────────────────────────────────────
std::string GetTexturePathFromModelAndTexPath(const std::string& modelPath,
    const char* texPath) {
    int pathIndex1 = (int)modelPath.rfind('/');
    int pathIndex2 = (int)modelPath.rfind('\\');
    int pathIndex = max(pathIndex1, pathIndex2);
    std::string folderPath = "";
    if (pathIndex >= 0) {
        folderPath = modelPath.substr(0, pathIndex + 1);
    }
    return folderPath + texPath;
}

string GetExtension(const std::string& path) {
    auto idx = path.rfind('.');
    return path.substr(idx + 1, path.length() - idx - 1);
}

wstring GetExtension(const std::wstring& path) {
    auto idx = path.rfind(L'.');
    return path.substr(idx + 1, path.length() - idx - 1);
}

pair<string, string> SplitFileName(const std::string& path, const char splitter) {
    auto idx = path.find(splitter);
    pair<string, string> ret;
    ret.first = path.substr(0, idx);
    ret.second = path.substr(idx + 1, path.length() - idx - 1);
    return ret;
}

std::wstring GetWideStringFromString(const std::string& str) {
    auto num1 = MultiByteToWideChar(CP_ACP,
        MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
        str.c_str(), -1, nullptr, 0);
    wstring wstr;
    wstr.resize(num1);
    auto num2 = MultiByteToWideChar(CP_ACP,
        MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
        str.c_str(), -1, &wstr[0], num1);
    assert(num1 == num2);
    return wstr;
}