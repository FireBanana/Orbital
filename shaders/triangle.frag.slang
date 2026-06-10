struct UniformConstants
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4 camera;
    uint32_t frame;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 WorldPos;
    float3 Normal : NORMAL;
    float2 Uv : TEXCOORD0;
};

static float3 g_light_dir = {-0.8, -1.0, 3.0};

[[vk::push_constant]]
UniformConstants state;

[[vk::binding(0, 0)]]
Sampler2D diffuse_texture;

float schlick(float3 normal, float3 pos)
{
    float ior_air = 1.0003;
    float ior_skin = 1.55;

    float a = acos(dot(normal, normalize(state.camera.xyz - pos))) / 2.0;
    float r0 = pow(((ior_air - ior_skin) / (ior_air + ior_skin)), 2.0);

    return pow(r0 + (1. - r0)*(1. - cos(a)), 5.0);
}

[shader("fragment")]
float4 main(VSOutput input)
{
    float3 light = max(dot(normalize(g_light_dir), normalize(input.Normal)), float3(0.));
    light /= 3.142;
    float ambient = 30.;

    float4 color = diffuse_texture.Sample(input.Uv);
    return float4(color.rgb * light + ambient * schlick(input.Normal, input.WorldPos.xyz), 1.0);
}