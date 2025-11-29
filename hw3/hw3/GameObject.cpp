#include "stdafx.h"
#include "GameObject.h"
#include "TerrainObject.h"
#include <filesystem>

GameObject::GameObject()
{
	m_xmf4x4Transform = Matrix4x4::Identity();
	m_xmf4x4World = Matrix4x4::Identity();
}

void GameObject::Initialize()
{
	UpdateTransform(NULL);
	GenerateBigBoundingBox();
}

void GameObject::SetMesh(std::shared_ptr<Mesh> pMesh)
{
	m_pMesh = pMesh;
}

void GameObject::SetShader(std::shared_ptr<Shader> pShader)
{
	m_pMaterials.resize(1);
	m_pMaterials[0] = std::make_shared<Material>();
	m_pMaterials[0]->SetShader(pShader);
}

void GameObject::SetShader(int nMaterial, std::shared_ptr<Shader> pShader)
{
	if (m_pMaterials[nMaterial]) {
		m_pMaterials[nMaterial]->SetShader(pShader);
	}
}

void GameObject::SetMaterial(int nMaterial, std::shared_ptr<Material> pMaterial)
{
	if (nMaterial >= m_pMaterials.size()) {
		m_pMaterials.resize(nMaterial + 1);
	}
	m_pMaterials[nMaterial] = pMaterial;
}

void GameObject::SetChild(std::shared_ptr<GameObject> pChild)
{
	if (pChild)
	{
		pChild->m_pParent = shared_from_this();
		m_pChildren.push_back(pChild);
	}
}

const std::vector<std::shared_ptr<GameObject>>& GameObject::GetChildren()const
{
	return m_pChildren;
}

void GameObject::Update(float fTimeElapsed)
{
	Animate(fTimeElapsed);
	UpdateTransform(nullptr);
}

void GameObject::Animate(float fTimeElapsed) 
{
	for (auto pChild : m_pChildren) {
		pChild->Animate(fTimeElapsed);
	}
}

void GameObject::RenderOBB(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	if (!m_pParent) {
		// Set
		pd3dCommandList->SetGraphicsRootSignature(RenderManager::g_pd3dRootSignature.Get());

		XMFLOAT4 xmf4DebugColor(0.f, 1.f, 0.f, 1.f);

		pd3dCommandList->SetGraphicsRoot32BitConstants(6, 3, &m_xmOBBInWorld.Center, 0);
		pd3dCommandList->SetGraphicsRoot32BitConstants(6, 3, &m_xmOBBInWorld.Extents, 4);
		pd3dCommandList->SetGraphicsRoot32BitConstants(6, 4, &m_xmOBBInWorld.Orientation, 8);
		pd3dCommandList->SetGraphicsRoot32BitConstants(6, 4, &xmf4DebugColor, 12);

		// Draw
		pd3dCommandList->SetGraphicsRootSignature(RenderManager::g_pd3dRootSignature.Get());
		pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		SHADER->Get<OBBDebugShader>()->OnPrepareRender(pd3dCommandList);

		pd3dCommandList->DrawInstanced(1, 1, 0, 0);

		for (auto& pObj : m_pChildren) {
			pObj->RenderOBB(pd3dCommandList);
		}
	}
	
	if (m_pMesh) {
		// Set
		pd3dCommandList->SetGraphicsRootSignature(RenderManager::g_pd3dRootSignature.Get());

		XMFLOAT4 xmf4DebugColor(1.f, 0.f, 0.f, 1.f);

		BoundingOrientedBox xmOBB = m_pMesh->GetOBBOrigin();
		BoundingOrientedBox xmOBBToDraw;
		xmOBB.Transform(xmOBBToDraw, XMLoadFloat4x4(&m_xmf4x4World));
		pd3dCommandList->SetGraphicsRoot32BitConstants(6, 3, &xmOBBToDraw.Center, 0);
		pd3dCommandList->SetGraphicsRoot32BitConstants(6, 3, &xmOBBToDraw.Extents, 4);
		pd3dCommandList->SetGraphicsRoot32BitConstants(6, 4, &xmOBBToDraw.Orientation, 8);
		pd3dCommandList->SetGraphicsRoot32BitConstants(6, 4, &xmf4DebugColor, 12);

		// Draw
		pd3dCommandList->SetGraphicsRootSignature(RenderManager::g_pd3dRootSignature.Get());
		pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		SHADER->Get<OBBDebugShader>()->OnPrepareRender(pd3dCommandList);

		pd3dCommandList->DrawInstanced(1, 1, 0, 0);
	}

	for (auto& pObj : m_pChildren) {
		pObj->RenderOBB(pd3dCommandList);
	}
}

