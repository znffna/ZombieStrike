#pragma once

#include "stdafx.h"

class CGameObject
{
public:
	CGameObject();
	virtual ~CGameObject();

	// Active Flag
	bool IsActive() { return m_bActive; }
	void SetActive(bool bActive) { m_bActive = bActive; }

	// Object ID
	UINT GetObjectID() { return m_nObjectID; }
	void SetObjectID(UINT nObjectID) { m_nObjectID = nObjectID; }

	// Object Name
	std::string GetName() { return m_strName; }
	void SetName(std::string strName) { m_strName = strName; }

	// Local Variables
	DirectX::XMFLOAT3 GetPosition() { return m_xmf3Position; }
	void SetPosition(DirectX::XMFLOAT3 xmf3Position) { m_xmf3Position = xmf3Position; }

	DirectX::XMFLOAT3 GetRotation() { return m_xmf3Rotation; }
	void SetRotation(DirectX::XMFLOAT3 xmf3Rotation) { m_xmf3Rotation = xmf3Rotation; }

	DirectX::XMFLOAT3 GetScale() { return m_xmf3Scale; }
	void SetScale(DirectX::XMFLOAT3 xmf3Scale) { m_xmf3Scale = xmf3Scale; }

	// Object World Matrix
	DirectX::XMFLOAT4X4 GetWorldMatrix() { return m_xmf4x4World; }
	void SetWorldMatrix(DirectX::XMFLOAT4X4 xmf4x4World) { m_xmf4x4World = xmf4x4World; }

	// Update Object World Matrix
	void UpdateWorldMatrix(DirectX::XMFLOAT4X4* xmf4x4ParentMatrix = nullptr);

	// Object Initialization / Release
	virtual void Initialize() {};
	virtual void Release() {}

	// Object Update
	virtual void Update(float fTimeElapsed) {}

	// Object Render
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList);

	// Object Collision
	virtual void OnCollision(CGameObject* pGameObject) {}


private:
	bool m_bActive; // Active Flag

	UINT m_nObjectID; // Object ID
	std::string m_strName;  // Object Name

	// Transform
	DirectX::XMFLOAT3 m_xmf3Position; // 위치
	DirectX::XMFLOAT3 m_xmf3Rotation; // 회전
	DirectX::XMFLOAT3 m_xmf3Scale; // 크기

	DirectX::XMFLOAT4X4 m_xmf4x4Local; // Object Local Matrix
	DirectX::XMFLOAT4X4 m_xmf4x4World; // Object World Matrix

};

