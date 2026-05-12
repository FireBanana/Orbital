struct UniformConstants
{
    float4x4 mvp;
    float4x4 model;
    float4 camera;
    uint32_t frame;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
};

static float3 g_light_dir = {-0.8, -1.0, -3.0};

[[vk::push_constant]]
UniformConstants state;

float schlick(float3 normal)
{
    float ior_air = 1.0003;
    float ior_skin = 1.55;

    float a = acos(dot(normalize(normal), normalize(-state.camera.xyz))) / 2.0;
    float r0 = pow(((ior_air - ior_skin) / (ior_air + ior_skin)), 2.0);

    return pow(r0 + (1. - r0)*(1. - cos(a)), 5.0);
}

[shader("fragment")]
float4 main(VSOutput input)
{
    //g_light_dir.x *= sin(state.frame * 0.01);
    //g_light_dir.z *= cos(state.frame * 0.01);

    float3 color = max(dot(normalize(g_light_dir), normalize(input.Normal)), float3(0.));
    color /= 3.142;
    float ambient = 1.5;

    return float4(color + ambient * schlick(input.Normal), 1.0);
}