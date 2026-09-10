#pragma once
#include <Windows.h>
#include "Engine/Core/FixedTimestep.h"
#include "Engine/Graphics/Renderer.h"
#include "Engine/Tools/ImGuiLayer.h"

namespace Engine
{
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
        // DX12の起動後
        virtual bool OnStart() { return true; }

        // 固定ステップごと
        virtual void OnUpdate(float dt) { (void)dt; }

        // 描画。1フレームごと
        virtual void OnRender(float alpha) { (void)alpha; }

        // 調整用の ImGui ウィンドウを出す。毎フレーム呼ばれる。
        virtual void OnGui() {}

        // 終了時に1回。DX12 を片付ける前に呼ばれる。
        virtual void OnShutdown() {}

        // ---ゲーム側から使える機能---
        HWND Window() const { return m_hwnd; }
        FixedTimestep& Clock() { return m_clock; }
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
        ImGuiLayer m_imgui;

        float m_clearColor[4] = { 0.05f, 0.06f, 0.12f, 1.0f };
    };
}