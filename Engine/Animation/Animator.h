#pragma once
#include "Engine/Animation/AnimationClip.h"

#include <DirectXMath.h>
#include <map>
#include <string>
#include <vector>

namespace Engine
{
    class Animator
    {
    public:
        // スケルトンのバインド逆行列
        void SetBindInverses(std::vector<DirectX::XMMATRIX> binds);

        // クリップを登録
        void AddClip(AnimationClip&& clip);

        // 再生を切り替える
        bool Play(const std::string& name, float blendSeconds = 0.0f, bool restart = false);

        void Update(float dt);

        // 組み立て済みの最終ポーズ
        const std::vector<DirectX::XMMATRIX>& Pose() const { return m_pose; }

        // ループしないクリップが末尾まで再生されたか
        bool IsFinished() const;

        const std::string& CurrentClipName() const;
        float  CurrentTime() const { return m_time; }
        size_t ClipCount() const { return m_clips.size(); }
        int    CurrentFrameCount() const;

        bool SetClipLoop(const std::string& name, bool loop);

        // デバッグ用
        void SetFixedFrame(int frame) { m_fixedFrame = frame; }

    private:
        // クリップの指定時刻のフレームを返す。
        const std::vector<DirectX::XMMATRIX>* SampleClip(int clipIndex, float time) const;

        // 今のポーズを組み立てる
        void BuildPose();

        std::vector<DirectX::XMMATRIX> m_bindInverses;

        std::vector<AnimationClip> m_clips;
        std::map<std::string, int> m_clipIndexByName;

        int   m_currentClip = -1;   // 再生中
        float m_time = 0.0f;        // 現在の再生時刻

        // ── クロスフェード ──
        int   m_prevClip = -1;        // 切り替え元
        float m_prevTime = 0.0f;
        float m_blendTime = 0.0f;     // ブレンド開始からの経過秒
        float m_blendDuration = 0.0f; // ブレンドにかける秒数

        int m_fixedFrame = -1;

        // 合成後の最終行列
        std::vector<DirectX::XMMATRIX> m_pose;
    };
}