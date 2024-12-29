///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// GameObject.h : CGameObject 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"

class CGameObject
{
public:
	CGameObject();
	virtual ~CGameObject();


	virtual void FixedUpdate(float deltaTime);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList);
private:
	bool m_bActive; // Active Flag
	std::string m_strName;  // Object Name
	std::string m_strTag;   // Object Tag
	std::string m_strLayer; // Object Layer

	int m_nObjectID; // Object ID

};

