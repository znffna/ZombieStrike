///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// GameObject.cpp : GameObject 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#include "GameObject.h"
#include "Shader.h"
#include "Camera.h"

#include "Scene.h"

CGameObject::CGameObject()
{
	// Object Info
	static UINT nGameObjectID = 0;
	m_bActive = true;
	m_nObjectID = nGameObjectID++;

	m_strName = "GameObject_" + std::to_string(m_nObjectID);
}

CGameObject::CGameObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
}

CGameObject::~CGameObject()
{
}

void CGameObject::UpdateLocalMatrix()
{
	XMMATRIX xmmtxScale = XMMatrixScaling(m_xmf3Scale.x, m_xmf3Scale.y, m_xmf3Scale.z);
	XMMATRIX xmmtxRotation = XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_xmf3Rotation.x), XMConvertToRadians(m_xmf3Rotation.y), XMConvertToRadians(m_xmf3Rotation.z));
	XMMATRIX xmmtxTranslation = XMMatrixTranslation(m_xmf3Position.x, m_xmf3Position.y, m_xmf3Position.z);
	
	XMStoreFloat4x4(&m_xmf4x4Local, xmmtxScale * xmmtxRotation * xmmtxTranslation);
}

void CGameObject::UpdateWorldMatrix(DirectX::XMFLOAT4X4* xmf4x4ParentMatrix)
{
	// Update Local Matrix
	UpdateLocalMatrix();

	// Update Object World Matrix
	if (xmf4x4ParentMatrix)
	{
		m_xmf4x4World = Matrix4x4::Multiply(m_xmf4x4Local, *xmf4x4ParentMatrix);
	}
	else
	{
		m_xmf4x4World = m_xmf4x4Local;
	}

	// Update Child Object World Matrix
	(&m_xmf4x4World);
}

void CGameObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (false == m_bActive) return;
	// Render Object

	// Set Shader Variables
	UpdateShaderVariables(pd3dCommandList); // GameObject Matrix Update

	for (int i = 0; i < m_pMaterials.size(); ++i)
	{
		std::shared_ptr<CMaterial> pMaterial = m_pMaterials[i];
		if (pMaterial)
		{
			if (pMaterial->m_pShader) pMaterial->m_pShader->OnPrepareRender(pd3dCommandList, 0); // Render(pd3dCommandList, pCamera);
			pMaterial->UpdateShaderVariables(pd3dCommandList);
		}
		// Render Mesh
		if (m_pMesh) m_pMesh->Render(pd3dCommandList, i);
	}

	// Render Child Object
	for (auto& pChild : m_pChilds)
	{
		pChild->Render(pd3dCommandList, pCamera);
	}
}

void CGameObject::SetShader(std::shared_ptr<CShader> pShader, int nIndex)
{
	if (nIndex < m_pMaterials.size())
	{
		m_pMaterials[nIndex]->SetShader(pShader);
	}
	else if(m_pMaterials.empty()){
		std::shared_ptr<CMaterial> pMaterial= std::make_shared<CMaterial>();
		pMaterial->SetShader(pShader);
		m_pMaterials.push_back(pMaterial);
	}
	else {
		// Error
		std::wstring DebugString = L"Error : GameObject::SetShader() - nIndex is out of range";
		OutputDebugString(DebugString.c_str());
		throw;
	}
}

void CGameObject::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
#ifdef _USE_OBJECT_MATERIAL_CBV
	// Create Constant Buffer
	UINT ncbElementBytes = ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255); //256의 배수
	m_pd3dcbGameObject = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_GENERIC_READ, NULL);

	// Map Constant Buffer
	m_pd3dcbGameObject->Map(0, nullptr, (void**)&m_pcbMappedObject);
	ZeroMemory(m_pcbMappedObject, sizeof(CB_GAMEOBJECT_INFO));
#endif // _USE_OBJECT_MATERIAL_CBV
}

void CGameObject::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
#ifdef _USE_OBJECT_MATERIAL_CBV
	// Update Constant Buffer
	//m_pcbMappedObject->m_xmf4x4World = m_xmf4x4World; // DirectX는 행렬을 전치해서 셰이더에 적용해야 한다.
	XMStoreFloat4x4(&m_pcbMappedObject->m_xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));