void GameObject::AdjustHeightFromTerrain(std::shared_ptr<TerrainObject> pTerrain)
{
	XMFLOAT3 xmf3Scale = pTerrain->GetScale();
	XMFLOAT3 xmf3Position = GetPosition();
	int z = (int)(xmf3Position.z / xmf3Scale.z);
	bool bReverseQuad = ((z % 2) != 0);
	float fHeight = pTerrain->GetHeight(xmf3Position.x, xmf3Position.z, bReverseQuad) + m_fHalfHeight;
	if (xmf3Position.y < fHeight)
	{
		xmf3Position.y = fHeight;
		SetPosition(xmf3Position);
	}
}

XMFLOAT3 GameObject::GetPosition()
{
	return(XMFLOAT3(m_xmf4x4World._41, m_xmf4x4World._42, m_xmf4x4World._43));
}

XMFLOAT3 GameObject::GetLook()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._31, m_xmf4x4World._32, m_xmf4x4World._33)));
}

XMFLOAT3 GameObject::GetUp()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._21, m_xmf4x4World._22, m_xmf4x4World._23)));
}

XMFLOAT3 GameObject::GetRight()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._11, m_xmf4x4World._12, m_xmf4x4World._13)));
}

void GameObject::SetPosition(float x, float y, float z)
{
	m_xmf4x4Transform._41 = x;
	m_xmf4x4Transform._42 = y;
	m_xmf4x4Transform._43 = z;
	UpdateTransform(NULL);
}

void GameObject::SetPosition(XMFLOAT3 xmf3Position)
{
	SetPosition(xmf3Position.x, xmf3Position.y, xmf3Position.z);
}

void GameObject::SetScale(float x, float y, float z)
{
	XMMATRIX mtxScale = XMMatrixScaling(x, y, z);
	m_xmf4x4Transform = Matrix4x4::Multiply(mtxScale, m_xmf4x4Transform);
	UpdateTransform(NULL);
}

void GameObject::MoveStrafe(float fDistance)
{
	XMFLOAT3 xmf3Position = GetPosition();
	XMFLOAT3 xmf3Right = GetRight();
	xmf3Position = Vector3::Add(xmf3Position, xmf3Right, fDistance);
	GameObject::SetPosition(xmf3Position);
}

void GameObject::MoveUp(float fDistance)
{
	XMFLOAT3 xmf3Position = GetPosition();
	XMFLOAT3 xmf3Up = GetUp();
	xmf3Position = Vector3::Add(xmf3Position, xmf3Up, fDistance);
	GameObject::SetPosition(xmf3Position);
}

void GameObject::MoveForward(float fDistance)
{
	XMFLOAT3 xmf3Position = GetPosition();
	XMFLOAT3 xmf3Look = GetLook();
	xmf3Position = Vector3::Add(xmf3Position, xmf3Look, fDistance);
	GameObject::SetPosition(xmf3Position);
}

void GameObject::Rotate(float fPitch, float fYaw, float fRoll)
{
	XMMATRIX mtxRotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(fPitch), XMConvertToRadians(fYaw), XMConvertToRadians(fRoll));
	m_xmf4x4Transform = Matrix4x4::Multiply(mtxRotate, m_xmf4x4Transform);
	UpdateTransform(NULL);
}

void GameObject::Rotate(XMFLOAT3* pxmf3Axis, float fAngle)
{
	XMMATRIX mtxRotate = XMMatrixRotationAxis(XMLoadFloat3(pxmf3Axis), XMConvertToRadians(fAngle));
	m_xmf4x4Transform = Matrix4x4::Multiply(mtxRotate, m_xmf4x4Transform);
	UpdateTransform(NULL);
}

void GameObject::Rotate(XMFLOAT4* pxmf4Quaternion)
{
	XMMATRIX mtxRotate = XMMatrixRotationQuaternion(XMLoadFloat4(pxmf4Quaternion));
	m_xmf4x4Transform = Matrix4x4::Multiply(mtxRotate, m_xmf4x4Transform);
	UpdateTransform(NULL);
}

void GameObject::CacheLastFrameTransform()
{
	m_xmf4x4CachedLastFrameTransform = m_xmf4x4Transform;
}

