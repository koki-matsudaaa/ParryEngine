#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <vector>

namespace Engine
{
    // ────────────────────────────────────────────────────────────
    // 画面を出すための最低限。
    //
    // デバイス・スワップチェーン・深度バッファ・フェンスを持ち、
    // 1フレームの開始と終了だけを引き受ける。
    //
    // 「何を描くか」は知らない。モデルを描くコマンドは
    // BeginFrame と EndFrame の間で、外から積む。
    // ────────────────────────────────────────────────────────────
    class Renderer
    {
    public:
        Renderer() = default;
        ~Renderer() = default;

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        // DX12 を一式立ち上げる。失敗したら false。
        bool Initialize(HWND hwnd);

        // 確保した資源を解放する。
        void Shutdown();

        // フレームの開始。今回のバックバッファを描画先にして、指定色で塗る。
        void BeginFrame(const float clearColor[4]);

        // フレームの終了。コマンドを積み終えてから呼ぶ。
        // GPU に投げて、描き終わるのを待ってから画面に出す。
        void EndFrame();

        // 描画コマンドを積む先。外部から使う。
        ID3D12GraphicsCommandList* CommandList() const;

    private:
        // GPU の処理が終わるまで待つ。
        void WaitForGpu();

        std::vector<ID3D12Resource*> m_backBuffers;
        ID3D12DescriptorHeap* m_rtvHeap = nullptr;

        ID3D12Resource* m_depthBuffer = nullptr;
        ID3D12DescriptorHeap* m_dsvHeap = nullptr;

        ID3D12Fence* m_fence = nullptr;
        UINT64       m_fenceVal = 0;

        D3D12_VIEWPORT m_viewport = {};
        D3D12_RECT     m_scissor = {};

        UINT m_backBufferIndex = 0;
    };
}