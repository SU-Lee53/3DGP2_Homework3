
cbuffer cbCameraData : register(b0)
{
    matrix gmtxView;
    matrix gmtxProjection;
    float3 gvCameraPosition;
};

#define MAX_EFFECT_PER_DRAW 100

struct ParticleData
{
    float3 vPosition;
    float fElapsedTime;
    float3 vForce;
    float pad;
};

cbuffer cbParticleData : register(b1)
{
    ParticleData gParticleData[MAX_EFFECT_PER_DRAW];
};

cbuffer cbParticleData : register(b2)
{
    uint gnDataIndex;
};

struct VS_PARTICLE_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 initialVelocity : VELOCITY;
    float2 initialSize : SIZE;
    float randomValue : RANDOM;
    float startTime : STARTTIME;
    float lifeTime : LIFETIME;
    float mass : MASS;
};

struct VS_PARTICLE_OUTPUT
{
    float3 positionW : POSITION;
    float4 color : COLOR;
    float2 size : SIZE;
};

struct GS_PARTICLE_OUTPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

const static float3 gvGravity = float3(0.f, -9.8f, 0.f);

VS_PARTICLE_OUTPUT VSExplosion(VS_PARTICLE_INPUT input, uint nInstanceID : SV_InstanceID)
{
    float fElapsedTime = gParticleData[gnDataIndex + nInstanceID].fElapsedTime;
    float3 vForce = gParticleData[gnDataIndex + nInstanceID].vForce;
    
    float3 vNewPosition = input.position + gParticleData[gnDataIndex + nInstanceID].vPosition;
    float4 cNewColor = float4(0.f, 0.f, 0.f, 0.f);
    float fNewAlpha = 0.f;
    float2 vNewSize = float2(0, 0);
    
    if (fElapsedTime <= input.lifeTime)
    {
        float fNewTime = fElapsedTime;
        float fNewTimeSq = fNewTime * fNewTime;
    
        float fOneMinusTime = 1 - frac(fElapsedTime / input.lifeTime); // 1 ~ 0
    
        // 새로운 위치의 계산
        
        // F = ma
        // 중력이 너무 약해서 좀 강하게 만듬
        float fForceX = vForce.x + (gvGravity.x * 350.f) * input.mass;
        float fForceY = vForce.y + (gvGravity.y * 350.f) * input.mass;
        float fForceZ = vForce.z + (gvGravity.z * 350.f) * input.mass;
    
        // F = ma -> a = F / m
        float fAccX = fForceX / input.mass;
        float fAccY = fForceY / input.mass;
        float fAccZ = fForceZ / input.mass;
    
        // s = v0t * 1/2at^2
        float fRandomValue = input.randomValue * 2000.f;
        float3 initialDirection = normalize(input.initialVelocity);
        float dX = (initialDirection.x * fRandomValue * fNewTime) + (0.5 * fAccX * fNewTimeSq);
        float dY = (initialDirection.y * fRandomValue * 1.5f * fNewTime) + (0.5 * fAccY * fNewTimeSq);
        float dZ = (initialDirection.z * fRandomValue * fNewTime) + (0.5 * fAccZ * fNewTimeSq);
    
        vNewPosition += float3(dX, dY, dZ);
    
        // 알파값 계산 -> 시간이 지날수록 투명
        // 1 ~ 0 으로 변하는 값을 이용하여 잔해가 타서 점점 검어지는것도 표현 가능할 듯
        fNewAlpha = fOneMinusTime; // 1 ~ 0
        cNewColor.r = input.color.r * fOneMinusTime;
        cNewColor.g = input.color.g * fOneMinusTime;
        cNewColor.b = input.color.b * fOneMinusTime;
        cNewColor.a = fNewAlpha;
    
        // 크기 계산 -> 시간이 지날수록 감소
        vNewSize = float2(input.initialSize.x * fOneMinusTime, input.initialSize.y * fOneMinusTime);
    }
    else
    {
        // lifeTime 이 지나면 저멀리 이상한곳으로 보냄
        vNewPosition += float3(9999999.f, 9999999.f, 9999999.f);
    }
    
    VS_PARTICLE_OUTPUT output;
    output.positionW = vNewPosition;
    output.color = cNewColor;
    output.size = vNewSize;
    return output;
}

[maxvertexcount(4)]
void GSExplosion(point VS_PARTICLE_OUTPUT input[1], inout TriangleStream<GS_PARTICLE_OUTPUT> outStream)
{
    float3 vUp = normalize(gmtxView._12_22_32);
    float3 vLook = gvCameraPosition.xyz - input[0].positionW;
    vLook = normalize(vLook);
    float3 vRight = cross(vUp, vLook);
    
    float4 vertices[4];
    float2 vSize = input[0].size;
    vertices[0] = float4(input[0].positionW + (vSize.x * vRight) + (vSize.x * vUp), 1.f);
    vertices[1] = float4(input[0].positionW - (vSize.x * vRight) + (vSize.x * vUp), 1.f);
    vertices[2] = float4(input[0].positionW + (vSize.x * vRight) - (vSize.x * vUp), 1.f);
    vertices[3] = float4(input[0].positionW - (vSize.x * vRight) - (vSize.x * vUp), 1.f);
    
    float2 uvs[4] = { float2(0.f, 0.f), float2(1.f, 0.f), float2(0.f, 1.f), float2(1.f, 1.f) };
    
    float4x4 mtxViewProjection = mul(gmtxView, gmtxProjection);
    
    // 출력
    GS_PARTICLE_OUTPUT output;
    
    output.position = mul(vertices[0], mtxViewProjection);
    output.color = input[0].color;
    output.uv = uvs[0];
    outStream.Append(output);
    
    output.position = mul(vertices[1], mtxViewProjection);
    output.color = input[0].color;
    output.uv = uvs[1];
    outStream.Append(output);
    
    output.position = mul(vertices[2], mtxViewProjection);
    output.color = input[0].color;
    output.uv = uvs[2];
    outStream.Append(output);
    
    output.position = mul(vertices[3], mtxViewProjection);
    output.color = input[0].color;
    output.uv = uvs[3];
    outStream.Append(output);
}

float4 PSExplosion(GS_PARTICLE_OUTPUT input) : SV_Target0
{
    return input.color;
}