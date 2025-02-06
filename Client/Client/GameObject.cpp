///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// GameObject.cpp : GameObject 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#include "GameObject.h"
#include "Shader.h"

CGameObject::CGameObject()
{
	// Object Info
	static UINT nGameObjectID = 0;
	m_bActive = true;
	m_nObjectID = nGameObjectID++;

	m_strName = "GameObject_" + std::to_string(m_nObjectID);

	// Transform
	m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_xmf3Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_xmf3Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);

	m_xmf4x4Local = Matrix4x4::Identity();
	m_xmf4x4World = Matrix4x4::Identity();

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

void CGameObject::Render(ID3D12GraphicsCommandList* pd3dCommandList)
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
		pChild->Render(pd3dCommandList);
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
	// Create Constant Buffer
	UINT ncbElementBytes = ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255); //256의 배수
	m_pd3dcbGameObject = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_GENERIC_READ, NULL);

	// Map Constant Buffer
	m_pd3dcbGameObject->Map(0, nullptr, (void**)&m_pcbMappedObject);
	ZeroMemory(m_pcbMappedObject, sizeof(CB_GAMEOBJECT_INFO));
}

void CGameObject::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
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
#endif

	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = m_pd3dcbGameObject->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(ROOT_PARAMETER_OBJECT, d3dGpuVirtualAddress);
}

void CGameObject::ReleaseShaderVariables()
{
	if (m_pd3dcbGameObject) m_pd3dcbGameObject->Unmap(0, nullptr);
	m_pd3dcbGameObject.Reset();
}

///////////////////////////////////////////////////////////////////////////////
//

CRotatingObject::CRotatingObject()
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
