#pragma once
#include "Mesh.h"
#include "Material.h"

enum MOVE_DIR : UINT {
	MOVE_DIR_FORWARD		= 1,
	MOVE_DIR_BACKWARD	= MOVE_DIR_FORWARD << 1,
	MOVE_DIR_LEFT		= MOVE_DIR_BACKWARD << 1,
	MOVE_DIR_RIGHT		= MOVE_DIR_LEFT << 1,
	MOVE_DIR_UP			= MOVE_DIR_RIGHT << 1,
	MOVE_DIR_DOWN		= MOVE_DIR_UP << 1,
};

struct CB_OBJECT_DATA {
	XMFLOAT4X4 xmf4x4World;
};

struct CB_OBB_DEBUG_DATA {
	XMFLOAT3 gvOBBCenter;
	XMFLOAT3 gvOBBExtent;
	XMFLOAT4 gvOBBOrientationQuat;
};

class Camera;

// 12.07
// TODO : 이전 프레임의 World 행렬 보관 필요
// 나중에 렌더링시 motion vector 를 구하기 위함

class GameObject : public std::enable_shared_from_this<GameObject> {
public:
	GameObject();

public:
	void SetMesh(std::shared_ptr<Mesh> pMesh);
	void SetShader(std::shared_ptr<Shader> pShader);
	void SetShader(int nMaterial, std::shared_ptr<Shader> pShader);
	void SetMaterial(int nMaterial, std::shared_ptr<Material> pMaterial);

	void SetChild(std::shared_ptr<GameObject> pChild);
	const std::vector<std::shared_ptr<GameObject>>& GetChildren() const;

public:
	virtual void Initialize();
	virtual void Update(float fTimeElapsed);
	virtual void Animate(float fTimeElapsed);

