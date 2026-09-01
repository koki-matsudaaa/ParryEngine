#include "Engine/Combat/Posture.h"

namespace Engine
{
    bool Posture::Add(float amount)
    {
        if (m_broken) return false;   // ダウン中は貯めない

        m_value += amount;
        m_sinceDamage = 0;            // 回復の待ち時間をやり直す

        if (m_value >= m_max)
        {
            m_value = m_max;
            m_broken = true;
            m_breakTimer = breakFrames;
            return true;              // ダウンした瞬間
        }
        return false;
    }

    void Posture::Step()
    {
        if (m_broken)
        {
            if (--m_breakTimer <= 0) Recover();
            return;                   // ダウン中は回復しない
        }

        m_sinceDamage++;

        // 少し間を置いてから回復させる。
        if (m_sinceDamage >= regenDelayFrames)
        {
            m_value -= regenPerFrame;
            if (m_value < 0.0f) m_value = 0.0f;
        }
    }

    void Posture::Recover()
    {
        m_broken = false;
        m_value = 0.0f;
        m_breakTimer = 0;
    }
}