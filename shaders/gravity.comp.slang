[[vk::binding(0, 0)]]
RWTexture2D<float4> renderTarget;

[shader("compute")]
[[numthreads(1, 1, 1)]]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID, uint3 LocalInvocationID : SV_GroupThreadID)
{
    var color = renderTarget[GlobalInvocationID.xy];
    renderTarget[GlobalInvocationID.xy] = color;
}