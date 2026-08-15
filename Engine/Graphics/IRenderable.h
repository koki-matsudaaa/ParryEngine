#pragma once
#include "Engine/Core/pch.h"

// 描画に使うパイプラインの種別。
// モデルごとに頂点構造・シェーダが違うため、どのパイプラインで描くかを
// モデル自身に答えさせる。描画ループは型を知らずに種別だけ見て切り替える。
enum class PipelineType
{
    PMD,
    FBX,
    Static,  // 静的メッシュ
};

class IRenderable
{
public:
    virtual ~IRenderable() = default;

    // 自分自身の描画コマンドを cmdList に積む。
    virtual void Draw(ID3D12GraphicsCommandList* cmdList) = 0;

    // 自分をどのパイプラインで描くべきか答える。
    // 描画ループはこれを見て、種別が変わったときだけパイプラインを切り替える。
    virtual PipelineType GetPipelineType() const = 0;

    // スキニングを使うモデルは、今のポーズのボーン行列を返す。
    // 描画側はこれを見て、必要なときだけ b3 に流し込む。
    virtual const XMMATRIX* GetBoneMatrices() const { return nullptr; }
    virtual size_t          GetBoneMatrixCount() const { return 0; }
};