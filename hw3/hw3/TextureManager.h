#pragma once
#include "Texture.h"

#define MAX_TEXTURE_PER_GAME 100;

class TextureManager {
public:
	TextureManager(ComPtr<ID3D12Device> pd3dDevice);

	void LoadGameTextures(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

	std::shared_ptr<Texture> LoadTexture(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::string& strKey, const std::wstring& wstrTexturePath, UINT nResourceType);
	std::shared_ptr<Texture> GetTexture(const std::string& strKey);
	
	void ReleaseUploadBuffers();

public:
	D3D12_CPU_DESCRIPTOR_HANDLE CreateSRV(std::shared_ptr<Texture> pTexture);
	D3D12_CPU_DESCRIPTOR_HANDLE CreateUAV(std::shared_ptr<Texture> pTexture);


private:
	ComPtr<ID3D12Device> m_pd3dDevice;	// GameFramework::m_pd3dDeivce
	std::unordered_map<std::string, std::shared_ptr<Texture>> m_pTextureMap;

	ComPtr<ID3D12DescriptorHeap>				m_pd3dDescriptorHeap;
	UINT nSRVUAVCreated = 0;

};

