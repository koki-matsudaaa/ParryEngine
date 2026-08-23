#include "Engine/Core/Application.h"
#include "Engine/Graphics/FbxModel.h"
#include "Engine/Graphics/Camera.h"
#include "Engine/Combat/ActionStateMachine.h"

#include <cstdio>

using namespace DirectX;
using Engine::ActionPhase;

class GameApp : public Engine::Application
{
    // ── 単位の決め事 ──────────────────────────
    // このプロジェクトは 1 unit = 1 メートル とする。
    // 重力 9.8、歩行 4m/s、パリィ判定の半径 0.5m のように、
    // 現実の数字をそのまま調整値として使えるようにするため。
    static constexpr float kCharacterScale = 0.01f;  // 180  → 1.8m
    static constexpr float kTerrainScale = 20.0f;  //   2  → 40m 四方

    Engine::FbxModel m_terrain;
    Engine::FbxModel m_character;
    Engine::Camera m_camera;
    Engine::ActionStateMachine m_actions;

    bool m_prevAttack = false;  // 押した瞬間を拾うための前フレーム状態

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
            std::printf("Run の読み込みに失敗\n");

        if (!m_character.LoadClip("Slash", "Assets/Model/Bot_Slash.fbx", false))
            std::printf("Slash の読み込みに失敗\n");

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

        // ── アクションの定義 ──
        m_actions.SetAnimator(&m_character.GetAnimator());
        {
            Engine::ActionData slash;
            slash.name = "Slash";
            slash.clipName = "Slash";
            slash.startup = 12;   // 発生まで 12F
            slash.active = 6;    // 判定 6F
            slash.recovery = 24;   // 硬直 24F
            slash.cancelFrom = 30;   // 30F 目からキャンセル可
            m_actions.AddAction(slash);
        }
        std::printf("Z キーで攻撃\n");

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

        // 数字キーでモーションを切り替える。0.15秒かけて移り変わる。
        if (GetAsyncKeyState('1') & 0x8000) m_character.Play("Idle", 0.15f);
        if (GetAsyncKeyState('2') & 0x8000) m_character.Play("Run", 0.15f);
        if (GetAsyncKeyState('3') & 0x8000) m_character.Play("Slash", 0.15f);

        // 攻撃モーションが終わったら待機に戻る。
        if (m_character.CurrentClipName() == "Slash" && m_character.IsFinished())
            m_character.Play("Idle", 0.2f);

        // ── 攻撃 ──
        // 押した瞬間だけ拾う。
        const bool attackHeld = (GetAsyncKeyState('Z') & 0x8000) != 0;
        const bool attackPressed = attackHeld && !m_prevAttack;
        m_prevAttack = attackHeld;

        if (attackPressed && m_actions.CanCancel())
            m_actions.StartAction("Slash");

        // アクションを1フレーム進める。dt を渡さないのが要点。
        const ActionPhase prevPhase = m_actions.Phase();
        m_actions.Step();

        // 段階が変わったときだけ表示する。
        {
            const ActionPhase now = m_actions.Phase();
            if (now != prevPhase)
            {
                const char* names[] = { "----", "発生", "判定", "硬直" };
                std::printf("[%3d F] %s\n",
                    m_actions.ElapsedFrames(), names[(int)now]);
            }
        }

        // 何もしていないなら待機へ戻す。
        if (m_actions.IsIdle() && m_character.CurrentClipName() != "Idle")
            m_character.Play("Idle", 0.2f);

        // 移動モーションの確認用 (アクション中は受け付けない)。
        if (m_actions.IsIdle())
        {
            if (GetAsyncKeyState('1') & 0x8000) m_character.Play("Idle", 0.15f);
            if (GetAsyncKeyState('2') & 0x8000) m_character.Play("Run", 0.15f);
        }
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