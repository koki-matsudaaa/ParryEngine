// ParryEngine — 動作確認用の仮エントリポイント (M0)
//
// この段階では描画は無い。目的は次の2点だけ。
//   1. Game が Engine.lib と正しくリンクできること
//   2. 固定タイムステップが期待どおりに動くこと
//
// M1 で Engine 側に Application / Renderer を作り、ここは
// 「Application を生成して走らせるだけ」の数行に置き換える。

#include "Engine/Core/FixedTimestep.h"

#include <chrono>
#include <cstdio>

int main()
{
    Engine::FixedTimestep clock;   // 既定 1/60秒

    std::printf("ParryEngine M0\n");
    std::printf("step = %.6f sec (%.1f fps)\n\n",
                clock.StepSeconds(), clock.StepsPerSecond());

    auto prev = std::chrono::high_resolution_clock::now();
    int  loops = 0;

    // 2秒ぶん回して、消化されたステップ数が 120 前後になるか確かめる。
    while (clock.FrameCount() < 120)
    {
        auto now = std::chrono::high_resolution_clock::now();
        float real = std::chrono::duration<float>(now - prev).count();
        prev = now;

        int steps = clock.Advance(real);

        for (int i = 0; i < steps; i++)
        {
            // ここが将来の world.Step(clock.StepSeconds())。
            // dt は常に固定なので、同じ入力なら必ず同じ結果になる。
        }

        loops++;
    }

    std::printf("消化ステップ数 : %llu\n", clock.FrameCount());
    std::printf("ループ回数     : %d\n", loops);
    std::printf("補間係数 alpha : %.3f\n", clock.Alpha());
    std::printf("\nEngine.lib へのリンク成功。M1 へ進めます。\n");
    return 0;
}
