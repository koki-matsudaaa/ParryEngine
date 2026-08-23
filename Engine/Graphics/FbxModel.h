#pragma once
#include "Engine/Core/Dx12Context.h"
#include "Engine/Graphics/IRenderable.h"
#include "Engine/Animation/AnimationClip.h"
#include "Engine/Animation/Animator.h"
#include <fbxsdk.h>
#include <vector>
#include <string>
#include <algorithm>

namespace Engine
{
    class FbxModel : public IRenderable
    {
    public:

        struct FbxVtx
        {
            XMFLOAT3 position;
            XMFLOAT3 normal;
            XMFLOAT2 uv;
            uint32_t boneIndex[4] = { 0,0,0,0 };   // 影響するボーン番号 (最大4)
            float    boneWeight[4] = { 0,0,0,0 };  // その重み (合計1になる)
        };

        // ボーン１本分の情報
        struct Bone
        {
            std::string name;                    // ボーン名
            XMMATRIX    bindInverse;             // バインドポーズの逆行列
        };

        FbxModel() = default;
        ~FbxModel() = default;

        // FBXを読み込み、GPUリソースを構築する。
        // メッシュ・スケルトンに加え、アニメが入っていれば1本クリップとして登録する。
        bool Load(const std::string& path,
            const std::string& texturePath = "",
            const std::string& defaultClipName = "Default");

        // 別のFBXからモーションだけを読み、クリップとして追加する。
        bool LoadClip(const std::string& name, const std::string& path, bool loop = true);

        // 描画コマンドを積む (IRenderable)。
        void Draw(ID3D12GraphicsCommandList* cmdList) override;

        // ── モーション再生 (Animator へ委譲) ──────────────
        bool Play(const std::string& name, float blendSeconds = 0.0f)
        {
            return m_animator.Play(name, blendSeconds);
        }

        void UpdateAnimation(float dt) { m_animator.Update(dt); }

        bool IsFinished() const { return m_animator.IsFinished(); }
        const std::string& CurrentClipName() const { return m_animator.CurrentClipName(); }
        size_t GetClipCount() const { return m_animator.ClipCount(); }
        int    GetFrameCount() const { return m_animator.CurrentFrameCount(); }
        float  CurrentTime() const { return m_animator.CurrentTime(); }

        bool SetClipLoop(const std::string& name, bool loop)
        {
            return m_animator.SetClipLoop(name, loop);
        }

        void SetFixedFrame(int frame) { m_animator.SetFixedFrame(frame); }

        // IRenderable。描画側が b3 に流すポーズ。
        const std::vector<XMMATRIX>& GetCurrentBoneMatrices() const
        {
            return m_animator.Pose();
        }

        // 再生を細かく制御したいとき (状態遷移など) は直接触る。
        Animator& GetAnimator() { return m_animator; }

        // ボーン数 (定数バッファのサイズ決めに使う)。
        size_t GetBoneCount() const { return m_bones.size(); }

        // IRenderable として、今のポーズを描画側へ渡す。
        const XMMATRIX* GetBoneMatrices() const override
        {
            const auto& mats = GetCurrentBoneMatrices();
            return mats.empty() ? nullptr : mats.data();
        }

        size_t GetBoneMatrixCount() const override
        {
            return GetCurrentBoneMatrices().size();
        }

        // 地形などボーンの無いモデルは Static パイプラインで描く。
        PipelineType GetPipelineType() const override
        {
            return m_useStaticPipeline ? PipelineType::Static : PipelineType::FBX;
        }

        // このモデルをStaticパイプライン(ボーンなし)で描くようにする。地形など。
        void SetStaticPipeline(bool on) { m_useStaticPipeline = on; }

        // 読み込んだ全頂点の最小・最大 (当たり判定の半径算出などに使える)。
        XMFLOAT3 GetMin() const { return m_min; }
        XMFLOAT3 GetMax() const { return m_max; }


        const std::vector<FbxVtx>& GetVertices() const { return m_vertices; }
        const std::vector<unsigned int>& GetIndices() const { return m_indices; }

    private:
        // FbxManager は SDK 全体で1つあれば足りるので static 共有する。
        // 初回ロード時に作られ、以降使い回す。
        static FbxManager* GetSharedManager();

        // 1つのメッシュから頂点(位置)とインデックスを取り出して詰める。
        void ReadMesh(FbxMesh* mesh);

        // 集めた頂点/インデックスで GPU バッファを作る。
        void BuildBuffers();

        // FBX を読み込んでシーンを返す。
        FbxScene* ImportScene(const std::string& path);

        // シーン内の全メッシュから、頂点・ボーン・スキンウェイトを読む。
        void ReadMeshes(FbxScene* scene);

        // バインド逆行列を Animator へ渡す。
        void SetupSkeleton();

        // テクスチャを読み、t0 用の SRV ヒープを作る。
        void CreateTextureResources(const std::string& texturePath);

        // メッシュのスキンからボーン情報を読み取る。
        void ReadBones(FbxMesh* mesh);

        // スキンウェイトを読む (段2)。頂点ごとに、影響するボーンと重みを集める。
        void ReadSkinWeights(FbxMesh* mesh, unsigned int vertexBase);

        // FbxAMatrix を XMMATRIX に変換する。
        XMMATRIX ToXMMatrix(const FbxAMatrix& m);

        // アニメーションを一定間隔で焼き込む。
        bool BakeClip(FbxScene* scene, AnimationClip& clip);

        std::string m_path;   // 読み込み元のパス。ログでどのモデルか分かるように

        std::vector<FbxVtx>        m_vertices; // 全メッシュ分をまとめて持つ
        std::vector<unsigned int>  m_indices;

        std::vector<Bone> m_bones;                       // ボーン一覧
        std::map<std::string, int> m_boneIndexByName;    // 名前 → ボーン番号

        Animator m_animator;   // 再生・ブレンド・ポーズ生成はすべてこの中

        // GPUリソース
        ID3D12Resource* m_vertBuff = nullptr;
        ID3D12Resource* m_idxBuff = nullptr;
        D3D12_VERTEX_BUFFER_VIEW  m_vbView = {};
        D3D12_INDEX_BUFFER_VIEW   m_ibView = {};

        // 頂点の最小・最大
        XMFLOAT3 m_min{ 0,0,0 };
        XMFLOAT3 m_max{ 0,0,0 };

        // テクスチャ関連
        bool m_useStaticPipeline = false; // trueならStatic(ボーンなし)で描く
        ID3D12Resource* m_texture = nullptr;
        ID3D12DescriptorHeap* m_texHeap = nullptr;
    };
}