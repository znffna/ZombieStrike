///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// GameObject.h : CGameObject 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"
#include "Mesh.h"

class CTexture;
class CShader;
class CCamera;

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
	CTexture(int nTextures, UINT nTextureType, int nRootParameters)
	{
		m_pd3dTextures.resize(nTextures);
		m_pd3dTextureUploadBuffers.resize(nTextures);
		m_strTextureNames.resize(nTextures);

		m_nResourceTypes.resize(nTextures);

		m_pdxgiBufferFormats.resize(nTextures);
		m_nBufferElements.resize(nTextures);
		m_nBufferStrides.resize(nTextures);

		m_d3dSrvGpuDescriptorHandles.resize(nTextures);

		m_nRootParameters = nRootParameters;
		m_nRootParameterIndices.resize(nRootParameters);


	};
	virtual ~CTexture() {};

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
	void SetRootParameterIndex(int nIndex, UINT nRootParameterIndex)
	{
		m_nRootParameterIndices[nIndex] = nRootParameterIndex;
	};
	void SetGpuDescriptorHandle(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSrvGpuDescriptorHandle) 
	{
		m_d3dSrvGpuDescriptorHandles[nIndex] = d3dSrvGpuDescriptorHandle;
	};

	int GetRootParameters() { return(m_nRootParameters); }
	int GetTextures() { return((int)m_pd3dTextures.size()); }
	std::wstring GetTextureName(int nIndex) { return(m_strTextureNames[nIndex]); }
	ID3D12Resource* GetResource(int nIndex) { return(m_pd3dTextures[nIndex].Get()); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(int nIndex) { return(m_d3dSrvGpuDescriptorHandles[nIndex]); }
	int GetRootParameter(int nIndex) { return(m_nRootParameterIndices[nIndex]); }

	UINT GetTextureType(int nIndex) { return(m_nResourceTypes[nIndex]); }
	DXGI_FORMAT GetBufferFormat(int nIndex) { return(m_pdxgiBufferFormats[nIndex]); }
	int GetBufferElements(int nIndex) { return(m_nBufferElements[nIndex]); }

	D3D12_SHADER_RESOURCE_VIEW_DESC GetShaderResourceViewDesc(int nIndex) 
	{
		ID3D12Resource* pShaderResource = GetResource(nIndex);
		D3D12_RESOURCE_DESC d3dResourceDesc = pShaderResource->GetDesc();

		D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc;
		d3dShaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		int nTextureType = GetTextureType(nIndex);
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
	};


	// Shader Variables
	void UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, int nParameterIndex, int nTextureIndex)
	{
		pd3dCommandList->SetGraphicsRootDescriptorTable(m_nRootParameterIndices[nParameterIndex], m_d3dSrvGpuDescriptorHandles[nTextureIndex]);
	};

	void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
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
	};

	// Load Texture
	void LoadTextureFromDDSFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::wstring strTextureName, UINT nResourceType, UINT nIndex)
	{
		if (nIndex >= m_pd3dTextures.size()) {
			OutputDebugString(L"Texture index out of range\n");
			return;
		};

		m_nResourceTypes[nIndex] = nResourceType;
		m_strTextureNames[nIndex] = strTextureName;
		m_pd3dTextures[nIndex] = ::CreateTextureResourceFromDDSFile(pd3dDevice, pd3dCommandList, strTextureName.c_str(), m_pd3dTextureUploadBuffers[nIndex].GetAddressOf(), D3D12_RESOURCE_STATE_GENERIC_READ/*D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE*/);
	};

	void LoadTextureFromWICFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::wstring strTextureName, UINT nResourceType, UINT nIndex)
	{
		if (nIndex >= m_pd3dTextures.size()) {
			OutputDebugString(L"Texture index out of range\n");
			return;
		};

		m_nResourceTypes[nIndex] = nResourceType;
		m_strTextureNames[nIndex] = strTextureName;
		m_pd3dTextures[nIndex] = ::CreateTextureResourceFromWICFile(pd3dDevice, pd3dCommandList, strTextureName.c_str(), m_pd3dTextureUploadBuffers[nIndex].GetAddressOf(), D3D12_RESOURCE_STATE_GENERIC_READ/*D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE*/);
	};

	void LoadBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nElements, UINT nStride, DXGI_FORMAT ndxgiFormat, UINT nIndex)
	{
		if (nIndex >= m_pd3dTextures.size()) {
			OutputDebugString(L"Texture index out of range\n");
			return;
		};

		m_nResourceTypes[nIndex] = RESOURCE_BUFFER;
		m_nBufferElements[nIndex] = nElements;
		m_nBufferStrides[nIndex] = nStride;
		m_pdxgiBufferFormats[nIndex] = ndxgiFormat;
		m_pd3dTextures[nIndex] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pData, nElements * nStride, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dTextureUploadBuffers[nIndex]);
	};

	ComPtr<ID3D12Resource> CreateTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, UINT nIndex, UINT nResourceType, UINT nWidth, UINT nHeight, UINT nElements, UINT nMipLevels, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS d3dResourceFlags, D3D12_RESOURCE_STATES d3dResourceStates, D3D12_CLEAR_VALUE* pd3dClearValue)
	{
		if (nIndex >= m_pd3dTextures.size()) {
			OutputDebugString(L"Texture index out of range\n");
			return nullptr;
		};

		m_nResourceTypes[nIndex] = nResourceType;
		//m_pd3dTextures[nIndex] = ::CreateTexture2DResource(pd3dDevice, pd3dCommandList, nWidth, nHeight, nElements, nMipLevels, dxgiFormat, d3dResourceFlags, d3dResourceStates, pd3dClearValue);
		return(m_pd3dTextures[nIndex]);
	};

