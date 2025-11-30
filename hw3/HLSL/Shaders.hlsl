#include "Common.hlsl"
#include "Light.hlsl"

//#define _WITH_VERTEX_LIGHTING

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Standard Shader

struct VS_STANDARD_INPUT
{
	float3 position		: POSITION;
    float2 uv			: TEXCOORD0;
	float3 normal		: NORMAL;
    float3 tangent		: TANGENT;
    float3 biTangent	: BITANGENT;
};

struct VS_STANDARD_OUTPUT
{
	float4 position		: SV_POSITION;
	float3 positionW	: POSITION;
    float2 uv			: TEXCOORD0;
	float3 normalW		: NORMAL;
	float3 tangentW		: TANGENT;
	float3 biTangentW	: BITANGENT;
};

VS_STANDARD_OUTPUT VSStandard(VS_STANDARD_INPUT input, uint nInstanceID : SV_InstanceID)
{
    VS_STANDARD_OUTPUT output;

    matrix mtxViewProjection = mul(gmtxView, gmtxProjection);
	
    matrix mtxWorld = gnBaseIndex == -1 ? gmtxWorld : sbInstanceData[gnBaseIndex + nInstanceID];
    
    output.positionW = mul(float4(input.position, 1.f), mtxWorld).xyz;
    output.position = mul(float4(output.positionW, 1.f), mtxViewProjection);
	
    output.normalW = mul(float4(input.normal, 0.f), mtxWorld).xyz;
    output.tangentW = mul(float4(input.tangent, 0.f), mtxWorld).xyz;
    output.biTangentW = mul(float4(input.biTangent, 0.f), mtxWorld).xyz;
	
    output.uv = input.uv;
	
	return output;
}

float3 ComputeNormal(float3 normalW, float3 tangentW, float2 uv)
{
    float3 N = normalize(normalW);
    float3 T = normalize(tangentW - dot(tangentW, N) * N);
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);
    
    float3 normalFromMap = gtxtNormalMap.Sample(gssWrap, uv).rgb;
    float3 normal = (normalFromMap * 2.0f) - 1.0f; // [0, 1] ---> [-1, 1]
    
    return normalize(mul(normal, TBN));
}

