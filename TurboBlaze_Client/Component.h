///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-30
// Component.h : CComponent Ŭ������ ��� ����
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"

class CGameObject;

class CComponent
{
public:
	///////////////////////////////////////////////////////////////////////////
	// Special Constructor / Destructor
	///////////////////////////////////////////////////////////////////////////

	CComponent() = delete; // �⺻ �����ڴ� �����Ǹ� ����ü�� �����̴�.
	CComponent(std::shared_ptr<CGameObject>& pOwnerObject);
	virtual ~CComponent();

	///////////////////////////////////////////////////////////////////////////
	// setter / getter
	///////////////////////////////////////////////////////////////////////////

	// Active
	virtual void SetActive(bool bActive) { m_bActive = bActive; }
	bool IsActive() { return m_bActive; }


	// Owner GameObject
	std::shared_ptr<CGameObject> GetOwnerGameObject() { return m_pOwnerGameObject.lock(); }


private:
	std::weak_ptr<CGameObject> m_pOwnerGameObject; // �����ϴ� ���� ��ü [ �������� ������ ���� ]
	bool m_bActive; // Ȱ��ȭ ����
};

///////////////////////////////////////////////////////////////////////////////
//

class CTransform : public CComponent
{
public:
///////////////////////////////////////////////////////////////////////////
// Special Constructor / Destructor
///////////////////////////////////////////////////////////////////////////
	CTransform(std::shared_ptr<CGameObject>& pOwnerObject);
	virtual ~CTransform();

///////////////////////////////////////////////////////////////////////////
// override Function
///////////////////////////////////////////////////////////////////////////
	// Transform�� ��� Active�̴�. 
	virtual void SetActive(bool bActive) override { }

///////////////////////////////////////////////////////////////////////////
// CTransform setter / getter
///////////////////////////////////////////////////////////////////////////
	// Position
	void SetPosition(const XMFLOAT3& xmf3Position) { m_xmf3Position = xmf3Position; }
	void SetPosition(const float fx, const float fy, const float fz) { m_xmf3Position = XMFLOAT3(fx, fy, fz); }
	XMFLOAT3 GetPosition() { return m_xmf3Position; }

	// Rotation
		// SetRotation �Լ��� �����[Quaternion]�� �޾Ƽ� �����Ѵ�.
	void SetRotation(const XMFLOAT4& xmf4Rotation) { m_xmf4Rotation = xmf4Rotation; } 
		// SetRotation �Լ��� ���Ϸ� ������ �޾Ƽ� ������� ��ȯ�Ͽ� �����Ѵ�.
	void SetRotation(const XMFLOAT3& xmf3Rotation) { SetRotation(xmf3Rotation.x, xmf3Rotation.y, xmf3Rotation.z); }
	void SetRotation(const float fRoll, const float fpitch, const float fyaw) { XMStoreFloat4(&m_xmf4Rotation, XMQuaternionRotationRollPitchYaw(fRoll, fpitch, fyaw)); }
	XMFLOAT4 GetRotation() { return m_xmf4Rotation; }
	XMFLOAT3 GetRollPitchYaw() {
		// TODO: ������� ���Ϸ� ������ ��ȯ�Ͽ� ��ȯ
		return XMFLOAT3(0.0f, 0.0f, 0.0f);  // �ӽ÷� 0, 0, 0 ��ȯ
	}
	// Scale
	void SetScale(const XMFLOAT3& xmf3Scale) { m_xmf3Scale = xmf3Scale; }
	void SetScale(const float fx, const float fy, const float fz) { m_xmf3Scale = XMFLOAT3(fx, fy, fz); }
	XMFLOAT3 GetScale() { return m_xmf3Scale; }

///////////////////////////////////////////////////////////////////////////
// CTransform method
///////////////////////////////////////////////////////////////////////////
	// �̵�
	void Move(const XMFLOAT3& xmf3Shift) { m_xmf3Position = XMFLOAT3(m_xmf3Position.x + xmf3Shift.x, m_xmf3Position.y + xmf3Shift.y, m_xmf3Position.z + xmf3Shift.z); }
	void Move(const float fx, const float fy, const float fz) { Move(XMFLOAT3(fx, fy, fz)); }
	// ȸ��
		// ������� ȸ��
	void Rotate(const XMFLOAT4& xmf4RotateQuaternion);
		// ���Ϸ� ������ ȸ��
	void Rotate(const XMFLOAT3& eulerAngles);
	void Rotate(float roll, float pitch, float yaw) {Rotate(XMFLOAT3(roll, pitch, yaw));}
	// ũ�� ����
	void Scale(const XMFLOAT3& xmf3Scale) { m_xmf3Scale = XMFLOAT3(m_xmf3Scale.x * xmf3Scale.x, m_xmf3Scale.y * xmf3Scale.y, m_xmf3Scale.z * xmf3Scale.z); }
	void Scale(const float fx, const float fy, const float fz) { Scale(XMFLOAT3(fx, fy, fz)); }

	// ��� ��ȯ
	XMFLOAT4X4 GetLocalMatrix() {
		XMFLOAT4X4 xmf4x4Matrix;
		XMMATRIX xmmtxScale = XMMatrixScaling(m_xmf3Scale.x, m_xmf3Scale.y, m_xmf3Scale.z);
		XMMATRIX xmmtxRotation = XMMatrixRotationQuaternion(XMLoadFloat4(&m_xmf4Rotation));
		XMMATRIX xmmtxTranslation = XMMatrixTranslation(m_xmf3Position.x, m_xmf3Position.y, m_xmf3Position.z);

		XMStoreFloat4x4(&xmf4x4Matrix, xmmtxScale * xmmtxRotation * xmmtxTranslation);

		return xmf4x4Matrix;
	}

private:
	XMFLOAT3 m_xmf3Position; // ��ġ
	XMFLOAT4 m_xmf4Rotation; // ȸ�� [ �����, Quaternion ]
	XMFLOAT3 m_xmf3Scale; // ũ��
};
