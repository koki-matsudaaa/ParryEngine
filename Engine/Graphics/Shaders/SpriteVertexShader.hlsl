
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

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

Output SpriteVS(float2 pos : POSITION, float2 uv : TEXCOORD)
{
    Output output;

    float2 px = posPx + pos * sizePx;

    float2 ndc = float2(px.x / screenSize.x * 2.0f - 1.0f, 1.0f - px.y / screenSize.y * 2.0f);

    output.svpos = float4(ndc, 0.0f, 1.0f);
    output.uv = uvOffset + uv * uvSize;
    return output;
}