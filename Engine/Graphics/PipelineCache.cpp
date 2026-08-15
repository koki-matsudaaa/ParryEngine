#include "Engine/Core/pch.h"
#include "Engine/Graphics/PipelineCache.h"
#include "Engine/Core/Dx12Context.h"

#include <d3dcompiler.h>

namespace Engine
{
    namespace
    {
        // 不透明メッシュ共通の設定。
        // シェーダと入力レイアウトだけ、後から差し替えて使う。
        D3D12_GRAPHICS_PIPELINE_STATE_DESC MakeBaseDesc()
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
            desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

            desc.BlendState.AlphaToCoverageEnable = false;
            desc.BlendState.IndependentBlendEnable = false;
            desc.BlendState.RenderTarget[0].BlendEnable = false;
            desc.BlendState.RenderTarget[0].LogicOpEnable = false;
            desc.BlendState.RenderTarget[0].RenderTargetWriteMask =
                D3D12_COLOR_WRITE_ENABLE_ALL;

            desc.RasterizerState.MultisampleEnable = false;
            desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // 裏面も描く
            desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            desc.RasterizerState.DepthClipEnable = true;
            desc.RasterizerState.FrontCounterClockwise = false;
            desc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
            desc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
            desc.RasterizerState.SlopeScaledDepthBias =
                D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
            desc.RasterizerState.AntialiasedLineEnable = false;
            desc.RasterizerState.ForcedSampleCount = 0;
            desc.RasterizerState.ConservativeRaster =
                D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

            desc.DepthStencilState.DepthEnable = true;
            desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
            desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
            desc.DepthStencilState.StencilEnable = false;
            desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

            desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
            desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            desc.NumRenderTargets = 1;
            desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            return desc;
        }

