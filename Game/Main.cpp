#include "Engine/Core/Application.h"
#include "Engine/Graphics/FbxModel.h"

#include <cstdio>

class GameApp : public Engine::Application
{
    FbxModel m_terrain;

protected:
    bool OnStart() override
    {
        SetClearColor(0.10f, 0.16f, 0.24f);

        // 実行時のカレントはソリューションのルート。
        // (Game のプロパティ → デバッグ → 作業ディレクトリ = $(SolutionDir))
        if (!m_terrain.Load("Assets/Model/SnowTerrain.fbx"))
        {
            std::printf("FBX の読み込みに失敗しました\n");
            return false;
        }

        std::printf("読み込み成功\n");
        std::printf("  頂点数       : %zu\n", m_terrain.GetVertices().size());
        std::printf("  インデックス : %zu\n", m_terrain.GetIndices().size());
        std::printf("  ボーン数     : %zu\n", m_terrain.GetBoneCount());
        std::printf("ウィンドウを閉じると終了します\n");
        return true;
    }

    void OnUpdate(float dt) override
    {
        (void)dt;
    }

    void OnRender(float alpha) override
    {
        // ここに描画コマンドを積む。次のステップで実装する。
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