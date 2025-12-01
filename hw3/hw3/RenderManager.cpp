#include "stdafx.h"
#include "RenderManager.h"
#include "Material.h"
#include "StructuredBuffer.h"
#include "UIManager.h"
#include "TerrainObject.h"
#include "BuildingObject.h"	// MirrorObject

ComPtr<ID3D12RootSignature> RenderManager::g_pd3dRootSignature = nullptr;
bool RenderManager::g_bRenderOBBForDebug = false;

RenderManager::RenderManager(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	m_pd3dDevice = pd3dDevice;

	m_InstanceDataSBuffer.Create(pd3dDevice, pd3dCommandList, ASSUMED_REQUIRED_STRUCTURED_BUFFER_SIZE, sizeof(INSTANCE_DATA), true);
	m_InstanceDataOnMirrorSBuffer.Create(pd3dDevice, pd3dCommandList, ASSUMED_REQUIRED_STRUCTURED_BUFFER_SIZE, sizeof(INSTANCE_DATA), true);
	m_LightOnMirrorCBuffer.Create(pd3dDevice, pd3dCommandList, ConstantBufferSize<CB_LIGHT_DATA>::value, true);

	m_InstanceDataTransparentSBuffer.Create(pd3dDevice, pd3dCommandList, ASSUMED_REQUIRED_STRUCTURED_BUFFER_SIZE, sizeof(INSTANCE_DATA), true);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = 50000;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	m_pd3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_pd3dDescriptorHeap.GetAddressOf()));

	m_DescriptorHandle.cpuHandle = m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_DescriptorHandle.gpuHandle = m_pd3dDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	CreateGlobalRootSignature(pd3dDevice);
}

void RenderManager::Add(std::shared_ptr<GameObject> pGameObject)
{
	INSTANCE_KEY key{ pGameObject->GetMesh(), pGameObject->GetMaterials(), key.pMesh->GetSubSetCount() };
	XMFLOAT4X4 xmf4x4InstanceData;
	XMStoreFloat4x4(&xmf4x4InstanceData, XMMatrixTranspose(XMLoadFloat4x4(&pGameObject->GetWorldMatrix())));

	auto it = m_InstanceIndexMap.find(key);
	if (it == m_InstanceIndexMap.end()) {
		m_InstanceIndexMap[key] = m_nInstanceIndex++;
		m_InstanceDatas.push_back({ key, {} });
		m_nDrawCalls += key.pMaterials.size();

		m_InstanceDatas[m_InstanceIndexMap[key]].second.emplace_back(xmf4x4InstanceData);
	}
	else {
		m_InstanceDatas[it->second].second.emplace_back(xmf4x4InstanceData);
	}

}

void RenderManager::AddMirror(std::shared_ptr<MirrorObject> pMirrorObject)
{
	// TODO : Make it Happen
	m_pMirrorObjects.push_back(pMirrorObject);
}

void RenderManager::AddTransparent(std::shared_ptr<GameObject> pGameObject)
{
	m_pTransparentObjects.push_back(pGameObject);
}

void RenderManager::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	pd3dCommandList->SetGraphicsRootSignature(g_pd3dRootSignature.Get());
	pd3dCommandList->SetDescriptorHeaps(1, m_pd3dDescriptorHeap.GetAddressOf());
	CUR_SCENE->UpdateShaderVariable(pd3dCommandList);
	CUR_SCENE->GetLightCBuffer().SetBufferToPipeline(pd3dCommandList, 7);
	
	DescriptorHandle descHandle = m_DescriptorHandle;

	// Per Scene Descriptor 에 복사
	auto pCamera = CUR_SCENE->GetPlayer()->GetCamera();
	pCamera->UpdateShaderVariables(pd3dCommandList);
	pCamera->SetViewportsAndScissorRects(pd3dCommandList);

	m_pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle,
		pCamera->GetCBuffer().GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.cpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;

	pd3dCommandList->SetGraphicsRootDescriptorTable(0, descHandle.gpuHandle);
	descHandle.gpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;

	// Skybox 추가 필요
	if (CUR_SCENE->GetSkyboxTexture()) {
		m_pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, CUR_SCENE->GetSkyboxTexture()->GetSRVCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		descHandle.cpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;

		pd3dCommandList->SetGraphicsRootDescriptorTable(1, descHandle.gpuHandle);
		descHandle.gpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;
	}
	else {
		descHandle.cpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;
		descHandle.gpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;
	}


	RenderObjects(pd3dCommandList, descHandle);

	// TODO 추가
	// RenderTerrain
	if (m_pTerrain) {
		RenderTerrain(pd3dCommandList, descHandle);
	}

	RenderSkybox(pd3dCommandList, descHandle);

	RenderMirrors(pd3dCommandList, descHandle);

	// 위에서 조명을 거울 세계의 조명으로 바꾸었으므로 다시 원래대로 돌려야 함
	CUR_SCENE->GetLightCBuffer().SetBufferToPipeline(pd3dCommandList, 7);

	RenderTransparent(pd3dCommandList, descHandle);
}

