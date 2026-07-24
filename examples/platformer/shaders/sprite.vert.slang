struct Sprite
{
    float x;
    float y;
    float width;
    float height;
    uint32_t totalSpriteX;
    uint32_t totalSpriteY;
    uint32_t currentIndex;
    float screenWidth;
    float screenHeight;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

[[vk::push_constant]]
Sprite data;

static const uint quadIndices[6] = { 0, 1, 2, 2, 1, 3 };

[shader("vertex")]
VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput out;
    uint idx = quadIndices[vertexID];

    float2 point = float2(idx & 1, idx >> 1);
    float2 pixelPosition = float2(data.x, data.y) + point * float2(data.width, data.height);
    float2 uvSpace = pixelPosition / float2(data.screenWidth, data.screenHeight);

    out.uv = point;

    out.position = float4(uvSpace * 2.0 - 1.0, 0.0, 1.0);
    return out;
}