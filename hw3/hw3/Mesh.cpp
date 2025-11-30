#include "stdafx.h"
#include "Mesh.h"
#include "TerrainObject.h"

//////////
// Mesh //
//////////

Mesh::Mesh()
{
}

void Mesh::UpdateOBB(XMFLOAT4X4 xmf4x4World)
{
	m_xmOBBInWorld = m_xmOBB;
	m_xmOBB.Transform(m_xmOBBInWorld, XMLoadFloat4x4(&xmf4x4World));
}

void Mesh::ReleaseUploadBuffers()
{
	if (m_pd3dPositionUploadBuffer) {
		m_pd3dPositionUploadBuffer.Reset();
	}

	if (m_nSubMeshes && m_pd3dSubSetIndexUploadBuffers.size() != 0) {
		for (int i = 0; i < m_nSubMeshes; ++i) {
			if (m_pd3dSubSetIndexUploadBuffers[i]) {
				m_pd3dSubSetIndexUploadBuffers[i].Reset();
			}
		}

		m_pd3dSubSetIndexUploadBuffers.clear();
	}
}

void Mesh::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, int nSubSet, int nInstanceCount)
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 1, &m_d3dPositionBufferView);
	if ((m_nSubMeshes > 0) && (nSubSet < m_nSubMeshes))
	{
		pd3dCommandList->IASetIndexBuffer(&(m_d3dSubSetIndexBufferViews[nSubSet]));
		pd3dCommandList->DrawIndexedInstanced(m_nSubSetIndices[nSubSet], nInstanceCount, 0, 0, 0);
	}
	else
	{
		pd3dCommandList->DrawInstanced(m_nVertices, nInstanceCount, m_nOffset, 0);
	}
}

//////////////////
// StandardMesh //
//////////////////

