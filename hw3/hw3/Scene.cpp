#include "stdafx.h"
#include "Scene.h"
#include "TerrainObject.h"

Scene::Scene()
{
}

void Scene::BuildDefaultLightsAndMaterials()
{
}

void Scene::BuildObjects(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
}

void Scene::ReleaseUploadBuffers()
{
	if (m_pPlayer) {
		m_pPlayer->ReleaseUploadBuffers();
	}

	for (auto& pObj : m_pGameObjects) {
		pObj->ReleaseUploadBuffers();
	}

}

bool Scene::ProcessInput(UCHAR* pKeysBuffer)
{
	return false;
}

void Scene::Update(float fTimeElapsed)
{
	if (m_pPlayer) {
		m_pPlayer->Update(fTimeElapsed);
	}
	
	for (auto& pObj : m_pGameObjects) {
		pObj->Update(fTimeElapsed);
	}

	if (m_pTerrain) {
		m_pTerrain->Update(fTimeElapsed);
	}
}

void Scene::Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	if (m_pPlayer) {
		m_pPlayer->UpdateTransform();
		m_pPlayer->OnPrepareRender();
		if (m_pPlayer->IsInFrustum(GetCamera())) {
			m_pPlayer->AddToRenderMap(false);
		}
	}

	for (auto& pObj : m_pGameObjects) {
		pObj->UpdateTransform();
		pObj->OnPrepareRender();
		if (pObj->IsInFrustum(GetCamera())) {
			pObj->AddToRenderMap(false);
		}
	}

	for (auto& pSprite : m_pSprites) {
		pSprite->AddToUI(0);
	}

	if (m_pTerrain) {
		RENDER->SetTerrain(m_pTerrain);
	}
}

void Scene::RenderDebug(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	if (m_pPlayer) {
		m_pPlayer->RenderOBB(pd3dCommandList);
	}

	for (auto& pObj : m_pGameObjects) {
		pObj->RenderOBB(pd3dCommandList);
	}

}

void Scene::CreateShaderVariables(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	m_LightCBuffer.Create(pd3dDevice, pd3dCommandList, ConstantBufferSize<CB_LIGHT_DATA>::value, true);

	for (auto& pSprite : m_pSprites) {
		pSprite->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	}
}

void Scene::UpdateShaderVariable(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	CB_LIGHT_DATA lightData;
	lightData.nLights = m_pLights.size();

	for (int i = 0; i < m_pLights.size(); ++i) {
		lightData.LightData[i] = m_pLights[i]->MakeLightData();
	}

	lightData.globalAmbientLight = m_xmf4GlobalAmbient;

	m_LightCBuffer.UpdateData(&lightData);

}

CB_LIGHT_DATA Scene::GetLightCBData()
{
	CB_LIGHT_DATA lightData;
	lightData.nLights = m_pLights.size();

	for (int i = 0; i < m_pLights.size(); ++i) {
		lightData.LightData[i] = m_pLights[i]->MakeLightData();
	}

	lightData.globalAmbientLight = m_xmf4GlobalAmbient;

	return lightData;
}

bool Scene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	return false;
}

bool Scene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'W': m_pPlayer->MoveForward(+1.0f); break;
		case 'S': m_pPlayer->MoveForward(-1.0f); break;
		case 'A': m_pPlayer->MoveStrafe(-1.0f); break;
		case 'D': m_pPlayer->MoveStrafe(+1.0f); break;
		case 'Q': m_pPlayer->MoveUp(+1.0f); break;
		case 'R': m_pPlayer->MoveUp(-1.0f); break;

		default:
			break;
		}
		break;
	default:
		break;
	}
	return false;
}

void Scene::CreateRootSignature(ComPtr<ID3D12Device> pd3dDevice)
{
}

