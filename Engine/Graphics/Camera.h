#pragma once
#include <DirectXMath.h>

namespace Engine
{
    class Camera
    {
    public:
        // 見る対象の座標
        void SetTarget(const DirectX::XMFLOAT3& t) { m_target = t; }

        // 水平方向の回り込み。
        void AddYaw(float radians) { m_yaw += radians; }

        // 見下ろし角。
        void AddPitch(float radians);

        void  SetDistance(float d) { m_distance = d; }
        float GetDistance() const { return m_distance; }

        // 移動をカメラ基準にするとき、ゲーム側がこの角度を読む。
        float GetYaw() const { return m_yaw; }

        void  SetPitch(float radians);
        float GetPitch() const { return m_pitch; }

        // 実際に見ている点
        DirectX::XMFLOAT3 LookPoint() const;

        DirectX::XMFLOAT3 Eye() const;
        DirectX::XMMATRIX View() const;
        DirectX::XMMATRIX Projection() const;

        // --- 調整パラメータ ---
        float fovY = DirectX::XM_PIDIV4;
        float aspect = 1280.0f / 720.0f;
        float nearZ = 0.1f;
        float farZ = 500.0f;

        // 注視点を足元からどれだけ上げるか。
        float lookHeight = 1.2f;

        float minPitch = -0.20f;   // これ以上見上げない
        float maxPitch = 1.20f;    // これ以上見下ろさない

        float sideOffset = 0.0f;

    private:
        DirectX::XMFLOAT3 m_target{ 0.0f, 0.0f, 0.0f };
        float m_yaw = 0.0f;
        float m_pitch = 0.35f;
        float m_distance = 4.0f;
    };
}