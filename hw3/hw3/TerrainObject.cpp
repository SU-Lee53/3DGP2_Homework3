#include "stdafx.h"
#include "TerrainObject.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RawFormatImage

RawFormatImage::RawFormatImage(const std::string& strFileName, int nWidth, int nLength, bool bFlipY)
{
	m_nWidth = nWidth;
	m_nLength = nLength;

	std::ifstream in{ strFileName, std::ios::binary };
	if (!in)
		__debugbreak();

	std::vector<BYTE> pixelReads;

	while (in) {
		BYTE b;
		in.read((char*)&b, sizeof(BYTE));
		pixelReads.push_back(b);
	}


	if (bFlipY) {
		m_pRawImagePixels.resize(pixelReads.size());
		for (int z = 0; z < m_nLength; ++z) {
			for (int x = 0; x < m_nWidth; ++x) {
				m_pRawImagePixels[x + ((m_nLength - 1 - z) * m_nWidth)] = pixelReads[x + (z * m_nWidth)];
			}
		}
	}
	else {
		m_pRawImagePixels = pixelReads;
	}
}

RawFormatImage::~RawFormatImage(void)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HeightMapRawImage

HeightMapRawImage::HeightMapRawImage(const std::string& strFileName, int nWidth, int nLength, XMFLOAT3 xmf3Scale)
	: RawFormatImage(strFileName, nWidth, nLength, true)
{
	m_xmf3Scale = xmf3Scale;
}

HeightMapRawImage::~HeightMapRawImage()
{
}

float HeightMapRawImage::GetHeight(float fx, float fz, bool bReverseQuad)
{
	fx = fx / m_xmf3Scale.x;
	fz = fz / m_xmf3Scale.z;
	if ((fx < 0.0f) || (fz < 0.0f) || (fx >= m_nWidth) || (fz >= m_nLength)) return(0.0f);

	int x = (int)fx;
	int z = (int)fz;
	float fxPercent = fx - x;
	float fzPercent = fz - z;

	float fBottomLeft = (float)m_pRawImagePixels[x + (z * m_nWidth)];
	float fBottomRight = (float)m_pRawImagePixels[(x + 1) + (z * m_nWidth)];
	float fTopLeft = (float)m_pRawImagePixels[x + ((z + 1) * m_nWidth)];
	float fTopRight = (float)m_pRawImagePixels[(x + 1) + ((z + 1) * m_nWidth)];
	if (bReverseQuad)
	{
		if (fzPercent >= fxPercent)
			fBottomRight = fBottomLeft + (fTopRight - fTopLeft);
		else
			fTopLeft = fTopRight + (fBottomLeft - fBottomRight);
	}
	else
	{
		if (fzPercent < (1.0f - fxPercent))
			fTopRight = fTopLeft + (fBottomRight - fBottomLeft);
		else
			fBottomLeft = fTopLeft + (fBottomRight - fTopRight);
	}
	float fTopHeight = fTopLeft * (1 - fxPercent) + fTopRight * fxPercent;
	float fBottomHeight = fBottomLeft * (1 - fxPercent) + fBottomRight * fxPercent;
	float fHeight = fBottomHeight * (1 - fzPercent) + fTopHeight * fzPercent;

	return fHeight;
}

