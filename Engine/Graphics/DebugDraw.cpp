#include "Engine/Graphics/DebugDraw.h"
#include "Engine/Core/Dx12Context.h"

#include <cmath>
#include <cstring>

using namespace DirectX;

namespace Engine
{
    bool DebugDraw::Initialize(ID3D12PipelineState* pso)
    {
        m_pso = pso;

        auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(kMaxVertices * sizeof(Vtx));

        if (FAILED(_dev->CreateCommittedResource(
            &heapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&m_vertBuff))))
            return false;

        // 毎フレーム書き換える
        if (FAILED(m_vertBuff->Map(0, nullptr,
            reinterpret_cast<void**>(&m_mapped))))
            return false;

        m_vbView.BufferLocation = m_vertBuff->GetGPUVirtualAddress();
        m_vbView.SizeInBytes = static_cast<UINT>(kMaxVertices * sizeof(Vtx));
        m_vbView.StrideInBytes = sizeof(Vtx);

        m_lines.reserve(kMaxVertices);
        return true;
    }

    void DebugDraw::Shutdown()
    {
        if (m_vertBuff)
        {
            m_vertBuff->Unmap(0, nullptr);
            m_vertBuff->Release();
            m_vertBuff = nullptr;
        }
        m_mapped = nullptr;
    }

    void DebugDraw::Line(const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT3& color)
    {
        if (m_lines.size() + 2 > kMaxVertices) return;   // 溢れたら捨てる

        m_lines.push_back({ a, color });
        m_lines.push_back({ b, color });
    }

    void DebugDraw::Circle(const XMFLOAT3& c, float r, int axis, const XMFLOAT3& color)
    {
        const int kSeg = 16;
        XMFLOAT3 prev{};

        for (int i = 0; i <= kSeg; i++)
        {
            const float t = (XM_2PI * i) / kSeg;
            const float s = std::sin(t) * r;
            const float q = std::cos(t) * r;

            XMFLOAT3 p = c;
            if (axis == 0) { p.x += q; p.y += s; }
            else if (axis == 1) { p.y += q; p.z += s; }
            else { p.z += q; p.x += s; }

            if (i > 0) Line(prev, p, color);
            prev = p;
        }
    }

    void DebugDraw::Sphere(const XMFLOAT3& c, float r, const XMFLOAT3& color)
    {
        Circle(c, r, 0, color);
        Circle(c, r, 1, color);
        Circle(c, r, 2, color);
    }

    void DebugDraw::Capsule(const XMFLOAT3& a, const XMFLOAT3& b,
        float radius, const XMFLOAT3& color)
    {
        Sphere(a, radius, color);
        Sphere(b, radius, color);

        // 軸の向き
        float dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
        const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (len < 0.0001f) return;
        dx /= len; dy /= len; dz /= len;

        // 軸と平行でない向きを1つ選ぶ
        float ux = 0.0f, uy = 1.0f, uz = 0.0f;
        if (std::fabs(dy) > 0.99f) { ux = 1.0f; uy = 0.0f; }

        // 右 = 軸 × 上
        float rx = dy * uz - dz * uy;
        float ry = dz * ux - dx * uz;
        float rz = dx * uy - dy * ux;
        const float rl = std::sqrt(rx * rx + ry * ry + rz * rz);
        rx /= rl; ry /= rl; rz /= rl;

        // もう1つの垂直方向 = 右 × 軸
        const float vx = ry * dz - rz * dy;
        const float vy = rz * dx - rx * dz;
        const float vz = rx * dy - ry * dx;

        // 側面を4本の線で表す
        const float offs[4][3] = {
            {  rx,  ry,  rz }, { -rx, -ry, -rz },
            {  vx,  vy,  vz }, { -vx, -vy, -vz },
        };

        for (const auto& o : offs)
        {
            Line(
                XMFLOAT3(a.x + o[0] * radius, a.y + o[1] * radius, a.z + o[2] * radius),
                XMFLOAT3(b.x + o[0] * radius, b.y + o[1] * radius, b.z + o[2] * radius),
                color);
        }
    }

    void DebugDraw::Flush(ID3D12GraphicsCommandList* cmdList)
    {
        if (m_lines.empty() || !m_pso || !m_mapped) return;

        std::memcpy(m_mapped, m_lines.data(), m_lines.size() * sizeof(Vtx));

        cmdList->SetPipelineState(m_pso);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
        cmdList->IASetVertexBuffers(0, 1, &m_vbView);
        cmdList->DrawInstanced(static_cast<UINT>(m_lines.size()), 1, 0, 0);
    }
}