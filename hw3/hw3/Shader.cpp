#include "stdafx.h"
#include "Shader.h"
#include <fstream>

////////////
// Shader //
////////////

D3D12_RASTERIZER_DESC Shader::CreateRasterizerState()
{
	D3D12_RASTERIZER_DESC desc{};
	{
		desc.FillMode = D3D12_FILL_MODE_SOLID;
		desc.CullMode = D3D12_CULL_MODE_BACK;
		desc.FrontCounterClockwise = FALSE;
		desc.DepthBias = 0;
		desc.DepthBiasClamp = 0.0f;
		desc.SlopeScaledDepthBias = 0.0f;
		desc.DepthClipEnable = TRUE;
		desc.MultisampleEnable = FALSE;
		desc.AntialiasedLineEnable = FALSE;
		desc.ForcedSampleCount = 0;
		desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}

	return desc;
}

D3D12_BLEND_DESC Shader::CreateBlendState()
{
	D3D12_BLEND_DESC desc{};
	{
		desc.AlphaToCoverageEnable = FALSE;
		desc.IndependentBlendEnable = FALSE;
		desc.RenderTarget[0].BlendEnable = FALSE;
		desc.RenderTarget[0].LogicOpEnable = FALSE;
		desc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		desc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		desc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	return desc;
}

D3D12_DEPTH_STENCIL_DESC Shader::CreateDepthStencilState()
{
	D3D12_DEPTH_STENCIL_DESC desc{};
	{
		desc.DepthEnable = TRUE;
		desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		desc.StencilEnable = FALSE;
		desc.StencilReadMask = 0x00;
		desc.StencilWriteMask = 0x00;
		desc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
		desc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		desc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	}

	return desc;
}

D3D12_SHADER_BYTECODE Shader::CreateVertexShader()
{
	return D3D12_SHADER_BYTECODE();
}

D3D12_SHADER_BYTECODE Shader::CreateGeometryShader()
{
	return D3D12_SHADER_BYTECODE();
}

D3D12_SHADER_BYTECODE Shader::CreatePixelShader()
{
	return D3D12_SHADER_BYTECODE();
}

void Shader::OnPrepareRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, int nPipelineState)
{
	pd3dCommandList->SetPipelineState(m_pd3dPipelineStates[nPipelineState].Get());
}

