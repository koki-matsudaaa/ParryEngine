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

        // 再生するモーションを選ぶ
        void ClipCombo(const char* label, std::string& target, const Animator* anim)
        {
            const char* preview = target.empty() ? "(なし)" : target.c_str();
            if (!ImGui::BeginCombo(label, preview)) return;

            if (ImGui::Selectable("(なし)", target.empty()))
                target.clear();

            if (anim)
            {
                for (size_t i = 0; i < anim->ClipCount(); i++)
                {
                    const std::string& n = anim->ClipName(i);
                    if (ImGui::Selectable(n.c_str(), target == n))
                        target = n;
                }
            }
            ImGui::EndCombo();
        }

        // 判定を付けるボーンを選ぶ
        void HitBoneCombo(ActionData& a, const std::vector<std::string>* bones)
        {
            if (!bones) return;

            const char* preview = a.hitBone.empty() ? "(既定)" : a.hitBone.c_str();
            if (!ImGui::BeginCombo("ボーン", preview)) return;

            if (ImGui::Selectable("(既定)", a.hitBone.empty()))
                a.hitBone.clear();

            for (const auto& n : *bones)
            {
                if (ImGui::Selectable(n.c_str(), a.hitBone == n))
                    a.hitBone = n;
            }
            ImGui::EndCombo();
        }
    }

    void DrawActionEditor(ActionStateMachine& sm, float ppf, const std::vector<std::string>* boneNames)
    {
        ImGui::PushID(&sm);
        auto& all = sm.Actions();

        for (auto& a : all)
        {
            ImGui::PushID(a.name.c_str());

            const bool open = ImGui::CollapsingHeader(a.name.c_str());

            const bool running = (sm.CurrentActionName() == a.name);
            DrawActionBar(a, running ? sm.ElapsedFrames() : -1, ppf);

            if (open)
            {
                ClipCombo("モーション", a.clipName, sm.GetAnimator());
                ImGui::SliderInt("発生", &a.startup, 0, 150);
                ImGui::SliderInt("判定", &a.active, 0, 60);
                ImGui::SliderInt("硬直", &a.recovery, 0, 90);
                ImGui::SliderInt("キャンセル", &a.cancelFrom, -1, 90);
                ImGui::SliderFloat("体幹削り", &a.postureDamage, 0.0f, 100.0f, "%.0f");
                ImGui::SliderInt("AIの重み", &a.aiWeight, 0, 10);
                ImGui::SliderFloat("届く距離", &a.range, 0.5f, 5.0f, "%.1f m");
                ImGui::SliderInt("弾かれた隙", &a.deflectFrames, 0, 120);
                ClipCombo("弾かれモーション", a.deflectClip, sm.GetAnimator());

                if (a.isAttack && ImGui::TreeNode("当たり判定"))
                {
                    HitBoneCombo(a, boneNames);
                    ImGui::SliderFloat3("位置", &a.hitOffset.x, -1.0f, 1.0f, "%.2f m");
                    ImGui::SliderFloat3("向き", &a.hitRotation.x, -180.0f, 180.0f, "%.0f 度");
                    ImGui::SliderFloat("長さ", &a.hitLength, 0.1f, 2.0f, "%.2f m");
                    ImGui::SliderFloat("太さ", &a.hitRadius, 0.01f, 0.4f, "%.2f m");
                    ImGui::TreePop();
                }

                ImGui::Checkbox("攻撃", &a.isAttack);
                ImGui::SameLine();
                ImGui::Checkbox("パリィ技", &a.isParry);
                ImGui::SameLine();
                ImGui::Checkbox("弾けない", &a.unblockable);
                ImGui::SameLine();
                ImGui::Checkbox("回避技", &a.isDodge);

                ImGui::Checkbox("ガード技", &a.isGuard);
                ImGui::SameLine();
                ImGui::Checkbox("ホールド", &a.holdable);

                ImGui::SliderFloat("移動速度", &a.moveSpeed, 0.0f, 12.0f, "%.1f m/s");

                NextActionCombo(a, all);
                CancelToTree(a, all);

                ImGui::Text("合計 %d F (%.2f 秒)",
                    a.TotalFrames(), a.TotalFrames() / 60.0f);
                ImGui::Spacing();
            }
            ImGui::PopID();
        }

        ImGui::Separator();

        static char newName[32] = "";
        ImGui::InputText("新しい技の名前", newName, sizeof(newName));
        ImGui::SameLine();
        if (ImGui::Button("追加") && newName[0] != '\0')
        {
            ActionData a;
            a.name = newName;
            a.startup = 20;
            a.active = 5;
            a.recovery = 20;
            a.isAttack = true;
            sm.AddAction(a);
            newName[0] = '\0';
        }
        ImGui::PopID();
    }
}