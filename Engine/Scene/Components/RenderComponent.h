#pragma once
#include "GameObject.h"
#include "IRenderable.h"

// 「この GameObject は、どのモデルを描くか」の対応づけだけを持つ。
// 重いGPUリソース(頂点バッファ等)はモデル側が持ち、ここは借りて指すだけ。
// だから同じモデルを複数のオブジェクト(弾を100発など)が共有できる。
class RenderComponent : public GameComponent
{
private:
    IRenderable* m_model = nullptr;   // 所有しない。寿命は外(Scene等)が持つ。
    bool m_visible = true;            // false の間は描画リストに登録しない。

public:
    // 描くモデルを差し込む。PmdModel* でも将来の FbxModel* でも入る
    // (どちらも IRenderable を継承しているため)。
    void SetModel(IRenderable* model)
    {
        m_model = model;
    }

    IRenderable* GetModel() const
    {
        return m_model;
    }

    void SetVisible(bool v) { m_visible = v; }
    bool IsVisible() const { return m_visible; }

    // 毎フレーム呼ばれる。今は「自分が描画対象である」と知らせるだけ。
    // 実際の描画リスト登録は Scene 側の仕組みと繋いだ段階で実装する。
    // (このコンポーネント自体はロジックを持たないので常に生存 = true)
    bool Update(float /*dt*/) override
    {
        return true;
    }
};