#include "Texture.h"

#include "ResourceManager.h"
///////////////////////////////////////////////////////////////////////////////
//

CTexture::CTexture(TextureRecipe texturerecipe)
	: m_textureRecipe(texturerecipe)
{
	m_nTextures = 1;
	m_pd3dTextures.resize(m_nTextures);
	m_pd3dTextureUploadBuffers.resize(m_nTextures);
	m_strTextureNames.resize(m_nTextures);

	m_nTextureType = texturerecipe.type;

	m_pdxgiBufferFormats.resize(m_nTextures);
	m_nBufferElements.resize(m_nTextures);
	m_nBufferStrides.resize(m_nTextures);

	m_d3dSrvGpuDescriptorHandles.resize(m_nTextures);

	m_nRootParameters = 1;
	m_nRootParameterIndices.resize(m_nRootParameters);
}

CTexture::CTexture(int nTextures, UINT nTextureType, int nRootParameters)
{
	m_nTextures = nTextures;
	m_pd3dTextures.resize(m_nTextures);
	m_pd3dTextureUploadBuffers.resize(m_nTextures);
	m_strTextureNames.resize(m_nTextures);

	m_nTextureType = nTextureType;

	m_pdxgiBufferFormats.resize(m_nTextures);
	m_nBufferElements.resize(m_nTextures);
	m_nBufferStrides.resize(m_nTextures);

	m_d3dSrvGpuDescriptorHandles.resize(m_nTextures);

	m_nRootParameters = nRootParameters;
	m_nRootParameterIndices.resize(m_nRootParameters);
}


D3D12_SHADER_RESOURCE_VIEW_DESC CTexture::GetShaderResourceViewDesc(int nIndex)
{
	ID3D12Resource* pShaderResource = GetResource(nIndex);
	D3D12_RESOURCE_DESC d3dResourceDesc = pShaderResource->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc;
	d3dShaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	int nTextureType = GetTextureType();
	switch (nTextureType)
	{
	case RESOURCE_TEXTURE2D: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize == 1)
	case RESOURCE_TEXTURE2D_ARRAY: //[]
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		d3dShaderResourceViewDesc.Texture2D.MipLevels = -1;
		d3dShaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.Texture2D.PlaneSlice = 0;
		d3dShaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;
	case RESOURCE_TEXTURE2DARRAY: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize != 1)
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		d3dShaderResourceViewDesc.Texture2DArray.MipLevels = -1;
		d3dShaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.Texture2DArray.PlaneSlice = 0;
		d3dShaderResourceViewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		d3dShaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;
		d3dShaderResourceViewDesc.Texture2DArray.ArraySize = d3dResourceDesc.DepthOrArraySize;
		break;
	case RESOURCE_TEXTURE_CUBE: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize == 6)
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		d3dShaderResourceViewDesc.TextureCube.MipLevels = 1;
		d3dShaderResourceViewDesc.TextureCube.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		break;
	case RESOURCE_BUFFER: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		d3dShaderResourceViewDesc.Format = m_pdxgiBufferFormats[nIndex];
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		d3dShaderResourceViewDesc.Buffer.FirstElement = 0;
		d3dShaderResourceViewDesc.Buffer.NumElements = m_nBufferElements[nIndex];
		d3dShaderResourceViewDesc.Buffer.StructureByteStride = 0;
		d3dShaderResourceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		break;
	}
	return(d3dShaderResourceViewDesc);
}

// Shader Variables

