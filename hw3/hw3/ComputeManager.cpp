#include "stdafx.h"
#include "ComputeManager.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ComputeProgram

void ComputeProgram::Dispatch(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	pd3dCommandList->SetPipelineState(m_pd3dPipelineState.Get());
	pd3dCommandList->Dispatch(m_xThreads, m_yThreads, m_zThreads);
}

void ComputeProgram::Dispatch(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT xThread, UINT yThreads, UINT zThreads)
{
	pd3dCommandList->SetPipelineState(m_pd3dPipelineState.Get());
	pd3dCommandList->Dispatch(xThread, yThreads, zThreads);
}

void ComputeProgram::SetInputHandle(D3D12_CPU_DESCRIPTOR_HANDLE inputHandle)
{
	m_d3dInputSRVCPUHandle = inputHandle;
}

void ComputeProgram::SetOutputHandle(D3D12_CPU_DESCRIPTOR_HANDLE outputHandle)
{
	m_d3dOutputUAVCPUHandle = outputHandle;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HorizentalBlur

void HorizentalBlur::CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature)
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = pd3dRootSignature.Get();
		d3dPipelineDesc.CS = SHADER->GetShaderByteCode("HorzBlurCS");
		d3dPipelineDesc.NodeMask = 0;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	pd3dDevice->CreateComputePipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineState.GetAddressOf()));
}

void HorizentalBlur::UpdateShaderVariables(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VerticalBlur

void VerticalBlur::CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature)
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = pd3dRootSignature.Get();
		d3dPipelineDesc.CS = SHADER->GetShaderByteCode("VertBlurCS");
		d3dPipelineDesc.NodeMask = 0;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	pd3dDevice->CreateComputePipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineState.GetAddressOf()));
}

void VerticalBlur::UpdateShaderVariables(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ComputeManager

ComputeManager::ComputeManager(ComPtr<ID3D12Device> pd3dDevice)
{
	m_pd3dDevice = pd3dDevice;

	D3D12_DESCRIPTOR_HEAP_DESC d3dHeapDesc;
	{
		d3dHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		d3dHeapDesc.NumDescriptors = 4;
		d3dHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		d3dHeapDesc.NodeMask = 0;
	}

	pd3dDevice->CreateDescriptorHeap(&d3dHeapDesc, IID_PPV_ARGS(m_pd3dDescriptorHeap.GetAddressOf()));

	CreateComputeRootSignature();

	m_pUAVTextures[0] = TEXTURE->GetTexture("RWTexture1");
	m_pUAVTextures[1] = TEXTURE->GetTexture("RWTexture2");
}

void ComputeManager::SetBackBufferHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
	m_d3dBackBufferSRVCPUHandle = cpuHandle;
}

