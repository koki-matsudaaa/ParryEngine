#include "pch.h"
#include "SpriteRenderPipeline.h"

// ──────────────────────────────────────────
// SpriteTexture
// ──────────────────────────────────────────
bool SpriteTexture::CreateView()
{
    if (!m_texture) return false;

    D3D12_DESCRIPTOR_HEAP_DESC hd = {};
    hd.NumDescriptors = 1;
    hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(_dev->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&m_texHeap)))) return false;

    D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Format = m_texture->GetDesc().Format;
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Texture2D.MipLevels = 1;
    _dev->CreateShaderResourceView(m_texture, &srv,
        m_texHeap->GetCPUDescriptorHandleForHeapStart());

    return true;
}

bool SpriteTexture::Load(const std::string& path)
{
    // LoadTextureFromFile は非 const 参照を取るのでコピーを渡す。
    std::string p = path;
    m_texture = LoadTextureFromFile(p);
    m_loaded = (m_texture != nullptr);

    if (!m_texture)
    {
        // 画像がまだ無いときは白テクスチャで代用する (白い四角として見える)。
        char buf[256];
        sprintf_s(buf, "SpriteTexture: '%s' not found. use white texture.\n", path.c_str());
        OutputDebugStringA(buf);
        m_texture = CreateWhiteTexture();
    }

    return CreateView();
}

bool SpriteTexture::CreateWhite()
{
    m_texture = CreateWhiteTexture();
    m_loaded = false; // ファイル由来ではない
    return CreateView();
}

void SpriteTexture::Destroy()
{
    // m_texture は _resourceTable にキャッシュされている可能性があるので
    // ここでは解放しない (他のモデルと同じ扱い)。ヒープだけ返す。
    if (m_texHeap) { m_texHeap->Release(); m_texHeap = nullptr; }
    m_texture = nullptr;
    m_loaded = false;
}


// ──────────────────────────────────────────
// SpriteRenderPipeline
// ──────────────────────────────────────────
bool SpriteRenderPipeline::Create()
{
    // ── ルートシグネチャ ──────────────────────
    // [0] b0: スプライト1枚ぶんの情報 (VS と PS の両方で使う)
    // [1] t0: テクスチャ
    {
        D3D12_DESCRIPTOR_RANGE range = {};
        range.NumDescriptors = 1;
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.BaseShaderRegister = 0; // t0
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER params[2] = {};
        params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[0].Descriptor.ShaderRegister = 0; // b0
        params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[1].DescriptorTable.pDescriptorRanges = &range;
        params[1].DescriptorTable.NumDescriptorRanges = 1;
        params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // サンプラ (s0): 文字がにじまないよう CLAMP。
        D3D12_STATIC_SAMPLER_DESC samp = {};
        samp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        samp.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        samp.MaxLOD = D3D12_FLOAT32_MAX;
        samp.MinLOD = 0.0f;
        samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        samp.ShaderRegister = 0; // s0

        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        desc.pParameters = params;
        desc.NumParameters = 2;
        desc.pStaticSamplers = &samp;
        desc.NumStaticSamplers = 1;

        ID3DBlob* sigBlob = nullptr;
        ID3DBlob* sigErr = nullptr;
        HRESULT hr = D3D12SerializeRootSignature(&desc,
            D3D_ROOT_SIGNATURE_VERSION_1_0, &sigBlob, &sigErr);
        if (FAILED(hr))
        {
            OutputDebugStringA("Sprite root signature serialize failed\n");
            if (sigErr) { OutputDebugStringA((char*)sigErr->GetBufferPointer()); sigErr->Release(); }
            return false;
        }
        hr = _dev->CreateRootSignature(0, sigBlob->GetBufferPointer(),
            sigBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSig));
        sigBlob->Release();
        if (sigErr) sigErr->Release();
        if (FAILED(hr))
        {
            OutputDebugStringA("Sprite root signature creation failed\n");
            return false;
        }
    }

    // ── シェーダ (既存と同じく実行時コンパイル) ──
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT result = D3DCompileFromFile(L"SpriteVertexShader.hlsl",
        nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "SpriteVS", "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0, &vsBlob, &errorBlob);
    if (FAILED(result)) {
        OutputDebugStringA("SpriteVS compile failed\n");
        if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        return false;
    }

    result = D3DCompileFromFile(L"SpritePixelShader.hlsl",
        nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "SpritePS", "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0, &psBlob, &errorBlob);
    if (FAILED(result)) {
        OutputDebugStringA("SpritePS compile failed\n");
        if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        vsBlob->Release();
        return false;
    }

    // ── PSO ──────────────────────────────────
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
          D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
          D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC gp = {};
    gp.pRootSignature = m_rootSig;
    gp.VS.pShaderBytecode = vsBlob->GetBufferPointer();
    gp.VS.BytecodeLength = vsBlob->GetBufferSize();
    gp.PS.pShaderBytecode = psBlob->GetBufferPointer();
    gp.PS.BytecodeLength = psBlob->GetBufferSize();
    gp.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // αブレンド (3D 側 gpipeline と同じ設定)
    D3D12_RENDER_TARGET_BLEND_DESC blend = {};
    blend.BlendEnable = true;
    blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blend.LogicOpEnable = false;
    blend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blend.BlendOp = D3D12_BLEND_OP_ADD;
    blend.SrcBlendAlpha = D3D12_BLEND_ONE;
    blend.DestBlendAlpha = D3D12_BLEND_ZERO;
    blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    gp.BlendState.AlphaToCoverageEnable = false;
    gp.BlendState.IndependentBlendEnable = false;
    gp.BlendState.RenderTarget[0] = blend;

    gp.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    gp.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    gp.RasterizerState.DepthClipEnable = true;
    gp.RasterizerState.MultisampleEnable = false;
    gp.RasterizerState.FrontCounterClockwise = false;
    gp.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    gp.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    gp.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    gp.RasterizerState.AntialiasedLineEnable = false;
    gp.RasterizerState.ForcedSampleCount = 0;
    gp.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // 2D なので深度は使わない。DSV が束ねられていても触らない。
    gp.DepthStencilState.DepthEnable = false;
    gp.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    gp.DepthStencilState.StencilEnable = false;
    gp.DSVFormat = DXGI_FORMAT_UNKNOWN;

    gp.InputLayout.pInputElementDescs = inputLayout;
    gp.InputLayout.NumElements = _countof(inputLayout);
    gp.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    gp.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    gp.NumRenderTargets = 1;
    gp.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    gp.SampleDesc.Count = 1;
    gp.SampleDesc.Quality = 0;

    result = _dev->CreateGraphicsPipelineState(&gp, IID_PPV_ARGS(&m_pipeline));
    vsBlob->Release();
    psBlob->Release();
    if (FAILED(result))
    {
        OutputDebugStringA("Sprite pipeline state creation failed\n");
        return false;
    }

    // ── 単位クアッド (0,0)-(1,1)。全スプライトで共有する ──
    const Vertex verts[6] = {
        { { 0.0f, 0.0f }, { 0.0f, 0.0f } },
        { { 1.0f, 0.0f }, { 1.0f, 0.0f } },
        { { 0.0f, 1.0f }, { 0.0f, 1.0f } },
        { { 0.0f, 1.0f }, { 0.0f, 1.0f } },
        { { 1.0f, 0.0f }, { 1.0f, 0.0f } },
        { { 1.0f, 1.0f }, { 1.0f, 1.0f } },
    };

    auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto vbDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(verts));
    if (FAILED(_dev->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &vbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertBuff))))
    {
        OutputDebugStringA("Sprite vertex buffer creation failed\n");
        return false;
    }

    void* vmap = nullptr;
    m_vertBuff->Map(0, nullptr, &vmap);
    memcpy(vmap, verts, sizeof(verts));
    m_vertBuff->Unmap(0, nullptr);

    m_vbView.BufferLocation = m_vertBuff->GetGPUVirtualAddress();
    m_vbView.SizeInBytes = sizeof(verts);
    m_vbView.StrideInBytes = sizeof(Vertex);

    // ── 定数バッファ (256 バイト刻みで kMaxSprites 枚ぶん) ──
    // メインループは毎フレーム GPU 完了待ちをしているので二重化は不要。
    auto cbDesc = CD3DX12_RESOURCE_DESC::Buffer(kSlotSize * kMaxSprites);
    if (FAILED(_dev->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &cbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_constBuff))))
    {
        OutputDebugStringA("Sprite constant buffer creation failed\n");
        return false;
    }
    // UPLOAD ヒープなのでマップしっぱなしにして毎フレーム書き込んでよい。
    m_constBuff->Map(0, nullptr, (void**)&m_mapped);

    return true;
}