void RenderManager::SetDescriptorHeapToPipeline(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) const
{
	pd3dCommandList->SetDescriptorHeaps(1, m_pd3dDescriptorHeap.GetAddressOf());
}

void RenderManager::RenderObjects(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& refDescHandle)
{
	UINT uiSBufferOffset = 0;
	for (auto&& [instanceKey, instanceData] : m_InstanceDatas) {
		m_InstanceDataSBuffer.UpdateData(instanceData, uiSBufferOffset);
		uiSBufferOffset += instanceData.size();
	}
#ifdef WITH_UPLOAD_BUFFER
	m_InstanceDataSBuffer.UpdateResources(m_pd3dDevice, pd3dCommandList);

#endif

	m_pd3dDevice->CopyDescriptorsSimple(1, refDescHandle.cpuHandle,
		m_InstanceDataSBuffer.GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	refDescHandle.cpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;

	pd3dCommandList->SetGraphicsRootDescriptorTable(3, refDescHandle.gpuHandle);
	refDescHandle.gpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;

	int nInstanceBase = 0;
	int nInstanceCount = 0;
	for (auto& [instanceKey, instanceData] : m_InstanceDatas) {
		nInstanceCount = instanceData.size();

		for (int i = 0; i < instanceKey.pMaterials.size(); ++i) {
			// Color + nType
			instanceKey.pMaterials[i]->UpdateShaderVariable(pd3dCommandList);
			m_pd3dDevice->CopyDescriptorsSimple(1, refDescHandle.cpuHandle,
				instanceKey.pMaterials[i]->GetCBuffer().GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			refDescHandle.cpuHandle.ptr += 2 * GameFramework::g_uiDescriptorHandleIncrementSize;
			// 원래 WorldMatrix 도 걸려야하지만 StructuredBuffer 로 넘기므로 descriptor 만 그냥 증가시킴 (2 * ...)

			// Textures
			instanceKey.pMaterials[i]->CopyTextureDescriptors(m_pd3dDevice, refDescHandle);

			// Descriptor Set
			pd3dCommandList->SetGraphicsRootDescriptorTable(4, refDescHandle.gpuHandle);
			refDescHandle.gpuHandle.ptr += (2 + Material::g_nTexturesPerMaterial) * GameFramework::g_uiDescriptorHandleIncrementSize;
			// 2 (CB_MATERIAL_DATA, World) + Texture 7개 

			pd3dCommandList->SetGraphicsRoot32BitConstant(5, nInstanceBase, 0);

			instanceKey.pMaterials[i]->OnPrepareRender(pd3dCommandList);
			instanceKey.pMesh->Render(pd3dCommandList, i, nInstanceCount);
		}

		nInstanceBase += nInstanceCount;
	}

}

void RenderManager::RenderObjectsInMirrorWorld(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& refDescHandle, const XMFLOAT4& xmf4MirrorPlane, ComPtr<ID3D12PipelineState> pd3dObjectsOnMirrorPipelineState, ComPtr<ID3D12PipelineState> pd3dTerrainOnMirrorPipelineState, ComPtr<ID3D12PipelineState> pd3dBillboardsOnMirrorPipelineState)
{
	// 일단 조명을 거울평면으로 반사시킨다 
	XMMATRIX xmmtxReflect = XMMatrixReflect(XMLoadFloat4(&xmf4MirrorPlane));

	CB_LIGHT_DATA lightData = CUR_SCENE->GetLightCBData();
	for (int i = 0; i < lightData.nLights; ++i) {
		XMVECTOR xmvLightPos = XMLoadFloat3(&lightData.LightData[i].xmf3Position);
		XMVECTOR xmvReflectedLightPos = XMVector3TransformCoord(xmvLightPos, xmmtxReflect);
		XMStoreFloat3(&lightData.LightData[i].xmf3Position, xmvReflectedLightPos);
	
		XMVECTOR xmvLightDir = XMLoadFloat3(&lightData.LightData[i].xmf3Direction);
		XMVECTOR xmvReflectedLightDir = XMVector3TransformNormal(xmvLightDir, xmmtxReflect);
		XMStoreFloat3(&lightData.LightData[i].xmf3Direction, xmvReflectedLightDir);
	}
	m_LightOnMirrorCBuffer.UpdateData(&lightData);
	m_LightOnMirrorCBuffer.SetBufferToPipeline(pd3dCommandList, 7);

	// Scene 에서 거울에 반사될 오브젝트들만 골라옴 (뒤에있으면 안그림)
	FilterObjectsInSceneBehindMirror(xmf4MirrorPlane);

	// 월드 행렬을 거울 기준으로 반사시킴
	UINT uiSBufferOffset = 0;
	for (auto&& [instanceKey, instanceData] : m_InstanceInMirrorDatas) {
		m_InstanceDataOnMirrorSBuffer.UpdateData(instanceData, uiSBufferOffset);
		uiSBufferOffset += instanceData.size();
	}
#ifdef WITH_UPLOAD_BUFFER
	m_InstanceInMirrorDatasBuffer.UpdateResources(m_pd3dDevice, pd3dCommandList);

#endif

	m_pd3dDevice->CopyDescriptorsSimple(1, refDescHandle.cpuHandle,
		m_InstanceDataOnMirrorSBuffer.GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	refDescHandle.cpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;

	pd3dCommandList->SetGraphicsRootDescriptorTable(3, refDescHandle.gpuHandle);
	refDescHandle.gpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;

	int nInstanceBase = 0;
	int nInstanceCount = 0;
	for (auto& [instanceKey, instanceData] : m_InstanceInMirrorDatas) {
		nInstanceCount = instanceData.size();
		if (nInstanceCount == 0) {
			continue;
		}

		for (int i = 0; i < instanceKey.pMaterials.size(); ++i) {
			// Color + nType
			instanceKey.pMaterials[i]->UpdateShaderVariable(pd3dCommandList);
			m_pd3dDevice->CopyDescriptorsSimple(1, refDescHandle.cpuHandle,
				instanceKey.pMaterials[i]->GetCBuffer().GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			refDescHandle.cpuHandle.ptr += 2 * GameFramework::g_uiDescriptorHandleIncrementSize;
			// 원래 WorldMatrix 도 걸려야하지만 StructuredBuffer 로 넘기므로 descriptor 만 그냥 증가시킴 (2 * ...)

			// Textures
			instanceKey.pMaterials[i]->CopyTextureDescriptors(m_pd3dDevice, refDescHandle);

			// Descriptor Set
			pd3dCommandList->SetGraphicsRootDescriptorTable(4, refDescHandle.gpuHandle);
			refDescHandle.gpuHandle.ptr += (2 + Material::g_nTexturesPerMaterial) * GameFramework::g_uiDescriptorHandleIncrementSize;
			// 2 (CB_MATERIAL_DATA, World) + Texture 7개 

			pd3dCommandList->SetGraphicsRoot32BitConstant(5, nInstanceBase, 0);

			pd3dCommandList->SetPipelineState(pd3dObjectsOnMirrorPipelineState.Get());
			pd3dCommandList->OMSetStencilRef(255);
			instanceKey.pMesh->Render(pd3dCommandList, i, nInstanceCount);
		}

		nInstanceBase += nInstanceCount;
	}

	// Terrain 도 반사해서 그림
	m_pTerrain->RenderOnMirror(m_pd3dDevice, pd3dCommandList, refDescHandle, xmf4MirrorPlane, pd3dTerrainOnMirrorPipelineState, pd3dBillboardsOnMirrorPipelineState);
}

void RenderManager::RenderTerrain(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& refDescHandle)
{
	m_pTerrain->Render(m_pd3dDevice, pd3dCommandList, refDescHandle);
}

void RenderManager::RenderMirrors(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& refDescHandle)
{
	for (auto& pMirror : m_pMirrorObjects) {
		pMirror->Render(m_pd3dDevice, pd3dCommandList, refDescHandle);
	}
}

void RenderManager::RenderTransparent(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& refDescHandle)
{
	if (m_pTransparentObjects.size() == 0) {
		return;
	}

	// 그리기 전에 카메라 거리 기준 먼것부터 정렬 필요 (거리 기준 내림차순)
	XMFLOAT3 xmf3CameraPosition = CUR_SCENE->GetCamera()->GetPosition();
	XMVECTOR xmvCameraPosition = XMLoadFloat3(&xmf3CameraPosition); // 11.11 TODO : 여기부터
	std::sort(m_pTransparentObjects.begin(), m_pTransparentObjects.end(), [&xmvCameraPosition](const std::shared_ptr<GameObject>& lhs, const std::shared_ptr<GameObject> rhs) {
		XMVECTOR xmvLhsPos = XMLoadFloat3(&lhs->GetPosition());
		XMVECTOR xmvRhsPos = XMLoadFloat3(&rhs->GetPosition());
	
		float fLhsDistanceSq = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(xmvCameraPosition, xmvLhsPos)));
		float fRhsDistanceSq = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(xmvCameraPosition, xmvRhsPos)));
	
		return fLhsDistanceSq > fRhsDistanceSq;
	});

	// 월드 행렬 담음
	UINT uiSBufferOffset = 0;
	for (auto& pObj : m_pTransparentObjects) {
		XMFLOAT4X4 xmf4x4World;
		XMStoreFloat4x4(&xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&pObj->GetWorldMatrix())));
		m_InstanceDataTransparentSBuffer.UpdateData(xmf4x4World, uiSBufferOffset);
		uiSBufferOffset += 1;
	}