void ComputeManager::Dispatch(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	DescriptorHandle descHandle = {
		m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		m_pd3dDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
	};

	pd3dCommandList->SetComputeRootDescriptorTable(0, descHandle.gpuHandle);
	pd3dCommandList->SetDescriptorHeaps(1, m_pd3dDescriptorHeap.GetAddressOf());

	// Descriptor Table 구성
	// Horz
	m_pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, m_d3dBackBufferSRVCPUHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.cpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;
	
	m_pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, m_pUAVTextures[0]->GetUAVCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.cpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;
	
	// Vert
	m_pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, m_pUAVTextures[0]->GetSRVCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.cpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;
	
	m_pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, m_pUAVTextures[1]->GetUAVCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.cpuHandle.ptr += GameFramework::g_uiDescriptorHandleIncrementSize;

	auto& pHorzBlur = m_pComputePrograms[typeid(HorizentalBlur)];
	auto& pVertBlur = m_pComputePrograms[typeid(VerticalBlur)];

	pHorzBlur->SetInputHandle(m_d3dBackBufferSRVCPUHandle);
	pHorzBlur->SetOutputHandle(m_pUAVTextures[0]->GetUAVCPUHandle());

	pVertBlur->SetInputHandle(m_pUAVTextures[0]->GetUAVCPUHandle());
	pVertBlur->SetOutputHandle(m_pUAVTextures[1]->GetUAVCPUHandle());

	UINT nClientWidth = GameFramework::g_nClientWidth;
	UINT nClientHeight = GameFramework::g_nClientHeight;

	CD3DX12_RESOURCE_BARRIER resourceBarriers[2];
	resourceBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_pUAVTextures[0]->GetTexResource().Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_FLAG_NONE);
	resourceBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_pUAVTextures[1]->GetTexResource().Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_FLAG_NONE);
	pd3dCommandList->ResourceBarrier(2, resourceBarriers);

	// Horizental Blur 수행
	UINT nThreadX = std::ceil((float)nClientWidth / 256.f);
	UINT nThreadY = nClientHeight;
	UINT nThreadZ = 1;
	pd3dCommandList->SetComputeRootDescriptorTable(0, descHandle.gpuHandle);
	pHorzBlur->Dispatch(pd3dCommandList, nThreadX, nThreadY, nThreadZ);
	descHandle.gpuHandle.ptr += 2 * GameFramework::g_uiDescriptorHandleIncrementSize;


	pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pUAVTextures[0]->GetTexResource().Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_BARRIER_FLAG_NONE));

	// Vertical Blur 수행
	pd3dCommandList->SetComputeRootDescriptorTable(0, descHandle.gpuHandle);
	nThreadX = nClientWidth;
	nThreadY = std::ceil((float)nClientHeight / 256.f);
	nThreadZ = 1;
	pVertBlur->Dispatch(pd3dCommandList, nThreadX, nThreadY, nThreadZ);

	pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pUAVTextures[1]->GetTexResource().Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_BARRIER_FLAG_NONE));

	CopyUAVToBackBuffer(pd3dCommandList);

}

void ComputeManager::LoadComputePrograms()
{
	std::unique_ptr<HorizentalBlur> pHorzBlur = std::make_unique<HorizentalBlur>();
	std::unique_ptr<VerticalBlur> pVertBlur = std::make_unique<VerticalBlur>();
	pHorzBlur->CreatePipelineState(m_pd3dDevice, m_pd3dComputeRootSignature);
	pVertBlur->CreatePipelineState(m_pd3dDevice, m_pd3dComputeRootSignature);

	m_pComputePrograms.insert({ typeid(HorizentalBlur), std::move(pHorzBlur) });
	m_pComputePrograms.insert({ typeid(VerticalBlur), std::move(pVertBlur) });

}

void ComputeManager::CreateComputeRootSignature()
{
	D3D12_DESCRIPTOR_RANGE d3dDescriptorRanges[2];

	// Input : SRV
	d3dDescriptorRanges[0].NumDescriptors = 1;
	d3dDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;	// t0
	d3dDescriptorRanges[0].BaseShaderRegister = 0;
	d3dDescriptorRanges[0].RegisterSpace = 0;
	d3dDescriptorRanges[0].OffsetInDescriptorsFromTableStart = 0;
	
	// Output : UAV
	d3dDescriptorRanges[1].NumDescriptors = 1;
	d3dDescriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;	// u0
	d3dDescriptorRanges[1].BaseShaderRegister = 0;
	d3dDescriptorRanges[1].RegisterSpace = 0;
	d3dDescriptorRanges[1].OffsetInDescriptorsFromTableStart = 1;

	D3D12_ROOT_PARAMETER d3dRootParameters[1];
	d3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	d3dRootParameters[0].DescriptorTable.NumDescriptorRanges = 2;
	d3dRootParameters[0].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[0];
	d3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

	// Point, Linear 필터 2개 준비
	D3D12_STATIC_SAMPLER_DESC d3dSamplerDescs[2];
	d3dSamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	d3dSamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDescs[0].MipLODBias = 0;
	d3dSamplerDescs[0].MaxAnisotropy = 1;
	d3dSamplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	d3dSamplerDescs[0].MinLOD = 0;
	d3dSamplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDescs[0].ShaderRegister = 0;
	d3dSamplerDescs[0].RegisterSpace = 0;
	d3dSamplerDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	
	d3dSamplerDescs[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dSamplerDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDescs[1].MipLODBias = 0;
	d3dSamplerDescs[1].MaxAnisotropy = 1;
	d3dSamplerDescs[1].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	d3dSamplerDescs[1].MinLOD = 0;
	d3dSamplerDescs[1].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDescs[1].ShaderRegister = 0;
	d3dSamplerDescs[1].RegisterSpace = 0;
	d3dSamplerDescs[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc{};
	{
		d3dRootSignatureDesc.NumParameters = _countof(d3dRootParameters);
		d3dRootSignatureDesc.pParameters = d3dRootParameters;
		d3dRootSignatureDesc.NumStaticSamplers = _countof(d3dSamplerDescs);
		d3dRootSignatureDesc.pStaticSamplers = d3dSamplerDescs;
		d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;
	}

	ComPtr<ID3DBlob> pd3dSignatureBlob = nullptr;
	ComPtr<ID3DBlob> pd3dErrorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pd3dSignatureBlob.GetAddressOf(), pd3dErrorBlob.GetAddressOf());
	if (FAILED(hr)) {
		char* pErrorString = (char*)pd3dErrorBlob->GetBufferPointer();
		HWND hWnd = ::GetActiveWindow();
		MessageBoxA(hWnd, pErrorString, NULL, 0);
		OutputDebugStringA(pErrorString);
		__debugbreak();
	}

	m_pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), IID_PPV_ARGS(m_pd3dComputeRootSignature.GetAddressOf()));
}

