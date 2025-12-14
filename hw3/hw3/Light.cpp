#include "stdafx.h"
#include "Light.h"
#include "Camera.h"

Light::Light(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	m_LightCameraCBuffer.Create(pd3dDevice, pd3dCommandList, ConstantBufferSize<CB_LIGHT_DATA>::value, true);
}

void Light::Update()
{
	// View 업데이트
	XMStoreFloat4x4(&m_xmf4x4ViewFromLight, XMMatrixLookToLH(XMLoadFloat3(&m_xmf3Position), XMVector3Normalize(XMLoadFloat3(&m_xmf3Direction)), XMVectorSet(0, 1, 0, 0)));
}

void Light::UpdateShaderVariables() const
{
	CB_CAMERA_DATA data{};
	{
		XMStoreFloat4x4(&data.m_xmf4x4View, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4ViewFromLight)));
		XMStoreFloat4x4(&data.m_xmf4x4Projection, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4ProjectionFromLight)));
		data.m_xmf3Position = m_xmf3Position;
	}

	m_LightCameraCBuffer.UpdateData(&data);
}

LightData PointLight::MakeLightData()
{
	LightData data{};
	{
		data.nType = LIGHT_TYPE_POINT_LIGHT;
		data.bEnable = TRUE;
		data.xmf4Diffuse = m_xmf4Diffuse;
		data.xmf4Ambient = m_xmf4Ambient;
		data.xmf4Specular = m_xmf4Specular;
		data.xmf3Position = m_xmf3Position;
		data.fRange = m_fRange;
		data.xmf3Attenuation.x = m_fAttenuation0;
		data.xmf3Attenuation.y = m_fAttenuation1;
		data.xmf3Attenuation.z = m_fAttenuation2;
	}

	return data;
}

LightData SpotLight::MakeLightData()
{
	XMMATRIX xmmtxViewFromLights = XMLoadFloat4x4(&m_xmf4x4ViewFromLight);
	XMMATRIX xmmtxProjectionFromLights = XMLoadFloat4x4(&m_xmf4x4ProjectionFromLight);
	XMMATRIX xmmtxToTexture = XMMatrixTranspose(XMMatrixMultiply(XMMatrixMultiply(xmmtxViewFromLights, xmmtxProjectionFromLights), XMLoadFloat4x4(&g_xmf4x4ToTexture)));

	LightData data{};
	{
		data.nType = LIGHT_TYPE_SPOT_LIGHT;
		data.bEnable = TRUE;
		data.xmf4Diffuse = m_xmf4Diffuse;
		data.xmf4Ambient = m_xmf4Ambient;
		data.xmf4Specular = m_xmf4Specular;
		data.xmf3Position = m_xmf3Position;
		data.xmf3Direction = m_xmf3Direction;
		data.fRange = m_fRange;
		data.fFalloff = m_fFalloff;
		data.xmf3Attenuation.x = m_fAttenuation0;
		data.xmf3Attenuation.y = m_fAttenuation1;
		data.xmf3Attenuation.z = m_fAttenuation2;
		data.fTheta = m_fTheta;
		data.fPhi = m_fPhi;

		XMStoreFloat4x4(&data.xmf4x4ToTextures, xmmtxToTexture);
	}

	return data;
}

void SpotLight::SetViewportsAndScissorRects(ComPtr<ID3D12GraphicsCommandList> pd3dCommmandList)
{
	D3D12_VIEWPORT d3dViewport{ 0, 0, (float)SPOT_SHADOW_MAP_SIZE, (float)SPOT_SHADOW_MAP_SIZE, 0.0f, 1.0f};
	D3D12_RECT d3dScissorRect{ 0, 0, SPOT_SHADOW_MAP_SIZE, SPOT_SHADOW_MAP_SIZE };

	pd3dCommmandList->RSSetViewports(1, &d3dViewport);
	pd3dCommmandList->RSSetScissorRects(1, &d3dScissorRect);
}

LightData DirectionalLight::MakeLightData()
{
	XMMATRIX xmmtxViewFromLights = XMLoadFloat4x4(&m_xmf4x4ViewFromLight);
	XMMATRIX xmmtxProjectionFromLights = XMLoadFloat4x4(&m_xmf4x4ProjectionFromLight);
	XMMATRIX xmmtxToTexture = XMMatrixTranspose(XMMatrixMultiply(XMMatrixMultiply(xmmtxViewFromLights, xmmtxProjectionFromLights), XMLoadFloat4x4(&g_xmf4x4ToTexture)));

	LightData data{};
	{
		data.nType = LIGHT_TYPE_DIRECTIONAL_LIGHT;
		data.bEnable = TRUE;
		data.xmf4Diffuse = m_xmf4Diffuse;
		data.xmf4Ambient = m_xmf4Ambient;
		data.xmf4Specular = m_xmf4Specular;
		data.xmf3Direction = m_xmf3Direction;

		XMStoreFloat4x4(&data.xmf4x4ToTextures, xmmtxToTexture);
	}

	return data;
}

void DirectionalLight::SetViewportsAndScissorRects(ComPtr<ID3D12GraphicsCommandList> pd3dCommmandList)
{
	D3D12_VIEWPORT d3dViewport{ 0, 0, (float)DIRECTIONAL_SHADOW_MAP_SIZE, (float)DIRECTIONAL_SHADOW_MAP_SIZE, 0.0f, 1.0f };
	D3D12_RECT d3dScissorRect{ 0, 0, DIRECTIONAL_SHADOW_MAP_SIZE, DIRECTIONAL_SHADOW_MAP_SIZE };

	pd3dCommmandList->RSSetViewports(1, &d3dViewport);
	pd3dCommmandList->RSSetScissorRects(1, &d3dScissorRect);
}
