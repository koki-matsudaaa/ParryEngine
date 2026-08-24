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

    struct ActionData
    {
        std::string name;            // ゲーム側が指定する名前
        std::string clipName;        // 再生するアニメクリップの名前
        float blendSeconds = 0.1f;   // クリップへの移り変わりにかける秒

        int startup = 0;    // 発生まで
        int active = 0;     // 判定が出ている
        int recovery = 0;   // 硬直

        // キャンセルを受け付け始めるフレーム。-1 ならキャンセル不可。
        int cancelFrom = -1;

        // キャンセルで移れるアクション名
        std::vector<std::string> cancelTo;

        int TotalFrames() const { return startup + active + recovery; }
    };
}