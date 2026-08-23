#pragma once
#include "Engine/Combat/ActionData.h"
#include "Engine/Animation/Animator.h"

#include <map>
#include <string>
#include <vector>

namespace Engine
{
    class ActionStateMachine
    {
    public:
        // アニメの再生先。アクション開始時にクリップを切り替える。
        void SetAnimator(Animator* animator) { m_animator = animator; }

        // アクションを登録する。同名があれば差し替える。
        void AddAction(const ActionData& action);

        // アクションを開始する。登録が無ければ false。
        // 割り込んでよいかの判断は呼び出し側が行う (CanCancel を見る)。
        bool StartAction(const std::string& name);

        // 1フレーム進める。毎ステップ1回だけ呼ぶ。
        void Step();

        // 進行を打ち切って何もしていない状態に戻す。
        void Cancel();

        ActionPhase Phase() const;
        bool IsIdle() const { return m_current < 0; }

        // アクション開始からの経過フレーム。
        int ElapsedFrames() const { return m_frame; }

        const std::string& CurrentActionName() const;

        // 今このアクションを中断して別の行動に移れるか。
        bool CanCancel() const;

        // 登録されているアクションを名前で引く。
        const ActionData* Find(const std::string& name) const;

    private:
        Animator* m_animator = nullptr;

        std::vector<ActionData> m_actions;
        std::map<std::string, int> m_indexByName;

        int m_current = -1;   // 実行中のアクション
        int m_frame = 0;      // アクションの経過フレーム
    };
}