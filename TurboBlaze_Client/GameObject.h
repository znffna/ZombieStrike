///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// GameObject.h : CGameObject 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"
#include "Component.h"

enum OBJECT_TAG
{
	OBJECT_TAG_UNTAGGED = 0x00,
	OBJECT_TAG_PLAYER,
	OBJECT_TAG_ENEMY,
	OBJECT_TAG_ITEM,
	OBJECT_TAG_BULLET,
	OBJECT_TAG_EFFECT,
	OBJECT_TAG_UI,
	OBJECT_TAG_MAX
};

enum OBJECT_LAYER
{
	OBJECT_LAYER_Default = 0x00,
	OBJECT_LAYER_UI,
	OBJECT_LAYER_Physics,
	OBJECT_LAYER_None
};

// Helper function to get string from Tag enum
std::string GetStringFromTag(OBJECT_TAG tag);

// Helper function to get string from OBJECT_LAYER enum
std::string GetStringFromLayer(OBJECT_LAYER layer);

//////////////////////////////////////////////////////////////////////////////////
// CGameObject Class Definition

class CTransform;

class CGameObject : public std::enable_shared_from_this<CGameObject> 
	// Class의 멤버함수에서 (this를)shared_ptr를 사용하기 위해 enable_shared_from_this를 상속받는다.
{
public:
	///////////////////////////////////////////////////////////////////////////
	// Special Constructor / Destructor
	///////////////////////////////////////////////////////////////////////////
	CGameObject();
	CGameObject(
		const std::string& strName,
		OBJECT_TAG tag = OBJECT_TAG_UNTAGGED,
		OBJECT_LAYER layer = OBJECT_LAYER_Default);
	virtual ~CGameObject();

	///////////////////////////////////////////////////////////////////////////
	// Getter / Setter
	///////////////////////////////////////////////////////////////////////////

	// Active Flag
	bool IsActive() { return m_bActive; }
	void SetActive(bool bActive) { m_bActive = bActive; }

	// Object ID
	int GetObjectID() { return m_nObjectID; }

	// Object Name
	std::string GetName() { return m_strName; }
	void SetName(std::string strName) { m_strName = strName; }

	// Object Tag
	OBJECT_TAG GetTag() { return m_Tag; }
	void SetTag(OBJECT_TAG tag) { m_Tag = tag; }

	// Object Layer
	OBJECT_LAYER GetLayer() { return m_Layer; }
	void SetLayer(OBJECT_LAYER layer) { m_Layer = layer; }

	///////////////////////////////////////////////////////////////////////////
	// Object method
	///////////////////////////////////////////////////////////////////////////
	virtual void FixedUpdate(float deltaTime);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList);

private:
	// GameObject static variable
	static unsigned int nNextObjectID; // class GameObject ID Counter

	// GameObject Identifier
	bool m_bActive; // Active Flag
	unsigned int m_nObjectID; // Object ID
	std::string m_strName;  // Object Name
	OBJECT_TAG m_Tag;   // Object OBJECT_TAG
	OBJECT_LAYER m_Layer; // Object OBJECT_LAYER

	// GameObject Transform
	CTransform m_Transform; // Object Transform

	// GameObject Components
	std::vector<std::shared_ptr<CComponent>> m_vecpComponents; // Object Components
};