void CTexture::CreateByTextureRecipe(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	switch (m_textureRecipe.source)
	{
	case TEXTURE_SOURCE_FILE:
	{
		auto pResource = CResourceManager::Instance().GetTexture(m_textureRecipe.filePath);
		if (pResource.texture) {
			// 이미 로드된 경우 리소스 설정
			m_nTextures = 1;
			m_pd3dTextures.resize(m_nTextures);
			m_pd3dTextureUploadBuffers.resize(m_nTextures);
			m_strTextureNames.resize(m_nTextures);
			m_d3dSrvGpuDescriptorHandles.resize(m_nTextures);

			m_pd3dTextures[0] = pResource.texture;
			m_d3dSrvGpuDescriptorHandles[0] = pResource.gpuHandle;

			m_nRootParameterIndices[0] = m_textureRecipe.rootparameterindex;
			m_strTextureNames[0] = m_textureRecipe.filePath;
			m_nTextureType = m_textureRecipe.type;
		}
		else {
			// 로드되지 않은 경우 확장자에 따라 로드
			std::wstring ext = m_textureRecipe.filePath.substr(m_textureRecipe.filePath.find_last_of(L".") + 1);
			std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

			m_nTextures = 1;
			m_pd3dTextures.resize(m_nTextures);
			m_pd3dTextureUploadBuffers.resize(m_nTextures);
			m_strTextureNames.resize(m_nTextures);
			m_d3dSrvGpuDescriptorHandles.resize(m_nTextures);

			if (ext == L"dds") {
				LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, m_textureRecipe.filePath, m_textureRecipe.type, 0);
			}
			else {
				// WIC 지원 포맷 (png, jpg, jpeg, bmp 등)
				LoadTextureFromWICFile(pd3dDevice, pd3dCommandList, m_textureRecipe.filePath, m_textureRecipe.type, 0);
			}
			CResourceManager::Instance().CreateShaderResourceView(pd3dDevice, this, 0, m_textureRecipe.rootparameterindex);
			CResourceManager::Instance().SetTexture(m_textureRecipe.filePath, CResourceManager::TextureInfo{ m_pd3dTextures[0], m_d3dSrvGpuDescriptorHandles[0], ROOT_PARAMETER_STANDARD_TEXTURES });
		}
	}
		break;
	case TEXTURE_SOURCE_RENDERTARGET:
	{
		CreateTexture(pd3dDevice, pd3dCommandList, 0, RESOURCE_TEXTURE2D, m_textureRecipe.width, m_textureRecipe.height, m_textureRecipe.nElements, m_textureRecipe.mipLevels, m_textureRecipe.format, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, m_textureRecipe.initialState, &m_textureRecipe.pd3dClearValue);
	}
		break;
	case TEXTURE_SOURCE_BUFFER:
	{
		CreateBuffer(pd3dDevice, pd3dCommandList, m_textureRecipe.pData, m_textureRecipe.nElements, m_textureRecipe.nStride, m_textureRecipe.format, m_textureRecipe.heapType, m_textureRecipe.initialState, 0);
	}
		break;
	default:
		throw std::runtime_error("m_textureRecipe.source has wrong value");
		break;
	}
}

// Shader Variables
void CTexture::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	// Already Initialized Check
	if (m_bInitialized) return;

	// Create Buffer By Recipe
	try {
		CreateByTextureRecipe(pd3dDevice, pd3dCommandList);
			}
	catch (const std::exception& e) {
		std::string errorMsg = "Failed to create texture shader variables: ";
		errorMsg += e.what();
		OutputDebugStringA(errorMsg.c_str());
		m_bInitialized = false;
		return;
	}
	// Create Shader Resource View Descriptor Heap [이것도 같은 뷰를 사용하도록 해야할 것 같은데..]

	// Set Initialized Flag
	m_bInitialized = true;
}


void CTexture::UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, int nParameterIndex, int nTextureIndex)
{
	pd3dCommandList->SetGraphicsRootDescriptorTable(m_nRootParameterIndices[nParameterIndex], m_d3dSrvGpuDescriptorHandles[nTextureIndex]);
}

void CTexture::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_nRootParameters == m_pd3dTextures.size())
	{
		for (int i = 0; i < m_nRootParameters; i++)
		{
			if (m_d3dSrvGpuDescriptorHandles[i].ptr && (m_nRootParameterIndices[i] != -1))
				pd3dCommandList->SetGraphicsRootDescriptorTable(m_nRootParameterIndices[i], m_d3dSrvGpuDescriptorHandles[i]);
		}
	}
	else
	{
		if (m_d3dSrvGpuDescriptorHandles[0].ptr) pd3dCommandList->SetGraphicsRootDescriptorTable(m_nRootParameterIndices[0], m_d3dSrvGpuDescriptorHandles[0]);
	}
}

