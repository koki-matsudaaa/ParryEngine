#pragma once
#include <d3d12.h>
#include "Engine/Graphics/IRenderable.h"

namespace Engine
{
    class PipelineCache
    {
    public:
        PipelineCache() = default;
        ~PipelineCache() = default;

        PipelineCache(const PipelineCache&) = delete;
        PipelineCache& operator=(const PipelineCache&) = delete;

        // シェーダをコンパイルし、ルートシグネチャと PSO を作る
        bool Initialize();
        void Shutdown();

        //   b0 : シーン定数
        //   b2 : オブジェクト定数
        //   t0 : テクスチャ
        //   b3 : ボーン行列
        ID3D12RootSignature* RootSignature() const { return m_rootSignature; }

        // 種別に対応するPSO
        ID3D12PipelineState* Get(PipelineType type) const;

        // 線を描くための PSO。当たり判定の可視化に使う。
        ID3D12PipelineState* LinePso() const { return m_linePso; }

    private:
        bool CreateRootSignature();
        bool CreateStaticPipeline();
        bool CreateSkeletalPipeline();
        bool CreateLinePipeline();

        ID3D12RootSignature* m_rootSignature = nullptr;
        ID3D12PipelineState* m_staticPso = nullptr;
        ID3D12PipelineState* m_skeletalPso = nullptr;
        ID3D12PipelineState* m_linePso = nullptr;
    };
}