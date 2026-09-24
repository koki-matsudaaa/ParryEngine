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
        // アニメの再生先
        void SetAnimator(Animator* animator) { m_animator = animator; }

        // 登録されたモーションの一覧
        const Animator* GetAnimator() const { return m_animator; }

        // アクションを登録
        void AddAction(const ActionData& action);

        // アクションを開始
        bool StartAction(const std::string& name);

        // 1フレーム進める
        void Step();

        void Cancel();

        void Clear();

        ActionPhase Phase() const;
        bool IsIdle() const { return m_current < 0; }

        // アクション開始からの経過フレーム。
        int ElapsedFrames() const { return m_frame; }

        const std::string& CurrentActionName() const;

        // 今このアクションを中断して別の行動に移れるか。
        bool CanCancel() const;

        // 今このアクションを中断して、指定の行動へ移れるか。
        bool CanCancelInto(const std::string& next) const;

        // このフレームでパリィに入った
        bool JustBecameActive() const;

        // 今パリィ受付中か
        bool OnParry() const;

        // 今無敵中か
        bool IsInvincible() const;

        // ホールド
        void SetHold(bool held) { m_hold = held; }

        // 今ガード中か
        bool IsGuarding() const;

        // frames 分止める
        void AddStagger(int frames);

        int Stagger() const { return m_stagger; }

        // 登録されているアクションを名前で引く。
        const ActionData* Find(const std::string& name) const;

        // 今実行中のアクション
        const ActionData* CurrentAction() const;

        // 登録されているアクション一覧
        std::vector<ActionData>& Actions() { return m_actions; }

    private:
        Animator* m_animator = nullptr;

        std::vector<ActionData> m_actions;
        std::map<std::string, int> m_indexByName;

        int m_current = -1;   // 実行中のアクション
        int m_frame = 0;      // アクションの経過フレーム

        // 直前フレームの段階保持
        ActionPhase m_prevPhase = ActionPhase::None;

        bool m_hold = false;

        int m_stagger = 0;
    };
}