void CTexture::ReleaseUploadBuffers()
{
	if (false == m_pd3dTextureUploadBuffers.empty())
	{
		for (UINT i = 0; i < m_nTextures; i++) if (m_pd3dTextureUploadBuffers[i]) m_pd3dTextureUploadBuffers[i].Reset();
	}
}

// Load Texture

void CTexture::LoadTextureFromDDSFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::wstring strTextureName, UINT nResourceType, UINT nIndex)
{
	if (nIndex >= m_pd3dTextures.size()) {
		OutputDebugString(L"Texture index out of range\n");
		return;
	};

	m_nTextureType = nResourceType;
	m_strTextureNames[nIndex] = strTextureName;
	m_pd3dTextures[nIndex] = ::CreateTextureResourceFromDDSFile(pd3dDevice, pd3dCommandList, strTextureName.c_str(), m_pd3dTextureUploadBuffers[nIndex].GetAddressOf(), D3D12_RESOURCE_STATE_GENERIC_READ/*D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE*/);
	m_pd3dTextures[nIndex]->SetName(strTextureName.c_str());
}

void CTexture::LoadTextureFromWICFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::wstring strTextureName, UINT nResourceType, UINT nIndex)
{
	if (nIndex >= m_pd3dTextures.size()) {
		OutputDebugString(L"Texture index out of range\n");
		return;
	};

	m_nTextureType = nResourceType;
	m_strTextureNames[nIndex] = strTextureName;
	m_pd3dTextures[nIndex] = ::CreateTextureResourceFromWICFile(pd3dDevice, pd3dCommandList, strTextureName.c_str(), m_pd3dTextureUploadBuffers[nIndex].GetAddressOf(), D3D12_RESOURCE_STATE_GENERIC_READ/*D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE*/);
	m_pd3dTextures[nIndex]->SetName(strTextureName.c_str());
}

void CTexture::LoadBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nElements, UINT nStride, DXGI_FORMAT ndxgiFormat, UINT nIndex)
{
	if (nIndex >= m_pd3dTextures.size()) {
		OutputDebugString(L"Texture index out of range\n");
		return;
	};

	m_nTextureType = RESOURCE_BUFFER;
	m_nBufferElements[nIndex] = nElements;
	m_nBufferStrides[nIndex] = nStride;
	m_pdxgiBufferFormats[nIndex] = ndxgiFormat;
	m_pd3dTextures[nIndex] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pData, nElements * nStride, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dTextureUploadBuffers[nIndex]);
}

ComPtr<ID3D12Resource> CTexture::CreateTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, UINT nIndex, UINT nResourceType, UINT nWidth, UINT nHeight, UINT nElements, UINT nMipLevels, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS d3dResourceFlags, D3D12_RESOURCE_STATES d3dResourceStates, D3D12_CLEAR_VALUE* pd3dClearValue)
{
	if (nIndex >= m_pd3dTextures.size()) {
		OutputDebugString(L"Texture index out of range\n");
		return nullptr;
	};

	m_nTextureType = nResourceType;
	m_pd3dTextures[nIndex] = ::CreateTexture2DResource(pd3dDevice, pd3dCommandList, nWidth, nHeight, nElements, nMipLevels, dxgiFormat, d3dResourceFlags, d3dResourceStates, pd3dClearValue);
	return(m_pd3dTextures[nIndex]);
}

void CTexture::CreateBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nElements, UINT nStride, DXGI_FORMAT ndxgiFormat, D3D12_HEAP_TYPE d3dHeapType, D3D12_RESOURCE_STATES d3dResourceStates, UINT nIndex)
{
	m_nTextureType = RESOURCE_BUFFER;
	m_pdxgiBufferFormats[nIndex] = ndxgiFormat;
	m_nBufferElements[nIndex] = nElements;
	m_nBufferStrides[nIndex] = nStride;
	m_pd3dTextures[nIndex] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pData, nElements * nStride, d3dHeapType, d3dResourceStates, &m_pd3dTextureUploadBuffers[nIndex]);
}
