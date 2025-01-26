///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// GameObject.cpp : GameObject 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#include "GameObject.h"

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

void CGameObject::UpdateWorldMatrix(DirectX::XMFLOAT4X4* xmf4x4ParentMatrix)
{
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
	// Render Object

	// Set Shader Variables
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
