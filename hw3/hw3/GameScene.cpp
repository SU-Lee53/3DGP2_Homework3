#include "stdafx.h"
#include "GameScene.h"
#include "TerrainObject.h"
#include "BuildingObject.h"

GameScene::GameScene()
{
}

void GameScene::BuildDefaultLightsAndMaterials(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	m_pLights.reserve(2);

	m_xmf4GlobalAmbient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);

	std::shared_ptr<SpotLight> pLight1 = std::make_shared<SpotLight>(pd3dDevice, pd3dCommandList);
	pLight1->m_bEnable = true;
	pLight1->m_fRange = 1200.0f;
	pLight1->m_xmf4Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	pLight1->m_xmf4Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pLight1->m_xmf4Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
	pLight1->m_xmf3Position = XMFLOAT3(-50.0f, 20.0f, -5.0f);
	pLight1->m_xmf3Direction = XMFLOAT3(0.0f, 0.0f, 1.0f);
	pLight1->m_fAttenuation0 = 1.0f;
	pLight1->m_fAttenuation1 = 0.01f;
	pLight1->m_fAttenuation2 = 0.0001f;
	pLight1->m_fFalloff = 8.0f;
	pLight1->m_fPhi = (float)cos(XMConvertToRadians(40.0f));
	pLight1->m_fTheta = (float)cos(XMConvertToRadians(20.0f));

	XMStoreFloat4x4(&pLight1->m_xmf4x4ViewFromLight, XMMatrixLookToLH(XMLoadFloat3(&pLight1->m_xmf3Position), XMLoadFloat3(&pLight1->m_xmf3Direction), XMVectorSet(0, 1, 0, 0)));
	XMStoreFloat4x4(&pLight1->m_xmf4x4ProjectionFromLight, XMMatrixPerspectiveFovLH(XMConvertToRadians(pLight1->m_fPhi * 2), 1, 10.f, pLight1->m_fRange));
	m_pLights.push_back(pLight1);

	std::shared_ptr<DirectionalLight> pLight2 = std::make_shared<DirectionalLight>(pd3dDevice, pd3dCommandList);
	pLight2->m_bEnable = true;
	pLight2->m_xmf4Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	pLight2->m_xmf4Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	pLight2->m_xmf4Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 0.0f);
	pLight2->m_xmf3Direction = XMFLOAT3(1.0f, -1.0f, 1.f);

	// Scene의 중심 구함
	XMFLOAT3 xmf3SceneCenter = GetBoundingBox().Center;

	// Drectional Light 의 위치 계산
	float fLightDistance = 5000.f;
	pLight2->m_xmf3Position = Vector3::Subtract(xmf3SceneCenter, Vector3::ScalarProduct(pLight2->m_xmf3Direction, fLightDistance));
	XMStoreFloat4x4(&pLight2->m_xmf4x4ViewFromLight, XMMatrixLookToLH(XMLoadFloat3(&pLight2->m_xmf3Position), XMVector3Normalize(XMLoadFloat3(&pLight2->m_xmf3Direction)), XMVectorSet(0.f, 1.f, 0.f, 0.f)));

	// Orthographic 의 Width, Height, Near, Far 계산
	XMVECTOR xmvAABBMin = g_XMFltMax;
	XMVECTOR xmvAABBMax = g_XMFltMin;
	XMVECTOR pxmvLightSpaceSceneAABBPoints[8];

	XMFLOAT3 xmf3SceneCorners[8];
	GetBoundingBox().GetCorners(xmf3SceneCorners);
	for (int i = 0; i < _countof(xmf3SceneCorners); ++i) {
		XMFLOAT4 xmf4SceneAABBPoint = XMFLOAT4(xmf3SceneCorners[i].x, xmf3SceneCorners[i].y, xmf3SceneCorners[i].z, 1.0f);
		pxmvLightSpaceSceneAABBPoints[i] = XMVector4Transform(XMLoadFloat4(&xmf4SceneAABBPoint), XMLoadFloat4x4(&pLight2->m_xmf4x4ViewFromLight));
		xmvAABBMin = XMVectorMin(pxmvLightSpaceSceneAABBPoints[i], xmvAABBMin);
		xmvAABBMax = XMVectorMax(pxmvLightSpaceSceneAABBPoints[i], xmvAABBMax);
	}
	XMFLOAT3 xmf3AABBMin;
	XMFLOAT3 xmf3AABBMax;
	XMStoreFloat3(&xmf3AABBMin, xmvAABBMin);
	XMStoreFloat3(&xmf3AABBMax, xmvAABBMax);

	XMStoreFloat4x4(&pLight2->m_xmf4x4ProjectionFromLight, XMMatrixOrthographicOffCenterLH(
			xmf3AABBMin.x,
			xmf3AABBMax.x,
			xmf3AABBMin.y,
			xmf3AABBMax.y,
			xmf3AABBMin.z,
			xmf3AABBMax.z
		)
	);

	//auto& [fMinX, fMaxX] = std::minmax_element(std::begin(xmf3SceneCorners), std::end(xmf3SceneCorners), [](const XMFLOAT3& lhs, const XMFLOAT3& rhs) { return lhs.x < rhs.x; });
	//auto& [fMinY, fMaxY] = std::minmax_element(std::begin(xmf3SceneCorners), std::end(xmf3SceneCorners), [](const XMFLOAT3& lhs, const XMFLOAT3& rhs) { return lhs.y < rhs.y; });
	//auto& [fMinZ, fMaxZ] = std::minmax_element(std::begin(xmf3SceneCorners), std::end(xmf3SceneCorners), [](const XMFLOAT3& lhs, const XMFLOAT3& rhs) { return lhs.z < rhs.z; });
	//
	//XMStoreFloat4x4(&pLight2->m_xmf4x4ProjectionFromLight, XMMatrixOrthographicLH(std::abs(fMaxX->x - fMinX->x), std::abs(fMaxY->y - fMinY->y), fMinZ->z, fMaxZ->z));
	//XMStoreFloat4x4(&pLight2->m_xmf4x4ProjectionFromLight, XMMatrixOrthographicOffCenterLH(fMinX->x, fMaxX->x, fMinY->y, fMaxY->y, fMinZ->z, fMaxZ->z));
	m_pLights.push_back(pLight2);

	//m_xmf4GlobalAmbient = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
}

