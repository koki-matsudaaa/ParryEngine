#include "Engine/Core/Application.h"
#include "Engine/Core/Dx12Context.h"

#include <chrono>

namespace Engine
{
    int Application::Run()
    {
        // /utf-8 でコンパイルしているので文字列リテラルは UTF-8。
        // Windows のコンソールは既定で CP932 なので、合わせておかないと化ける。
        SetConsoleOutputCP(CP_UTF8);

        // テクスチャ読み込み (WIC) が COM を使うので先に初期化する。
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);

        CreateGameWindow(m_hwnd, m_windowClass);
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