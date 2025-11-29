#include "stdafx.h"
#include "ConstantBuffer.h"

ConstantBuffer::ConstantBuffer(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT uiElementSize, bool bCreateView)
{
	Create(pd3dDevice, pd3dCommandList, uiElementSize, bCreateView);
}

ConstantBuffer::~ConstantBuffer()
{
	//if (m_pd3dCBuffer) {
	//	m_pd3dCBuffer->Unmap(0, NULL);
	//}
}
void ConstantBuffer::Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT uiElementSize, bool bCreateView)
{
	m_pd3dCBuffer = ::CreateBufferResource(
		pd3dDevice,
		pd3dCommandList,
		nullptr,
		uiElementSize,
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		nullptr
	);

	m_pd3dCBuffer->Map(0, NULL, &m_pMappedPtr);

	if (bCreateView) {
		HRESULT hr;

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
		{
			heapDesc.NumDescriptors = 1;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			heapDesc.NodeMask = 0;
		}

		hr = pd3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_pd3dCBVHeap.GetAddressOf()));
		if (FAILED(hr)) {
			__debugbreak();
		}

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		{
			cbvDesc.BufferLocation = m_pd3dCBuffer->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = uiElementSize;
		}
		pd3dDevice->CreateConstantBufferView(&cbvDesc, m_pd3dCBVHeap->GetCPUDescriptorHandleForHeapStart());
	}
}

void ConstantBuffer::SetBufferToPipeline(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT uiRootParameterIndex) const
{
	pd3dCommandList->SetGraphicsRootConstantBufferView(uiRootParameterIndex, m_pd3dCBuffer->GetGPUVirtualAddress());
}

D3D12_CPU_DESCRIPTOR_HANDLE ConstantBuffer::GetCPUDescriptorHandle() const
{
	assert(m_pd3dCBVHeap);
	return m_pd3dCBVHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE ConstantBuffer::GetGPUDescriptorHandle() const
{
	assert(m_pd3dCBVHeap);
	return m_pd3dCBVHeap->GetGPUDescriptorHandleForHeapStart();
}
