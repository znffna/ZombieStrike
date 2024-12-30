///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// GameObject.cpp : GameObject 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#include "GameObject.h"

// Helper function to get string from Tag enum
std::string GetStringFromTag(OBJECT_TAG tag) {
	static const std::unordered_map<OBJECT_TAG, std::string> tagToString = {
		{ OBJECT_TAG::OBJECT_TAG_PLAYER, "Player" },
		{ OBJECT_TAG::OBJECT_TAG_ENEMY, "Enemy" },
		{ OBJECT_TAG::OBJECT_TAG_ITEM, "Item" },
		{ OBJECT_TAG::OBJECT_TAG_BULLET, "Bullet" },
		{ OBJECT_TAG::OBJECT_TAG_EFFECT, "Effect" },
		{ OBJECT_TAG::OBJECT_TAG_UI, "UI" },
		{ OBJECT_TAG::OBJECT_TAG_UNTAGGED, "UNTAGGED" }
	};
	return tagToString.at(tag);
}

// Helper function to get string from OBJECT_LAYER enum
std::string GetStringFromLayer(OBJECT_LAYER layer) {
	static const std::unordered_map<OBJECT_LAYER, std::string> layerToString = {
		{ OBJECT_LAYER::OBJECT_LAYER_Default, "Default" },
		{ OBJECT_LAYER::OBJECT_LAYER_UI, "UI" },
		{ OBJECT_LAYER::OBJECT_LAYER_Physics, "Physics" },
		{ OBJECT_LAYER::OBJECT_LAYER_None, "None" }
	};
	return layerToString.at(layer);
}

///////////////////////////////////////////////////////////////////////////
// CGameObject Implementation

unsigned int CGameObject::nNextObjectID = 0;

CGameObject::CGameObject()
{
	m_bActive = true;
	m_nObjectID = nNextObjectID++;
	m_strName = "GameObject" + std::to_string(m_nObjectID);
	m_Tag = OBJECT_TAG_UNTAGGED;
	m_Layer = OBJECT_LAYER_Default;
}

CGameObject::CGameObject(const std::string& strName, OBJECT_TAG tag, OBJECT_LAYER layer)
	: m_strName(strName), m_Tag(tag), m_Layer(layer)
{
	m_bActive = true;
	m_nObjectID = nNextObjectID++;
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


