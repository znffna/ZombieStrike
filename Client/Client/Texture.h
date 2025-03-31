///////////////////////////////////////////////////////////////////////////////
// Date: 2025-03-28
// Texture.h : CTexture 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once
#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////////////
//

enum RESOURCE_TYPE
{
	RESOURCE_TEXTURE1D = 0x01,
	RESOURCE_TEXTURE2D = 0x02,
	RESOURCE_TEXTURE2D_ARRAY = 0x03, //[]
	RESOURCE_TEXTURE2DARRAY = 0x04,
	RESOURCE_TEXTURE_CUBE = 0x05,
	RESOURCE_BUFFER = 0x06,
	RESOURCE_STRUCTURED_BUFFER = 0x07
};

class CTexture
{
public:
	CTexture() {};
	CTexture(int nTextures, UINT nTextureType, int nRootParameters);
	virtual ~CTexture()
	{
		m_pd3dTextures.clear();
		m_pd3dTextureUploadBuffers.clear();
		for (auto& name : m_strTextureNames)
		{
			std::wstring debugoutput = L"Texture Name: " + name + L" has destroyed\n";
			OutputDebugString(debugoutput.c_str());
		}
		m_strTextureNames.clear();
		m_nResourceTypes.clear();
		m_pdxgiBufferFormats.clear();
		m_nBufferElements.clear();
		m_nBufferStrides.clear();
		m_nRootParameterIndices.clear();
		m_d3dSrvGpuDescriptorHandles.clear();

	};

	// Texture Name
	std::string GetName() { return m_strName; }
	void SetName(std::string strName) { m_strName = strName; }

	// Texture Type
	UINT GetTextureType() { return m_nTextureType; }
	void SetTextureType(UINT nTextureType) { m_nTextureType = nTextureType; }

	// Texture
	ComPtr<ID3D12Resource> GetTexture(int nIndex = 0) { return m_pd3dTextures[nIndex]; }
	void SetTexture(ComPtr<ID3D12Resource> pd3dTexture, int nIndex = 0) { m_pd3dTextures[nIndex] = pd3dTexture; }

	// Getter / Setter
	void SetRootParameterIndex(int nIndex, UINT nRootParameterIndex) { m_nRootParameterIndices[nIndex] = nRootParameterIndex; }
	void SetGpuDescriptorHandle(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSrvGpuDescriptorHandle) { m_d3dSrvGpuDescriptorHandles[nIndex] = d3dSrvGpuDescriptorHandle; }

	int GetRootParameters() { return(m_nRootParameters); }
	int GetTextures() { return((int)m_pd3dTextures.size()); }
	std::wstring GetTextureName(int nIndex) { return(m_strTextureNames[nIndex]); }
	ID3D12Resource* GetResource(int nIndex) { return(m_pd3dTextures[nIndex].Get()); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(int nIndex) { return(m_d3dSrvGpuDescriptorHandles[nIndex]); }
	int GetRootParameter(int nIndex) { return(m_nRootParameterIndices[nIndex]); }

	UINT GetTextureType(int nIndex) { return(m_nResourceTypes[nIndex]); }
	DXGI_FORMAT GetBufferFormat(int nIndex) { return(m_pdxgiBufferFormats[nIndex]); }
	int GetBufferElements(int nIndex) { return(m_nBufferElements[nIndex]); }

	D3D12_SHADER_RESOURCE_VIEW_DESC GetShaderResourceViewDesc(int nIndex);;

	// Shader Variables
	void UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, int nParameterIndex, int nTextureIndex);;
	void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);;

	// Load Texture
	void LoadTextureFromDDSFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::wstring strTextureName, UINT nResourceType, UINT nIndex);;
	void LoadTextureFromWICFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::wstring strTextureName, UINT nResourceType, UINT nIndex);;
	void LoadBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nElements, UINT nStride, DXGI_FORMAT ndxgiFormat, UINT nIndex);;
	ComPtr<ID3D12Resource> CreateTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, UINT nIndex, UINT nResourceType, UINT nWidth, UINT nHeight, UINT nElements, UINT nMipLevels, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS d3dResourceFlags, D3D12_RESOURCE_STATES d3dResourceStates, D3D12_CLEAR_VALUE* pd3dClearValue);;

private:
	std::string m_strName; // Texture Name

	UINT m_nTextureType = 0x00; // Texture Type

	// Texture Variables
	UINT m_nTextures;
	std::vector<ComPtr<ID3D12Resource>> m_pd3dTextures;
	std::vector<ComPtr<ID3D12Resource>> m_pd3dTextureUploadBuffers;
	std::vector<std::wstring> m_strTextureNames;

	std::vector<UINT> m_nResourceTypes;

	std::vector<DXGI_FORMAT> m_pdxgiBufferFormats;
	std::vector<int> m_nBufferElements;
	std::vector<int> m_nBufferStrides;

	int m_nRootParameters = 0;
	std::vector<int> m_nRootParameterIndices;
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_d3dSrvGpuDescriptorHandles;
};