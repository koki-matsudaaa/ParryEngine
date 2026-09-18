#include "Engine/Combat/AttackSelect.h"

namespace Engine
{
    std::string AttackSelect::Pick(const std::vector<ActionData>& actions)
    {
        int total = 0;
        for (const auto& a : actions)
            if (a.aiWeight > 0) total += a.aiWeight;

        if (total <= 0) return {};

        int r = static_cast<int>(m_rng() % total);
        for (const auto& a : actions)
        {
            if (a.aiWeight <= 0) continue;
            if (r < a.aiWeight) return a.name;
            r -= a.aiWeight;
        }
        return {};
    }
}