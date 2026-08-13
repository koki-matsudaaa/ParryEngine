#pragma once
#include "pch.h"
#include "GameObject.h"


// HP を持つ。ダメージを受けて 0 以下になったら所有者を Kill する。
class HealthComponent : public GameComponent
{
private:
    float m_hp = 100.0f;

public:
    void SetHp(float hp) { m_hp = hp; }
    float GetHp() const { return m_hp; }

    // ダメージを受ける。0 以下になったら所有者に死を伝える。
    void TakeDamage(float dmg)
    {
        m_hp -= dmg;
        char buf[64];
        sprintf_s(buf, "Enemy HP: %.0f\n", m_hp);
        OutputDebugStringA(buf);
        if (m_hp <= 0.0f) { m_hp = 0.0f; Owner()->Destroy(); }
    }

    bool Update(float /*dt*/) override
    {
        return true;
    }
};