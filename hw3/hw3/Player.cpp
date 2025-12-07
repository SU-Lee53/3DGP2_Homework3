#include "stdafx.h"
#include "Player.h"
#include "TerrainObject.h"

Player::Player()
{
	m_pCamera = nullptr;

	m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_xmf3Right = XMFLOAT3(1.0f, 0.0f, 0.0f);
	m_xmf3Up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	m_xmf3Look = XMFLOAT3(0.0f, 0.0f, 1.0f);

	m_xmf3Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_xmf3Gravity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_fMaxVelocityXZ = 0.0f;
	m_fMaxVelocityY = 0.0f;
	m_fFriction = 0.0f;

	m_fPitch = 0.0f;
	m_fRoll = 0.0f;
	m_fYaw = 0.0f;
}

void Player::Update(float fTimeElapsed)
{
	m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, m_xmf3Gravity);
	float fLength = sqrtf(m_xmf3Velocity.x * m_xmf3Velocity.x + m_xmf3Velocity.z * m_xmf3Velocity.z);
	float fMaxVelocityXZ = m_fMaxVelocityXZ;
	if (fLength > m_fMaxVelocityXZ)
	{
		m_xmf3Velocity.x *= (fMaxVelocityXZ / fLength);
		m_xmf3Velocity.z *= (fMaxVelocityXZ / fLength);
	}
	float fMaxVelocityY = m_fMaxVelocityY;
	fLength = sqrtf(m_xmf3Velocity.y * m_xmf3Velocity.y);
	if (fLength > m_fMaxVelocityY) m_xmf3Velocity.y *= (fMaxVelocityY / fLength);

	XMFLOAT3 xmf3Velocity = Vector3::ScalarProduct(m_xmf3Velocity, fTimeElapsed, false);
	Move(xmf3Velocity, false);

	UINT nCurrentCameraMode = m_pCamera->GetMode();
	if (nCurrentCameraMode == CAMERA_MODE_THIRD_PERSON) {
		m_pCamera->Update(m_xmf3Position, fTimeElapsed);
		m_pCamera->SetLookAt(m_xmf3Position);
	}
	
	if (nCurrentCameraMode == CAMERA_MODE_FIRST_PERSON) {
		m_pCamera->Update(m_xmf3Position, fTimeElapsed);
		m_pCamera->SetLookAt(m_xmf3Look);
	}

	m_pCamera->RegenerateViewMatrix();

	fLength = Vector3::Length(m_xmf3Velocity);
	float fDeceleration = (m_fFriction * fTimeElapsed);
	if (fDeceleration > fLength) fDeceleration = fLength;
	m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, Vector3::ScalarProduct(m_xmf3Velocity, -fDeceleration, true));

	Animate(fTimeElapsed);

}

void Player::Animate(float fTimeElapsed)
{
	for (auto pChild : m_pChildren) {
		pChild->Animate(fTimeElapsed);
	}
}

void Player::Move(UINT uiDirection, float fDistance, bool bUpdateVelocity)
{
	if (uiDirection)
	{
		XMFLOAT3 xmf3Shift = XMFLOAT3(0, 0, 0);
		if (uiDirection & MOVE_DIR_FORWARD) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Look, fDistance);
		if (uiDirection & MOVE_DIR_BACKWARD) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Look, -fDistance);
		if (uiDirection & MOVE_DIR_RIGHT) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Right, fDistance);
		if (uiDirection & MOVE_DIR_LEFT) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Right, -fDistance);
		if (uiDirection & MOVE_DIR_UP) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Up, fDistance);
		if (uiDirection & MOVE_DIR_DOWN) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Up, -fDistance);

		Move(xmf3Shift, bUpdateVelocity);
	}
}

void Player::Move(const XMFLOAT3& xmf3Shift, bool bUpdateVelocity)
{
	if (bUpdateVelocity)
	{
		m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, xmf3Shift);
	}
	else
	{
		m_xmf3Position = Vector3::Add(m_xmf3Position, xmf3Shift);
		m_pCamera->Move(xmf3Shift);
	}

	UpdateTransform(nullptr);
}

void Player::Move(float fxOffset, float fyOffset, float fzOffset)
{
}

