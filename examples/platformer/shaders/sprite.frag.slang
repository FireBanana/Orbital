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

struct VSInput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

[[vk::binding(0, 0)]]
Sampler2D sprite;

[[vk::push_constant]]
Sprite data;

[shader("fragment")]
float4 main(VSInput input) : SV_Target
{
    float2 cellSize = float2(1.0 / data.totalSpriteX,
                             1.0 / data.totalSpriteY);

    uint cellX = data.currentIndex % data.totalSpriteX;
    uint cellY = data.currentIndex / data.totalSpriteX;

    float2 cellOrigin = float2(cellX, cellY) * cellSize;
    float2 uv = cellOrigin + frac(input.uv) * cellSize;
    uv.y = 1. - uv.y;

    float4 col = sprite.Sample(uv);

    if(col.a < 0.5)
        discard;

    return col;
}