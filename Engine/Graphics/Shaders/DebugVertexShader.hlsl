
cbuffer SceneData : register(b0)
{
    matrix view;
    matrix proj;
    float3 eye;
};

struct Output
{
    float4 svpos : SV_POSITION;
    float3 color : COLOR;
};

// 頂点はすでにワールド座標。world は掛けない。
Output DebugVS(float4 pos : POSITION, float3 color : COLOR)
{
    Output output;
    output.svpos = mul(mul(proj, view), pos);
    output.color = color;
    return output;
}