XMFLOAT3 HeightMapRawImage::GetHeightMapNormal(int x, int z)
{
	if ((x < 0.0f) || (z < 0.0f) || (x >= m_nWidth) || (z >= m_nLength)) {
		return XMFLOAT3(0.f, 1.f, 0.f);
	}

	int nHeightMapIndex = x + (z * m_nWidth);
	int xHeightMapAdd = (x < (m_nWidth - 1)) ? 1 : -1;
	int zHeightMapAdd = (z < (m_nLength - 1)) ? m_nWidth : -m_nWidth;

	float y1 = (float)m_pRawImagePixels[nHeightMapIndex] * m_xmf3Scale.y;
	float y2 = (float)m_pRawImagePixels[nHeightMapIndex + xHeightMapAdd] * m_xmf3Scale.y;
	float y3 = (float)m_pRawImagePixels[nHeightMapIndex + zHeightMapAdd] * m_xmf3Scale.y;

	XMFLOAT3 xmf3Edge1 = XMFLOAT3(0.f, y3 - y1, m_xmf3Scale.z);
	XMFLOAT3 xmf3Edge2 = XMFLOAT3(m_xmf3Scale.x, y2 - y1, 0.f);

	XMFLOAT3 xmf3Normal = Vector3::CrossProduct(xmf3Edge1, xmf3Edge2, true);

	return xmf3Normal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainObject

TerrainObject::TerrainObject()
{
}

TerrainObject::~TerrainObject()
{
}

void TerrainObject::Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::string& strFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color)
{
	m_nWidth = nWidth;
	m_nLength = nLength;
	m_xmf3Scale = xmf3Scale;
	m_pHeightMapImage = make_shared<HeightMapRawImage>(strFileName, nWidth, nLength, xmf3Scale);

	int cxQuadsPerBlock = nBlockWidth - 1;
	int czQuadsPerBlock = nBlockLength - 1;

	long cxBlocks = (m_nWidth - 1) / cxQuadsPerBlock;
	long czBlocks = (m_nLength - 1) / czQuadsPerBlock;

	m_pTerrainMeshes.resize(cxBlocks * czBlocks);

	std::shared_ptr<TerrainMesh> pHeightMapGridMesh;
	for (int z = 0, zStart = 0; z < czBlocks; ++z) {
		for (int x = 0, xStart = 0; x < cxBlocks; ++x) {
			xStart = x * (nBlockWidth - 1);
			zStart = z * (nBlockLength - 1);

			pHeightMapGridMesh = make_shared<TerrainMesh>();
			pHeightMapGridMesh->Create(pd3dDevice, pd3dCommandList,
				xStart, zStart, nBlockWidth, nBlockLength, xmf3Scale, xmf4Color, m_pHeightMapImage);
			m_pTerrainMeshes[x + (z * cxBlocks)] = pHeightMapGridMesh;
		}
	}

	XMStoreFloat4(&m_xmf4MapBoundaryPlanes[0], XMPlaneFromPointNormal(XMVectorSet(0.f, 0.f, 0.f, 1.f), XMVectorSet(1.f, 0.f, 0.f, 0.f)));
	XMStoreFloat4(&m_xmf4MapBoundaryPlanes[1], XMPlaneFromPointNormal(XMVectorSet((GetWidth() * 0.5f), 0.f, 0.f, 1.f), XMVectorSet(-1.f, 0.f, 0.f, 0.f)));

	XMStoreFloat4(&m_xmf4MapBoundaryPlanes[2], XMPlaneFromPointNormal(XMVectorSet(0.f, 0.f, 0.f, 1.f), XMVectorSet(0.f, 0.f, 1.f, 0.f)));
	XMStoreFloat4(&m_xmf4MapBoundaryPlanes[3], XMPlaneFromPointNormal(XMVectorSet(0.f, 0.f, (GetLength() * 0.5f), 1.f), XMVectorSet(0.f, 0.f, -1.f, 0.f)));

	m_xmOBB.Extents = XMFLOAT3{ (GetLength() * 0.5f), 500.f, (GetLength() * 0.5f) };
	XMStoreFloat4(&m_xmOBB.Orientation, XMQuaternionRotationRollPitchYaw(0.f, 0.f, 0.f));
	m_xmOBB.Center = XMFLOAT3{ GetWidth() * 0.5f, GetHeight(GetWidth() * 0.5, GetLength() * 0.5f), GetLength() * 0.5f };

	std::shared_ptr<Texture> pBaseTexture = TEXTURE->LoadTexture(pd3dCommandList, "TerrainBase", L"Terrain/Base_Texture.dds", RESOURCE_TYPE_TEXTURE2D);
	std::shared_ptr<Texture> pDetailedTexture = TEXTURE->LoadTexture(pd3dCommandList, "TerrainDetailed", L"Terrain/Detail_Texture_7.dds", RESOURCE_TYPE_TEXTURE2D);

	std::shared_ptr<Material> pTerrainMaterial = std::make_shared<Material>(pd3dDevice, pd3dCommandList);
	pTerrainMaterial->SetTexture(TEXTURE_INDEX_ALBEDO_MAP, pBaseTexture);
	pTerrainMaterial->SetTexture(TEXTURE_INDEX_DETAIL_ALBEDO_MAP, pDetailedTexture);

	std::shared_ptr<TerrainShader> pTerrainShader = std::make_shared<TerrainShader>();
	pTerrainShader->Create(pd3dDevice);
	pTerrainMaterial->SetShader(pTerrainShader);

	m_pMaterials.push_back(pTerrainMaterial);

	m_TerrainCBuffer.Create(pd3dDevice, pd3dCommandList, ConstantBufferSize<CB_TERRAIN_DATA>::value, true);
	m_TerrainOnMirrorCBuffer.Create(pd3dDevice, pd3dCommandList, ConstantBufferSize<CB_TERRAIN_DATA>::value, true);

	CreateChildWaterGridObject(pd3dDevice, pd3dCommandList, nWidth, nLength, nBlockWidth, nBlockLength, xmf3Scale, xmf4Color);
	m_pChildTerrain = static_pointer_cast<TerrainObject>(m_pChildren[0]);

	// Billboard 생성
	CreateBillboards(pd3dDevice, pd3dCommandList, "../Models/Textures/Terrain/ObjectsMap.raw");

	UpdateTransform();
}

