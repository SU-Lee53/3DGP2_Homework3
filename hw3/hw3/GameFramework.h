#pragma once
#include "GameTimer.h"
#include "Scene.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "UIManager.h"
#include "TextureManager.h"
#include "ShaderManager.h"
#include "EffectManager.h"

class Scene;

class FrameBuffer {
public:
	void CreateDescriptorHeaps(ComPtr<ID3D12Device> pd3dDevice, UINT nSwapChainBuffers);
	void CreateViews(ComPtr<ID3D12Device> pd3dDevice, ComPtr<IDXGISwapChain> pdxgiSwapChain, UINT nSwapChainBuffers);
	void ResetBackBuffers(UINT nSwapChainBuffers) {
		for (int i = 0; i < nSwapChainBuffers; i++)
			if (m_pSwapChainBackBuffers[i])
				m_pSwapChainBackBuffers[i].Reset();
	}

	ComPtr<ID3D12Resource> GetBackBuffer(UINT nBufferIndex) {
		return m_pSwapChainBackBuffers[nBufferIndex];
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferCPUHandle(UINT nBackBufferIndex) {
		D3D12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUHandle = m_pd3dRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		d3dRTVCPUHandle.ptr += nBackBufferIndex * m_nRtvDescriptorIncrementSize;
		return d3dRTVCPUHandle;
	}

private:
	ComPtr<ID3D12Resource>				m_pSwapChainBackBuffers[2];
	ComPtr<ID3D12DescriptorHeap>		m_pd3dRTVDescriptorHeap = NULL;
	ComPtr<ID3D12DescriptorHeap>		m_pd3dSRVUAVDescriptorHeap = NULL;

public:
	UINT m_nRtvDescriptorIncrementSize;

};

class GameFramework {
public:
	GameFramework(HINSTANCE hInstance, HWND hWnd, UINT uiWidth, UINT uiHeight, bool bEnableDebugLayer);
	~GameFramework();

public:
	void BuildObjects();
	void ProcessInput();
	void Update();
	void Render();

public:
	static void ChangeScene(UINT nSceneIndex);

private:
	void RenderBegin();
	void RenderEnd();
	void Present();
	void MoveToNextFrame();

public:
	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

public:
	static std::vector<std::shared_ptr<Scene>>	g_pScenes;
	static std::shared_ptr<Scene> g_pCurrentScene;
	static bool g_bSceneChanged;

public:
	GameTimer				m_GameTimer{};

	POINT					m_ptOldCursorPos;
	TSTRING					m_tstrFrameRate;

	// Resources
	static std::unique_ptr<ResourceManager>		g_pResourceManager;
	static std::unique_ptr<TextureManager>		g_pTextureManager;
	static std::unique_ptr<ShaderManager>		g_pShaderManager;
	
	// Rendering
	static std::unique_ptr<RenderManager>		g_pRenderManager;
	static std::unique_ptr<UIManager>			g_pUIManager;
	static std::unique_ptr<EffectManager>		g_pEffectManager;

#pragma region D3D
private:
	void CreateFactory();
	void CreateDevice();
	void CreateFence();
	void CreateSwapChain();
	void CreateCommandList();
	void CreateRtvAndDsvDescriptorHeaps();
	void CreateRenderTargetViews();
	void CreateDepthStencilView();



private:
	void WaitForGPUComplete();

private:
	void ChangeSwapChainState();

public:
	static HINSTANCE g_hInstance;
	static HWND g_hWnd;

	static bool g_bMsaa4xEnable;
	static UINT g_nMsaa4xQualityLevels;

	static long g_nClientWidth;
	static long g_nClientHeight;

	static UINT g_uiDescriptorHandleIncrementSize;

	const static UINT g_nSwapChainBuffers = 2;

private:

	// DXGI
	ComPtr<IDXGIFactory4>	m_pdxgiFactory;
	ComPtr<IDXGISwapChain3>	m_pdxgiSwapChain;
	UINT					m_nSwapChainBufferIndex = 0;

	// Device
	ComPtr<ID3D12Device>				m_pd3dDevice;

	// RTV + DSV
	static const UINT					m_nSwapChainBuffers = 2;

	//ComPtr<ID3D12Resource>			m_ppd3dSwapChainBackBuffers[m_nSwapChainBuffers];
	//ComPtr<ID3D12DescriptorHeap>		m_pd3dRtvDescriptorHeap = NULL;
	//UINT								m_nRtvDescriptorIncrementSize;

	FrameBuffer						m_FrameBuffers;

	ComPtr<ID3D12Resource>				m_pd3dDepthStencilBuffer = NULL;
	ComPtr<ID3D12DescriptorHeap>		m_pd3dDsvDescriptorHeap = NULL;
	UINT								m_nDsvDescriptorIncrementSize;

	// Command List + Allocator
	ComPtr<ID3D12CommandQueue>			m_pd3dCommandQueue;
	ComPtr<ID3D12GraphicsCommandList>	m_pd3dCommandList;
	ComPtr<ID3D12CommandAllocator>		m_pd3dCommandAllocator;

	// Fence
	ComPtr<ID3D12Fence>			m_pd3dFence;
	HANDLE						m_hFenceEvent;

	D3D12_VIEWPORT				m_d3dViewport;
	UINT64						m_nFenceValues[m_nSwapChainBuffers];
	D3D12_RECT					m_d3dScissorRect;

	bool m_bEnableDebugLayer = false;
#pragma endregion
};


#define RENDER		GameFramework::g_pRenderManager
#define UI			GameFramework::g_pUIManager
#define EFFECT		GameFramework::g_pEffectManager

#define RESOURCE	GameFramework::g_pResourceManager
#define TEXTURE		GameFramework::g_pTextureManager
#define SHADER		GameFramework::g_pShaderManager

#define CUR_SCENE	GameFramework::g_pCurrentScene