void StandardMesh::LoadMeshFromFile(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::ifstream& inFile)
{
	std::string strRead;

	int nVertices;
	inFile.read((char*)(&m_nVertices), sizeof(int));
	m_strMeshName= ::ReadStringFromFile(inFile);

	while (true) {
		strRead = ::ReadStringFromFile(inFile);
		if (strRead == "<Bounds>:") {
			inFile.read((char*)&m_xmf3AABBCenter, sizeof(XMFLOAT3));
			inFile.read((char*)&m_xmf3AABBExtents, sizeof(XMFLOAT3));
			m_xmOBB.Center = m_xmf3AABBCenter;
			m_xmOBB.Extents = m_xmf3AABBExtents;
		}
		else if (strRead == "<Positions>:") {
			int nPositions;
			inFile.read((char*)&nPositions, sizeof(int));
			if (nPositions > 0) {
				m_nType |= VERTEX_TYPE_POSITION;
				m_xmf3Positions.resize(nPositions);
				inFile.read((char*)m_xmf3Positions.data(), sizeof(XMFLOAT3) * nPositions);

				m_pd3dPositionBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_xmf3Positions.data(), m_xmf3Positions.size() * sizeof(XMFLOAT3),
					D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dPositionUploadBuffer.GetAddressOf());

				m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
				m_d3dPositionBufferView.StrideInBytes = sizeof(XMFLOAT3);
				m_d3dPositionBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
			}
		}
		else if (strRead == "<Colors>:") {
			int nColors;
			inFile.read((char*)&nColors, sizeof(int));
			if (nColors > 0) {
				m_nType |= VERTEX_TYPE_COLOR;
				m_xmf4Colors.resize(nColors);
				inFile.read((char*)m_xmf4Colors.data(), sizeof(XMFLOAT4) * nColors);

				// 버퍼 안만듬 (안쓸꺼임)
			}
		}
		else if (strRead == "<TextureCoords0>:") {
			int nTexCoordss;
			inFile.read((char*)&nTexCoordss, sizeof(int));
			if (nTexCoordss > 0) {
				m_nType |= VERTEX_TYPE_TEXTURE_COORD0;
				m_xmf2TextureCoords0.resize(nTexCoordss);
				inFile.read((char*)m_xmf2TextureCoords0.data(), sizeof(XMFLOAT2) * nTexCoordss);

				m_pd3dTextureCoord0Buffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_xmf2TextureCoords0.data(), m_xmf2TextureCoords0.size() * sizeof(XMFLOAT2),
					D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dTextureCoord0UploadBuffer.GetAddressOf());

				m_d3dTextureCoord0BufferView.BufferLocation = m_pd3dTextureCoord0Buffer->GetGPUVirtualAddress();
				m_d3dTextureCoord0BufferView.StrideInBytes = sizeof(XMFLOAT2);
				m_d3dTextureCoord0BufferView.SizeInBytes = sizeof(XMFLOAT2) * m_nVertices;
			}
		}
		else if (strRead == "<TextureCoords1>:") {
			int nTexCoordss;
			inFile.read((char*)&nTexCoordss, sizeof(int));
			if (nTexCoordss > 1) {
				m_nType |= VERTEX_TYPE_TEXTURE_COORD1;
				m_xmf2TextureCoords1.resize(nTexCoordss);
				inFile.read((char*)m_xmf2TextureCoords1.data(), sizeof(XMFLOAT2) * nTexCoordss);

				m_pd3dTextureCoord1Buffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_xmf2TextureCoords1.data(), m_xmf2TextureCoords1.size() * sizeof(XMFLOAT2),
					D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dTextureCoord1UploadBuffer.GetAddressOf());

				m_d3dTextureCoord1BufferView.BufferLocation = m_pd3dTextureCoord1Buffer->GetGPUVirtualAddress();
				m_d3dTextureCoord1BufferView.StrideInBytes = sizeof(XMFLOAT2);
				m_d3dTextureCoord1BufferView.SizeInBytes = sizeof(XMFLOAT2) * m_nVertices;
			}
		}
		else if (strRead == "<Normals>:") {
			int nNormals;
			inFile.read((char*)&nNormals, sizeof(int));
			if (nNormals > 0) {
				m_nType |= VERTEX_TYPE_NORMAL;
				m_xmf3Normals.resize(nNormals);
				inFile.read((char*)m_xmf3Normals.data(), sizeof(XMFLOAT3) * nNormals);

				m_pd3dNormalBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_xmf3Normals.data(), m_xmf3Normals.size() * sizeof(XMFLOAT3),
					D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dNormalUploadBuffer.GetAddressOf());

				m_d3dNormalBufferView.BufferLocation = m_pd3dNormalBuffer->GetGPUVirtualAddress();
				m_d3dNormalBufferView.StrideInBytes = sizeof(XMFLOAT3);
				m_d3dNormalBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
			}
		}
		else if (strRead == "<Tangents>:") {
			int nTangents;
			inFile.read((char*)&nTangents, sizeof(int));
			if (nTangents > 0) {
				m_nType |= VERTEX_TYPE_TANGENT;
				m_xmf3Tangents.resize(nTangents);
				inFile.read((char*)m_xmf3Tangents.data(), sizeof(XMFLOAT3) * nTangents);

				m_pd3dTangentBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_xmf3Tangents.data(), m_xmf3Tangents.size() * sizeof(XMFLOAT3),
					D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dTangentUploadBuffer.GetAddressOf());

				m_d3dTangentBufferView.BufferLocation = m_pd3dTangentBuffer->GetGPUVirtualAddress();
				m_d3dTangentBufferView.StrideInBytes = sizeof(XMFLOAT3);
				m_d3dTangentBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
			}
		}
		else if (strRead == "<BiTangents>:") {
			int nBiTangents;
			inFile.read((char*)&nBiTangents, sizeof(int));
			if (nBiTangents > 0) {
				m_xmf3BiTangents.resize(nBiTangents);
				inFile.read((char*)m_xmf3BiTangents.data(), sizeof(XMFLOAT3) * nBiTangents);

				m_pd3dBiTangentBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_xmf3BiTangents.data(), m_xmf3BiTangents.size() * sizeof(XMFLOAT3),
					D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dBiTangentUploadBuffer.GetAddressOf());

				m_d3dBiTangentBufferView.BufferLocation = m_pd3dBiTangentBuffer->GetGPUVirtualAddress();
				m_d3dBiTangentBufferView.StrideInBytes = sizeof(XMFLOAT3);
				m_d3dBiTangentBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
			}
		}
		else if (strRead == "<SubMeshes>:") {
			inFile.read((char*)&m_nSubMeshes, sizeof(int));
			if (m_nSubMeshes > 0) {
				m_nSubSetIndices.resize(m_nSubMeshes);
				m_IndicesBySubset.resize(m_nSubMeshes);

				m_pd3dSubSetIndexBuffers.resize(m_nSubMeshes);
				m_pd3dSubSetIndexUploadBuffers.resize(m_nSubMeshes);
				m_d3dSubSetIndexBufferViews.resize(m_nSubMeshes);

				for (int i = 0; i < m_nSubMeshes; ++i) {
					strRead = ::ReadStringFromFile(inFile);
					if (strRead == "<SubMesh>:") {
						int nIndex;
						inFile.read((char*)&nIndex, sizeof(int));
						inFile.read((char*)&m_nSubSetIndices[i], sizeof(int));
						if (m_nSubSetIndices[i] > 0) {
							m_IndicesBySubset[i].resize(m_nSubSetIndices[i]);
							inFile.read((char*)m_IndicesBySubset[i].data(), sizeof(UINT) * m_nSubSetIndices[i]);

							m_pd3dSubSetIndexBuffers[i] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_IndicesBySubset[i].data(), m_IndicesBySubset[i].size() * sizeof(UINT),
								D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, m_pd3dSubSetIndexUploadBuffers[i].GetAddressOf());

							m_d3dSubSetIndexBufferViews[i].BufferLocation = m_pd3dSubSetIndexBuffers[i]->GetGPUVirtualAddress();
							m_d3dSubSetIndexBufferViews[i].Format = DXGI_FORMAT_R32_UINT;
							m_d3dSubSetIndexBufferViews[i].SizeInBytes = sizeof(UINT) * m_nSubSetIndices[i];
						}
					}
				}
			}
		}
		else if ((strRead == "</Mesh>"))
		{
			break;
		}
	}
}