void TerrainObject::CreateChildWaterGridObject(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color)
{
	std::shared_ptr<TerrainObject> pWaterGridObject = std::make_shared<TerrainObject>();

	int cxQuadsPerBlock = nBlockWidth - 1;
	int czQuadsPerBlock = nBlockLength - 1;

	long cxBlocks = (m_nWidth - 1) / cxQuadsPerBlock;
	long czBlocks = (m_nLength - 1) / czQuadsPerBlock;

	pWaterGridObject->m_pTerrainMeshes.resize(cxBlocks * czBlocks);

	std::shared_ptr<TerrainMesh> pHeightMapGridMesh;
	for (int z = 0, zStart = 0; z < czBlocks; ++z) {
		for (int x = 0, xStart = 0; x < cxBlocks; ++x) {
			xStart = x * (nBlockWidth - 1);
			zStart = z * (nBlockLength - 1);

			pHeightMapGridMesh = make_shared<TerrainMesh>();
			pHeightMapGridMesh->Create(pd3dDevice, pd3dCommandList,
				xStart, zStart, nBlockWidth, nBlockLength, m_xmf3Scale, xmf4Color, nullptr);
			pWaterGridObject->m_pTerrainMeshes[x + (z * cxBlocks)] = pHeightMapGridMesh;
		}
	}

	std::shared_ptr<Texture> pBaseTexture = TEXTURE->LoadTexture(pd3dCommandList, "WaterBase", L"Terrain/Water_Base_Texture_0.dds", RESOURCE_TYPE_TEXTURE2D);
	std::shared_ptr<Texture> pDetailedTexture = TEXTURE->LoadTexture(pd3dCommandList, "WaterDetailed", L"Terrain/Water_Detail_Texture_0.dds", RESOURCE_TYPE_TEXTURE2D);

	std::shared_ptr<Material> pWaterMaterial = std::make_shared<Material>(pd3dDevice, pd3dCommandList);
	pWaterMaterial->SetTexture(TEXTURE_INDEX_ALBEDO_MAP, pBaseTexture);
	pWaterMaterial->SetTexture(TEXTURE_INDEX_DETAIL_ALBEDO_MAP, pDetailedTexture);

	pWaterMaterial->SetShader(m_pMaterials[0]->GetShader());

	pWaterGridObject->m_pMaterials.push_back(pWaterMaterial);

	pWaterGridObject->SetName("WaterGridFrame");
	pWaterGridObject->m_TerrainCBuffer.Create(pd3dDevice, pd3dCommandList, ConstantBufferSize<CB_TERRAIN_DATA>::value, true);
	pWaterGridObject->m_TerrainOnMirrorCBuffer.Create(pd3dDevice, pd3dCommandList, ConstantBufferSize<CB_TERRAIN_DATA>::value, true);
	pWaterGridObject->m_fBlendFactor = 0.4f;

	XMStoreFloat4x4(&pWaterGridObject->m_xmf4x4Transform, XMMatrixTranslation(0.f, m_fWaterHeight, 0.f));

	SetChild(pWaterGridObject);
}

