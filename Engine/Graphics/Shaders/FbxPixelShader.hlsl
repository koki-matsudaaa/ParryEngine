
Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

struct Output
{
    float4 svpos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

float4 FbxPS(Output input) : SV_TARGET
{
    float3 light = normalize(float3(1, -1, 1));
    float brightness = saturate(dot(-light, normalize(input.normal)));
    brightness = brightness * 0.8 + 0.2;

    float4 texColor = tex.Sample(smp, input.uv); // テクスチャの色
    return float4(texColor.rgb * brightness, texColor.a);
}