float4 PSStandard(VS_STANDARD_OUTPUT input) : SV_TARGET
{
    float4 cAlbedoColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 cSpecularColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 cNormalColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 cMetallicColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 cEmissionColor = float4(0.0f, 0.0f, 0.0f, 1.0f);

    if (gMaterial.m_textureMask & MATERIAL_TYPE_ALBEDO_MAP)
        cAlbedoColor = gtxtAlbedoMap.Sample(gssWrap, input.uv);
    if (gMaterial.m_textureMask & MATERIAL_TYPE_SPECULAR_MAP)
        cSpecularColor = gtxtSpecularMap.Sample(gssWrap, input.uv);
    if (gMaterial.m_textureMask & MATERIAL_TYPE_NORMAL_MAP)
        cNormalColor = gtxtNormalMap.Sample(gssWrap, input.uv);
    if (gMaterial.m_textureMask & MATERIAL_TYPE_METALLIC_MAP)
        cMetallicColor = gtxtMetallicMap.Sample(gssWrap, input.uv);
    if (gMaterial.m_textureMask & MATERIAL_TYPE_EMISSION_MAP)
        cEmissionColor = gtxtEmissionMap.Sample(gssWrap, input.uv);

    float4 cIllumination = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 cColor = cAlbedoColor + cSpecularColor + cEmissionColor;
    if (gMaterial.m_textureMask & MATERIAL_TYPE_NORMAL_MAP)
    {
        float3 normalW = ComputeNormal(input.normalW, input.tangentW, input.uv);
        cIllumination = Lighting(input.positionW, normalW);
    }
    
    cColor = lerp(cColor, cIllumination, 0.5f);
    return cColor;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mirror Shader
// Stencil 버퍼에 그릴때 쓸 쉐이더 -> 하는김에 거울부분의 Depth 도 1로 맞추기 위해 xyww 를 사용

VS_STANDARD_OUTPUT VSMirror(VS_STANDARD_INPUT input, uint nInstanceID : SV_InstanceID)
{
    VS_STANDARD_OUTPUT output;

    matrix mtxViewProjection = mul(gmtxView, gmtxProjection);
    
    output.positionW = mul(float4(input.position, 1.f), gmtxWorld).xyz;
    output.position = mul(float4(output.positionW, 1.f), mtxViewProjection).xyww;
	
    output.normalW = mul(float4(input.normal, 0.f), gmtxWorld).xyz;
    output.tangentW = mul(float4(input.tangent, 0.f), gmtxWorld).xyz;
    output.biTangentW = mul(float4(input.biTangent, 0.f), gmtxWorld).xyz;
	
    output.uv = input.uv;
	
	return output;
}

float4 PSMirror(VS_STANDARD_OUTPUT input) : SV_TARGET
{
    return float4(0.f, 0.f, 0.f, 1.f);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainShader

struct VS_TERRAIN_INPUT
{
    float3 position : POSITION;
    float4 color    : COLOR;
    float2 uv0      : TEXCOORD0;
    float2 uv1      : TEXCOORD1;
};

struct VS_TERRAIN_OUTPUT
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
    float2 uv0      : TEXCOORD0;
    float2 uv1      : TEXCOORD1;
};

VS_TERRAIN_OUTPUT VSTerrain(VS_TERRAIN_INPUT input)
{
    VS_TERRAIN_OUTPUT output;
    
    matrix mtxViewProjection = mul(gmtxView, gmtxProjection);
    
    float3 positionW = mul(float4(input.position, 1.f), gmtxTerrainWorld).xyz;
    output.position = mul(float4(positionW, 1.f), mtxViewProjection);
	
    output.color = input.color;
    output.uv0 = input.uv0;
    output.uv1 = input.uv1;
	
    return output;
    
}

float4 PSTerrain(VS_TERRAIN_OUTPUT input) : SV_TARGET0
{
    float4 cBaseColor = gtxtAlbedoMap.Sample(gssWrap, input.uv0);
    float4 cDetailColor = gtxtDetailAlbedoMap.Sample(gssWrap, input.uv1 + gvTerrainUVOffset);
    
    float4 cFinalColor = lerp(cBaseColor, cDetailColor, 0.5);
    
    return cFinalColor;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Terrain TESSELLATION Shader

struct VS_TERRAIN_TESSELLATION_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};

struct VS_TERRAIN_TESSELLATION_OUTPUT
{
    float3 positionW : POSITION;
    float4 color : COLOR;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};

struct HS_TERRAIN_TESSELLATION_OUTPUT
{
    float3 positionW : POSITION;
    float4 color : COLOR;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};

struct HS_TERRAIN_TESSELLATION_CONSTANT_OUTPUT
{
    float fTessEdges[4] : SV_TessFactor;
    float fTessInsides[2] : SV_InsideTessFactor;
};

struct DS_TERRAIN_TESSELLATION_OUTPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};

VS_TERRAIN_TESSELLATION_OUTPUT VSTerrainTessellated(VS_TERRAIN_TESSELLATION_INPUT input)
{
    VS_TERRAIN_TESSELLATION_OUTPUT output;
    
    output.positionW = input.position;
    output.color = input.color;
    output.uv0 = input.uv0;
    output.uv1 = input.uv1;
    
    return output;
}

[domain("quad")]
[partitioning("fractional_even")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("HSTerrainTessellatedConstant")]
[maxtessfactor(64.0f)]
HS_TERRAIN_TESSELLATION_OUTPUT HSTerrainTessellated(InputPatch<VS_TERRAIN_TESSELLATION_OUTPUT, 4> input, uint i : SV_OutputControlPointID)
{
    HS_TERRAIN_TESSELLATION_OUTPUT output;
    output.positionW = input[i].positionW;
    output.color = input[i].color;
    output.uv0 = input[i].uv0;
    output.uv1 = input[i].uv1;

    return output;
}

#define MAX_LOD_DISTANCE 100.f

float CalculateTessFactor(float3 positionW)
{
    float fDistToCamera = distance(positionW, gvCameraPosition);
    float s = saturate((fDistToCamera - 10.f) / MAX_LOD_DISTANCE - 10.f);
    return pow(2, lerp(5.f, 1.f, s));
}