void Player::Rotate(float x, float y, float z)
{
	UINT nCurrentCameraMode = m_pCamera->GetMode();
	if ((nCurrentCameraMode == CAMERA_MODE_FIRST_PERSON) || (nCurrentCameraMode == CAMERA_MODE_THIRD_PERSON))
	{
		if (x != 0.0f)
		{
			m_fPitch += x;
			if (m_fPitch > +89.0f) { x -= (m_fPitch - 89.0f); m_fPitch = +89.0f; }
			if (m_fPitch < -89.0f) { x -= (m_fPitch + 89.0f); m_fPitch = -89.0f; }
		}
		if (y != 0.0f)
		{
			m_fYaw += y;
			if (m_fYaw > 360.0f) m_fYaw -= 360.0f;
			if (m_fYaw < 0.0f) m_fYaw += 360.0f;
		}
		if (z != 0.0f)
		{
			m_fRoll += z;
			if (m_fRoll > +20.0f) { z -= (m_fRoll - 20.0f); m_fRoll = +20.0f; }
			if (m_fRoll < -20.0f) { z -= (m_fRoll + 20.0f); m_fRoll = -20.0f; }
		}
		m_pCamera->Rotate(x, y, z);

		//if (nCurrentCameraMode == CAMERA_MODE_FIRST_PERSON) {
		//	if (x != 0.0f)
		//	{
		//		XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Right), XMConvertToRadians(x));
		//		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
		//		m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
		//	}
		//}

		if (y != 0.0f)
		{
			XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Up), XMConvertToRadians(y));
			m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
			m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		}
	}
	else if (nCurrentCameraMode == CAMERA_MODE_SPACESHIP)
	{
		m_pCamera->Rotate(x, y, z);
		if (x != 0.0f)
		{
			XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Right), XMConvertToRadians(x));
			m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
			m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
		}
		if (y != 0.0f)
		{
			XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Up), XMConvertToRadians(y));
			m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
			m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		}
		if (z != 0.0f)
		{
			XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Look), XMConvertToRadians(z));
			m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
			m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		}
	}

	m_xmf3Look = Vector3::Normalize(m_xmf3Look);
	m_xmf3Right = Vector3::CrossProduct(m_xmf3Up, m_xmf3Look, true);
	m_xmf3Up = Vector3::CrossProduct(m_xmf3Look, m_xmf3Right, true);
}

void Player::CreateShaderVariables(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	if (m_pCamera) {
		m_pCamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	}
}

void Player::UpdateShaderVariables(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
}

std::shared_ptr<Camera> Player::OnChangeCamera(UINT nNewCameraMode, DWORD nCurrentCameraMode)
{
	std::shared_ptr<Camera> pNewCamera;

	switch (nNewCameraMode)
	{
	case CAMERA_MODE_FIRST_PERSON:
		pNewCamera = std::make_shared<FirstPersonCamera>(m_pCamera);
		break;
	case CAMERA_MODE_THIRD_PERSON:
		pNewCamera = std::make_shared<ThirdPersonCamera>(m_pCamera);
		break;
	case CAMERA_MODE_SPACESHIP:
		pNewCamera = std::make_shared<SpaceShipCamera>(m_pCamera);
		break;
	}
	if (nCurrentCameraMode == CAMERA_MODE_SPACESHIP)
	{
		m_xmf3Right = Vector3::Normalize(XMFLOAT3(m_xmf3Right.x, 0.0f, m_xmf3Right.z));
		m_xmf3Up = Vector3::Normalize(XMFLOAT3(0.0f, 1.0f, 0.0f));
		m_xmf3Look = Vector3::Normalize(XMFLOAT3(m_xmf3Look.x, 0.0f, m_xmf3Look.z));

		m_fPitch = 0.0f;
		m_fRoll = 0.0f;
		m_fYaw = Vector3::Angle(XMFLOAT3(0.0f, 0.0f, 1.0f), m_xmf3Look);
		if (m_xmf3Look.x < 0.0f) m_fYaw = -m_fYaw;
	}
	else if ((nNewCameraMode == CAMERA_MODE_SPACESHIP) && m_pCamera)
	{
		m_xmf3Right = m_pCamera->GetRightVector();
		m_xmf3Up = m_pCamera->GetUpVector();
		m_xmf3Look = m_pCamera->GetLookVector();
	}

	if (pNewCamera)
	{
		pNewCamera->SetMode(nNewCameraMode);
		pNewCamera->SetPlayer(static_pointer_cast<Player>(shared_from_this()));
	}


	return(pNewCamera);
}