D3D12_SHADER_BYTECODE Shader::CompileShaderFromFile(const std::wstring& wstrFileName, const std::string& strShaderName, const std::string& strShaderProfile, ID3DBlob** ppd3dShaderBlob)
{
	UINT nCompileFlags = 0;
#ifdef _DEBUG
	nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> pd3dErrorBlob = nullptr;
	HRESULT hResult = ::D3DCompileFromFile(wstrFileName.data(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, strShaderName.data(), strShaderProfile.data(), nCompileFlags, 0, ppd3dShaderBlob, pd3dErrorBlob.GetAddressOf());
	char* pErrorString = NULL;
	if (pd3dErrorBlob) {
		pErrorString = (char*)pd3dErrorBlob->GetBufferPointer();
		HWND hWnd = ::GetActiveWindow();
		MessageBoxA(hWnd, pErrorString, NULL, 0);
		OutputDebugStringA(pErrorString);
		__debugbreak();
	}
	
	D3D12_SHADER_BYTECODE d3dShaderByteCode{};
	d3dShaderByteCode.BytecodeLength = (*ppd3dShaderBlob)->GetBufferSize();
	d3dShaderByteCode.pShaderBytecode = (*ppd3dShaderBlob)->GetBufferPointer();

	return d3dShaderByteCode;
}

D3D12_SHADER_BYTECODE Shader::ReadCompiledShaderFromFile(const std::wstring& wstrFileName, ID3DBlob** ppd3dShaderBlob)
{
	std::ifstream in { wstrFileName.data(), std::ios::binary};
	
	if (!in) {
		__debugbreak();
	}

	in.seekg(0, std::ios::end);
	int nFileSize = in.tellg();
	in.seekg(0, std::ios::beg);

	std::unique_ptr<BYTE[]> pByteCode = std::make_unique<BYTE[]>(nFileSize);
	in.read((char*)pByteCode.get(), nFileSize);

	D3D12_SHADER_BYTECODE d3dShaderByteCode{};

	HRESULT hr = D3DCreateBlob(nFileSize, ppd3dShaderBlob);
	if (FAILED(hr)) {
		__debugbreak();
	}

	::memcpy((*ppd3dShaderBlob)->GetBufferPointer(), pByteCode.get(), nFileSize);

	d3dShaderByteCode.BytecodeLength = (*ppd3dShaderBlob)->GetBufferSize();
	d3dShaderByteCode.pShaderBytecode = (*ppd3dShaderBlob)->GetBufferPointer();

	return d3dShaderByteCode;
}

ComPtr<ID3D12PipelineState> Shader::GetPipelineState(int nIndex)
{
	assert(nIndex < m_pd3dPipelineStates.size());
	return m_pd3dPipelineStates[nIndex];
}

D3D12_SHADER_BYTECODE Shader::CompileShader(const std::wstring& wstrFileName, const std::string& strShaderName, const std::string& strShaderProfile, ID3DBlob** ppd3dShaderBlob)
{
	UINT nCompileFlags = 0;
#ifdef _DEBUG
	nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> pd3dErrorBlob = nullptr;
	HRESULT hResult = ::D3DCompileFromFile(wstrFileName.data(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, strShaderName.data(), strShaderProfile.data(), nCompileFlags, 0, ppd3dShaderBlob, pd3dErrorBlob.GetAddressOf());
	if (FAILED(hResult)) {
		char* pErrorString = NULL;
		if (pd3dErrorBlob) {
			pErrorString = (char*)pd3dErrorBlob->GetBufferPointer();
			HWND hWnd = ::GetActiveWindow();
			MessageBoxA(hWnd, pErrorString, NULL, 0);
			OutputDebugStringA(pErrorString);
		}
		__debugbreak();
	}

	char* pErrorString = NULL;
	if (pd3dErrorBlob) {
		pErrorString = (char*)pd3dErrorBlob->GetBufferPointer();
		HWND hWnd = ::GetActiveWindow();
		MessageBoxA(hWnd, pErrorString, NULL, 0);
		OutputDebugStringA(pErrorString);
		__debugbreak();
	}

	D3D12_SHADER_BYTECODE d3dShaderByteCode{};
	d3dShaderByteCode.BytecodeLength = (*ppd3dShaderBlob)->GetBufferSize();
	d3dShaderByteCode.pShaderBytecode = (*ppd3dShaderBlob)->GetBufferPointer();

	return d3dShaderByteCode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StandardShader

void StandardShader::Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature)
{
	m_pd3dPipelineStates.resize(2);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = pd3dRootSignature ? pd3dRootSignature.Get() : RenderManager::g_pd3dRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("StandardVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("StandardPS");
		d3dPipelineDesc.RasterizerState = CreateRasterizerState();
		d3dPipelineDesc.BlendState = CreateBlendState();
		d3dPipelineDesc.DepthStencilState = CreateDepthStencilState();
		d3dPipelineDesc.InputLayout = CreateInputLayout();
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineStates[0].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	// 블렌딩용 하나더
	{
		d3dPipelineDesc.DepthStencilState.DepthEnable = true;
		d3dPipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		d3dPipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		d3dPipelineDesc.BlendState.AlphaToCoverageEnable = false;
		d3dPipelineDesc.BlendState.IndependentBlendEnable = false;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendEnable = true;
		d3dPipelineDesc.BlendState.RenderTarget[0].LogicOpEnable = false;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_BLEND_FACTOR;	// 직접 정한 Blend Factor 를 이용하여 블렌딩 할 예정임
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_BLEND_FACTOR;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		d3dPipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineStates[1].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}

D3D12_INPUT_LAYOUT_DESC StandardShader::CreateInputLayout()
{

	/*
	typedef struct D3D12_INPUT_ELEMENT_DESC
	{
		LPCSTR SemanticName;
		UINT SemanticIndex;
		DXGI_FORMAT Format;
		UINT InputSlot;
		UINT AlignedByteOffset;
		D3D12_INPUT_CLASSIFICATION InputSlotClass;
		UINT InstanceDataStepRate;
	} 	D3D12_INPUT_ELEMENT_DESC;
	*/

	m_d3dInputElements = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 4, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.NumElements = m_d3dInputElements.size();
	inputLayoutDesc.pInputElementDescs = m_d3dInputElements.data();

	return inputLayoutDesc;
}

D3D12_SHADER_BYTECODE StandardShader::CreateVertexShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "VSStandard", "vs_5_1", m_pd3dVertexShaderBlob.GetAddressOf());
}

D3D12_SHADER_BYTECODE StandardShader::CreatePixelShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "PSStandard", "ps_5_1", m_pd3dPixelShaderBlob.GetAddressOf());
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MirrorShader

void MirrorShader::Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature)
{
	HRESULT hr{};

	// 총 4개의 파이프라인
	// 
	// 1. 월드의 객체들을 그림 <- 이미 그림
	// ++ 거울 뒷면 그려야함 -> 안그러면 원래 뒤에 있던 객체들이 보임. 얘는 m_pd3dPipelineState[3]으로
	// 2. 거울을 스텐실 버퍼에 그림
	// 3. 거울에 반사된 객체들을 그림
	// 4. 거울을 블렌딩해서 그림

	// 추가 : Terrain 과 Terrain 위의 빌보드 그리기 용도 전부 다 따로 만들어야함
	// 추가 : 거울 부분만 Depth를 1로 초기화하도록 Pipeline 을 추가

	m_pd3dPipelineStates.resize(7);

	// 2. 거울을 스텐실 버퍼에 그림
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc = {};
	{
		d3dPipelineDesc.pRootSignature = pd3dRootSignature ? pd3dRootSignature.Get() : RenderManager::g_pd3dRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("StandardVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("StandardPS");
		d3dPipelineDesc.RasterizerState = CreateRasterizerState();
		d3dPipelineDesc.BlendState = CreateBlendState();
		d3dPipelineDesc.BlendState.AlphaToCoverageEnable = false;
		d3dPipelineDesc.BlendState.IndependentBlendEnable = false;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendEnable = false;
		d3dPipelineDesc.BlendState.RenderTarget[0].LogicOpEnable = false;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		d3dPipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = 0;	// 렌더 타겟 변경 X
		d3dPipelineDesc.DepthStencilState = CreateDepthStencilState();
		d3dPipelineDesc.DepthStencilState.DepthEnable = true;
		d3dPipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		d3dPipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		d3dPipelineDesc.DepthStencilState.StencilEnable = true;
		d3dPipelineDesc.DepthStencilState.StencilReadMask = 0xFF;
		d3dPipelineDesc.DepthStencilState.StencilWriteMask = 0xFF;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;	// 스텐실 통과시 255로 덮음
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		d3dPipelineDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		d3dPipelineDesc.InputLayout = CreateInputLayout();
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineStates[0].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	// ++. 거울부분만 Depth를 1로 초기화함
	// 어떻게? -> Depth Test 를 하고 Stencil 이 덮어씌워진 곳만 xyww 한 곳을 넘겨줌
	{
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("MirrorVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("MirrorPS");
		d3dPipelineDesc.RasterizerState = CreateRasterizerState();
		d3dPipelineDesc.BlendState = CreateBlendState();
		d3dPipelineDesc.BlendState.AlphaToCoverageEnable = false;
		d3dPipelineDesc.BlendState.IndependentBlendEnable = false;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendEnable = false;
		d3dPipelineDesc.BlendState.RenderTarget[0].LogicOpEnable = false;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		d3dPipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = 0;	// 렌더 타겟 변경 X
		d3dPipelineDesc.DepthStencilState = CreateDepthStencilState();
		d3dPipelineDesc.DepthStencilState.DepthEnable = true;
		d3dPipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		d3dPipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		d3dPipelineDesc.DepthStencilState.StencilEnable = true;
		d3dPipelineDesc.DepthStencilState.StencilReadMask = 0xFF;
		d3dPipelineDesc.DepthStencilState.StencilWriteMask = 0xFF;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
		d3dPipelineDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
		d3dPipelineDesc.InputLayout = CreateInputLayout();
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineStates[1].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	// 2. 거울에 반사된 객체들을 그림
	// Stencil 값이 1 인 곳에만 그림
	{
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("StandardVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("StandardPS");
		d3dPipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		d3dPipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		d3dPipelineDesc.RasterizerState.FrontCounterClockwise = true;
		d3dPipelineDesc.RasterizerState.DepthClipEnable = true;
		d3dPipelineDesc.BlendState = CreateBlendState();
		d3dPipelineDesc.DepthStencilState.DepthEnable = true;
		d3dPipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		d3dPipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		d3dPipelineDesc.DepthStencilState.StencilEnable = true;
		d3dPipelineDesc.DepthStencilState.StencilReadMask = 0xFF;
		d3dPipelineDesc.DepthStencilState.StencilWriteMask = 0xFF;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
		d3dPipelineDesc.InputLayout = CreateInputLayout();
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineStates[2].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	// 2-1. 거울에 반사된 Terrain을 그림
	// Stencil 값이 1 인 곳에만 그림
	{
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("TerrainVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("TerrainPS");
		d3dPipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		d3dPipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		d3dPipelineDesc.RasterizerState.FrontCounterClockwise = true;
		d3dPipelineDesc.RasterizerState.DepthClipEnable = true;
		d3dPipelineDesc.BlendState = CreateBlendState();
		d3dPipelineDesc.DepthStencilState.DepthEnable = true;
		d3dPipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		d3dPipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		d3dPipelineDesc.DepthStencilState.StencilEnable = true;
		d3dPipelineDesc.DepthStencilState.StencilReadMask = 0xFF;
		d3dPipelineDesc.DepthStencilState.StencilWriteMask = 0xFF;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
		d3dPipelineDesc.InputLayout = CreateTerrainInputLayout();
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineStates[3].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	// 2-2. 거울에 반사된 Billboard를 그림
	// Stencil 값이 1 인 곳에만 그림
	{
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("BillboardVS");
		d3dPipelineDesc.GS = SHADER->GetShaderByteCode("BillboardGS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("BillboardPS");
		d3dPipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		d3dPipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		d3dPipelineDesc.RasterizerState.FrontCounterClockwise = false;
		d3dPipelineDesc.RasterizerState.DepthClipEnable = true;
		d3dPipelineDesc.BlendState = CreateBlendState();
		d3dPipelineDesc.DepthStencilState.DepthEnable = true;
		d3dPipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		d3dPipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		d3dPipelineDesc.DepthStencilState.StencilEnable = true;
		d3dPipelineDesc.DepthStencilState.StencilReadMask = 0xFF;
		d3dPipelineDesc.DepthStencilState.StencilWriteMask = 0xFF;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
		d3dPipelineDesc.InputLayout.NumElements = 0;
		d3dPipelineDesc.InputLayout.pInputElementDescs = nullptr;
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineStates[4].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	// 3. 거울을 블렌딩하여 그림
	{
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("StandardVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("StandardPS");
		d3dPipelineDesc.GS = D3D12_SHADER_BYTECODE{ nullptr, 0 };	// billboard GS 가 남아있으므로 초기화
		d3dPipelineDesc.RasterizerState = CreateRasterizerState();
		d3dPipelineDesc.BlendState.AlphaToCoverageEnable = false;
		d3dPipelineDesc.BlendState.IndependentBlendEnable = false;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendEnable = true;
		d3dPipelineDesc.BlendState.RenderTarget[0].LogicOpEnable = false;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_BLEND_FACTOR;	// 직접 정한 Blend Factor 를 이용하여 블렌딩 할 예정임
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_BLEND_FACTOR;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		d3dPipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		d3dPipelineDesc.DepthStencilState = CreateDepthStencilState();
		d3dPipelineDesc.InputLayout = CreateInputLayout();
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineStates[5].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	// ++. 거울 뒷면을 그림
	{
		d3dPipelineDesc.RasterizerState = CreateRasterizerState();
		d3dPipelineDesc.BlendState = CreateBlendState();
		d3dPipelineDesc.DepthStencilState = CreateDepthStencilState();
		d3dPipelineDesc.DepthStencilState.DepthEnable = true;
		d3dPipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		d3dPipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		d3dPipelineDesc.DepthStencilState.StencilEnable = false;
		d3dPipelineDesc.InputLayout = CreateInputLayout();
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineStates[6].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

}

D3D12_INPUT_LAYOUT_DESC MirrorShader::CreateInputLayout()
{
	m_d3dInputElements = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 4, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.NumElements = m_d3dInputElements.size();
	inputLayoutDesc.pInputElementDescs = m_d3dInputElements.data();

	return inputLayoutDesc;
}

D3D12_INPUT_LAYOUT_DESC MirrorShader::CreateTerrainInputLayout()
{
	m_d3dTerrainInputElements = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.NumElements = m_d3dTerrainInputElements.size();
	inputLayoutDesc.pInputElementDescs = m_d3dTerrainInputElements.data();

	return inputLayoutDesc;
}

D3D12_SHADER_BYTECODE MirrorShader::CreateVertexShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "VSStandard", "vs_5_1", m_pd3dVertexShaderBlob.GetAddressOf());
}

D3D12_SHADER_BYTECODE MirrorShader::CreatePixelShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "PSStandard", "ps_5_1", m_pd3dPixelShaderBlob.GetAddressOf());
}

D3D12_SHADER_BYTECODE MirrorShader::CreateMirrorOnStencilVertexShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "VSMirror", "vs_5_1", m_pd3dVertexShaderBlob.GetAddressOf());
}

D3D12_SHADER_BYTECODE MirrorShader::CreateMirrorOnStencilPixelShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "PSMirror", "ps_5_1", m_pd3dPixelShaderBlob.GetAddressOf());
}

D3D12_SHADER_BYTECODE MirrorShader::CreateTerrainVertexShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "VSTerrain", "vs_5_1", m_pd3dVertexShaderBlob.GetAddressOf());
}

D3D12_SHADER_BYTECODE MirrorShader::CreateTerrainPixelShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "PSTerrain", "ps_5_1", m_pd3dPixelShaderBlob.GetAddressOf());
}

D3D12_SHADER_BYTECODE MirrorShader::CreateBillboardOnTerrainVertexShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "VSBillboard", "vs_5_1", m_pd3dVertexShaderBlob.GetAddressOf());
}

