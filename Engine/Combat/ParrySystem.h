#pragma once
#include "Engine/Combat/ActionStateMachine.h"

namespace Engine
{
    enum class ParryResult
    {
        None,
        Success,   // 弾いた
        Guarded,   // ガード
        Hit,       // 当たった
    };

    class ParrySystem
    {
    public:
        // 判定が発生したフレームだけ
        ParryResult Resolve(const ActionStateMachine& attacker,
            const ActionStateMachine& defender,
            float distance) const;

        // 調整パラメータ
        int hitStopOnParry = 10;
        int hitStopOnHit = 5;
        float parryPostureDamage = 25.0f;
        int   hitStopOnGuard = 4;
        float guardPostureRate = 0.35f;   // ガード時に通る体幹の割合
    };
}