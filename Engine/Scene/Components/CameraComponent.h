#pragma once
#include "pch.h"
#include "GameObject.h"
#include "TerrainComponent.h"

class CameraComponent : public GameComponent
{
private:
    XMFLOAT3 m_eye{ 0.0f, 0.0f, 0.0f };
    XMFLOAT3 m_target{ 0.0f, 0.0f, 0.0f };
    bool     m_initialized = false;
    bool m_boosting = false;
    TerrainComponent* m_terrain = nullptr; // 地形 (カメラのめり込み防止用)

    // カメラの回り込み角 (機体の向きとは独立。右スティックで動かす)
    float m_yaw = 0.0f;   // 水平角 (機体の周りをぐるっと)
    float m_pitch = 0.4f; // 見下ろし角 (0=真横, 大きいほど上から見下ろす)

    // ── ロックオン ──
    bool     m_hasLockTarget = false;   // ロック対象がいるか
    XMFLOAT3 m_lockTargetPos{ 0,0,0 };  // ロック対象の座標 (main から毎フレーム渡す)

    // ロック具合 0..1。ON/OFF を直接使うと解除の瞬間に注視点が飛ぶので、
    // なめらかに渡り歩かせる。解除後しばらくは最後の敵座標へ寄せたままになる。
    float m_lockBlend = 0.0f;

    static XMFLOAT3 Lerp(const XMFLOAT3& a, const XMFLOAT3& b, float t)
    {
        return XMFLOAT3(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t);
    }

public:
    // 調整パラメータ
    float backDistance = 120.0f;  // 機体からカメラまでの距離
    float lookAtHeight = 40.0f;  // 注視点を機体のどれだけ上にするか
    // ロック中に落ち着く見下ろし角。上下の入力を切るかわりに、この角度へ戻す。
    float lockPitch = 0.35f;

    // リセット時に戻す見下ろし角 (m_pitch の初期値と同じにしておく)。
    float defaultPitch = 0.4f;

    // 追従の機敏さ。水平と上下で分ける。
    float sharpnessH = 5.0f;     // 水平(XZ)の追従。しっかり追う。
    float sharpnessV = 4.0f;     // 上下(Y)の追従。ゆるく追う(ジャンプで機体が上に行く)。
    float sharpnessHBoost = 1.5f;

    // カメラ回転の速さ (右スティック感度)
    float camRotSpeedYaw = 1.0f;
    float camRotSpeedPitch = 0.5f;

    // 右スティック等からカメラ回転入力を受ける。dtは呼び出し側で掛けても、
    // ここで掛けてもよいが、ここでは生の-1..1を受けて内部でdt管理はしない
    // 簡易版とし、Updateのdtで角度を進める。
    float m_inputYaw = 0.0f;   // -1..1
    float m_inputPitch = 0.0f; // -1..1
    void SetCameraInput(float yawInput, float pitchInput)
    {
        m_inputYaw = yawInput;
        m_inputPitch = pitchInput;
    }

    const XMFLOAT3& GetEye() const { return m_eye; }
    const XMFLOAT3& GetTarget() const { return m_target; }

    // カメラの水平角 (移動をカメラ基準にするため、移動側が読む)
    float GetYaw() const { return m_yaw; }

    void SetBoosting(bool on) { m_boosting = on; }

    // ── ロックオン対象の座標を渡す (main から毎フレーム) ──
    // 対象がいる時に呼ぶ。呼ばれなかったフレームは ClearLockTarget() で解除する。
    void SetLockTarget(const XMFLOAT3& pos)
    {
        m_hasLockTarget = true;
        m_lockTargetPos = pos;
    }
    void ClearLockTarget() { m_hasLockTarget = false; }

    // ロック中の注視点の敵寄り具合 (0=自機, 1=敵)。
    float lockLookBias = 0.7f;   // 水平。自機も画面に残したいので少し手前
    float lockLookBiasY = 1.0f;  // 上下。1.0にすると敵の高さを見るので、
    // ジャンプしても敵が画面内で上下に動かない

// 地形をセット (カメラが地形にめり込まないようにする)
    void SetTerrain(TerrainComponent* terrain) { m_terrain = terrain; }
    float terrainClearance = 5.0f; // 地形からこれだけ上を最低ラインにする

