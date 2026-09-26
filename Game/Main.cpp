#include "Engine/Core/Application.h"
#include "Engine/Graphics/FbxModel.h"
#include "Engine/Graphics/Camera.h"
#include "Engine/Combat/ActionStateMachine.h"
#include "Engine/Input/InputBuffer.h"
#include "Engine/Combat/ParrySystem.h"
#include "Engine/Combat/Posture.h"
#include "Engine/Combat/ActionFile.h"
#include "Engine/Tools/Timeline.h"
#include "Engine/Tools/TimelineRecorder.h"
#include "Engine/Tools/ActionEditor.h"
#include "Engine/Combat/AttackSelect.h"
#include "Engine/Core/Transform.h"

#include "imgui.h"

#include <cstdio>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace DirectX;
using Engine::ActionPhase;

class GameApp : public Engine::Application
{
    static constexpr float kCharacterScale = 0.01f;
    static constexpr float kTerrainScale = 1.0f;
    static constexpr const char* kPlayerActionFile = "Assets/Data/PlayerActions.txt";
    static constexpr const char* kEnemyActionFile = "Assets/Data/EnemyActions.txt";

    Engine::FbxModel m_terrain;
    Engine::FbxModel m_sky;
    Engine::FbxModel m_character;
    Engine::Camera m_camera;
    Engine::ActionStateMachine m_actions;
    Engine::InputBuffer m_input;
    Engine::ParrySystem m_parry;
    Engine::Posture m_playerPosture;
    Engine::Transform m_playerTf;

    Engine::FbxModel m_enemy;
    Engine::ActionStateMachine m_enemyActions;
    Engine::Posture m_enemyPosture;
    Engine::AttackSelect m_enemyAttacks;
    Engine::TimelineRecorder m_playerTrack;
    Engine::TimelineRecorder m_enemyTrack;
    Engine::Transform m_enemyTf;

    bool m_showTools = true;  

    bool m_prevAttack = false;
    bool m_prevParry = false;

    int  m_hitStop = 0;        // 残りの停止フレーム
    int  m_enemyTimer = 0;     // 次の攻撃までの残りフレーム
    int  m_enemyInterval = 90; // 攻撃の間隔

    int  m_parryCount = 0;     // 弾いた回数
    int  m_hitCount = 0;       // 食らった回数
    int  m_enemyHitCount = 0;  // 敵を斬った回数

    float m_zoom = 6.0f;       // タイムラインの横倍率
    float m_trackZoom = 3.0f;        // 履歴の横倍率
    bool  m_pauseOnParry = false;    // 弾いた瞬間に記録を止める
    int   m_lastParryOffset = -1;    // 受付の何F目で成立したか

    // 位置と向き
    float m_moveSpeed = 3.5f;       // 歩く速さ
    float m_turnSpeed = 12.0f;      // 振り向く速さ

    bool  m_lockOn = false;          // ロックオン中か
    bool  m_prevLockKey = false;
    float m_lockCameraSpeed = 8.0f;  // カメラの振り向き速度

    // ロックオン中のカメラ
    float m_lockDistance = 5.0f;     // 引き
    float m_lockHeight = 1.4f;       // 見る高さ
    float m_lockSide = 0.6f;         // 肩越しのずらし
    float m_lockPitch = 0.30f;       // 見下ろし角
    float m_lockTargetBias = 0.25f;  // 注視点を敵側へ寄せる割合

    // 通常時のカメラ
    float m_freeDistance = 4.0f;
    float m_freeHeight = 1.2f;

    float m_enemySpeed = 2.0f;       // 敵の速さ
    float m_enemyKeepRange = 1.8f;   // 敵の攻撃範囲
    float m_enemyChaseRange = 2.4f;  // 追跡開始する距離
    bool  m_enemyChasing = false;

