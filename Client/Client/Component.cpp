///////////////////////////////////////////////////////////////////////////////
// Date: 2025-03-14
// Component.cpp : CComponent 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#include "Component.h"

CComponent::CComponent(CGameObject* pObject)
	: m_bActive(true), gameObject(pObject)
{
	Initialize(); // CComponent의 요소 초기화
}

CComponent::~CComponent()
{
	OnDestroy(); // CComponent의 요소만 제거
}

// 복사할당 연산자: owner는 복사하지 않고, 나머지 멤버만 복사한다.
CComponent& CComponent::operator=(const CComponent& rhs)
{
	if (this == &rhs) return *this;

	// gameObject는 복사하지 않음(= owner 제외)
	m_bActive = rhs.m_bActive;

	return *this;
}