private:
	std::string m_strName; // Texture Name

	UINT m_nTextureType = 0x00; // Texture Type

	// Texture Variables
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

struct CB_GAMEOBJECT_INFO
{
	XMFLOAT4X4						m_xmf4x4World;
};

struct CB_MATERIAL_INFO
{
	XMFLOAT4						m_xmf4Ambient;
	XMFLOAT4						m_xmf4Diffuse;
	XMFLOAT4						m_xmf4Specular;
	XMFLOAT4						m_xmf4Emissive;
	UINT							m_nTexturesMask;
};

class CMaterial
{
public:
	CMaterial() {};
	virtual ~CMaterial() {};

	// CMaterial Name
	std::string GetName() { return m_strName; }
	void SetName(std::string strName) { m_strName = strName; }

	// CMaterial Color
	DirectX::XMFLOAT4 GetAmbient() { return m_xmf4Ambient; }
	void SetAmbient(DirectX::XMFLOAT4 xmf4Ambient) { m_xmf4Ambient = xmf4Ambient; }

	DirectX::XMFLOAT4 GetDiffuse() { return m_xmf4Diffuse; }
	void SetDiffuse(DirectX::XMFLOAT4 xmf4Diffuse) { m_xmf4Diffuse = xmf4Diffuse; }

	DirectX::XMFLOAT4 GetSpecular() { return m_xmf4Specular; }
	void SetSpecular(DirectX::XMFLOAT4 xmf4Specular) { m_xmf4Specular = xmf4Specular; }

	// Texture
	std::shared_ptr<CTexture> GetTexture() { return m_pTexture; }
	void SetTexture(std::shared_ptr<CTexture> pTexture) { m_pTexture = pTexture; }

	// Shader
	void SetShader(std::shared_ptr<CShader> pShader) { m_pShader = pShader; }

	// Shader Variables
	void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
	{
#ifdef _USE_OBJECT_MATERIAL_CBV
		// Create Constant Buffer
		UINT ncbElementBytes = ((sizeof(CB_MATERIAL_INFO) + 255) & ~255); //256의 배수
		m_pd3dcbMaterial = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_GENERIC_READ, NULL);

		// Map Constant Buffer
		m_pd3dcbMaterial->Map(0, nullptr, (void**)&m_pcbMappedMaterial);
		ZeroMemory(m_pcbMappedMaterial, sizeof(CB_MATERIAL_INFO));
#endif // _USE_OBJECT_MATERIAL_CBV
	}

	void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
	{
#ifdef _USE_OBJECT_MATERIAL_CBV
		m_pcbMappedMaterial->m_xmf4Ambient = m_xmf4Ambient;
		m_pcbMappedMaterial->m_xmf4Diffuse = m_xmf4Diffuse;
		m_pcbMappedMaterial->m_xmf4Specular = m_xmf4Specular;
		m_pcbMappedMaterial->m_xmf4Emissive = m_xmf4Emissive;
		
		m_pcbMappedMaterial->m_nTexturesMask = 0x00;

		if (m_pTexture)
		{
			//pcbMappedObjectInfo->m_nTexturesMask |= m_pTexture->GetTextureType();
			//m_pTexture->UpdateShaderVariables(pd3dCommandList);
		}
		else {
			m_pcbMappedMaterial->m_nTexturesMask = 0x00;
		}

		D3D12_GPU_VIRTUAL_ADDRESS GPUAddress = m_pd3dcbMaterial->GetGPUVirtualAddress();
		pd3dCommandList->SetGraphicsRootConstantBufferView(ROOT_PARAMETER_MATERIAL, GPUAddress);
#else
		pd3dCommandList->SetGraphicsRoot32BitConstants(ROOT_PARAMETER_MATERIAL, 4, &m_xmf4Ambient, 0);
		pd3dCommandList->SetGraphicsRoot32BitConstants(ROOT_PARAMETER_MATERIAL, 4, &m_xmf4Diffuse, 4);
		pd3dCommandList->SetGraphicsRoot32BitConstants(ROOT_PARAMETER_MATERIAL, 4, &m_xmf4Specular, 8);
		pd3dCommandList->SetGraphicsRoot32BitConstants(ROOT_PARAMETER_MATERIAL, 4, &m_xmf4Emissive, 12);

		pd3dCommandList->SetGraphicsRoot32BitConstants(ROOT_PARAMETER_MATERIAL, 1, &m_nTexturesMask, 16);

		if (m_pTexture){
			m_pTexture->UpdateShaderVariables(pd3dCommandList);
		}
#endif // _USE_OBJECT_MATERIAL_CBV
	}

	void ReleaseShaderVariables()
	{
#ifdef _USE_OBJECT_MATERIAL_CBV
		if (m_pd3dcbMaterial) m_pd3dcbMaterial->Unmap(0, nullptr);
		m_pd3dcbMaterial.Reset();
#endif // _USE_OBJECT_MATERIAL_CBV
	}


