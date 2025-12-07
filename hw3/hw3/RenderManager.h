#pragma once

class Mesh;
class Material;
class StructuredBuffer;

#define MAX_INSTANCING_COUNT						500
#define ASSUMED_MESH_PER_INSTANCE					30
#define ASSUMED_MATERIAL_PER_MESH					2
#define ASSUMED_REQUIRED_STRUCTURED_BUFFER_SIZE		MAX_INSTANCING_COUNT * ASSUMED_MESH_PER_INSTANCE * ASSUMED_MATERIAL_PER_MESH

#define ASSUMED_OBJECT_TYPE_PER_SCENE				2
#define ASSUMED_REQUIRED_DESCRIPTOR_COUNT			Scene::g_uiDescriptorCountPerScene + 1 + ASSUMED_MESH_PER_INSTANCE * ASSUMED_MATERIAL_PER_MESH * ASSUMED_OBJECT_TYPE_PER_SCENE

struct INSTANCE_DATA {
	XMFLOAT4X4 xmf4x4World;
};

struct INSTANCE_KEY {
	std::shared_ptr<Mesh>					pMesh;
	std::vector<std::shared_ptr<Material>>	pMaterials;
	UINT									uiDescriptorCountPerInstance;

	bool operator==(const INSTANCE_KEY& other) const noexcept {
		return pMesh == other.pMesh && pMaterials == other.pMaterials;
	}
};

template<>
struct std::hash<INSTANCE_KEY> {
	size_t operator()(const INSTANCE_KEY& key) const {
		size_t h1 = std::hash<std::shared_ptr<Mesh>>{}(key.pMesh);
		size_t h2 = std::hash<std::shared_ptr<Material>>{}(key.pMaterials[0]);
		return h1 ^ h2;
	}
};

class MirrorObject;

class RenderManager {
public:
	RenderManager(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

	void Add(std::shared_ptr<GameObject> pGameObject);
	void AddMirror(std::shared_ptr<MirrorObject> pMirrorObject);
	void AddTransparent(std::shared_ptr<GameObject> pGameObject);

	void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	void Clear();

	void SetTerrain(std::shared_ptr<TerrainObject> pTerrainObject) { m_pTerrain = pTerrainObject; }

	size_t GetMeshCount() const;
	UINT GetDrawCallCount() const { return m_nDrawCalls; };
	ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() { return m_pd3dDescriptorHeap; }
	void SetDescriptorHeapToPipeline(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) const;

public:
	void RenderObjectsInMirrorWorld(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& refDescHandle, 
		const XMFLOAT4& xmf4MirrorPlane, ComPtr<ID3D12PipelineState> pd3dObjectsOnMirrorPipelineState, ComPtr<ID3D12PipelineState> pd3dTerrainOnMirrorPipelineState, ComPtr<ID3D12PipelineState> pd3dBillboardsOnMirrorPipelineState);

private:
	void RenderObjects(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& refDescHandle);
	void RenderTerrain(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& refDescHandle);
	void RenderMirrors(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& refDescHandle);
	void RenderTransparent(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& refDescHandle);
	void RenderSkybox(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& refDescHandle);

	void CreateGlobalRootSignature(ComPtr<ID3D12Device> pd3dDevice);

private:
	void FilterObjectsInSceneBehindMirror(const XMFLOAT4& xmf4MirrorPlane);
	void AddInMirrorSpace(std::shared_ptr<GameObject> pGameObject, const XMFLOAT4& xmf4MirrorPlane);

private:
	std::unordered_map<INSTANCE_KEY, UINT> m_InstanceIndexMap;
	std::vector<std::pair<INSTANCE_KEY, std::vector<INSTANCE_DATA>>> m_InstanceDatas;
	UINT m_nInstanceIndex = 0;

	std::vector<std::shared_ptr<MirrorObject>> m_pMirrorObjects;
	std::unordered_map<INSTANCE_KEY, UINT> m_InstanceInMirrorIndexMap;
	std::vector<std::pair<INSTANCE_KEY, std::vector<INSTANCE_DATA>>> m_InstanceInMirrorDatas;
	UINT m_nInstanceInMirrorIndex = 0;

	// 인스턴싱 불가능함
	// 카메라 기준 정렬이 필요한데 동시에 그리는건 말이 안됨
	std::vector<std::shared_ptr<GameObject>> m_pTransparentObjects;

	std::shared_ptr<class TerrainObject>	m_pTerrain;
	
	ComPtr<ID3D12Device>			m_pd3dDevice = nullptr;	// GameFramewok::m_pd3dDevice 의 참조
	ComPtr<ID3D12DescriptorHeap>	m_pd3dDescriptorHeap = nullptr;
	DescriptorHandle				m_DescriptorHandle;

	StructuredBuffer				m_InstanceDataSBuffer;
	StructuredBuffer				m_InstanceDataOnMirrorSBuffer;
	StructuredBuffer				m_InstanceDataTransparentSBuffer;
	ConstantBuffer					m_LightOnMirrorCBuffer;

	UINT m_nDrawCalls = 0;

public:
	static ComPtr<ID3D12RootSignature> g_pd3dRootSignature;
	static bool g_bRenderOBBForDebug;
};
