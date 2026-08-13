
cbuffer SpriteData : register(b0)
{
    float2 posPx;
    float2 sizePx;
    float2 uvOffset;
    float2 uvSize;
    float4 color;
    float2 screenSize;
    float2 padding;
};

Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 SpritePS(Output input) : SV_TARGET
{
    float4 c = tex.Sample(smp, input.uv) * color;

    clip(c.a - 0.01f);

    return c;
}