#include "Engine/Core/pch.h"
#include "Engine/Graphics/FbxModel.h"
#include <cfloat>

// ──────────────────────────────────────────
// FbxManager の共有 (SDK全体で1つ)
// ──────────────────────────────────────────
FbxManager* FbxModel::GetSharedManager()
{
    // 関数内 static。初回呼び出し時に1度だけ生成され、以降同じものを返す。
    static FbxManager* s_manager = nullptr;
    if (s_manager == nullptr)
    {
        s_manager = FbxManager::Create();
        // 入出力設定 (IOSettings) を作って登録。読み込みに必要。
        FbxIOSettings* ios = FbxIOSettings::Create(s_manager, IOSROOT);
        s_manager->SetIOSettings(ios);
    }
    return s_manager;
}

// ──────────────────────────────────────────
// 読み込み本体
// ──────────────────────────────────────────
bool FbxModel::Load(const std::string& path, const std::string& texturePath)
{
    FbxManager* manager = GetSharedManager();

    // インポータを作る。
    FbxImporter* importer = FbxImporter::Create(manager, "");
    if (!importer->Initialize(path.c_str(), -1, manager->GetIOSettings()))
    {
        // 初期化失敗 (ファイルが無い/壊れている等)。
        OutputDebugStringA("FBX Importer Initialize failed\n");
        importer->Destroy();
        return false;
    }

    // シーンを作ってインポート。
    FbxScene* scene = FbxScene::Create(manager, "scene");
    if (!importer->Import(scene))
    {
        OutputDebugStringA("FBX Import failed\n");
        importer->Destroy();
        return false;
    }
    importer->Destroy(); // インポート後は不要

    // ポリゴンを全て三角形に統一する (四角面などが混じっていても安全に扱える)。
    FbxGeometryConverter converter(manager);
    converter.Triangulate(scene, true);

    // 最小・最大を初期化。
    m_min = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
    m_max = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    // シーン内の全メッシュを取り出して読み込む。
    int meshCount = scene->GetSrcObjectCount<FbxMesh>();
    for (int i = 0; i < meshCount; i++)
    {
        FbxMesh* mesh = scene->GetSrcObject<FbxMesh>(i);
        if (mesh)
        {
            unsigned int vtxBase = static_cast<unsigned int>(m_vertices.size());
            ReadMesh(mesh);
            ReadBones(mesh);
            ReadSkinWeights(mesh, vtxBase);
        }
    }

    // scene->Destroy();前に、アニメを焼き込む
    BakeAnimation(scene);

    // テクスチャを読み込む　自動
    //std::string texPath = ExtractTexturePath(scene);
    // テクスチャを読み込む 直接
    // テクスチャパス: 引数で指定があればそれを、無ければ既定(Robot)を使う。
    std::string texPath = texturePath.empty()
        ? std::string("Model/Robot_Base_color.png")
        : texturePath;

    scene->Destroy(); // シーンはもう不要 (頂点は自前配列に写した)

    if (m_vertices.empty())
    {
        OutputDebugStringA("FBX: no vertices read\n");
        return false;
    }

    if (!texPath.empty())
    {
        m_texture = LoadTextureFromFile(texPath);
    }
    if (!texPath.empty())
    {
        m_texture = LoadTextureFromFile(texPath);
    }
    if (!m_texture)
    {
        OutputDebugStringA(("Texture load FAILED: " + texPath + "\n").c_str());
        m_texture = CreateWhiteTexture();
    }
    else
    {
        OutputDebugStringA(("Texture loaded OK: " + texPath + "\n").c_str());
    }

    // t0用のディスクリプタヒープを作り、SRVを1つ作る
    {
        D3D12_DESCRIPTOR_HEAP_DESC hd = {};
        hd.NumDescriptors = 1;
        hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        _dev->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&m_texHeap));

        D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Format = m_texture->GetDesc().Format;
        srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Texture2D.MipLevels = 1;
        _dev->CreateShaderResourceView(m_texture, &srv,
            m_texHeap->GetCPUDescriptorHandleForHeapStart());
    }

    BuildBuffers();
    return true;
}

