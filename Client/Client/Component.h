///////////////////////////////////////////////////////////////////////////////
// Date: 2025-03-14
// Component.h : CComponent 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once
#include "stdafx.h"

//#define _WITH_TRANSFORM_HIERARCHY

class CGameObject;
class CMesh;

class CComponent
{
public:
	CComponent();
	virtual ~CComponent();

	void SetOwner(std::weak_ptr<CGameObject> pOwnerGameObject) { m_pOwnerGameObject = pOwnerGameObject; }
	const std::shared_ptr<CGameObject> GetOwner() { return m_pOwnerGameObject.lock(); }

	virtual void Update(float fTimeElapsed) { }

private:
	std::weak_ptr<CGameObject> m_pOwnerGameObject;
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
	DirectX::XMFLOAT3 GetRight() { return Vector3::Normalize(XMFLOAT3(m_xmf4x4World._11, m_xmf4x4World._12, m_xmf4x4World._13)); }
	DirectX::XMFLOAT3 GetUp() { return (Vector3::Normalize(XMFLOAT3(m_xmf4x4World._21, m_xmf4x4World._22, m_xmf4x4World._23))); }
	DirectX::XMFLOAT3 GetLook() { return (Vector3::Normalize(XMFLOAT3(m_xmf4x4World._31, m_xmf4x4World._32, m_xmf4x4World._33))); }
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
	DirectX::XMFLOAT3 m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f); // 위치
	DirectX::XMFLOAT3 m_xmf3Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f); // 회전[Euler Angle]
	DirectX::XMFLOAT3 m_xmf3Scale = XMFLOAT3(1.0f, 1.0f, 1.0f); // 크기

	DirectX::XMFLOAT4X4 m_xmf4x4Local = Matrix4x4::Identity(); // Local Matrix [즉시 갱신]
	DirectX::XMFLOAT4X4 m_xmf4x4World = Matrix4x4::Identity(); // World Matrix [UpdateMatrix()로 갱신]
};

//////////////////////////////////////////////////////////////////////////
//
class CCamera;

class CColliderComponent;
using CCollider = CColliderComponent; // Alias
class CColliderComponent : public CComponent
{
public:
	CColliderComponent() { }
	virtual ~CColliderComponent() { }

	virtual void Update(float fTimeElapsed) override;

	virtual void SetCollider(std::shared_ptr<CMesh> pMesh) = 0;
	virtual void SetCollider(const XMFLOAT3& xmf3Center, const XMFLOAT3& Extends) = 0;

	virtual void UpdateCollider(const XMFLOAT4X4& xmf4x4World) = 0;
	//virtual void RenderCollider(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera) = 0;

	virtual bool IsCollided(CColliderComponent* pCollider) = 0;
	virtual bool IsCollided(std::shared_ptr<CColliderComponent> pCollider) {IsCollided(pCollider.get());};

	const enum ColliderType { AABB, OBB, Sphere };

	virtual int GetColliderType() = 0;
};

//////////////////////////////////////////////////////////////////////////
//

class CSphereCollider : public CColliderComponent
{
public:
	CSphereCollider() { }
	virtual ~CSphereCollider() { }

	virtual void SetCollider(std::shared_ptr<CMesh> pMesh) override;
	virtual void SetCollider(const XMFLOAT3& xmf3Center, const XMFLOAT3& Extends) override
	{
		m_xmBoundingSphere.Center = xmf3Center;
		m_xmBoundingSphere.Radius = Vector3::Length(Extends);
	};
	void SetCollider(const XMFLOAT3& xmf3Center, float fRadius)
	{
		m_xmBoundingSphere.Center = xmf3Center;
		m_xmBoundingSphere.Radius = fRadius;
	};

	virtual void UpdateCollider(const XMFLOAT4X4& xmf4x4World) override
	{
		m_xmWorldBoundingSphere.Center = Vector3::TransformCoord(m_xmBoundingSphere.Center, xmf4x4World);
		m_xmWorldBoundingSphere.Radius = m_xmBoundingSphere.Radius;

	};

	virtual bool IsCollided(CColliderComponent* pCollider) override;;

	int GetColliderType() override { return ColliderType::Sphere; };
	const BoundingSphere GetBoundingSphere() { return m_xmWorldBoundingSphere; }
private:
	BoundingSphere m_xmBoundingSphere;
	BoundingSphere m_xmWorldBoundingSphere;
};

class CAABBBoxCollider : public CColliderComponent
{
public:
	CAABBBoxCollider() { }
	virtual ~CAABBBoxCollider() { }

	virtual void SetCollider(std::shared_ptr<CMesh> pMesh) override;
	virtual void SetCollider(const XMFLOAT3& xmf3Center, const XMFLOAT3& xmf3Extents) override
	{
		m_xmBoundingBox.Center = xmf3Center;
		m_xmBoundingBox.Extents = xmf3Extents;
	};