HS_TERRAIN_TESSELLATION_CONSTANT_OUTPUT HSTerrainTessellatedConstant(InputPatch<VS_TERRAIN_TESSELLATION_OUTPUT, 4> input, uint nPatchID : SV_PrimitiveID)
{
    HS_TERRAIN_TESSELLATION_CONSTANT_OUTPUT output;
    
    float3 e1 = 0.5f * (input[0].positionW + input[2].positionW);
    float3 e0 = 0.5f * (input[2].positionW + input[3].positionW);
    float3 e2 = 0.5f * (input[1].positionW + input[3].positionW);
    float3 e3 = 0.5f * (input[0].positionW + input[1].positionW);
    
    output.fTessEdges[0] = CalculateTessFactor(e0);
    output.fTessEdges[1] = CalculateTessFactor(e1);
    output.fTessEdges[2] = CalculateTessFactor(e2);
    output.fTessEdges[3] = CalculateTessFactor(e3);
    
    float3 c = 0.25f * (input[0].positionW + input[1].positionW + input[2].positionW + input[3].positionW);
    
    output.fTessInsides[0] = CalculateTessFactor(c);
    output.fTessInsides[1] = output.fTessInsides[0];
    
    return output;
}

float4 BernsteinCoefficient(float t)
{
    float tInv = 1.0f - t;
    
    // P(t) = 
    // (1 - t)^3 * P0 + 
    // 3(1 - t)^2 * t * P1 +
    // 3(1 - t) * t^2 * p2 + 
    // t^3 * p3
    return float4(tInv * tInv * tInv, 3.f * t * tInv * tInv, 3.f * t * t * tInv, t * t * t);
}

float3 CubicBezierSum(OutputPatch<HS_TERRAIN_TESSELLATION_OUTPUT, 4> patch, float4 u, float4 v)
{
    float3 sum = float3(0, 0, 0);
    sum = v.x * (u.x * patch[0].positionW + u.y * patch[1].positionW + u.z * patch[2].positionW + u.w * patch[3].positionW);
    sum += v.x * (u.x * patch[4].positionW + u.y * patch[5].positionW + u.z * patch[6].positionW + u.w * patch[7].positionW);
    sum += v.x * (u.x * patch[8].positionW + u.y * patch[9].positionW + u.z * patch[10].positionW + u.w * patch[11].positionW);
    sum += v.x * (u.x * patch[12].positionW + u.y * patch[13].positionW + u.z * patch[14].positionW + u.w * patch[15].positionW);
    
    return sum;
}

[domain("quad")]
DS_TERRAIN_TESSELLATION_OUTPUT DSTerrainTessellated(HS_TERRAIN_TESSELLATION_CONSTANT_OUTPUT patchConstant, float2 uv : SV_DomainLocation, OutputPatch<HS_TERRAIN_TESSELLATION_OUTPUT, 4> patch)
{
    DS_TERRAIN_TESSELLATION_OUTPUT output;
    
    //float4 uB = BernsteinCoefficient(uv.x);
    //float4 vB = BernsteinCoefficient(uv.y);
    //float3 position = CubicBezierSum(patch, uB, vB);
    //
    //matrix mtxWVP = mul(gmtxTerrainWorld, gmtxView);
    //mtxWVP = mul(mtxWVP, gmtxProjection);
    //output.position = mul(float4(position, 1.f), mtxWVP);
    
    float3 positionW = lerp(lerp(patch[0].positionW, patch[1].positionW, uv.x), lerp(patch[2].positionW, patch[3].positionW, uv.x), uv.y);
    output.color = lerp(lerp(patch[0].color, patch[1].color, uv.x), lerp(patch[2].color, patch[3].color, uv.x), uv.y);
    output.uv0 = lerp(lerp(patch[0].uv0, patch[1].uv0, uv.x), lerp(patch[2].uv0, patch[3].uv0, uv.x), uv.y);
    output.uv1 = lerp(lerp(patch[0].uv1, patch[1].uv1, uv.x), lerp(patch[2].uv1, patch[3].uv1, uv.x), uv.y);
    
    matrix mtxWVP = mul(gmtxTerrainWorld, gmtxView);
    mtxWVP = mul(mtxWVP, gmtxProjection);
    output.position = mul(float4(positionW, 1.f), mtxWVP);
    
    return output;
}