        // シェーダを1本コンパイルする。
        // 失敗したらエラー内容を出力ウィンドウに出す (原因が分からないと詰むので)。
        bool CompileShader(const wchar_t* path, const char* entry,
            const char* target, ID3DBlob** out)
        {
            UINT flags = 0;
#ifdef _DEBUG
            flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
            ID3DBlob* err = nullptr;
            HRESULT hr = D3DCompileFromFile(path, nullptr,
                D3D_COMPILE_STANDARD_FILE_INCLUDE,
                entry, target, flags, 0, out, &err);

            if (FAILED(hr))
            {
                OutputDebugStringA("shader compile failed: ");
                OutputDebugStringA(entry);
                OutputDebugStringA("\n");
                if (err) OutputDebugStringA((const char*)err->GetBufferPointer());
            }
            if (err) err->Release();
            return SUCCEEDED(hr);
        }
    }

    bool PipelineCache::Initialize()
    {
        if (!CreateRootSignature())    return false;
        if (!CreateStaticPipeline())   return false;
        if (!CreateSkeletalPipeline()) return false;
        return true;
    }

    void PipelineCache::Shutdown()
    {
        if (m_skeletalPso) { m_skeletalPso->Release(); m_skeletalPso = nullptr; }
        if (m_staticPso) { m_staticPso->Release();     m_staticPso = nullptr; }
        if (m_rootSignature) { m_rootSignature->Release(); m_rootSignature = nullptr; }
    }

    ID3D12PipelineState* PipelineCache::Get(PipelineType type) const
    {
        switch (type)
        {
        case PipelineType::Static: return m_staticPso;
        case PipelineType::FBX:    return m_skeletalPso;
        default:                   return nullptr;  // PMD は使わない
        }
    }

    bool PipelineCache::CreateRootSignature()
    {
        // t0 (テクスチャ) 用のディスクリプタレンジ
        D3D12_DESCRIPTOR_RANGE range = {};
        range.NumDescriptors = 1;
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.BaseShaderRegister = 0; // t0
        range.OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER params[4] = {};

        // [0] b0 : シーン定数 (view / proj / eye)
        params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[0].Descriptor.ShaderRegister = 0;
        params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        // [1] b2 : オブジェクト定数 (world)
        params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[1].Descriptor.ShaderRegister = 2;
        params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        // [2] t0 : テクスチャ
        params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[2].DescriptorTable.pDescriptorRanges = &range;
        params[2].DescriptorTable.NumDescriptorRanges = 1;
        params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // [3] b3 : ボーン行列 (スキニング用。Static では使わない)
        params[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[3].Descriptor.ShaderRegister = 3;
        params[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        // サンプラ (s0)
        D3D12_STATIC_SAMPLER_DESC samp = {};
        samp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samp.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        samp.MaxLOD = D3D12_FLOAT32_MAX;
        samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        samp.ShaderRegister = 0;

        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        desc.pParameters = params;
        desc.NumParameters = _countof(params);
        desc.pStaticSamplers = &samp;
        desc.NumStaticSamplers = 1;

        ID3DBlob* sigBlob = nullptr;
        ID3DBlob* sigErr = nullptr;
        HRESULT hr = D3D12SerializeRootSignature(&desc,
            D3D_ROOT_SIGNATURE_VERSION_1_0, &sigBlob, &sigErr);
        if (FAILED(hr))
        {
            OutputDebugStringA("root signature serialize failed\n");
            if (sigErr) OutputDebugStringA((const char*)sigErr->GetBufferPointer());
            if (sigErr) sigErr->Release();
            return false;
        }
        if (sigErr) sigErr->Release();

        hr = _dev->CreateRootSignature(0, sigBlob->GetBufferPointer(),
            sigBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
        sigBlob->Release();

        return SUCCEEDED(hr);
    }

    bool PipelineCache::CreateStaticPipeline()
    {
        // 実行時のカレントはソリューションのルート。
        // ソースから直接コンパイルしているので、シェーダを書き換えたら
        // 再起動するだけで反映される (将来のホットリロードの下地)。
        ID3DBlob* vs = nullptr;
        ID3DBlob* ps = nullptr;

        if (!CompileShader(L"Engine/Graphics/Shaders/StaticVertexShader.hlsl",
            "StaticVS", "vs_5_0", &vs)) return false;
        if (!CompileShader(L"Engine/Graphics/Shaders/StaticPixelShader.hlsl",
            "StaticPS", "ps_5_0", &ps))
        {
            vs->Release();
            return false;
        }

        // ボーンを持たないメッシュの頂点構造。位置・法線・UV。
        D3D12_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
              D3D12_APPEND_ALIGNED_ELEMENT,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
              D3D12_APPEND_ALIGNED_ELEMENT,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0,
              D3D12_APPEND_ALIGNED_ELEMENT,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = MakeBaseDesc();
        desc.VS.pShaderBytecode = vs->GetBufferPointer();
        desc.VS.BytecodeLength = vs->GetBufferSize();
        desc.PS.pShaderBytecode = ps->GetBufferPointer();
        desc.PS.BytecodeLength = ps->GetBufferSize();
        desc.InputLayout.pInputElementDescs = layout;
        desc.InputLayout.NumElements = _countof(layout);
        desc.pRootSignature = m_rootSignature;

        HRESULT hr = _dev->CreateGraphicsPipelineState(&desc,
            IID_PPV_ARGS(&m_staticPso));

        vs->Release();
        ps->Release();

        if (FAILED(hr)) OutputDebugStringA("static PSO creation failed\n");
        return SUCCEEDED(hr);
    }

    bool PipelineCache::CreateSkeletalPipeline()
    {
        ID3DBlob* vs = nullptr;
        ID3DBlob* ps = nullptr;

        if (!CompileShader(L"Engine/Graphics/Shaders/FbxVertexShader.hlsl",
            "FbxVS", "vs_5_0", &vs)) return false;
        if (!CompileShader(L"Engine/Graphics/Shaders/FbxPixelShader.hlsl",
            "FbxPS", "ps_5_0", &ps))
        {
            vs->Release();
            return false;
        }

        // Static の頂点構造に、影響するボーン番号と重みを足したもの。
        D3D12_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0,
              D3D12_APPEND_ALIGNED_ELEMENT,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0,
              D3D12_APPEND_ALIGNED_ELEMENT,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0,
              D3D12_APPEND_ALIGNED_ELEMENT,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "BONE_INDEX",  0, DXGI_FORMAT_R32G32B32A32_UINT,  0,
              D3D12_APPEND_ALIGNED_ELEMENT,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "BONE_WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
              D3D12_APPEND_ALIGNED_ELEMENT,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = MakeBaseDesc();
        desc.VS.pShaderBytecode = vs->GetBufferPointer();
        desc.VS.BytecodeLength = vs->GetBufferSize();
        desc.PS.pShaderBytecode = ps->GetBufferPointer();
        desc.PS.BytecodeLength = ps->GetBufferSize();
        desc.InputLayout.pInputElementDescs = layout;
        desc.InputLayout.NumElements = _countof(layout);
        desc.pRootSignature = m_rootSignature;

        HRESULT hr = _dev->CreateGraphicsPipelineState(&desc,
            IID_PPV_ARGS(&m_skeletalPso));

        vs->Release();
        ps->Release();

        if (FAILED(hr)) OutputDebugStringA("skeletal PSO creation failed\n");
        return SUCCEEDED(hr);
    }
}