// ──────────────────────────────────────────
// 1メッシュから頂点(位置)を取り出す
// ──────────────────────────────────────────
void FbxModel::ReadMesh(FbxMesh* mesh)
{
    FbxVector4* controlPoints = mesh->GetControlPoints();
    int* polygonVertices = mesh->GetPolygonVertices();
    int polygonVertexCount = mesh->GetPolygonVertexCount();

    // 法線を取得する。ポリゴン頂点ごとに1つ入っている。
    FbxArray<FbxVector4> normals;
    mesh->GetPolygonVertexNormals(normals);

    // UVを取得
    FbxArray<FbxVector2> uvs;
    FbxStringList uvSetNames;
    mesh->GetUVSetNames(uvSetNames);
    bool hasUV = false;
    if (uvSetNames.GetCount() > 0)
    {
        mesh->GetPolygonVertexUVs(uvSetNames.GetStringAt(0), uvs);
        hasUV = true;
    }

    unsigned int base = static_cast<unsigned int>(m_vertices.size());

    for (int i = 0; i < polygonVertexCount; i++)
    {
        int cpIndex = polygonVertices[i];

        FbxVtx v;
        // 位置
        v.position.x = static_cast<float>(controlPoints[cpIndex][0]);
        v.position.y = static_cast<float>(controlPoints[cpIndex][1]);
        v.position.z = static_cast<float>(controlPoints[cpIndex][2]);

        // 法線 (ポリゴン頂点の並び i にそのまま対応)
        if (i < normals.Size())
        {
            v.normal.x = static_cast<float>(normals[i][0]);
            v.normal.y = static_cast<float>(normals[i][1]);
            v.normal.z = static_cast<float>(normals[i][2]);
        }
        else
        {
            v.normal = XMFLOAT3(0.0f, 1.0f, 0.0f); // 保険 (上向き)
        }

        // UV (ポリゴン頂点順に対応。FBXはVが反転しているので 1-v で直す)
        if (hasUV && i < uvs.Size())
        {
            v.uv.x = static_cast<float>(uvs[i][0]);
            v.uv.y = static_cast<float>(1.0 - uvs[i][1]);
        }
        else
        {
            v.uv = XMFLOAT2(0.0f, 0.0f);
        }

        m_vertices.push_back(v);

        m_min.x = min(m_min.x, v.position.x);
        m_min.y = min(m_min.y, v.position.y);
        m_min.z = min(m_min.z, v.position.z);
        m_max.x = max(m_max.x, v.position.x);
        m_max.y = max(m_max.y, v.position.y);
        m_max.z = max(m_max.z, v.position.z);
    }

    for (int i = 0; i < polygonVertexCount; i++)
    {
        m_indices.push_back(base + static_cast<unsigned int>(i));
    }
}

// ──────────────────────────────────────────
// GPUバッファ構築 (PmdModel と同じ要領)
// ──────────────────────────────────────────
void FbxModel::BuildBuffers()
{
    // 頂点バッファ
    {
        auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(
            m_vertices.size() * sizeof(FbxVtx));
        _dev->CreateCommittedResource(
            &heapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&m_vertBuff));

        FbxVtx* map = nullptr;
        m_vertBuff->Map(0, nullptr, (void**)&map);
        std::copy(m_vertices.begin(), m_vertices.end(), map);
        m_vertBuff->Unmap(0, nullptr);

        m_vbView.BufferLocation = m_vertBuff->GetGPUVirtualAddress();
        m_vbView.SizeInBytes = static_cast<UINT>(m_vertices.size() * sizeof(FbxVtx));
        m_vbView.StrideInBytes = sizeof(FbxVtx);
    }

    // インデックスバッファ
    {
        auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(
            m_indices.size() * sizeof(unsigned int));
        _dev->CreateCommittedResource(
            &heapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&m_idxBuff));

        unsigned int* map = nullptr;
        m_idxBuff->Map(0, nullptr, (void**)&map);
        std::copy(m_indices.begin(), m_indices.end(), map);
        m_idxBuff->Unmap(0, nullptr);

        m_ibView.BufferLocation = m_idxBuff->GetGPUVirtualAddress();
        m_ibView.Format = DXGI_FORMAT_R32_UINT; // unsigned int なので 32bit
        m_ibView.SizeInBytes = static_cast<UINT>(m_indices.size() * sizeof(unsigned int));
    }
}

