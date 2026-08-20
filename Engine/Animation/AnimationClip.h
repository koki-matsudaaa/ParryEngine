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

        // frames[フレーム番号][ボーン番号] = そのボーンの最終行列
        std::vector<std::vector<DirectX::XMMATRIX>> frames;

        int FrameCount() const { return static_cast<int>(frames.size()); }
    };
}