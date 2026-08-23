#pragma once
#include "Engine/Animation/AnimationClip.h"

#include <DirectXMath.h>
#include <map>
#include <string>
#include <vector>

namespace Engine
{
    // ────────────────────────────────────────────────────────────
    // モーションの再生を受け持つ。
    //
    // どのクリップを、どこまで、何と混ぜて再生しているかを持ち、
    // 毎フレーム最終ポーズ (ボーン行列の配列) を組み立てる。
    //
    // FBX もメッシュも GPU も知らない。必要なのはクリップと
    // バインド逆行列だけ。この先の状態遷移はこのクラスを叩く。
    // ────────────────────────────────────────────────────────────
    class Animator
    {
    public:
        // スケルトンのバインド逆行列。モデルを読み終えた時点で1回渡す。
        void SetBindInverses(std::vector<DirectX::XMMATRIX> binds);

        // クリップを登録する。同名があれば差し替える。
        // 最初の1本は自動で再生対象になる。
        void AddClip(AnimationClip&& clip);

        // 再生を切り替える。blendSeconds > 0 なら滑らかに移り変わる。
        bool Play(const std::string& name, float blendSeconds = 0.0f);

        // 毎フレーム呼ぶ。時刻を進めてポーズを作り直す。
        void Update(float dt);

        // 組み立て済みの最終ポーズ。描画側はこれを b3 へ流す。
        const std::vector<DirectX::XMMATRIX>& Pose() const { return m_pose; }

        // ループしないクリップが末尾まで再生されたか。
        bool IsFinished() const;

        const std::string& CurrentClipName() const;
        float  CurrentTime() const { return m_time; }
        size_t ClipCount() const { return m_clips.size(); }
        int    CurrentFrameCount() const;

        bool SetClipLoop(const std::string& name, bool loop);

        // デバッグ用。指定フレームで固定する。-1 で解除。
        void SetFixedFrame(int frame) { m_fixedFrame = frame; }

    private:
        // クリップの指定時刻のフレームを返す。範囲外なら nullptr。
        const std::vector<DirectX::XMMATRIX>* SampleClip(int clipIndex, float time) const;

        // 今のポーズを組み立てる。ブレンド中なら2本を混ぜる。
        void BuildPose();

        std::vector<DirectX::XMMATRIX> m_bindInverses;

        std::vector<AnimationClip> m_clips;
        std::map<std::string, int> m_clipIndexByName;

        int   m_currentClip = -1;   // 今再生中 (-1 = 無し)
        float m_time = 0.0f;        // 現在の再生時刻 (秒)

        // ── クロスフェード ──
        int   m_prevClip = -1;        // 切り替え元 (-1 = ブレンドしていない)
        float m_prevTime = 0.0f;
        float m_blendTime = 0.0f;     // ブレンド開始からの経過秒
        float m_blendDuration = 0.0f; // ブレンドにかける秒数

        int m_fixedFrame = -1;

        // 合成後の最終行列。Pose() が返すのはこれ。
        std::vector<DirectX::XMMATRIX> m_pose;
    };
}