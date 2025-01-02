///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-30
// Component.h : CComponent 클래스의 헤더 파일
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
	CComponent(const std::shared_ptr<CGameObject>& pOwnerObject);
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
	std::weak_ptr<CGameObject> m_pOwnerGameObject; // 소유하는 게임 객체 [ 소유권은 가지지 않음 ]
	bool m_bActive; // 활성화 여부
};

///////////////////////////////////////////////////////////////////////////////
//

class CTransform : public CComponent
{
public:
///////////////////////////////////////////////////////////////////////////
// Special Constructor / Destructor
///////////////////////////////////////////////////////////////////////////
	CTransform(const std::shared_ptr<CGameObject>& pOwnerObject);
	virtual ~CTransform();

///////////////////////////////////////////////////////////////////////////
// override Function
///////////////////////////////////////////////////////////////////////////
	// Transform은 상시 Active이다. 
	virtual void SetActive(bool bActive) override { }

///////////////////////////////////////////////////////////////////////////
// CTransform setter / getter
///////////////////////////////////////////////////////////////////////////
	// Dirty
	bool IsDirty() { return m_bLocalDirty; }

	// Position
	void SetPosition(const XMFLOAT3& xmf3Position) { m_xmf3Position = xmf3Position; }
	void SetPosition(const float fx, const float fy, const float fz) { m_xmf3Position = XMFLOAT3(fx, fy, fz); }
	XMFLOAT3 GetPosition() { return m_xmf3Position; }

	// Rotation
		// SetRotation 함수는 사원수[Quaternion]를 받아서 저장한다.
	void SetRotation(const XMFLOAT4& xmf4Rotation) { m_xmf4Rotation = xmf4Rotation; } 
		// SetRotation 함수는 오일러 각도를 받아서 사원수로 변환하여 저장한다.
	void SetRotation(const XMFLOAT3& xmf3Rotation) { SetRotation(xmf3Rotation.x, xmf3Rotation.y, xmf3Rotation.z); }
	void SetRotation(const float fRoll, const float fpitch, const float fyaw) { XMStoreFloat4(&m_xmf4Rotation, XMQuaternionRotationRollPitchYaw(fRoll, fpitch, fyaw)); }
	XMFLOAT4 GetRotation() { return m_xmf4Rotation; }
	XMFLOAT3 GetRollPitchYaw() {
		// TODO: 사원수를 오일러 각도로 변환하여 반환
		return XMFLOAT3(0.0f, 0.0f, 0.0f);  // 임시로 0, 0, 0 반환
	}
	// Scale
	void SetScale(const XMFLOAT3& xmf3Scale) { m_xmf3Scale = xmf3Scale; }
	void SetScale(const float fx, const float fy, const float fz) { m_xmf3Scale = XMFLOAT3(fx, fy, fz); }
	XMFLOAT3 GetScale() { return m_xmf3Scale; }

///////////////////////////////////////////////////////////////////////////
// CTransform method
///////////////////////////////////////////////////////////////////////////
	// Transform 변경 체크 및 업데이트
	void SetLocalDirty();
	void SetWorldDirty(bool bPropagation);

	// 이동
	void Move(const XMFLOAT3& xmf3Shift) { m_xmf3Position = XMFLOAT3(m_xmf3Position.x + xmf3Shift.x, m_xmf3Position.y + xmf3Shift.y, m_xmf3Position.z + xmf3Shift.z); SetLocalDirty(); }
	void Move(const float fx, const float fy, const float fz) { Move(XMFLOAT3(fx, fy, fz)); }
	// 회전
		// 사원수로 회전
	void Rotate(const XMFLOAT4& xmf4RotateQuaternion);
		// 오일러 각도로 회전
	void Rotate(const XMFLOAT3& eulerAngles);
	void Rotate(float roll, float pitch, float yaw) {Rotate(XMFLOAT3(roll, pitch, yaw));}
	// 크기 조정
	void Scale(const XMFLOAT3& xmf3Scale) { m_xmf3Scale = XMFLOAT3(m_xmf3Scale.x * xmf3Scale.x, m_xmf3Scale.y * xmf3Scale.y, m_xmf3Scale.z * xmf3Scale.z); SetLocalDirty();	}
	void Scale(const float fx, const float fy, const float fz) { Scale(XMFLOAT3(fx, fy, fz)); }

	// 행렬 반환
	XMFLOAT4X4 GetLocalMatrix();
	XMFLOAT4X4 GetWorldMatrix();

	// 월드 행렬 업데이트 [ bUpdateChild -> true시 계층구조 전부 Update ]
	void UpdateWorldMatrix(bool bUpdateChild);
	void UpdateWorldMatrixTopDown() { UpdateWorldMatrix(true); }
///////////////////////////////////////////////////////////////////////////
// Member Variable
private:
	// Transform 정보
	XMFLOAT3 m_xmf3Position; // 위치
	XMFLOAT4 m_xmf4Rotation; // 회전 [ 사원수, Quaternion ]
	XMFLOAT3 m_xmf3Scale; // 크기

	XMFLOAT4X4 m_xmf4x4LocalMatrix; // 로컬 행렬
	XMFLOAT4X4 m_xmf4x4WorldMatrix; // 월드 행렬
	bool m_bLocalDirty = true; // Local Matrix를 다시 계산해야하는지 여부
	bool m_bWorldDirty = true; // World Matrix를 다시 계산해야하는지 여부

	// 부모 Transform
	std::shared_ptr<CTransform> m_pParentTransform;

	// 자식 Transform
	std::vector<std::shared_ptr<CTransform>> m_vecpChildTransforms; // 자식 Transform
};
