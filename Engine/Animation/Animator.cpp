#include "Engine/Animation/Animator.h"

using namespace DirectX;

namespace Engine
{
    void Animator::SetBindInverses(std::vector<XMMATRIX> binds)
    {
        m_bindInverses = std::move(binds);
        m_pose.assign(m_bindInverses.size(), XMMatrixIdentity());
        BuildPose();
    }

    void Animator::AddClip(AnimationClip&& clip)
    {
        auto it = m_clipIndexByName.find(clip.name);
        if (it != m_clipIndexByName.end())
        {
            m_clips[it->second] = std::move(clip);   // 同名は差し替え
            return;
        }

        m_clipIndexByName[clip.name] = static_cast<int>(m_clips.size());
        m_clips.push_back(std::move(clip));

        // 最初の1本は自動で再生対象にする。
        if (m_currentClip < 0) { m_currentClip = 0; m_time = 0.0f; }

        BuildPose();
    }

    bool Animator::Play(const std::string& name, float blendSeconds, bool restart)
    {
        auto it = m_clipIndexByName.find(name);
        if (it == m_clipIndexByName.end()) return false;

        if (!restart && m_currentClip == it->second) return true;   // 既に再生中

        // 今のクリップを「切り替え元」として取っておき、そこから移り変わる。
        if (blendSeconds > 0.0f && m_currentClip >= 0)
        {
            m_prevClip = m_currentClip;
            m_prevTime = m_time;
            m_blendTime = 0.0f;
            m_blendDuration = blendSeconds;
        }
        else
        {
            m_prevClip = -1;          // 即座に切り替える
            m_blendDuration = 0.0f;
        }

        m_currentClip = it->second;
        m_time = 0.0f;

        BuildPose();
        return true;
    }

    void Animator::Update(float dt)
    {
        // 再生中のクリップを進める。
        if (m_currentClip >= 0)
        {
            const AnimationClip& clip = m_clips[m_currentClip];
            if (clip.duration > 0.0f)
            {
                m_time += dt;
                if (clip.loop)
                {
                    while (m_time >= clip.duration) m_time -= clip.duration;
                }
                else if (m_time > clip.duration)
                {
                    m_time = clip.duration;   // ループしないクリップは末尾で止める
                }
            }
        }

        // ブレンド中は、切り替え元も動かし続ける。
        // 止めると、移り変わりの途中で元のポーズが固まって見える。
        if (m_prevClip >= 0)
        {
            const AnimationClip& prev = m_clips[m_prevClip];
            if (prev.duration > 0.0f)
            {
                m_prevTime += dt;
                if (prev.loop)
                {
                    while (m_prevTime >= prev.duration) m_prevTime -= prev.duration;
                }
                else if (m_prevTime > prev.duration)
                {
                    m_prevTime = prev.duration;
                }
            }

            m_blendTime += dt;
            if (m_blendTime >= m_blendDuration)
            {
                m_prevClip = -1;          // 移り変わり完了
                m_blendDuration = 0.0f;
            }
        }

        BuildPose();
    }

    bool Animator::IsFinished() const
    {
        if (m_currentClip < 0) return false;

        const AnimationClip& clip = m_clips[m_currentClip];
        if (clip.loop || clip.duration <= 0.0f) return false;

        return m_time >= clip.duration;
    }

    const std::string& Animator::CurrentClipName() const
    {
        static const std::string empty;
        return (m_currentClip >= 0) ? m_clips[m_currentClip].name : empty;
    }

    int Animator::CurrentFrameCount() const
    {
        return (m_currentClip >= 0) ? m_clips[m_currentClip].FrameCount() : 0;
    }

    bool Animator::SetClipLoop(const std::string& name, bool loop)
    {
        auto it = m_clipIndexByName.find(name);
        if (it == m_clipIndexByName.end()) return false;

        m_clips[it->second].loop = loop;
        return true;
    }

    const std::vector<XMMATRIX>* Animator::SampleClip(int clipIndex, float time) const
    {
        if (clipIndex < 0 || clipIndex >= static_cast<int>(m_clips.size())) return nullptr;

        const AnimationClip& clip = m_clips[clipIndex];
        if (clip.frames.empty()) return nullptr;

        // 時刻 → フレーム番号 (一番近いフレームを選ぶ簡易版)。
        int frame = (m_fixedFrame >= 0)
            ? m_fixedFrame
            : static_cast<int>(time * clip.fps);

        if (frame < 0) frame = 0;
        if (frame >= clip.FrameCount()) frame = clip.FrameCount() - 1;

        const std::vector<XMMATRIX>& f = clip.frames[frame];
        return (f.size() == m_bindInverses.size()) ? &f : nullptr;
    }

    void Animator::BuildPose()
    {
        const size_t boneCount = m_bindInverses.size();
        if (boneCount == 0) { m_pose.clear(); return; }

        if (m_pose.size() != boneCount)
            m_pose.assign(boneCount, XMMatrixIdentity());

        const std::vector<XMMATRIX>* cur = SampleClip(m_currentClip, m_time);

        // クリップが1本も無ければ変形なし (バインドポーズのまま)。
        if (!cur)
        {
            for (auto& m : m_pose) m = XMMatrixIdentity();
            return;
        }

        const std::vector<XMMATRIX>* prev =
            (m_blendDuration > 0.0f) ? SampleClip(m_prevClip, m_prevTime) : nullptr;

        // ブレンドしていないときは、バインド逆行列を掛けるだけ。
        if (!prev)
        {
            for (size_t b = 0; b < boneCount; b++)
                m_pose[b] = m_bindInverses[b] * (*cur)[b];
            return;
        }

        float t = m_blendTime / m_blendDuration;   // 0 = 切り替え元, 1 = 切り替え先
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        for (size_t b = 0; b < boneCount; b++)
        {
            // 行列をそのまま混ぜると回転が縮んでモデルが潰れる。
            // 拡大・回転・平行移動に分解し、回転だけ球面補間で混ぜる。
            XMVECTOR sA, rA, pA, sB, rB, pB;
            if (XMMatrixDecompose(&sA, &rA, &pA, (*prev)[b]) &&
                XMMatrixDecompose(&sB, &rB, &pB, (*cur)[b]))
            {
                const XMVECTOR s = XMVectorLerp(sA, sB, t);
                const XMVECTOR r = XMQuaternionSlerp(rA, rB, t);
                const XMVECTOR p = XMVectorLerp(pA, pB, t);

                const XMMATRIX g = XMMatrixAffineTransformation(
                    s, XMVectorZero(), r, p);

                m_pose[b] = m_bindInverses[b] * g;
            }
            else
            {
                // 分解できない場合は切り替え先をそのまま使う。
                m_pose[b] = m_bindInverses[b] * (*cur)[b];
            }
        }
    }
}