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
	void SetMass(float fMass) { m_fMass = fMass; m_fInverseMass = 1.0f / fMass; }
	void SetInverseMass(float fInverseMass) { m_fInverseMass = fInverseMass; m_fMass = 1.0f / fInverseMass; }
	void SetForce(const XMFLOAT3& xmf3Force) { m_xmf3Force = xmf3Force; }
	void SetTorque(const XMFLOAT3& xmf3Torque) { m_xmf3Torque = xmf3Torque; }


	XMFLOAT3 GetVelocity() { return m_xmf3Velocity; }
	XMFLOAT3 GetGravity() { return m_xmf3Gravity; }
	float GetMaxVelocityXZ() { return m_fMaxVelocityXZ; }
	float GetMaxVelocityY() { return m_fMaxVelocityY; }
	float GetFriction() { return m_fFriction; }
	float GetMass() { return m_fMass; }
	float GetInverseMass() { return m_fInverseMass; }
	XMFLOAT3 GetForce() { return m_xmf3Force; }
	XMFLOAT3 GetTorque() { return m_xmf3Torque; }

	void AddVelocity(const XMFLOAT3& xmf3Velocity) { m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, xmf3Velocity); }
	void AddVelocity(float x, float y, float z) { m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, XMFLOAT3(x, y, z)); }
	void AddGravity(const XMFLOAT3& xmf3Gravity) { m_xmf3Gravity = Vector3::Add(m_xmf3Gravity, xmf3Gravity); }
	void AddGravity(float x, float y, float z) { m_xmf3Gravity = Vector3::Add(m_xmf3Gravity, XMFLOAT3(x, y, z)); }
	
	void UpdateRigidBody(float fTimeElapsed);

	void UpdateVelocity(float fTimeElapsed);
	void ApplyDamping(float fTimeElapsed);
	void ApplyForce(const XMFLOAT3& f);
	void ApplyForceAtPoint(const XMFLOAT3& f, const XMFLOAT3& point);
	void Integrate(float deltaTime, XMFLOAT3& position, XMFLOAT4& rotation);

	virtual void Update(float fTimeElapsed) override
	{
		UpdateRigidBody(fTimeElapsed);
	}

private:
	float 		 				m_fMass = 1.0f;
	float			 			m_fInverseMass = 1.0f / m_fMass;

	XMFLOAT3					m_xmf3Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3					m_xmf3Force = XMFLOAT3(0.0f, 0.0f, 0.0f);

	XMFLOAT3					m_xmf3AngularVelocity = { 0, 0, 0 }; // 각속도
	XMFLOAT3					m_xmf3Torque = { 0, 0, 0 };          // 토크

	XMMATRIX					m_xmmInertiaTensor = XMMatrixIdentity();         // 관성 모멘트 행렬
	XMMATRIX					m_xmmInverseInertiaTensor = XMMatrixIdentity(); // 관성 모멘트 역행렬

	XMFLOAT3     				m_xmf3Gravity = XMFLOAT3(0.0f, -400.0f, 0.0f);
	float           			m_fMaxVelocityXZ = 300.0f;
	float           			m_fMaxVelocityY = 400.0f;

	float           			m_fFriction = 250.0f;

public:
	virtual void OnTerrainUpdateCallback(float fTimeElapsed);
	void SetTerrainUpdatedContext(LPVOID pContext) { m_pTerrainUpdatedContext = pContext; }

	LPVOID						m_pTerrainUpdatedContext = NULL;
};
