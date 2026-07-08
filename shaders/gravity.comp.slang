[[vk::binding(0, 0)]]
[vk::image_format("rgba8")]
RWTexture2D<float4> renderTarget;

[[vk::binding(1, 0)]]
RWStructuredBuffer<float4> data;

float EP = 0.00001;

float3 bodybodyInteraction(float4 bi, float4 bj)
{
    float3 r = (bj - bi).xyz;
    float distSqr = dot(r, r) + EP;
    float distSixth = pow(distSqr, 3);
    float invDistCube = 1.0/sqrt(distSixth);
    float s = bj.w * invDistCube;

    return (r * s).xyz;
}

[shader("compute")]
[[numthreads(9, 9, 1)]]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    var planet1 = data[GlobalInvocationID.x];
    var planet2 = data[GlobalInvocationID.y];

    var effect = bodybodyInteraction(planet1, planet2);

    data[GlobalInvocationID.x] = planet1;
}