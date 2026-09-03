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

[[vk::binding(2, 0)]]
Sampler2D roughMetal_texture;

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

///-----------------------------------

// schlick, f90 = 1
float3 fresnel(float VoH, float3 f0)
{
    float f = pow(1.0 - VoH, 5.0);
    return f + f0 * (1.0 - f);
}

// Smith GGX Correlated
float geometry(float NoV, float NoL, float r)
{
    float a = r;
    float GGXV = NoL * (NoV * (1.0 - a) + a);
    float GGXL = NoV * (NoL * (1.0 - a) + a);
    return 0.5 / (GGXV + GGXL);
}

// GGX high precision version
float ndf(float NoH, float r)
{
    float a = NoH * r;
    float k = r / (1.0 - NoH * NoH + a * a);
    return k * k * (1.0 / PI);
}

// specular
float3 specular(float3 v, float3 l, float3  n, float roughness)
{
    float3 h = normalize(v + l);
    float NoV = abs(dot(n, v)) + 1e-5;
    float NoL = clamp(dot(n, l), 0. , 1.);
    float NoH = clamp(dot(n, h), 0. , 1.);
    float LoH = clamp(dot(l, h), 0. , 1.);

    float r = roughness * roughness;

    float d = ndf(NoH, r);
    float3 f = fresnel(LoH, float3(0.04)); // dielectric value
    float v = geometry(NoV, NoL, r);

    return d * v * f;
}

// qualitativeOrenNayar
float3 qon(float3 normal, float3 albedo, float3 in, float3 out, float r)
{
    var a = 1. - 0.5 * ((r * r) / (r * r + 0.33));
    var b = 0.45  * ((r * r) / (r * r + 0.09));

    var s = dot(in, out) - (dot(normal, in) * dot(normal, out));
    var t = s <= 0  ? 0 : (1. /  max(max(dot(normal, in), dot(normal, out)), 1e-4));

    return (albedo / PI) * (a + b * (s * t));
}

///-----------------------------------

[shader("fragment")]
float4 main(VSOutput input) : SV_Target
{
    float4 normal = normal_texture.Sample(input.Uv);
    float4 color = diffuse_texture.Sample(input.Uv);
    float4 roughMetal = roughMetal_texture.Sample(input.Uv);
    float roughness = roughMetal.g;
    float metal = roughMetal.b;

    var norm = input.Normal * normal.xyz;
    float3 light = max(dot(normalize(g_light_dir), normalize(norm)), float3(0.));
    float ambient = 1.;

    float3x3 model = (float3x3)state.model;
    float3 n = normalize(input.Normal * normal.xyz);
    float3 l = normalize(mul(g_light_dir, model));
    float3 v = normalize(mul(state.camera.xyz - input.WorldPos.xyz, model));

    var spec = specular(v, l, n, roughness);
    spec *= max(0.0, dot(n, l));

    var diff = qon(n, color.rgb, l, v, roughness);
    diff *= max(0.0, dot(n, l));

    var col = diff + spec;
    col = col / (col + 1.);
    col = pow(col, 1/2.2);

    return float4(col, 1.);
}
