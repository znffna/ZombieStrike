#include "RigidBody.h"
#include "Collider.h"
#include "GameObject.h"

#include "Transform.h"

///////////////////////////////////////////////////////////////////////////////
//

void CRigidBody::Init(CGameObject* pObject)
{
	m_pTransform = pObject->GetComponent<CTransform>();
	m_pCollider = pObject->GetComponent<CCollider>();
}

void CRigidBody::UpdateRigidBody(float fTimeElapsed)
{
	// 중력 적용 및 속도 갱신
	UpdateVelocity(fTimeElapsed);

	// 지형과의 높이 처리
	if (m_pTerrainUpdatedContext) OnTerrainUpdateCallback(fTimeElapsed);

	//TODO 카메라 처리 (이건 카메라를 Component로 하던지, 아무튼 RigidBody에서 하는건 아닌듯 함.)
	/*
	DWORD nCurrentCameraMode = m_pCamera->GetMode();
	if (nCurrentCameraMode == THIRD_PERSON_CAMERA) m_pCamera->Update(m_xmf3Position, fTimeElapsed);
	if (m_pCameraUpdatedContext) OnCameraUpdateCallback(fTimeElapsed);
	if (nCurrentCameraMode == THIRD_PERSON_CAMERA) m_pCamera->SetLookAt(m_xmf3Position);
	m_pCamera->RegenerateViewMatrix();
	*/

	ApplyDamping(fTimeElapsed);
}

// 마찰에 의한 속도 감소
void CRigidBody::ApplyDamping(float fTimeElapsed)
{
	// 이동 후처치(속도 감소(및 정지))
	float fLength = Vector3::Length(m_xmf3Velocity);
	float fDeceleration = (m_fFriction * fTimeElapsed);
	if (fDeceleration > fLength) fDeceleration = fLength;
	m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, Vector3::ScalarProduct(m_xmf3Velocity, -fDeceleration, true));

}

// 일반적인 힘 적용 (중심에 힘을 가할 때)
void CRigidBody::ApplyForce(const XMFLOAT3& f)
{
	XMStoreFloat3(&m_xmf3Force, XMVectorAdd(XMLoadFloat3(&m_xmf3Force), XMLoadFloat3(&f)));
}

// 특정 위치에 힘을 가해 Torque를 생성하는 함수
void CRigidBody::ApplyForceAtPoint(const XMFLOAT3& f, const XMFLOAT3& point)
{
	// Transform에서 World Matrix 가져오기
	XMFLOAT4X4 xmf4x4worldMatrix = m_pTransform->GetWorldMatrix();
	XMMATRIX worldMatrix = XMLoadFloat4x4(&xmf4x4worldMatrix);

	// 중심 좌표 추출 (Matrix에서 Translation 부분 가져오기)
	XMVECTOR centerOfMass = worldMatrix.r[3];

	// XMFLOAT3 → XMVECTOR 변환
	XMVECTOR forceVec = XMLoadFloat3(&f);
	XMVECTOR pointVec = XMLoadFloat3(&point);

	// Torque = r x F (외적)
	XMVECTOR r = XMVectorSubtract(pointVec, centerOfMass);
	XMVECTOR torqueVec = XMVector3Cross(r, forceVec);

	// 결과 저장
	XMStoreFloat3(&m_xmf3Torque, XMVectorAdd(XMLoadFloat3(&m_xmf3Torque), torqueVec));
	XMStoreFloat3(&m_xmf3Force, XMVectorAdd(XMLoadFloat3(&m_xmf3Force), forceVec));
}

// RigidBody의 업데이트 (물리 연산)
void CRigidBody::Integrate(float deltaTime, XMFLOAT3& position, XMFLOAT4& rotation)
{
	// Transform에서 World Matrix 가져오기
	XMFLOAT4X4 xmf4x4worldMatrix = m_pTransform->GetWorldMatrix();
	XMMATRIX worldMatrix = XMLoadFloat4x4(&xmf4x4worldMatrix);
	XMVECTOR positionVec = worldMatrix.r[3]; // Translation(위치)

	// 기존 속도 업데이트
	XMVECTOR velocityVec = XMLoadFloat3(&m_xmf3Velocity);
	velocityVec = XMVectorAdd(velocityVec, XMVectorScale(XMLoadFloat3(&m_xmf3Force), deltaTime));

	// 위치 업데이트
	positionVec = XMVectorAdd(positionVec, XMVectorScale(velocityVec, deltaTime));

	// 각속도 업데이트
	XMVECTOR angularVelocityVec = XMLoadFloat3(&m_xmf3AngularVelocity);
	XMVECTOR torqueVec = XMLoadFloat3(&m_xmf3Torque);

	XMVECTOR angularAcceleration = XMVector3Transform(torqueVec, m_xmmInverseInertiaTensor);
	angularVelocityVec = XMVectorAdd(angularVelocityVec, XMVectorScale(angularAcceleration, deltaTime));

	// 감쇠 적용
	angularVelocityVec = XMVectorScale(angularVelocityVec, 0.99f);

	// 회전 업데이트 (Quaternion이 아니라 Matrix 직접 수정)
	XMMATRIX rotationMatrix = XMMatrixRotationNormal(angularVelocityVec, XMVectorGetX(XMVector3Length(angularVelocityVec)) * deltaTime);
	worldMatrix = XMMatrixMultiply(rotationMatrix, worldMatrix);

	// 새 위치 적용
	worldMatrix.r[3] = positionVec;

	// 업데이트된 Matrix 저장
	m_pTransform->SetWorldMatrix(worldMatrix);

	// 힘 및 토크 초기화 (순간적인 힘 처리)
	m_xmf3Force = { 0, 0, 0 };
	m_xmf3Torque = { 0, 0, 0 };
}

