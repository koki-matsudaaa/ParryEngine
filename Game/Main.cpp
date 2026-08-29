#include "Engine/Core/Application.h"
#include "Engine/Graphics/FbxModel.h"
#include "Engine/Graphics/Camera.h"
#include "Engine/Combat/ActionStateMachine.h"
#include "Engine/Input/InputBuffer.h"
#include "Engine/Combat/ParrySystem.h"
#include "imgui.h"

#include <cstdio>

using namespace DirectX;
using Engine::ActionPhase;

class GameApp : public Engine::Application
{
    // 1 unit = 1 m
    static constexpr float kCharacterScale = 0.01f; // 180  → 1.8m
    static constexpr float kTerrainScale = 20.0f;   //   2  → 40m 四方

    Engine::FbxModel m_terrain;
    Engine::FbxModel m_character;
    Engine::Camera m_camera;
    Engine::ActionStateMachine m_actions;
    Engine::InputBuffer m_input;
    Engine::FbxModel m_enemy;
    Engine::ActionStateMachine m_enemyActions;
    Engine::ParrySystem m_parry;

    bool m_prevAttack = false;
    bool m_prevParry = false;

    int  m_hitStop = 0;        // 残りの停止フレーム
    int  m_enemyTimer = 0;     // 次の攻撃までの残りフレーム
    int  m_enemyInterval = 90; // 攻撃の間隔

    int  m_parryCount = 0;     // 弾いた回数
    int  m_hitCount = 0;       // 食らった回数

protected:
    bool OnStart() override
    {
        SetClearColor(0.10f, 0.16f, 0.24f);

        if (!LoadTerrain())   return false;
        if (!LoadCharacter()) return false;
        SetupCamera();
        SetupActions();

        std::printf("Z = 攻撃   矢印キー = カメラ\n");
        return true;
    }

private:
    bool LoadTerrain()
    {
        if (!m_terrain.Load("Assets/Model/SnowTerrain.fbx",
            "Assets/Model/grass_texture_01.png"))
        {
            std::printf("地形の読み込みに失敗しました\n");
            return false;
        }
        m_terrain.SetStaticPipeline(true);   // ボーンが無いので Static で描く
        return true;
    }

    bool LoadCharacter()
    {
        // メッシュとスケルトンは1回だけ。この FBX のアニメが "Idle" になる。
        if (!m_character.Load("Assets/Model/Bot_Idle.fbx", "", "Idle"))
        {
            std::printf("キャラクターの読み込みに失敗しました\n");
            return false;
        }

        // モーションだけを追加で読む。メッシュは重複しない。
        m_character.LoadClip("Run", "Assets/Model/Bot_Run.fbx");
        m_character.LoadClip("Slash", "Assets/Model/Bot_Slash.fbx", false);

        m_character.Play("Idle");
        std::printf("クリップ %zu 本 / ボーン %zu 本\n",
            m_character.GetClipCount(), m_character.GetBoneCount());

        // 敵
        if (!m_enemy.Load("Assets/Model/Bot_Idle.fbx", "", "Idle"))
        {
            std::printf("敵の読み込みに失敗しました\n");
            return false;
        }
        m_enemy.LoadClip("Slash", "Assets/Model/Bot_Slash.fbx", false);
        m_enemy.Play("Idle");

        return true;
    }

    void SetupCamera()
    {
        m_camera.SetTarget(XMFLOAT3(0.0f, 0.0f, 0.0f));
        m_camera.SetDistance(4.0f);
    }

    void SetupActions()
    {
        m_actions.SetAnimator(&m_character.GetAnimator());

        Engine::ActionData slash;
        slash.name = "Slash";
        slash.clipName = "Slash";
        slash.startup = 30;
        slash.active = 25;
        slash.recovery = 30;
        slash.cancelFrom = 30;
        slash.cancelTo = { "Slash" };
        m_actions.AddAction(slash);

        m_input.SetWindow(20);

        // ── パリィ ──
        Engine::ActionData parry;
        parry.name = "Parry";
        parry.clipName = "Slash";   // 専用モーションが無いので流用
        parry.isParry = true;
        parry.startup = 2;    // 構えるまで
        parry.active = 7;    // 受付 7F
        parry.recovery = 15;   // 外したときの隙
        m_actions.AddAction(parry);

        // ── 敵の攻撃 ──
        m_enemyActions.SetAnimator(&m_enemy.GetAnimator());

        Engine::ActionData enemySlash;
        enemySlash.name = "EnemySlash";
        enemySlash.clipName = "Slash";
        enemySlash.startup = 40;   // 見てから反応できる長さ
        enemySlash.active = 10;
        enemySlash.recovery = 30;
        m_enemyActions.AddAction(enemySlash);
    }

protected:
    void OnUpdate(float dt) override
    {
        m_character.UpdateAnimation(dt);
        m_enemy.UpdateAnimation(dt);

        // 矢印キーでカメラを回す。
        const float rotSpeed = 2.0f;   // ラジアン/秒
        if (GetAsyncKeyState(VK_LEFT) & 0x8000) m_camera.AddYaw(-rotSpeed * dt);
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000) m_camera.AddYaw(+rotSpeed * dt);
        if (GetAsyncKeyState(VK_UP) & 0x8000) m_camera.AddPitch(-rotSpeed * dt);
        if (GetAsyncKeyState(VK_DOWN) & 0x8000) m_camera.AddPitch(+rotSpeed * dt);