    bool m_prevDodge = false;
    XMFLOAT3 m_actionMove{ 0.0f, 0.0f, 0.0f };   // アクション中に進む向き

protected:
    bool OnStart() override
    {
        SetClearColor(0.10f, 0.16f, 0.24f);

        if (!LoadTerrain())   return false;
        if (!LoadCharacter()) return false;
        SetupCamera();
        SetupActions();
        LoadActionFiles();

        m_enemyTf.position = XMFLOAT3(0.0f, 0.0f, 1.5f);

        // 履歴をためる
        m_playerTrack.SetCapacity(240);
        m_enemyTrack.SetCapacity(240);

        std::printf("Z = 攻撃   矢印キー = カメラ\n");
        return true;
    }

private:
    bool LoadTerrain()
    {
        m_terrain.SetMeshFilter("Plane");
        if (!m_terrain.Load("Assets/Model/SnowTerrain.fbx",
            "Assets/Model/grass_texture_01.png"))
        {
            std::printf("地面の読み込みに失敗しました\n");
            return false;
        }
        m_terrain.SetStaticPipeline(true);

        m_sky.SetMeshFilter("Sphere");
        if (!m_sky.Load("Assets/Model/SnowTerrain.fbx",
            "Assets/Model/blue_sky.png"))
        {
            std::printf("空の読み込みに失敗しました\n");
            return false;
        }
        m_sky.SetStaticPipeline(true);
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

        // モーションだけを追加で読む
        m_character.LoadClip("Run", "Assets/Model/Bot_Run.fbx");
        m_character.LoadClip("Slash", "Assets/Model/Bot_Slash.fbx", false);
        m_character.LoadClip("Parry", "Assets/Model/Bot_Block.fbx", false);
        m_character.LoadClip("StrafeL", "Assets/Model/Bot_StrafeL.fbx");
        m_character.LoadClip("StrafeR", "Assets/Model/Bot_StrafeR.fbx");
        m_character.LoadClip("RunBack", "Assets/Model/Bot_RunBack.fbx");
        m_character.LoadClip("Dodge", "Assets/Model/Bot_Dodge.fbx", false);
        m_character.LoadClip("Guard", "Assets/Model/Bot_Guard.fbx");


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
        m_enemy.LoadClip("Deflected", "Assets/Model/Bot_Deflected.fbx", false);
        m_enemy.LoadClip("Break", "Assets/Model/Bot_Break.fbx", false);
        m_enemy.LoadClip("Run", "Assets/Model/Bot_Run.fbx");
        m_enemy.LoadClip("Slash2", "Assets/Model/Bot_Slash2.fbx", false);
        m_enemy.LoadClip("Slash3", "Assets/Model/Bot_Slash3.fbx", false);
        m_enemy.LoadClip("Thrust", "Assets/Model/Bot_Thrust.fbx", false);
        m_enemy.LoadClip("Heavy", "Assets/Model/Bot_Heavy.fbx", false);
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
        slash.cancelTo = { "Slash", "Parry" };
        m_actions.AddAction(slash);

        m_input.SetWindow(20);

        // パリィ
        Engine::ActionData parry;
        parry.name = "Parry";
        parry.clipName = "Parry";
        parry.isParry = true;
        parry.startup = 2;    // 構えるまで
        parry.active = 15;    // 受付 7F
        parry.recovery = 15;  // 外したときの隙
        m_actions.AddAction(parry);

        // 回避
        Engine::ActionData dodge;
        dodge.name = "Dodge";
        dodge.clipName = "Dodge";
        dodge.blendSeconds = 0.05f;
        dodge.isDodge = true;
        dodge.startup = 3;    // 回避するまで
        dodge.active = 14;    // 無敵
        dodge.recovery = 12;  // 後隙
        dodge.moveSpeed = 7.0f;
        m_actions.AddAction(dodge);

        // 敵の攻撃
        m_enemyActions.SetAnimator(&m_enemy.GetAnimator());

        Engine::ActionData enemySlash;
        enemySlash.name = "EnemySlash";
        enemySlash.clipName = "Slash";
        enemySlash.startup = 40;   // 見てから反応できる長さ
        enemySlash.active = 10;
        enemySlash.recovery = 30;
        enemySlash.aiWeight = 1;
        m_enemyActions.AddAction(enemySlash);

        // 弾かれた方
        Engine::ActionData deflected;
        deflected.name = "Deflected";
        deflected.clipName = "Deflected";
        deflected.startup = 0;
        deflected.active = 0;
        deflected.recovery = 25;
        m_enemyActions.AddAction(deflected);
    }

