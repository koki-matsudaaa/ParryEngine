#pragma once
#include "Dx12Context.h"

// スプライト1枚ぶんのテクスチャと、その SRV ディスクリプタヒープ。
// 作り方は FloorPlateModel::Create() のテクスチャ部分と同じ。
class SpriteTexture
{
public:
    // 読めなければ白テクスチャで代用する (画像を用意する前でも動く)。
    bool Load(const std::string& path);

    // 画像を使わず、1x1 の白テクスチャだけ作る。
    // 単色の矩形 (照準・ゲージなど) を描くのに使う。
    bool CreateWhite();

    void Destroy();

    ID3D12DescriptorHeap* GetHeap() const { return m_texHeap; }

    // 実ファイルが読めたかどうか。白テクスチャ代用なら false。
    bool IsLoaded() const { return m_loaded; }

private:
    // m_texture から SRV ディスクリプタヒープを作る (Load / CreateWhite 共通)。
    bool CreateView();

    ID3D12Resource* m_texture = nullptr;
    ID3D12DescriptorHeap* m_texHeap = nullptr;
    bool                  m_loaded = false;
};


// 2D スプライト用の描画パイプライン。
// 3D 用 (PMD/FBX/Static) とは別のルートシグネチャ・PSO を持ち、
// 深度テストなし・αブレンドありで画面のピクセル座標に直接描く。
class SpriteRenderPipeline
{
public:
    bool Create();
    void Destroy();

    // このフレームのスプライト描画を始める。PSO とルートシグネチャを張り替える。
    void Begin(ID3D12GraphicsCommandList* cmdList);

    // 画面ピクセル座標 (左上原点) に1枚描く。
    void Draw(ID3D12GraphicsCommandList* cmdList,
        ID3D12DescriptorHeap* texHeap,
        float x, float y, float w, float h,
        const XMFLOAT2& uvOffset,
        const XMFLOAT2& uvSize,
        const XMFLOAT4& color);

private:
    // 頂点シェーダ・ピクセルシェーダ共通の定数バッファ。
    // HLSL 側の cbuffer SpriteData と並びを合わせること。
    struct SpriteData
    {
        XMFLOAT2 posPx;
        XMFLOAT2 sizePx;
        XMFLOAT2 uvOffset;
        XMFLOAT2 uvSize;
        XMFLOAT4 color;
        XMFLOAT2 screenSize;
        XMFLOAT2 padding;
    };

    struct Vertex
    {
        XMFLOAT2 pos; // 0..1 の単位クアッド
        XMFLOAT2 uv;
    };

    static const UINT kMaxSprites = 128; // 1フレームに描ける枚数
    // 定数バッファの GPU アドレスは 256 バイト境界が必須なので切り上げる。
    static const UINT kSlotSize = (UINT)((sizeof(SpriteData) + 0xff) & ~0xffu);

    ID3D12RootSignature* m_rootSig = nullptr;
    ID3D12PipelineState* m_pipeline = nullptr;

    ID3D12Resource* m_vertBuff = nullptr;
    D3D12_VERTEX_BUFFER_VIEW m_vbView = {};

    ID3D12Resource* m_constBuff = nullptr;
    uint8_t* m_mapped = nullptr;
    UINT            m_slot = 0; // このフレームで使った定数バッファのスロット数
};