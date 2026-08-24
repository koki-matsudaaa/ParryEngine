#pragma once
#include <Windows.h>
#include <d3d12.h>

namespace Engine
{
    class ImGuiLayer
    {
    public:
        bool Initialize(HWND hwnd);
        void Shutdown();

        // ImGui のフレームを開始する。以降 ImGui:: を呼べる。
        void NewFrame();

        // 積まれた ImGui の描画コマンドをコマンドリストに書き出す。
        void Render(ID3D12GraphicsCommandList* cmdList);

    private:
        ID3D12DescriptorHeap* m_srvHeap = nullptr;   // フォントテクスチャ用
        bool m_initialized = false;
    };
}