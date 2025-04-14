///////////////////////////////////////////////////////////////////////////////
// Date: 2025-03-14
// Component.h : CComponent Ŭ������ ��� ����
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
	CComponent(CGameObject* pObject = nullptr);
	virtual ~CComponent();

	virtual void Init(CGameObject* pObject) {};

	virtual std::shared_ptr<CComponent> Clone() const = 0;

	virtual void Update(float fTimeElapsed) { }
};