private:
	std::string m_strName; // CMaterial Name

	DirectX::XMFLOAT4 m_xmf4Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f); // Ambient Color
	DirectX::XMFLOAT4 m_xmf4Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // Diffuse Color
	DirectX::XMFLOAT4 m_xmf4Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f); // Specular Color
	DirectX::XMFLOAT4 m_xmf4Emissive = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f); // Emissive Color

	UINT m_nTexturesMask = 0x00; // Texture Mask

	
	// Shader Variables
	ComPtr<ID3D12Resource> m_pd3dcbMaterial;
	CB_MATERIAL_INFO* m_pcbMappedMaterial = nullptr;
public:
	std::shared_ptr<CTexture> m_pTexture; // Texture
	std::shared_ptr<CShader> m_pShader; // Shader
};

class CGameObject
{
public:
	CGameObject();
	CGameObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual ~CGameObject();

	// Active Flag
	bool IsActive() { return m_bActive; }
	void SetActive(bool bActive) { m_bActive = bActive; }

	// Object ID
	UINT GetObjectID() { return m_nObjectID; }
	void SetObjectID(UINT nObjectID) { m_nObjectID = nObjectID; }

	// Object Name
	std::string GetName() { return m_strName; }
	void SetName(std::string strName) { m_strName = strName; }

	// Local Variables
	DirectX::XMFLOAT3 GetPosition() { return m_xmf3Position; }
	void SetPosition(DirectX::XMFLOAT3 xmf3Position) { m_xmf3Position = xmf3Position; }
	void SetPosition(float fx, float fy, float fz) { SetPosition(XMFLOAT3(fx, fy, fz)); }

	DirectX::XMFLOAT3 GetRotation() { return m_xmf3Rotation; }
	float GetPitch() { return m_xmf3Rotation.x; } // X 축을	기준으로 회전
	float GetYaw() { return m_xmf3Rotation.y; } // Y 축을 기준으로 회전
	float GetRoll() { return m_xmf3Rotation.z; } // Z 축을 기준으로 회전

	void SetRotation(DirectX::XMFLOAT3 xmf3Rotation) { m_xmf3Rotation = xmf3Rotation; }

	DirectX::XMFLOAT3 GetScale() { return m_xmf3Scale; }
	void SetScale(DirectX::XMFLOAT3 xmf3Scale) { m_xmf3Scale = xmf3Scale; }

	// Object World Matrix
	DirectX::XMFLOAT4X4 GetWorldMatrix() { return m_xmf4x4World; }
	void SetWorldMatrix(DirectX::XMFLOAT4X4 xmf4x4World) { m_xmf4x4World = xmf4x4World; }

	// Obecjt Local Matrix
	void UpdateLocalMatrix();
	DirectX::XMFLOAT4X4 GetLocalMatrix() { UpdateLocalMatrix(); return m_xmf4x4Local; }

	// Update Object World Matrix
	void UpdateWorldMatrix(DirectX::XMFLOAT4X4* xmf4x4ParentMatrix = nullptr);

	// Move
	void Move(DirectX::XMFLOAT3 xmf3Shift) { m_xmf3Position = Vector3::Add(m_xmf3Position, xmf3Shift); }
	void Move(float x, float y, float z) { m_xmf3Position = Vector3::Add(m_xmf3Position, DirectX::XMFLOAT3(x, y, z)); }

	// Rotate
	void Rotate(DirectX::XMFLOAT3 xmf3Rotate) { m_xmf3Rotation = Vector3::Add(m_xmf3Rotation, xmf3Rotate); }

	// Object Initialization / Release
	virtual void Initialize() {};
	virtual void Release() {}

	// Object Update
	virtual void Update(float fTimeElapsed) {}

