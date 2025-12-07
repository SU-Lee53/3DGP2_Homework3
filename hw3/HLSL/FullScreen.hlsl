
Texture2D<float4> gtxtTextureInputRO : register(t0);
SamplerState gssPoint : register(s0);
SamplerState gssLinear : register(s1);

struct VS_FULLSCREEN_OUTPUT
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
};

VS_FULLSCREEN_OUTPUT VSFullScreen(uint nVertexID : SV_VertexID)
{
    float2 verts[6] =
    {
        float2(-1, 1),
        float2(1, 1),
        float2(-1, -1),
        float2(1, 1),
        float2(1, -1),
        float2(-1, -1)
    };
    
    
    float2 uvs[6] =
    {
        float2(0.f, 0.f),
        float2(1.f, 0.f),
        float2(0.f, 1.f),
        float2(1.f, 0.f),
        float2(1.f, 1.f),
        float2(0.f, 1.f)
    };
    
    VS_FULLSCREEN_OUTPUT output;
    
    output.position = float4(verts[nVertexID], 0.f, 1.f);
    output.uv = uvs[nVertexID];
    
    return output;
}

float4 PSFullScreen(VS_FULLSCREEN_OUTPUT input) : SV_Target
{
    return gtxtTextureInputRO.Sample(gssPoint, input.uv);
}