void GameObject::UpdateTransform(XMFLOAT4X4* pxmf4x4Parent)
{
	m_xmf4x4World = (pxmf4x4Parent) ? Matrix4x4::Multiply(m_xmf4x4Transform, *pxmf4x4Parent) : m_xmf4x4Transform;

	if (m_pMesh) {
		m_pMesh->UpdateOBB(m_xmf4x4World);
	}

	if (!m_pParent) {
		m_xmOBB.Transform(m_xmOBBInWorld, XMLoadFloat4x4(&m_xmf4x4World));
	}

	for (auto& pChild : m_pChildren) {
		pChild->UpdateTransform(&m_xmf4x4World);
	}
}

std::shared_ptr<GameObject> GameObject::FindFrame(const std::string& strFrameName)
{
	std::shared_ptr<GameObject> pFrameObject;
	if (strFrameName == m_strFrameName) {
		return shared_from_this();
	}

	for (auto& pChild : m_pChildren) {
		if (pFrameObject = pChild->FindFrame(strFrameName)) {
			return pFrameObject;
		}
	}

	return nullptr;
}

bool GameObject::IsInFrustum(std::shared_ptr<Camera> pCamera)
{
	return pCamera->IsInFrustum(m_xmOBBInWorld);
}

void GameObject::GenerateBigBoundingBox(bool bFlipYZ, bool bCenterOnFloor)
{
	if (m_pChildren.size() == 0) {
		return;
	}

	float fMinX = std::numeric_limits<float>::max();
	float fMinY = std::numeric_limits<float>::max();
	float fMinZ = std::numeric_limits<float>::max();
	
	float fMaxX = std::numeric_limits<float>::lowest();
	float fMaxY = std::numeric_limits<float>::lowest();
	float fMaxZ = std::numeric_limits<float>::lowest();

	for (auto pChild : m_pChildren) {
		pChild->UpdateMinMaxInBoundingBox(fMinX, fMaxX, fMinY, fMaxY, fMinZ, fMaxZ);
	}

	float fHalfLengthX = abs(fMaxX - fMinX) / 2;
	float fHalfLengthY = abs(fMaxY - fMinY) / 2;
	float fHalfLengthZ = abs(fMaxZ - fMinZ) / 2;

	float fCenterX = (fMaxX + fMinX) / 2;
	float fCenterY = (fMaxY + fMinY) / 2;
	float fCenterZ = (fMaxZ + fMinZ) / 2;
	if (bFlipYZ) {
		std::swap(fCenterY, fCenterZ);
		fCenterY = -fCenterY;
	}

	m_xmOBB.Center = XMFLOAT3(fCenterX, fCenterY, fCenterZ);
	m_xmOBB.Extents = XMFLOAT3(fHalfLengthX, fHalfLengthY, fHalfLengthZ);
	m_fHalfHeight = bCenterOnFloor ? 0.f : fHalfLengthY;

	// 가끔 중심점이 밑바닥에 있어 fHalfLengthY 를 사용하면 땅 위에 떠버리는 경우들이 있음
	// 코드로는 확인이 어려울것으로 보이고 직접 눈으로 확인해서 이상하면 이걸 수동으로 설정해주어야할듯함

}

void GameObject::UpdateMinMaxInBoundingBox(float& fMinX, float& fMaxX, float& fMinY, float& fMaxY, float& fMinZ, float& fMaxZ)
{
	if (m_pMesh) {
		BoundingOrientedBox xmOBBFromMesh = m_pMesh->GetOBB();
		XMVECTOR xmvOBBCenter = XMLoadFloat3(&xmOBBFromMesh.Center);
		XMVECTOR xmvOBBExtent = XMLoadFloat3(&xmOBBFromMesh.Extents);
		XMVECTOR xmvOBBOrientation = XMLoadFloat4(&xmOBBFromMesh.Orientation);

		XMFLOAT3 xmf3OBBMax;
		XMFLOAT3 xmf3OBBMin;
		XMStoreFloat3(&xmf3OBBMax, XMVector3Rotate(XMVectorAdd(xmvOBBCenter, xmvOBBExtent), xmvOBBOrientation));
		XMStoreFloat3(&xmf3OBBMin, XMVector3Rotate(XMVectorSubtract(xmvOBBCenter, xmvOBBExtent), xmvOBBOrientation));

		fMinX = std::min(fMinX, xmf3OBBMin.x);
		fMinY = std::min(fMinY, xmf3OBBMin.y);
		fMinZ = std::min(fMinZ, xmf3OBBMin.z);
		
		fMaxX = std::max(fMaxX, xmf3OBBMax.x);
		fMaxY = std::max(fMaxY, xmf3OBBMax.y);
		fMaxZ = std::max(fMaxZ, xmf3OBBMax.z);
	}

	for (auto pChild : m_pChildren) {
		pChild->UpdateMinMaxInBoundingBox(fMinX, fMaxX, fMinY, fMaxY, fMinZ, fMaxZ);
	}
}

