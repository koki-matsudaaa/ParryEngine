#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <vector>
#include <DirectXMath.h>
#include "Engine/Graphics/PipelineCache.h"
#include "Engine/Graphics/DebugDraw.h"

namespace Engine
{
    class Renderer
    {
    public:
        Renderer() = default;
        ~Renderer() = default;

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        // DX12 を一式立ち上げる
        bool Initialize(HWND hwnd);

        // 確保した資源を解放する
        void Shutdown();

        // フレームの開始。
        void BeginFrame(const float clearColor[4]);

        // フレームの終了。コマンドを積み終えてから呼ぶ。
        // GPU に投げて、描き終わるのを待ってから画面に出す。
        void EndFrame();

        // 描画コマンドを積む先。
        ID3D12GraphicsCommandList* CommandList() const;

        // カメラを設定
        void SetCamera(const DirectX::XMMATRIX& view,
            const DirectX::XMMATRIX& proj,
            const DirectX::XMFLOAT3& eye);

        // モデルを1つ描く。どのパイプラインで描くかはモデル自身が答える。
        void DrawModel(IRenderable* model, const DirectX::XMMATRIX& world);

        // 線を積む先
        DebugDraw& Debug() { return m_debug; }

        // 積んだ線をまとめて描く
        void FlushDebug();

    private:
        // GPU の処理が終わるまで待つ。
        void WaitForGpu();

        // b0 : 全オブジェクト共通 (カメラ)
        struct SceneConstants
        {
            DirectX::XMMATRIX view;
            DirectX::XMMATRIX proj;
            DirectX::XMFLOAT3 eye;
        };

        // b2 : オブジェクトごと
        struct ObjectConstants
        {
            DirectX::XMMATRIX world;
        };

        bool CreateConstantBuffers();

        ID3D12Resource* m_sceneCB = nullptr;
        SceneConstants* m_sceneMap = nullptr;

        ID3D12Resource* m_objectCB = nullptr;
        uint8_t* m_objectRaw = nullptr;
        UINT            m_objectIndex = 0;   // 今フレームで何個目か

        ID3D12Resource* m_boneCB = nullptr;
        uint8_t* m_boneRaw = nullptr;
        UINT            m_boneIndex = 0;

        PipelineCache m_pipelines;
        std::vector<ID3D12Resource*> m_backBuffers;
        ID3D12DescriptorHeap* m_rtvHeap = nullptr;

        ID3D12Resource* m_depthBuffer = nullptr;
        ID3D12DescriptorHeap* m_dsvHeap = nullptr;

        ID3D12Fence* m_fence = nullptr;
        UINT64       m_fenceVal = 0;

        D3D12_VIEWPORT m_viewport = {};
        D3D12_RECT     m_scissor = {};

        UINT m_backBufferIndex = 0;

        DebugDraw m_debug;
    };
}