StandardMesh::StandardMesh()
{
}

StandardMesh::~StandardMesh()
{
}

void StandardMesh::ReleaseUploadBuffers()
{
	Mesh::ReleaseUploadBuffers();

	if (m_pd3dTextureCoord0UploadBuffer) {
		m_pd3dTextureCoord0UploadBuffer.Reset();
		m_pd3dTextureCoord0UploadBuffer = nullptr;
	}

	if (m_pd3dTextureCoord1UploadBuffer) {
		m_pd3dTextureCoord1UploadBuffer.Reset();
		m_pd3dTextureCoord1UploadBuffer = nullptr;
	}

	if (m_pd3dNormalUploadBuffer) {
		m_pd3dNormalUploadBuffer.Reset();
		m_pd3dNormalUploadBuffer = nullptr;
	}

	if (m_pd3dTangentUploadBuffer) {
		m_pd3dTangentUploadBuffer.Reset();
		m_pd3dTangentUploadBuffer = nullptr;
	}

	if (m_pd3dBiTangentUploadBuffer) {
		m_pd3dBiTangentUploadBuffer.Reset();
		m_pd3dBiTangentUploadBuffer = nullptr;
	}

}

void StandardMesh::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, int nSubSet, int nInstanceCount)
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	D3D12_VERTEX_BUFFER_VIEW d3dVertexBufferViews[] = {
		m_d3dPositionBufferView,
		m_d3dTextureCoord0BufferView,
		m_d3dNormalBufferView,
		m_d3dTangentBufferView,
		m_d3dBiTangentBufferView
	};

	pd3dCommandList->IASetVertexBuffers(m_nSlot, _countof(d3dVertexBufferViews), d3dVertexBufferViews);
	if ((m_nSubMeshes > 0) && (nSubSet < m_nSubMeshes))
	{
		pd3dCommandList->IASetIndexBuffer(&(m_d3dSubSetIndexBufferViews[nSubSet]));
		pd3dCommandList->DrawIndexedInstanced(m_nSubSetIndices[nSubSet], nInstanceCount, 0, 0, 0);
	}
	else
	{
		pd3dCommandList->DrawInstanced(m_nVertices, nInstanceCount, m_nOffset, 0);
	}
}

