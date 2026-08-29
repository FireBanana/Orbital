#define PI 3.141593

static const float constant1_FON = 0.5f - 2.0f / (3.0f * PI);
static const float constant2_FON = 2.0f  / 3.0f - 28.0f / (15.0f * PI);
static const float rpi = 1.0f / PI;

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

// float FONExact(float mu, float r)
// {
//     float AF = 1.0f / (1.0f + constant1_FON * r); // FON A coeff
//     float BF = r * AF;                            // FON B coeff
//     float Si = sqrt(1.0f - (mu  * mu));
//     float G = Si * (acos(mu) - Si * mu) + (2.0f / 3.0f) * ((Si / mu) * (1.0f - (Si *  Si * Si)) - Si);
//     return AF + (BF * rpi) * G;
// }

// float FONApprox(float mu, float r)
// {
//     float mucomp = 1.0f - mu;
//     const float g1 = 0.0571085289f;
//     const float g2 = 0.491881867f;
//     const float g3 = -0.332181442f;
//     const float g4 = 0.0714429953f;
//
//     float gOverPi = mucomp * (g1 + mucomp * (g2 + mucomp * (g3 + mucomp * g4)));
//     return (1.0f + r * gOverPi) / (1.0f + constant1_FON * r);
// }

// https://arxiv.org/pdf/2410.18026
// wi: surface point towards the light
// wo: surface point towards the camera
// float3 efficientOrenNayar(float3 albedo, float roughness, float3 wi, float3 wo, bool exact)
// {
//     float muI = wi.z;                                         // Input angle cos
//     float muO = wo.z;                                         // Output angle cos
//     float s  = dot(wi, wo) - muI * muO;                       // QON s term
//     float sOverT = s > 0.0f ? s / max(muI, muO) : s;          // FON s/t
//     float ACoeff = 1.0f / (1.0f + constant1_FON * roughness); // FON A coefficient
//     float3 singleScatterLobe = (albedo * rpi) * ACoeff * (1.0f + roughness * sOverT);
//     float woF = exact ? FONExact(muO, roughness) : FONApprox(muO, roughness); // FON wo albedos (exact and approx)
//     float wiF = exact ? FONExact(muI, roughness) : FONApprox(muI, roughness); // FON wi albedos (exact and approx)
//     float avgAlbedo = ACoeff * (1.0f + constant2_FON * roughness);
//     float3 albedoMs = (albedo * albedo) * avgAlbedo / (float3(1.0f) - albedo * (1.0f - avgAlbedo));
//     const float eps = 1.0e-7f;
//     float3 multiScatterLobe = (albedoMs * rpi) * max(eps, 1.0f - woF) * max(eps, 1.0f - wiF) / max(eps, 1.0f - avgAlbedo);
//
//     return singleScatterLobe + multiScatterLobe;
// }

///-----------------------------------

// qualitativeOrenNayar
float3 qon(float3 normal, float3 albedo, float3 in, float3 out, float r)
{
    var a = 1. - 0.5 * ((r * r) / (r * r + 0.33));
    var b = 0.45  * ((r * r) / (r * r + 0.09));

    var s = dot(in, out) - (dot(normal, in) * dot(normal, out));
    var t = s <= 0  ? 0 : (1. /  max(max(dot(normal, in), dot(normal, out)), 1e-4));

    return (albedo / PI) * (a + b * (s * t));
}

[shader("fragment")]
float4 main(VSOutput input) : SV_Target
{
    float4 normal = normal_texture.Sample(input.Uv);
    //normal = pow(normal, 1/2.2);

    float4 color = diffuse_texture.Sample(input.Uv);

    var norm = input.Normal * normal.xyz;
    float3 light = max(dot(normalize(g_light_dir), normalize(norm)), float3(0.));
    float ambient = 1.;

    //return float4(color.rgb * light + ambient * schlick(norm, input.WorldPos.xyz), 1.0);

    float3x3 model = (float3x3)state.model;
    float3 n = normalize(input.Normal);
    float3 l = normalize(mul(g_light_dir, model));
    float3 v = normalize(mul(state.camera.xyz - input.WorldPos.xyz, model));

    var diff = qon(n, color.rgb, l, v, 0.5);
    diff *= max(0.0, dot(n, l));

    diff = pow(diff, 1/2.2);

    return float4(diff, 1.);
}