void GameObject::AddToRenderMap(bool bTransparent)
{
	if (m_pMesh) {
		if (bTransparent) {
			RENDER->AddTransparent(shared_from_this());
		}
		else {
			RENDER->Add(shared_from_this());
		}
	}

	for (auto& pChild : m_pChildren) {
		pChild->AddToRenderMap(bTransparent);
	}
}

void GameObject::ReleaseUploadBuffers()
{
	if (m_pMesh) m_pMesh->ReleaseUploadBuffers();

	for (auto& pChild : m_pChildren) {
		pChild->ReleaseUploadBuffers();
	}
}

bool GameObject::CheckCollisionSet(std::shared_ptr<GameObject> pOther)
{
	auto it = m_pCollisionSet.find(pOther);
	return it != m_pCollisionSet.end();
}

void GameObject::GenerateRayForPicking(XMVECTOR& xmvPickPosition, const XMMATRIX& xmmtxView, XMVECTOR& xmvPickRayOrigin, XMVECTOR& xmvPickRayDirection) const
{
	XMMATRIX xmmtxInvWorld = XMMatrixInverse(nullptr, XMMatrixMultiply(XMLoadFloat4x4(&m_xmf4x4World), xmmtxView));


	XMFLOAT3 xmf3CameraOrigin{ 0.f, 0.f,0.f };
	xmvPickRayOrigin = XMVector3TransformCoord(XMLoadFloat3(&xmf3CameraOrigin), xmmtxInvWorld);
	xmvPickRayDirection = XMVector3TransformCoord(xmvPickPosition, xmmtxInvWorld);
	xmvPickRayDirection = XMVector3Normalize(xmvPickRayDirection - xmvPickRayOrigin);
}

bool GameObject::PickObjectByRayIntersection(XMVECTOR& xmvPickPosition, const XMMATRIX& xmmtxView, float& fHitDistance) const
{
	bool bResult;

	if (m_pMesh) {
		XMVECTOR xmvPickRayOrigin, xmvPickRayDirection;
		GenerateRayForPicking(xmvPickPosition, xmmtxView, xmvPickRayOrigin, xmvPickRayDirection);

		return m_pMesh->GetOBBOrigin().Intersects(xmvPickRayOrigin, xmvPickRayDirection, fHitDistance);
	}

	for (auto pChild : m_pChildren) {
		bResult = pChild->PickObjectByRayIntersection(xmvPickPosition, xmmtxView, fHitDistance);
	}

	return bResult;
}