	virtual void RenderOBB(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

	virtual void AdjustHeightFromTerrain(std::shared_ptr<class TerrainObject> pTerrain);

	XMFLOAT3 GetPosition();
	XMFLOAT3 GetLook();
	XMFLOAT3 GetUp();
	XMFLOAT3 GetRight();

	void SetPosition(float x, float y, float z);
	void SetPosition(XMFLOAT3 xmf3Position);
	void SetScale(float x, float y, float z);

	void MoveStrafe(float fDistance = 1.0f);
	void MoveUp(float fDistance = 1.0f);
	void MoveForward(float fDistance = 1.0f);

	void Rotate(float fPitch = 10.0f, float fYaw = 10.0f, float fRoll = 10.0f);
	void Rotate(XMFLOAT3* pxmf3Axis, float fAngle);
	void Rotate(XMFLOAT4* pxmf4Quaternion);

	void CacheLastFrameTransform();

	std::shared_ptr<GameObject> GetParent() { return m_pParent; }
	virtual void UpdateTransform(XMFLOAT4X4* pxmf4x4Parent = NULL);
	std::shared_ptr<GameObject> FindFrame(const std::string& strFrameName);

	UINT GetMeshType() { return (m_pMesh) ? m_pMesh->GetType() : 0; }

	bool IsInFrustum(std::shared_ptr<Camera> pCamera);
	BoundingOrientedBox GetOBB() const { return m_xmOBBInWorld; };

public:
	void GenerateBigBoundingBox(bool bFlipYZ = false, bool bCenterOnFloor = false);

private:
	void UpdateMinMaxInBoundingBox(float& fMinX, float& fMaxX, float& fMinY, float& fMaxY, float& fMinZ, float& fMaxZ);

public:
	const XMFLOAT4X4& GetWorldMatrix() const { return m_xmf4x4World; }
	const XMFLOAT4X4& GetPrevWorldMatrix() const { return m_xmf4x4CachedLastFrameTransform; }
	std::shared_ptr<Mesh> GetMesh() const { return m_pMesh; }
	std::vector<std::shared_ptr<Material>>& GetMaterials() { return m_pMaterials; }

	void SetName(const std::string& strName) { m_strFrameName = strName; }
	const std::string& GetName() const { return m_strFrameName; }

public:
	virtual void OnPrepareRender() {}
	virtual void AddToRenderMap(bool bTransparent = false);

public:
	void ReleaseUploadBuffers();

public:
	virtual void OnBeginCollision(std::shared_ptr<GameObject> pOther) {}
	virtual void OnInCollision(std::shared_ptr<GameObject> pOther) {}
	virtual void OnEndCollision(std::shared_ptr<GameObject> pOther) {}

	std::unordered_set<std::shared_ptr<GameObject>>& GetCollisionSet() { return m_pCollisionSet; }
	void AddToCollisionSet(std::shared_ptr<GameObject> pOther) { m_pCollisionSet.insert(pOther); }
	void EraseFromCollisionSet(std::shared_ptr<GameObject> pOther) { m_pCollisionSet.erase(m_pCollisionSet.find(pOther)); }
	bool CheckCollisionSet(std::shared_ptr<GameObject> pOther);

	void GenerateRayForPicking(XMVECTOR& xmvPickPosition, const XMMATRIX& xmmtxView, XMVECTOR& xmvPickRayOrigin, XMVECTOR& xmvPickRayDirection) const;
	bool PickObjectByRayIntersection(XMVECTOR& xmvPickPosition, const XMMATRIX& xmmtxView, float& fHitDistance) const;

	void SetExplosible(bool bExplosible) { m_bCanExplode = bExplosible; }
	bool IsExplosible() { return m_bCanExplode; }

public:
	std::string m_strFrameName;

	std::shared_ptr<Mesh> m_pMesh;
	std::vector<std::shared_ptr<Material>> m_pMaterials;

	XMFLOAT4X4 m_xmf4x4CachedLastFrameTransform;
	XMFLOAT4X4 m_xmf4x4Transform;
	XMFLOAT4X4 m_xmf4x4World;

	std::shared_ptr<GameObject> m_pParent;
	std::vector<std::shared_ptr<GameObject>> m_pChildren;

	BoundingOrientedBox m_xmOBB;
	BoundingOrientedBox m_xmOBBInWorld;
	float m_fHalfHeight = 0.f;	// Terrain 충돌 확인용 fHalfHeight

	// Collision
	std::unordered_set<std::shared_ptr<GameObject>> m_pCollisionSet = {};
	
	bool m_bCanExplode = false;

public:
	void LoadMaterialsFromFile(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::ifstream& inFile);

	static std::shared_ptr<GameObject> LoadFrameHierarchyFromFile(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, ComPtr<ID3D12RootSignature> pd3dRootSignature, std::shared_ptr<GameObject> pParent, std::ifstream& inFile);
	static std::shared_ptr<GameObject> LoadGeometryFromFile(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, ComPtr<ID3D12RootSignature> pd3dRootSignature, const std::string& strFilePath);

	static std::shared_ptr<GameObject> CopyObject(const GameObject& srcObject, std::shared_ptr<GameObject> pParent = nullptr);

};

class HellicopterObject : public GameObject {
public:
	HellicopterObject();
	virtual ~HellicopterObject();

protected:
	std::shared_ptr<GameObject> m_pMainRotorFrame = nullptr;
	std::shared_ptr<GameObject> m_pTailRotorFrame = nullptr;

public:
	virtual void Initialize() override;
	virtual void Animate(float fTimeElapsed) override;
};

class ApacheObject : public HellicopterObject {
public:
	ApacheObject();
	virtual ~ApacheObject();

public:
	virtual void Initialize() override;
	virtual void Animate(float fTimeElapsed) override;
};

class GunshipObject : public HellicopterObject {
public:
	GunshipObject();
	virtual ~GunshipObject();

public:
	virtual void Initialize() override;
};

class SuperCobraObject : public HellicopterObject {
public:
	SuperCobraObject();
	virtual ~SuperCobraObject();

public:
	virtual void Initialize() override;
};

class Mi24Object : public HellicopterObject {
public:
	Mi24Object();
	virtual ~Mi24Object();

public:
	virtual void Initialize() override;
};

////////////////////////////////////////////////////////////////////////////////////////////////
//
class HummerObject : public GameObject {
public:
	HummerObject();
	virtual ~HummerObject();

	virtual void Initialize() override;
	virtual void Animate(float fTimeElapsed) override;
protected:
	std::shared_ptr<GameObject> m_pLFWheelFrame = nullptr;
	std::shared_ptr<GameObject> m_pLRWheelFrame = nullptr;
	std::shared_ptr<GameObject> m_pRFWheelFrame = nullptr;
	std::shared_ptr<GameObject> m_pRRWheelFrame = nullptr;

};

////////////////////////////////////////////////////////////////////////////////////////////////
//
class TankObject : public GameObject {
public:
	TankObject();
	virtual ~TankObject();

protected:
	std::shared_ptr<GameObject> m_pTurretFrame = nullptr;
	std::shared_ptr<GameObject> m_pCannonFrame = nullptr;
	std::shared_ptr<GameObject> m_pGunFrame = nullptr;

public:
	virtual void Initialize() override;
	virtual void Animate(float fTimeElapsed) override;
};

class M26Object : public TankObject {
public:
	M26Object();
	virtual ~M26Object();

	virtual void Initialize() override;
};
