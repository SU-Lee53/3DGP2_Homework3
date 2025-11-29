#include "stdafx.h"
#include "IntroScene.h"

IntroScene::IntroScene()
{
}

void IntroScene::BuildDefaultLightsAndMaterials()
{
}

void IntroScene::BuildObjects(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	CreateRootSignature(pd3dDevice);
	Material::PrepareShaders(pd3dDevice, m_pd3dRootSignature);
	BuildDefaultLightsAndMaterials();

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

	m_pPlayer = pAirplanePlayer;
	m_pPlayer->Initialize();

	std::shared_ptr<TexturedSprite> pTextureSprite = std::make_shared<TexturedSprite>("intro", 0.f, 0.f, 1.0f, 1.0f);
	std::shared_ptr<TextSprite> pTextSprite1 = std::make_shared<TextSprite>("3D GAME PROGRAMMING 2", 0.0f, 0.0f, 1.f, 0.15f, XMFLOAT4(1, 0, 0, 1), 1, false);
	std::shared_ptr<TextSprite> pTextSprite2 = std::make_shared<TextSprite>("START", 0.1f, 0.85f, 0.4f, 0.95f, XMFLOAT4(1, 0, 0, 1), 1, true);
	std::shared_ptr<TextSprite> pTextSprite3 = std::make_shared<TextSprite>("EXIT", 0.6f, 0.85f, 0.85f, 0.95f, XMFLOAT4(1, 0, 0, 1), 1, true);

	m_pSprites.push_back(pTextureSprite);
	m_pSprites.push_back(pTextSprite1);
	m_pSprites.push_back(pTextSprite2);
	m_pSprites.push_back(pTextSprite3);

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void IntroScene::ReleaseUploadBuffers()
{
	Scene::ReleaseUploadBuffers();
}

bool IntroScene::ProcessInput(UCHAR* pKeysBuffer)
{
	if (pKeysBuffer[VK_LBUTTON] & 0xF0) {
		auto pClickedSprite = CheckButtonClicked();
		if (pClickedSprite == m_pSprites[2]) {
			GameFramework::ChangeScene(1);
		}
		
		if (pClickedSprite == m_pSprites[3]) {
			PostQuitMessage(0);
		}

	}

	return true;
}

void IntroScene::Update(float fTimeElapsed)
{
	Scene::Update(fTimeElapsed);
}

void IntroScene::Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	Scene::Render(pd3dDevice, pd3dCommandList);
}

void IntroScene::CreateShaderVariables(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	Scene::CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void IntroScene::UpdateShaderVariable(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	Scene::UpdateShaderVariable(pd3dCommandList);
}

void IntroScene::CreateRootSignature(ComPtr<ID3D12Device> pd3dDevice)
{
	Scene::CreateRootSignature(pd3dDevice);
}

bool IntroScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	return false;
}

bool IntroScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	return false;
}