void GameObject::LoadMaterialsFromFile(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::ifstream& inFile)
{
	std::string strRead;

	int nMaterials;
	int materialIndex;
	inFile.read((char*)(&nMaterials), sizeof(int));
	m_pMaterials.resize(nMaterials);

	std::shared_ptr<Material> pMaterial;

	auto setTextureFunction = [&](UINT nTextureIndex) -> bool {
		strRead = ::ReadStringFromFile(inFile);
		if (strRead == "null") {
			return false;
		}

		if (strRead[0] == '@') {	// 중복임
			strRead.erase(0, 1);
			pMaterial->SetTexture(nTextureIndex, TEXTURE->GetTexture(strRead));
		}
		else {
			std::wstring strTextureFilename = std::wstring(strRead.begin(), strRead.end()) + L".dds";
			pMaterial->SetTexture(nTextureIndex, TEXTURE->LoadTexture(pd3dCommandList, strRead, strTextureFilename, RESOURCE_TYPE_TEXTURE2D));
		}

		// Set Standard shader
		pMaterial->SetShader(SHADER->Get<StandardShader>());

		return true;
	};

	while (true) {
		strRead = ::ReadStringFromFile(inFile);

		if (strRead == "<Material>:")
		{
			inFile.read((char*)(&materialIndex), sizeof(int));
			pMaterial = std::make_shared<Material>(pd3dDevice, pd3dCommandList);
			SetMaterial(materialIndex, pMaterial);
		}
		else if (strRead == "<AlbedoColor>:")
		{
			inFile.read((char*)(&pMaterial->m_xmf4AlbedoColor), sizeof(XMFLOAT4));
		}
		else if (strRead == "<EmissiveColor>:")
		{
			inFile.read((char*)(&pMaterial->m_xmf4EmissiveColor), sizeof(XMFLOAT4));
		}
		else if (strRead == "<SpecularColor>:")
		{
			inFile.read((char*)(&pMaterial->m_xmf4SpecularColor), sizeof(XMFLOAT4));
		}
		else if (strRead == "<Glossiness>:")
		{
			inFile.read((char*)(&pMaterial->m_fGlossiness), sizeof(float));
		}
		else if (strRead == "<Smoothness>:")
		{
			inFile.read((char*)(&pMaterial->m_fSmoothness), sizeof(float));
		}
		else if (strRead == "<Metallic>:")
		{
			inFile.read((char*)(&pMaterial->m_fMetallic), sizeof(float));
		}
		else if (strRead == "<SpecularHighlight>:")
		{
			inFile.read((char*)(&pMaterial->m_fSpecularHighlight), sizeof(float));
		}
		else if (strRead == "<GlossyReflection>:")
		{
			inFile.read((char*)(&pMaterial->m_fGlossyReflection), sizeof(float));
		}
		else if (strRead == "<AlbedoMap>:")
		{
			bool bResult = setTextureFunction(TEXTURE_INDEX_ALBEDO_MAP);
			if (bResult) {
				pMaterial->SetMaterialType(MATERIAL_TYPE_ALBEDO_MAP);
			}
		}
		else if (strRead == "<SpecularMap>:")
		{
			bool bResult = setTextureFunction(TEXTURE_INDEX_SPECULAR_MAP);
			if (bResult) {
				pMaterial->SetMaterialType(MATERIAL_TYPE_SPECULAR_MAP);
			}
		}
		else if (strRead == "<NormalMap>:")
		{
			bool bResult = setTextureFunction(TEXTURE_INDEX_NORMAL_MAP);
			if (bResult) {
				pMaterial->SetMaterialType(MATERIAL_TYPE_NORMAL_MAP);
			}
		}
		else if (strRead == "<MetallicMap>:")
		{
			bool bResult = setTextureFunction(TEXTURE_INDEX_METALLIC_MAP);
			if (bResult) {
				pMaterial->SetMaterialType(MATERIAL_TYPE_METALLIC_MAP);
			}
		}
		else if (strRead == "<EmissionMap>:")
		{
			bool bResult = setTextureFunction(TEXTURE_INDEX_EMISSION_MAP);
			if (bResult) {
				pMaterial->SetMaterialType(MATERIAL_TYPE_EMISSION_MAP);
			}
		}
		else if (strRead == "<DetailAlbedoMap>:")
		{
			bool bResult = setTextureFunction(TEXTURE_INDEX_DETAIL_ALBEDO_MAP);
			if (bResult) {
				pMaterial->SetMaterialType(MATERIAL_TYPE_DETAIL_ALBEDO_MAP);
			}
		}
		else if (strRead == "<DetailNormalMap>:")
		{
			bool bResult = setTextureFunction(TEXTURE_INDEX_DETAIL_NORMAL_MAP);
			if (bResult) {
				pMaterial->SetMaterialType(MATERIAL_TYPE_DETAIL_NORMAL_MAP);
			}
		}
		else if (strRead == "</Materials>")
		{
			break;
		}
	}
}

