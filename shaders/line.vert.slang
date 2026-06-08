// Baked-in NDC-space line strip (a simple zigzag)
static const float2 kLineVerts[] = {
    float2(-0.8f, -0.5f),
    float2(-0.4f,  0.5f),
    float2( 0.0f, -0.2f),
    float2( 0.4f,  0.5f),
    float2( 0.8f, -0.5f),
};

struct VSOut {
    float4 pos : SV_Position;
};

[shader("vertex")]
VSOut vsMain(uint vid : SV_VertexID) {
    VSOut o;
    o.pos = float4(kLineVerts[vid], 0.0f, 1.0f);
    return o;
}