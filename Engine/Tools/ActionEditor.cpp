#include "Engine/Tools/ActionEditor.h"
#include "Engine/Tools/Timeline.h"
#include "imgui.h"

#include <algorithm>

namespace Engine
{
    namespace
    {
        // 終わったあとに繋ぐアクションを選ぶ
        void NextActionCombo(ActionData& a, const std::vector<ActionData>& all)
        {
            const char* preview = a.nextAction.empty() ? "(なし)" : a.nextAction.c_str();
            if (!ImGui::BeginCombo("連撃の次", preview)) return;

            if (ImGui::Selectable("(なし)", a.nextAction.empty()))
                a.nextAction.clear();

            for (const auto& other : all)
            {
                if (other.name == a.name) continue;
                if (ImGui::Selectable(other.name.c_str(), a.nextAction == other.name))
                    a.nextAction = other.name;
            }
            ImGui::EndCombo();
        }

        // キャンセルで移れるアクション
        void CancelToTree(ActionData& a, const std::vector<ActionData>& all)
        {
            if (!ImGui::TreeNode("キャンセル先")) return;

            for (const auto& other : all)
            {
                auto it = std::find(a.cancelTo.begin(), a.cancelTo.end(), other.name);
                bool on = (it != a.cancelTo.end());

                if (ImGui::Checkbox(other.name.c_str(), &on))
                {
                    if (on) a.cancelTo.push_back(other.name);
                    else    a.cancelTo.erase(it);
                }
            }
            if (a.cancelTo.empty())
                ImGui::TextDisabled("空 = 何にでも移れる");
            ImGui::TreePop();
        }
    }

    void DrawActionEditor(ActionStateMachine& sm, float ppf)
    {
        auto& all = sm.Actions();

        for (auto& a : all)
        {
            ImGui::PushID(a.name.c_str());

            const bool open = ImGui::CollapsingHeader(a.name.c_str());

            const bool running = (sm.CurrentActionName() == a.name);
            DrawActionBar(a, running ? sm.ElapsedFrames() : -1, ppf);

            if (open)
            {
                ImGui::SliderInt("発生", &a.startup, 0, 60);
                ImGui::SliderInt("判定", &a.active, 0, 60);
                ImGui::SliderInt("硬直", &a.recovery, 0, 90);
                ImGui::SliderInt("キャンセル", &a.cancelFrom, -1, 90);
                ImGui::SliderFloat("体幹削り", &a.postureDamage, 0.0f, 100.0f, "%.0f");

                ImGui::Checkbox("パリィ技", &a.isParry);
                ImGui::SameLine();
                ImGui::Checkbox("弾けない", &a.unblockable);

                NextActionCombo(a, all);
                CancelToTree(a, all);

                ImGui::Text("合計 %d F (%.2f 秒)",
                    a.TotalFrames(), a.TotalFrames() / 60.0f);
                ImGui::Spacing();
            }
            ImGui::PopID();
        }
    }
}