std::shared_ptr<GameObject> GameObject::LoadFrameHierarchyFromFile(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, ComPtr<ID3D12RootSignature> pd3dRootSignature, std::shared_ptr<GameObject> pParent, std::ifstream& inFile)
{
	std::string strRead;

	int nFrames = 0, nTextures = 0;
	std::shared_ptr<GameObject> pGameObject;

	while (true) {
		strRead = ::ReadStringFromFile(inFile);

		if (strRead == "<Frame>:") {
			pGameObject = std::make_shared<GameObject>();

			inFile.read((char*)&nFrames, sizeof(int));
			inFile.read((char*)&nTextures, sizeof(int));
			pGameObject->m_strFrameName = ::ReadStringFromFile(inFile);

			if (pParent) {
				pGameObject->m_pParent = pParent;
			}
		}
		else if (strRead == "<Transform>:") {
			XMFLOAT3 xmf3Position, xmf3Rotation, xmf3Scale;
			XMFLOAT4 xmf4Quaternion;
			inFile.read((char*)&xmf3Position, sizeof(XMFLOAT3));
			inFile.read((char*)&xmf3Rotation, sizeof(XMFLOAT3)); //Euler Angle
			inFile.read((char*)&xmf3Scale, sizeof(XMFLOAT3));
			inFile.read((char*)&xmf4Quaternion, sizeof(XMFLOAT4)); //Quaternion
		}
		else if (strRead == "<TransformMatrix>:") {
			inFile.read((char*)&pGameObject->m_xmf4x4Transform, sizeof(XMFLOAT4X4));
		}
		else if (strRead == "<Mesh>:") {
			std::shared_ptr<StandardMesh> pMesh = std::make_shared<StandardMesh>();
			pMesh->LoadMeshFromFile(pd3dDevice, pd3dCommandList, inFile);
			pGameObject->SetMesh(pMesh);
		}
		else if (strRead == "<Materials>:") {
			pGameObject->LoadMaterialsFromFile(pd3dDevice, pd3dCommandList, inFile);
		}
		else if (strRead == "<Children>:") {
			int nChildren;
			inFile.read((char*)&nChildren, sizeof(int));
			if (pGameObject) {
				pGameObject->m_pChildren.reserve(nChildren);
			}

			if (nChildren > 0) {
				for (int i = 0; i < nChildren; ++i) {
					std::shared_ptr<GameObject> pChild = GameObject::LoadFrameHierarchyFromFile(pd3dDevice, pd3dCommandList, pd3dRootSignature, pGameObject, inFile);
					if (pChild) {
						pGameObject->m_pChildren.push_back(pChild);
					}
				}
			}
		}
		else if ((strRead == "</Frame>")){
			break;
		}
	}

	return pGameObject;
}

std::shared_ptr<GameObject> GameObject::LoadGeometryFromFile(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, ComPtr<ID3D12RootSignature> pd3dRootSignature, const std::string& strFilePath)
{
	std::string strFileName = std::filesystem::path{ strFilePath }.stem().string();

	if (auto pObj = RESOURCE->GetGameObject(strFileName)) {
		return pObj;
	}

	std::ifstream inFile{ strFilePath, std::ios::binary };
	if (!inFile) {
		__debugbreak();
	}

	std::shared_ptr<GameObject> pGameObject = GameObject::LoadFrameHierarchyFromFile(pd3dDevice, pd3dCommandList, pd3dRootSignature, nullptr, inFile);
	

	if (pGameObject) {
		return RESOURCE->AddGameObject(strFileName, pGameObject);
	}

	return nullptr;
}

std::shared_ptr<GameObject> GameObject::CopyObject(const GameObject& srcObject, std::shared_ptr<GameObject> pParent)
{
	std::shared_ptr<GameObject> pClone = std::make_shared<GameObject>();
	pClone->m_strFrameName = srcObject.m_strFrameName;
	pClone->m_pMesh = srcObject.m_pMesh;
	pClone->m_pMaterials = srcObject.m_pMaterials;
	pClone->m_xmf4x4Transform = srcObject.m_xmf4x4Transform;
	pClone->m_xmf4x4World = srcObject.m_xmf4x4World;

	pClone->m_xmOBB = srcObject.m_xmOBB;
	pClone->m_xmOBBInWorld = srcObject.m_xmOBBInWorld;

	pClone->m_pParent = pParent;

	pClone->m_pChildren.reserve(srcObject.m_pChildren.size());
	for (auto& pChild : srcObject.m_pChildren) {
		std::shared_ptr<GameObject> pChildClone = CopyObject(*pChild, pClone);
		pClone->m_pChildren.push_back(pChildClone);
	}

	return pClone;
}

///////////////////////
// HellicopterObject //
///////////////////////

HellicopterObject::HellicopterObject()
{
}

HellicopterObject::~HellicopterObject()
{
}

void HellicopterObject::Initialize()
{
}

void HellicopterObject::Animate(float fTimeElapsed)
{
	if (m_pMainRotorFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationY(XMConvertToRadians(360.0f * 2.0f) * fTimeElapsed);
		m_pMainRotorFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pMainRotorFrame->m_xmf4x4Transform);
	}
	if (m_pTailRotorFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationX(XMConvertToRadians(360.0f * 4.0f) * fTimeElapsed);
		m_pTailRotorFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pTailRotorFrame->m_xmf4x4Transform);
	}

	GameObject::Animate(fTimeElapsed);
}

