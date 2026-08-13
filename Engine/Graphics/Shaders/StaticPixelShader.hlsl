Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

struct Output
{
    float4 svpos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

float4 StaticPS(Output input) : SV_TARGET
{
    float3 light = normalize(float3(1, -1, 1));
    float brightness = saturate(dot(-light, normalize(input.normal)));
    brightness = brightness * 0.6 + 0.4; // ínñ ÅEãÛÇÕñæÇÈÇﬂÇ…
    float4 texColor = tex.Sample(smp, input.uv);
    return float4(texColor.rgb * brightness, texColor.a);
}