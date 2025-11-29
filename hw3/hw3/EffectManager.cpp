#include "stdafx.h"
#include "EffectManager.h"

void EffectManager::Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	m_pd3dDevice = pd3dDevice;
	CreateRootSignature();
	m_ParticleCBuffer.Create(pd3dDevice, pd3dCommandList, ConstantBufferSize<CB_PARTICLE_DATA>::value, false);

	std::shared_ptr<ExplosionEffect> pExplodeEffect = std::make_shared<ExplosionEffect>();
	pExplodeEffect->Create(pd3dDevice, pd3dCommandList, m_pd3dRootSignature, 100000);
	m_pEffects.insert({ typeid(ExplosionEffect), EffectPair{ pExplodeEffect, {}} });

}

void EffectManager::Update(float fTimeElapsed)
{
	if (m_nParticles <= 0) {
		return;
	}

	for (auto& effectPair : m_pEffects) {
		// Lifetime 이 지난 이펙트들을 제거
		auto& [pEffect, parameters] = effectPair.second;
		size_t nErased = std::erase_if(parameters, [&pEffect](const EffectParameter& param) {
			return pEffect->IsEnd(param.fElapsedTime);
		});
		m_nParticles -= nErased;

		for (auto& param : parameters) {
			param.fElapsedTime += fTimeElapsed;
		}
	}
}

void EffectManager::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	if (m_nParticles <= 0) {
		return;
	}

	pd3dCommandList->SetGraphicsRootSignature(m_pd3dRootSignature.Get());

	auto pCamera = CUR_SCENE->GetPlayer()->GetCamera();	// 이미 행렬 업데이트 되어있어야 함 (RenderManager에서)
	pCamera->GetCBuffer().SetBufferToPipeline(pd3dCommandList, 0);

	UINT uiCBufferOffset = 0;
	for (auto& effectPair : m_pEffects) {
		auto& [pEffect, parameters] = effectPair.second;
		m_ParticleCBuffer.UpdateData(parameters, uiCBufferOffset);
		uiCBufferOffset += parameters.size();
	}

	m_ParticleCBuffer.SetBufferToPipeline(pd3dCommandList, 1);

	UINT nDataOffsetBaseInCBuffer = 0;
	for (auto& effectPair : m_pEffects) {
		auto& [pEffect, parameters] = effectPair.second;
		pEffect->Render(m_pd3dDevice, pd3dCommandList, nDataOffsetBaseInCBuffer, parameters.size());
	}
}

void EffectManager::CreateRootSignature()
{
	D3D12_ROOT_PARAMETER d3dRootParameters[3];
	// Camera Data
	d3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	d3dRootParameters[0].Descriptor.ShaderRegister = 0;
	d3dRootParameters[0].Descriptor.RegisterSpace = 0;
	d3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	
	// Particle Data
	d3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	d3dRootParameters[1].Descriptor.ShaderRegister = 1;
	d3dRootParameters[1].Descriptor.RegisterSpace = 0;
	d3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	
	// Particle Data Index
	d3dRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	d3dRootParameters[2].Constants.Num32BitValues = 1;
	d3dRootParameters[2].Constants.ShaderRegister = 2;
	d3dRootParameters[2].Constants.RegisterSpace = 0;
	d3dRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

	D3D12_STATIC_SAMPLER_DESC d3dSamplerDescs[1];
	d3dSamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dSamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dSamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dSamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dSamplerDescs[0].MipLODBias = 0;
	d3dSamplerDescs[0].MaxAnisotropy = 1;
	d3dSamplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	d3dSamplerDescs[0].MinLOD = 0;
	d3dSamplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDescs[0].ShaderRegister = 0;
	d3dSamplerDescs[0].RegisterSpace = 0;
	d3dSamplerDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

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

	m_pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), IID_PPV_ARGS(m_pd3dRootSignature.GetAddressOf()));
}
