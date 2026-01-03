///////////////////////////////////////////////////////////////////////////////
// Date: 2025-03-14
// Component.h : CComponent 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once
#include "stdafx.h"

//#define _WITH_TRANSFORM_HIERARCHY

class CGameObject;
class CMesh;
class CCamera;

class CComponent
{
public:
	CComponent(CGameObject* pObject);
	virtual ~CComponent();

	virtual void Init(CGameObject* pObject) {}

	virtual std::shared_ptr<CComponent> Clone(CGameObject* newOwner) const = 0;

	virtual void Update(float fTimeElapsed) { }

	CGameObject* gameObject; // Owner Object
protected:
	bool m_bActive; // Active Flag
public:
	void SetActive(bool bActive) { m_bActive = bActive; }
	bool IsActive() const { return m_bActive; }
};
