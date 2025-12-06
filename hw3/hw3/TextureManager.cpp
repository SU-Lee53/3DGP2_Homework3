#include "stdafx.h"
#include "TextureManager.h"

TextureManager::TextureManager(ComPtr<ID3D12Device> pd3dDevice)
{
	m_pd3dDevice = pd3dDevice;

	D3D12_DESCRIPTOR_HEAP_DESC d3dHeapDesc;
	{
		d3dHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		d3dHeapDesc.NumDescriptors = MAX_TEXTURE_PER_GAME;
		d3dHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		d3dHeapDesc.NodeMask = 0;
	}
	pd3dDevice->CreateDescriptorHeap(&d3dHeapDesc, IID_PPV_ARGS(m_pd3dDescriptorHeap.GetAddressOf()));
}

void TextureManager::LoadGameTextures(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	TEXTURE->LoadTexture(pd3dCommandList, "font", L"font.dds", RESOURCE_TYPE_TEXTURE2D);
	TEXTURE->LoadTexture(pd3dCommandList, "intro", L"intro.dds", RESOURCE_TYPE_TEXTURE2D);
	TEXTURE->LoadTexture(pd3dCommandList, "indicator", L"Indicator.dds", RESOURCE_TYPE_TEXTURE2D);
	TEXTURE->LoadTexture(pd3dCommandList, "window", L"Window.dds", RESOURCE_TYPE_TEXTURE2D);
	TEXTURE->LoadTexture(pd3dCommandList, "window_normal", L"Window_normal.dds", RESOURCE_TYPE_TEXTURE2D);

	TEXTURE->LoadTexture(pd3dCommandList, "Skybox", L"Skybox/Skybox.dds", RESOURCE_TYPE_TEXTURE2DARRAY);
}

std::shared_ptr<Texture> TextureManager::LoadTexture(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::string& strKey, const std::wstring& wstrTexturePath, UINT nResourceType)
{
    auto it = m_pTextureMap.find(strKey);
    if (it == m_pTextureMap.end()) {    // Map 에 텍스쳐가 없음
        std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();
		pTexture->LoadTextureFromFile(m_pd3dDevice, pd3dCommandList, wstrTexturePath, nResourceType);

		m_pTextureMap.insert({ strKey, pTexture });
		return pTexture;
    }

    return it->second;
}

std::shared_ptr<Texture> TextureManager::GetTexture(const std::string& strKey)
{
	auto it = m_pTextureMap.find(strKey);
	if (it != m_pTextureMap.end()) {
		return it->second;
	}

	return nullptr;
}

void TextureManager::ReleaseUploadBuffers()
{
	for (auto& [key, pTexture] : m_pTextureMap) {
		pTexture->ReleaseUploadBuffers();
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::CreateSRV(std::shared_ptr<Texture> pTexture)
{
	D3D12_CPU_DESCRIPTOR_HANDLE descHandle = m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	descHandle.ptr += ++nSRVCreated * GameFramework::g_uiDescriptorHandleIncrementSize;

	m_pd3dDevice->CreateShaderResourceView(pTexture->m_pd3dTextureResource.Get(), &pTexture->GetSRVDesc(), descHandle);

	// GPUHandle 은 0이 들어가야 정상
	// DescriptorHeap 이 D3D12_DESCRIPTOR_HEAP_FLAG_NONE 이기 때문
	return descHandle;
}
