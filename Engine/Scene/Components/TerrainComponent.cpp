#include "pch.h"
#include "TerrainComponent.h"

bool TerrainComponent::RayTriangle(const XMFLOAT3& origin, const XMFLOAT3& dir,
    const Tri& tri, float& outT)
{
    // Moller-Trumbore のレイ-三角形交差判定。
    XMVECTOR o = XMLoadFloat3(&origin);
    XMVECTOR d = XMLoadFloat3(&dir);
    XMVECTOR a = XMLoadFloat3(&tri.a);
    XMVECTOR b = XMLoadFloat3(&tri.b);
    XMVECTOR c = XMLoadFloat3(&tri.c);

    XMVECTOR e1 = XMVectorSubtract(b, a);
    XMVECTOR e2 = XMVectorSubtract(c, a);

    XMVECTOR pvec = XMVector3Cross(d, e2);
    float det = XMVectorGetX(XMVector3Dot(e1, pvec));

    // 平行に近い (det≒0) なら交差なし。
    const float EPS = 1e-6f;
    if (det > -EPS && det < EPS) return false;

    float invDet = 1.0f / det;
    XMVECTOR tvec = XMVectorSubtract(o, a);
    float u = XMVectorGetX(XMVector3Dot(tvec, pvec)) * invDet;
    if (u < 0.0f || u > 1.0f) return false;

    XMVECTOR qvec = XMVector3Cross(tvec, e1);
    float v = XMVectorGetX(XMVector3Dot(d, qvec)) * invDet;
    if (v < 0.0f || u + v > 1.0f) return false;

    float t = XMVectorGetX(XMVector3Dot(e2, qvec)) * invDet;
    if (t < 0.0f) return false; // レイの後ろは無視

    outT = t;
    return true;
}