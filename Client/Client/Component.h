///////////////////////////////////////////////////////////////////////////////
// Date: 2025-03-14
// Component.h : CComponent 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once
#include "stdafx.h"

//#define _WITH_TRANSFORM_HIERARCHY

class CGameObject;

class CComponent
{
public:
	CComponent();
	virtual ~CComponent();

	void SetOwner(CGameObject* pOwnerGameObject) { m_pOwnerGameObject = pOwnerGameObject; }
	const CGameObject* GetOwner() { return m_pOwnerGameObject; }

private:
	CGameObject* m_pOwnerGameObject = nullptr;
};

//////////////////////////////////////////////////////////////////////////
//
class CTransformComponent;
using CTransform = CTransformComponent; // Alias


class CTransformComponent : public CComponent, public std::enable_shared_from_this<CTransform>
{
public:
	CTransformComponent() { }
	~CTransformComponent() { }

	// Transform Getter
	DirectX::XMFLOAT3 GetPosition() { return m_xmf3Position; }
	DirectX::XMFLOAT3 GetRight() { return Vector3::Normalize(XMFLOAT3(m_xmf4x4World._31, m_xmf4x4World._32, m_xmf4x4World._33)); }
	DirectX::XMFLOAT3 GetUp() { return (Vector3::Normalize(XMFLOAT3(m_xmf4x4World._21, m_xmf4x4World._22, m_xmf4x4World._23))); }
	DirectX::XMFLOAT3 GetLook() { return (Vector3::Normalize(XMFLOAT3(m_xmf4x4World._11, m_xmf4x4World._12, m_xmf4x4World._13))); }
	DirectX::XMFLOAT3 GetScale() { return m_xmf3Scale; }

	DirectX::XMFLOAT3 GetRotation() { return m_xmf3Rotation; }
	float GetPitch() { return m_xmf3Rotation.x; } // X 축을	기준으로 회전
	float GetYaw() { return m_xmf3Rotation.y; } // Y 축을 기준으로 회전
	float GetRoll() { return m_xmf3Rotation.z; } // Z 축을 기준으로 회전

	DirectX::XMFLOAT4X4 GetLocalMatrix() { return m_xmf4x4Local; }
	DirectX::XMFLOAT4X4 GetWorldMatrix() { return m_xmf4x4World; }

	DirectX::XMFLOAT3 GetLocalPosition() { return(XMFLOAT3(m_xmf4x4Local._41, m_xmf4x4Local._42, m_xmf4x4Local._43)); };

	// Transform Setter
	void SetPosition(DirectX::XMFLOAT3 xmf3Position) { SetPosition(xmf3Position.x, xmf3Position.y, xmf3Position.z); }
	void SetPosition(float fx, float fy, float fz);
	void SetScale(DirectX::XMFLOAT3 xmf3Scale) { SetScale(xmf3Scale.x, xmf3Scale.y, xmf3Scale.z); };
	void SetScale(float fx, float fy, float fz);

	void Move(DirectX::XMFLOAT3 xmf3Shift);
	void Move(float x, float y, float z) { Move(DirectX::XMFLOAT3(x, y, z)); }

	void MoveStrafe(float fDistance = 1.0f);
	void MoveUp(float fDistance = 1.0f);
	void MoveForward(float fDistance = 1.0f);

	void Rotate(float fPitch = 10.0f, float fYaw = 10.0f, float fRoll = 10.0f);
	void Rotate(const XMFLOAT3& pxmf3Axis, float fAngle);
	void Rotate(const XMFLOAT4& pxmf4Quaternion);

	XMFLOAT3 ExtractEulerAngles(const XMFLOAT4X4& worldMatrix, const XMFLOAT3& scale);

	void SetLocalMatrix(DirectX::XMFLOAT4X4 xmf4x4Local) { m_xmf4x4Local = xmf4x4Local; }
	void SetWorldMatrix(DirectX::XMFLOAT4X4 xmf4x4World) { m_xmf4x4World = xmf4x4World; }

	void UpdateTransform(const DirectX::XMFLOAT4X4* xmf4x4ParentMatrix = nullptr);
	void UpdateTransform(const std::shared_ptr<CGameObject> pParentObject);	

private:
	// Transform
#ifdef _DEBUG
	DirectX::XMFLOAT3 m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f); // 위치
	DirectX::XMFLOAT3 m_xmf3Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f); // 회전[Euler Angle]
	DirectX::XMFLOAT3 m_xmf3Scale = XMFLOAT3(1.0f, 1.0f, 1.0f); // 크기
#endif
	DirectX::XMFLOAT4X4 m_xmf4x4Local = Matrix4x4::Identity(); // Local Matrix [즉시 갱신]
	DirectX::XMFLOAT4X4 m_xmf4x4World = Matrix4x4::Identity(); // World Matrix [UpdateMatrix()로 갱신]


#ifdef _WITH_TRANSFORM_HIERARCHY
public:
	// 계층 구조	

	/*
	std::shared_ptr<CTransform> GetParent() { return m_pParentTransform.lock(); }
	std::shared_ptr<CTransform> GetChildren(int nIndex) { return m_vecChildTransforms[nIndex]; }
	std::vector<std::shared_ptr<CTransform>> GetChildrens() { return m_vecChildTransforms; }

	void SetParent(std::shared_ptr<CTransform> pParentTransform);
	void SetChild(std::shared_ptr<CTransform> pChildTransform);

	void RemoveChild(std::shared_ptr<CTransform> pChildTransform);
	void RemoveAllChildren() { m_vecChildTransforms.clear(); };
	

private:
	// 계층 구조
	std::weak_ptr<CTransform> m_pParentTransform;
	std::vector<std::shared_ptr<CTransform>> m_vecChildTransforms;
	*/
#endif
};

//////////////////////////////////////////////////////////////////////////
//
