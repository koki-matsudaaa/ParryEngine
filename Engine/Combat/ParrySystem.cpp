#include "Engine/Combat/ParrySystem.h"

namespace Engine
{
    ParryResult ParrySystem::Resolve(const ActionStateMachine& attacker,
        const ActionStateMachine& defender,
        bool hits) const
    {
        // 判定が出ている間はいつでも当たる。1回の振りで1度だけ。
        if (attacker.Phase() != ActionPhase::Active) return ParryResult::None;
        if (attacker.HasHit()) return ParryResult::None;

        const ActionData* a = attacker.CurrentAction();

        // 攻撃でなければ何も起きない
        if (!a || !a->isAttack) return ParryResult::None;

        // 形が当たっていなければ空振り
        if (!hits) return ParryResult::None;

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