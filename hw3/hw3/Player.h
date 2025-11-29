#pragma once
#include "GameObject.h"
#include "Camera.h"

class Player : public GameObject {
public:
	Player();

	virtual void Initialize() {}
	virtual void Update(float fTimeElapsed) override;
	virtual void Animate(float fTimeElapsed) override;

	XMFLOAT3 GetPosition() const { return m_xmf3Position; }
	XMFLOAT3 GetLookVector() const { return m_xmf3Look; }
	XMFLOAT3 GetUpVector() const { return m_xmf3Up; }
	XMFLOAT3 GetRightVector() const { return m_xmf3Right; }

	void SetFriction(float fFriction) { m_fFriction = fFriction; }
	void SetGravity(const XMFLOAT3& xmf3Gravity) { m_xmf3Gravity = xmf3Gravity; }
	void SetMaxVelocityXZ(float fMaxVelocity) { m_fMaxVelocityXZ = fMaxVelocity; }
	void SetMaxVelocityY(float fMaxVelocity) { m_fMaxVelocityY = fMaxVelocity; }
	void SetVelocity(const XMFLOAT3& xmf3Velocity) { m_xmf3Velocity = xmf3Velocity; }
	void SetPosition(const XMFLOAT3& xmf3Position) { Move(XMFLOAT3(xmf3Position.x - m_xmf3Position.x, xmf3Position.y - m_xmf3Position.y, xmf3Position.z - m_xmf3Position.z), false); }

	const XMFLOAT3& GetVelocity() const { return m_xmf3Velocity; }
	float GetYaw() const { return m_fYaw; }
	float GetPitch() const { return m_fPitch; }
	float GetRoll() const { return m_fRoll; }

	std::shared_ptr<Camera> GetCamera() const { return m_pCamera; }
	void SetCamera(std::shared_ptr<Camera> pCamera) { m_pCamera = pCamera; }

	void Move(UINT nDirection, float fDistance, bool bUpdateVelocity = false);
	void Move(const XMFLOAT3& xmf3Shift, bool bUpdateVelocity = false);
	void Move(float fxOffset = 0.0f, float fyOffset = 0.0f, float fzOffset = 0.0f);
	void Rotate(float x, float y, float z);

	virtual void CreateShaderVariables(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	virtual void UpdateShaderVariables(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

	std::shared_ptr<Camera> OnChangeCamera(UINT nNewCameraMode, DWORD nCurrentCameraMode);
	virtual std::shared_ptr<Camera> ChangeCamera(UINT nNewCameraMode, float fTimeElapsed) { return nullptr; }
	virtual void OnPrepareRender() override;

	virtual void AdjustHeightFromTerrain(std::shared_ptr<class TerrainObject> pTerrain) override;

	float GetHP() const { return m_fHP; }


	virtual void AddToRenderMap(bool bTransparent = false) override;

protected:
	XMFLOAT3					m_xmf3Position;
	XMFLOAT3					m_xmf3Right;
	XMFLOAT3					m_xmf3Up;
	XMFLOAT3					m_xmf3Look;

	float           			m_fPitch;
	float           			m_fYaw;
	float           			m_fRoll;

	XMFLOAT3					m_xmf3Velocity;
	XMFLOAT3     				m_xmf3Gravity;
	float           			m_fMaxVelocityXZ;
	float           			m_fMaxVelocityY;
	float           			m_fFriction;

	float						m_fHP = 100.f;

	std::shared_ptr<Camera>		m_pCamera = nullptr;

};

class AirplanePlayer : public Player {
public:
	AirplanePlayer(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, ComPtr<ID3D12RootSignature> pd3dGraphicsRootSignature);
	virtual ~AirplanePlayer();


private:
	virtual void Initialize() override;
	virtual void Animate(float fTimeElapsed) override;

public:
	virtual std::shared_ptr<Camera> ChangeCamera(UINT nNewCameraMode, float fTimeElapsed) override;

public:
	// Collision
	virtual void OnBeginCollision(std::shared_ptr<GameObject> pOther);
	virtual void OnInCollision(std::shared_ptr<GameObject> pOther);
	virtual void OnEndCollision(std::shared_ptr<GameObject> pOther);

private:
	std::shared_ptr<GameObject> m_pMainRotorFrame = nullptr;
	std::shared_ptr<GameObject> m_pTailRotorFrame = nullptr;

};