void GameScene::BuildObjects(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	CreateRootSignature(pd3dDevice);
	Material::PrepareShaders(pd3dDevice, m_pd3dRootSignature);

	GameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dRootSignature, "../Models/SuperCobra.bin");
	GameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dRootSignature, "../Models/Tower.bin");
	GameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dRootSignature, "../Models/Tank.bin");

	std::shared_ptr<AirplanePlayer> pAirplanePlayer = std::make_shared<AirplanePlayer>(pd3dDevice, pd3dCommandList, m_pd3dRootSignature);
	std::shared_ptr<ThirdPersonCamera> pCamera = std::make_shared<ThirdPersonCamera>();

	pCamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	pCamera->SetTimeLag(0.25f);
	pCamera->SetOffset(XMFLOAT3(0.0f, 105.0f, -140.0f));
	pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
	pCamera->SetViewport(0, 0, GameFramework::g_nClientWidth, GameFramework::g_nClientHeight, 0.0f, 1.0f);
	pCamera->SetScissorRect(0, 0, GameFramework::g_nClientWidth, GameFramework::g_nClientHeight);
	pCamera->SetPlayer(pAirplanePlayer);
	pAirplanePlayer->SetCamera(pCamera);

	auto pSuperCobra = RESOURCE->CopyGameObject("SuperCobra");
	pSuperCobra->SetScale(10.0f, 10.0f, 10.0f);
	pAirplanePlayer->SetChild(pSuperCobra);

	m_pPlayer = pAirplanePlayer;
	m_pPlayer->Initialize();

	m_pPlayer->SetFriction(20.f);
	m_pPlayer->SetGravity(XMFLOAT3(0.0f, 0.0f, 0.0f));
	m_pPlayer->SetMaxVelocityXZ(500.f);
	m_pPlayer->SetMaxVelocityY(300.0f);
	m_pPlayer->SetPosition(XMFLOAT3(3500.f, 1000.0f, 3500.0f));

	XMFLOAT3 xmf3Scale(18.0f, 4.0f, 18.0f);
	XMFLOAT4 xmf4Color(0.0f, 0.5f, 0.0f, 0.0f);
	m_pTerrain = std::make_shared<TerrainObject>();

#ifdef TERRAIN_TESSELLATION
	m_pTerrain->Initialize(pd3dDevice, pd3dCommandList, "../Models/Textures/Terrain/HeightMap.raw", 257, 257, 13, 13, xmf3Scale, xmf4Color);
#else
	m_pTerrain->Initialize(pd3dDevice, pd3dCommandList, "../Models/Textures/Terrain/HeightMap.raw", 257, 257, 257, 257, xmf3Scale, xmf4Color);
