#pragma once

class Shader {
public:
	virtual void Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature = nullptr) {}

	virtual void OnPrepareRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, int nPipelineState = 0);
	ComPtr<ID3D12PipelineState> GetPipelineState(int nIndex);

protected:
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() { return D3D12_INPUT_LAYOUT_DESC{}; }
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();

	virtual D3D12_SHADER_BYTECODE CreateVertexShader();
	virtual D3D12_SHADER_BYTECODE CreateGeometryShader();
	virtual D3D12_SHADER_BYTECODE CreatePixelShader();

	D3D12_SHADER_BYTECODE CompileShaderFromFile(const std::wstring& wstrFileName, const std::string& pszShaderName, const std::string& pszShaderProfile, ID3DBlob** ppd3dShaderBlob);
	D3D12_SHADER_BYTECODE ReadCompiledShaderFromFile(const std::wstring& wsvFileName, ID3DBlob** ppd3dShaderBlob);

protected:
	ComPtr<ID3DBlob> m_pd3dVertexShaderBlob = nullptr;
	ComPtr<ID3DBlob> m_pd3dGeometryShaderBlob = nullptr;
	ComPtr<ID3DBlob> m_pd3dPixelShaderBlob	= nullptr;

	std::vector<ComPtr<ID3D12PipelineState>>	m_pd3dPipelineStates{};
	std::vector<D3D12_INPUT_ELEMENT_DESC>		m_d3dInputElements;

public:
	static D3D12_SHADER_BYTECODE CompileShader(const std::wstring& wstrFileName, const std::string& pszShaderName, const std::string& pszShaderProfile, ID3DBlob** ppd3dShaderBlob);

};

class StandardShader : public Shader {
public:
	virtual void Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature = nullptr) override;

protected:
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() override;

	virtual D3D12_SHADER_BYTECODE CreateVertexShader() override;
	virtual D3D12_SHADER_BYTECODE CreatePixelShader() override;

};

class MirrorShader : public Shader {
public:
	virtual void Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature = nullptr) override;

protected:
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() override;
	D3D12_INPUT_LAYOUT_DESC CreateTerrainInputLayout();

	virtual D3D12_SHADER_BYTECODE CreateVertexShader() override;
	virtual D3D12_SHADER_BYTECODE CreatePixelShader() override;

	virtual D3D12_SHADER_BYTECODE CreateMirrorOnStencilVertexShader();
	virtual D3D12_SHADER_BYTECODE CreateMirrorOnStencilPixelShader();

	virtual D3D12_SHADER_BYTECODE CreateTerrainVertexShader();
	virtual D3D12_SHADER_BYTECODE CreateTerrainPixelShader();

	virtual D3D12_SHADER_BYTECODE CreateBillboardOnTerrainVertexShader();
	virtual D3D12_SHADER_BYTECODE CreateBillboardOnTerrainGeometryShader();
	virtual D3D12_SHADER_BYTECODE CreateBillboardOnTerrainPixelShader();

protected:
	std::vector<D3D12_INPUT_ELEMENT_DESC>		m_d3dTerrainInputElements;
};

class TerrainShader : public Shader {
public:
	virtual void Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature = nullptr) override;

protected:
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() override;

	virtual D3D12_SHADER_BYTECODE CreateVertexShader() override;
	virtual D3D12_SHADER_BYTECODE CreatePixelShader() override;

	D3D12_SHADER_BYTECODE CreateBillboardVertexShader();
	D3D12_SHADER_BYTECODE CreateBillboardGeometryShader();
	D3D12_SHADER_BYTECODE CreateBillboardPixelShader();

};

class SkyboxShader : public Shader {
public:
	virtual void Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature = nullptr) override;

protected:
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState() override;
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState() override;

};

class OBBDebugShader : public Shader {
public:
	virtual void Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature = nullptr) override;

protected:
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState() override;

	virtual D3D12_SHADER_BYTECODE CreateVertexShader() override;
	virtual D3D12_SHADER_BYTECODE CreateGeometryShader() override;
	virtual D3D12_SHADER_BYTECODE CreatePixelShader() override;

};
