#pragma once

namespace Engine
{
    // 体力
    class Health
    {
    public:
        void  SetMax(float max) { m_max = max; m_value = max; }
        float Max() const { return m_max; }
        float Value() const { return m_value; }
        float Ratio() const { return (m_max > 0.0f) ? m_value / m_max : 0.0f; }

        bool IsDead() const { return m_value <= 0.0f; }

        // 減らす
        bool Damage(float amount)
        {
            m_value -= amount;
            if (m_value < 0.0f) m_value = 0.0f;
            return IsDead();
        }

        void Reset() { m_value = m_max; }

    private:
        float m_max = 100.0f;
        float m_value = 100.0f;
    };
}