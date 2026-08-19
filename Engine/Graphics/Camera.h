#pragma once
#include <DirectXMath.h>

namespace Engine
{
    // ────────────────────────────────────────────────────────────
    // 注視点のまわりを回り込むカメラ。
    //
    // 対象の足元の座標を渡すと、その周囲を yaw / pitch / 距離 で回る。
    // ロックオン中のボスを見る、という使い方も同じ仕組みで足りる。
    // ────────────────────────────────────────────────────────────
    class Camera
    {
    public:
        // 見る対象の座標 (足元)。
        void SetTarget(const DirectX::XMFLOAT3& t) { m_target = t; }

        // 水平方向の回り込み。
        void AddYaw(float radians) { m_yaw += radians; }

        // 見下ろし角。上下は行き過ぎないよう頭打ちにする
        // (真上や真下に回り込むと、上方向ベクトルが破綻するため)。
        void AddPitch(float radians);

        void  SetDistance(float d) { m_distance = d; }
        float GetDistance() const { return m_distance; }

        // 移動をカメラ基準にするとき、ゲーム側がこの角度を読む。
        float GetYaw() const { return m_yaw; }

        DirectX::XMFLOAT3 Eye() const;
        DirectX::XMMATRIX View() const;
        DirectX::XMMATRIX Projection() const;

        // ── 調整パラメータ (1 unit = 1m) ──
        float fovY = DirectX::XM_PIDIV4;
        float aspect = 1280.0f / 720.0f;
        float nearZ = 0.1f;
        float farZ = 500.0f;

        // 注視点を足元からどれだけ上げるか。胸のあたりを見る。
        float lookHeight = 1.2f;

        float minPitch = -0.20f;   // これ以上見上げない
        float maxPitch = 1.20f;    // これ以上見下ろさない

    private:
        DirectX::XMFLOAT3 m_target{ 0.0f, 0.0f, 0.0f };
        float m_yaw = 0.0f;
        float m_pitch = 0.35f;
        float m_distance = 4.0f;
    };
}