///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// GameObject.h : CGameObject 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"
#include "Mesh.h"

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
	float GetPitch() { return m_xmf3Rotation.x; } // X 축을	기준으로 회전
	float GetYaw() { return m_xmf3Rotation.y; } // Y 축을 기준으로 회전
	float GetRoll() { return m_xmf3Rotation.z; } // Z 축을 기준으로 회전

	void SetRotation(DirectX::XMFLOAT3 xmf3Rotation) { m_xmf3Rotation = xmf3Rotation; }

	DirectX::XMFLOAT3 GetScale() { return m_xmf3Scale; }
	void SetScale(DirectX::XMFLOAT3 xmf3Scale) { m_xmf3Scale = xmf3Scale; }

	// Object World Matrix
	DirectX::XMFLOAT4X4 GetWorldMatrix() { return m_xmf4x4World; }
	void SetWorldMatrix(DirectX::XMFLOAT4X4 xmf4x4World) { m_xmf4x4World = xmf4x4World; }

	// Update Object World Matrix
	void UpdateWorldMatrix(DirectX::XMFLOAT4X4* xmf4x4ParentMatrix = nullptr);

	// Move
	void Move(DirectX::XMFLOAT3 xmf3Shift) { m_xmf3Position = Vector3::Add(m_xmf3Position, xmf3Shift); }
	void Move(float x, float y, float z) { m_xmf3Position = Vector3::Add(m_xmf3Position, DirectX::XMFLOAT3(x, y, z)); }

	// Rotate
	void Rotate(DirectX::XMFLOAT3 xmf3Rotate) { m_xmf3Rotation = Vector3::Add(m_xmf3Rotation, xmf3Rotate); }

	// Object Initialization / Release
	virtual void Initialize() {};
	virtual void Release() {}

	// Object Update
	virtual void Update(float fTimeElapsed) {}

	// Object Render
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList);

	// Object Collision
	//virtual void OnCollision(CGameObject* pGameObject) {}


private:
	bool m_bActive; // Active Flag

	UINT m_nObjectID; // Object ID
	std::string m_strName;  // Object Name

protected:
	// Transform
	DirectX::XMFLOAT3 m_xmf3Position; // 위치
	DirectX::XMFLOAT3 m_xmf3Rotation; // 회전
	DirectX::XMFLOAT3 m_xmf3Scale; // 크기

	DirectX::XMFLOAT4X4 m_xmf4x4Local; // Object Local Matrix
	DirectX::XMFLOAT4X4 m_xmf4x4World; // Object World Matrix

};

////////////////////////////////////////////////////////////////////////////////////////
//

class CRotatingObject : public CGameObject
{
public:
	CRotatingObject();
	virtual ~CRotatingObject();

	// Object Update
	virtual void Update(float fTimeElapsed) override;

	// Set Rotation Speed
	void SetRotationSpeed(float fRotationSpeed) { m_fRotationSpeed = fRotationSpeed; }

	// Set Rotation Axis
	void SetRotationAxis(DirectX::XMFLOAT3 xmf3RotationAxis) { m_xmf3RotationAxis = xmf3RotationAxis; }

private:
	float m_fRotationSpeed; // 초당 회전 속도
	XMFLOAT3 m_xmf3RotationAxis; // 회전 축

};