D3D12_SHADER_BYTECODE MirrorShader::CreateBillboardOnTerrainGeometryShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "GSBillboard", "gs_5_1", m_pd3dGeometryShaderBlob.GetAddressOf());
}

D3D12_SHADER_BYTECODE MirrorShader::CreateBillboardOnTerrainPixelShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "PSBillboard", "ps_5_1", m_pd3dPixelShaderBlob.GetAddressOf());
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainShader

void TerrainShader::Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature)
{
	m_pd3dPipelineStates.resize(2);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = pd3dRootSignature ? pd3dRootSignature.Get() : RenderManager::g_pd3dRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("TerrainVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("TerrainPS");
		d3dPipelineDesc.RasterizerState = CreateRasterizerState();
		d3dPipelineDesc.BlendState.AlphaToCoverageEnable = false;
		d3dPipelineDesc.BlendState.IndependentBlendEnable = false;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendEnable = true;
		d3dPipelineDesc.BlendState.RenderTarget[0].LogicOpEnable = false;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_BLEND_FACTOR;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_BLEND_FACTOR;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		d3dPipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		d3dPipelineDesc.DepthStencilState = CreateDepthStencilState();
		d3dPipelineDesc.InputLayout = CreateInputLayout();
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineStates[0].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	{
		d3dPipelineDesc.VS =SHADER->GetShaderByteCode("BillboardVS");
		d3dPipelineDesc.GS =SHADER->GetShaderByteCode("BillboardGS");
		d3dPipelineDesc.PS =SHADER->GetShaderByteCode("BillboardPS");
		d3dPipelineDesc.RasterizerState = CreateRasterizerState();
		d3dPipelineDesc.BlendState = CreateBlendState();
		d3dPipelineDesc.BlendState.AlphaToCoverageEnable = true;
		d3dPipelineDesc.DepthStencilState = CreateDepthStencilState();
		d3dPipelineDesc.InputLayout.NumElements = 0;
		d3dPipelineDesc.InputLayout.pInputElementDescs = nullptr;
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineStates[1].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}

D3D12_INPUT_LAYOUT_DESC TerrainShader::CreateInputLayout()
{

	/*
	typedef struct D3D12_INPUT_ELEMENT_DESC
	{
		LPCSTR SemanticName;
		UINT SemanticIndex;
		DXGI_FORMAT Format;
		UINT InputSlot;
		UINT AlignedByteOffset;
		D3D12_INPUT_CLASSIFICATION InputSlotClass;
		UINT InstanceDataStepRate;
	} 	D3D12_INPUT_ELEMENT_DESC;
	*/

	m_d3dInputElements = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.NumElements = m_d3dInputElements.size();
	inputLayoutDesc.pInputElementDescs = m_d3dInputElements.data();

	return inputLayoutDesc;
}

D3D12_SHADER_BYTECODE TerrainShader::CreateVertexShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "VSTerrain", "vs_5_1", m_pd3dVertexShaderBlob.GetAddressOf());
}