std::shared_ptr<StandardMesh> StandardMesh::GenerateMirrorMesh(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, float fWidth, float fHeight, int nWindowsInWidth, int nWindowsInHeights)
{
	/*                              
		   0          1	            
			+--------+              
			|        |              
			|        |              +y
			|        |				 |  +z
			|        | fHeight		 | /
			|        |				 |/
			|        |				 +-------- +x
			+--------+
	      2   fWidth   3
	
		- 일단 Index 안쓰고 정점 6개로 그리도록 함
		- 월드 원점에다 만듬. 방향(Normal)은 -Z 방향을 바라봄
	*/

	std::shared_ptr<StandardMesh> pMirrorMesh = std::make_shared<StandardMesh>();
	pMirrorMesh->m_strMeshName = "Mirror";
	pMirrorMesh->m_nVertices = 6;

	float fHalfWidth = fWidth / 2;
	float fHalfHeight = fHeight / 2;

	// Position
	{
		pMirrorMesh->m_xmf3Positions.resize(6);
		pMirrorMesh->m_xmf3Positions[0] = XMFLOAT3(-fHalfWidth, +fHalfHeight, 0.f);	// 0
		pMirrorMesh->m_xmf3Positions[1] = XMFLOAT3(+fHalfWidth, +fHalfHeight, 0.f);	// 1
		pMirrorMesh->m_xmf3Positions[2] = XMFLOAT3(-fHalfWidth, -fHalfHeight, 0.f);	// 2

		pMirrorMesh->m_xmf3Positions[3] = XMFLOAT3(+fHalfWidth, +fHalfHeight, 0.f);	// 1
		pMirrorMesh->m_xmf3Positions[4] = XMFLOAT3(+fHalfWidth, -fHalfHeight, 0.f);	// 3
		pMirrorMesh->m_xmf3Positions[5] = XMFLOAT3(-fHalfWidth, -fHalfHeight, 0.f);	// 2


		pMirrorMesh->m_pd3dPositionBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pMirrorMesh->m_xmf3Positions.data(), pMirrorMesh->m_xmf3Positions.size() * sizeof(XMFLOAT3),
			D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pMirrorMesh->m_pd3dPositionUploadBuffer.GetAddressOf());
		pMirrorMesh->m_d3dPositionBufferView.BufferLocation = pMirrorMesh->m_pd3dPositionBuffer->GetGPUVirtualAddress();
		pMirrorMesh->m_d3dPositionBufferView.StrideInBytes = sizeof(XMFLOAT3);
		pMirrorMesh->m_d3dPositionBufferView.SizeInBytes = sizeof(XMFLOAT3) * pMirrorMesh->m_nVertices;
	}


	// Normal, Tangent, BiTangent
	{
		pMirrorMesh->m_xmf3Normals.resize(6);
		pMirrorMesh->m_xmf3Tangents.resize(6);
		pMirrorMesh->m_xmf3BiTangents.resize(6);
		for (int i = 0; i < 6; ++i) {
			pMirrorMesh->m_xmf3Normals[i] = XMFLOAT3(0.f, 0.f, -1.f);
			pMirrorMesh->m_xmf3Tangents[i] = XMFLOAT3(1.f, 0.f, 0.f);
			pMirrorMesh->m_xmf3BiTangents[i] = XMFLOAT3(0.f, 1.f, 0.f);
		}


		pMirrorMesh->m_pd3dNormalBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pMirrorMesh->m_xmf3Normals.data(), pMirrorMesh->m_xmf3Normals.size() * sizeof(XMFLOAT3),
			D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pMirrorMesh->m_pd3dNormalUploadBuffer.GetAddressOf());
		pMirrorMesh->m_d3dNormalBufferView.BufferLocation = pMirrorMesh->m_pd3dNormalBuffer->GetGPUVirtualAddress();
		pMirrorMesh->m_d3dNormalBufferView.StrideInBytes = sizeof(XMFLOAT3);
		pMirrorMesh->m_d3dNormalBufferView.SizeInBytes = sizeof(XMFLOAT3) * pMirrorMesh->m_nVertices;

		pMirrorMesh->m_pd3dTangentBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pMirrorMesh->m_xmf3Tangents.data(), pMirrorMesh->m_xmf3Tangents.size() * sizeof(XMFLOAT3),
			D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pMirrorMesh->m_pd3dTangentUploadBuffer.GetAddressOf());
		pMirrorMesh->m_d3dTangentBufferView.BufferLocation = pMirrorMesh->m_pd3dTangentBuffer->GetGPUVirtualAddress();
		pMirrorMesh->m_d3dTangentBufferView.StrideInBytes = sizeof(XMFLOAT3);
		pMirrorMesh->m_d3dTangentBufferView.SizeInBytes = sizeof(XMFLOAT3) * pMirrorMesh->m_nVertices;

		pMirrorMesh->m_pd3dBiTangentBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pMirrorMesh->m_xmf3BiTangents.data(), pMirrorMesh->m_xmf3BiTangents.size() * sizeof(XMFLOAT3),
			D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pMirrorMesh->m_pd3dBiTangentUploadBuffer.GetAddressOf());
		pMirrorMesh->m_d3dBiTangentBufferView.BufferLocation = pMirrorMesh->m_pd3dBiTangentBuffer->GetGPUVirtualAddress();
		pMirrorMesh->m_d3dBiTangentBufferView.StrideInBytes = sizeof(XMFLOAT3);
		pMirrorMesh->m_d3dBiTangentBufferView.SizeInBytes = sizeof(XMFLOAT3) * pMirrorMesh->m_nVertices;

	}


	// UV
	{
		// UV의 경우 창문 갯수만큼 0 ~ nWindowsIn... 를 해버리면
		// 0 ~ 1 을 벗어나는 범위에서 Wrap 을 하도록 함
		// 그러면 창문하나 텍스쳐만 가지고 건물 전체에 바를 수 있음

		pMirrorMesh->m_xmf2TextureCoords0.resize(6);
		pMirrorMesh->m_xmf2TextureCoords0[0] = XMFLOAT2(0.f, 0.f);						// 0
		pMirrorMesh->m_xmf2TextureCoords0[1] = XMFLOAT2((float)nWindowsInWidth, 0.f);	// 1
		pMirrorMesh->m_xmf2TextureCoords0[2] = XMFLOAT2(0.f, (float)nWindowsInHeights);	// 2

		pMirrorMesh->m_xmf2TextureCoords0[3] = XMFLOAT2((float)nWindowsInWidth, 0.f);						// 1
		pMirrorMesh->m_xmf2TextureCoords0[4] = XMFLOAT2((float)nWindowsInWidth, (float)nWindowsInHeights);	// 3
		pMirrorMesh->m_xmf2TextureCoords0[5] = XMFLOAT2(0.f, (float)nWindowsInHeights);						// 2

		pMirrorMesh->m_pd3dTextureCoord0Buffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pMirrorMesh->m_xmf2TextureCoords0.data(), pMirrorMesh->m_xmf2TextureCoords0.size() * sizeof(XMFLOAT2),
			D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pMirrorMesh->m_pd3dTextureCoord0UploadBuffer.GetAddressOf());
		pMirrorMesh->m_d3dTextureCoord0BufferView.BufferLocation = pMirrorMesh->m_pd3dTextureCoord0Buffer->GetGPUVirtualAddress();
		pMirrorMesh->m_d3dTextureCoord0BufferView.StrideInBytes = sizeof(XMFLOAT2);
		pMirrorMesh->m_d3dTextureCoord0BufferView.SizeInBytes = sizeof(XMFLOAT2) * pMirrorMesh->m_nVertices;
	}


	return pMirrorMesh;
}

