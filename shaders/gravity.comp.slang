[[vk::binding(0, 0)]]
[vk::image_format("rgba8")]
RWTexture2D<float4> renderTarget;

[shader("compute")]
[[numthreads(1, 1, 1)]]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID, uint3 LocalInvocationID : SV_GroupThreadID)
{
    var color = renderTarget[GlobalInvocationID.xy];
    color *= 0.3;
    renderTarget[GlobalInvocationID.xy] = color;
}