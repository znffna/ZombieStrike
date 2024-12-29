///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// GameObject.cpp : GameObject 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#include "GameObject.h"

CGameObject::CGameObject()
{
	static int nObjectID;
	m_bActive = true;
	m_nObjectID = nObjectID++;
	m_strName = "GameObject" + std::to_string(nObjectID);
	m_strTag = "Untagged";
	m_strLayer = "Default";
}

CGameObject::~CGameObject()
{
}

void CGameObject::FixedUpdate(float deltaTime)
{

}

void CGameObject::Render(ID3D12GraphicsCommandList* pd3dCommandList)
{

}
