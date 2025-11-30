#pragma once
#include "GameObject.h"

class RawFormatImage
{
public:
	RawFormatImage(const std::string& strFileName, int nWidth, int nLength, bool bFlipY = false);
	~RawFormatImage(void);

	BYTE GetRawImagePixel(int x, int z) { return(m_pRawImagePixels[x + (z * m_nWidth)]); }
	void SetRawImagePixel(int x, int z, BYTE nPixel) { m_pRawImagePixels[x + (z * m_nWidth)] = nPixel; }

	const std::vector<BYTE>& GetRawImagePixels() const { return m_pRawImagePixels; }

	int GetRawImageWidth() { return m_nWidth; }
	int GetRawImageLength() { return m_nLength; }

protected:
	std::vector<BYTE> m_pRawImagePixels = {};

	int m_nWidth = 0;
	int m_nLength = 0;

};

class HeightMapRawImage : public RawFormatImage {
public:
	HeightMapRawImage(const std::string& strFileName, int nWidth, int nLength, XMFLOAT3 xmf3Scale);
	~HeightMapRawImage();

	XMFLOAT3 GetScale() { return m_xmf3Scale; }
	float GetHeight(float fx, float fz, bool bReverseQuad = false);
	XMFLOAT3 GetHeightMapNormal(int x, int z);

	BYTE& operator[](size_t idx) {
		return m_pRawImagePixels[idx];
	}

private:
	XMFLOAT3 m_xmf3Scale = {};

};

struct CB_TERRAIN_DATA {
	XMFLOAT4X4 xmf4x4TerrainWorld;
	XMFLOAT2 xmf2UVTranslation = XMFLOAT2(0, 0);
};

struct BillboardParameters {
	XMFLOAT3 xmf3Position;
	UINT nTextureIndex;
	XMFLOAT2 xmf2Size;
	XMUINT2 pad = XMUINT2(0,0);
};

#define MAX_BILLBOARD_COUNT 500

struct CB_BILLBOARD_DATA {
	BillboardParameters billboardData[MAX_BILLBOARD_COUNT];
};

class TerrainObject : public GameObject {
public:
	TerrainObject();
	~TerrainObject();

public:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		const std::string & strFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color);
	void CreateChildWaterGridObject(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color);

	void CreateBillboards(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::string& strFileName);

	void UpdateShaderVariables(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	virtual void Update(float fTimeElapsed) override;
	void Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& refDescHandle);
	void RenderOnMirror(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& refDescHandle,
		const XMFLOAT4& xmf4MirrorPlane, ComPtr<ID3D12PipelineState> pd3dTerrainOnMirrorPipelineState, ComPtr<ID3D12PipelineState> pd3dBillboardsOnMirrorPipelineState);
	UINT UpdateBillboardData();
	UINT UpdateBillboardDataInMirror(const XMFLOAT4& xmf4MirrorPlane);


public:
	float GetHeight(float x, float z, bool bReverseQuad = false);
	XMFLOAT3 GetNormal(float x, float z) { return m_pHeightMapImage->GetHeightMapNormal(int(x / m_xmf3Scale.x), int(z / m_xmf3Scale.z));  }

	int GetHeightMapWidth() { return m_pHeightMapImage->GetRawImageWidth(); }
	int GetHeightMapLength() { return m_pHeightMapImage->GetRawImageLength(); }

	XMFLOAT3 GetScale() { return m_xmf3Scale; }

	float GetWidth() { return (m_nWidth - 1) * m_xmf3Scale.x; }
	float GetLength() { return (m_nLength - 1) * m_xmf3Scale.z; }
	float GetWaterHeight() { return m_fWaterHeight; }

	std::array<XMFLOAT4, 4>& GetWallPlanes() { return m_xmf4MapBoundaryPlanes; }

	bool GetWireframeMode() { return m_bDrawWireframe; }
	void SetWireframeMode(bool bMode);

private:
	std::shared_ptr<HeightMapRawImage>			m_pHeightMapImage = nullptr;
	std::vector<std::shared_ptr<TerrainMesh>>	m_pTerrainMeshes = {};

	std::shared_ptr<TerrainObject>				m_pChildTerrain = nullptr;

	std::array<std::shared_ptr<Texture>, 3>		m_pTerrainTextures;

	int m_nWidth = 0;
	int m_nLength = 0;
	XMFLOAT3 m_xmf3Scale;

	float m_fWaterHeight = 250.f;
	float m_fBlendFactor = 1.f;

	bool m_bDrawWireframe = false;

	XMFLOAT2									m_xmf2UVTranslation = XMFLOAT2(0,0);
	ConstantBuffer								m_TerrainCBuffer;

	std::array<XMFLOAT4, 4>						m_xmf4MapBoundaryPlanes = {};

	std::array<std::shared_ptr<Texture>, 3>		m_pBillboardTextures;
	std::vector<BillboardParameters>			m_Billboards;
	ConstantBuffer								m_BillboardCBuffer;

	// Mirror CBuffer
	ConstantBuffer								m_TerrainOnMirrorCBuffer;
	ConstantBuffer								m_BillboardOnMirrorCBuffer;


};

