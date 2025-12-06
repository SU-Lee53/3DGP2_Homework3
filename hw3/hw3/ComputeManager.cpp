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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MotionBlur

void MotionBlur::CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice)
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC d3dPipelineDesc;
	{


	}
}

void MotionBlur::UpdateShaderVariables()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ComputeManager

ComputeManager::ComputeManager(ComPtr<ID3D12Device> pd3dDevice)
{
}

void ComputeManager::Run(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
}

void ComputeManager::CreateRootSignature()
{
	D3D12_DESCRIPTOR_RANGE d3dDescriptorRanges[1];
	d3dDescriptorRanges[0].NumDescriptors = 2;
	d3dDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	d3dDescriptorRanges[0].BaseShaderRegister = 0;
	d3dDescriptorRanges[0].RegisterSpace = 0;
	d3dDescriptorRanges[0].OffsetInDescriptorsFromTableStart = 0;

	D3D12_ROOT_PARAMETER d3dRootParameters[1];
	d3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	d3dRootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
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