std::shared_ptr<StandardMesh> StandardMesh::GenerateBuildingTopMesh(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, float fWidth, float fLength)
{
	std::shared_ptr<StandardMesh> pTopMesh = std::make_shared<StandardMesh>();
	pTopMesh->m_nVertices = 6;

	float fHalfWidth = fWidth / 2;
	float fHalfLength = fLength / 2;

	/*
			 0                       1
	           +-------------------+             +y
			  /               	  /  			 |  +z
			 /				  	 /  fLength		 | /
	        /				    /				 |/
		   +-------------------+				 +-------- +x
		 2        fWidth         3

		- 일단 Index 안쓰고 정점 6개로 그리도록 함
		- 월드 원점에다 만듬. 방향(Normal)은 +Y 방향을 바라봄
	*/

	// Position
	{
		pTopMesh->m_xmf3Positions.resize(6);
		pTopMesh->m_xmf3Positions[0] = XMFLOAT3(-fHalfWidth, 0.f, +fHalfLength);	// 0
		pTopMesh->m_xmf3Positions[1] = XMFLOAT3(+fHalfWidth, 0.f, +fHalfLength);	// 1
		pTopMesh->m_xmf3Positions[2] = XMFLOAT3(-fHalfWidth, 0.f, -fHalfLength);	// 2

		pTopMesh->m_xmf3Positions[3] = XMFLOAT3(+fHalfWidth, 0.f, +fHalfLength);	// 1
		pTopMesh->m_xmf3Positions[4] = XMFLOAT3(+fHalfWidth, 0.f, -fHalfLength);	// 3
		pTopMesh->m_xmf3Positions[5] = XMFLOAT3(-fHalfWidth, 0.f, -fHalfLength);	// 2

		pTopMesh->m_pd3dPositionBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pTopMesh->m_xmf3Positions.data(), pTopMesh->m_xmf3Positions.size() * sizeof(XMFLOAT3),
			D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pTopMesh->m_pd3dPositionUploadBuffer.GetAddressOf());
		pTopMesh->m_d3dPositionBufferView.BufferLocation = pTopMesh->m_pd3dPositionBuffer->GetGPUVirtualAddress();
		pTopMesh->m_d3dPositionBufferView.StrideInBytes = sizeof(XMFLOAT3);
		pTopMesh->m_d3dPositionBufferView.SizeInBytes = sizeof(XMFLOAT3) * pTopMesh->m_nVertices;
	}

	// Normal, Tangent, BiTangent
	{
		pTopMesh->m_xmf3Normals.resize(6);
		pTopMesh->m_xmf3Tangents.resize(6);
		pTopMesh->m_xmf3BiTangents.resize(6);
		for (int i = 0; i < 6; ++i) {
			pTopMesh->m_xmf3Normals[i] = XMFLOAT3(0.f, 1.f, 0.f);
			pTopMesh->m_xmf3Tangents[i] = XMFLOAT3(1.f, 0.f, 0.f);
			pTopMesh->m_xmf3BiTangents[i] = XMFLOAT3(0.f, 0.f, -1.f);
		}

		pTopMesh->m_pd3dNormalBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pTopMesh->m_xmf3Normals.data(), pTopMesh->m_xmf3Normals.size() * sizeof(XMFLOAT3),
			D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pTopMesh->m_pd3dNormalUploadBuffer.GetAddressOf());
		pTopMesh->m_d3dNormalBufferView.BufferLocation = pTopMesh->m_pd3dNormalBuffer->GetGPUVirtualAddress();
		pTopMesh->m_d3dNormalBufferView.StrideInBytes = sizeof(XMFLOAT3);
		pTopMesh->m_d3dNormalBufferView.SizeInBytes = sizeof(XMFLOAT3) * pTopMesh->m_nVertices;


		pTopMesh->m_pd3dTangentBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pTopMesh->m_xmf3Tangents.data(), pTopMesh->m_xmf3Tangents.size() * sizeof(XMFLOAT3),
			D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pTopMesh->m_pd3dTangentUploadBuffer.GetAddressOf());
		pTopMesh->m_d3dTangentBufferView.BufferLocation = pTopMesh->m_pd3dTangentBuffer->GetGPUVirtualAddress();
		pTopMesh->m_d3dTangentBufferView.StrideInBytes = sizeof(XMFLOAT3);
		pTopMesh->m_d3dTangentBufferView.SizeInBytes = sizeof(XMFLOAT3) * pTopMesh->m_nVertices;


		pTopMesh->m_pd3dBiTangentBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pTopMesh->m_xmf3BiTangents.data(), pTopMesh->m_xmf3BiTangents.size() * sizeof(XMFLOAT3),
			D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pTopMesh->m_pd3dBiTangentUploadBuffer.GetAddressOf());
		pTopMesh->m_d3dBiTangentBufferView.BufferLocation = pTopMesh->m_pd3dBiTangentBuffer->GetGPUVirtualAddress();
		pTopMesh->m_d3dBiTangentBufferView.StrideInBytes = sizeof(XMFLOAT3);
		pTopMesh->m_d3dBiTangentBufferView.SizeInBytes = sizeof(XMFLOAT3) * pTopMesh->m_nVertices;
	}

	// UV
	{
		pTopMesh->m_xmf2TextureCoords0.resize(6);
		pTopMesh->m_xmf2TextureCoords0[0] = XMFLOAT2(0.f, 0.f);		// 0
		pTopMesh->m_xmf2TextureCoords0[1] = XMFLOAT2(1.f, 0.f);		// 1
		pTopMesh->m_xmf2TextureCoords0[2] = XMFLOAT2(0.f, 1.f);		// 2

		pTopMesh->m_xmf2TextureCoords0[3] = XMFLOAT2(1.f, 0.f);		// 1
		pTopMesh->m_xmf2TextureCoords0[4] = XMFLOAT2(1.f, 1.f);		// 3
		pTopMesh->m_xmf2TextureCoords0[5] = XMFLOAT2(0.f, 1.f);		// 2

		pTopMesh->m_pd3dTextureCoord0Buffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pTopMesh->m_xmf2TextureCoords0.data(), pTopMesh->m_xmf2TextureCoords0.size() * sizeof(XMFLOAT2),
			D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pTopMesh->m_pd3dTextureCoord0UploadBuffer.GetAddressOf());
		pTopMesh->m_d3dTextureCoord0BufferView.BufferLocation = pTopMesh->m_pd3dTextureCoord0Buffer->GetGPUVirtualAddress();
		pTopMesh->m_d3dTextureCoord0BufferView.StrideInBytes = sizeof(XMFLOAT2);
		pTopMesh->m_d3dTextureCoord0BufferView.SizeInBytes = sizeof(XMFLOAT2) * pTopMesh->m_nVertices;
	}

	return pTopMesh;
}