D3D12_SHADER_BYTECODE TerrainShader::CreatePixelShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "PSTerrain", "ps_5_1", m_pd3dPixelShaderBlob.GetAddressOf());
}

D3D12_SHADER_BYTECODE TerrainShader::CreateBillboardVertexShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "VSBillboard", "vs_5_1", m_pd3dVertexShaderBlob.GetAddressOf());
}

D3D12_SHADER_BYTECODE TerrainShader::CreateBillboardGeometryShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "GSBillboard", "gs_5_1", m_pd3dGeometryShaderBlob.GetAddressOf());
}

D3D12_SHADER_BYTECODE TerrainShader::CreateBillboardPixelShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "PSBillboard", "ps_5_1", m_pd3dPixelShaderBlob.GetAddressOf());
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OBBDebugShader

void OBBDebugShader::Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature)
{
	m_pd3dPipelineStates.resize(1);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = RenderManager::g_pd3dRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("DebugVS");
		d3dPipelineDesc.GS = SHADER->GetShaderByteCode("DebugGS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("DebugPS");
		d3dPipelineDesc.RasterizerState = CreateRasterizerState();
		d3dPipelineDesc.BlendState = CreateBlendState();
		d3dPipelineDesc.DepthStencilState = CreateDepthStencilState();
		d3dPipelineDesc.InputLayout.NumElements = 0;
		d3dPipelineDesc.InputLayout.pInputElementDescs = nullptr;
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineStates[0].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}

D3D12_RASTERIZER_DESC OBBDebugShader::CreateRasterizerState()
{
	D3D12_RASTERIZER_DESC desc{};
	{
		desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
		desc.CullMode = D3D12_CULL_MODE_NONE;
		desc.FrontCounterClockwise = FALSE;
		desc.DepthBias = 0;
		desc.DepthBiasClamp = 0.0f;
		desc.SlopeScaledDepthBias = 0.0f;
		desc.DepthClipEnable = TRUE;
		desc.MultisampleEnable = FALSE;
		desc.AntialiasedLineEnable = FALSE;
		desc.ForcedSampleCount = 0;
		desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}

	return desc;
}


D3D12_SHADER_BYTECODE OBBDebugShader::CreateVertexShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "VSDebug", "vs_5_1", m_pd3dVertexShaderBlob.GetAddressOf());
}

