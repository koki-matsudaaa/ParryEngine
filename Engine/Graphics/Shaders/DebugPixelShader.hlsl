
struct Input
{
    float4 svpos : SV_POSITION;
    float3 color : COLOR;
};

float4 DebugPS(Input input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}