TerrainMesh::TerrainMesh()
{
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
}

TerrainMesh::~TerrainMesh()
{
	__debugbreak();
}

void TerrainMesh::Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, int xStart, int zStart, int nWidth, int nLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color, std::shared_ptr<HeightMapRawImage> pHeightMap)
{
	m_nVertices = nWidth * nLength;
	m_nOffset = 0;
	m_nSlot = 0;
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

	m_nWidth = nWidth;
	m_nLength = nLength;
	m_xmf3Scale = xmf3Scale;

	int cxHeightMap = pHeightMap ? pHeightMap->GetRawImageWidth() : 0;
	int czHeightMap = pHeightMap ? pHeightMap->GetRawImageLength() : 0;

#ifdef TERRAIN_TESSELATION
	int nControlPointPerEdge = 2;
	m_nVertices = nControlPointPerEdge * nControlPointPerEdge;
	m_d3dPrimitiveTopology = (D3D12_PRIMITIVE_TOPOLOGY)(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + m_nVertices - 1);	// 4x4 Quad 패치

#else
	m_nVertices = nWidth * nLength;
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

#endif

	m_xmf3Positions.resize(m_nVertices);
	m_xmf4Colors.resize(m_nVertices);
	m_xmf2TextureCoords0.resize(m_nVertices);
	m_xmf2TextureCoords1.resize(m_nVertices);

	float fHeight = 0.0f, fMinHeight = +FLT_MAX, fMaxHeight = -FLT_MAX;

#ifdef TERRAIN_TESSELATION
	int nIncrementX = (nWidth / (nControlPointPerEdge - 1)) - 1;
	int nIncrementZ = (nLength / (nControlPointPerEdge - 1)) - 1;

	for (int i = 0, z = (zStart + nLength - 1); z >= zStart; z -= nIncrementZ) {
		for (int x = xStart; x < (xStart + nWidth); x += nIncrementX, i++) {
			fHeight = GetHeight(x, z, pHeightMap);
			m_xmf3Positions[i] = XMFLOAT3((x * m_xmf3Scale.x), fHeight, (z*m_xmf3Scale.z));
			m_xmf4Colors[i] = Vector4::Add(GetColor(x, z, pHeightMap), xmf4Color);
			m_xmf2TextureCoords0[i] = XMFLOAT2(float(x) / float(cxHeightMap - 1), float(czHeightMap - 1 - z) / float(czHeightMap - 1));
			m_xmf2TextureCoords1[i] = XMFLOAT2(float(x) / float(m_xmf3Scale.x * 0.5f), float(z) / float(m_xmf3Scale.z * 0.5f));
			if (fHeight < fMinHeight) fMinHeight = fHeight;
			if (fHeight > fMaxHeight) fMaxHeight = fHeight;
		}
	}

#else
	for (int i = 0, z = zStart; z < (zStart + nLength); z++)
	{
		for (int x = xStart; x < (xStart + nWidth); x++, i++)
		{
			fHeight = GetHeight(x, z, pHeightMap);
			m_xmf3Positions[i] = XMFLOAT3((x * m_xmf3Scale.x), fHeight, (z * m_xmf3Scale.z));
			m_xmf4Colors[i] = Vector4::Add(GetColor(x, z, pHeightMap), xmf4Color);
			m_xmf2TextureCoords0[i] = XMFLOAT2(float(x) / float(cxHeightMap - 1), float(czHeightMap - 1 - z) / float(czHeightMap - 1));
			m_xmf2TextureCoords1[i] = XMFLOAT2(float(x) / float(m_xmf3Scale.x * 0.5f), float(z) / float(m_xmf3Scale.z * 0.5f));
			if (fHeight < fMinHeight) fMinHeight = fHeight;
			if (fHeight > fMaxHeight) fMaxHeight = fHeight;
		}
	}
#endif

	m_pd3dPositionBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_xmf3Positions.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dPositionUploadBuffer.GetAddressOf());
	m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
	m_d3dPositionBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dPositionBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;

	m_pd3dColorBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_xmf4Colors.data(), sizeof(XMFLOAT4) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dColorUploadBuffer.GetAddressOf());
	m_d3dColorBufferView.BufferLocation = m_pd3dColorBuffer->GetGPUVirtualAddress();
	m_d3dColorBufferView.StrideInBytes = sizeof(XMFLOAT4);
	m_d3dColorBufferView.SizeInBytes = sizeof(XMFLOAT4) * m_nVertices;

	m_pd3dTextureCoord0Buffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_xmf2TextureCoords0.data(), sizeof(XMFLOAT2) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dTextureCoord0UploadBuffer.GetAddressOf());
	m_d3dTextureCoord0BufferView.BufferLocation = m_pd3dTextureCoord0Buffer->GetGPUVirtualAddress();
	m_d3dTextureCoord0BufferView.StrideInBytes = sizeof(XMFLOAT2);
	m_d3dTextureCoord0BufferView.SizeInBytes = sizeof(XMFLOAT2) * m_nVertices;

	m_pd3dTextureCoord1Buffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_xmf2TextureCoords1.data(), sizeof(XMFLOAT2) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dTextureCoord1UploadBuffer.GetAddressOf());
	m_d3dTextureCoord1BufferView.BufferLocation = m_pd3dTextureCoord1Buffer->GetGPUVirtualAddress();
	m_d3dTextureCoord1BufferView.StrideInBytes = sizeof(XMFLOAT2);
	m_d3dTextureCoord1BufferView.SizeInBytes = sizeof(XMFLOAT2) * m_nVertices;