#endif
	m_pPlayer->SetPosition(XMFLOAT3(m_pTerrain->GetWidth() / 2, 2000.0f, m_pTerrain->GetLength() / 2));

	m_pSkyboxTexture = TEXTURE->GetTexture("Skybox");

	// 오브젝트 배치
	{
		float fTerrainWidth = m_pTerrain->GetWidth();
		float fTerrainLength = m_pTerrain->GetLength();

		// Tower
		{
			auto pTower = RESOURCE->CopyGameObject("Tower");
			pTower->SetScale(20, 20, 20);
			std::shared_ptr<GameObject> pTowerObject = std::make_shared<GameObject>();
			pTowerObject->SetChild(pTower);
			//pTowerObject->Initialize();
			pTowerObject->UpdateTransform(nullptr);
			pTowerObject->GenerateBigBoundingBox(true, true);
		
			for (int i = 0; i < 10; i++) {
				auto pCopied = GameObject::CopyObject(*pTowerObject);
		
				float fPosX = 0.f;
				float fPosZ = 0.f;
				while (true) {
					fPosX = RandomGenerator::GenerateRandomFloatInRange(1.f, fTerrainWidth) - 1;
					fPosZ = RandomGenerator::GenerateRandomFloatInRange(1.f, fTerrainLength) - 1;
					float fHeight = m_pTerrain->GetHeight(fPosX, fPosZ);
					if (fHeight > m_pTerrain->GetWaterHeight()) break;
				}
		
				pCopied->SetPosition(XMFLOAT3(fPosX, 0.f, fPosZ));
				m_pGameObjects.push_back(pCopied);
			}
		}
		
		// Tank
		{
			auto pTank = RESOURCE->CopyGameObject("Tank");
			
			pTank->SetScale(50, 50, 50);
			std::shared_ptr<GameObject> pTankObject = std::make_shared<GameObject>();
			pTankObject->SetName("Tank");
			pTankObject->SetChild(pTank);
			pTankObject->Initialize();
			
		
			XMFLOAT3 xmf3RotationAxis = XMFLOAT3(0.f, 1.f, 0.f);
		
			for (int i = 0; i < 10; i++) {
				auto pCopied = GameObject::CopyObject(*pTankObject);
		
				float fPosX = 0.f;
				float fPosZ = 0.f;
				float fRotationY = 0.f;
				while (true) {
					fPosX = RandomGenerator::GenerateRandomFloatInRange(1.f, fTerrainWidth) - 1;
					fPosZ = RandomGenerator::GenerateRandomFloatInRange(1.f, fTerrainLength) - 1;
					float fHeight = m_pTerrain->GetHeight(fPosX, fPosZ);
					if (fHeight > m_pTerrain->GetWaterHeight()) break;
				}
		
				pCopied->SetPosition(XMFLOAT3(fPosX, 0.f, fPosZ));
				pCopied->Rotate(&xmf3RotationAxis, RandomGenerator::GenerateRandomFloatInRange(0.f, 360.f));
				pCopied->SetExplosible(true);
		
				m_pGameObjects.push_back(pCopied);

				// 빌보드 오브젝트 미리 만듬
				// pCopied 의 것이 아니라 그냥 10개를 미리 만드는 것이고, 나중에 위치를 바꿔서 UIManager 에 넣어줌
				m_pBillboardSprites[i] = std::make_shared<BillboardSprite>(pd3dDevice, pd3dCommandList, "indicator", XMFLOAT3(0.f, 0.f, 0.f), XMFLOAT2(80.f, 80.f));
			}
		}

		// Building
		{
			float fWidth = 350.f;
			float fLength = 300.f;
			float fHeight = 1500.f;

			std::shared_ptr<BuildingObject> pBuildingObject = std::make_shared<BuildingObject>();
			pBuildingObject->Initialize(pd3dDevice, pd3dCommandList, fWidth, fLength, fHeight, 20, 63);
			pBuildingObject->SetPosition(XMFLOAT3(3900.f, 0.f, 3900.f));
			m_pGameObjects.push_back(pBuildingObject);
		}
	}

	for (auto pObj : m_pGameObjects) {
		pObj->AdjustHeightFromTerrain(m_pTerrain);
	}

	m_pHPTextSprite = std::make_shared<TextSprite>("", 0.0f, 0.0f, 0.3f, 0.05f, XMFLOAT4(1, 0, 0, 1), 1, true);
	m_pSprites.push_back(m_pHPTextSprite);

	GenerateSceneBoundingBox();

	BuildDefaultLightsAndMaterials(pd3dDevice, pd3dCommandList);
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void GameScene::ReleaseUploadBuffers()
{
	Scene::ReleaseUploadBuffers();
}

