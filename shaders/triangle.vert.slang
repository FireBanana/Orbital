struct UniformConstants
{
    float4x4 mvp;
    float4x4 model;
    float4 camera;
    uint32_t frame;
};

struct VSInput
{
    float3 Position : POSITION0;
    float3 Normal : NORMAL;
    float2 Uv : TEXCOORD0;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
};

[[vk::push_constant]]
UniformConstants state;

[shader("vertex")]
VSOutput main(VSInput input)
{        
    VSOutput output;
    output.Position = mul(state.mvp, float4(input.Position, 1.0));
    output.Normal = input.Normal;
    return output;
}