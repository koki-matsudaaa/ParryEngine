#pragma once
#include <DirectXMath.h>
#include <cmath>

namespace Engine
{
    // 角度の差
    inline float AngleDiff(float from, float to)
    {
        float diff = to - from;
        while (diff > DirectX::XM_PI) diff -= DirectX::XM_2PI;
        while (diff < -DirectX::XM_PI) diff += DirectX::XM_2PI;
        return diff;
    }

    // 位置と向き
    struct Transform
    {
        DirectX::XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
        float yaw = 0.0f;

        // 向いている方向
        DirectX::XMFLOAT3 Forward() const
        {
            return DirectX::XMFLOAT3(std::sin(yaw), 0.0f, std::cos(yaw));
        }

        // 行列　拡大、回転、移動
        DirectX::XMMATRIX World(float scale) const
        {
            return DirectX::XMMatrixScaling(scale, scale, scale)
                * DirectX::XMMatrixRotationY(yaw)
                * DirectX::XMMatrixTranslation(position.x, position.y, position.z);
        }

        // 目標への回転をなるべく減らす
        void TurnTowards(float targetYaw, float maxRadians)
        {
            float diff = AngleDiff(yaw, targetYaw);
            if (diff > maxRadians) diff = maxRadians;
            if (diff < -maxRadians) diff = -maxRadians;
            yaw += diff;
        }
    };
}