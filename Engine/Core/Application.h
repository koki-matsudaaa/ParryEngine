#pragma once
#include <Windows.h>
#include "Engine/Core/FixedTimestep.h"
#include "Engine/Graphics/Renderer.h"

namespace Engine
{
    // ────────────────────────────────────────────────────────────
    // アプリケーションの土台。
    //
    // ウィンドウの生成・メッセージ処理・固定タイムステップのループ・
    // 描画の開始と終了を持つ。
    // ゲーム側はこれを継承し、On... の4つを実装するだけでよい。
    //
    // このクラスはゲームの内容を一切知らない。
    // 知る必要が出たら、それは設計を間違えている合図。
    // ────────────────────────────────────────────────────────────
    class Application
    {
    public:
        Application() = default;
        virtual ~Application() = default;

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        // ウィンドウを出してループを回す。閉じられるまで戻らない。
        int Run();

    protected:
        // ── ゲーム側が実装する ──────────────────────────

        // 起動時に1回。DX12 の準備が済んだ後に呼ばれる。
        // false を返すと起動を中止する。
        virtual bool OnStart() { return true; }

        // 固定ステップごとに呼ばれる。dt は常に同じ値 (既定 1/60秒)。
        // 1フレームに0回のことも、複数回のこともある。
        virtual void OnUpdate(float dt) { (void)dt; }

        // 描画。1フレームに1回だけ呼ばれる。
        // 画面はすでに塗られた状態で来るので、この中で描画コマンドを積む。
        // alpha は直前のステップと現在のステップの間の位置 (0..1)。
        virtual void OnRender(float alpha) { (void)alpha; }

        // 終了時に1回。DX12 を片付ける前に呼ばれる。
        virtual void OnShutdown() {}

        // ── ゲーム側から使える機能 ──────────────────────

        HWND Window() const { return m_hwnd; }
        const FixedTimestep& Clock() const { return m_clock; }
        Renderer& GetRenderer() { return m_renderer; }

        // 毎フレーム画面を塗る色。OnStart で設定する。
        void SetClearColor(float r, float g, float b, float a = 1.0f)
        {
            m_clearColor[0] = r; m_clearColor[1] = g;
            m_clearColor[2] = b; m_clearColor[3] = a;
        }

        // ループを終わらせたいときに呼ぶ。
        void Quit() { PostQuitMessage(0); }

    private:
        HWND          m_hwnd = nullptr;
        WNDCLASSEX    m_windowClass = {};
        FixedTimestep m_clock;
        Renderer      m_renderer;

        float m_clearColor[4] = { 0.05f, 0.06f, 0.12f, 1.0f };
    };
}