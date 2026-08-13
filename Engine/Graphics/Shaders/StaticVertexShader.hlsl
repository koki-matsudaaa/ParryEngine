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

struct Output
{
    float4 svpos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

Output StaticVS(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD)
{
    Output output;
    pos = mul(world, pos);
    output.svpos = mul(mul(proj, view), pos);
    normal.w = 0;
    output.normal = normalize(mul(world, normal).xyz);
    output.uv = uv;
    return output;
}