void SpriteRenderPipeline::Begin(ID3D12GraphicsCommandList* cmdList)
{
    if (!m_pipeline) return;

    m_slot = 0; // 定数バッファのスロットを先頭から使い直す
    cmdList->SetPipelineState(m_pipeline);
    cmdList->SetGraphicsRootSignature(m_rootSig);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 1, &m_vbView);
}

void SpriteRenderPipeline::Draw(ID3D12GraphicsCommandList* cmdList,
    ID3D12DescriptorHeap* texHeap,
    float x, float y, float w, float h,
    const XMFLOAT2& uvOffset,
    const XMFLOAT2& uvSize,
    const XMFLOAT4& color)
{
    if (!m_pipeline || !m_mapped) return;
    if (m_slot >= kMaxSprites) return; // 使い切ったらこのフレームは黙って捨てる

    SpriteData* dst = reinterpret_cast<SpriteData*>(m_mapped + kSlotSize * m_slot);
    dst->posPx = XMFLOAT2(x, y);
    dst->sizePx = XMFLOAT2(w, h);
    dst->uvOffset = uvOffset;
    dst->uvSize = uvSize;
    dst->color = color;
    dst->screenSize = XMFLOAT2((float)window_width, (float)window_height);
    dst->padding = XMFLOAT2(0.0f, 0.0f);

    if (texHeap)
    {
        cmdList->SetDescriptorHeaps(1, &texHeap);
        cmdList->SetGraphicsRootDescriptorTable(
            1, texHeap->GetGPUDescriptorHandleForHeapStart());
    }
    cmdList->SetGraphicsRootConstantBufferView(
        0, m_constBuff->GetGPUVirtualAddress() + kSlotSize * m_slot);

    cmdList->DrawInstanced(6, 1, 0, 0);

    ++m_slot;
}

void SpriteRenderPipeline::Destroy()
{
    if (m_constBuff)
    {
        if (m_mapped) { m_constBuff->Unmap(0, nullptr); m_mapped = nullptr; }
        m_constBuff->Release();
        m_constBuff = nullptr;
    }
    if (m_vertBuff) { m_vertBuff->Release();  m_vertBuff = nullptr; }
    if (m_pipeline) { m_pipeline->Release();  m_pipeline = nullptr; }
    if (m_rootSig) { m_rootSig->Release();   m_rootSig = nullptr; }
}