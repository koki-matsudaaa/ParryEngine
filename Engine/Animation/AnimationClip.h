#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>

namespace Engine
{
    struct AnimationClip
    {
        std::string name;        // ゲーム側が使う名前 ("Idle" "Slash" など)
        float fps = 30.0f;       // 何分の1秒ごとに焼いたか
        float duration = 0.0f;   // 長さ (秒)
        bool  loop = true;       // 末尾まで来たら先頭へ戻すか

        // frames[フレーム番号][ボーン番号] = そのボーンのグローバル変換。
        // バインド逆行列はまだ掛けていない。掛けるのはポーズを組み立てる最後。
        // 先に掛けてしまうと、ブレンドのときに回転を正しく補間できない。
        std::vector<std::vector<DirectX::XMMATRIX>> frames;

        int FrameCount() const { return static_cast<int>(frames.size()); }
    };
}