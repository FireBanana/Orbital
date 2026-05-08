struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
};

static float3 g_light_dir = {-0.8, -1.0, -3.0};

[shader("fragment")]
float4 main(VSOutput input)
{
    float3 color = dot(normalize(g_light_dir), normalize(input.Normal));
    return float4(color, 1.0);
}