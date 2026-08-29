#pragma once
#include "Engine/Combat/ActionStateMachine.h"

namespace Engine
{
    enum class ParryResult
    {
        None,
        Success,   // 弾いた
        Hit,       // 当たった
    };

    class ParrySystem
    {
    public:
        // 判定が発生したフレームだけ
        ParryResult Resolve(const ActionStateMachine& attacker,
            const ActionStateMachine& defender) const;

        // 調整パラメータ
        int hitStopOnParry = 10;
        int hitStopOnHit = 5;
    };
}