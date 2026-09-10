#pragma once

namespace Engine
{
    // 体幹
    class Posture
    {
    public:
        void  SetMax(float max) { m_max = max; }
        float Max() const { return m_max; }
        float Value() const { return m_value; }
        float Ratio() const { return (m_max > 0.0f) ? m_value / m_max : 0.0f; }

        bool IsBroken() const { return m_broken; }

        // 体幹削り
        bool Add(float amount);

        void Step();

        // ダウン状態を解除して
        void Recover();

        // 調整パラメータ
        float regenPerFrame = 0.15f;   // 1フレームあたりの自然回復量
        int   regenDelayFrames = 60;   // 削られてから回復が始まるまで
        int   breakFrames = 120;       // ダウン状態が続くフレーム数

    private:
        float m_max = 100.0f;
        float m_value = 0.0f;

        int  m_sinceDamage = 0;   // 削られてからの経過フレーム
        int  m_breakTimer = 0;    // ダウン状態の残りフレーム
        bool m_broken = false;
    };
}