#pragma once
#include "pch.h"
#include "GameObject.h"

// 球の当たり判定。中心はオブジェクトの位置 + オフセット。
// モデルの原点が足元にある場合、オフセットで体の中心へ持ち上げる。
class ColliderComponent : public GameComponent
{
private:
    float    m_radius = 1.0f;
    XMFLOAT3 m_offset{ 0.0f, 0.0f, 0.0f };

public:
    void SetRadius(float r) { m_radius = r; }
    float GetRadius() const { return m_radius; }

    // 中心のずらし量。足元原点のモデルなら y を上げる。
    void SetOffset(const XMFLOAT3& o) { m_offset = o; }
    const XMFLOAT3& GetOffset() const { return m_offset; }

    // 当たり判定の中心。照準や弾の狙い点もここを使う。
    XMFLOAT3 GetCenter() const
    {
        const XMFLOAT3& p = Owner()->transform.position;
        return XMFLOAT3(p.x + m_offset.x, p.y + m_offset.y, p.z + m_offset.z);
    }

    bool Update(float /*dt*/) override
    {
        return true; // データを持つだけ。常に生存。
    }
};