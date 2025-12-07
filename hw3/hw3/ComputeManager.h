#pragma once


class ComputeProgram {
public:
	virtual void CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature) = 0;
	virtual void UpdateShaderVariables(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle) = 0;

	virtual void Dispatch(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT xThread, UINT yThreads, UINT zThreads);

public:
	void SetInputHandle(D3D12_CPU_DESCRIPTOR_HANDLE inputHandle);
	void SetOutputHandle(D3D12_CPU_DESCRIPTOR_HANDLE outputHandle);


protected:
	ComPtr<ID3D12PipelineState> m_pd3dPipelineState = nullptr;

private:
	D3D12_CPU_DESCRIPTOR_HANDLE		m_d3dInputSRVCPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE		m_d3dOutputUAVCPUHandle;
};

class HorizentalBlur : public ComputeProgram {
public:
	virtual void CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature) override;
	virtual void UpdateShaderVariables(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle) override;

	virtual void Dispatch(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT xThread, UINT yThreads, UINT zThreads);
};

class VerticalBlur : public ComputeProgram {
public:
	virtual void CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature) override;
	virtual void UpdateShaderVariables(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle) override;

	virtual void Dispatch(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT xThread, UINT yThreads, UINT zThreads);
};

class ComputeManager {
public:
	ComputeManager(ComPtr<ID3D12Device> pd3dDevice);

	void SetBackBufferHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);
	void Dispatch(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	void CopyUAVToBackBuffer(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

private:
	void CreateComputeRootSignature();
	void LoadComputePrograms();

private:
	void CreateGraphicsRootSignature();
	void CreateGraphcisPipelineState();

private:
	// Compute
	ComPtr<ID3D12RootSignature>		m_pd3dComputeRootSignature = nullptr;
	ComPtr<ID3D12Device>			m_pd3dDevice = nullptr;	// GameFramewok::m_pd3dDevice ÀÇ ÂüÁ¶
	ComPtr<ID3D12DescriptorHeap>	m_pd3dDescriptorHeap = nullptr;

	// Graphics
	ComPtr<ID3D12PipelineState>		m_pd3dUAVToBackBufferRootSignature = nullptr;
	ComPtr<ID3D12PipelineState>		m_pd3dUAVToBackBufferPipelineState = nullptr;

	// UAV texture
	std::shared_ptr<Texture>		m_pUAVTextures[2];

	// BackBuffer
	ComPtr<ID3D12Resource>			m_pd3dCurrentBackBufferResource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE		m_d3dBackBufferRTVCPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE		m_d3dBackBufferSRVCPUHandle;

	std::unordered_map<std::type_index, std::unique_ptr<ComputeProgram>> m_pComputePrograms;
};