	// Object Render
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr);

	// Object Collision
	//virtual void OnCollision(CGameObject* pGameObject) {}

	// Mesh
	void SetMesh(std::shared_ptr<CMesh> pMesh) { m_pMesh = pMesh; }

	// Material
	void MaterialResize(int nMaterials) { m_pMaterials.resize(nMaterials); }
	void AddMaterial(std::shared_ptr<CMaterial> pMaterial) { m_pMaterials.push_back(pMaterial); }
	void SetMaterial(std::shared_ptr<CMaterial> pMaterial, int nIndex = 0) { m_pMaterials[nIndex] = pMaterial; }

	// Shader
	void SetShader(std::shared_ptr<CShader> pShader, int nIndex = 0);

	// Shader Variables
	void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseShaderVariables();

	// Child
	void SetChild(std::shared_ptr<CGameObject> pChild) { m_pChilds.push_back(pChild); }

protected:
	bool m_bActive; // Active Flag

	UINT m_nObjectID; // Object ID
	std::string m_strName;  // Object Name

	std::shared_ptr<CMesh> m_pMesh; // Object Mesh

	// CMaterial
	std::vector<std::shared_ptr<CMaterial>> m_pMaterials; // Object CMaterial

	// Transform
	DirectX::XMFLOAT3 m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f); // 위치
	DirectX::XMFLOAT3 m_xmf3Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f); // 회전[Euler Angle]
	DirectX::XMFLOAT3 m_xmf3Scale = XMFLOAT3(1.0f, 1.0f, 1.0f); // 크기

	DirectX::XMFLOAT4X4 m_xmf4x4Local = Matrix4x4::Identity(); // Object Local Matrix
	DirectX::XMFLOAT4X4 m_xmf4x4World = Matrix4x4::Identity(); // Object World Matrix

	// Child
	std::vector<std::shared_ptr<CGameObject>> m_pChilds; // Child Object

	// Shader Variables
	ComPtr<ID3D12Resource> m_pd3dcbGameObject;
	CB_GAMEOBJECT_INFO* m_pcbMappedObject = nullptr;
};

////////////////////////////////////////////////////////////////////////////////////////
//

class CRotatingObject : public CGameObject
{
public:
	CRotatingObject();
	CRotatingObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual ~CRotatingObject();

	// Object Update
	virtual void Update(float fTimeElapsed) override;

	// Set Rotation Speed
	void SetRotationSpeed(float fRotationSpeed) { m_fRotationSpeed = fRotationSpeed; }

	// Set Rotation Axis
	void SetRotationAxis(DirectX::XMFLOAT3 xmf3RotationAxis) { m_xmf3RotationAxis = xmf3RotationAxis; }

private:
	float m_fRotationSpeed = 0.0f; // 초당 회전 속도
	XMFLOAT3 m_xmf3RotationAxis = XMFLOAT3(0.0f, 1.0f, 0.0f); // 회전 축

};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CSkyBox : public CGameObject
{
public:
	CSkyBox(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
	virtual ~CSkyBox();

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CHeightMapTerrain : public CGameObject
{
public:
	CHeightMapTerrain(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature,
		LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength,
		XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color);
	virtual ~CHeightMapTerrain();

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera) override;

	//지형의 높이를 계산하는 함수이다(월드 좌표계). 높이 맵의 높이에 스케일의 y를 곱한 값이다. 
	float GetHeight(float x, float z) { return(m_pHeightMapImage->GetHeight(x / m_xmf3Scale.x, z / m_xmf3Scale.z) * m_xmf3Scale.y); }
	
	//지형의 법선 벡터를 계산하는 함수이다(월드 좌표계). 높이 맵의 법선 벡터를 사용한다. 
	XMFLOAT3 GetNormal(float x, float z) {
		return(m_pHeightMapImage->GetHeightMapNormal(int(x / m_xmf3Scale.x),
			int(z / m_xmf3Scale.z)));
	}

	int GetHeightMapWidth() { return(m_pHeightMapImage->GetHeightMapWidth()); }
	int GetHeightMapLength() { return(m_pHeightMapImage->GetHeightMapLength()); }

	XMFLOAT3 GetScale() { return(m_xmf3Scale); }

	//지형의 크기(가로/세로)를 반환한다. 높이 맵의 크기에 스케일을 곱한 값이다. 
	float GetWidth() { return(m_nWidth * m_xmf3Scale.x); }
	float GetLength() { return(m_nLength * m_xmf3Scale.z); }

private:
	//지형의 높이 맵으로 사용할 이미지이다. 
	std::shared_ptr<CHeightMapImage> m_pHeightMapImage;

	//높이 맵의 가로와 세로 크기이다. 
	int m_nWidth;
	int m_nLength;

	//지형을 실제로 몇 배 확대할 것인가를 나타내는 스케일 벡터이다. 
	XMFLOAT3 m_xmf3Scale;
};