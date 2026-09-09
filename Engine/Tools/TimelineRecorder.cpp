#include "Engine/Tools/TimelineRecorder.h"

namespace Engine
{
    void TimelineRecorder::SetCapacity(int frames)
    {
        if (frames < 1) frames = 1;

        m_frames.assign(frames, FrameRecord{});
        m_events.assign(frames, TimelineEvent::None);
        m_head = 0;
        m_count = 0;
    }

    void TimelineRecorder::Clear()
    {
        m_head = 0;
        m_count = 0;
    }

    int TimelineRecorder::Index(int i) const
    {
        const int cap = Capacity();
        return (m_head - m_count + i + cap) % cap;
    }

    void TimelineRecorder::Record(const ActionStateMachine& sm)
    {
        if (m_paused || m_frames.empty()) return;

        FrameRecord r;
        r.phase = sm.Phase();
        r.isParry = sm.OnParry();

        m_frames[m_head] = r;
        m_events[m_head] = TimelineEvent::None;   // 前周の印を消す

        m_head = (m_head + 1) % Capacity();
        if (m_count < Capacity()) m_count++;
    }

    void TimelineRecorder::MarkEvent(TimelineEvent e)
    {
        if (m_count == 0) return;

        const int last = (m_head - 1 + Capacity()) % Capacity();
        m_events[last] = e;
    }

    const FrameRecord& TimelineRecorder::At(int i) const
    {
        static const FrameRecord empty;
        if (i < 0 || i >= m_count) return empty;
        return m_frames[Index(i)];
    }

    TimelineEvent TimelineRecorder::EventAt(int i) const
    {
        if (i < 0 || i >= m_count) return TimelineEvent::None;
        return m_events[Index(i)];
    }
}