    static void Apply(Engine::ActionStateMachine& sm,
        const std::vector<Engine::ActionData>& list)
    {
        sm.Clear();
        for (const auto& a : list) sm.AddAction(a);
    }

    void LoadActionFiles()
    {
        std::vector<Engine::ActionData> list;

        if (Engine::LoadActions(list, kPlayerActionFile))
        {
            Apply(m_actions, list);
            std::printf("プレイヤーのアクションを読み込みました\n");
        }
        if (Engine::LoadActions(list, kEnemyActionFile))
        {
            Apply(m_enemyActions, list);
            std::printf("敵のアクションを読み込みました\n");
        }
    }

    void SaveActionFiles()
    {
        Engine::SaveActions(m_actions.Actions(), kPlayerActionFile);
        Engine::SaveActions(m_enemyActions.Actions(), kEnemyActionFile);
        std::printf("アクションを保存しました\n");
    }

    // プレイヤーから見た敵の方向
    float YawToEnemy() const
    {
        const float dx = m_enemyTf.position.x - m_playerTf.position.x;
        const float dz = m_enemyTf.position.z - m_playerTf.position.z;
        return std::atan2(dx, dz);
    }

    // プレイヤーと敵の距離
    float DistanceToEnemy() const
    {
        const float dx = m_enemyTf.position.x - m_playerTf.position.x;
        const float dz = m_enemyTf.position.z - m_playerTf.position.z;
        return std::sqrt(dx * dx + dz * dz);
    }

    // モーションを止めるかどうか
    static float StaggerDt(const Engine::ActionStateMachine& sm, float dt)
    {
        if (sm.Stagger() <= 0) return dt;

        const auto* a = sm.CurrentAction();
        return (a && !a->deflectClip.empty()) ? dt : 0.0f;
    }

    // カメラ基準で方向を作る
    bool ReadMoveInput(float& outX, float& outZ) const
    {
        float ix = 0.0f;
        float iz = 0.0f;
        if (GetAsyncKeyState('W') & 0x8000) iz += 1.0f;
        if (GetAsyncKeyState('S') & 0x8000) iz -= 1.0f;
        if (GetAsyncKeyState('D') & 0x8000) ix += 1.0f;
        if (GetAsyncKeyState('A') & 0x8000) ix -= 1.0f;

        if (ix == 0.0f && iz == 0.0f) return false;

        // カメラの向きから「前」と「右」を作る
        const float cy = m_camera.GetYaw();
        const float fx = std::sin(cy), fz = std::cos(cy);
        const float rx = std::cos(cy), rz = -std::sin(cy);

        float mx = fx * iz + rx * ix;
        float mz = fz * iz + rz * ix;

        // 正規化
        const float len = std::sqrt(mx * mx + mz * mz);
        outX = mx / len;
        outZ = mz / len;
        return true;
    }

    // 移動
    void UpdateMovement(float dt)
    {
        if (!m_actions.IsIdle()) return;

        // ロック中は、止まっていても敵の方を向き続ける
        if (m_lockOn)
            m_playerTf.TurnTowards(YawToEnemy(), m_turnSpeed * dt);

        float mx = 0.0f, mz = 0.0f;
        if (!ReadMoveInput(mx, mz))
        {
            if (m_character.CurrentClipName() != "Idle")
                m_character.Play("Idle", 0.2f);
            return;
        }

        m_playerTf.position.x += mx * m_moveSpeed * dt;
        m_playerTf.position.z += mz * m_moveSpeed * dt;

        // ロックしていないときは、進む方向を向く
        if (!m_lockOn)
            m_playerTf.TurnTowards(std::atan2(mx, mz), m_turnSpeed * dt);

        // ロック中　正面からどれだけずれているかで判断
        const char* clip = "Run";
        if (m_lockOn)
        {
            const float rel = Engine::AngleDiff(m_playerTf.yaw, std::atan2(mx, mz));
            if (rel > XM_PIDIV4 * 3 || rel < -XM_PIDIV4 * 3) 
                clip = "RunBack";
            else if (rel > XM_PIDIV4) 
                clip = "StrafeR";
            else if (rel < -XM_PIDIV4) 
                clip = "StrafeL";
        }

        if (m_character.CurrentClipName() != clip)
            m_character.Play(clip, 0.15f);
    }

