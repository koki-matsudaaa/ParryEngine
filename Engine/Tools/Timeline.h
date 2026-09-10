#pragma once
#include "Engine/Combat/ActionData.h"
#include "Engine/Tools/TimelineRecorder.h"

namespace Engine
{
    // 発生・判定・硬直を表示、キャンセル可能は縦線
    void DrawActionBar(const ActionData& action,
        int   playheadFrame = -1,
        float pixelsPerFrame = 6.0f);

    // 10フレームごとにメモリ
    void DrawFrameRuler(int totalFrames, float pixelsPerFrame = 6.0f);

    // 色の意味
    void DrawPhaseLegend();

    void DrawRecordedTrack(const TimelineRecorder& rec, float pixelsPerFrame = 3.0f);
}