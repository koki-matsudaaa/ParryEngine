#include "Engine/Core/FixedTimestep.h"

namespace Engine
{
    FixedTimestep::FixedTimestep(float stepSeconds, int maxStepsPerFrame)
        : m_step(stepSeconds > 0.0f ? stepSeconds : 1.0f / 60.0f)
        , m_maxSteps(maxStepsPerFrame > 0 ? maxStepsPerFrame : 1)
    {
    }

    void FixedTimestep::Reset()
    {
        m_accumulator = 0.0f;
        m_alpha = 0.0f;
    }

    int FixedTimestep::Advance(float realDeltaSeconds)
    {
        // ポーズ中は時間を溜めない。ただし1ステップ要求があればそれだけ通す。
        if (m_paused)
        {
            if (m_singleStepRequested)
            {
                m_singleStepRequested = false;
                m_frameCount++;
                m_alpha = 0.0f;
                return 1;
            }
            return 0;
        }

        // 負の値や異常値が来ても壊れないようにしておく。
        if (realDeltaSeconds < 0.0f) realDeltaSeconds = 0.0f;

        m_accumulator += realDeltaSeconds;

        int steps = 0;
        while (m_accumulator >= m_step && steps < m_maxSteps)
        {
            m_accumulator -= m_step;
            steps++;
        }

        // 上限に達してもまだ溜まっている = 明らかに処理落ちしている。
        // 溜まった分を捨てて、次フレームで一気に進まないようにする。
        // (時間の進みは遅れるが、動作が破綻するよりはよい)
        if (m_accumulator >= m_step)
        {
            m_accumulator = 0.0f;
        }

        m_frameCount += static_cast<unsigned long long>(steps);
        m_alpha = m_accumulator / m_step;
        return steps;
    }
}
