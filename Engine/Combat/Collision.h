#pragma once
#include <DirectXMath.h>

namespace Engine
{
    // 当たり判定の形。
    struct Capsule
    {
        DirectX::XMFLOAT3 a{ 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 b{ 0.0f, 0.0f, 0.0f };
        float radius = 0.1f;
    };

    // 2本の線分で、一番近い点どうしの距離の2乗
    float SegmentDistanceSq(const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT3& q1,
        const DirectX::XMFLOAT3& p2, const DirectX::XMFLOAT3& q2);

    // カプセル同士が重なっているか。
    inline bool Overlap(const Capsule& x, const Capsule& y)
    {
        const float r = x.radius + y.radius;
        return SegmentDistanceSq(x.a, x.b, y.a, y.b) <= r * r;
    }
}