#ifdef WITH_UPLOAD_BUFFER
	m_InstanceDataTransparentSBuffer.UpdateResources(m_pd3dDevice, pd3dCommandList);

#endif

	m_pd3dDevice->CopyDescriptorsSimple(1, refDescHandle.cpuHandle,
		m_InstanceDataTransparentSBuffer.GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	refDescHandle.cpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;

	pd3dCommandList->SetGraphicsRootDescriptorTable(3, refDescHandle.gpuHandle);
	refDescHandle.gpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;

	int nInstanceBase = 0;
	int nInstanceCount = 0;
	for (auto& pObj : m_pTransparentObjects) {
		nInstanceCount = 1;

		const auto& pMesh = pObj->GetMesh();
		const auto& pMaterials = pObj->GetMaterials();

		for (int i = 0; i < pMaterials.size(); ++i) {
			// Color + nType
			pMaterials[i]->UpdateShaderVariable(pd3dCommandList);
			m_pd3dDevice->CopyDescriptorsSimple(1, refDescHandle.cpuHandle,
				pMaterials[i]->GetCBuffer().GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			refDescHandle.cpuHandle.ptr += 2 * GameFramework::g_uiDescriptorHandleIncrementSize;
			// 원래 WorldMatrix 도 걸려야하지만 StructuredBuffer 로 넘기므로 descriptor 만 그냥 증가시킴 (2 * ...)

			// Textures
			pMaterials[i]->CopyTextureDescriptors(m_pd3dDevice, refDescHandle);

			// Descriptor Set
			pd3dCommandList->SetGraphicsRootDescriptorTable(4, refDescHandle.gpuHandle);
			refDescHandle.gpuHandle.ptr += (2 + Material::g_nTexturesPerMaterial) * GameFramework::g_uiDescriptorHandleIncrementSize;
			// 2 (CB_MATERIAL_DATA, World) + Texture 7개 

			pd3dCommandList->SetGraphicsRoot32BitConstant(5, nInstanceBase, 0);

			// pipeline[1] -> 블렌딩 사용
			pMaterials[i]->OnPrepareRender(pd3dCommandList, 1);
			
			// Blend factor 설정
			float fBlendFactor = 0.4f; // pMaterials[i]->m_fBlendFactor;
			float pfBlendFactor[4] = { fBlendFactor, fBlendFactor, fBlendFactor, fBlendFactor };
			pd3dCommandList->OMSetBlendFactor(pfBlendFactor);
			pMesh->Render(pd3dCommandList, i, nInstanceCount);
		}

		nInstanceBase += nInstanceCount;
	}
}

void RenderManager::RenderSkybox(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& refDescHandle)
{
	auto pSkyboxShader = SHADER->Get<SkyboxShader>();
	pSkyboxShader->OnPrepareRender(pd3dCommandList);
	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	pd3dCommandList->DrawInstanced(1, 1, 0, 0);
}

void RenderManager::CreateGlobalRootSignature(ComPtr<ID3D12Device> pd3dDevice)
{
	// 11.14
	// 계획 수정 -> 조명은 Descriptor 로 사용 (기존 DescriptorTable)


	CD3DX12_DESCRIPTOR_RANGE d3dDescriptorRanges[8];
	d3dDescriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, 0);	// b1 : Camera, Lights
	d3dDescriptorRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 0);	// t0 : Skybox

	d3dDescriptorRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2, 0, 0);	// b2 : Terrain world matrix
	d3dDescriptorRanges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 3, 0, 1);	// b3 : Billboard datas
	d3dDescriptorRanges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1, 0, 2);	// t1 ~ t3 : terrain billboards
	
	d3dDescriptorRanges[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 0, 0);	// t4 : World 행렬들을 전부 담을 StructuredBuffer

	d3dDescriptorRanges[6].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 4, 0, 0);	// b4 : Material, StructuredBuffer 에서 World 행렬의 위치(Base) + b5 World 행렬 (인스턴싱 안하는 경우(Mirror))
	d3dDescriptorRanges[7].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7, 5, 0, 2);	// t5 ~ t11 : 각각 albedo, specular, normal, metallic, emission, detail albedo, detail normal


	CD3DX12_ROOT_PARAMETER d3dRootParameters[8];
	{
		// Per Scene
		d3dRootParameters[0].InitAsDescriptorTable(1, &d3dDescriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);	// b1
		d3dRootParameters[1].InitAsDescriptorTable(1, &d3dDescriptorRanges[1], D3D12_SHADER_VISIBILITY_ALL);	// t0

		// Terrain
		d3dRootParameters[2].InitAsDescriptorTable(3, &d3dDescriptorRanges[2], D3D12_SHADER_VISIBILITY_ALL);	// b2, b3 / t1 ~ t3

		// Instance data
		d3dRootParameters[3].InitAsDescriptorTable(1, &d3dDescriptorRanges[5], D3D12_SHADER_VISIBILITY_ALL);	// t8

		// Material
		d3dRootParameters[4].InitAsDescriptorTable(2, &d3dDescriptorRanges[6], D3D12_SHADER_VISIBILITY_ALL);	// b4, b5 / t9 ~ t15

		// Instance Base
		d3dRootParameters[5].InitAsConstants(1, 6, 0, D3D12_SHADER_VISIBILITY_ALL);	// Center + Extent + Orientation + Color

		// 디버그용 OBB 그리기
		d3dRootParameters[6].InitAsConstants(16, 7, 0, D3D12_SHADER_VISIBILITY_ALL);	// Center + Extent + Orientation + Color

		// Lights
		d3dRootParameters[7].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);	// b0

	}

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_STATIC_SAMPLER_DESC d3dSamplerDescs[2];
	d3dSamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dSamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dSamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dSamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dSamplerDescs[0].MipLODBias = 0;
	d3dSamplerDescs[0].MaxAnisotropy = 1;
	d3dSamplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	d3dSamplerDescs[0].MinLOD = 0;
	d3dSamplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDescs[0].ShaderRegister = 0;
	d3dSamplerDescs[0].RegisterSpace = 0;
	d3dSamplerDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	d3dSamplerDescs[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dSamplerDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDescs[1].MipLODBias = 0;
	d3dSamplerDescs[1].MaxAnisotropy = 1;
	d3dSamplerDescs[1].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	d3dSamplerDescs[1].MinLOD = 0;
	d3dSamplerDescs[1].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDescs[1].ShaderRegister = 1;
	d3dSamplerDescs[1].RegisterSpace = 0;
	d3dSamplerDescs[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;


	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc{};
	{
		d3dRootSignatureDesc.NumParameters = _countof(d3dRootParameters);
		d3dRootSignatureDesc.pParameters = d3dRootParameters;
		d3dRootSignatureDesc.NumStaticSamplers = _countof(d3dSamplerDescs);
		d3dRootSignatureDesc.pStaticSamplers = d3dSamplerDescs;
		d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;
	}

	ComPtr<ID3DBlob> pd3dSignatureBlob = nullptr;
	ComPtr<ID3DBlob> pd3dErrorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pd3dSignatureBlob.GetAddressOf(), pd3dErrorBlob.GetAddressOf());
	if (FAILED(hr)) {
		char* pErrorString = (char*)pd3dErrorBlob->GetBufferPointer();
		HWND hWnd = ::GetActiveWindow();
		MessageBoxA(hWnd, pErrorString, NULL, 0);
		OutputDebugStringA(pErrorString);
		__debugbreak();
	}

	pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), IID_PPV_ARGS(g_pd3dRootSignature.GetAddressOf()));
}