//////////////////
// ApacheObject //
//////////////////

ApacheObject::ApacheObject()
{
}

ApacheObject::~ApacheObject()
{
}

void ApacheObject::Initialize()
{
	m_pMainRotorFrame = FindFrame("rotor");
	m_pTailRotorFrame = FindFrame("black_m_7");
}

void ApacheObject::Animate(float fTimeElapsed)
{
	if (m_pMainRotorFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationY(XMConvertToRadians(360.0f * 2.0f) * fTimeElapsed);
		m_pMainRotorFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pMainRotorFrame->m_xmf4x4Transform);
	}
	if (m_pTailRotorFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationY(XMConvertToRadians(360.0f * 4.0f) * fTimeElapsed);
		m_pTailRotorFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pTailRotorFrame->m_xmf4x4Transform);
	}

	GameObject::Animate(fTimeElapsed);
}

///////////////////
// GunshipObject //
///////////////////

GunshipObject::GunshipObject()
{
}

GunshipObject::~GunshipObject()
{
}

void GunshipObject::Initialize()
{
	m_pMainRotorFrame = FindFrame("Rotor");
	m_pTailRotorFrame = FindFrame("Back_Rotor");
}

//////////////////////
// SuperCobraObject //
//////////////////////

SuperCobraObject::SuperCobraObject()
{
}

SuperCobraObject::~SuperCobraObject()
{
}

void SuperCobraObject::Initialize()
{
	m_pMainRotorFrame = FindFrame("MainRotor_LOD0");
	m_pTailRotorFrame = FindFrame("TailRotor_LOD0");
}

////////////////
// Mi24Object //
////////////////

Mi24Object::Mi24Object()
{
}

Mi24Object::~Mi24Object()
{
}

void Mi24Object::Initialize()
{
	m_pMainRotorFrame = FindFrame("Top_Rotor");
	m_pTailRotorFrame = FindFrame("Tail_Rotor");
}

//////////////////
// HummerObject //
//////////////////

HummerObject::HummerObject()
{
}

HummerObject::~HummerObject()
{
}

void HummerObject::Initialize()
{
	m_pLFWheelFrame = FindFrame("wheel_LF");
	m_pLRWheelFrame = FindFrame("wheel_LR");
	m_pRFWheelFrame = FindFrame("Wheel_RF");
	m_pRRWheelFrame = FindFrame("wheel_RR");
}

void HummerObject::Animate(float fTimeElapsed)
{
	if (m_pLFWheelFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationX(XMConvertToRadians(360.0f * 2.0f) * fTimeElapsed);
		m_pLFWheelFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pLFWheelFrame->m_xmf4x4Transform);
	}
	
	if (m_pLRWheelFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationX(XMConvertToRadians(360.0f * 2.0f) * fTimeElapsed);
		m_pLRWheelFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pLRWheelFrame->m_xmf4x4Transform);
	}
	
	if (m_pRFWheelFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationX(XMConvertToRadians(360.0f * 2.0f) * fTimeElapsed);
		m_pRFWheelFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pRFWheelFrame->m_xmf4x4Transform);
	}
	
	if (m_pRRWheelFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationX(XMConvertToRadians(360.0f * 2.0f) * fTimeElapsed);
		m_pRRWheelFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pRRWheelFrame->m_xmf4x4Transform);
	}

	GameObject::Animate(fTimeElapsed);
}

////////////////
// TankObject //
////////////////

TankObject::TankObject()
{
}

TankObject::~TankObject()
{
}

void TankObject::Initialize()
{
}

void TankObject::Animate(float fTimeElapsed)
{
	if (m_pTurretFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationY(XMConvertToRadians(30.0f) * fTimeElapsed);
		m_pTurretFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pTurretFrame->m_xmf4x4Transform);
	}

	GameObject::Animate(fTimeElapsed);
}

///////////////
// M26Object //
///////////////

M26Object::M26Object()
{
}

M26Object::~M26Object()
{
}

void M26Object::Initialize()
{
	m_pTurretFrame = FindFrame("TURRET");
	m_pCannonFrame = FindFrame("cannon");
	m_pGunFrame = FindFrame("gun");
}