D3D12_SHADER_BYTECODE OBBDebugShader::CreateGeometryShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "GSDebug", "gs_5_1", m_pd3dVertexShaderBlob.GetAddressOf());
}

D3D12_SHADER_BYTECODE OBBDebugShader::CreatePixelShader()
{
	return CompileShaderFromFile(L"../HLSL/Shaders.hlsl", "PSDebug", "ps_5_1", m_pd3dPixelShaderBlob.GetAddressOf());
}

void SkyboxShader::Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature)
{
	m_pd3dPipelineStates.resize(1);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = RenderManager::g_pd3dRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("SkyboxVS");
		d3dPipelineDesc.GS = SHADER->GetShaderByteCode("SkyboxGS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("SkyboxPS");
		d3dPipelineDesc.RasterizerState = CreateRasterizerState();
		d3dPipelineDesc.BlendState = CreateBlendState();
		d3dPipelineDesc.DepthStencilState = CreateDepthStencilState();
		d3dPipelineDesc.InputLayout.NumElements = 0;
		d3dPipelineDesc.InputLayout.pInputElementDescs = nullptr;
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineStates[0].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}

D3D12_RASTERIZER_DESC SkyboxShader::CreateRasterizerState()
{
	D3D12_RASTERIZER_DESC desc{};
	{
		desc.FillMode = D3D12_FILL_MODE_SOLID;
		desc.CullMode = D3D12_CULL_MODE_BACK;
		desc.FrontCounterClockwise = TRUE;
		desc.DepthBias = 0;
		desc.DepthBiasClamp = 0.0f;
		desc.SlopeScaledDepthBias = 0.0f;
		desc.DepthClipEnable = TRUE;
		desc.MultisampleEnable = FALSE;
		desc.AntialiasedLineEnable = FALSE;
		desc.ForcedSampleCount = 0;
		desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}

	return desc;
}

D3D12_DEPTH_STENCIL_DESC SkyboxShader::CreateDepthStencilState()
{
	D3D12_DEPTH_STENCIL_DESC desc{};
	{
		desc.DepthEnable = TRUE;
		desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	}

	return desc;
}
