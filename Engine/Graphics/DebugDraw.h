#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>

namespace Engine
{
    class DebugDraw
    {
    public:
        bool Initialize(ID3D12PipelineState* pso);
        void Shutdown();

        void Clear() { m_lines.clear(); }

        void Line(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b,
            const DirectX::XMFLOAT3& color);

        // axis: 0 = XY 面, 1 = YZ 面, 2 = ZX 面
        void Circle(const DirectX::XMFLOAT3& center, float radius, int axis,
            const DirectX::XMFLOAT3& color);

        // 円を3枚重ねた球
        void Sphere(const DirectX::XMFLOAT3& center, float radius,
            const DirectX::XMFLOAT3& color);

        // 2点を結ぶカプセル。当たり判定の形はこれを使う。
        void Capsule(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b,
            float radius, const DirectX::XMFLOAT3& color);

        void Flush(ID3D12GraphicsCommandList* cmdList);

    private:
        struct Vtx
        {
            DirectX::XMFLOAT3 pos;
            DirectX::XMFLOAT3 color;
        };

        static constexpr size_t kMaxVertices = 8192;

        ID3D12PipelineState* m_pso = nullptr;
        ID3D12Resource* m_vertBuff = nullptr;
        Vtx* m_mapped = nullptr;
        D3D12_VERTEX_BUFFER_VIEW m_vbView{};
        std::vector<Vtx> m_lines;
    };
}