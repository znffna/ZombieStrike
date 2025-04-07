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
	CComponent();
	virtual ~CComponent();

	virtual std::shared_ptr<CComponent> Clone() const = 0;

	void SetOwner(std::weak_ptr<CGameObject> pOwnerGameObject) { m_pOwnerGameObject = pOwnerGameObject; }
	const std::shared_ptr<CGameObject> GetOwner() { return m_pOwnerGameObject.lock(); }

	virtual void Update(float fTimeElapsed) { }

private:
	std::weak_ptr<CGameObject> m_pOwnerGameObject;
};

