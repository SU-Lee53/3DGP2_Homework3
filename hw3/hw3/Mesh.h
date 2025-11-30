#pragma once
#include "InputLayout.h"

enum VERTEX_TYPE : UINT {
	VERTEX_TYPE_POSITION = 1,
	VERTEX_TYPE_COLOR = (VERTEX_TYPE_POSITION << 1),
	VERTEX_TYPE_NORMAL = (VERTEX_TYPE_COLOR << 1),
	VERTEX_TYPE_TANGENT = (VERTEX_TYPE_NORMAL << 1),
	VERTEX_TYPE_TEXTURE_COORD0 = (VERTEX_TYPE_TANGENT << 1),
	VERTEX_TYPE_TEXTURE_COORD1 = (VERTEX_TYPE_TEXTURE_COORD0 << 1),


	VERTEX_TYPE_UNDEFINED = 0
};

class Mesh {
public:
	Mesh();

	size_t GetSubSetCount() { return m_nSubMeshes; }
	VERTEX_TYPE GetType() { return (VERTEX_TYPE)m_nType; }

	const BoundingOrientedBox& GetOBB() const { return m_xmOBBInWorld; }
	const BoundingOrientedBox& GetOBBOrigin() const { return m_xmOBB; }
	void UpdateOBB(XMFLOAT4X4 xmf4x4World);

	virtual void ReleaseUploadBuffers();

protected:
	std::string					m_strMeshName;

protected:
	D3D12_PRIMITIVE_TOPOLOGY	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	UINT						m_nSlot = 0;
	UINT						m_nVertices = 0;
	UINT						m_nOffset = 0;

	UINT						m_nType = VERTEX_TYPE_UNDEFINED;

protected:
	std::vector<XMFLOAT3>		m_xmf3Positions{};
	ComPtr<ID3D12Resource>		m_pd3dPositionBuffer = nullptr;
	ComPtr<ID3D12Resource>		m_pd3dPositionUploadBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW	m_d3dPositionBufferView{};
	
	UINT									m_nSubMeshes = 0;
	std::vector<int>						m_nSubSetIndices;
	std::vector<std::vector<UINT>>			m_IndicesBySubset;
	std::vector<ComPtr<ID3D12Resource>>		m_pd3dSubSetIndexBuffers;
	std::vector<ComPtr<ID3D12Resource>>		m_pd3dSubSetIndexUploadBuffers;
	std::vector<D3D12_INDEX_BUFFER_VIEW>	m_d3dSubSetIndexBufferViews;

	XMFLOAT3				m_xmf3AABBCenter = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3				m_xmf3AABBExtents = XMFLOAT3(0.0f, 0.0f, 0.0f);
	BoundingOrientedBox		m_xmOBB{};
	BoundingOrientedBox		m_xmOBBInWorld{};

public:
	virtual void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) {}
	virtual void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, int nSubSet, int nInstanceCount = 1);
};

class StandardMesh : public Mesh {

public:
	StandardMesh();
	virtual ~StandardMesh();

public:
	void LoadMeshFromFile(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::ifstream& inFile);

	virtual void ReleaseUploadBuffers() override;
	virtual void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, int nSubSet, int nInstanceCount = 1) override;

protected:
	std::vector<XMFLOAT4>		m_xmf4Colors{};
	std::vector<XMFLOAT3>		m_xmf3Normals{};
	std::vector<XMFLOAT3>		m_xmf3Tangents{};
	std::vector<XMFLOAT3>		m_xmf3BiTangents{};
	std::vector<XMFLOAT2>		m_xmf2TextureCoords0{};
	std::vector<XMFLOAT2>		m_xmf2TextureCoords1{};

	ComPtr<ID3D12Resource>		m_pd3dTextureCoord0Buffer = nullptr;
	ComPtr<ID3D12Resource>		m_pd3dTextureCoord0UploadBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW	m_d3dTextureCoord0BufferView;

	ComPtr<ID3D12Resource>		m_pd3dTextureCoord1Buffer = nullptr;
	ComPtr<ID3D12Resource>		m_pd3dTextureCoord1UploadBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW	m_d3dTextureCoord1BufferView;

	ComPtr<ID3D12Resource>		m_pd3dNormalBuffer = nullptr;
	ComPtr<ID3D12Resource>		m_pd3dNormalUploadBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW	m_d3dNormalBufferView;

	ComPtr<ID3D12Resource>		m_pd3dTangentBuffer = nullptr;
	ComPtr<ID3D12Resource>		m_pd3dTangentUploadBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW	m_d3dTangentBufferView;

	ComPtr<ID3D12Resource>		m_pd3dBiTangentBuffer = nullptr;
	ComPtr<ID3D12Resource>		m_pd3dBiTangentUploadBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW	m_d3dBiTangentBufferView;

public:
	static std::shared_ptr<StandardMesh> GenerateMirrorMesh(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		float fWidth, float fHeight, int nWindowsInWidth, int nWindowsInHeights);
	
	static std::shared_ptr<StandardMesh> GenerateBuildingTopMesh(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		float fWidth, float fLength);


};

class HeightMapRawImage;

#define TERRAIN_TESSELATION

class TerrainMesh : public Mesh {

public:
	TerrainMesh();
	virtual ~TerrainMesh();

public:
	void Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, int xStart, int zStart, 
		int nWidth, int nLength, XMFLOAT3 xmf3Scale = XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4 xmf4Color = XMFLOAT4(1.0f, 1.0f, 0.0f, 0.0f), std::shared_ptr<HeightMapRawImage> pHeightMap = nullptr);

	virtual void ReleaseUploadBuffers() override;
	virtual void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, int nSubSet, int nInstanceCount = 1) override;

public:
	XMFLOAT3 GetScale() { return(m_xmf3Scale); }
	int GetWidth() { return(m_nWidth); }
	int GetLength() { return(m_nLength); }

	virtual float GetHeight(int x, int z, std::shared_ptr<HeightMapRawImage> pHeightMap);
	virtual XMFLOAT4 GetColor(int x, int z, std::shared_ptr<HeightMapRawImage> pHeightMap);

protected:
	int							m_nWidth;
	int							m_nLength;
	XMFLOAT3					m_xmf3Scale;

	std::vector<XMFLOAT4>		m_xmf4Colors{};
	std::vector<XMFLOAT2>		m_xmf2TextureCoords0{};
	std::vector<XMFLOAT2>		m_xmf2TextureCoords1{};

	ComPtr<ID3D12Resource>		m_pd3dColorBuffer = nullptr;
	ComPtr<ID3D12Resource>		m_pd3dColorUploadBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW	m_d3dColorBufferView;

	ComPtr<ID3D12Resource>		m_pd3dTextureCoord0Buffer = nullptr;
	ComPtr<ID3D12Resource>		m_pd3dTextureCoord0UploadBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW	m_d3dTextureCoord0BufferView;

	ComPtr<ID3D12Resource>		m_pd3dTextureCoord1Buffer = nullptr;
	ComPtr<ID3D12Resource>		m_pd3dTextureCoord1UploadBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW	m_d3dTextureCoord1BufferView;


};