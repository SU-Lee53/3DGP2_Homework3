#pragma once
#include "Shader.h"

class Texture;

enum TEXTURE_INDEX {
	TEXTURE_INDEX_ALBEDO_MAP = 0,
	TEXTURE_INDEX_SPECULAR_MAP = 1,
	TEXTURE_INDEX_NORMAL_MAP = 2,
	TEXTURE_INDEX_METALLIC_MAP = 3,
	TEXTURE_INDEX_EMISSION_MAP = 4,
	TEXTURE_INDEX_DETAIL_ALBEDO_MAP = 5,
	TEXTURE_INDEX_DETAIL_NORMAL_MAP = 6,

	TEXTURE_INDEX_DIFFUSE_MAP = 0,
};

enum MATERIAL_TYPE {
	MATERIAL_TYPE_ALBEDO_MAP = 0x01,
	MATERIAL_TYPE_SPECULAR_MAP = 0x02,
	MATERIAL_TYPE_NORMAL_MAP = 0x04,
	MATERIAL_TYPE_METALLIC_MAP = 0x08,
	MATERIAL_TYPE_EMISSION_MAP = 0x10,
	MATERIAL_TYPE_DETAIL_ALBEDO_MAP = 0x20,
	MATERIAL_TYPE_DETAIL_NORMAL_MAP = 0x40
};

struct CB_MATERIAL_DATA {
	XMFLOAT4 xmf4Ambient;
	XMFLOAT4 xmf4Diffuse;
	XMFLOAT4 xmf4Specular;
	XMFLOAT4 xmf4Emissive;
	UINT nMaterialType;
};

class Material {
public:
	Material() = default;
	Material(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

public:
	void Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	
	void SetShader(std::shared_ptr<Shader> pShader) { m_pShader = pShader; }
	const std::shared_ptr<Shader>& GetShader() const { return m_pShader; }

	void UpdateShaderVariable(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	void CopyTextureDescriptors(ComPtr<ID3D12Device> pd3dCommandList, DescriptorHandle& descHandle);
	void SetMaterialToPipeline(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT uiRootParameterIndex);

	void OnPrepareRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, int nPipelineIndex = 0);

	const ConstantBuffer& GetCBuffer() const { return m_MaterialCBuffer; }

	void SetTexture(UINT nTextureIndex, std::shared_ptr<Texture> pTexture);

	void SetMaterialType(UINT nType) { m_nType |= nType; }

	void SetTransparent(bool bTransparent) { m_bTransparent = bTransparent; }
	void SetBlendFactor(float fBlendFactor) { m_fBlendFactor = fBlendFactor; }

public:
	XMFLOAT4	m_xmf4AlbedoColor	= XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	XMFLOAT4	m_xmf4EmissiveColor	= XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4	m_xmf4SpecularColor	= XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4	m_xmf4AmbientColor	= XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	float		m_fGlossiness = 0.0f;
	float		m_fSmoothness = 0.0f;
	float		m_fSpecularHighlight = 0.0f;
	float		m_fMetallic = 0.0f;
	float		m_fGlossyReflection = 0.0f;

	UINT		m_nType = 0x00;

	bool		m_bTransparent = false;
	float		m_fBlendFactor = 0.f;


private:
	std::shared_ptr<Shader> m_pShader;

	ConstantBuffer m_MaterialCBuffer;

	std::array<std::shared_ptr<Texture>, 7> m_pTextures;

private:
	static std::shared_ptr<Shader> m_pIlluminatedShader;

public:
	static void PrepareShaders(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature);

	const static UINT g_nTexturesPerMaterial = 7;
};

