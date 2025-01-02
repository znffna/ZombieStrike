///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// GameObject.h : CGameObject Ŭ������ ��� ����
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"
#include "Component.h"

#define COMPONENT_KEY(T) typeid(T).name()

// ���ӿ�����Ʈ �±� - ���ӿ�����Ʈ�� ������ �����ϱ� ���� �±�
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

// ���ӿ�����Ʈ ���̾� - ���ӿ�����Ʈ�� ����ó�� �з��� ���� ���̾�
enum OBJECT_LAYER
{
	OBJECT_LAYER_Default = 0x00,
	OBJECT_LAYER_UI,
};

// Helper function to get string from Tag enum
std::string GetStringFromTag(OBJECT_TAG tag);

// Helper function to get string from OBJECT_LAYER enum
std::string GetStringFromLayer(OBJECT_LAYER layer);

//////////////////////////////////////////////////////////////////////////////////
// CGameObject Class Definition

class CTransform;

class CGameObject : public std::enable_shared_from_this<CGameObject> 
	// Class�� ����Լ����� (this��)shared_ptr�� ����ϱ� ���� enable_shared_from_this�� ��ӹ޴´�.
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

	// Object Components
	template <typename T>
	std::shared_ptr<T> AddComponent()
	{
		// T�� CComponent�� ��ӹ޴��� ������ Ÿ�ӿ� Ȯ��
		static_assert(std::is_base_of<CComponent, T>::value,
			"T must inherit from CComponent");

		std::shared_ptr<T> pComponent = std::make_shared<T>(shared_from_this());

		m_mapComponents[COMPONENT_KEY(T)] = pComponent; // CComponent�� GetName() �Լ��� �ʿ�, �̸��� Key�� ���
		return pComponent;
	}

	template <typename T>
	std::shared_ptr<T> GetComponent()
	{
		// T�� CComponent�� ��ӹ޴��� ������ Ÿ�ӿ� Ȯ��
		static_assert(std::is_base_of<CComponent, T>::value,
			"T must inherit from CComponent");

		auto iter = m_mapComponents.find(COMPONENT_KEY(T));

		if (iter != m_mapComponents.end())
		{
			return std::dynamic_pointer_cast<T>(iter->second);
		}
		return nullptr;
	}

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
	std::shared_ptr<CTransform> m_Transform; // Object Transform

	// GameObject Components
	std::unordered_map<std::string, std::shared_ptr<CComponent>> m_mapComponents; // Object Components
};