#ifndef TERRAIN_TESSELATION
	m_nSubMeshes = 1;
	m_nSubSetIndices.resize(m_nSubMeshes);
	m_IndicesBySubset.resize(m_nSubMeshes);

	m_pd3dSubSetIndexBuffers.resize(m_nSubMeshes);
	m_pd3dSubSetIndexUploadBuffers.resize(m_nSubMeshes);
	m_d3dSubSetIndexBufferViews.resize(m_nSubMeshes);

	m_nSubSetIndices[0] = ((nWidth * 2) * (nLength - 1)) + ((nLength - 1) - 1);
	m_IndicesBySubset[0].resize(m_nSubSetIndices[0]);

	for (int j = 0, z = 0; z < nLength - 1; z++)
	{
		if ((z % 2) == 0)
		{
			for (int x = 0; x < nWidth; x++)
			{
				if ((x == 0) && (z > 0)) m_IndicesBySubset[0][j++] = (UINT)(x + (z * nWidth));
				m_IndicesBySubset[0][j++] = (UINT)(x + (z * nWidth));
				m_IndicesBySubset[0][j++] = (UINT)((x + (z * nWidth)) + nWidth);
			}
		}
		else
		{
			for (int x = nWidth - 1; x >= 0; x--)
			{
				if (x == (nWidth - 1)) m_IndicesBySubset[0][j++] = (UINT)(x + (z * nWidth));
				m_IndicesBySubset[0][j++] = (UINT)(x + (z * nWidth));
				m_IndicesBySubset[0][j++] = (UINT)((x + (z * nWidth)) + nWidth);
			}
		}
	}

	m_pd3dSubSetIndexBuffers[0] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_IndicesBySubset[0].data(), sizeof(UINT) * m_nSubSetIndices[0], D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, m_pd3dSubSetIndexUploadBuffers[0].GetAddressOf());

	m_d3dSubSetIndexBufferViews[0].BufferLocation = m_pd3dSubSetIndexBuffers[0]->GetGPUVirtualAddress();
	m_d3dSubSetIndexBufferViews[0].Format = DXGI_FORMAT_R32_UINT;
	m_d3dSubSetIndexBufferViews[0].SizeInBytes = sizeof(UINT) * m_nSubSetIndices[0];