    bool Update(float dt) override
    {
        Transform& tf = Owner()->transform;
        const XMFLOAT3& p = tf.position;

        // 右スティック入力でカメラ角度を進める。
        // ロック中は向きをロックに任せるので、上下の入力は受け付けない。
        m_yaw += m_inputYaw * camRotSpeedYaw * dt;
        if (!m_hasLockTarget)
        {
            m_pitch += m_inputPitch * camRotSpeedPitch * dt;
        }

        // ── ロックオン中: カメラの水平角を「敵と反対側」に強制する ──
        // 敵→自機 の方向をカメラが向くyawに直し、m_yaw をそこへ寄せる。
        // (右スティックのyaw操作はロック中は無視される)
        if (m_hasLockTarget)
        {
            float dx = m_lockTargetPos.x - p.x;
            float dz = m_lockTargetPos.z - p.z;
            // 自機から敵を見る水平角。desiredEye の式が
            //   eye = p - (sin(yaw), cos(yaw)) * horiz
            // となっているので、カメラが敵の反対側=自機の手前に来るには
            // yaw を「自機→敵の方向」に合わせればよい。
            if (dx * dx + dz * dz > 0.0001f)
            {
                float lockYaw = atan2f(dx, dz);
                // 角度差を -PI..PI に畳んでから補間 (ラップ対策)
                float diff = lockYaw - m_yaw;
                while (diff > XM_PI) diff -= XM_2PI;
                while (diff < -XM_PI) diff += XM_2PI;
                // なめらかに敵方向へ向ける
                m_yaw += diff * (1.0f - expf(-8.0f * dt));
            }

            // 上下も同様にロックへ任せる。既定の見下ろし角へなめらかに戻す。
            // (急に固定すると、変な角度でロックしたとき画面が跳ねる)
            m_pitch += (lockPitch - m_pitch) * (1.0f - expf(-6.0f * dt));
        }

        // ロックのON/OFFをなめらかに 0..1 で渡り歩く。
        // 解除後も少しの間は最後の敵座標へ寄せたままにして、注視点の段差を消す。
        float targetBlend = m_hasLockTarget ? 1.0f : 0.0f;
        m_lockBlend += (targetBlend - m_lockBlend) * (1.0f - expf(-6.0f * dt));

        // pitch(見下ろし角)は上下に振れすぎないよう制限。
        const float pitchMin = -0.3f; // やや下から見上げる程度まで
        const float pitchMax = 1.2f;  // かなり上から見下ろすまで
        if (m_pitch < pitchMin) m_pitch = pitchMin;
        if (m_pitch > pitchMax) m_pitch = pitchMax;

        // カメラ位置を、機体を中心に yaw/pitch で球面配置する。
        //   水平距離 = backDistance * cos(pitch)、高さ = backDistance * sin(pitch)
        float horiz = backDistance * cosf(m_pitch);
        float vert = backDistance * sinf(m_pitch);

        XMFLOAT3 desiredEye
        {
            p.x - sinf(m_yaw) * horiz,
            p.y + vert,
            p.z - cosf(m_yaw) * horiz
        };

        // 注視点は機体の少し上。
        XMFLOAT3 desiredTarget
        {
            p.x,
            p.y + lookAtHeight,
            p.z
        };

        // ロック中は、自機と敵を lockLookBias で内分した点を見る (やや敵寄り)。
        if (m_lockBlend > 0.001f)
        {
            float t = lockLookBias * m_lockBlend;
            desiredTarget.x = p.x + (m_lockTargetPos.x - p.x) * t;
            desiredTarget.z = p.z + (m_lockTargetPos.z - p.z) * t;

            // 高さは敵に合わせる。ここを自機基準にすると、ジャンプで視点だけ上がって
            // 敵が画面の下へ流れてしまう。
            float tY = lockLookBiasY * m_lockBlend;
            float baseY = p.y + lookAtHeight;
            desiredTarget.y = baseY + (m_lockTargetPos.y - baseY) * tY;
        }

        if (!m_initialized)
        {
            m_eye = desiredEye;
            m_target = desiredTarget;
            m_initialized = true;
            return true;
        }

        // 水平と上下で別々の速さで追従する。
        // ブースト中は水平追従をゆるめて、機体が進行方向へずれるようにする。
        float hSharp = m_boosting ? sharpnessHBoost : sharpnessH;
        float tH = 1.0f - expf(-hSharp * dt);
        float tV = 1.0f - expf(-sharpnessV * dt);

        // XZ は水平用、Y は上下用の補間を使う。
        m_eye.x = m_eye.x + (desiredEye.x - m_eye.x) * tH;
        m_eye.z = m_eye.z + (desiredEye.z - m_eye.z) * tH;
        m_eye.y = m_eye.y + (desiredEye.y - m_eye.y) * tV; // ← 上下はゆるく

        // カメラが地形にめり込まないよう、地形の高さ+余裕を下限にする。
        if (m_terrain)
        {
            float groundH;
            if (m_terrain->GetGroundHeight(m_eye.x, m_eye.z, 1000.0f, groundH))
            {
                float minY = groundH + terrainClearance;
                if (m_eye.y < minY)
                    m_eye.y = minY;
            }
        }
        else
        {
            // 地形が無ければ固定の下限。
            const float minCameraY = 5.0f;
            if (m_eye.y < minCameraY)
                m_eye.y = minCameraY;
        }

        m_target.x = m_target.x + (desiredTarget.x - m_target.x) * tH;
        m_target.z = m_target.z + (desiredTarget.z - m_target.z) * tH;

        // 上下はゆるく追う。ただしロック中は水平と同じ速さにして、
        // ジャンプ中に狙いが縦へ遅れないようにする。
        float tTargetV = (m_lockBlend > 0.5f) ? tH : tV;
        m_target.y = m_target.y + (desiredTarget.y - m_target.y) * tTargetV;

        return true;
    }

    // 次の Update で補間せず一気に定位置へ着ける (リセット直後用)。
    // これが無いと、機体を初期位置へ戻したときカメラがマップを横断してくる。
    void SnapToOwner()
    {
        m_initialized = false;
        m_lockBlend = 0.0f; // 前回の敵の方を向いたまま着地しないように
    }

    // 向きと入力を初期状態へ戻す (リトライ時に前回の視点を持ち越さない)。
    // 機体の向きの真後ろに構える。機体の transform を戻した後に呼ぶこと。
    void ResetAngles()
    {
        m_yaw = Owner()->transform.rotation.y;
        m_pitch = defaultPitch;
        m_inputYaw = 0.0f;
        m_inputPitch = 0.0f;
        m_boosting = false;
    }
};