void TerrainObject::CreateBillboards(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::string& strFileName)
{
	m_BillboardCBuffer.Create(pd3dDevice, pd3dCommandList, ConstantBufferSize<CB_BILLBOARD_DATA>::value, true);
	m_BillboardOnMirrorCBuffer.Create(pd3dDevice, pd3dCommandList, ConstantBufferSize<CB_BILLBOARD_DATA>::value, true);
	std::shared_ptr<RawFormatImage> pObjectMapRaw = make_shared<RawFormatImage>(strFileName, m_nWidth, m_nLength, true);

	// 맵 전체에 100개를 뿌린다
	// 다만 물보다는 높게 있어야 함

	for (int i = 0; i < MAX_BILLBOARD_COUNT; ++i) {
		BillboardParameters billboardParams;
		float fPosX = 0.f;
		float fPosZ = 0.f;
		float fHeight = 0.f;
		while (true) {
			fPosX = (rand() % m_nWidth * m_xmf3Scale.x) - 1;
			fPosZ = (rand() % m_nLength * m_xmf3Scale.z) - 1;
			fHeight = GetHeight(fPosX, fPosZ);
			if (fHeight > m_fWaterHeight) break;
		}

		float nTextureIndex = rand() % 3;
		float fOffset = (nTextureIndex == 2) ? 40.f : 33.f;
		billboardParams.xmf3Position = XMFLOAT3(fPosX, fHeight + fOffset, fPosZ);
		billboardParams.xmf2Size = XMFLOAT2(20.f, fOffset * 2.f);
		billboardParams.nTextureIndex = nTextureIndex;
		m_Billboards.push_back(billboardParams);
	}

	m_pBillboardTextures[0] = TEXTURE->LoadTexture(pd3dCommandList, "Tree01", L"Terrain/Billboards/Tree01.dds", RESOURCE_TYPE_TEXTURE2D);
	m_pBillboardTextures[1] = TEXTURE->LoadTexture(pd3dCommandList, "Tree02", L"Terrain/Billboards/Tree02.dds", RESOURCE_TYPE_TEXTURE2D);
	m_pBillboardTextures[2] = TEXTURE->LoadTexture(pd3dCommandList, "Tree03", L"Terrain/Billboards/Tree03.dds", RESOURCE_TYPE_TEXTURE2D);
}

void TerrainObject::Update(float fTimeElapsed)
{
	m_pChildTerrain->m_xmf2UVTranslation.x += 0.1f * fTimeElapsed;
	m_pChildTerrain->m_xmf2UVTranslation.y += 0.1f * fTimeElapsed;
}

