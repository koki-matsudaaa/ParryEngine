#pragma once
#include <Windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <d3dx12.h>
#include <vector>
#include <map>
#include <string>
#include <functional>

#pragma comment(lib, "DirectXTex.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;
using namespace std;

// ウィンドウサイズ
extern const unsigned int window_width;
extern const unsigned int window_height;

// DirectX12 グローバルオブジェクト
extern IDXGIFactory4* _dxgiFactory;
extern ID3D12Device* _dev;
extern ID3D12CommandAllocator* _cmdAllocator;
extern ID3D12GraphicsCommandList* _cmdList;
extern ID3D12CommandQueue* _cmdQueue;
extern IDXGISwapChain4* _swapchain;

// テクスチャロードラムダテーブル
using LoadLambda_t = function<HRESULT(const wstring& path, TexMetadata*, ScratchImage&)>;
extern map<string, LoadLambda_t> loadLambdaTable;

// ファイル名→リソースのキャッシュテーブル
extern map<string, ID3D12Resource*> _resourceTable;

// デバッグ出力
void DebugOutputFormatString(const char* format, ...);

// ウィンドウ
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
void    CreateGameWindow(HWND& hwnd, WNDCLASSEX& windowClass);

// DX12初期化
void    EnableDebugLayer();
HRESULT InitializeDXGIDevice();
HRESULT InitializeCommand();
HRESULT CreateSwapChain(const HWND& hwnd, IDXGIFactory4*& dxgiFactory);
HRESULT CreateFinalRenderTarget(ID3D12DescriptorHeap*& rtvHeaps,
    vector<ID3D12Resource*>& backBuffers);

// テクスチャユーティリティ
ID3D12Resource* CreateWhiteTexture();
ID3D12Resource* CreateBlackTexture();
ID3D12Resource* CreateGrayGradationTexture();
ID3D12Resource* LoadTextureFromFile(std::string& texPath);

// 文字列ユーティリティ
std::string  GetTexturePathFromModelAndTexPath(const std::string& modelPath,
    const char* texPath);
std::string  GetExtension(const std::string& path);
std::wstring GetExtension(const std::wstring& path);
std::pair<std::string, std::string> SplitFileName(const std::string& path,
    const char splitter = '*');
std::wstring GetWideStringFromString(const std::string& str);