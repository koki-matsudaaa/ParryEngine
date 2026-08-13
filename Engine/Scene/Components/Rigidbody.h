#pragma once
#include "GameObject.h"
#include "TerrainComponent.h"

class RigidBody : public GameComponent
{
private:
    XMFLOAT3 m_velocity{ 0.0f, 0.0f, 0.0f };
    XMFLOAT3 m_accel{ 0.0f, 0.0f, 0.0f }; // 今フレーム分の力(加速度)の合計
    float m_frameSpeedLimit = 0.0f;       // 今フレームだけの最高速要求(0なら通常)
    bool m_grounded = true;               // 設置しているか
    bool m_prevGrounded = true;           // 前フレームの接地状態 (着地検出用)
    float m_frameFriction = -1.0f;

    TerrainComponent* m_terrain = nullptr; // 地形 (あれば高さをそこから取る)

public:
    // 調整パラメータ
    // 調整パラメータ
    float maxSpeed = 30.0f;  // 水平の最高速度 (単位/秒)
    float friction = 4.0f;   // 水平の減衰の強さ (大きいほど早く止まる)
    float gravity = 30.0f;

    // 空中用。地上と同じ値だと、ブーストが切れた瞬間に急停止してしまう。
    // 空気抵抗はほぼ無いものとして、慣性で飛び続けさせる。
    float airFriction = 0.2f;   // 空中の減衰
    float airMaxSpeed = 70.0f;  // 空中の水平最高速 (boostMaxSpeed に合わせておく)

    float groundY = 0.0f;    // 地形が無いときのフォールバック地面高さ

    // 着地の重さ: 着地の瞬間、落下が速いほど水平速度を削る (踏ん張り)。
    float landingBrake = 0.6f;       // 削る割合の最大 (0=削らない, 1=完全停止)
    float landingSpeedRef = 40.0f;   // この落下速度で削りが最大になる基準

    // 地形レイの発射: 機体の少し上から下へ撃つ。
    // 機体より上にある床を拾わない (床の下でワープするのを防ぐ)。
    float rayUpMargin = 3.0f; // 機体の頭上どれだけから撃つか

    // 機体の足元オフセット。モデルの原点が足でなく中心などにある場合、
    // 地面に合わせるための補正。0なら原点=足。
    float footOffset = 0.0f;

    // 地形をセットする (main.cpp から機体のRigidBodyに渡す)。
    void SetTerrain(TerrainComponent* terrain) { m_terrain = terrain; }

    bool IsGrounded() const { return m_grounded; }

    void AddForce(const XMFLOAT3& f)
    {
        m_accel.x += f.x;
        m_accel.y += f.y;
        m_accel.z += f.z;
    }

    const XMFLOAT3& GetVelocity() const { return m_velocity; }
    void SetVelocity(const XMFLOAT3& v) { m_velocity = v; }

    void RequestSpeedLimit(float limit)
    {
        if (limit > m_frameSpeedLimit) m_frameSpeedLimit = limit;
    }

    void RequestFriction(float f)
    {
        m_frameFriction = f;
    }

    bool Update(float dt) override
    {
        Transform& tf = Owner()->transform;

        float speedLimit = (m_frameSpeedLimit > maxSpeed) ? m_frameSpeedLimit : maxSpeed;

        // 重力
        m_velocity.y -= gravity * dt;

        // 受けた力を速度に積む
        m_velocity.x += m_accel.x * dt;
        m_velocity.y += m_accel.y * dt;
        m_velocity.z += m_accel.z * dt;

        // 水平方向だけ摩擦で減衰
        float useFriction = (m_frameFriction >= 0.0f) ? m_frameFriction : friction;
        float damp = 1.0f - useFriction * dt;
        if (damp < 0.0f) damp = 0.0f;
        m_velocity.x *= damp;
        m_velocity.z *= damp;

        // 水平の最高速度で頭打ち
        float speedSq = m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z;
        if (speedSq > speedLimit * speedLimit)
        {
            float speed = sqrtf(speedSq);
            float scale = speedLimit / speed;
            m_velocity.x *= scale;
            m_velocity.z *= scale;
        }

        // 位置を進める
        tf.position.x += m_velocity.x * dt;
        tf.position.y += m_velocity.y * dt;
        tf.position.z += m_velocity.z * dt;

        // ── 接地判定 ──
        // 地形があれば、機体の真下の地形の高さを取る。無ければ groundY。
        float ground = groundY;
        if (m_terrain)
        {
            float h;
            float rayFrom = tf.position.y + rayUpMargin;
            if (m_terrain->GetGroundHeight(tf.position.x, tf.position.z,
                rayFrom, h))
            {
                ground = h + footOffset;
            }
            // 地形の範囲外 (レイが当たらない) ときは groundY をフォールバックに使う。
        }

        if (tf.position.y <= ground)
        {
            tf.position.y = ground;

            // 着地の瞬間 (前フレーム空中 → 今接地) を検出。
            if (!m_prevGrounded)
            {
                // 落下速度 (下向き) が速いほど、水平速度を強く削る = ずしっと。
                float fallSpeed = (m_velocity.y < 0.0f) ? -m_velocity.y : 0.0f;
                float ratio = fallSpeed / landingSpeedRef;
                if (ratio > 1.0f) ratio = 1.0f;
                float brake = 1.0f - landingBrake * ratio; // 削った後に残る割合
                m_velocity.x *= brake;
                m_velocity.z *= brake;
            }

            if (m_velocity.y < 0) m_velocity.y = 0.0f;
            m_grounded = true;
        }
        else
        {
            m_grounded = false;
        }
        m_prevGrounded = m_grounded;

        // 今フレームの要求を使い切る
        m_accel = XMFLOAT3(0.0f, 0.0f, 0.0f);
        m_frameSpeedLimit = 0.0f;
        m_frameFriction = -1.0f;
        return true;
    }
};