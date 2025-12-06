#pragma once


class ComputeProgram {
public:
	virtual void CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice) = 0;
	virtual void UpdateShaderVariables() = 0;

	virtual void Dispatch(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	virtual void Dispatch(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT xThread, UINT yThreads, UINT zThreads);


protected:
	ComPtr<ID3D12PipelineState> m_pd3dPipelineState = nullptr;

	UINT m_xThreads = 0;
	UINT m_yThreads = 0;
	UINT m_zThreads = 0;

};

class MotionBlur : public ComputeProgram {
public:
	void CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice) override;
	void UpdateShaderVariables() override;

};



class ComputeManager {
public:
	ComputeManager(ComPtr<ID3D12Device> pd3dDevice);

	void Run(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	//void Clear();

private:
	void CreateRootSignature();

private:
	ComPtr<ID3D12RootSignature>		m_pd3dComputeRootSignature = nullptr;
	ComPtr<ID3D12Device>			m_pd3dDevice = nullptr;	// GameFramewok::m_pd3dDevice ÀÇ ÂüÁ¶
	ComPtr<ID3D12DescriptorHeap>	m_pd3dDescriptorHeap = nullptr;

};