// メッシュのスキンからボーン情報を読み取る。
void FbxModel::ReadBones(FbxMesh* mesh)
{
    // メッシュにぶら下がっているスキン変形器の数 (通常1つ)。
    int skinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
    for (int s = 0; s < skinCount; s++)
    {
        FbxSkin* skin = static_cast<FbxSkin*>(
            mesh->GetDeformer(s, FbxDeformer::eSkin));
        if (!skin) continue;

        // スキンの中のクラスター (1クラスター = 1ボーン)。
        int clusterCount = skin->GetClusterCount();
        for (int c = 0; c < clusterCount; c++)
        {
            FbxCluster* cluster = skin->GetCluster(c);
            FbxNode* boneNode = cluster->GetLink(); // 対応するボーン
            if (!boneNode) continue;

            std::string boneName = boneNode->GetName();

            // すでに登録済みならスキップ (複数メッシュで同じボーンを共有する場合)。
            if (m_boneIndexByName.count(boneName)) continue;

            // バインドポーズの行列を取得する。
            // TransformMatrix: メッシュのバインド時の変換
            // TransformLinkMatrix: ボーンのバインド時の変換
            FbxAMatrix meshBind;
            FbxAMatrix boneBind;
            cluster->GetTransformMatrix(meshBind);
            cluster->GetTransformLinkMatrix(boneBind);

            // バインドの逆行列 = ボーン基準へ変換する行列。
            FbxAMatrix bindInverse = boneBind.Inverse() * meshBind;

            // 自前のBone構造体に格納。
            Bone bone;
            bone.name = boneName;
            bone.node = boneNode;
            bone.bindInverse = ToXMMatrix(bindInverse); // FbxAMatrix → XMMATRIX 変換

            int index = static_cast<int>(m_bones.size());
            m_bones.push_back(bone);
            m_boneIndexByName[boneName] = index;
        }
    }

    // 確認用: 読めたボーン数を出力。
    char buf[64];
    sprintf_s(buf, "Bones read: %zu\n", m_bones.size());
    OutputDebugStringA(buf);
}

// スキンウェイトを読む (段2)。頂点ごとに、影響するボーンと重みを集める。
void FbxModel::ReadSkinWeights(FbxMesh* mesh, unsigned int vertexBase)
{
    // 頂点ごとに (ボーン番号, 重み) のリストを一時的に溜める。
    // vertexBase は、このメッシュの頂点が m_vertices の何番目から始まるか。
    int controlPointCount = mesh->GetControlPointsCount();
    std::vector<std::vector<std::pair<int, float>>> perControlPoint(controlPointCount);

    int skinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
    for (int s = 0; s < skinCount; s++)
    {
        FbxSkin* skin = static_cast<FbxSkin*>(mesh->GetDeformer(s, FbxDeformer::eSkin));
        if (!skin) continue;

        int clusterCount = skin->GetClusterCount();
        for (int c = 0; c < clusterCount; c++)
        {
            FbxCluster* cluster = skin->GetCluster(c);
            FbxNode* boneNode = cluster->GetLink();
            if (!boneNode) continue;

            // このボーンの番号を、名前から引く。
            std::string boneName = boneNode->GetName();
            auto it = m_boneIndexByName.find(boneName);
            if (it == m_boneIndexByName.end()) continue;
            int boneIndex = it->second;

            // このボーンが影響する頂点(コントロールポイント)と重み。
            int* indices = cluster->GetControlPointIndices();
            double* weights = cluster->GetControlPointWeights();
            int count = cluster->GetControlPointIndicesCount();

            for (int k = 0; k < count; k++)
            {
                int cpIndex = indices[k];
                float weight = static_cast<float>(weights[k]);
                if (cpIndex < controlPointCount)
                    perControlPoint[cpIndex].push_back({ boneIndex, weight });
            }
        }
    }

    // コントロールポイントごとの情報を、実際の頂点 (ポリゴン頂点で展開済み) に移す。
    // ReadMesh でポリゴン頂点を展開したので、その対応を再現する。
    int* polygonVertices = mesh->GetPolygonVertices();
    int polygonVertexCount = mesh->GetPolygonVertexCount();

    for (int i = 0; i < polygonVertexCount; i++)
    {
        int cpIndex = polygonVertices[i];
        unsigned int vtxIndex = vertexBase + i; // m_vertices での位置
        if (vtxIndex >= m_vertices.size()) continue;

        // このコントロールポイントに影響するボーンを、重み順に上位4つ選ぶ。
        auto influences = perControlPoint[cpIndex];
        std::sort(influences.begin(), influences.end(),
            [](auto& a, auto& b) { return a.second > b.second; });

        float total = 0.0f;
        for (int b = 0; b < 4; b++)
        {
            if (b < (int)influences.size())
            {
                m_vertices[vtxIndex].boneIndex[b] = influences[b].first;
                m_vertices[vtxIndex].boneWeight[b] = influences[b].second;
                total += influences[b].second;
            }
        }

        // 重みの合計を1に正規化する。
        if (total > 0.0f)
        {
            for (int b = 0; b < 4; b++)
                m_vertices[vtxIndex].boneWeight[b] /= total;
        }
    }

    // 確認用: 最初の頂点のウェイトを出す
    if (!m_vertices.empty())
    {
        char buf[128];
        sprintf_s(buf, "Vtx0 weights: %.2f %.2f %.2f %.2f (bones %u %u %u %u)\n",
            m_vertices[0].boneWeight[0], m_vertices[0].boneWeight[1],
            m_vertices[0].boneWeight[2], m_vertices[0].boneWeight[3],
            m_vertices[0].boneIndex[0], m_vertices[0].boneIndex[1],
            m_vertices[0].boneIndex[2], m_vertices[0].boneIndex[3]);
        OutputDebugStringA(buf);
    }
}