bool GameScene::ProcessInput(UCHAR* pKeysBuffer)
{
	DWORD dwDirection = 0;
	if (pKeysBuffer['W'] & 0xF0)	dwDirection |= MOVE_DIR_FORWARD;
	if (pKeysBuffer['S'] & 0xF0)	dwDirection |= MOVE_DIR_BACKWARD;
	if (pKeysBuffer['A'] & 0xF0)	dwDirection |= MOVE_DIR_LEFT;
	if (pKeysBuffer['D'] & 0xF0)	dwDirection |= MOVE_DIR_RIGHT;
	if (pKeysBuffer['E'] & 0xF0)	dwDirection |= MOVE_DIR_UP;
	if (pKeysBuffer['Q'] & 0xF0)	dwDirection |= MOVE_DIR_DOWN;

	float cxDelta = 0.0f, cyDelta = 0.0f;
	POINT ptCursorPos;
	if (GetCapture() == GameFramework::g_hWnd) {
		SetCursor(NULL);
		GetCursorPos(&ptCursorPos);
		cxDelta = (float)(ptCursorPos.x - m_ptOldCursorPos.x) / 3.0f;
		cyDelta = (float)(ptCursorPos.y - m_ptOldCursorPos.y) / 3.0f;
		SetCursorPos(m_ptOldCursorPos.x, m_ptOldCursorPos.y);
	}

	if ((dwDirection != 0) || (cxDelta != 0.0f) || (cyDelta != 0.0f)) {
		if (cxDelta || cyDelta) {
			if (pKeysBuffer[VK_RBUTTON] & 0xF0)
				m_pPlayer->Rotate(cyDelta, 0.0f, -cxDelta);
			else
				m_pPlayer->Rotate(cyDelta, cxDelta, 0.0f);
		}
		if (dwDirection) {
			m_pPlayer->Move(dwDirection, 1.5f, true);
		}
	}

	if (pKeysBuffer[VK_RBUTTON] & 0xF0) {
		POINT ptCursorClicked{};
		::GetCursorPos(&ptCursorClicked);
		::ScreenToClient(GameFramework::g_hWnd, &ptCursorClicked);
		auto p = PickObjectPointedByCursor(ptCursorClicked.x, ptCursorClicked.y, GetCamera());
		if (p) {
			if (p->IsExplosible()) {
				XMFLOAT3 xmf3Pos = p->GetPosition();
				EffectParameter param;
				param.xmf3Position = xmf3Pos;
				param.xmf3Force = XMFLOAT3(0, RandomGenerator::GenerateRandomFloatInRange(0.f, 5.f), 0);
				param.fElapsedTime = 0.f;

				EFFECT->AddEffect<ExplosionEffect>(param);
				std::erase(m_pGameObjects, p);
			}
		}
	}

	return true;
}

void GameScene::Update(float fTimeElapsed)
{
	if (m_pLights.size() != 0)
	{
		std::shared_ptr<SpotLight> pSpotLight = static_pointer_cast<SpotLight>(m_pLights[0]);
		pSpotLight->m_xmf3Position = m_pPlayer->GetPosition();
		pSpotLight->m_xmf3Direction = m_pPlayer->GetLookVector();
	}
	
	m_pPlayer->AdjustHeightFromTerrain(m_pTerrain);

	for (auto pObj : m_pGameObjects) {
		pObj->AdjustHeightFromTerrain(m_pTerrain);
	}

	XMFLOAT3 xmf3RotationAxis = XMFLOAT3(0.f, 0.f, 1.f);
	//m_pGameObjects[0]->Rotate(&xmf3RotationAxis, 50.f * fTimeElapsed);

	m_pHPTextSprite->SetText(std::format("HP:{}", (int)m_pPlayer->GetHP()));

	Scene::CheckCollision();
	Scene::Update(fTimeElapsed);
}

void GameScene::Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	int nTank = 0;
	for (auto pObj : m_pGameObjects) {
		if (pObj->GetName() == "Tank") {
			XMFLOAT3 xmf3TankPosition = pObj->GetPosition();
			XMFLOAT3 xmf3Up = XMFLOAT3(0.f, 1.f, 0.f);
			XMFLOAT3 xmf3TankRight = pObj->GetRight();
			float fBillboardHeight = 150.f;

			XMFLOAT3 xmf3BillboardPos;
			XMStoreFloat3(&xmf3BillboardPos, XMVectorAdd(XMVectorAdd(XMLoadFloat3(&xmf3TankPosition), XMLoadFloat3(&xmf3Up) * fBillboardHeight), XMLoadFloat3(&xmf3TankRight) * 30));

			m_pBillboardSprites[nTank]->SetPosition(xmf3BillboardPos);
			m_pBillboardSprites[nTank]->AddToUI(0);
			nTank++;
		}
	}

	Scene::Render(pd3dDevice, pd3dCommandList);
}

void GameScene::CreateShaderVariables(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	Scene::CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void GameScene::UpdateShaderVariable(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	Scene::UpdateShaderVariable(pd3dCommandList);
}

void GameScene::CreateRootSignature(ComPtr<ID3D12Device> pd3dDevice)
{
	Scene::CreateRootSignature(pd3dDevice);
}

bool GameScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID) {
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		::SetCapture(hWnd);
		::GetCursorPos(&m_ptOldCursorPos);
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		::ReleaseCapture();
		break;
	case WM_MOUSEMOVE:
		break;
	default:
		break;
	}

	return true;
}

bool GameScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	return false;
}
