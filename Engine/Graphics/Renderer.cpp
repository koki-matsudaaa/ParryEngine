#include "Engine/Core/pch.h"
#include "Engine/Graphics/Renderer.h"
#include "Engine/Core/Dx12Context.h"

namespace Engine
{
    namespace
    {
        // 定数バッファのアドレスは 256 バイト境界でないといけない。
        constexpr UINT kObjectStride = 256;
        constexpr UINT kMaxObjectsPerFrame = 256;

        // CPU から毎フレーム書き込めるバッファ (アップロードヒープ) を作る。
        ID3D12Resource* CreateUploadBuffer(size_t bytes)
        {
            D3D12_HEAP_PROPERTIES heap = {};
            heap.Type = D3D12_HEAP_TYPE_UPLOAD;

            D3D12_RESOURCE_DESC desc = {};
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            desc.Width = bytes;
            desc.Height = 1;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.SampleDesc.Count = 1;
            desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            ID3D12Resource* res = nullptr;
            if (FAILED(_dev->CreateCommittedResource(
                &heap, D3D12_HEAP_FLAG_NONE, &desc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&res))))
            {
                return nullptr;
            }
            return res;
        }
    }

    bool Renderer::Initialize(HWND hwnd)
    {
#ifdef _DEBUG
        EnableDebugLayer();   // デバイス生成より前に呼ぶ必要がある
#endif
        if (FAILED(InitializeDXGIDevice()))               return false;
        if (FAILED(InitializeCommand()))                  return false;
        if (FAILED(CreateSwapChain(hwnd, _dxgiFactory)))  return false;
        if (FAILED(CreateFinalRenderTarget(m_rtvHeap, m_backBuffers))) return false;

        // ── 深度バッファ ──────────────────────────
        D3D12_RESOURCE_DESC depthResDesc = {};
        depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthResDesc.Width = window_width;
        depthResDesc.Height = window_height;
        depthResDesc.DepthOrArraySize = 1;
        depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthResDesc.SampleDesc.Count = 1;
        depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        depthResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthResDesc.MipLevels = 1;

        D3D12_HEAP_PROPERTIES depthHeapProp = {};
        depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
        depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        // 深度は「一番奥 = 1.0」で埋めるのが既定。
        D3D12_CLEAR_VALUE depthClearValue = {};
        depthClearValue.DepthStencil.Depth = 1.0f;
        depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

        if (FAILED(_dev->CreateCommittedResource(
            &depthHeapProp, D3D12_HEAP_FLAG_NONE, &depthResDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue,
            IID_PPV_ARGS(&m_depthBuffer))))
        {
            return false;
        }

        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        if (FAILED(_dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap))))
        {
            return false;
        }

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        _dev->CreateDepthStencilView(m_depthBuffer, &dsvDesc,
            m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

        // ── フェンス (CPU が GPU の完了を待つための仕組み) ──
        if (FAILED(_dev->CreateFence(m_fenceVal, D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&m_fence))))
        {
            return false;
        }

        // ── 拡張子ごとのテクスチャ読み込み方を登録 ──
        // Dx12Context の LoadTextureFromFile がこの表を引く。
        loadLambdaTable["sph"] =
        loadLambdaTable["spa"] =
        loadLambdaTable["bmp"] =
        loadLambdaTable["png"] =
        loadLambdaTable["jpg"] =
            [](const std::wstring& path, DirectX::TexMetadata* meta,
                DirectX::ScratchImage& img) -> HRESULT
            {
                return DirectX::LoadFromWICFile(path.c_str(),
                    DirectX::WIC_FLAGS_NONE, meta, img);
            };
        loadLambdaTable["tga"] =
            [](const std::wstring& path, DirectX::TexMetadata* meta,
                DirectX::ScratchImage& img) -> HRESULT
            {
                return DirectX::LoadFromTGAFile(path.c_str(), meta, img);
            };
        loadLambdaTable["dds"] =
            [](const std::wstring& path, DirectX::TexMetadata* meta,
                DirectX::ScratchImage& img) -> HRESULT
            {
                return DirectX::LoadFromDDSFile(path.c_str(),
                    DirectX::DDS_FLAGS_NONE, meta, img);
            };

        // ── ビューポートとシザー矩形 ──────────────────
        m_viewport.Width = static_cast<float>(window_width);
        m_viewport.Height = static_cast<float>(window_height);
        m_viewport.TopLeftX = 0.0f;
        m_viewport.TopLeftY = 0.0f;
        m_viewport.MinDepth = 0.0f;
        m_viewport.MaxDepth = 1.0f;

        m_scissor.left = 0;
        m_scissor.top = 0;
        m_scissor.right = static_cast<LONG>(window_width);
        m_scissor.bottom = static_cast<LONG>(window_height);

        // ── シェーダとパイプライン ──────────────────
        if (!m_pipelines.Initialize()) return false;
        if (!CreateConstantBuffers()) return false;

        return true;
    }

    void Renderer::Shutdown()
    {
        // GPU が使っている最中に解放すると落ちるので、必ず待ってから。
        if (m_fence) WaitForGpu();

        m_pipelines.Shutdown();
        if (m_objectCB) { m_objectCB->Unmap(0, nullptr); m_objectCB->Release(); m_objectCB = nullptr; }
        if (m_sceneCB) { m_sceneCB->Unmap(0, nullptr);  m_sceneCB->Release();  m_sceneCB = nullptr; }
        m_objectRaw = nullptr;
        m_sceneMap = nullptr;

        if (m_fence) { m_fence->Release();       m_fence = nullptr; }
        if (m_dsvHeap) { m_dsvHeap->Release();     m_dsvHeap = nullptr; }
        if (m_depthBuffer) { m_depthBuffer->Release(); m_depthBuffer = nullptr; }
        if (m_rtvHeap) { m_rtvHeap->Release();     m_rtvHeap = nullptr; }

        for (auto* buf : m_backBuffers) if (buf) buf->Release();
        m_backBuffers.clear();
    }

    ID3D12GraphicsCommandList* Renderer::CommandList() const
    {
        return _cmdList;
    }

    void Renderer::BeginFrame(const float clearColor[4])
    {
        m_objectIndex = 0;   // オブジェクト定数の使用位置を巻き戻す
        m_backBufferIndex = _swapchain->GetCurrentBackBufferIndex();

        // 画面に出す用 → 描画先 へ状態を移す
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_backBuffers[m_backBufferIndex],
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
        _cmdList->ResourceBarrier(1, &barrier);

        // 今回のバックバッファに対応する RTV の位置を求める
        auto rtvH = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvH.ptr += static_cast<ULONG_PTR>(
            m_backBufferIndex * _dev->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

        auto dsvH = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

        _cmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);
        _cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
        _cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH,
            1.0f, 0, 0, nullptr);

        _cmdList->RSSetViewports(1, &m_viewport);
        _cmdList->RSSetScissorRects(1, &m_scissor);
    }

    void Renderer::EndFrame()
    {
        // 描画先 → 画面に出す用 へ戻す
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_backBuffers[m_backBufferIndex],
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);
        _cmdList->ResourceBarrier(1, &barrier);
        _cmdList->Close();

        ID3D12CommandList* cmdlists[] = { _cmdList };
        _cmdQueue->ExecuteCommandLists(1, cmdlists);

        WaitForGpu();

        _swapchain->Present(1, 0);

        // 次のフレームのためにコマンドを積み直せる状態へ戻す
        _cmdAllocator->Reset();
        _cmdList->Reset(_cmdAllocator, nullptr);
    }

    void Renderer::WaitForGpu()
    {
        ++m_fenceVal;
        _cmdQueue->Signal(m_fence, m_fenceVal);

        if (m_fence->GetCompletedValue() != m_fenceVal)
        {
            auto event = CreateEvent(nullptr, false, false, nullptr);
            m_fence->SetEventOnCompletion(m_fenceVal, event);
            WaitForSingleObjectEx(event, INFINITE, false);
            CloseHandle(event);
        }
    }

    bool Renderer::CreateConstantBuffers()
    {
        // b0 : カメラ。1フレームに1回だけ書く。
        m_sceneCB = CreateUploadBuffer((sizeof(SceneConstants) + 255) & ~255u);
        if (!m_sceneCB) return false;
        if (FAILED(m_sceneCB->Map(0, nullptr,
            reinterpret_cast<void**>(&m_sceneMap)))) return false;

        // b2 : オブジェクトごとの world。
        // 1フレームに複数描けるよう、256バイト刻みで並べて確保しておく。
        // 1つしか用意しないと、後から書いた world で全部が上書きされてしまう。
        m_objectCB = CreateUploadBuffer(kObjectStride * kMaxObjectsPerFrame);
        if (!m_objectCB) return false;
        if (FAILED(m_objectCB->Map(0, nullptr,
            reinterpret_cast<void**>(&m_objectRaw)))) return false;

        return true;
    }

    void Renderer::SetCamera(const DirectX::XMMATRIX& view,
        const DirectX::XMMATRIX& proj,
        const DirectX::XMFLOAT3& eye)
    {
        m_sceneMap->view = view;
        m_sceneMap->proj = proj;
        m_sceneMap->eye = eye;
    }

    void Renderer::DrawModel(IRenderable* model, const DirectX::XMMATRIX& world)
    {
        if (!model) return;

        // どのパイプラインで描くかはモデル自身が答える。
        ID3D12PipelineState* pso = m_pipelines.Get(model->GetPipelineType());
        if (!pso) return;

        if (m_objectIndex >= kMaxObjectsPerFrame) return;

        // このオブジェクト専用の場所に world を書く。
        auto* dst = reinterpret_cast<DirectX::XMMATRIX*>(
            m_objectRaw + m_objectIndex * kObjectStride);
        *dst = world;

        _cmdList->SetPipelineState(pso);
        _cmdList->SetGraphicsRootSignature(m_pipelines.RootSignature());
        _cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        _cmdList->SetGraphicsRootConstantBufferView(
            0, m_sceneCB->GetGPUVirtualAddress());
        _cmdList->SetGraphicsRootConstantBufferView(
            1, m_objectCB->GetGPUVirtualAddress() + m_objectIndex * kObjectStride);

        // 頂点バッファとテクスチャの割り当ては、モデル自身が行う。
        model->Draw(_cmdList);

        m_objectIndex++;
    }
}