void TerrainObject::Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& refDescHandle)
{
	if (m_pMaterials[0]->GetShader()) {
		m_pMaterials[0]->GetShader()->OnPrepareRender(pd3dCommandList, 0);
	}

	UINT nBillboardsToDraw = 0;
	if ((m_pMaterials.size() == 1) && (m_pMaterials[0]))
	{
		// Terrain Data
		CB_TERRAIN_DATA terrainData;
		{
			XMStoreFloat4x4(&terrainData.xmf4x4TerrainWorld, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));
			terrainData.xmf2UVTranslation = m_xmf2UVTranslation;
		}
		m_TerrainCBuffer.UpdateData(&terrainData);

		pd3dDevice->CopyDescriptorsSimple(1, refDescHandle.cpuHandle, m_TerrainCBuffer.GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		refDescHandle.cpuHandle.ptr += 1 * GameFramework::g_uiDescriptorHandleIncrementSize;

		// Billboard Data & Texture
		nBillboardsToDraw = UpdateBillboardData();
		if (nBillboardsToDraw != 0) {
			pd3dDevice->CopyDescriptorsSimple(1, refDescHandle.cpuHandle, m_BillboardCBuffer.GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			refDescHandle.cpuHandle.ptr += 1 * GameFramework::g_uiDescriptorHandleIncrementSize;

			// Billboard Textures
			for (int i = 0; i < m_pBillboardTextures.size(); ++i) {
				pd3dDevice->CopyDescriptorsSimple(1, refDescHandle.cpuHandle, m_pBillboardTextures[i]->GetSRVCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				refDescHandle.cpuHandle.ptr += 1 * GameFramework::g_uiDescriptorHandleIncrementSize;
			}
		}
		else {
			// 그릴것이 없어도 Offset 은 일단 옮긴다
			refDescHandle.cpuHandle.ptr += (1 + m_pBillboardTextures.size()) * GameFramework::g_uiDescriptorHandleIncrementSize;
		}

		// Data set
		pd3dCommandList->SetGraphicsRootDescriptorTable(2, refDescHandle.gpuHandle);
		refDescHandle.gpuHandle.ptr += (2 + 3) * GameFramework::g_uiDescriptorHandleIncrementSize;
		// Terrain Data + BillboardData + Billboard 3개

		// Color + nType
		m_pMaterials[0]->UpdateShaderVariable(pd3dCommandList);
		pd3dDevice->CopyDescriptorsSimple(1, refDescHandle.cpuHandle,
			m_pMaterials[0]->GetCBuffer().GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		refDescHandle.cpuHandle.ptr += 2 * GameFramework::g_uiDescriptorHandleIncrementSize;
		// 원래 WorldMatrix 도 걸려야하지만 StructuredBuffer 로 넘기므로 descriptor 만 그냥 증가시킴 (2 * ...)

		// Textures
		m_pMaterials[0]->CopyTextureDescriptors(pd3dDevice, refDescHandle);

		// Descriptor Set
		pd3dCommandList->SetGraphicsRootDescriptorTable(4, refDescHandle.gpuHandle);
		refDescHandle.gpuHandle.ptr += (2 + Material::g_nTexturesPerMaterial) * GameFramework::g_uiDescriptorHandleIncrementSize;
		// 2 (CB_MATERIAL_DATA, World) + Texture 7개 

		float pfBlendFactor[4] = { m_fBlendFactor,m_fBlendFactor,m_fBlendFactor,m_fBlendFactor };
		pd3dCommandList->OMSetBlendFactor(pfBlendFactor);
	}

	if (m_pTerrainMeshes.size() >= 1)
	{
		for (int i = 0; i < m_pTerrainMeshes.size(); i++)
		{
			if (m_pTerrainMeshes[i]) m_pTerrainMeshes[i]->Render(pd3dCommandList, 0);
		}
	}

	// Draw Billboard
	if (m_pMaterials[0]->GetShader()) {
		m_pMaterials[0]->GetShader()->OnPrepareRender(pd3dCommandList, 1);
	}

	if (nBillboardsToDraw != 0) {
		pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		pd3dCommandList->DrawInstanced(nBillboardsToDraw, 1, 0, 0);
	}

	for (auto& pChild : m_pChildren) {
		static_pointer_cast<TerrainObject>(pChild)->Render(pd3dDevice, pd3dCommandList, refDescHandle);
	}

}

void TerrainObject::RenderOnMirror(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& refDescHandle, const XMFLOAT4& xmf4MirrorPlane, ComPtr<ID3D12PipelineState> pd3dTerrainOnMirrorPipelineState, ComPtr<ID3D12PipelineState> pd3dBillboardsOnMirrorPipelineState)
{
	XMMATRIX xmmtxReflect = XMMatrixReflect(XMLoadFloat4(&xmf4MirrorPlane));

	UINT nBillboardsToDraw = 0;
	if ((m_pMaterials.size() == 1) && (m_pMaterials[0]))
	{
		// Terrain Data
		CB_TERRAIN_DATA terrainData;
		{
			XMStoreFloat4x4(&terrainData.xmf4x4TerrainWorld, XMMatrixTranspose(XMMatrixMultiply(XMLoadFloat4x4(&m_xmf4x4World), xmmtxReflect)));
			terrainData.xmf2UVTranslation = m_xmf2UVTranslation;
		}
		m_TerrainOnMirrorCBuffer.UpdateData(&terrainData);

		pd3dDevice->CopyDescriptorsSimple(1, refDescHandle.cpuHandle, m_TerrainOnMirrorCBuffer.GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		refDescHandle.cpuHandle.ptr += 1 * GameFramework::g_uiDescriptorHandleIncrementSize;

		// Billboard Data & Texture
		// 이번에는 절두체 컬링을 수행하지 않고 거울에 대해서 수행함
		nBillboardsToDraw = UpdateBillboardDataInMirror(xmf4MirrorPlane);
		if (nBillboardsToDraw != 0) {
			pd3dDevice->CopyDescriptorsSimple(1, refDescHandle.cpuHandle, m_BillboardOnMirrorCBuffer.GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			refDescHandle.cpuHandle.ptr += 1 * GameFramework::g_uiDescriptorHandleIncrementSize;

			// Billboard Textures
			for (int i = 0; i < m_pBillboardTextures.size(); ++i) {
				pd3dDevice->CopyDescriptorsSimple(1, refDescHandle.cpuHandle, m_pBillboardTextures[i]->GetSRVCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				refDescHandle.cpuHandle.ptr += 1 * GameFramework::g_uiDescriptorHandleIncrementSize;
			}
		}
		else {
			// 그릴것이 없어도 Offset 은 일단 옮긴다
			refDescHandle.cpuHandle.ptr += (1 + m_pBillboardTextures.size()) * GameFramework::g_uiDescriptorHandleIncrementSize;
		}

		// Data set
		pd3dCommandList->SetGraphicsRootDescriptorTable(2, refDescHandle.gpuHandle);
		refDescHandle.gpuHandle.ptr += (2 + 3) * GameFramework::g_uiDescriptorHandleIncrementSize;
		// Terrain Data + BillboardData + Billboard 3개

		// Color + nType
		m_pMaterials[0]->UpdateShaderVariable(pd3dCommandList);
		pd3dDevice->CopyDescriptorsSimple(1, refDescHandle.cpuHandle,
			m_pMaterials[0]->GetCBuffer().GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		refDescHandle.cpuHandle.ptr += 2 * GameFramework::g_uiDescriptorHandleIncrementSize;
		// 원래 WorldMatrix 도 걸려야하지만 StructuredBuffer 로 넘기므로 descriptor 만 그냥 증가시킴 (2 * ...)

		// Textures
		m_pMaterials[0]->CopyTextureDescriptors(pd3dDevice, refDescHandle);

		// Descriptor Set
		pd3dCommandList->SetGraphicsRootDescriptorTable(4, refDescHandle.gpuHandle);
		refDescHandle.gpuHandle.ptr += (2 + Material::g_nTexturesPerMaterial) * GameFramework::g_uiDescriptorHandleIncrementSize;
		// 2 (CB_MATERIAL_DATA, World) + Texture 7개 
	}

	pd3dCommandList->SetPipelineState(pd3dTerrainOnMirrorPipelineState.Get());
	pd3dCommandList->OMSetStencilRef(255);

	if (m_pTerrainMeshes.size() >= 1)
	{
		for (int i = 0; i < m_pTerrainMeshes.size(); i++)
		{
			if (m_pTerrainMeshes[i]) m_pTerrainMeshes[i]->Render(pd3dCommandList, 0);
		}
	}

	// Draw Billboard
	pd3dCommandList->SetPipelineState(pd3dBillboardsOnMirrorPipelineState.Get());
	pd3dCommandList->OMSetStencilRef(255);

	if (nBillboardsToDraw != 0) {
		pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		pd3dCommandList->DrawInstanced(nBillboardsToDraw, 1, 0, 0);
	}

	for (auto& pChild : m_pChildren) {
		static_pointer_cast<TerrainObject>(pChild)->RenderOnMirror(pd3dDevice, pd3dCommandList, refDescHandle, xmf4MirrorPlane, pd3dTerrainOnMirrorPipelineState, pd3dBillboardsOnMirrorPipelineState);
	}

}

UINT TerrainObject::UpdateBillboardData()
{
	// 절두체 컬링 수행
	auto pCamera = CUR_SCENE->GetCamera();

	std::vector<BillboardParameters> billboardsInCamera;

	for (int i = 0; i < m_Billboards.size(); ++i) {
		if (pCamera->IsInFrustum(m_Billboards[i].xmf3Position)) {
			billboardsInCamera.push_back(m_Billboards[i]);
		}
	}

	if (billboardsInCamera.size() != 0) {
		CB_BILLBOARD_DATA billboardData;
		memcpy(billboardData.billboardData, billboardsInCamera.data(), billboardsInCamera.size() * sizeof(BillboardParameters));
		m_BillboardCBuffer.UpdateData(&billboardData);
	}

	return billboardsInCamera.size();
}

UINT TerrainObject::UpdateBillboardDataInMirror(const XMFLOAT4& xmf4MirrorPlane)
{
	XMMATRIX xmmtxReflect = XMMatrixReflect(XMLoadFloat4(&xmf4MirrorPlane));
	std::vector<BillboardParameters> billboardsInMirror;

	for (int i = 0; i < m_Billboards.size(); ++i) {
		XMVECTOR xmvInstancePosition = XMLoadFloat3(&m_Billboards[i].xmf3Position);
		float fDistance = XMVectorGetX(XMPlaneDotCoord(XMPlaneNormalize(XMLoadFloat4(&xmf4MirrorPlane)), xmvInstancePosition));
		if (fDistance > 0.f) {
			BillboardParameters billboardParam = m_Billboards[i];
			XMStoreFloat3(&billboardParam.xmf3Position, XMVector3TransformCoord(XMLoadFloat3(&billboardParam.xmf3Position), xmmtxReflect));
			billboardsInMirror.push_back(billboardParam);
		}
	}

	if (billboardsInMirror.size() != 0) {
		CB_BILLBOARD_DATA billboardData;
		memcpy(billboardData.billboardData, billboardsInMirror.data(), billboardsInMirror.size() * sizeof(BillboardParameters));
		m_BillboardOnMirrorCBuffer.UpdateData(&billboardData);
	}

	return billboardsInMirror.size();
}
float TerrainObject::GetHeight(float x, float z, bool bReverseQuad)
{ 
	// 범위 벗어나면 그냥 0 리턴
	if (x > GetWidth() || z > GetLength()) {
		return 0.f;
	}

	return m_pHeightMapImage->GetHeight(x, z, bReverseQuad) * m_xmf3Scale.y; 
} 