
Texture2D<float4> gtxtTextureInputRO : register(t0);
RWTexture2D<float4> gtxtTextureOutputRW : register(u0);

SamplerState gssPoint : register(s0);
SamplerState gssLinear : register(s1);

groupshared float4 gSharedCache[256];

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
    for (int i = -5; i <= 5; i++)
    {
        int k = vGroupThreadID.x + 5 + i;
        cBlurredColor += gfWeights[i + 5] * gSharedCache[k];
    }
    
    gtxtTextureOutputRW[vDispatchThreadID.xy] = cBlurredColor;
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
    for (int i = -5; i <= 5; i++)
    {
        int k = vGroupThreadID.y + 5 + i;
        cBlurredColor += gfWeights[i + 5] * gSharedCache[k];
    }
    
    gtxtTextureOutputRW[vDispatchThreadID.xy] = cBlurredColor;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// FullScreen Graphics Pass

struct VS_FULLSCREEN_OUTPUT
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
};

VS_FULLSCREEN_OUTPUT VSFullScreen(uint nVertexID : SV_VertexID)
{
    VS_FULLSCREEN_OUTPUT output;
    
    if (nVertexID == 0)
    {
        output.position = float4(float2(-1.f, 1.f), 0.f, 1.f);
        output.uv = float2(0.f, 0.f);
    }
    else if (nVertexID == 1)
    {
        output.position = float4(float2(1.f, 1.f), 0.f, 1.f);8
        output.uv = float2(1.f, 0.f);
    }
    else if (nVertexID == 2)
    {
        output.position = float4(float2(-1.f, -1.f), 0.f, 1.f);
        output.uv = float2(0.f, 1.f);
    }
    else if (nVertexID == 3)
    {
        output.position = float4(float2(1.f, 1.f), 0.f, 1.f);
        output.uv = float2(1.f, 0.f);
    }
    else if (nVertexID == 4)
    {
        output.position = float4(float2(1.f, -1.f), 0.f, 1.f);
        output.uv = float2(1.f, 1.f);
    }
    else if (nVertexID == 5)
    {
        output.position = float4(float2(-1.f, -1.f), 0.f, 1.f);
        output.uv = float2(0.f, 1.f);
    }
    
    return output;
}

float4 PSFullScreen(VS_FULLSCREEN_OUTPUT input) : SV_Target
{
    return gtxtTextureInputRO.Sample(gssPoint, input.uv);
}