#ifdef _DEBUG
	// Debug Output
	std::wstring DebugString = L"GameObject [" + std::to_wstring(m_nObjectID) + L"] - position ("
		+ std::to_wstring(m_xmf4x4World._41) + L", "
		+ std::to_wstring(m_xmf4x4World._42) + L", "
		+ std::to_wstring(m_xmf4x4World._43) + L")\n";
	OutputDebugString(DebugString.c_str());
#endif // _DEBUG

	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = m_pd3dcbGameObject->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(ROOT_PARAMETER_OBJECT, d3dGpuVirtualAddress);
#else 
	// Update Shader Variables
	XMFLOAT4X4 xmf4x4World;
	XMStoreFloat4x4(&xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));

	pd3dCommandList->SetGraphicsRoot32BitConstants(ROOT_PARAMETER_OBJECT, 16, &xmf4x4World, 0);
#endif // _USE_OBJECT_MATERIAL_CBV
}

void CGameObject::ReleaseShaderVariables()
{
#ifdef _USE_OBJECT_MATERIAL_CBV
	if (m_pd3dcbGameObject) m_pd3dcbGameObject->Unmap(0, nullptr);
	m_pd3dcbGameObject.Reset();
#endif // _USE_OBJECT_MATERIAL_CBV
}

///////////////////////////////////////////////////////////////////////////////
//

CRotatingObject::CRotatingObject()
{
	m_fRotationSpeed = 30.0f;
	m_xmf3RotationAxis = XMFLOAT3(0.0f, 1.0f, 0.0f);
}

CRotatingObject::CRotatingObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
	: CGameObject(pd3dDevice, pd3dCommandList)
{
	m_fRotationSpeed = 30.0f;
	m_xmf3RotationAxis = XMFLOAT3(0.0f, 1.0f, 0.0f);
}

CRotatingObject::~CRotatingObject()
{
}

void CRotatingObject::Update(float fTimeElapsed)
{
	Rotate(Vector3::ScalarProduct(m_xmf3RotationAxis, m_fRotationSpeed * fTimeElapsed));
}

///////////////////////////////////////////////////////////////////////////////
//

CSkyBox::CSkyBox(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature)
	: CGameObject(pd3dDevice, pd3dCommandList)
{
	std::shared_ptr<CMesh> pSkyBoxMesh = std::make_shared<CSkyBoxMesh>(pd3dDevice, pd3dCommandList, 20.0f, 20.0f, 20.0f);
	SetMesh(pSkyBoxMesh);

	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	std::shared_ptr<CTexture> pSkyBoxTexture = std::make_shared<CTexture>(1, RESOURCE_TEXTURE_CUBE, 1);
	pSkyBoxTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"SkyBox/SkyBox_0.dds", RESOURCE_TEXTURE_CUBE, 0);
	// pSkyBoxTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"SkyBox/SkyBox_1.dds", RESOURCE_TEXTURE_CUBE, 0);

	std::shared_ptr<CShader> pSkyBoxShader = std::make_shared<CSkyBoxShader>();
	pSkyBoxShader->CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
	pSkyBoxShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	// pSkyBoxShader->CreateCbvSrvDescriptorHeaps(pd3dDevice, 0, 1);
	CScene::CreateShaderResourceViews(pd3dDevice, pSkyBoxTexture.get(), 0, ROOT_PARAMETER_SKYBOX);

	std::shared_ptr<CMaterial> pSkyBoxMaterial = std::make_shared<CMaterial>();
	pSkyBoxMaterial->SetTexture(pSkyBoxTexture);
	pSkyBoxMaterial->SetShader(pSkyBoxShader);

	m_pMaterials.resize(1);
	SetMaterial(pSkyBoxMaterial, 0);
}

CSkyBox::~CSkyBox()
{
}

void CSkyBox::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if(pCamera)
	{
		XMFLOAT3 xmf3CameraPos = pCamera->GetPosition();
		SetPosition(xmf3CameraPos.x, xmf3CameraPos.y, xmf3CameraPos.z);
	}
	UpdateWorldMatrix();

	CGameObject::Render(pd3dCommandList, pCamera);
}

///////////////////////////////////////////////////////////////////////////////
//

