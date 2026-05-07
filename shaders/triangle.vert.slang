struct UniformConstants
{
    float4x4 mvp;
    float3 color;
    float _padding;
};

struct VSInput
{
    float3 Position : POSITION0;
    float2 Uv;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 Color : TEXCOORD0;
};

constexpr float3 g_light_dir = {-0.8, -1.0, 0.0};

[[vk::push_constant]]
UniformConstants state;

[shader("vertex")]
VSOutput main(VSInput input)
{        
    VSOutput output;
    output.Position = mul(state.mvp, float4(input.Position, 1.0));
    output.Color = state.color;
    return output;
}