void Player::OnPrepareRender()
{
	m_xmf4x4Transform._11 = m_xmf3Right.x; m_xmf4x4Transform._12 = m_xmf3Right.y; m_xmf4x4Transform._13 = m_xmf3Right.z;
	m_xmf4x4Transform._21 = m_xmf3Up.x; m_xmf4x4Transform._22 = m_xmf3Up.y; m_xmf4x4Transform._23 = m_xmf3Up.z;
	m_xmf4x4Transform._31 = m_xmf3Look.x; m_xmf4x4Transform._32 = m_xmf3Look.y; m_xmf4x4Transform._33 = m_xmf3Look.z;
	m_xmf4x4Transform._41 = m_xmf3Position.x; m_xmf4x4Transform._42 = m_xmf3Position.y; m_xmf4x4Transform._43 = m_xmf3Position.z;

	UpdateTransform(NULL);
}

void Player::AdjustHeightFromTerrain(std::shared_ptr<class TerrainObject> pTerrain)
{
	// 11.14
	// TODO : 플레이어가 Terrain 범위를 벗어나면 터지는것만 고치자
	XMFLOAT3 xmf3Scale = pTerrain->GetScale();
	XMFLOAT3 xmf3PlayerPosition = GetPosition();
	int z = (int)(xmf3PlayerPosition.z / xmf3Scale.z);
	bool bReverseQuad = ((z % 2) != 0);
	float fHeight = pTerrain->GetHeight(xmf3PlayerPosition.x, xmf3PlayerPosition.z, bReverseQuad) + m_fHalfHeight;
	if (xmf3PlayerPosition.y < fHeight)
	{
		XMFLOAT3 xmf3PlayerVelocity = GetVelocity();
		float fDamage = XMVectorGetX(XMVector3LengthEst(XMLoadFloat3(&xmf3PlayerVelocity)));
		if (fDamage > 130.f) {
			m_fHP -= (fDamage * 0.05f);
			if (m_fHP < 0.f) {
				m_fHP = 0.f;
			}
		}

		xmf3PlayerVelocity.y = 0.0f;
		SetVelocity(xmf3PlayerVelocity);
		xmf3PlayerPosition.y = fHeight;
		SetPosition(xmf3PlayerPosition);

	}
}

void Player::AddToRenderMap(bool bTransparent)
{
	UINT nCurrentCameraMode = (m_pCamera) ? m_pCamera->GetMode() : 0x00;
	bTransparent = nCurrentCameraMode == CAMERA_MODE_FIRST_PERSON;
	if (m_pMesh) {
		if (bTransparent) {
			RENDER->AddTransparent(shared_from_this());
		}
		else {
			RENDER->Add(shared_from_this());
		}
	}

	for (auto& pChild : m_pChildren) {
		pChild->AddToRenderMap(bTransparent);
	}
}

AirplanePlayer::AirplanePlayer(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, ComPtr<ID3D12RootSignature> pd3dGraphicsRootSignature)
{
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

AirplanePlayer::~AirplanePlayer()
{
}

void AirplanePlayer::Initialize()
{
	m_pMainRotorFrame = FindFrame("MainRotor");
	m_pTailRotorFrame = FindFrame("TailRotor");

	m_pCamera->SetPosition(Vector3::Add(m_xmf3Position, m_pCamera->GetOffset()));
	Update(0.0f);

	GenerateBigBoundingBox();
}

void AirplanePlayer::Animate(float fTimeElapsed)
{
	if (m_pMainRotorFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationY(XMConvertToRadians(360.0f * 2.0f) * fTimeElapsed);
		m_pMainRotorFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pMainRotorFrame->m_xmf4x4Transform);
	}
	if (m_pTailRotorFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationX(XMConvertToRadians(360.0f * 4.0f) * fTimeElapsed);
		m_pTailRotorFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pTailRotorFrame->m_xmf4x4Transform);
	}

	Player::Animate(fTimeElapsed);
}