    // 回避する向き
    void SetDodgeDirection()
    {
        float mx = 0.0f, mz = 0.0f;
        if (ReadMoveInput(mx, mz))
        {
            m_actionMove = XMFLOAT3(mx, 0.0f, mz);
            if (!m_lockOn) m_playerTf.yaw = std::atan2(mx, mz);
            return;
        }

        // 入力なし
        const XMFLOAT3 f = m_playerTf.Forward();
        m_actionMove = XMFLOAT3(-f.x, 0.0f, -f.z);
    }

    // アクションが自分から進む分を動かす
    void UpdateActionMove(float dt)
    {
        const Engine::ActionData* a = m_actions.CurrentAction();
        if (!a || a->moveSpeed <= 0.0f) return;
        if (m_actions.Phase() == ActionPhase::Recovery) return;

        m_playerTf.position.x += m_actionMove.x * a->moveSpeed * dt;
        m_playerTf.position.z += m_actionMove.z * a->moveSpeed * dt;
    }

    // 敵の行動
    void UpdateEnemy(float dt)
    {
        if (m_enemyPosture.IsBroken()) return;   // 崩壊中は動かない
        if (!m_enemyActions.IsIdle())   return;  // 何かしている最中は動かない

        const float dist = DistanceToEnemy();

        // ガクガク防ぎ　仮
        if (dist > m_enemyChaseRange)       m_enemyChasing = true;
        else if (dist <= m_enemyKeepRange)  m_enemyChasing = false;

        if (m_enemyChasing)
        {
            // 間合いの外
            const XMFLOAT3 f = m_enemyTf.Forward();
            m_enemyTf.position.x += f.x * m_enemySpeed * dt;
            m_enemyTf.position.z += f.z * m_enemySpeed * dt;

            if (m_enemy.CurrentClipName() != "Run")
                m_enemy.Play("Run", 0.15f);

            // 待ち時間を戻す
            m_enemyTimer = m_enemyInterval;
            return;
        }

        if (m_enemy.CurrentClipName() != "Idle")
            m_enemy.Play("Idle", 0.2f);

        if (--m_enemyTimer <= 0)
        {
            const std::string next = m_enemyAttacks.Pick(m_enemyActions.Actions());
            if (!next.empty()) m_enemyActions.StartAction(next);
            m_enemyTimer = m_enemyInterval;
        }
    }

protected:
    void OnUpdate(float dt) override
    {
        m_character.UpdateAnimation(StaggerDt(m_actions, dt));
        m_enemy.UpdateAnimation(StaggerDt(m_enemyActions, dt));

        // 矢印キーでカメラを回す
        const float rotSpeed = 2.0f;   // ラジアン/秒
        if (!m_lockOn)
        {
            if (GetAsyncKeyState(VK_LEFT) & 0x8000)  m_camera.AddYaw(-rotSpeed * dt);
            if (GetAsyncKeyState(VK_RIGHT) & 0x8000) m_camera.AddYaw(+rotSpeed * dt);
        }
        if (GetAsyncKeyState(VK_UP) & 0x8000)   m_camera.AddPitch(-rotSpeed * dt);
        if (GetAsyncKeyState(VK_DOWN) & 0x8000) m_camera.AddPitch(+rotSpeed * dt);

        // ロック中
        if (m_lockOn)
        {
            const float diff = Engine::AngleDiff(m_camera.GetYaw(), YawToEnemy());
            const float rate = std::min(1.0f, m_lockCameraSpeed * dt);
            m_camera.AddYaw(diff * rate);

            m_camera.SetPitch(m_camera.GetPitch()
                + (m_lockPitch - m_camera.GetPitch()) * rate);
        }

        // 入力
        const bool attackHeld = (GetAsyncKeyState('Z') & 0x8000) != 0;
        if (attackHeld && !m_prevAttack) m_input.Push("Attack");
        m_prevAttack = attackHeld;

        const bool guardHeld = (GetAsyncKeyState('X') & 0x8000) != 0;
        m_actions.SetHold(guardHeld);

        const bool parryHeld = (GetAsyncKeyState('C') & 0x8000) != 0;
        if (parryHeld && !m_prevParry) m_input.Push("Parry");
        m_prevParry = parryHeld;

        const bool dodgeHeld = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0;
        if (dodgeHeld && !m_prevDodge) m_input.Push("Dodge");
        m_prevDodge = dodgeHeld;

        const bool lockHeld = (GetAsyncKeyState('Q') & 0x8000) != 0;
        if (lockHeld && !m_prevLockKey) m_lockOn = !m_lockOn;
        m_prevLockKey = lockHeld;

        // ヒットストップ
        if (m_hitStop > 0)
        {
            m_hitStop--;
            return;
        }

        // 行動の発動
        if (m_input.Has("Dodge") && m_actions.CanCancelInto("Dodge"))
        {
            m_input.Consume("Dodge");
            SetDodgeDirection();          // 向きは技を出す前に決める
            m_actions.StartAction("Dodge");
        }
        else if (m_input.Has("Parry") && m_actions.CanCancelInto("Parry"))
        {
            m_input.Consume("Parry");
            m_actions.StartAction("Parry");
            if (m_lockOn) m_playerTf.yaw = YawToEnemy();
        }
        else if (m_input.Has("Attack") && m_actions.CanCancelInto("Slash"))
        {
            m_input.Consume("Attack");
            m_actions.StartAction("Slash");
            if (m_lockOn) m_playerTf.yaw = YawToEnemy();
        }
        else if (guardHeld && m_actions.IsIdle())
        {
            m_actions.StartAction("Guard");
            if (m_lockOn) m_playerTf.yaw = YawToEnemy();
        }

        // 移動
        UpdateMovement(dt);
        UpdateActionMove(dt);

        // 敵はいつもプレイヤーの方を向く
        m_enemyTf.yaw = std::atan2(
            m_playerTf.position.x - m_enemyTf.position.x,
            m_playerTf.position.z - m_enemyTf.position.z);

        UpdateEnemy(dt);

        // 1フレーム進める
        m_actions.Step();
        m_enemyActions.Step();
        m_input.Step();
        m_playerPosture.Step();
        m_enemyPosture.Step();

        // 進めた後の状態を記録する
        m_playerTrack.Record(m_actions);
        m_enemyTrack.Record(m_enemyActions);

        const float dist = DistanceToEnemy();

        // 判定　敵からプレイヤー
        switch (m_parry.Resolve(m_enemyActions, m_actions, dist))
        {
        case Engine::ParryResult::Success:
        {
            m_hitStop = m_parry.hitStopOnParry;
            m_parryCount++;

            m_playerTrack.MarkEvent(Engine::TimelineEvent::ParrySuccess);
            m_enemyTrack.MarkEvent(Engine::TimelineEvent::ParrySuccess);

            // 受付の何フレーム目で取れたか
            if (const auto* a = m_actions.CurrentAction())
                m_lastParryOffset = m_actions.ElapsedFrames() - a->startup;

            // 弾いた瞬間から１コマずつ進める
            if (m_pauseOnParry) Clock().SetPaused(true);

            if (m_enemyPosture.Add(m_parry.parryPostureDamage))
            {
                // 崩壊。ここは連撃も打ち切る
                m_enemyActions.Cancel();
                m_enemy.Play("Break", 0.15f);
                std::printf("弾いた! → 体幹崩壊!\n");
            }
            else
            {
                if (const auto* a = m_enemyActions.CurrentAction())
                {
                    m_enemyActions.AddStagger(a->deflectFrames);

                    // モーションがある場合  
                    if (!a->deflectClip.empty())
                        m_enemy.Play(a->deflectClip, 0.05f);
                }

                std::printf("弾いた!  (敵の体幹 %.0f / %.0f)\n",
                    m_enemyPosture.Value(), m_enemyPosture.Max());
            }
            break;
        }
        case Engine::ParryResult::Hit:
        {
            m_hitStop = m_parry.hitStopOnHit;
            m_hitCount++;

            m_playerTrack.MarkEvent(Engine::TimelineEvent::Hit);
            m_enemyTrack.MarkEvent(Engine::TimelineEvent::Hit);

            // この攻撃の威力を、攻撃側のアクションから引く
            float dmg = 20.0f;
            if (const auto* a = m_enemyActions.CurrentAction())
                dmg = a->postureDamage;

            if (m_playerPosture.Add(dmg))
                std::printf("被弾 → こちらの体幹崩壊!\n");
            else
                std::printf("被弾  (自分の体幹 %.0f / %.0f)\n",
                    m_playerPosture.Value(), m_playerPosture.Max());
            break;
        }
        case Engine::ParryResult::Guarded:
        {
            m_hitStop = m_parry.hitStopOnGuard;

            float dmg = 20.0f;
            if (const auto* a = m_enemyActions.CurrentAction())
                dmg = a->postureDamage;

            m_playerPosture.Add(dmg * m_parry.guardPostureRate);
            std::printf("ガード  (自分の体幹 %.0f / %.0f)\n",
                m_playerPosture.Value(), m_playerPosture.Max());
            break;
        }
        default:
            break;
        }

        // 判定　プレイヤーから敵
        switch (m_parry.Resolve(m_actions, m_enemyActions, dist))
        {
        case Engine::ParryResult::Success:
        {
            // 敵に弾かれた
            m_hitStop = m_parry.hitStopOnParry;
            m_playerPosture.Add(m_parry.parryPostureDamage);

            int frames = 0;
            if (const auto* a = m_actions.CurrentAction())
                frames = a->deflectFrames;
            m_actions.AddStagger(frames);

            std::printf("弾かれた!\n");
            break;
        }
        case Engine::ParryResult::Hit:
        {
            m_hitStop = m_parry.hitStopOnHit;
            m_enemyHitCount++;

            m_playerTrack.MarkEvent(Engine::TimelineEvent::Hit);
            m_enemyTrack.MarkEvent(Engine::TimelineEvent::Hit);

            float dmg = 20.0f;
            if (const auto* a = m_actions.CurrentAction())
                dmg = a->postureDamage;

            if (m_enemyPosture.Add(dmg))
            {
                m_enemyActions.Cancel();
                m_enemy.Play("Break", 0.15f);
                std::printf("斬った → 敵の体幹崩壊!\n");
            }
            else
            {
                // 怯み
                m_enemyActions.StartAction("Flinch");
                std::printf("斬った  (敵の体幹 %.0f / %.0f)\n",
                    m_enemyPosture.Value(), m_enemyPosture.Max());
            }
            break;
        }
        default:
            break;
        }

    }