        // アクション（硬直）が終わったら待機へ戻す
        if (m_actions.IsIdle() && m_character.CurrentClipName() != "Idle")
            m_character.Play("Idle", 0.2f);

        // ── 入力 ──
        const bool attackHeld = (GetAsyncKeyState('Z') & 0x8000) != 0;
        if (attackHeld && !m_prevAttack) m_input.Push("Attack");
        m_prevAttack = attackHeld;

        const bool parryHeld = (GetAsyncKeyState('X') & 0x8000) != 0;
        if (parryHeld && !m_prevParry) m_input.Push("Parry");
        m_prevParry = parryHeld;

        // ── ヒットストップ ──
        // 当たった瞬間に世界を止める。止めている間はフレームを進めない。
        // 手応えの大半はこの数フレームで決まる。
        if (m_hitStop > 0)
        {
            m_hitStop--;
            return;
        }

        // ── 行動の発動 ──
        if (m_input.Has("Parry") && m_actions.CanCancelInto("Parry"))
        {
            m_input.Consume("Parry");
            m_actions.StartAction("Parry");
        }
        else if (m_input.Has("Attack") && m_actions.CanCancelInto("Slash"))
        {
            m_input.Consume("Attack");
            m_actions.StartAction("Slash");
        }

        // ── 敵 ──
        // 一定間隔で攻撃を繰り返すだけ。AI は M6 で作る。
        if (m_enemyActions.IsIdle() && --m_enemyTimer <= 0)
        {
            m_enemyActions.StartAction("EnemySlash");
            m_enemyTimer = m_enemyInterval;
        }

        // ── 1フレーム進める ──
        m_actions.Step();
        m_enemyActions.Step();
        m_input.Step();

        // ── 判定 ──
        switch (m_parry.Resolve(m_enemyActions, m_actions))
        {
        case Engine::ParryResult::Success:
            m_hitStop = m_parry.hitStopOnParry;
            m_parryCount++;
            std::printf("弾いた!\n");
            break;

        case Engine::ParryResult::Hit:
            m_hitStop = m_parry.hitStopOnHit;
            m_hitCount++;
            std::printf("被弾\n");
            break;

        default:
            break;
        }

        // ── 待機へ戻す ──
        if (m_actions.IsIdle() && m_character.CurrentClipName() != "Idle")
            m_character.Play("Idle", 0.2f);
        if (m_enemyActions.IsIdle() && m_enemy.CurrentClipName() != "Idle")
            m_enemy.Play("Idle", 0.2f);
    }

    void OnRender(float alpha) override
    {
        (void)alpha;

        GetRenderer().SetCamera(m_camera.View(), m_camera.Projection(), m_camera.Eye());

        //GetRenderer().DrawModel(&m_terrain,
        //    XMMatrixScaling(kTerrainScale, kTerrainScale, kTerrainScale));

        // Player
        GetRenderer().DrawModel(&m_character,
            XMMatrixScaling(kCharacterScale, kCharacterScale, kCharacterScale));

        // 敵。
        GetRenderer().DrawModel(&m_enemy,
            XMMatrixScaling(kCharacterScale, kCharacterScale, kCharacterScale)
            * XMMatrixRotationY(XM_PI)
            * XMMatrixTranslation(0.0f, 0.0f, 1.5f));
    }

    void OnGui() override
    {
        ImGui::Begin("アクション調整");

        ImGui::Text("Z キーで攻撃");
        ImGui::Separator();

        // 単位はすべてフレーム (1/60秒)。ここを動かすと即座に効く。
        for (auto& a : m_actions.Actions())
        {
            ImGui::PushID(a.name.c_str());
            ImGui::Text("[ %s ]", a.name.c_str());

            ImGui::SliderInt("発生", &a.startup, 0, 60);
            ImGui::SliderInt("判定", &a.active, 1, 60);
            ImGui::SliderInt("硬直", &a.recovery, 0, 90);
            ImGui::SliderInt("キャンセル", &a.cancelFrom, -1, 90);

            ImGui::Text("合計 %d F (%.2f 秒)",
                a.TotalFrames(), a.TotalFrames() / 60.0f);
            ImGui::Separator();
            ImGui::PopID();
        }

        // 先行入力の受付幅。手触りが一番変わる数字。
        int window = m_input.Window();
        if (ImGui::SliderInt("先行入力の受付", &window, 0, 30))
            m_input.SetWindow(window);

        ImGui::Separator();

        // 今の進行状況。
        ImGui::Text("いま : %s   %d F",
            Engine::ToString(m_actions.Phase()), m_actions.ElapsedFrames());

        ImGui::Separator();
        ImGui::Text("敵 : %s  %d F",
            Engine::ToString(m_enemyActions.Phase()),
            m_enemyActions.ElapsedFrames());

        ImGui::SliderInt("敵の攻撃間隔", &m_enemyInterval, 30, 240);
        ImGui::SliderInt("弾き時の停止", &m_parry.hitStopOnParry, 0, 30);
        ImGui::SliderInt("被弾時の停止", &m_parry.hitStopOnHit, 0, 30);

        ImGui::Separator();
        ImGui::Text("弾いた %d 回 / 食らった %d 回", m_parryCount, m_hitCount);
        if (ImGui::Button("記録をリセット")) { m_parryCount = 0; m_hitCount = 0; }

        ImGui::End();
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