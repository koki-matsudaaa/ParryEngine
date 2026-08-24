#include "Engine/Tools/ImGuiLayer.h"
#include "Engine/Core/Dx12Context.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

namespace Engine
{
    bool ImGuiLayer::Initialize(HWND hwnd)
    {
        // ImGui がフォントテクスチャを置くための棚を1枚ぶん用意する。
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;  // シェーダから見える必要がある
        if (FAILED(_dev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_srvHeap))))
            return false;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        // 既定のフォントは ASCII しか持たないので、日本語が豆腐になる。
        // Windows に必ず入っているメイリオを、日本語の字形範囲つきで読む。
        // 読めなければ既定フォントのまま動く (英数字だけ表示される)。
        {
            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/meiryo.ttc", 18.0f,
                nullptr, io.Fonts->GetGlyphRangesJapanese());
        }

        ImGui_ImplWin32_Init(hwnd);

        ImGui_ImplDX12_InitInfo info = {};
        info.Device = _dev;
        info.CommandQueue = _cmdQueue;          // フォントテクスチャの転送に使う
        info.NumFramesInFlight = 2;             // スワップチェーンの枚数と合わせる
        info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        info.DSVFormat = DXGI_FORMAT_UNKNOWN;   // ImGui は深度を使わない
        info.SrvDescriptorHeap = m_srvHeap;
        info.LegacySingleSrvCpuDescriptor = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
        info.LegacySingleSrvGpuDescriptor = m_srvHeap->GetGPUDescriptorHandleForHeapStart();

        if (!ImGui_ImplDX12_Init(&info)) return false;

        m_initialized = true;
        return true;
    }

    void ImGuiLayer::Shutdown()
    {
        if (!m_initialized) return;

        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        if (m_srvHeap) { m_srvHeap->Release(); m_srvHeap = nullptr; }
        m_initialized = false;
    }

    void ImGuiLayer::NewFrame()
    {
        if (!m_initialized) return;

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::Render(ID3D12GraphicsCommandList* cmdList)
    {
        if (!m_initialized) return;

        ImGui::Render();

        // ImGui のフォントは自前の棚にあるので、割り当て直してから描く。
        cmdList->SetDescriptorHeaps(1, &m_srvHeap);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
    }
}