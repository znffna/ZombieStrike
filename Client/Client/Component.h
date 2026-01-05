///////////////////////////////////////////////////////////////////////////////
// Date: 2025-03-14
// Component.h : CComponent 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once
#include "stdafx.h"

//#define _WITH_TRANSFORM_HIERARCHY

class CGameObject;

class CComponent
{
public:
	CComponent(CGameObject* pObject);
	virtual ~CComponent();

	CComponent(const CComponent& rhs) : m_bActive(rhs.m_bActive) {};
	CComponent& operator=(const CComponent& rhs); // owner는 복사하지 않음

	virtual void Initialize() {}
	virtual void OnDestroy() {}

	virtual void Update(float fTimeElapsed) { }

	virtual std::unique_ptr<CComponent> Clone(CGameObject* pNewOwner) const = 0;

	CGameObject* GetOwner() const { return gameObject; }
	CGameObject* gameObject = nullptr; // Owner Object
protected:
	bool m_bActive; // Active Flag

	void SetOwnerInternal(CGameObject* pObject) { gameObject = pObject; }

public:
	void SetActive(bool bActive) { m_bActive = bActive; }
	bool IsActive() const { return m_bActive; }
};
