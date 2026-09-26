struct LightConstants
{
    float4x4 lightVP;
};

struct VSInput
{
    float3 Position : POSITION0;
    //float3 Normal : NORMAL;
    //float2 Uv : TEXCOORD0;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
};

[[vk::push_constant]]
LightConstants state;

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;
    // Todo add model matrix
    output.Position = mul(state.lightVP, float4(input.Position, 1.0));
    return output;
}