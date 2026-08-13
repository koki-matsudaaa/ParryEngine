// FBX第2段階: 位置 + 法線

cbuffer SceneData : register(b0)
{
    matrix view;
    matrix proj;
    float3 eye;
};

cbuffer ObjectData : register(b2)
{
    matrix world;
};

// ボーン行列の配列 (スキニング用)。最大128本。
cbuffer BoneData : register(b3)
{
    matrix bones[128];
};

struct Output
{
    float4 svpos : SV_POSITION;
    float3 normal : NORMAL; // ← ピクセルシェーダへ渡す
    float2 uv : TEXCOORD;
};

Output FbxVS(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, uint4 boneIndex : BONE_INDEX, float4 boneWeight : BONE_WEIGHT)
{
    Output output;

    // スキニング: 4つのボーン行列を重み付きで合成する。
    matrix skin =
        bones[boneIndex.x] * boneWeight.x +
        bones[boneIndex.y] * boneWeight.y +
        bones[boneIndex.z] * boneWeight.z +
        bones[boneIndex.w] * boneWeight.w;

    // 合成行列で頂点位置を変形 (Tポーズ → 今のポーズへ)。
    float4 skinnedPos = mul(skin, pos);

    // その後、いつも通り world/view/proj で変換。
    skinnedPos = mul(world, skinnedPos);
    output.svpos = mul(mul(proj, view), skinnedPos);

    // 法線も同じ合成行列で変形 (移動成分は効かせないので w=0)。
    normal.w = 0;
    float4 skinnedNormal = mul(skin, normal);
    skinnedNormal = mul(world, skinnedNormal);
    output.normal = normalize(skinnedNormal.xyz);

    output.uv = uv;
    return output;
}