// FbxAMatrix を XMMATRIX に変換する。
XMMATRIX FbxModel::ToXMMatrix(const FbxAMatrix& m)
{
    XMMATRIX out;
    for (int row = 0; row < 4; row++)
        for (int col = 0; col < 4; col++)
            out.r[row].m128_f32[col] = static_cast<float>(m.Get(row, col));
    return out;
}

// アニメーションを一定間隔で焼き込む (段3・方式A)。
void FbxModel::BakeAnimation(FbxScene* scene)
{
    // アニメーションスタック (アニメの塊) を取得。Mixamoは通常1つ。
    int stackCount = scene->GetSrcObjectCount<FbxAnimStack>();
    if (stackCount == 0)
    {
        OutputDebugStringA("No animation found\n");
        return;
    }
    FbxAnimStack* stack = scene->GetSrcObject<FbxAnimStack>(0);
    scene->SetCurrentAnimationStack(stack);

    // アニメの時間範囲を取得。
    FbxTimeSpan span = stack->GetLocalTimeSpan();
    FbxTime start = span.GetStart();
    FbxTime end = span.GetStop();

    double startSec = start.GetSecondDouble();
    double endSec = end.GetSecondDouble();
    m_animDuration = static_cast<float>(endSec - startSec);

    // 1/fps 間隔でサンプリングするフレーム数。
    int frameCount = static_cast<int>(m_animDuration * m_animFps) + 1;

    m_animFrames.resize(frameCount);

    for (int f = 0; f < frameCount; f++)
    {
        // このフレームの時刻。
        double timeSec = startSec + (double)f / m_animFps;
        FbxTime evalTime;
        evalTime.SetSecondDouble(timeSec);

        AnimFrame& frame = m_animFrames[f];
        frame.boneMatrices.resize(m_bones.size());

        // 全ボーンについて、この時刻での姿勢を計算。
        for (size_t b = 0; b < m_bones.size(); b++)
        {
            FbxNode* boneNode = m_bones[b].node;

            // この時刻での、親を全部考慮したグローバル変換。
            FbxAMatrix globalAtTime = boneNode->EvaluateGlobalTransform(evalTime);

            // スキニング用の最終行列 = グローバル変換 × バインド逆行列。
            //   バインド時の姿勢から、今の姿勢へどう動いたかを表す。
            XMMATRIX global = ToXMMatrix(globalAtTime);
            frame.boneMatrices[b] = m_bones[b].bindInverse * global;
        }
    }

    char buf[64];
    sprintf_s(buf, "Baked animation: %d frames, %.2f sec\n", frameCount, m_animDuration);
    OutputDebugStringA(buf);
}

