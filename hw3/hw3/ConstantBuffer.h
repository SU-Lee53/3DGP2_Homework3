#pragma once

class ConstantBuffer {
public:
	ConstantBuffer() = default;
	ConstantBuffer(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT uiElementSize, bool bCreateView = false);
	~ConstantBuffer();

public:
	void Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT uiElementSize, bool bCreateView = false);

	template<typename T>
	void UpdateData(T* pData) const;

	template<typename T>
	void UpdateData(std::vector<T> data, UINT offset = 0);

	template<typename T>
	void UpdateData(const T& data, UINT offset, UINT nDatas);

	template<typename T>
	void UpdateData(const T& data, UINT index);

	void SetBufferToPipeline(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT uiRootParameterIndex) const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle() const;

private:
	ComPtr<ID3D12DescriptorHeap>	m_pd3dCBVHeap = nullptr;
	ComPtr<ID3D12Resource>			m_pd3dCBuffer = nullptr;
	void*							m_pMappedPtr = nullptr;

};

template<typename T>
inline void ConstantBuffer::UpdateData(T* pData) const
{
	::memcpy(m_pMappedPtr, pData, sizeof(T));
}

template<typename T>
inline void ConstantBuffer::UpdateData(std::vector<T> data, UINT offset)
{
	T* pMappedPtr = reinterpret_cast<T*>(m_pMappedPtr);
	::memcpy(pMappedPtr + offset, data.data(), data.size() * sizeof(T));

}

template<typename T>
inline void ConstantBuffer::UpdateData(const T& data, UINT offset, UINT nDatas)
{
	T* pMappedPtr = reinterpret_cast<T*>(m_pMappedPtr);
	::memcpy(pMappedPtr + offset, &data, nDatas * sizeof(T));
}

template<typename T>
inline void ConstantBuffer::UpdateData(const T& data, UINT index)
{
	T* pMappedPtr = reinterpret_cast<T*>(m_pMappedPtr);
	::memcpy(pMappedPtr + index, &data, sizeof(T));
}
