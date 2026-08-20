#include "Engine/Core/Application.h"
#include "Engine/Graphics/FbxModel.h"
#include "Engine/Graphics/Camera.h"

#include <cstdio>

using namespace DirectX;

class GameApp : public Engine::Application
{
    // ── 単位の決め事 ──────────────────────────
    // このプロジェクトは 1 unit = 1 メートル とする。
    // 重力 9.8、歩行 4m/s、パリィ判定の半径 0.5m のように、
    // 現実の数字をそのまま調整値として使えるようにするため。
    static constexpr float kCharacterScale = 0.01f;  // 180  → 1.8m
    static constexpr float kTerrainScale = 20.0f;  //   2  → 40m 四方

    FbxModel m_terrain;
    FbxModel m_character;
    Engine::Camera m_camera;

protected:
    bool OnStart() override
    {
        SetClearColor(0.10f, 0.16f, 0.24f);

        if (!m_terrain.Load("Assets/Model/SnowTerrain.fbx", "Assets/Model/grass_texture_01.png"))
        {
            std::printf("FBX の読み込みに失敗しました\n");
            return false;
        }

        m_camera.SetTarget(XMFLOAT3(0.0f, 0.0f, 0.0f));
        m_camera.SetDistance(4.0f);
        std::printf("矢印キーでカメラを回せます\n");

        // ボーンを持たないメッシュなので Static パイプラインで描く。
        m_terrain.SetStaticPipeline(true);

        // カメラの距離を決めるため、モデルの大きさを出しておく。
        const XMFLOAT3 mn = m_terrain.GetMin();
        const XMFLOAT3 mx = m_terrain.GetMax();
        std::printf("頂点数 %zu\n", m_terrain.GetVertices().size());
        std::printf("範囲 X[%.1f .. %.1f]  Y[%.1f .. %.1f]  Z[%.1f .. %.1f]\n",
            mn.x, mx.x, mn.y, mx.y, mn.z, mx.z);

        // メッシュとスケルトンは1回だけ読む。この FBX のアニメは "Idle" になる。
        if (!m_character.Load("Assets/Model/Bot_Idle.fbx", "", "Idle"))
        {
            std::printf("キャラクターの読み込みに失敗しました\n");
            return false;
        }

        // モーションだけを追加で読む。メッシュは重複しない。
        if (!m_character.LoadClip("Run", "Assets/Model/Bot_Run.fbx"))
            std::printf("Run の読み込みに失敗（ファイルがまだ無い？）\n");
        if (!m_character.LoadClip("Slash", "Assets/Model/Bot_Slash.fbx"))
            std::printf("Slash の読み込みに失敗（ファイルがまだ無い？）\n");

        m_character.Play("Idle");
        std::printf("クリップ数 %zu / 再生中 %s\n",
            m_character.GetClipCount(), m_character.CurrentClipName().c_str());
        std::printf("1=Idle  2=Run  3=Slash で切り替え\n");

        const XMFLOAT3 cmn = m_character.GetMin();
        const XMFLOAT3 cmx = m_character.GetMax();
        std::printf("キャラ ボーン数 %zu / フレーム数 %d\n",
            m_character.GetBoneCount(), m_character.GetFrameCount());
        std::printf("キャラ範囲 X[%.1f .. %.1f]  Y[%.1f .. %.1f]  Z[%.1f .. %.1f]\n",
            cmn.x, cmx.x, cmn.y, cmx.y, cmn.z, cmx.z);

        return true;
    }

    void OnUpdate(float dt) override
    {
        m_character.UpdateAnimation(dt);

        // 矢印キーでカメラを回す。
        const float rotSpeed = 2.0f;   // ラジアン/秒
        if (GetAsyncKeyState(VK_LEFT) & 0x8000) m_camera.AddYaw(-rotSpeed * dt);
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000) m_camera.AddYaw(+rotSpeed * dt);
        if (GetAsyncKeyState(VK_UP) & 0x8000) m_camera.AddPitch(-rotSpeed * dt);
        if (GetAsyncKeyState(VK_DOWN) & 0x8000) m_camera.AddPitch(+rotSpeed * dt);

        // 数字キーでモーションを切り替える。
        if (GetAsyncKeyState('1') & 0x8000) m_character.Play("Idle");
        if (GetAsyncKeyState('2') & 0x8000) m_character.Play("Run");
        if (GetAsyncKeyState('3') & 0x8000) m_character.Play("Slash");
    }

    void OnRender(float alpha) override
    {
        (void)alpha;

        GetRenderer().SetCamera(m_camera.View(), m_camera.Projection(), m_camera.Eye());

        GetRenderer().DrawModel(&m_terrain,
            XMMatrixScaling(kTerrainScale, kTerrainScale, kTerrainScale));
        GetRenderer().DrawModel(&m_character,
            XMMatrixScaling(kCharacterScale, kCharacterScale, kCharacterScale));
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