void ComputeManager::CopyUAVToBackBuffer(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
}

void ComputeManager::CreateGraphicsRootSignature()
{
	D3D12_DESCRIPTOR_RANGE d3dDescriptorRanges[2];

	// Input : SRV
	d3dDescriptorRanges[0].NumDescriptors = 1;
	d3dDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;	// t0
	d3dDescriptorRanges[0].BaseShaderRegister = 0;
	d3dDescriptorRanges[0].RegisterSpace = 0;
	d3dDescriptorRanges[0].OffsetInDescriptorsFromTableStart = 0;

	D3D12_ROOT_PARAMETER d3dRootParameters[1];
	d3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	d3dRootParameters[0].DescriptorTable.NumDescriptorRanges = 2;
	d3dRootParameters[0].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[0];
	d3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

	// Point, Linear 필터 2개 준비
	D3D12_STATIC_SAMPLER_DESC d3dSamplerDescs[2];
	d3dSamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	d3dSamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDescs[0].MipLODBias = 0;
	d3dSamplerDescs[0].MaxAnisotropy = 1;
	d3dSamplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	d3dSamplerDescs[0].MinLOD = 0;
	d3dSamplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDescs[0].ShaderRegister = 0;
	d3dSamplerDescs[0].RegisterSpace = 0;
	d3dSamplerDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	d3dSamplerDescs[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dSamplerDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDescs[1].MipLODBias = 0;
	d3dSamplerDescs[1].MaxAnisotropy = 1;
	d3dSamplerDescs[1].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	d3dSamplerDescs[1].MinLOD = 0;
	d3dSamplerDescs[1].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDescs[1].ShaderRegister = 0;
	d3dSamplerDescs[1].RegisterSpace = 0;
	d3dSamplerDescs[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc{};
	{
		d3dRootSignatureDesc.NumParameters = _countof(d3dRootParameters);
		d3dRootSignatureDesc.pParameters = d3dRootParameters;
		d3dRootSignatureDesc.NumStaticSamplers = _countof(d3dSamplerDescs);
		d3dRootSignatureDesc.pStaticSamplers = d3dSamplerDescs;
		d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;
	}

	ComPtr<ID3DBlob> pd3dSignatureBlob = nullptr;
	ComPtr<ID3DBlob> pd3dErrorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pd3dSignatureBlob.GetAddressOf(), pd3dErrorBlob.GetAddressOf());
	if (FAILED(hr)) {
		char* pErrorString = (char*)pd3dErrorBlob->GetBufferPointer();
		HWND hWnd = ::GetActiveWindow();
		MessageBoxA(hWnd, pErrorString, NULL, 0);
		OutputDebugStringA(pErrorString);
		__debugbreak();
	}

	m_pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), IID_PPV_ARGS(m_pd3dComputeRootSignature.GetAddressOf()));
}

void ComputeManager::CreateGraphcisPipelineState()
{
}
