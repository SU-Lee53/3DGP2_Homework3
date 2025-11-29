#pragma once
#include "GameObject.h"

// 목표 : 63빌딩처럼 사방이 유리인 빌딩을 만든다
// 그러기 위해서는 육면체를 바로 만드는것이 아닌 각 건물 면을 사각형으로 따로 잡고, 
// 각각을 MirrorObject 같은 형태로 구현하는것이 수월하지 않나 싶음
//

class MirrorObject : public GameObject {
public:
	MirrorObject();
	virtual ~MirrorObject();

public:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	virtual void UpdateTransform(XMFLOAT4X4* pxmf4x4Parent = NULL) override;

	virtual void AddToRenderMap(bool bTransparent = false) override;
	void Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& refDescHandle);

	void SetBlendFactor(float fFactor) { m_fBlendFactor = fFactor; }

private:
	XMFLOAT4 m_xmf4MirrorPlane;
	float m_fBlendFactor = 0.f;

	ConstantBuffer m_WorldCBuffer;

};

class BuildingObject : public GameObject {
public:
	BuildingObject();
	virtual ~BuildingObject();

public:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		float fWidth, float fLength, float fHeight, int nWindowsInWidth, int nWindowsInHeights);

protected:
	std::shared_ptr<GameObject> m_pMainRotorFrame = nullptr;
	std::shared_ptr<GameObject> m_pTailRotorFrame = nullptr;

};

