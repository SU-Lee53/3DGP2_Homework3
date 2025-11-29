#include "stdafx.h"
#include "BuildingObject.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MirrorObject 

MirrorObject::MirrorObject()
{
}

MirrorObject::~MirrorObject()
{
}

void MirrorObject::Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	m_WorldCBuffer.Create(pd3dDevice, pd3dCommandList, ConstantBufferSize<CB_OBJECT_DATA>::value, true);
}

void MirrorObject::UpdateTransform(XMFLOAT4X4* pxmf4x4Parent)
{
	m_xmf4x4World = (pxmf4x4Parent) ? Matrix4x4::Multiply(m_xmf4x4Transform, *pxmf4x4Parent) : m_xmf4x4Transform;

	if (m_pMesh) {
		m_pMesh->UpdateOBB(m_xmf4x4World);
	}

	// 거울 평변 업데이트
	XMStoreFloat4(&m_xmf4MirrorPlane, XMPlaneFromPointNormal(XMLoadFloat3(&GetPosition()), XMLoadFloat3(&GetLook()) * -1));

	for (auto& pChild : m_pChildren) {
		pChild->UpdateTransform(&m_xmf4x4World);
	}
}

void MirrorObject::AddToRenderMap(bool bTransparent)
{
	RENDER->AddMirror(static_pointer_cast<MirrorObject>(shared_from_this()));
}

