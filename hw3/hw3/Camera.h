#pragma once

#define ASPECT_RATIO				(float(GameFramework::g_nClientWidth) / float(GameFramework::g_nClientHeight))

struct CB_CAMERA_DATA {
	XMFLOAT4X4						m_xmf4x4View;
	XMFLOAT4X4						m_xmf4x4Projection;
	XMFLOAT3						m_xmf3Position;
};

enum CAMERA_MODE : UINT {
	CAMERA_MODE_THIRD_PERSON = 1,
	CAMERA_MODE_FIRST_PERSON,
	CAMERA_MODE_SPACESHIP,
};

class Player;

class Camera {
public:
	Camera();
	Camera(const std::shared_ptr<Camera> pOther);

	virtual void CreateShaderVariables(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	virtual void UpdateShaderVariables(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	void SetCameraToPipeline(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT uiRootParameterIndex);


	void GenerateViewMatrix();
	void GenerateViewMatrix(XMFLOAT3 xmf3Position, XMFLOAT3 xmf3LookAt, XMFLOAT3 xmf3Up);
	void RegenerateViewMatrix();

	void GenerateProjectionMatrix(float fNearPlaneDistance, float fFarPlaneDistance, float fAspectRatio, float fFOVAngle);

	void SetViewport(int xTopLeft, int yTopLeft, int nWidth, int nHeight, float fMinZ = 0.0f, float fMaxZ = 1.0f);
	void SetScissorRect(LONG xLeft, LONG yTop, LONG xRight, LONG yBottom);

	virtual void SetViewportsAndScissorRects(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

	const BoundingFrustum& GetFrustum() const { return m_xmFrustumWorld; }
	bool IsInFrustum(const BoundingOrientedBox& xmBoundingBox) const;
	bool IsInFrustum(const XMFLOAT3& xmf3Point) const;

	void SetPlayer(std::shared_ptr<Player> pPlayer) { m_pPlayer = pPlayer; }
	std::shared_ptr<Player> GetPlayer() const { return m_pPlayer; }

	void SetMode(UINT nMode) { m_nMode = nMode; }
	UINT GetMode() const { return m_nMode; }

	void SetPosition(XMFLOAT3 xmf3Position) { m_xmf3Position = xmf3Position; }
	XMFLOAT3& GetPosition() { return m_xmf3Position; }

	void SetLookAtPosition(XMFLOAT3 xmf3LookAtWorld) { m_xmf3LookAtWorld = xmf3LookAtWorld; }
	XMFLOAT3& GetLookAtPosition() { return m_xmf3LookAtWorld; }

	XMFLOAT3& GetRightVector() { return m_xmf3Right; }
	XMFLOAT3& GetUpVector() { return m_xmf3Up; }
	XMFLOAT3& GetLookVector() { return m_xmf3Look; }

	float& GetPitch() { return m_fPitch; }
	float& GetRoll() { return m_fRoll; }
	float& GetYaw() { return m_fYaw; }

	void SetOffset(XMFLOAT3 xmf3Offset) { m_xmf3Offset = xmf3Offset; }
	XMFLOAT3& GetOffset() { return m_xmf3Offset; }

	void SetTimeLag(float fTimeLag) { m_fTimeLag = fTimeLag; }
	float GetTimeLag()  const { return m_fTimeLag; }

	XMFLOAT4X4 GetViewMatrix() const { return m_xmf4x4View; } const
	XMFLOAT4X4 GetProjectionMatrix() const { return m_xmf4x4Projection; }
	D3D12_VIEWPORT GetViewport() const { return m_d3dViewport; }
	D3D12_RECT GetScissorRect() const { return m_d3dScissorRect; }

	virtual void Move(const XMFLOAT3& xmf3Shift) { m_xmf3Position.x += xmf3Shift.x; m_xmf3Position.y += xmf3Shift.y; m_xmf3Position.z += xmf3Shift.z; }
	virtual void Rotate(float fPitch = 0.0f, float fYaw = 0.0f, float fRoll = 0.0f) {}
	virtual void Update(const XMFLOAT3& xmf3LookAt, float fTimeElapsed) {}
	virtual void SetLookAt(const XMFLOAT3& xmf3LookAt) {}

	const ConstantBuffer& GetCBuffer() const { return m_CameraCBuffer; }

protected:
	XMFLOAT3						m_xmf3Position;
	XMFLOAT3						m_xmf3Right;
	XMFLOAT3						m_xmf3Up;
	XMFLOAT3						m_xmf3Look;

	float           				m_fPitch;
	float           				m_fRoll;
	float           				m_fYaw;

	UINT							m_nMode;

	XMFLOAT3						m_xmf3LookAtWorld;
	XMFLOAT3						m_xmf3Offset;
	float           				m_fTimeLag;

	XMFLOAT4X4						m_xmf4x4View;
	XMFLOAT4X4						m_xmf4x4InverseView;
	XMFLOAT4X4						m_xmf4x4Projection;

	D3D12_VIEWPORT					m_d3dViewport;
	D3D12_RECT						m_d3dScissorRect;

	BoundingFrustum					m_xmFrustumView;
	BoundingFrustum					m_xmFrustumWorld;

	std::shared_ptr<Player>			m_pPlayer = nullptr;

	ConstantBuffer					m_CameraCBuffer;

};

class SpaceShipCamera : public Camera {
public:
	SpaceShipCamera();
	SpaceShipCamera(const std::shared_ptr<Camera> pOther);
	virtual ~SpaceShipCamera() {}

	virtual void Rotate(float fPitch = 0.0f, float fYaw = 0.0f, float fRoll = 0.0f) override;
};

class FirstPersonCamera : public Camera {
public:
	FirstPersonCamera();
	FirstPersonCamera(const std::shared_ptr<Camera> pOther);
	virtual ~FirstPersonCamera() {}

	virtual void Update(const XMFLOAT3& xmf3LookAt, float fTimeElapsed) override;
	virtual void Rotate(float fPitch = 0.0f, float fYaw = 0.0f, float fRoll = 0.0f) override;
	virtual void SetLookAt(const XMFLOAT3& xmf3LookAt) override;
};

class ThirdPersonCamera : public Camera {
public:
	ThirdPersonCamera();
	ThirdPersonCamera(const std::shared_ptr<Camera> pOther);
	virtual ~ThirdPersonCamera() {}

	virtual void Update(const XMFLOAT3& xmf3LookAt, float fTimeElapsed) override;
	virtual void SetLookAt(const XMFLOAT3& xmf3LookAt) override;
};