	virtual void UpdateCollider(const XMFLOAT4X4& xmf4x4World) override
	{
		m_xmBoundingBox.Transform(m_xmWorldBoundingBox, XMLoadFloat4x4(&xmf4x4World));
	};

	virtual bool IsCollided(CColliderComponent* pCollider) override;;

	const BoundingBox GetBoundingBox() { return m_xmBoundingBox; }
	int GetColliderType() override { return ColliderType::AABB; }

private:
	BoundingBox m_xmBoundingBox;
	BoundingBox m_xmWorldBoundingBox;
};

class COBBBoxCollider : public CColliderComponent
{
public:
	COBBBoxCollider() { }
	virtual ~COBBBoxCollider() { }

	virtual void SetCollider(std::shared_ptr<CMesh> pMesh) override;
	virtual void SetCollider(const XMFLOAT3& xmf3Center, const XMFLOAT3& xmf3Extents) override
	{
		m_xmBoundingOrientedBox.Center = xmf3Center;
		m_xmBoundingOrientedBox.Extents = xmf3Extents;
	};
	void SetCollider(const BoundingOrientedBox& OBB) {m_xmBoundingOrientedBox = OBB;};

	virtual void UpdateCollider(const XMFLOAT4X4& xmf4x4World) override
	{
		m_xmBoundingOrientedBox.Transform(m_xmWorldBoundingOrientedBox, XMLoadFloat4x4(&xmf4x4World));
	};

	virtual bool IsCollided(CColliderComponent* pCollider) override;;

	const BoundingOrientedBox GetBoundingOrientedBox() { return m_xmWorldBoundingOrientedBox; }
	int GetColliderType() override { return ColliderType::OBB; }

private:

	BoundingOrientedBox m_xmBoundingOrientedBox;
	BoundingOrientedBox m_xmWorldBoundingOrientedBox;
};

//////////////////////////////////////////////////////////////////////////
//

class CRigidBodyComponent;
using CRigidBody = CRigidBodyComponent; // Alias

class CRigidBodyComponent : public CComponent
{
public:
	CRigidBodyComponent() {};
	virtual ~CRigidBodyComponent() {};

	void SetVelocity(const XMFLOAT3& xmf3Velocity) { m_xmf3Velocity = xmf3Velocity; }
	void SetVelocity(float x, float y, float z) { m_xmf3Velocity = XMFLOAT3(x, y, z); }
	void SetGravity(const XMFLOAT3& xmf3Gravity) { m_xmf3Gravity = xmf3Gravity; }
	void SetGravity(float x, float y, float z) { m_xmf3Gravity = XMFLOAT3(x, y, z); }
	void SetMaxVelocityXZ(float fMaxVelocityXZ) { m_fMaxVelocityXZ = fMaxVelocityXZ; }
	void SetMaxVelocityY(float fMaxVelocityY) { m_fMaxVelocityY = fMaxVelocityY; }
	void SetFriction(float fFriction) { m_fFriction = fFriction; }

	XMFLOAT3 GetVelocity() { return m_xmf3Velocity; }
	XMFLOAT3 GetGravity() { return m_xmf3Gravity; }
	float GetMaxVelocityXZ() { return m_fMaxVelocityXZ; }
	float GetMaxVelocityY() { return m_fMaxVelocityY; }
	float GetFriction() { return m_fFriction; }

	void AddVelocity(const XMFLOAT3& xmf3Velocity) { m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, xmf3Velocity); }
	void AddVelocity(float x, float y, float z) { m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, XMFLOAT3(x, y, z)); }
	void AddGravity(const XMFLOAT3& xmf3Gravity) { m_xmf3Gravity = Vector3::Add(m_xmf3Gravity, xmf3Gravity); }
	void AddGravity(float x, float y, float z) { m_xmf3Gravity = Vector3::Add(m_xmf3Gravity, XMFLOAT3(x, y, z)); }
	
	void UpdateRigidBody(float fTimeElapsed);

	void UpdateVelocity(float fTimeElapsed);

	virtual void Update(float fTimeElapsed) override
	{
		UpdateRigidBody(fTimeElapsed);
	}

private:
	XMFLOAT3					m_xmf3Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3     				m_xmf3Gravity = XMFLOAT3(0.0f, -400.0f, 0.0f);
	float           			m_fMaxVelocityXZ = 300.0f;
	float           			m_fMaxVelocityY = 400.0f;
	float           			m_fFriction = 250.0f;

public:
	virtual void OnTerrainUpdateCallback(float fTimeElapsed);
	void SetTerrainUpdatedContext(LPVOID pContext) { m_pTerrainUpdatedContext = pContext; }

	LPVOID						m_pTerrainUpdatedContext = NULL;
};