float4 PSTerrainTessellated(DS_TERRAIN_TESSELLATION_OUTPUT input) : SV_TARGET0
{
    float4 cBaseColor = gtxtAlbedoMap.Sample(gssWrap, input.uv0);
    float4 cDetailColor = gtxtDetailAlbedoMap.Sample(gssWrap, input.uv1 + gvTerrainUVOffset);
    
    float4 cFinalColor = lerp(cBaseColor, cDetailColor, 0.5);
    
    return cFinalColor;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BillboardShader

struct VS_BILLBOARD_OUTPUT
{
    uint nVertexID : VERTEXID;
};

struct GS_BILLBOARD_OUTPUT
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
    uint textureIndex : TEXTUREINDEX;
};

VS_BILLBOARD_OUTPUT VSBillboard(uint nVertexID : SV_VertexID)
{
    VS_BILLBOARD_OUTPUT output;
    output.nVertexID = nVertexID;
    return output;
}

[maxvertexcount(4)]
void GSBillboard(point VS_BILLBOARD_OUTPUT input[1], inout TriangleStream<GS_BILLBOARD_OUTPUT> outStream)
{
    float3 vUp = float3(0.f, 1.f, 0.f);
    float3 vLook = gvCameraPosition.xyz - gBillboard[input[0].nVertexID].vPosition;
    vLook = normalize(vLook);
    float3 vRight = cross(vUp, vLook);
    
    float fHalfWidth = gBillboard[input[0].nVertexID].vSize.x * 0.5f;
    float fHalfHeight = gBillboard[input[0].nVertexID].vSize.y * 0.5f;
    
    float4 vertices[4];
    vertices[0] = float4(gBillboard[input[0].nVertexID].vPosition + (fHalfWidth * vRight) + (fHalfHeight * vUp), 1.f);
    vertices[1] = float4(gBillboard[input[0].nVertexID].vPosition - (fHalfWidth * vRight) + (fHalfHeight * vUp), 1.f);
    vertices[2] = float4(gBillboard[input[0].nVertexID].vPosition + (fHalfWidth * vRight) - (fHalfHeight * vUp), 1.f);
    vertices[3] = float4(gBillboard[input[0].nVertexID].vPosition - (fHalfWidth * vRight) - (fHalfHeight * vUp), 1.f);
    
    float2 uvs[4] = { float2(0.f, 0.f), float2(1.f, 0.f), float2(0.f, 1.f), float2(1.f, 1.f) };
    
    matrix mtxViewProjection = mul(gmtxView, gmtxProjection);
    
    GS_BILLBOARD_OUTPUT output;
    
    output.position = mul(vertices[0], mtxViewProjection);
    output.uv = uvs[0];
    output.textureIndex = gBillboard[input[0].nVertexID].nTextureIndex;
    outStream.Append(output);
    
    output.position = mul(vertices[1], mtxViewProjection);
    output.uv = uvs[1];
    output.textureIndex = gBillboard[input[0].nVertexID].nTextureIndex;
    outStream.Append(output);
    
    output.position = mul(vertices[2], mtxViewProjection);
    output.uv = uvs[2];
    output.textureIndex = gBillboard[input[0].nVertexID].nTextureIndex;
    outStream.Append(output);
    
    output.position = mul(vertices[3], mtxViewProjection);
    output.uv = uvs[3];
    output.textureIndex = gBillboard[input[0].nVertexID].nTextureIndex;
    outStream.Append(output);
}

