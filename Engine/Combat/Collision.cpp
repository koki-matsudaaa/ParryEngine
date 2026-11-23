#include "Engine/Combat/Collision.h"

using namespace DirectX;

namespace Engine
{
    namespace
    {
        XMFLOAT3 Sub(const XMFLOAT3& a, const XMFLOAT3& b)
        {
            return XMFLOAT3(a.x - b.x, a.y - b.y, a.z - b.z);
        }

        float Dot(const XMFLOAT3& a, const XMFLOAT3& b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        // 線分からはみ出さないように収める
        float Clamp01(float v)
        {
            if (v < 0.0f) return 0.0f;
            if (v > 1.0f) return 1.0f;
            return v;
        }
    }

    float SegmentDistanceSq(const XMFLOAT3& p1, const XMFLOAT3& q1,
        const XMFLOAT3& p2, const XMFLOAT3& q2)
    {
        const XMFLOAT3 d1 = Sub(q1, p1);   // 線分1の向きと長さ
        const XMFLOAT3 d2 = Sub(q2, p2);   // 線分2の向きと長さ
        const XMFLOAT3 r = Sub(p1, p2);

        const float a = Dot(d1, d1);   // 線分1の長さの2乗
        const float e = Dot(d2, d2);   // 線分2の長さの2乗
        const float f = Dot(d2, r);

        const float kEps = 1e-8f;

        float s = 0.0f;   // 線分1のどこか
        float t = 0.0f;   // 線分2のどこか

        if (a <= kEps && e <= kEps)
        {
            const XMFLOAT3 diff = Sub(p1, p2);
            return Dot(diff, diff);
        }

        if (a <= kEps)
        {
            s = 0.0f;
            t = Clamp01(f / e);
        }
        else
        {
            const float c = Dot(d1, r);

            if (e <= kEps)
            {
                t = 0.0f;
                s = Clamp01(-c / a);
            }
            else
            {
                const float b = Dot(d1, d2);
                const float denom = a * e - b * b;

                s = (denom > kEps) ? Clamp01((b * f - c * e) / denom) : 0.0f;
                t = (b * s + f) / e;

                if (t < 0.0f) { t = 0.0f; s = Clamp01(-c / a); }
                else if (t > 1.0f) { t = 1.0f; s = Clamp01((b - c) / a); }
            }
        }

        const XMFLOAT3 c1(p1.x + d1.x * s, p1.y + d1.y * s, p1.z + d1.z * s);
        const XMFLOAT3 c2(p2.x + d2.x * t, p2.y + d2.y * t, p2.z + d2.z * t);
        const XMFLOAT3 diff = Sub(c1, c2);
        return Dot(diff, diff);
    }
}