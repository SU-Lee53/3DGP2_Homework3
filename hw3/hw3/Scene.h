#pragma once
#include "Light.h"
#include "Player.h"
#include "Sprite.h"

#define MAX_LIGHTS 16

class TerrainObject;

struct CB_LIGHT_DATA {
	LightData	LightData[MAX_LIGHTS];
	XMFLOAT4		globalAmbientLight;
	int				nLights;
};


class Scene {
public:
	Scene();

public:
	virtual void BuildDefaultLightsAndMaterials();
	virtual void BuildObjects(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	virtual void ReleaseUploadBuffers();

	virtual bool ProcessInput(UCHAR* pKeysBuffer);
	virtual void Update(float fTimeElapsed);
	virtual void Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	virtual void RenderDebug(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

	virtual void CreateShaderVariables(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	virtual void UpdateShaderVariable(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

	CB_LIGHT_DATA GetLightCBData();

protected:
	virtual void CreateRootSignature(ComPtr<ID3D12Device> pd3dDevice);

public:
	virtual bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	virtual bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	const ConstantBuffer& GetLightCBuffer() const { return m_LightCBuffer; }

public:
	std::shared_ptr<Sprite> CheckButtonClicked();

	const std::shared_ptr<TerrainObject>& GetTerrain() { return m_pTerrain; }

public:
	void CheckCollision();
	std::shared_ptr<GameObject> PickObjectPointedByCursor(int xClient, int yClient, std::shared_ptr<Camera> pCamera);

public:
	std::shared_ptr<Camera> GetCamera() const;
	const ConstantBuffer& GetCBuffer() const { return m_LightCBuffer; }
	std::shared_ptr<Player> GetPlayer() const;
	const std::vector<std::shared_ptr<GameObject>>& GetGameObjects() const;

	std::shared_ptr<Texture> GetSkyboxTexture() { return m_pSkyboxTexture; }

	void SetTerrainWireframeMode(bool bMode);

protected:
	std::shared_ptr<Player>						m_pPlayer;
	std::vector<std::shared_ptr<GameObject>>	m_pGameObjects;
	std::vector<std::shared_ptr<Light>>			m_pLights;
	std::vector<std::shared_ptr<Sprite>>		m_pSprites;

	std::shared_ptr<TerrainObject>				m_pTerrain;
	// TODO : Add Skybox
	std::shared_ptr<Texture>					m_pSkyboxTexture;

	XMFLOAT4									m_xmf4GlobalAmbient;

	ConstantBuffer								m_LightCBuffer;

protected:
	ComPtr<ID3D12RootSignature> m_pd3dRootSignature;

public:
	const static UINT g_uiDescriptorCountPerScene = 2;	// Camera + Lights
};