    void OnRender(float alpha) override
    {
        (void)alpha;

        // カメラ
        if (m_lockOn)
        {
            XMFLOAT3 t = m_playerTf.position;
            t.x += (m_enemyTf.position.x - t.x) * m_lockTargetBias;
            t.z += (m_enemyTf.position.z - t.z) * m_lockTargetBias;

            m_camera.SetTarget(t);
            m_camera.SetDistance(m_lockDistance);
            m_camera.lookHeight = m_lockHeight;
            m_camera.sideOffset = m_lockSide;
        }
        else
        {
            m_camera.SetTarget(m_playerTf.position);
            m_camera.SetDistance(m_freeDistance);
            m_camera.lookHeight = m_freeHeight;
            m_camera.sideOffset = 0.0f;
        }

        GetRenderer().SetCamera(m_camera.View(), m_camera.Projection(), m_camera.Eye());

        // 空
        GetRenderer().DrawModel(&m_sky,
            XMMatrixScaling(kTerrainScale, kTerrainScale, kTerrainScale));

        // 地面
        GetRenderer().DrawModel(&m_terrain,
            XMMatrixScaling(kTerrainScale, kTerrainScale, kTerrainScale));

        GetRenderer().DrawModel(&m_character, m_playerTf.World(kCharacterScale));
        GetRenderer().DrawModel(&m_enemy, m_enemyTf.World(kCharacterScale));
    }

