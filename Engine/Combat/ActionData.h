#pragma once
#include <string>
#include <vector>

namespace Engine
{
    enum class ActionPhase
    {
        None,       // 何もしていない
        Startup,    // 発生前
        Active,     // 判定が出ている
        Recovery,   // 硬直
    };

    inline const char* ToString(ActionPhase phase)
    {
        switch (phase)
        {
        case ActionPhase::Startup:  return "発生";
        case ActionPhase::Active:   return "判定";
        case ActionPhase::Recovery: return "硬直";
        default:                    return "----";
        }
    }

    struct ActionData
    {
        std::string name;            // ゲーム側が指定する名前
        std::string clipName;        // 再生するアニメクリップの名前
        float blendSeconds = 0.1f;   // クリップへの移り変わりにかける秒

        int startup = 0;    // 発生まで
        int active = 0;     // 判定が出ている
        int recovery = 0;   // 硬直

        // キャンセルを受け付け始めるフレーム
        int cancelFrom = -1;

        bool isParry = false;

        // 体幹削り量
        float postureDamage = 20.0f;

        // キャンセルで移れるアクション
        std::vector<std::string> cancelTo;

        // 終了時に自動で繋ぐアクション
        std::string nextAction;

        // 弾き不可の攻撃
        bool unblockable = false;

        // 敵攻撃の選択の重み
        int aiWeight = 0;

        int TotalFrames() const { return startup + active + recovery; }
    };
}