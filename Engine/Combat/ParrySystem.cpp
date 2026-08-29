#include "Engine/Combat/ParrySystem.h"

namespace Engine
{
    ParryResult ParrySystem::Resolve(const ActionStateMachine& attacker,
        const ActionStateMachine& defender) const
    {
        // 判定が出た最初の1フレームだけ
        if (!attacker.StartParry()) return ParryResult::None;

        // その瞬間に受付中だったか
        if (defender.OnParry()) return ParryResult::Success;

        return ParryResult::Hit;
    }
}