#pragma once
#include "Engine/Combat/ActionStateMachine.h"

#include <vector>

namespace Engine
{
    // 1フレーム分の記録
    struct FrameRecord
    {
        ActionPhase phase = ActionPhase::None;
        bool isParry = false;   // その瞬間パリィを受け付けていたか
    };

    enum class TimelineEvent { None, ParrySuccess, Hit };

    class TimelineRecorder
    {
    public:
        // 何フレーム分溜めるか
        void SetCapacity(int frames);

        int Capacity() const { return static_cast<int>(m_frames.size()); }
        int Count()    const { return m_count; }

        void Record(const ActionStateMachine& sm);

        // 直前に記録したフレームに印
        void MarkEvent(TimelineEvent e);

        // 中身だけ空にする
        void Clear();

        void SetPaused(bool p) { m_paused = p; }
        bool IsPaused() const { return m_paused; }

        const FrameRecord& At(int i) const;
        TimelineEvent   EventAt(int i) const;

    private:
        int Index(int i) const;

        std::vector<FrameRecord>   m_frames;
        std::vector<TimelineEvent> m_events;

        int  m_head = 0;    // 次に書き込む位置
        int  m_count = 0;   // 溜まっている数
        bool m_paused = false;
    };
}