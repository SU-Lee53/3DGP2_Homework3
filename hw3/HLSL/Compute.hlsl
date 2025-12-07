
Texture2D<float4> gtxtTextureInputRO : register(t0);
RWTexture2D<float4> gtxtTextureOutputRW : register(u0);

cbuffer cbBlurScaleData : register(b0)
{
    float gfBlurScale;
};

SamplerState gssPoint : register(s0);
SamplerState gssLinear : register(s1);

groupshared float4 gSharedCache[(256 + (2 * 5))];

static float gfWeights[11] =
{
    0.05f, 0.05f, 0.1f, 0.1f, 0.1f, 0.2f,
    0.1f, 0.1f, 0.1f, 0.05f, 0.05f
};

[numthreads(256, 1, 1)]
void CSHorzBlur(int3 vGroupThreadID : SV_GroupThreadID, int3 vDispatchThreadID : SV_DispatchThreadID)
{
    uint inputWidth = 0, inputHeight = 0;
    gtxtTextureInputRO.GetDimensions(inputWidth, inputHeight);
    
    if(vGroupThreadID.x < 5)
    {
        int x = max(vDispatchThreadID.x - 5, 0);
        gSharedCache[vGroupThreadID.x] = gtxtTextureInputRO[int2(x, vDispatchThreadID.y)];
    }
    else if (vGroupThreadID.x >= 256 - 5)
    {
        int x = min(vDispatchThreadID.x + 5, inputHeight - 1);
        gSharedCache[vGroupThreadID.x + 2 * 5] = gtxtTextureInputRO[int2(x, vDispatchThreadID.y)];
    }
    gSharedCache[vGroupThreadID.x + 5] = gtxtTextureInputRO[min(vDispatchThreadID.xy, int2(inputWidth - 1, inputHeight - 1))];
    
    GroupMemoryBarrierWithGroupSync();
    
    float4 cBlurredColor = float4(0, 0, 0, 0);
    float4 cOriginalColor = gSharedCache[vGroupThreadID.x + 5];
    float4 cFinalColor = float4(0, 0, 0, 0);
    for (int i = -5; i <= 5; i++)
    {
        int k = vGroupThreadID.x + 5 + i;
        cBlurredColor += gfWeights[i + 5] * gSharedCache[k];
    }
    cFinalColor = lerp(cOriginalColor, cBlurredColor, gfBlurScale);
    
    gtxtTextureOutputRW[vDispatchThreadID.xy] = cFinalColor;
}

[numthreads(1, 256, 1)]
void CSVertBlur(int3 vGroupThreadID : SV_GroupThreadID, int3 vDispatchThreadID : SV_DispatchThreadID)
{
    uint inputWidth = 0, inputHeight = 0;
    gtxtTextureInputRO.GetDimensions(inputWidth, inputHeight);
    
    if (vGroupThreadID.y < 5)
    {
        int y = max(vDispatchThreadID.y - 5, 0);
        gSharedCache[vGroupThreadID.y] = gtxtTextureInputRO[int2(vDispatchThreadID.x, y)];
    }
    else if (vGroupThreadID.y >= 256 - 5)
    {
        int y = min(vDispatchThreadID.y + 5, inputWidth - 1);
        gSharedCache[vGroupThreadID.y + 2 * 5] = gtxtTextureInputRO[int2(vDispatchThreadID.x, y)];
    }
    gSharedCache[vGroupThreadID.y + 5] = gtxtTextureInputRO[min(vDispatchThreadID.xy, int2(inputWidth - 1, inputHeight - 1))];
    
    GroupMemoryBarrierWithGroupSync();
    
    float4 cBlurredColor = float4(0, 0, 0, 0);
    float4 cOriginalColor = gSharedCache[vGroupThreadID.y + 5];
    float4 cFinalColor = float4(0, 0, 0, 0);
    for (int i = -5; i <= 5; i++)
    {
        int k = vGroupThreadID.y + 5 + i;
        cBlurredColor += gfWeights[i + 5] * gSharedCache[k];
    }
    cFinalColor = lerp(cOriginalColor, cBlurredColor, gfBlurScale);
    
    gtxtTextureOutputRW[vDispatchThreadID.xy] = cFinalColor;
}
