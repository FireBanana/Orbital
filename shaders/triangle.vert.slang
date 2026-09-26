struct UniformConstants // Uniform buffer
{
    float4x4 view;
    float4x4 projection;
    float4x4 lightVP;
    float4 camera;
    float4 lightPos;
    uint32_t frame;
};

struct ModelConstants // Push constant
{
    float4x4 model;
    float4 roughMetal; //2 unused
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
    float4 WorldPos;
    float4 LightSpacePos;
    float3 Normal : NORMAL;
    float2 Uv : TEXCOORD0;
};

static const float4x4 biasMat = float4x4(
    0.5,  0.0, 0.0, 0.5,
    0.0, -0.5, 0.0, 0.5,
    0.0,  0.0, 1.0, 0.0,
    0.0,  0.0, 0.0, 1.0
);

[[vk::push_constant]]
ModelConstants modelState;

[[vk::binding(4, 0)]]
ConstantBuffer<UniformConstants> uniformState;

//[[vk::binding(4, 0)]]
//Sampler2D texture1;

[shader("vertex")]
VSOutput main(VSInput input)
{        
    VSOutput output;
    output.Position = mul(uniformState.projection, mul(uniformState.view, mul(modelState.model, float4(input.Position, 1.0))));
    output.WorldPos = mul(modelState.model, float4(input.Position, 1.0));
    output.Normal = input.Normal;
    output.Uv = input.Uv;
    output.LightSpacePos = mul(biasMat, mul(uniformState.lightVP, float4(input.Position, 1.0)));
    return output;
}
