#include "GameObject.h"

CGameObject::CGameObject()
{
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