std::shared_ptr<Sprite> Scene::CheckButtonClicked()
{
	POINT ptCursorPos;
	GetCursorPos(&ptCursorPos);
	ScreenToClient(GameFramework::g_hWnd, &ptCursorPos);

	float fCursorX = (float)ptCursorPos.x / GameFramework::g_nClientWidth;
	float fCursorY = (float)ptCursorPos.y / GameFramework::g_nClientHeight;

	std::shared_ptr<Sprite> pReturn = nullptr;
	int nLastClickedSpriteLayerIndex = -1;
	for (const auto& pSprite : m_pSprites) {
		if (pSprite->IsCursorInSprite(fCursorX, fCursorY)) {
			pReturn = (int)pSprite->GetLayerIndex() > nLastClickedSpriteLayerIndex ? pSprite : pReturn;
		}
	}

	return pReturn;
}

void Scene::CheckCollision()
{
	// 일단 Player 와 오브젝트간의 충돌만 검사
	// 오브젝트는 움직이게 하지 않을것이기 때문
	// 계층의 OBB 와 일일히 검사하지 않고, Root 의 m_xmOBBInWorld 와만 충돌을 체크

	BoundingOrientedBox xmPlayerOBB = m_pPlayer->GetOBB();
	for (auto& pObj : m_pGameObjects) {
		BoundingOrientedBox xmObjectOBB = pObj->GetOBB();
		
		if (xmPlayerOBB.Intersects(xmObjectOBB)) {
			bool bAlreadyCollided = m_pPlayer->CheckCollisionSet(pObj);
			if (!bAlreadyCollided) {
				// OnBeginCollision
				m_pPlayer->OnBeginCollision(pObj);
				m_pPlayer->AddToCollisionSet(pObj);
			}
			else {
				// OnInCollision
				m_pPlayer->OnInCollision(pObj);
			}
		}
		else {
			bool bAlreadyCollided = m_pPlayer->CheckCollisionSet(pObj);
			if (bAlreadyCollided) {
				// OnEndCollision
				m_pPlayer->OnEndCollision(pObj);
				m_pPlayer->EraseFromCollisionSet(pObj);
			}
		}

	}
}

std::shared_ptr<GameObject> Scene::PickObjectPointedByCursor(int xClient, int yClient, std::shared_ptr<Camera> pCamera)
{
	XMFLOAT3 xmf3PickPosition;
	
	xmf3PickPosition.x = (((2.0f * xClient) / (float)pCamera->GetViewport().Width) - 1) / pCamera->GetProjectionMatrix()._11;
	xmf3PickPosition.y = -(((2.0f * yClient) / (float)pCamera->GetViewport().Height) - 1) / pCamera->GetProjectionMatrix()._22;
	xmf3PickPosition.z = 1.0f;

	XMVECTOR xmvPickPosition = XMLoadFloat3(&xmf3PickPosition);
	XMMATRIX xmmtxView = XMLoadFloat4x4(&pCamera->GetViewMatrix());

	BOOL bIntersected = FALSE;
	float fNearestHitDistance = std::numeric_limits<float>::max();
	std::shared_ptr<GameObject> pNearestObject = nullptr;
	for (auto& pObj : m_pGameObjects) {
		float fHitDistance = std::numeric_limits<float>::max();
		bIntersected = pObj->PickObjectByRayIntersection(xmvPickPosition, xmmtxView, fHitDistance);
		if ((bIntersected) && (fHitDistance < fNearestHitDistance)) {
			fNearestHitDistance = fHitDistance;
			pNearestObject = pObj;
		}
	}

	return pNearestObject;

}

std::shared_ptr<Camera> Scene::GetCamera() const
{
	return m_pPlayer->GetCamera();
}

std::shared_ptr<Player> Scene::GetPlayer() const
{
	return m_pPlayer;
}

const std::vector<std::shared_ptr<GameObject>>& Scene::GetGameObjects() const
{
	return m_pGameObjects;
}

void Scene::SetTerrainWireframeMode(bool bMode)
{
	if (m_pTerrain) {
		m_pTerrain->SetWireframeMode(bMode);
	}
}
