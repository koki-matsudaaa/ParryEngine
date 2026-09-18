#include "Engine/Combat/ParrySystem.h"

namespace Engine
{
    ParryResult ParrySystem::Resolve(const ActionStateMachine& attacker,
        const ActionStateMachine& defender) const
    {
        // 判定が出た最初の1フレームだけ
        if (!attacker.JustBecameActive()) return ParryResult::None;

        const ActionData* a = attacker.CurrentAction();

        // パリィ技の判定
        if (!a || a->isParry) return ParryResult::None;

        // はじけない攻撃の貫通
        if (a->unblockable) return ParryResult::Hit;

        // その瞬間に受付中だったか
        if (defender.OnParry()) return ParryResult::Success;

        return ParryResult::Hit;
    }
}