void RenderManager::FilterObjectsInSceneBehindMirror(const XMFLOAT4& xmf4MirrorPlane)
{
	AddInMirrorSpace(CUR_SCENE->GetPlayer(), xmf4MirrorPlane);

	for (const auto& pObj : CUR_SCENE->GetGameObjects()) {
		AddInMirrorSpace(pObj, xmf4MirrorPlane);
	}
}

void RenderManager::AddInMirrorSpace(std::shared_ptr<GameObject> pGameObject, const XMFLOAT4& xmf4MirrorPlane)
{
	XMMATRIX xmmtxReflect = XMMatrixReflect(XMLoadFloat4(&xmf4MirrorPlane));

	XMFLOAT3 xmf3InstancePosition = pGameObject->GetPosition();
	XMVECTOR xmvInstancePosition = XMLoadFloat3(&xmf3InstancePosition);
	float fDistance = XMVectorGetX(XMPlaneDotCoord(XMPlaneNormalize(XMLoadFloat4(&xmf4MirrorPlane)), xmvInstancePosition));

	if (fDistance > 0.f && pGameObject->GetMesh()) {
		INSTANCE_KEY key{ pGameObject->GetMesh(), pGameObject->GetMaterials(), key.pMesh->GetSubSetCount() };
		XMFLOAT4X4 xmf4x4InstanceData;
		XMStoreFloat4x4(&xmf4x4InstanceData, XMMatrixTranspose(XMMatrixMultiply(XMLoadFloat4x4(&pGameObject->GetWorldMatrix()), xmmtxReflect)));

		auto it = m_InstanceInMirrorIndexMap.find(key);
		if (it == m_InstanceInMirrorIndexMap.end()) {
			m_InstanceInMirrorIndexMap[key] = m_nInstanceInMirrorIndex++;
			m_InstanceInMirrorDatas.push_back({ key, {} });
			m_nDrawCalls += key.pMaterials.size();

			m_InstanceInMirrorDatas[m_InstanceInMirrorIndexMap[key]].second.emplace_back(xmf4x4InstanceData);
		}
		else {
			m_InstanceInMirrorDatas[it->second].second.emplace_back(xmf4x4InstanceData);
		}
	}

	for (const auto& pChild : pGameObject->GetChildren()) {
		AddInMirrorSpace(pChild, xmf4MirrorPlane);
	}
}

void RenderManager::Clear()
{
	m_InstanceIndexMap.clear();
	m_InstanceDatas.clear();
	m_nInstanceIndex = 0;
	m_nDrawCalls = 0;

	m_InstanceInMirrorIndexMap.clear();
	m_InstanceInMirrorDatas.clear();
	m_nInstanceInMirrorIndex = 0;

	m_pMirrorObjects.clear(); 
	m_pTransparentObjects.clear();
}

size_t RenderManager::GetMeshCount() const
{
#ifdef INSTANCES_IN_HASHMAP
	return m_InstanceMap.size();

#else
	return m_InstanceIndexMap.size();

#endif

}
