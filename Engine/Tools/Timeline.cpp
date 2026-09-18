#include "Engine/Tools/Timeline.h"
#include "imgui.h"

#include <cstdio>

namespace
{
    const ImVec4 kStartup = ImVec4(0.75f, 0.75f, 0.25f, 1.0f);  // 発生 (黄)
    const ImVec4 kActive = ImVec4(0.75f, 0.25f, 0.25f, 1.0f);   // 判定 (赤)
    const ImVec4 kRecovery = ImVec4(0.25f, 0.50f, 0.75f, 1.0f); // 硬直 (青)
    const ImVec4 kParry = ImVec4(0.25f, 0.75f, 0.50f, 1.0f);    // パリィ受付 (緑)
    const ImVec4 kUnblock = ImVec4(0.85f, 0.45f, 0.15f, 1.0f);  // 弾けない (橙)

    constexpr float kBarHeight = 18.0f;
    constexpr float kRulerHeight = 14.0f;
    constexpr float kTrackHeight = 14.0f;

    // 1区間を塗る
    float FillSegment(ImDrawList* dl, ImVec2 origin, float x,
        int frames, float ppf, const ImVec4& color)
    {
        const float w = frames * ppf;
        if (w <= 0.0f) return 0.0f;

        dl->AddRectFilled(ImVec2(origin.x + x, origin.y),
            ImVec2(origin.x + x + w, origin.y + kBarHeight),
            ImGui::GetColorU32(color));
        return w;
    }
}

namespace Engine
{
    void DrawFrameRuler(int totalFrames, float ppf)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 origin = ImGui::GetCursorScreenPos();

        for (int f = 0; f <= totalFrames; f += 10)
        {
            const float x = origin.x + f * ppf;

            dl->AddLine(ImVec2(x, origin.y),
                ImVec2(x, origin.y + kRulerHeight),
                IM_COL32(140, 140, 140, 255));

            char label[16];
            std::snprintf(label, sizeof(label), "%d", f);
            dl->AddText(ImVec2(x + 2.0f, origin.y),
                IM_COL32(170, 170, 170, 255), label);
        }

        // 手で描いた分の場所を ImGui に伝える
        ImGui::Dummy(ImVec2(totalFrames * ppf, kRulerHeight));
    }

    void DrawActionBar(const ActionData& action, int playheadFrame, float ppf)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const ImVec4& activeColor = action.unblockable ? kUnblock
            : action.isParry ? kParry
            : kActive;

        float x = 0.0f;
        x += FillSegment(dl, origin, x, action.startup, ppf, kStartup);
        x += FillSegment(dl, origin, x, action.active, ppf, activeColor);
        x += FillSegment(dl, origin, x, action.recovery, ppf, kRecovery);

        // ここから先はキャンセルで割り込める
        if (action.cancelFrom >= 0)
        {
            const float cx = origin.x + action.cancelFrom * ppf;
            dl->AddLine(ImVec2(cx, origin.y - 2.0f),
                ImVec2(cx, origin.y + kBarHeight + 2.0f),
                IM_COL32(20, 20, 20, 220), 2.0f);
        }

        // 今どこを再生しているか
        if (playheadFrame >= 0)
        {
            const float px = origin.x + playheadFrame * ppf;
            dl->AddLine(ImVec2(px, origin.y - 3.0f),
                ImVec2(px, origin.y + kBarHeight + 3.0f),
                IM_COL32(255, 255, 255, 255), 2.0f);
        }

        ImGui::Dummy(ImVec2(action.TotalFrames() * ppf, kBarHeight));
    }

    void DrawPhaseLegend()
    {
        ImGui::TextColored(kStartup, "発生"); ImGui::SameLine();
        ImGui::TextColored(kActive, "判定"); ImGui::SameLine();
        ImGui::TextColored(kRecovery, "硬直"); ImGui::SameLine();
        ImGui::TextColored(kParry, "受付"); ImGui::SameLine();
        ImGui::TextColored(kUnblock, "危"); ImGui::SameLine();
        ImGui::TextDisabled("| 黒線=キャンセル可 白線=再生位置");
    }
    
    void DrawRecordedTrack(const TimelineRecorder& rec, float ppf)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const float  full = rec.Capacity() * ppf;

        // 何もしていない時間も枠として見せる
        dl->AddRectFilled(origin, ImVec2(origin.x + full, origin.y + kTrackHeight),
            IM_COL32(28, 28, 32, 255));

        for (int i = 0; i < rec.Count(); i++)
        {
            const FrameRecord& f = rec.At(i);
            if (f.phase == ActionPhase::None) continue;

            const ImVec4* c = &kRecovery;
            if (f.phase == ActionPhase::Startup)     c = &kStartup;
            else if (f.phase == ActionPhase::Active) c = f.isParry ? &kParry : &kActive;

            const float x = origin.x + i * ppf;
            dl->AddRectFilled(ImVec2(x, origin.y),
                ImVec2(x + ppf, origin.y + kTrackHeight),
                ImGui::GetColorU32(*c));
        }

        // 起きたこと
        for (int i = 0; i < rec.Count(); i++)
        {
            const TimelineEvent e = rec.EventAt(i);
            if (e == TimelineEvent::None) continue;

            const float x = origin.x + i * ppf + ppf * 0.5f;
            const ImU32 col = (e == TimelineEvent::ParrySuccess)
                ? IM_COL32(255, 255, 255, 255)
                : IM_COL32(255, 80, 80, 255);

            dl->AddLine(ImVec2(x, origin.y - 4.0f),
                ImVec2(x, origin.y + kTrackHeight + 4.0f), col, 2.0f);
        }

        ImGui::Dummy(ImVec2(full, kTrackHeight));
    }
}