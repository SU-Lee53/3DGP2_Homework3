#pragma once
#include "Scene.h"
class GameScene : public Scene {
public:
	GameScene();

public:
	virtual void BuildDefaultLightsAndMaterials() override;
	virtual void BuildObjects(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) override;
	virtual void ReleaseUploadBuffers()override;

	virtual bool ProcessInput(UCHAR* pKeysBuffer) override;
	virtual void Update(float fTimeElapsed) override;
	virtual void Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) override;

	virtual void CreateShaderVariables(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) override;
	virtual void UpdateShaderVariable(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) override;

private:
	virtual void CreateRootSignature(ComPtr<ID3D12Device> pd3dDevice) override;

public:
	virtual bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;
	virtual bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;

private:
	POINT							m_ptOldCursorPos;
	std::shared_ptr<TextSprite>		m_pHPTextSprite;

	// 미리 만든다
	std::shared_ptr<BillboardSprite> m_pBillboardSprites[10];

};

