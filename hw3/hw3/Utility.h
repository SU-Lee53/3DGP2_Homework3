#pragma once

struct DescriptorHandle {
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
};



/////////////////////
// Load From Files //
/////////////////////

inline std::string ReadStringFromFile(std::ifstream& inFile)
{
	BYTE nStrLength = 0;
	UINT nReads = 0;
	inFile.read((char*)&nStrLength, sizeof(BYTE));

	std::unique_ptr<char[]> pcstrRead;
	pcstrRead = std::make_unique<char[]>(nStrLength);
	inFile.read(pcstrRead.get(), nStrLength);

	return std::string(pcstrRead.get(), nStrLength);	// [pcstrRead, pcstrRead + nStrLength)
}