// マテリアルのディフューズに紐づくテクスチャのファイルパスを取り出す。
std::string FbxModel::ExtractTexturePath(FbxScene* scene)
{
    int matCount = scene->GetSrcObjectCount<FbxSurfaceMaterial>();
    for (int i = 0; i < matCount; i++)
    {
        FbxSurfaceMaterial* mat = scene->GetSrcObject<FbxSurfaceMaterial>(i);
        if (!mat) continue;

        // ディフューズ(基本色)プロパティを探す
        FbxProperty prop = mat->FindProperty(FbxSurfaceMaterial::sDiffuse);
        if (!prop.IsValid()) continue;

        int texCount = prop.GetSrcObjectCount<FbxFileTexture>();
        if (texCount > 0)
        {
            FbxFileTexture* tex = prop.GetSrcObject<FbxFileTexture>(0);
            if (tex)
            {
                // FBXに記録された絶対パス。ファイル名だけ取り出して
                // 自分のModelフォルダ基準に付け替える方が安全な場合もあるが、
                // まずは記録されたパスをそのまま試す。
                std::string full = tex->GetFileName();

                // パスからファイル名だけ抜き出し、Model/ 基準に付け替える。
                size_t slash = full.find_last_of("/\\");
                std::string fileName =
                    (slash == std::string::npos) ? full : full.substr(slash + 1);
                return "Model/" + fileName;
            }
        }
    }
    return "";
}

// ──────────────────────────────────────────
// 描画
// ──────────────────────────────────────────
void FbxModel::Draw(ID3D12GraphicsCommandList* cmdList)
{
    // テクスチャヒープをセットし、t0 (ルートパラメータ2) に割り当てる。
    if (m_texHeap)
    {
        cmdList->SetDescriptorHeaps(1, &m_texHeap);
        cmdList->SetGraphicsRootDescriptorTable(
            2, m_texHeap->GetGPUDescriptorHandleForHeapStart());
    }

    cmdList->IASetVertexBuffers(0, 1, &m_vbView);
    cmdList->IASetIndexBuffer(&m_ibView);
    cmdList->DrawIndexedInstanced(
        static_cast<UINT>(m_indices.size()), 1, 0, 0, 0);
}

// アニメの時刻を進める。末尾まで来たら先頭へループ。
void FbxModel::UpdateAnimation(float dt)
{
    if (m_animFrames.empty()) return;

    m_animTime += dt;

    // 末尾を超えたらループさせる。
    if (m_animDuration > 0.0f)
    {
        while (m_animTime >= m_animDuration)
            m_animTime -= m_animDuration;
    }
}

// 今の時刻に対応するフレームのボーン行列を返す。
const std::vector<XMMATRIX>& FbxModel::GetCurrentBoneMatrices() const
{
    // 時刻 → フレーム番号 (一番近いフレームを選ぶ簡易版)。
    static std::vector<XMMATRIX> identityFallback;
    if (m_animFrames.empty())
    {
        // アニメが無ければ単位行列を返す (変形なし)。
        if (identityFallback.size() != m_bones.size())
            identityFallback.assign(m_bones.size(), XMMatrixIdentity());
        return identityFallback;
    }

    //int frameIndex = static_cast<int>(m_animTime * m_animFps);
    //if (frameIndex < 0) frameIndex = 0;
    //if (frameIndex >= (int)m_animFrames.size())
    //    frameIndex = (int)m_animFrames.size() - 1;

    int frameIndex;
    if (m_fixedFrame >= 0)
    {
        // フレーム固定モード: 指定フレームのポーズを使う。
        frameIndex = m_fixedFrame;
    }
    else
    {
        // 通常: 再生時刻からフレームを求める。
        frameIndex = static_cast<int>(m_animTime * m_animFps);
    }

    if (frameIndex < 0) frameIndex = 0;
    if (frameIndex >= (int)m_animFrames.size())
        frameIndex = (int)m_animFrames.size() - 1;

    return m_animFrames[frameIndex].boneMatrices;
}