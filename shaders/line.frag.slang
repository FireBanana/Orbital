struct VSIn {
    float4 pos : SV_Position;
};

[shader("fragment")]
float4 fsMain(VSIn input) : SV_Target {
    return float4(0.2f, 0.8f, 1.0f, 0.5f); // light blue
}