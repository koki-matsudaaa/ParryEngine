#pragma once
#include "Engine/Combat/ActionData.h"

#include <random>
#include <string>
#include <vector>

namespace Engine
{
    class AttackSelect
    {
    public:
        void SetSeed(unsigned int seed) { m_rng.seed(seed); }

        std::string Pick(const std::vector<ActionData>& actions);

    private:
        std::mt19937 m_rng;   // 標準ライブラリ乱数
    };
}