#endif
}

void TerrainMesh::ReleaseUploadBuffers()
{
	Mesh::ReleaseUploadBuffers();

	if (m_pd3dTextureCoord0UploadBuffer) {
		m_pd3dTextureCoord0UploadBuffer.Reset();
		m_pd3dTextureCoord0UploadBuffer = nullptr;
	}

	if (m_pd3dTextureCoord1UploadBuffer) {
		m_pd3dTextureCoord1UploadBuffer.Reset();
		m_pd3dTextureCoord1UploadBuffer = nullptr;
	}

	if (m_pd3dColorBuffer) {
		m_pd3dColorBuffer.Reset();
		m_pd3dColorBuffer = nullptr;
	}

}

void TerrainMesh::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, int nSubSet, int nInstanceCount)
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	D3D12_VERTEX_BUFFER_VIEW d3dVertexBufferViews[] = {
		m_d3dPositionBufferView,
		m_d3dColorBufferView,
		m_d3dTextureCoord0BufferView,
		m_d3dTextureCoord1BufferView,
	};

#ifdef TERRAIN_TESSELATION
	pd3dCommandList->IASetVertexBuffers(m_nSlot, _countof(d3dVertexBufferViews), d3dVertexBufferViews);
	pd3dCommandList->DrawInstanced(m_nVertices, 1, m_nOffset, 0);

#else
	pd3dCommandList->IASetVertexBuffers(m_nSlot, _countof(d3dVertexBufferViews), d3dVertexBufferViews);
	if ((m_nSubMeshes > 0) && (nSubSet < m_nSubMeshes))
	{
		pd3dCommandList->IASetIndexBuffer(&(m_d3dSubSetIndexBufferViews[nSubSet]));
		pd3dCommandList->DrawIndexedInstanced(m_nSubSetIndices[nSubSet], nInstanceCount, 0, 0, 0);
	}
	else
	{
		pd3dCommandList->DrawInstanced(m_nVertices, nInstanceCount, m_nOffset, 0);
	}
#endif
}

float TerrainMesh::GetHeight(int x, int z, std::shared_ptr<HeightMapRawImage> pHeightMap)
{
	if (!pHeightMap) return 0.f;

	std::vector<BYTE> pHeightMapPixels = pHeightMap->GetRawImagePixels();
	XMFLOAT3 xmf3Scale = pHeightMap->GetScale();
	int nWidth = pHeightMap->GetRawImageWidth();
	float fHeight = pHeightMapPixels[x + (z * nWidth)] * xmf3Scale.y;
	return fHeight;
}

XMFLOAT4 TerrainMesh::GetColor(int x, int z, std::shared_ptr<HeightMapRawImage> pHeightMap)
{
	if (!pHeightMap) return XMFLOAT4(0.f, 0.f, 0.f, 1.f);

	XMFLOAT3 xmf3LightDirection = XMFLOAT3(-1.0f, 1.0f, 1.0f);
	xmf3LightDirection = Vector3::Normalize(xmf3LightDirection);
	XMFLOAT3 xmf3Scale = pHeightMap->GetScale();
	XMFLOAT4 xmf4IncidentLightColor(0.9f, 0.8f, 0.4f, 1.0f);
	float fScale = Vector3::DotProduct(pHeightMap->GetHeightMapNormal(x, z), xmf3LightDirection);
	fScale += Vector3::DotProduct(pHeightMap->GetHeightMapNormal(x + 1, z), xmf3LightDirection);
	fScale += Vector3::DotProduct(pHeightMap->GetHeightMapNormal(x + 1, z + 1), xmf3LightDirection);
	fScale += Vector3::DotProduct(pHeightMap->GetHeightMapNormal(x, z + 1), xmf3LightDirection);
	fScale = (fScale / 4.0f) + 0.05f;
	if (fScale > 1.0f) fScale = 1.0f;
	if (fScale < 0.25f) fScale = 0.25f;
	XMFLOAT4 xmf4Color;
	XMStoreFloat4(&xmf4Color, fScale * XMLoadFloat4(&xmf4IncidentLightColor));
	return xmf4Color;
}