float4 PSBillboard(GS_BILLBOARD_OUTPUT input) : SV_Target0
{
    float4 cColor = gtxtTerrainBillboards[input.textureIndex].Sample(gssWrap, input.uv);
    if (cColor.a <= 0.3f)
        discard; //clip(cColor.a - 0.3f);
    
    return cColor;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Skybox

struct VS_SKYBOX_OUTPUT
{
    float4 position : POSITION;
};

struct GS_SKYBOX_OUTPUT
{
    float4 position : SV_Position;
    float3 uv : TEXCOORD0;
};

VS_SKYBOX_OUTPUT VSSkybox(uint nVertexID : SV_VertexID)
{
    VS_SKYBOX_OUTPUT output;
    output.position = float4(0.f, 0.f, 0.f, 1.f);
    return output;
}

[maxvertexcount(24)]
void GSSkybox(point VS_SKYBOX_OUTPUT input[1], inout TriangleStream<GS_SKYBOX_OUTPUT> outStream)
{
    float3 vCenter = gvCameraPosition;
    float3 vExtent = 5.f;
    
    float3 vAxisX = float3(1.f, 0.f, 0.f);
    float3 vAxisY = float3(0.f, 1.f, 0.f);
    float3 vAxisZ = float3(0.f, 0.f, 1.f);
    
    float3 ex = vAxisX * vExtent.x;
    float3 ey = vAxisY * vExtent.y;
    float3 ez = vAxisZ * vExtent.z;
    
    matrix mtxVP = mul(gmtxView, gmtxProjection);
    
    float4 T00 = mul(float4(vCenter - ex + ey + ez, 1.f), mtxVP).xyww;
    float4 T01 = mul(float4(vCenter + ex + ey + ez, 1.f), mtxVP).xyww;
    float4 T10 = mul(float4(vCenter - ex + ey - ez, 1.f), mtxVP).xyww;
    float4 T11 = mul(float4(vCenter + ex + ey - ez, 1.f), mtxVP).xyww;
    
    float4 B00 = mul(float4(vCenter - ex - ey + ez, 1.f), mtxVP).xyww;
    float4 B01 = mul(float4(vCenter + ex - ey + ez, 1.f), mtxVP).xyww;
    float4 B10 = mul(float4(vCenter - ex - ey - ez, 1.f), mtxVP).xyww;
    float4 B11 = mul(float4(vCenter + ex - ey - ez, 1.f), mtxVP).xyww;
    
    GS_SKYBOX_OUTPUT output;
    
    output.position = T11; output.uv = float3(1.f, 0.f, 0.f); outStream.Append(output);
    output.position = T01; output.uv = float3(0.f, 0.f, 0.f); outStream.Append(output);
    output.position = B11; output.uv = float3(1.f, 1.f, 0.f); outStream.Append(output);
    output.position = B01; output.uv = float3(0.f, 1.f, 0.f); outStream.Append(output);
    outStream.RestartStrip();
    
    output.position = T00; output.uv = float3(1.f, 0.f, 1.f); outStream.Append(output);
    output.position = T10; output.uv = float3(0.f, 0.f, 1.f); outStream.Append(output);
    output.position = B00; output.uv = float3(1.f, 1.f, 1.f); outStream.Append(output);
    output.position = B10; output.uv = float3(0.f, 1.f, 1.f); outStream.Append(output);
    outStream.RestartStrip();
    
    output.position = T00; output.uv = float3(0.f, 1.f, 2.f); outStream.Append(output);
    output.position = T01; output.uv = float3(1.f, 1.f, 2.f); outStream.Append(output);
    output.position = T10; output.uv = float3(0.f, 0.f, 2.f); outStream.Append(output);
    output.position = T11; output.uv = float3(1.f, 1.f, 2.f); outStream.Append(output);
    outStream.RestartStrip();
    
    output.position = B01; output.uv = float3(1.f, 0.f, 3.f); outStream.Append(output);
    output.position = B00; output.uv = float3(0.f, 0.f, 3.f); outStream.Append(output);
    output.position = B11; output.uv = float3(1.f, 1.f, 3.f); outStream.Append(output);
    output.position = B10; output.uv = float3(0.f, 1.f, 3.f); outStream.Append(output);
    outStream.RestartStrip();
                                                     
    output.position = T01; output.uv = float3(1.f, 0.f, 4.f); outStream.Append(output);
    output.position = T00; output.uv = float3(0.f, 0.f, 4.f); outStream.Append(output);
    output.position = B01; output.uv = float3(1.f, 1.f, 4.f); outStream.Append(output);
    output.position = B00; output.uv = float3(0.f, 1.f, 4.f); outStream.Append(output);
    outStream.RestartStrip();
    
    output.position = T10; output.uv = float3(1.f, 0.f, 5.f); outStream.Append(output);
    output.position = T11; output.uv = float3(0.f, 0.f, 5.f); outStream.Append(output);
    output.position = B10; output.uv = float3(1.f, 1.f, 5.f); outStream.Append(output);
    output.position = B11; output.uv = float3(0.f, 1.f, 5.f); outStream.Append(output);
    outStream.RestartStrip();
    
}

float4 PSSkybox(GS_SKYBOX_OUTPUT input) : SV_Target0
{
    return gtxtSkyboxarr.Sample(gssClamp, input.uv);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OBBDebugShader

struct VS_DEBUG_OUTPUT
{
    float4 position : POSITION;
};

struct GS_DEBUG_OUTPUT
{
    float4 position : SV_Position;
};

VS_DEBUG_OUTPUT VSDebug(uint nVertexID : SV_VertexID)
{
    VS_DEBUG_OUTPUT output;
    output.position = float4(0.f, 0.f, 0.f, 1.f);
    return output;
}

float3 RotateByQuaternion(float3 vPosition, float4 Quaternion)
{
    float3 t = 2.0f * cross(Quaternion.xyz, vPosition);
    return vPosition + Quaternion.w * t + cross(Quaternion.xyz, t);
}

[maxvertexcount(24)]
void GSDebug(point VS_DEBUG_OUTPUT input[1], inout TriangleStream<GS_DEBUG_OUTPUT> outStream)
{
    float3 vCenter = gvOBBCenter;
    float3 vExtent = gvOBBExtent;
    float4 qOrientation = normalize(gvOBBOrientationQuat);
    
    float3 vAxisX = RotateByQuaternion(float3(1, 0, 0), qOrientation);
    float3 vAxisY = RotateByQuaternion(float3(0, 1, 0), qOrientation);
    float3 vAxisZ = RotateByQuaternion(float3(0, 0, 1), qOrientation);
    
    float3 ex = vAxisX * vExtent.x;
    float3 ey = vAxisY * vExtent.y;
    float3 ez = vAxisZ * vExtent.z;
    
    matrix mtxVP = mul(gmtxView, gmtxProjection);
    
    float4 T00 = mul(float4(vCenter - ex + ey + ez, 1.f), mtxVP);
    float4 T01 = mul(float4(vCenter + ex + ey + ez, 1.f), mtxVP);
    float4 T10 = mul(float4(vCenter - ex + ey - ez, 1.f), mtxVP);
    float4 T11 = mul(float4(vCenter + ex + ey - ez, 1.f), mtxVP);
    
    float4 B00 = mul(float4(vCenter - ex - ey + ez, 1.f), mtxVP);
    float4 B01 = mul(float4(vCenter + ex - ey + ez, 1.f), mtxVP);
    float4 B10 = mul(float4(vCenter - ex - ey - ez, 1.f), mtxVP);
    float4 B11 = mul(float4(vCenter + ex - ey - ez, 1.f), mtxVP);
    
    GS_DEBUG_OUTPUT output;
    
    output.position = T11; outStream.Append(output);
    output.position = T01; outStream.Append(output);
    output.position = B11; outStream.Append(output);
    output.position = B01; outStream.Append(output);
    outStream.RestartStrip();
    
    output.position = T00; outStream.Append(output);
    output.position = T10; outStream.Append(output);
    output.position = B00; outStream.Append(output);
    output.position = B10; outStream.Append(output);
    outStream.RestartStrip();
    
    output.position = T00; outStream.Append(output);
    output.position = T01; outStream.Append(output);
    output.position = T10; outStream.Append(output);
    output.position = T11; outStream.Append(output);
    outStream.RestartStrip();
    
    output.position = B01; outStream.Append(output);
    output.position = B00; outStream.Append(output);
    output.position = B11; outStream.Append(output);
    output.position = B10; outStream.Append(output);
    outStream.RestartStrip();
    
    output.position = T01; outStream.Append(output);
    output.position = T00; outStream.Append(output);
    output.position = B01; outStream.Append(output);
    output.position = B00; outStream.Append(output);
    outStream.RestartStrip();
    
    output.position = T10; outStream.Append(output);
    output.position = T11; outStream.Append(output);
    output.position = B10; outStream.Append(output);
    output.position = B11; outStream.Append(output);
    outStream.RestartStrip();
    
}

float4 PSDebug(GS_DEBUG_OUTPUT input) : SV_Target0
{
    return gcColor;
}