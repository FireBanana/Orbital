[[vk::binding(0, 0)]]
[vk::image_format("rgba8")]
RWTexture2D<float4> renderTarget;

[shader("compute")]
[[numthreads(1, 1, 1)]]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    int2 pos = int2(GlobalInvocationID.xy);
    float4 colorSum = float4(0.0, 0.0, 0.0, 0.0);

    // Change this radius to control blur strength:
    const int RADIUS = 1;
    const float SAMPLE_COUNT = float((RADIUS * 2 + 1) * (RADIUS * 2 + 1));

    // Accumulate the massive neighborhood
    for (int x = -RADIUS; x <= RADIUS; ++x)
    {
        for (int y = -RADIUS; y <= RADIUS; ++y)
        {
            colorSum += renderTarget[pos + int2(x, y)];
        }
    }

    // Average the accumulated colors
    renderTarget[pos] = colorSum / SAMPLE_COUNT;
}