CHeightMapTerrain::CHeightMapTerrain(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color)
	: CGameObject(pd3dDevice, pd3dCommandList)
{
	//지형에 사용할 높이 맵의 가로, 세로의 크기이다. 
	m_nWidth = nWidth;
	m_nLength = nLength;

	/*지형 객체는 격자 메쉬들의 배열로 만들 것이다.
	nBlockWidth, nBlockLength는 격자 메쉬 하나의 가로, 세로 크기이다.
	cxQuadsPerBlock, czQuadsPerBlock은 격자 메쉬의 가로 방향과 세로 방향 사각형의 개수이다.*/
	int cxQuadsPerBlock = nBlockWidth - 1;
	int czQuadsPerBlock = nBlockLength - 1;

	long cxBlocks = (nWidth - 1) / cxQuadsPerBlock;
	long czBlocks = (nLength - 1) / czQuadsPerBlock;

	//xmf3Scale는 지형을 실제로 몇 배 확대할 것인가를 나타낸다. 
	m_xmf3Scale = xmf3Scale;

	//지형에 사용할 높이 맵을 생성한다. 
	m_pHeightMapImage = std::make_shared<CHeightMapImage>(pFileName, nWidth, nLength, xmf3Scale);

	{
		std::shared_ptr<CMesh> pHeightMapGridMesh;
		std::shared_ptr<CGameObject> pHeightMapGameObject;
		for (int z = 0, zStart = 0; z < czBlocks; z++)
		{
			for (int x = 0, xStart = 0; x < cxBlocks; x++)
			{
				pHeightMapGameObject = std::make_shared<CGameObject>();
				pHeightMapGameObject->MaterialResize(1);
				xStart = x * (nBlockWidth - 1);
				zStart = z * (nBlockLength - 1);
				pHeightMapGridMesh = std::make_shared<CHeightMapGridMesh>(pd3dDevice, pd3dCommandList, xStart, zStart, nBlockWidth, nBlockLength, xmf3Scale, xmf4Color, m_pHeightMapImage.get());
				pHeightMapGameObject->SetMesh(pHeightMapGridMesh);
				SetChild(pHeightMapGameObject);
			}
		}
	}
	//{

	//	//지형 전체를 표현하기 위한 격자 메쉬에 대한 포인터 배열을 생성한다. 
	//	CHeightMapGridMesh* pHeightMapGridMesh = NULL;
	//	//지형의 일부분을 나타내는 격자 메쉬를 생성하여 지형 메쉬에 저장한다. 
	//	pHeightMapGridMesh = new CHeightMapGridMesh(pd3dDevice, pd3dCommandList, 0,
	//		0, nBlockWidth, nBlockLength, xmf3Scale, xmf4Color, m_pHeightMapImage);

	//	SetMesh(pHeightMapGridMesh);
	//}

	//지형을 렌더링하기 위한 셰이더를 생성한다. 

#ifdef _WITH_TERRAIN_TESSELATION
	CTerrainTessellationShader* pShader = new CTerrainTessellationShader();
#else
	std::shared_ptr<CShader> pShader = std::make_shared<CTerrainShader>();
#endif
	pShader->CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
	pShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	SetShader(pShader);

	// 이 define 은 stdafx.h 에서 정의되어 있다.
	std::shared_ptr<CTexture> pTexture = std::make_shared<CTexture>(2, RESOURCE_TEXTURE2D, 1);
	pTexture->LoadTextureFromWICFile(pd3dDevice, pd3dCommandList, L"Image/Stone01.jpg", RESOURCE_TEXTURE2D, 0);
	pTexture->LoadTextureFromWICFile(pd3dDevice, pd3dCommandList, L"Image/Grass.jpg", RESOURCE_TEXTURE2D, 1);

	CScene::CreateShaderResourceViews(pd3dDevice, pTexture.get(), 0, ROOT_PARAMETER_TEXTURES);

	m_pMaterials.resize(1);
	m_pMaterials[0]->SetTexture(pTexture);
}

CHeightMapTerrain::~CHeightMapTerrain()
{
}

void CHeightMapTerrain::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	UpdateWorldMatrix();

	CGameObject::Render(pd3dCommandList, pCamera);
}