std::shared_ptr<Camera> AirplanePlayer::ChangeCamera(UINT nNewCameraMode, float fTimeElapsed)
{
	UINT nCurrentCameraMode = (m_pCamera) ? m_pCamera->GetMode() : 0x00;
	if (nCurrentCameraMode == nNewCameraMode) return(m_pCamera);
	switch (nNewCameraMode)
	{
	case CAMERA_MODE_THIRD_PERSON:
		SetFriction(20.f);
		SetGravity(XMFLOAT3(0.0f, 0.0f, 0.0f));
		SetMaxVelocityXZ(500.f);
		SetMaxVelocityY(300.0f);
		m_pCamera = OnChangeCamera(CAMERA_MODE_THIRD_PERSON, nCurrentCameraMode);
		m_pCamera->SetTimeLag(0.25f);
		m_pCamera->SetOffset(XMFLOAT3(0.0f, 105.0f, -140.0f));
		m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
		m_pCamera->SetViewport(0, 0, GameFramework::g_nClientWidth, GameFramework::g_nClientHeight, 0.0f, 1.0f);
		m_pCamera->SetScissorRect(0, 0, GameFramework::g_nClientWidth, GameFramework::g_nClientHeight);
		break;
	case CAMERA_MODE_FIRST_PERSON:
		SetFriction(20.f);
		SetGravity(XMFLOAT3(0.0f, 0.0f, 0.0f));
		SetMaxVelocityXZ(500.f);
		SetMaxVelocityY(300.0f);
		m_pCamera = OnChangeCamera(CAMERA_MODE_FIRST_PERSON, nCurrentCameraMode);
		m_pCamera->SetTimeLag(0.0f);
		m_pCamera->SetOffset(XMFLOAT3(0.0f, 20.0f, 30.5f));
		m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
		m_pCamera->SetViewport(0, 0, GameFramework::g_nClientWidth, GameFramework::g_nClientHeight, 0.0f, 1.0f);
		m_pCamera->SetScissorRect(0, 0, GameFramework::g_nClientWidth, GameFramework::g_nClientHeight);
		break;
	case CAMERA_MODE_SPACESHIP:
		SetFriction(100.5f);
		SetGravity(XMFLOAT3(0.0f, 0.0f, 0.0f));
		SetMaxVelocityXZ(40.0f);
		SetMaxVelocityY(40.0f);
		m_pCamera = OnChangeCamera(CAMERA_MODE_SPACESHIP, nCurrentCameraMode);
		m_pCamera->SetTimeLag(0.0f);
		m_pCamera->SetOffset(XMFLOAT3(0.0f, 0.0f, 0.0f));
		m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
		m_pCamera->SetViewport(0, 0, GameFramework::g_nClientWidth, GameFramework::g_nClientHeight, 0.0f, 1.0f);
		m_pCamera->SetScissorRect(0, 0, GameFramework::g_nClientWidth, GameFramework::g_nClientHeight);
		break;
	default:
		break;
	}

	m_pCamera->SetPosition(Vector3::Add(m_xmf3Position, m_pCamera->GetOffset()));
	Update(fTimeElapsed);

	return(m_pCamera);
}

void AirplanePlayer::OnBeginCollision(std::shared_ptr<GameObject> pOther)
{
	XMFLOAT3 xmf3PlayerVelocity = GetVelocity();
	float fDamage = XMVectorGetX(XMVector3LengthEst(XMLoadFloat3(&xmf3PlayerVelocity)));
	if (fDamage > 100.f) {
		m_fHP -= (fDamage * 0.05f);
		if (m_fHP < 0.f) {
			m_fHP = 0.f;
		}
	}

	xmf3PlayerVelocity.x = -1.f * xmf3PlayerVelocity.x;
	xmf3PlayerVelocity.y = -1.f * xmf3PlayerVelocity.y;
	xmf3PlayerVelocity.z = -1.f * xmf3PlayerVelocity.z;
	SetVelocity(xmf3PlayerVelocity);
}

void AirplanePlayer::OnInCollision(std::shared_ptr<GameObject> pOther)
{
	XMFLOAT3 xmf3PlayerVelocity = GetVelocity();
	xmf3PlayerVelocity.x = -1.f * xmf3PlayerVelocity.x;
	xmf3PlayerVelocity.y = -1.f * xmf3PlayerVelocity.y;
	xmf3PlayerVelocity.z = -1.f * xmf3PlayerVelocity.z;
	SetVelocity(xmf3PlayerVelocity);
}

void AirplanePlayer::OnEndCollision(std::shared_ptr<GameObject> pOther)
{
}