#include "Engine/Core/Application.h"

#include <cstdio>

// ゲーム本体。Engine::Application を継承して、中身だけを書く。
// ウィンドウもループも時間管理も Engine 側が持っているので、
// ここには「このゲームが何をするか」しか出てこない。
class GameApp : public Engine::Application
{
protected:
    bool OnStart() override
    {
        std::printf("ウィンドウを閉じると終了します\n");
        return true;
    }

    void OnUpdate(float dt) override
    {
        // ここに world.Step(dt) が入る。dt は常に 1/60 秒。
        (void)dt;
    }

    void OnRender(float alpha) override
    {
        // ここに renderer.Draw(alpha) が入る。
        (void)alpha;
    }

    void OnShutdown() override
    {
        std::printf("終了。総フレーム数 %llu\n", Clock().FrameCount());
    }
};

int main()
{
    GameApp app;
    return app.Run();
}