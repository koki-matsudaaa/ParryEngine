#include "Engine/Combat/ParrySystem.h"

namespace Engine
{
    ParryResult ParrySystem::Resolve(const ActionStateMachine& attacker,
        const ActionStateMachine& defender,
        float distance) const
    {
        // 判定が出た最初の1フレームだけ
        if (!attacker.JustBecameActive()) return ParryResult::None;

        const ActionData* a = attacker.CurrentAction();

        // パリィ技の判定
        if (!a || !a->isAttack) return ParryResult::None;

        // 間合いの外なら空振り
        if (distance > a->range) return ParryResult::None;

        // よけた時　はじけない攻撃より前に
        if (defender.IsInvincible()) return ParryResult::None;

        // はじけない攻撃の貫通
        if (a->unblockable) return ParryResult::Hit;

        // その瞬間に受付中だったか
        if (defender.OnParry()) return ParryResult::Success;

        // ガードしていれば受け止め
        if (defender.IsGuarding()) return ParryResult::Guarded;

        return ParryResult::Hit;
    }
}