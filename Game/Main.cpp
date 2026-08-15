#include "Engine/Core/Application.h"
#include "Engine/Graphics/FbxModel.h"

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

protected:
    bool OnStart() override
    {
        SetClearColor(0.10f, 0.16f, 0.24f);

        if (!m_terrain.Load("Assets/Model/SnowTerrain.fbx", "Assets/Model/grass_texture_01.png"))
        {
            std::printf("FBX の読み込みに失敗しました\n");
            return false;
        }

        // ボーンを持たないメッシュなので Static パイプラインで描く。
        m_terrain.SetStaticPipeline(true);

        // カメラの距離を決めるため、モデルの大きさを出しておく。
        const XMFLOAT3 mn = m_terrain.GetMin();
        const XMFLOAT3 mx = m_terrain.GetMax();
        std::printf("頂点数 %zu\n", m_terrain.GetVertices().size());
        std::printf("範囲 X[%.1f .. %.1f]  Y[%.1f .. %.1f]  Z[%.1f .. %.1f]\n",
            mn.x, mx.x, mn.y, mx.y, mn.z, mx.z);

        // キャラクター (仮)。ボーンがあるので Static にはしない。
        if (!m_character.Load("Assets/Model/Bot_Idle.fbx",
            "Assets/Model/Robot_Base_color.png"))
        {
            std::printf("キャラクターの読み込みに失敗しました\n");
            return false;
        }

        const XMFLOAT3 cmn = m_character.GetMin();
        const XMFLOAT3 cmx = m_character.GetMax();
        std::printf("キャラ ボーン数 %zu / フレーム数 %d\n",
            m_character.GetBoneCount(), m_character.GetFrameCount());
        std::printf("キャラ範囲 X[%.1f .. %.1f]  Y[%.1f .. %.1f]  Z[%.1f .. %.1f]\n",
            cmn.x, cmx.x, cmn.y, cmx.y, cmn.z, cmx.z);

        // アニメが実際に姿勢を変えているか確認する。
        // 全フレームが同じなら、焼き込みが効いていない。
        {
            m_character.SetFixedFrame(0);
            XMFLOAT4X4 f0;
            XMStoreFloat4x4(&f0, m_character.GetCurrentBoneMatrices()[5]);

            m_character.SetFixedFrame(25);
            XMFLOAT4X4 f25;
            XMStoreFloat4x4(&f25, m_character.GetCurrentBoneMatrices()[5]);

            m_character.SetFixedFrame(-1);   // 通常再生に戻す

            std::printf("frame0  ボーン5 の移動量 (%.3f, %.3f, %.3f)\n",
                f0._41, f0._42, f0._43);
            std::printf("frame25 ボーン5 の移動量 (%.3f, %.3f, %.3f)\n",
                f25._41, f25._42, f25._43);
        }

        return true;
    }

    void OnUpdate(float dt) override
    {
        // dt は常に 1/60 秒。アニメの進み方がPCの速さに左右されない。
        m_character.UpdateAnimation(dt);
    }

    void OnRender(float alpha) override
    {
        (void)alpha;

        // 身長 1.8m のキャラを、少し上から後ろ気味に見る。
        const XMVECTOR eye = XMVectorSet(0.0f, 1.6f, -4.0f, 1.0f);
        const XMVECTOR target = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
        const XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        const XMMATRIX view = XMMatrixLookAtLH(eye, target, up);

        // 近クリップは 0.1m。1m のままだと足元が切れる。
        const XMMATRIX proj = XMMatrixPerspectiveFovLH(
            XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 500.0f);

        XMFLOAT3 eyePos;
        XMStoreFloat3(&eyePos, eye);

        GetRenderer().SetCamera(view, proj, eyePos);

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