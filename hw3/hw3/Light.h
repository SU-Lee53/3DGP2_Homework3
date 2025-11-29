#pragma once

struct LightData {
    XMFLOAT4 xmf4Ambient;                // c0
    XMFLOAT4 xmf4Diffuse;                // c1
    XMFLOAT4 xmf4Specular;               // c2
    XMFLOAT3 xmf3Position;               // c3.xyz
    float fFalloff;                   // c3.w
    XMFLOAT3 xmf3Direction;              // c4.xyz
    float fTheta; //cos(fTheta)     // c4.w
    XMFLOAT3 xmf3Attenuation;            // c5.xyz
    float fPhi; //cos(fPhi)         // c5.w
    BOOL bEnable;                     // c6.x
    int nType;                        // c6.y
    float fRange;                     // c6.z
    float padding;                      // c6.w
};

enum LIGHT_TYPE : int {
    LIGHT_TYPE_POINT_LIGHT = 1,
    LIGHT_TYPE_SPOT_LIGHT = 2,
    LIGHT_TYPE_DIRECTIONAL_LIGHT = 3,
};

class Light {
public:
    virtual LightData MakeLightData() { return LightData{}; }

public:
    bool m_bEnable;
};

class PointLight : public Light {
public:
    virtual LightData MakeLightData() override;

    XMFLOAT4    m_xmf4Diffuse;
    XMFLOAT4    m_xmf4Ambient;
    XMFLOAT4    m_xmf4Specular;

    XMFLOAT3    m_xmf3Position;
    XMFLOAT3    m_xmf3Direction;

    float       m_fRange;
    float       m_fAttenuation0;
    float       m_fAttenuation1;
    float       m_fAttenuation2;
    
};

class SpotLight : public Light {
public:
    virtual LightData MakeLightData() override;

    XMFLOAT4    m_xmf4Diffuse;
    XMFLOAT4    m_xmf4Ambient;
    XMFLOAT4    m_xmf4Specular;

    XMFLOAT3    m_xmf3Position;
    XMFLOAT3    m_xmf3Direction;

    float       m_fRange;
    float       m_fFalloff;
    float       m_fAttenuation0;
    float       m_fAttenuation1;
    float       m_fAttenuation2;
    float       m_fTheta;
    float       m_fPhi;
};

class DirectionalLight : public Light {
public:
    virtual LightData MakeLightData() override;

    XMFLOAT4    m_xmf4Diffuse;
    XMFLOAT4    m_xmf4Ambient;
    XMFLOAT4    m_xmf4Specular;

    XMFLOAT3    m_xmf3Direction;
};

