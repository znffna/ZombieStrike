///////////////////////////////////////////////////////////////////////////////
// Date: 2025-03-28
// RigidBody.h : CRigidBody 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Component.h"
//////////////////////////////////////////////////////////////////////////
//

class CRigidBodyComponent;
using CRigidBody = CRigidBodyComponent; // Alias

class CRigidBodyComponent : public CComponent
{
public:
	CRigidBodyComponent() {};
	virtual ~CRigidBodyComponent() {};

	virtual std::shared_ptr<CComponent> Clone() const {	return std::make_shared<CRigidBodyComponent>(*this);};

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
