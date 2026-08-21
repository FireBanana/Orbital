#define PI 3.141593

const float constant1_FON = 0.5f - 2.0f / (3.0f * PI);
const float constant2_FON = 2.0f  / 3.0f - 28.0f / (15.0f * PI);
const float rpi = 1.0f / PI;

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

static float3 g_light_dir = {-0.8, 1.0, 3.0};

[[vk::push_constant]]
UniformConstants state;

[[vk::binding(0, 0)]]
Sampler2D diffuse_texture;

[[vk::binding(1, 0)]]
Sampler2D normal_texture;

float schlick(half3 normal, half3 pos)
{
    half ior_air = 1.0003;
    half ior_skin = 1.55;

    half a = acos(dot(normal, normalize(state.camera.xyz - pos))) / 2.0;
    half r0 = pow(((ior_air - ior_skin) / (ior_air + ior_skin)), 2.0);

    return pow(r0 + (1. - r0)*(1. - cos(a)), 5.0);
}

float GGXNormalDistribution(float3 normal, float3 halfVec, float roughness)
{
    float r2 = pow(roughness, 2.);
    return PI * ( r2 / pow( pow( dot(normal, halfVec), 2. ) * (r2 - 1.) + 1., 2.));
}

void evalSpecularLobe(float3 normal, float3 viewDir, float3 lightDir)
{
    var halfVec = normalize(viewDir + lightDir);
}

// https://arxiv.org/pdf/2410.18026
vec3 efficientOrenNayar(vec3 albedo, float roughness, vec3 wi, vec3 wo, bool exact)
{
    float muI = wi.z;                                         // Input angle cos
    float muO = wo.z;                                         // Output angle cos
    float s  = dot(wi, wo) - muI * muO;                       // QON s term
    float sOverT = s > 0.0f ? s / max(muI, muO) : s;          // FON s/t
    float ACoeff = 1.0f / (1.0f + constant1_FON * roughness); // FON A coefficient
    vec3 singleScatterLobe = (albedo * rpi) * ACoeff * (1.0f + roughness * sOverT);
    float woF = exact ? efficientOrenNayarExact(muO, roughness) : efficientOrenNayarApprox(muO, roughness); // FON wo albedos (exact and approx)
    float wiF = exact ? efficientOrenNayarExact(muI, roughness) : efficientOrenNayarApprox(muI, roughness); // FON wi albedos (exact and approx)
    float avgAlbedo = ACoeff * (1.0f + constant2_FON * roughness);

}

[shader("fragment")]
float4 main(VSOutput input)
{
    float4 normal = normal_texture.Sample(input.Uv);
    //normal = pow(normal, 1/2.2);

    float4 color = diffuse_texture.Sample(input.Uv);
    color = pow(color, 1/2.2);

    var norm = input.Normal * normal.xyz;
    float3 light = max(dot(normalize(g_light_dir), normalize(norm)), float3(0.));
    float ambient = 1.;

    return float4(color.rgb * light + ambient * schlick(norm, input.WorldPos.xyz), 1.0);
}
