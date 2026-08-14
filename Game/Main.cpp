#include "Engine/Core/Dx12Context.h"
#include "Engine/Core/FixedTimestep.h"

#include <chrono>
#include <cstdio>

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    // ウィンドウを作って表示する
    HWND       hwnd = nullptr;
    WNDCLASSEX windowClass = {};
    CreateGameWindow(hwnd, windowClass);
    ShowWindow(hwnd, SW_SHOW);

    Engine::FixedTimestep clock;
    auto prev = std::chrono::high_resolution_clock::now();

    std::printf("ウィンドウを閉じると終了します\n");

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

        // 実時間を測り、固定ステップの回数に変換する
        auto  now = std::chrono::high_resolution_clock::now();
        float real = std::chrono::duration<float>(now - prev).count();
        prev = now;

        int steps = clock.Advance(real);
        for (int i = 0; i < steps; i++)
        {
            // ここが将来の world.Step(clock.StepSeconds())
        }

        // ここが将来の renderer.Draw(clock.Alpha())
    }

    std::printf("終了。総フレーム数 %llu\n", clock.FrameCount());

    UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
    return 0;
}