void CRigidBody::ApplyCorrection(const XMFLOAT3& xmf3Correction)
{
	m_pTransform->Move(xmf3Correction);
	m_xmf3Velocity = Vector3::ScalarProduct(m_xmf3Velocity, -1.0f); // 속도 초기화
}

void CRigidBody::UpdateVelocity(float fTimeElapsed)
{
	// 중력 적용
	m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, m_xmf3Gravity);

	// 최대 속도 제한 ========================================
	// 수평 이동 속도 제한
	float fLength = sqrtf(m_xmf3Velocity.x * m_xmf3Velocity.x + m_xmf3Velocity.z * m_xmf3Velocity.z);
	float fMaxVelocityXZ = m_fMaxVelocityXZ;
	if (fLength > m_fMaxVelocityXZ)
	{
		m_xmf3Velocity.x *= (fMaxVelocityXZ / fLength);
		m_xmf3Velocity.z *= (fMaxVelocityXZ / fLength);
	}
	// 수직 이동 속도 제한
	float fMaxVelocityY = m_fMaxVelocityY;
	fLength = sqrtf(m_xmf3Velocity.y * m_xmf3Velocity.y);
	if (fLength > m_fMaxVelocityY) m_xmf3Velocity.y *= (fMaxVelocityY / fLength);

	// 초당 속도에서 이동 거리로 변환
	XMFLOAT3 xmf3Velocity = Vector3::ScalarProduct(m_xmf3Velocity, fTimeElapsed, false);

	// 이동 거리만큼 이동
	m_pTransform->Move(xmf3Velocity);
}

void CRigidBody::OnTerrainUpdateCallback(float fTimeElapsed)
{
	CHeightMapTerrain* pTerrain = (CHeightMapTerrain*)m_pTerrainUpdatedContext;
	XMFLOAT3 xmf3Scale = pTerrain->GetScale();
	int xmf4TerrainSize = pTerrain->GetHeightMapWidth();
	int xmf4TerrainLength = pTerrain->GetHeightMapLength();

	XMFLOAT3 xmf3PlayerPosition = m_pTransform->GetPosition();
	int z = (int)(xmf3PlayerPosition.z / xmf3Scale.z);
	bool bReverseQuad = ((z % 2) != 0);
	//float fHeight = pTerrain->GetHeight(xmf3PlayerPosition.x, xmf3PlayerPosition.z, bReverseQuad) + 6.0f;

	{ // x, z 축에서 Height Map의 넓이, 폭에 한정되게 만듬.
		if (xmf3PlayerPosition.x < 0.0f) xmf3PlayerPosition.x = 0.0f;
		if (xmf3PlayerPosition.x > (xmf4TerrainSize - 1) * xmf3Scale.x) xmf3PlayerPosition.x = (xmf4TerrainSize - 1) * xmf3Scale.x - 1.0f;
		if (xmf3PlayerPosition.z < 0.0f) xmf3PlayerPosition.z = 0.0f;
		if (xmf3PlayerPosition.z > (xmf4TerrainLength - 1) * xmf3Scale.z) xmf3PlayerPosition.z = (xmf4TerrainLength - 1) * xmf3Scale.z - 1.0f;
	}


	float fHeight = 0.0f;
	// 지상과의 높이 체크
	fHeight = pTerrain->GetHeight(xmf3PlayerPosition.x, xmf3PlayerPosition.z);
	if (xmf3PlayerPosition.y < fHeight)
	{
		XMFLOAT3 xmf3PlayerVelocity = GetVelocity();
		xmf3PlayerVelocity.y = 0.0f;
		SetVelocity(xmf3PlayerVelocity);
		xmf3PlayerPosition.y = fHeight;
		m_pTransform->SetPosition(xmf3PlayerPosition);
	}
	
}

