#pragma once
#include "pch.h"
#include "GameObject.h"
#include "Rigidbody.h"

class JumpComponent : public GameComponent
{
private:
    RigidBody* m_body = nullptr;
    bool m_jumping = false;
    bool m_prevJumping = false;

protected:
    void OnAttach() override
    {
        m_body = Owner()->GetComponent<RigidBody>();
    }

public:
    float jumpSpeed = 40.0f;

    void SetJumping(bool on)
    {
        m_jumping = on;
    }

    bool Update(float /*dt*/) override
    {
        if (!m_body) m_body = Owner()->GetComponent<RigidBody>();
        if (!m_body) return true;

        // 押した瞬間(前フレーム false → 今 true) かつ 接地中 のときだけ飛ぶ。
        if (m_jumping && !m_prevJumping && m_body->IsGrounded())
        {
            XMFLOAT3 v = m_body->GetVelocity();
            v.y = jumpSpeed;
            m_body->SetVelocity(v);
        }

        m_prevJumping = m_jumping;
        return true;
    }
};

