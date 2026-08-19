#include "Engine/Core/Application.h"
#include "Engine/Core/Dx12Context.h"

#include <chrono>

namespace Engine
{
    int Application::Run()
    {
        // 文字化け対策
        SetConsoleOutputCP(CP_UTF8);

        // COMライブラリの初期化（テクスチャ読み込み時に使用するため）
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);

        // ウィンドウ生成
        CreateGameWindow(m_hwnd, m_windowClass);

        // 表示
        ShowWindow(m_hwnd, SW_SHOW);

        if (!m_renderer.Initialize(m_hwnd))
        {
            OutputDebugStringA("Renderer::Initialize failed\n");
            UnregisterClass(m_windowClass.lpszClassName, m_windowClass.hInstance);
            CoUninitialize();
            return -1;
        }

        if (!OnStart())
        {
            m_renderer.Shutdown();
            UnregisterClass(m_windowClass.lpszClassName, m_windowClass.hInstance);
            CoUninitialize();
            return -1;
        }

        auto prev = std::chrono::high_resolution_clock::now();

        MSG msg = {};
        while (true)
        {
            // 溜まったウィンドウメッセージを片付ける
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            if (msg.message == WM_QUIT) break;

            // 実時間を測り、消化すべき固定ステップ数に変換する
            auto  now = std::chrono::high_resolution_clock::now();
            float real = std::chrono::duration<float>(now - prev).count();
            prev = now;

            const int steps = m_clock.Advance(real);
            for (int i = 0; i < steps; i++)
            {
                OnUpdate(m_clock.StepSeconds());
            }

            m_renderer.BeginFrame(m_clearColor);
            OnRender(m_clock.Alpha());
            m_renderer.EndFrame();
        }

        OnShutdown();
        m_renderer.Shutdown();

        UnregisterClass(m_windowClass.lpszClassName, m_windowClass.hInstance);
        CoUninitialize();
        return 0;
    }
}