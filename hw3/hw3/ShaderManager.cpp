#include "stdafx.h"
#include "ShaderManager.h"

ShaderManager::ShaderManager(ComPtr<ID3D12Device> pDevice)
{
	m_pd3dDevice = pDevice;
}

ShaderManager::~ShaderManager()
{
}

void ShaderManager::Initialize()
{
	ComPtr<ID3DBlob> m_pd3dVSBlob;
	ComPtr<ID3DBlob> m_pd3dGSBlob;
	ComPtr<ID3DBlob> m_pd3dPSBlob;

	// Shaders.hlsl
	m_pCompiledShaderByteCodeMap.insert( {"StandardVS", Shader::CompileShader(L"../HLSL/Shaders.hlsl", "VSStandard", "vs_5_1", m_pd3dVSBlob.GetAddressOf())} );
	m_pCompiledShaderByteCodeMap.insert( {"StandardPS", Shader::CompileShader(L"../HLSL/Shaders.hlsl", "PSStandard", "ps_5_1", m_pd3dPSBlob.GetAddressOf())} );
	m_pd3dBlobs.push_back(m_pd3dVSBlob);
	m_pd3dBlobs.push_back(m_pd3dPSBlob);

	m_pCompiledShaderByteCodeMap.insert( { "MirrorVS", Shader::CompileShader(L"../HLSL/Shaders.hlsl", "VSMirror", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) } );
	m_pCompiledShaderByteCodeMap.insert( { "MirrorPS", Shader::CompileShader(L"../HLSL/Shaders.hlsl", "PSMirror", "ps_5_1", m_pd3dPSBlob.GetAddressOf()) } );
	m_pd3dBlobs.push_back(m_pd3dVSBlob);   
	m_pd3dBlobs.push_back(m_pd3dPSBlob);   
										   
	m_pCompiledShaderByteCodeMap.insert( { "TerrainVS", Shader::CompileShader(L"../HLSL/Shaders.hlsl", "VSTerrain", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) } );
	m_pCompiledShaderByteCodeMap.insert( { "TerrainPS", Shader::CompileShader(L"../HLSL/Shaders.hlsl", "PSTerrain", "ps_5_1", m_pd3dPSBlob.GetAddressOf()) } );
	m_pd3dBlobs.push_back(m_pd3dVSBlob);   
	m_pd3dBlobs.push_back(m_pd3dGSBlob);   
	m_pd3dBlobs.push_back(m_pd3dPSBlob);   
										   
	m_pCompiledShaderByteCodeMap.insert( { "BillboardVS", Shader::CompileShader(L"../HLSL/Shaders.hlsl", "VSBillboard", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) } );
	m_pCompiledShaderByteCodeMap.insert( { "BillboardGS", Shader::CompileShader(L"../HLSL/Shaders.hlsl", "GSBillboard", "gs_5_1", m_pd3dGSBlob.GetAddressOf()) } );
	m_pCompiledShaderByteCodeMap.insert( { "BillboardPS", Shader::CompileShader(L"../HLSL/Shaders.hlsl", "PSBillboard", "ps_5_1", m_pd3dPSBlob.GetAddressOf()) } );
	m_pd3dBlobs.push_back(m_pd3dVSBlob);   
	m_pd3dBlobs.push_back(m_pd3dGSBlob);   
	m_pd3dBlobs.push_back(m_pd3dPSBlob);   
										   
	m_pCompiledShaderByteCodeMap.insert( { "SkyboxVS", Shader::CompileShader(L"../HLSL/Shaders.hlsl", "VSSkybox", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert( { "SkyboxGS", Shader::CompileShader(L"../HLSL/Shaders.hlsl", "GSSkybox", "gs_5_1", m_pd3dGSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert( { "SkyboxPS", Shader::CompileShader(L"../HLSL/Shaders.hlsl", "PSSkybox", "ps_5_1", m_pd3dPSBlob.GetAddressOf()) });
	m_pd3dBlobs.push_back(m_pd3dVSBlob);   
	m_pd3dBlobs.push_back(m_pd3dGSBlob);   
	m_pd3dBlobs.push_back(m_pd3dPSBlob);   
										   
	m_pCompiledShaderByteCodeMap.insert( { "DebugVS", Shader::CompileShader(L"../HLSL/Shaders.hlsl", "VSDebug", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) } );
	m_pCompiledShaderByteCodeMap.insert( { "DebugGS", Shader::CompileShader(L"../HLSL/Shaders.hlsl", "GSDebug", "gs_5_1", m_pd3dGSBlob.GetAddressOf()) } );
	m_pCompiledShaderByteCodeMap.insert( { "DebugPS", Shader::CompileShader(L"../HLSL/Shaders.hlsl", "PSDebug", "ps_5_1", m_pd3dPSBlob.GetAddressOf()) } );
	m_pd3dBlobs.push_back(m_pd3dVSBlob);   
	m_pd3dBlobs.push_back(m_pd3dGSBlob);   
	m_pd3dBlobs.push_back(m_pd3dPSBlob);   
										   
	// Sprite.hlsl						   
	m_pCompiledShaderByteCodeMap.insert( { "TextureSpriteVS", Shader::CompileShader(L"../HLSL/Sprite.hlsl", "VSTextureSprite", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) } );
	m_pCompiledShaderByteCodeMap.insert( { "TextureSpriteGS", Shader::CompileShader(L"../HLSL/Sprite.hlsl", "GSTextureSprite", "gs_5_1", m_pd3dGSBlob.GetAddressOf()) } );
	m_pCompiledShaderByteCodeMap.insert( { "TextureSpritePS", Shader::CompileShader(L"../HLSL/Sprite.hlsl", "PSTextureSprite", "ps_5_1", m_pd3dPSBlob.GetAddressOf()) } );
	m_pd3dBlobs.push_back(m_pd3dVSBlob);   
	m_pd3dBlobs.push_back(m_pd3dGSBlob);   
	m_pd3dBlobs.push_back(m_pd3dPSBlob);   
										   
	m_pCompiledShaderByteCodeMap.insert( { "TextSpriteVS", Shader::CompileShader(L"../HLSL/Sprite.hlsl", "VSTextSprite", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) } );
	m_pCompiledShaderByteCodeMap.insert( { "TextSpriteGS", Shader::CompileShader(L"../HLSL/Sprite.hlsl", "GSTextSprite", "gs_5_1", m_pd3dGSBlob.GetAddressOf()) } );
	m_pCompiledShaderByteCodeMap.insert( { "TextSpritePS", Shader::CompileShader(L"../HLSL/Sprite.hlsl", "PSTextSprite", "ps_5_1", m_pd3dPSBlob.GetAddressOf()) } );
	m_pd3dBlobs.push_back(m_pd3dVSBlob);
	m_pd3dBlobs.push_back(m_pd3dGSBlob);
	m_pd3dBlobs.push_back(m_pd3dPSBlob);
	
	m_pCompiledShaderByteCodeMap.insert( { "BillboardSpriteVS", Shader::CompileShader(L"../HLSL/Sprite.hlsl", "VSBillboardSprite", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) } );
	m_pCompiledShaderByteCodeMap.insert( { "BillboardSpriteGS", Shader::CompileShader(L"../HLSL/Sprite.hlsl", "GSBillboardSprite", "gs_5_1", m_pd3dGSBlob.GetAddressOf()) } );
	m_pCompiledShaderByteCodeMap.insert( { "BillboardSpritePS", Shader::CompileShader(L"../HLSL/Sprite.hlsl", "PSBillboardSprite", "ps_5_1", m_pd3dPSBlob.GetAddressOf()) } );
	m_pd3dBlobs.push_back(m_pd3dVSBlob);
	m_pd3dBlobs.push_back(m_pd3dGSBlob);
	m_pd3dBlobs.push_back(m_pd3dPSBlob);

	// Effect.hlsl
	m_pCompiledShaderByteCodeMap.insert( { "ExplosionVS", Shader::CompileShader(L"../HLSL/Effect.hlsl", "VSExplosion", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) } );
	m_pCompiledShaderByteCodeMap.insert( { "ExplosionGS", Shader::CompileShader(L"../HLSL/Effect.hlsl", "GSExplosion", "gs_5_1", m_pd3dGSBlob.GetAddressOf()) } );
	m_pCompiledShaderByteCodeMap.insert( { "ExplosionPS", Shader::CompileShader(L"../HLSL/Effect.hlsl", "PSExplosion", "ps_5_1", m_pd3dPSBlob.GetAddressOf()) } );
	m_pd3dBlobs.push_back(m_pd3dVSBlob);
	m_pd3dBlobs.push_back(m_pd3dGSBlob);
	m_pd3dBlobs.push_back(m_pd3dPSBlob);

	Load<StandardShader>();
	Load<TerrainShader>();
	Load<OBBDebugShader>();
	Load<MirrorShader>();
	Load<SkyboxShader>();
}

D3D12_SHADER_BYTECODE ShaderManager::GetShaderByteCode(const std::string& strShaderName)
{
	auto it = m_pCompiledShaderByteCodeMap.find(strShaderName);
	if (it != m_pCompiledShaderByteCodeMap.end()) {
		return it->second;
	}

	return D3D12_SHADER_BYTECODE{};
}

void ShaderManager::ReleaseBlobs()
{
	for (auto& pBlob : m_pd3dBlobs) {
		pBlob.Reset();
		pBlob = nullptr;
	}

	m_pd3dBlobs.clear();
}
