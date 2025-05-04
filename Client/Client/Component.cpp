///////////////////////////////////////////////////////////////////////////////
// Date: 2025-03-14
// Component.cpp : CComponent 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#include "Component.h"

CComponent::CComponent(CGameObject* pObject)
	: gameObject(pObject), m_bActive(true)
{
	if (gameObject) Init(gameObject);
}

CComponent::~CComponent()
{
}