void MirrorObject::Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& refDescHandle)
{
	// 1. 실세계 객체를 그린다 -> RenderManager 에서 이미 그려서 넘어오므로 PASS
	// 2. 거울 뒷면을 그린다
	{
		m_pMaterials[0]->UpdateShaderVariable(pd3dCommandList);
		pd3dDevice->CopyDescriptorsSimple(1, refDescHandle.cpuHandle,
			m_pMaterials[0]->GetCBuffer().GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		refDescHandle.cpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;

		CB_OBJECT_DATA objectData;
		{
			XMStoreFloat4x4(&objectData.xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));
		}
		m_WorldCBuffer.UpdateData(&objectData);
		pd3dDevice->CopyDescriptorsSimple(1, refDescHandle.cpuHandle,
			m_WorldCBuffer.GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		refDescHandle.cpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;

		// Textures
		m_pMaterials[0]->CopyTextureDescriptors(pd3dDevice, refDescHandle);

		// Descriptor Set
		pd3dCommandList->SetGraphicsRootDescriptorTable(4, refDescHandle.gpuHandle);
		refDescHandle.gpuHandle.ptr += (2 + Material::g_nTexturesPerMaterial) * GameFramework::g_uiDescriptorHandleIncrementSize;
		// 2 (CB_MATERIAL_DATA, World) + Texture 7개 

		pd3dCommandList->SetGraphicsRoot32BitConstant(5, -1, 0);

		// pipeline[6] : 거울 뒷면을 그림
		m_pMaterials[0]->OnPrepareRender(pd3dCommandList, 6);
		m_pMesh->Render(pd3dCommandList, 0, 1);
	}
	
	// 3. 거울을 스텐실 버퍼에 그린다
	{
		// pipeline[0] : 거울을 스텐실 버퍼에 그림
		m_pMaterials[0]->OnPrepareRender(pd3dCommandList, 0);
		pd3dCommandList->OMSetStencilRef(255);
		m_pMesh->Render(pd3dCommandList, 0, 1);
	}
	
	// 4. 거울 부분만 Depth 를 1로 초기화한다
	{
		// pipeline[1] : 거울만 Depth를 초기화
		m_pMaterials[0]->OnPrepareRender(pd3dCommandList, 1);
		pd3dCommandList->OMSetStencilRef(255);
		m_pMesh->Render(pd3dCommandList, 0, 1);
	}

	// 5. 거울에 반사된 객체들을 그린다.
	{
		// pipeline[2], [3], [4] : 거울 뒷면 객체 / Terrain / Billboard 를 그림
		RENDER->RenderObjectsInMirrorWorld(pd3dCommandList, refDescHandle, m_xmf4MirrorPlane, m_pMaterials[0]->GetShader()->GetPipelineState(2), m_pMaterials[0]->GetShader()->GetPipelineState(3), m_pMaterials[0]->GetShader()->GetPipelineState(4));
	}

	// 6. 거울을 블렌딩해서 그린다
	{
		m_pMaterials[0]->UpdateShaderVariable(pd3dCommandList);
		pd3dDevice->CopyDescriptorsSimple(1, refDescHandle.cpuHandle,
			m_pMaterials[0]->GetCBuffer().GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		refDescHandle.cpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;

		CB_OBJECT_DATA objectData;
		{
			XMStoreFloat4x4(&objectData.xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));
		}
		m_WorldCBuffer.UpdateData(&objectData);
		pd3dDevice->CopyDescriptorsSimple(1, refDescHandle.cpuHandle,
			m_WorldCBuffer.GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		refDescHandle.cpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;

		// Textures
		m_pMaterials[0]->CopyTextureDescriptors(pd3dDevice, refDescHandle);

		// Descriptor Set
		pd3dCommandList->SetGraphicsRootDescriptorTable(4, refDescHandle.gpuHandle);
		refDescHandle.gpuHandle.ptr += (2 + Material::g_nTexturesPerMaterial) * GameFramework::g_uiDescriptorHandleIncrementSize;
		// 2 (CB_MATERIAL_DATA, World) + Texture 7개 

		pd3dCommandList->SetGraphicsRoot32BitConstant(5, -1, 0);

		// pipeline[5] : 거울을 블렌딩해서 그림
		m_pMaterials[0]->OnPrepareRender(pd3dCommandList, 5);
		float fBlendFactors[4] = { m_fBlendFactor, m_fBlendFactor, m_fBlendFactor, m_fBlendFactor };
		pd3dCommandList->OMSetBlendFactor(fBlendFactors);
		m_pMesh->Render(pd3dCommandList, 0, 1);
	}


}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BuildingObject 

BuildingObject::BuildingObject()
{
}

BuildingObject::~BuildingObject()
{
}

void BuildingObject::Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, float fWidth, float fLength, float fHeight, int nWindowsInWidth, int nWindowsInHeights)
{
	//
	//             +y
	//	          |  +z
	//	          |  /
	//	  fHeight | /  fLength
	//	          |/  
	//	          +--------- +x
	//              fWidth

	// 11.11 계획 변경
	// 빌딩의 한 면만 거울로 만든다...

	std::shared_ptr<StandardMesh> pFrontBackMirrorMesh = StandardMesh::GenerateMirrorMesh(pd3dDevice, pd3dCommandList, fWidth, fHeight, nWindowsInWidth, nWindowsInHeights);
	std::shared_ptr<StandardMesh> pLeftRightMirrorMesh = StandardMesh::GenerateMirrorMesh(pd3dDevice, pd3dCommandList, fLength, fHeight, nWindowsInWidth, nWindowsInHeights);

	std::shared_ptr<Material> pMirrorMaterial = std::make_shared<Material>(pd3dDevice, pd3dCommandList);
	pMirrorMaterial->SetTexture(TEXTURE_INDEX_DIFFUSE_MAP, TEXTURE->GetTexture("window"));
	pMirrorMaterial->SetTexture(TEXTURE_INDEX_NORMAL_MAP, TEXTURE->GetTexture("window_normal"));
	pMirrorMaterial->SetMaterialType(MATERIAL_TYPE_ALBEDO_MAP | MATERIAL_TYPE_NORMAL_MAP);
	pMirrorMaterial->SetShader(SHADER->Get<MirrorShader>());

	std::shared_ptr<Material> pStandardMaterial = std::make_shared<Material>(pd3dDevice, pd3dCommandList);
	pStandardMaterial->SetTexture(TEXTURE_INDEX_DIFFUSE_MAP, TEXTURE->GetTexture("window"));
	pStandardMaterial->SetTexture(TEXTURE_INDEX_NORMAL_MAP, TEXTURE->GetTexture("window_normal"));
	pStandardMaterial->SetMaterialType(MATERIAL_TYPE_ALBEDO_MAP | MATERIAL_TYPE_NORMAL_MAP);
	pStandardMaterial->SetShader(SHADER->Get<StandardShader>());

	float fBlendFactor = 0.5f;
	float fHalfWidth = fWidth / 2;
	float fHalfLength = fLength / 2;
	float fHalfHeight = fHeight / 2;
	XMFLOAT3 xmf3AxisY = XMFLOAT3(0.f, 1.f, 0.f);

	// -Z 방향 -> 여기만 거울임
	std::shared_ptr<MirrorObject> pMirrorFront = std::make_shared<MirrorObject>();
	pMirrorFront->SetName("Mirror_Front");
	pMirrorFront->SetMesh(pFrontBackMirrorMesh);
	pMirrorFront->SetMaterial(0, pMirrorMaterial);
	pMirrorFront->SetBlendFactor(fBlendFactor);
	pMirrorFront->SetPosition(XMFLOAT3(0.f, fHalfHeight, -fHalfLength));
	SetChild(pMirrorFront);
	pMirrorFront->Initialize(pd3dDevice, pd3dCommandList);	// Plane 생성

	// +Z 방향
	std::shared_ptr<GameObject> pMirrorBack = std::make_shared<GameObject>();
	pMirrorBack->SetName("Mirror_Back");
	pMirrorBack->SetMesh(pFrontBackMirrorMesh);
	pMirrorBack->SetMaterial(0, pStandardMaterial);
	pMirrorBack->SetPosition(XMFLOAT3(0.f, fHalfHeight, fHalfLength));
	pMirrorBack->Rotate(&xmf3AxisY, 180.f);
	SetChild(pMirrorBack);
	pMirrorBack->Initialize();

	// -X 방향
	std::shared_ptr<GameObject> pMirrorLeft = std::make_shared<GameObject>();
	pMirrorLeft->SetName("Mirror_Left");
	pMirrorLeft->SetMesh(pLeftRightMirrorMesh);
	pMirrorLeft->SetMaterial(0, pStandardMaterial);
	pMirrorLeft->SetPosition(XMFLOAT3(-fHalfWidth, fHalfHeight, 0.f));
	pMirrorLeft->Rotate(&xmf3AxisY, 90.f);
	SetChild(pMirrorLeft);
	pMirrorLeft->Initialize();

	// +X 방향
	std::shared_ptr<GameObject> pMirrorRight = std::make_shared<GameObject>();
	pMirrorRight->SetName("Mirror_Right");
	pMirrorRight->SetMesh(pLeftRightMirrorMesh);
	pMirrorRight->SetMaterial(0, pStandardMaterial);
	pMirrorRight->SetPosition(XMFLOAT3(fHalfWidth, fHalfHeight, 0.f));
	pMirrorRight->Rotate(&xmf3AxisY, -90.f);
	SetChild(pMirrorRight);
	pMirrorRight->Initialize();

	// 이제 뚜껑을 만들어야 함
	std::shared_ptr<StandardMesh> pTopMesh = StandardMesh::GenerateBuildingTopMesh(pd3dDevice, pd3dCommandList, fWidth, fLength);
	std::shared_ptr<GameObject> pTopObject = std::make_shared<GameObject>();
	pTopObject->SetName("Building_Top");
	pTopObject->SetMesh(pTopMesh);
	pTopObject->SetMaterial(0, pStandardMaterial);
	pTopObject->SetPosition(XMFLOAT3(0.f, fHeight, 0.f));
	SetChild(pTopObject);
	pTopObject->Initialize();

	// m_xmOBB
	m_xmOBB.Center = XMFLOAT3(0.f, fHalfHeight, 0.f);
	m_xmOBB.Extents = XMFLOAT3(fHalfWidth, fHalfHeight, fHalfLength);
	m_fHalfHeight = 0.f;

}