    void OnGui() override
    {
        // --- コマ送り ---
        Engine::FixedTimestep& clock = Clock();
        if (!ImGui::GetIO().WantCaptureKeyboard)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_F1, false))
                m_showTools = !m_showTools;

            if (ImGui::IsKeyPressed(ImGuiKey_Space, false))
                clock.SetPaused(!clock.IsPaused());

            if (clock.IsPaused() && ImGui::IsKeyPressed(ImGuiKey_N, true))
                clock.RequestSingleStep();
        }

        if (!m_showTools) return;

        const ImVec2 screen = ImGui::GetMainViewport()->WorkSize;
        const float panelW = 420.0f;      // 幅
        const float timelineH = 180.0f;   // 高さ

        ImGui::SetNextWindowPos(ImVec2(screen.x - panelW, 0.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(panelW, screen.y), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.88f);
        ImGui::Begin("アクション調整");

        if (ImGui::Button("保存")) SaveActionFiles();
        ImGui::SameLine();
        if (ImGui::Button("読み込み")) LoadActionFiles();
        ImGui::SameLine();
        ImGui::TextDisabled("F1 で表示切替");

        ImGui::TextDisabled("WASD 移動 / Z 攻撃 / X ガード / C パリィ / Shift 回避 / Q ロック");
        ImGui::Separator();

        if (ImGui::BeginTabBar("tabs"))
        {
            if (ImGui::BeginTabItem("プレイヤー"))
            {
                ImGui::SliderFloat("拡大", &m_zoom, 2.0f, 16.0f, "%.1f px/F");
                Engine::DrawFrameRuler(120, m_zoom);
                ImGui::Separator();

                Engine::DrawActionEditor(m_actions, m_zoom);

                ImGui::Separator();

                // 先行入力の受付
                int window = m_input.Window();
                if (ImGui::SliderInt("先行入力の受付", &window, 0, 30))
                    m_input.SetWindow(window);

                ImGui::SliderFloat("歩く速さ", &m_moveSpeed, 1.0f, 8.0f);
                ImGui::SliderFloat("振り向く速さ", &m_turnSpeed, 2.0f, 30.0f);
                ImGui::Checkbox("ロックオン (Q)", &m_lockOn);
                ImGui::SliderFloat("ロック時のカメラ", &m_lockCameraSpeed, 1.0f, 20.0f);
                if (ImGui::TreeNode("ロック中のカメラ位置"))
                {
                    ImGui::SliderFloat("引き", &m_lockDistance, 2.0f, 10.0f, "%.1f m");
                    ImGui::SliderFloat("見る高さ", &m_lockHeight, 0.5f, 3.0f, "%.2f m");
                    ImGui::SliderFloat("肩越し", &m_lockSide, -2.0f, 2.0f, "%.2f m");
                    ImGui::SliderFloat("見下ろし角", &m_lockPitch,
                        m_camera.minPitch, m_camera.maxPitch, "%.2f");
                    ImGui::SliderFloat("敵へ寄せる", &m_lockTargetBias, 0.0f, 0.8f, "%.2f");
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("通常のカメラ"))
                {
                    ImGui::SliderFloat("引き##free", &m_freeDistance, 2.0f, 10.0f, "%.1f m");
                    ImGui::SliderFloat("見る高さ##free", &m_freeHeight, 0.5f, 3.0f, "%.2f m");
                    ImGui::TreePop();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("敵"))
            {
                Engine::DrawFrameRuler(120, m_zoom);
                ImGui::Separator();

                Engine::DrawActionEditor(m_enemyActions, m_zoom);

                ImGui::Separator();
                ImGui::SliderInt("敵の攻撃間隔", &m_enemyInterval, 30, 240);
                ImGui::Text("敵との距離 %.2f m", DistanceToEnemy());
                ImGui::SliderFloat("敵の歩く速さ", &m_enemySpeed, 0.5f, 6.0f);
                ImGui::SliderFloat("敵が詰める距離", &m_enemyKeepRange, 0.5f, 5.0f);
                ImGui::SliderFloat("敵が追い始める距離", &m_enemyChaseRange, 0.5f, 6.0f);

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("戦闘"))
            {
                // 今の進行状況
                ImGui::Text("自分 : %s   %d F",
                    Engine::ToString(m_actions.Phase()), m_actions.ElapsedFrames());
                ImGui::Text("敵   : %s   %d F",
                    Engine::ToString(m_enemyActions.Phase()),
                    m_enemyActions.ElapsedFrames());

                ImGui::Separator();
                ImGui::Text("体幹");

                ImGui::Text("自分");
                ImGui::SameLine();
                ImGui::ProgressBar(m_playerPosture.Ratio(), ImVec2(-1.0f, 0.0f),
                    m_playerPosture.IsBroken() ? "崩壊" : "");

                ImGui::Text("敵  ");
                ImGui::SameLine();
                ImGui::ProgressBar(m_enemyPosture.Ratio(), ImVec2(-1.0f, 0.0f),
                    m_enemyPosture.IsBroken() ? "崩壊" : "");

                ImGui::Separator();
                ImGui::SliderInt("弾き時の停止", &m_parry.hitStopOnParry, 0, 30);
                ImGui::SliderInt("被弾時の停止", &m_parry.hitStopOnHit, 0, 30);
                ImGui::SliderFloat("弾き1回の体幹", &m_parry.parryPostureDamage, 5.0f, 60.0f);
                ImGui::SliderFloat("体幹の自然回復", &m_enemyPosture.regenPerFrame, 0.0f, 1.0f);
                ImGui::SliderInt("回復までの待ち", &m_enemyPosture.regenDelayFrames, 0, 180);
                ImGui::SliderInt("崩壊の長さ", &m_enemyPosture.breakFrames, 30, 300);
                ImGui::SliderInt("ガード時の停止", &m_parry.hitStopOnGuard, 0, 30);
                ImGui::SliderFloat("ガードで通る割合", &m_parry.guardPostureRate, 0.0f, 1.0f);

                ImGui::Separator();
                ImGui::Text("弾いた %d / 食らった %d / 斬った %d",
                    m_parryCount, m_hitCount, m_enemyHitCount);
                if (ImGui::Button("記録をリセット"))
                {
                    m_parryCount = 0; m_hitCount = 0; m_enemyHitCount = 0;
                }

                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(0.0f, screen.y - timelineH), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(screen.x - panelW, timelineH), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.88f);
        ImGui::Begin("タイムライン");

        if (ImGui::Button(clock.IsPaused() ? "再開 (Space)" : "一時停止 (Space)"))
            clock.SetPaused(!clock.IsPaused());

        ImGui::SameLine();
        ImGui::BeginDisabled(!clock.IsPaused());
        if (ImGui::Button("1コマ進める (N)")) clock.RequestSingleStep();
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::Text("%llu F", clock.FrameCount());
        ImGui::Separator();

        Engine::DrawPhaseLegend();
        ImGui::SliderFloat("拡大", &m_trackZoom, 1.0f, 8.0f, "%.1f px/F");
        ImGui::Checkbox("弾いたら止める", &m_pauseOnParry);

        if (ImGui::Button("消去")) { m_playerTrack.Clear(); m_enemyTrack.Clear(); }

        ImGui::Text("自分");
        Engine::DrawRecordedTrack(m_playerTrack, m_trackZoom);

        ImGui::Text("敵  ");
        Engine::DrawRecordedTrack(m_enemyTrack, m_trackZoom);

        ImGui::Separator();
        if (m_lastParryOffset >= 0)
            ImGui::Text("直前の成立: 受付の %d F目", m_lastParryOffset);
        else
            ImGui::TextDisabled("直前の成立: まだ無し");

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