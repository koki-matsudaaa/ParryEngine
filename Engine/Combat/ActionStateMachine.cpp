#include "Engine/Combat/ActionStateMachine.h"

#include <algorithm>

namespace Engine
{
    void ActionStateMachine::AddAction(const ActionData& action)
    {
        auto it = m_indexByName.find(action.name);
        if (it != m_indexByName.end())
        {
            m_actions[it->second] = action;   // 同名の差し替え
            return;
        }

        m_indexByName[action.name] = static_cast<int>(m_actions.size());
        m_actions.push_back(action);
    }

    const ActionData* ActionStateMachine::Find(const std::string& name) const
    {
        auto it = m_indexByName.find(name);
        return (it != m_indexByName.end()) ? &m_actions[it->second] : nullptr;
    }

    const ActionData* ActionStateMachine::CurrentAction() const
    {
        return (m_current >= 0) ? &m_actions[m_current] : nullptr;
    }

    bool ActionStateMachine::StartAction(const std::string& name)
    {
        auto it = m_indexByName.find(name);
        if (it == m_indexByName.end()) return false;

        m_current = it->second;
        m_frame = 0;

        // 対応するアニメへ切り替える。
        const ActionData& a = m_actions[m_current];
        if (m_animator && !a.clipName.empty())
            m_animator->Play(a.clipName, a.blendSeconds);

        return true;
    }

    void ActionStateMachine::Step()
    {
        if (m_current < 0)
        {
            m_prevPhase = ActionPhase::None;
            return;
        }

        m_prevPhase = Phase();   // 進める前の段階を覚えておく

        m_frame++;

        // 全フレームを消化したら終了。
        if (m_frame >= m_actions[m_current].TotalFrames())
        {
            m_current = -1;
            m_frame = 0;
        }
    }

    void ActionStateMachine::Cancel()
    {
        m_current = -1;
        m_frame = 0;
    }

    ActionPhase ActionStateMachine::Phase() const
    {
        if (m_current < 0) return ActionPhase::None;

        const ActionData& a = m_actions[m_current];

        if (m_frame < a.startup)            return ActionPhase::Startup;
        if (m_frame < a.startup + a.active) return ActionPhase::Active;
        return ActionPhase::Recovery;
    }

    const std::string& ActionStateMachine::CurrentActionName() const
    {
        static const std::string empty;
        return (m_current >= 0) ? m_actions[m_current].name : empty;
    }

    bool ActionStateMachine::CanCancel() const
    {
        if (m_current < 0) return true;

        const ActionData& a = m_actions[m_current];
        return (a.cancelFrom >= 0 && m_frame >= a.cancelFrom);
    }

    bool ActionStateMachine::CanCancelInto(const std::string& next) const
    {
        if (m_current < 0) return true;   // 何もしていないなら自由

        const ActionData& a = m_actions[m_current];

        // キャンセル可能区間に入っているか。
        if (a.cancelFrom < 0 || m_frame < a.cancelFrom) return false;

        // 移り先の制限。空なら何にでも移れる。
        if (a.cancelTo.empty()) return true;

        return std::find(a.cancelTo.begin(), a.cancelTo.end(), next)
            != a.cancelTo.end();
    }

    bool ActionStateMachine::StartParry() const
    {
        return Phase() == ActionPhase::Active
            && m_prevPhase != ActionPhase::Active;
    }

    bool ActionStateMachine::OnParry() const
    {
        if (m_current < 0) return false;

        return m_actions[m_current].isParry
            && Phase() == ActionPhase::Active;
    }
}