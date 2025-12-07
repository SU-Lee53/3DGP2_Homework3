#pragma once

enum RESOURCE_TYPE : UINT {
	RESOURCE_TYPE_TEXTURE2D			= 0x01,
	RESOURCE_TYPE_TEXTURE2D_ARRAY	= 0x02,
	RESOURCE_TYPE_TEXTURE2DARRAY	= 0x03,
	RESOURCE_TYPE_TEXTURE_CUBE		= 0x04,
	RESOURCE_TYPE_BUFFER			= 0x05,

	RESOURCE_TYPE_RWTEXTURE2D		= 0x06
};

class Texture : public std::enable_shared_from_this<Texture> {
public:
	Texture();
	virtual ~Texture();

public:
	void LoadTextureFromFile(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::wstring& wstrTextureName, UINT nResourceType);
	void ReleaseUploadBuffers();

private:
	void LoadTextureFromDDSFile(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		const std::wstring& wstrPath, std::unique_ptr<uint8_t[]>& ddsData, std::vector<D3D12_SUBRESOURCE_DATA>& subResources);
	void LoadTextureFromWICFile(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		const std::wstring& wstrPath, std::unique_ptr<uint8_t[]>& ddsData, std::vector<D3D12_SUBRESOURCE_DATA>& subResources);

	void CreateUAVTexture(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nWidth, UINT nHeight);

public:
	ComPtr<ID3D12Resource> GetTexResource() const { return m_pd3dTextureResource; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle() const { return m_SRVCPUDescriptorHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAVCPUHandle() const { return m_UAVCPUDescriptorHandle; }

public:
	D3D12_SHADER_RESOURCE_VIEW_DESC GetSRVDesc() const;
	D3D12_RESOURCE_DESC GetResourceDesc() const { return m_pd3dTextureResource->GetDesc(); }

	bool IsTransparent() { return m_bTransparent; }

private:
	UINT									m_nTextureType;

	std::wstring							m_wstrTextureName;
	ComPtr<ID3D12Resource>					m_pd3dTextureResource;
	ComPtr<ID3D12Resource>					m_pd3dUploadBuffer;

	UINT									m_nResourceTypes;

	D3D12_CPU_DESCRIPTOR_HANDLE				m_SRVCPUDescriptorHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE				m_UAVCPUDescriptorHandle;

	bool									m_bTransparent = false;

public:
	std::wstring g_wstrTexturePathBase = L"../Models/Textures/";
	friend class TextureManager;

};

