#pragma once
#include <d3d12.h>
#include "Engine/Graphics/IRenderable.h"

namespace Engine
{
    // ────────────────────────────────────────────────────────────
    // ルートシグネチャと、頂点構造ごとのパイプラインステートを持つ。
    //
    // モデルは「自分はどの種別か」を答えるだけでよく (IRenderable)、
    // 実際の PSO の実体はここに集約されている。
    // 描画側は種別を見て Get() するだけで切り替えられる。
    // ────────────────────────────────────────────────────────────
    class PipelineCache
    {
    public:
        PipelineCache() = default;
        ~PipelineCache() = default;

        PipelineCache(const PipelineCache&) = delete;
        PipelineCache& operator=(const PipelineCache&) = delete;

        // シェーダをコンパイルし、ルートシグネチャと PSO を作る。
        bool Initialize();
        void Shutdown();

        // 全パイプライン共通のルートシグネチャ。
        //   b0 : シーン定数 (view / proj / eye)
        //   b2 : オブジェクト定数 (world)
        //   t0 : テクスチャ
        //   b3 : ボーン行列 (スキニング用)
        ID3D12RootSignature* RootSignature() const { return m_rootSignature; }

        // 種別に対応する PSO。未対応なら nullptr。
        ID3D12PipelineState* Get(PipelineType type) const;

    private:
        bool CreateRootSignature();
        bool CreateStaticPipeline();

        ID3D12RootSignature* m_rootSignature = nullptr;
        ID3D12PipelineState* m_staticPso = nullptr;
    };
}