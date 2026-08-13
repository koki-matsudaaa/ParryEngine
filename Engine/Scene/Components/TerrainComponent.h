#pragma once
#include "pch.h"
#include "GameObject.h"
#include "FbxModel.h"
#include "FloorPlateModel.h"
#include <vector>

// 地形の当たり判定。地形メッシュの全三角形を保持し、
// 指定位置の真下へレイを飛ばして地面の高さを返す。
class TerrainComponent : public GameComponent
{
public:
    struct Tri
    {
        XMFLOAT3 a, b, c; // 三角形の3頂点 (ワールド座標)
    };

private:
    std::vector<Tri> m_tris;

    // 地形全体を囲む箱 (AABB)。まず箱で大まかに判定し、
    // XZが範囲外なら三角形を調べない (軽い足切り)。
    XMFLOAT3 m_min{ 0,0,0 };
    XMFLOAT3 m_max{ 0,0,0 };
    bool m_hasBounds = false;

    // レイ(原点origin, 方向dir) と三角形の交差判定 (Moller-Trumbore)。
    static bool RayTriangle(const XMFLOAT3& origin, const XMFLOAT3& dir,
        const Tri& tri, float& outT);

public:
    // 三角形を1つ追加。
    void AddTriangle(const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT3& c)
    {
        m_tris.push_back({ a, b, c });
    }

    // 床(FloorPlateModel)の三角形を、位置・スケールで変換して追加する。
    void AddFloor(const FloorPlateModel& floor,
        const XMFLOAT3& pos, const XMFLOAT3& scale)
    {
        const auto& verts = floor.GetVertices();

        auto applyTransform = [&](const XMFLOAT3& p) -> XMFLOAT3
            {
                return XMFLOAT3(
                    p.x * scale.x + pos.x,
                    p.y * scale.y + pos.y,
                    p.z * scale.z + pos.z);
            };

        // FloorPlateModelは頂点を三角形順(3つで1三角形)に並べているので、
        // インデックスなしで3つずつ取る。
        for (size_t i = 0; i + 2 < verts.size(); i += 3)
        {
            XMFLOAT3 a = applyTransform(verts[i].position);
            XMFLOAT3 b = applyTransform(verts[i + 1].position);
            XMFLOAT3 c = applyTransform(verts[i + 2].position);
            m_tris.push_back({ a, b, c });

            // AABB(範囲)も更新する。
            for (const XMFLOAT3& p : { a, b, c })
            {
                if (!m_hasBounds) { m_min = m_max = p; m_hasBounds = true; }
                else {
                    if (p.x < m_min.x) m_min.x = p.x;
                    if (p.y < m_min.y) m_min.y = p.y;
                    if (p.z < m_min.z) m_min.z = p.z;
                    if (p.x > m_max.x) m_max.x = p.x;
                    if (p.y > m_max.y) m_max.y = p.y;
                    if (p.z > m_max.z) m_max.z = p.z;
                }
            }
        }
    }

    // FbxModel の頂点・インデックスから、三角形を一括で取り込む。
    // scale/offset は、地形オブジェクトの Transform に合わせるための変換。
    // (地形を拡大・移動して配置しているなら、その値を渡す)
    void BuildFromFbx(const FbxModel& model,
        float scale = 1.0f,
        const XMFLOAT3& offset = XMFLOAT3(0, 0, 0))
    {
        const auto& verts = model.GetVertices();
        const auto& indices = model.GetIndices();

        m_tris.clear();
        m_hasBounds = false;

        auto applyTransform = [&](const XMFLOAT3& p) -> XMFLOAT3
            {
                return XMFLOAT3(
                    p.x * scale + offset.x,
                    p.y * scale + offset.y,
                    p.z * scale + offset.z);
            };

        auto expand = [&](const XMFLOAT3& p)
            {
                if (!m_hasBounds)
                {
                    m_min = m_max = p;
                    m_hasBounds = true;
                }
                else
                {
                    if (p.x < m_min.x) m_min.x = p.x;
                    if (p.y < m_min.y) m_min.y = p.y;
                    if (p.z < m_min.z) m_min.z = p.z;
                    if (p.x > m_max.x) m_max.x = p.x;
                    if (p.y > m_max.y) m_max.y = p.y;
                    if (p.z > m_max.z) m_max.z = p.z;
                }
            };

        // インデックス3つで1三角形。
        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            XMFLOAT3 a = applyTransform(verts[indices[i]].position);
            XMFLOAT3 b = applyTransform(verts[indices[i + 1]].position);
            XMFLOAT3 c = applyTransform(verts[indices[i + 2]].position);
            m_tris.push_back({ a, b, c });
            expand(a); expand(b); expand(c);
        }

        char buf[128];
        sprintf_s(buf, "Terrain: %zu triangles built\n", m_tris.size());
        OutputDebugStringA(buf);
    }

    size_t GetTriangleCount() const { return m_tris.size(); }

    // 指定XZ位置の地面の高さを返す。
    //   fromY: レイの発射高さ (地形の最高点より十分上にする)。
    //   見つかれば true、outHeight に地面の高さ(Y)。
    bool GetGroundHeight(float x, float z, float fromY, float& outHeight) const
    {
        // AABBのXZ範囲外なら、そもそも地形の上にいないので即false。
        if (m_hasBounds)
        {
            if (x < m_min.x || x > m_max.x || z < m_min.z || z > m_max.z)
                return false;
        }

        XMFLOAT3 origin(x, fromY, z);
        XMFLOAT3 dir(0.0f, -1.0f, 0.0f); // 真下

        bool found = false;
        float nearestT = 1e9f;
        for (const auto& tri : m_tris)
        {
            float t;
            if (RayTriangle(origin, dir, tri, t))
            {
                if (t < nearestT) // 一番手前(高い)交点
                {
                    nearestT = t;
                    found = true;
                }
            }
        }
        if (found)
            outHeight = fromY - nearestT;
        return found;
    }

    bool Update(float /*dt*/) override { return true; }
};