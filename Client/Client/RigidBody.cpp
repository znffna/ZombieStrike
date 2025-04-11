#include "RigidBody.h"
#include "Collider.h"
#include "GameObject.h"

///////////////////////////////////////////////////////////////////////////////
//

void CRigidBodyComponent::UpdateRigidBody(float fTimeElapsed)
{
	auto owner = GetOwner();

	// �߷� ���� �� �ӵ� ����
	UpdateVelocity(fTimeElapsed);

	// �������� ���� ó��
	if (m_pTerrainUpdatedContext) OnTerrainUpdateCallback(fTimeElapsed);

	//TODO ī�޶� ó�� (�̰� ī�޶� Component�� �ϴ���, �ƹ�ư RigidBody���� �ϴ°� �ƴѵ� ��.)
	/*
	DWORD nCurrentCameraMode = m_pCamera->GetMode();
	if (nCurrentCameraMode == THIRD_PERSON_CAMERA) m_pCamera->Update(m_xmf3Position, fTimeElapsed);
	if (m_pCameraUpdatedContext) OnCameraUpdateCallback(fTimeElapsed);
	if (nCurrentCameraMode == THIRD_PERSON_CAMERA) m_pCamera->SetLookAt(m_xmf3Position);
	m_pCamera->RegenerateViewMatrix();
	*/

	ApplyDamping(fTimeElapsed);
}

// ������ ���� �ӵ� ����
void CRigidBodyComponent::ApplyDamping(float fTimeElapsed)
{
	// �̵� ��óġ(�ӵ� ����(�� ����))
	float fLength = Vector3::Length(m_xmf3Velocity);
	float fDeceleration = (m_fFriction * fTimeElapsed);
	if (fDeceleration > fLength) fDeceleration = fLength;
	m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, Vector3::ScalarProduct(m_xmf3Velocity, -fDeceleration, true));

}

// �Ϲ����� �� ���� (�߽ɿ� ���� ���� ��)
void CRigidBodyComponent::ApplyForce(const XMFLOAT3& f)
{
	XMStoreFloat3(&m_xmf3Force, XMVectorAdd(XMLoadFloat3(&m_xmf3Force), XMLoadFloat3(&f)));
}

// Ư�� ��ġ�� ���� ���� Torque�� �����ϴ� �Լ�
void CRigidBodyComponent::ApplyForceAtPoint(const XMFLOAT3& f, const XMFLOAT3& point)
{
	// Transform���� World Matrix ��������
	XMFLOAT4X4 xmf4x4worldMatrix = GetOwner()->GetWorldMatrix();
	XMMATRIX worldMatrix = XMLoadFloat4x4(&xmf4x4worldMatrix);

	// �߽� ��ǥ ���� (Matrix���� Translation �κ� ��������)
	XMVECTOR centerOfMass = worldMatrix.r[3];

	// XMFLOAT3 �� XMVECTOR ��ȯ
	XMVECTOR forceVec = XMLoadFloat3(&f);
	XMVECTOR pointVec = XMLoadFloat3(&point);

	// Torque = r x F (����)
	XMVECTOR r = XMVectorSubtract(pointVec, centerOfMass);
	XMVECTOR torqueVec = XMVector3Cross(r, forceVec);

	// ��� ����
	XMStoreFloat3(&m_xmf3Torque, XMVectorAdd(XMLoadFloat3(&m_xmf3Torque), torqueVec));
	XMStoreFloat3(&m_xmf3Force, XMVectorAdd(XMLoadFloat3(&m_xmf3Force), forceVec));
}

// RigidBody�� ������Ʈ (���� ����)
void CRigidBodyComponent::Integrate(float deltaTime, XMFLOAT3& position, XMFLOAT4& rotation)
{
	// Transform���� World Matrix ��������
	XMFLOAT4X4 xmf4x4worldMatrix = GetOwner()->GetWorldMatrix();
	XMMATRIX worldMatrix = XMLoadFloat4x4(&xmf4x4worldMatrix);
	XMVECTOR positionVec = worldMatrix.r[3]; // Translation(��ġ)

	// ���� �ӵ� ������Ʈ
	XMVECTOR velocityVec = XMLoadFloat3(&m_xmf3Velocity);
	velocityVec = XMVectorAdd(velocityVec, XMVectorScale(XMLoadFloat3(&m_xmf3Force), deltaTime));

	// ��ġ ������Ʈ
	positionVec = XMVectorAdd(positionVec, XMVectorScale(velocityVec, deltaTime));

	// ���ӵ� ������Ʈ
	XMVECTOR angularVelocityVec = XMLoadFloat3(&m_xmf3AngularVelocity);
	XMVECTOR torqueVec = XMLoadFloat3(&m_xmf3Torque);

	XMVECTOR angularAcceleration = XMVector3Transform(torqueVec, m_xmmInverseInertiaTensor);
	angularVelocityVec = XMVectorAdd(angularVelocityVec, XMVectorScale(angularAcceleration, deltaTime));

	// ���� ����
	angularVelocityVec = XMVectorScale(angularVelocityVec, 0.99f);

	// ȸ�� ������Ʈ (Quaternion�� �ƴ϶� Matrix ���� ����)
	XMMATRIX rotationMatrix = XMMatrixRotationNormal(angularVelocityVec, XMVectorGetX(XMVector3Length(angularVelocityVec)) * deltaTime);
	worldMatrix = XMMatrixMultiply(rotationMatrix, worldMatrix);

	// �� ��ġ ����
	worldMatrix.r[3] = positionVec;

	// ������Ʈ�� Matrix ����
	GetOwner()->SetWorldMatrix(worldMatrix);

	// �� �� ��ũ �ʱ�ȭ (�������� �� ó��)
	m_xmf3Force = { 0, 0, 0 };
	m_xmf3Torque = { 0, 0, 0 };
}

void CRigidBodyComponent::OnCollision(std::shared_ptr<CGameObject> pGameObject)
{
	// �浹 ó��
	auto pCollider = pGameObject->GetComponent<CCollider>();

	if (pCollider)
	{
		auto pTransform = GetOwner()->GetComponent<CTransformComponent>();

		auto pOtherCollider = pGameObject->GetComponent<CCollider>();
		auto pOtherTransform = pGameObject->GetComponent<CTransformComponent>();

		if (pOtherCollider)
		{
			XMFLOAT3 xmf3Position = pTransform->GetPosition();
			XMFLOAT3 xmf3OtherPosition = pOtherTransform->GetPosition();
			XMFLOAT3 xmf3Direction = Vector3::Subtract(xmf3Position, xmf3OtherPosition);
			float fDistance = Vector3::Length(xmf3Direction);
			if (fDistance < 1.0f)
			{
				xmf3Direction = Vector3::Normalize(xmf3Direction);
				xmf3Direction = Vector3::ScalarProduct(xmf3Direction, fDistance * 0.5f);
				pTransform->Move(xmf3Direction);
			}
		}
	}	
}

void CRigidBodyComponent::UpdateVelocity(float fTimeElapsed)
{
	// �߷� ����
	m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, m_xmf3Gravity);

	// �ִ� �ӵ� ���� ========================================
	// ���� �̵� �ӵ� ����
	float fLength = sqrtf(m_xmf3Velocity.x * m_xmf3Velocity.x + m_xmf3Velocity.z * m_xmf3Velocity.z);
	float fMaxVelocityXZ = m_fMaxVelocityXZ;
	if (fLength > m_fMaxVelocityXZ)
	{
		m_xmf3Velocity.x *= (fMaxVelocityXZ / fLength);
		m_xmf3Velocity.z *= (fMaxVelocityXZ / fLength);
	}
	// ���� �̵� �ӵ� ����
	float fMaxVelocityY = m_fMaxVelocityY;
	fLength = sqrtf(m_xmf3Velocity.y * m_xmf3Velocity.y);
	if (fLength > m_fMaxVelocityY) m_xmf3Velocity.y *= (fMaxVelocityY / fLength);

	// �ʴ� �ӵ����� �̵� �Ÿ��� ��ȯ
	XMFLOAT3 xmf3Velocity = Vector3::ScalarProduct(m_xmf3Velocity, fTimeElapsed, false);

	// �̵� �Ÿ���ŭ �̵�
	auto pTransform = GetOwner()->GetComponent<CTransformComponent>();
	pTransform->Move(xmf3Velocity);
}

void CRigidBodyComponent::OnTerrainUpdateCallback(float fTimeElapsed)
{
	CHeightMapTerrain* pTerrain = (CHeightMapTerrain*)m_pTerrainUpdatedContext;
	XMFLOAT3 xmf3Scale = pTerrain->GetScale();
	int xmf4TerrainSize = pTerrain->GetHeightMapWidth();
	int xmf4TerrainLength = pTerrain->GetHeightMapLength();

	auto owner = GetOwner();

	XMFLOAT3 xmf3PlayerPosition = owner->GetPosition();
	int z = (int)(xmf3PlayerPosition.z / xmf3Scale.z);
	bool bReverseQuad = ((z % 2) != 0);
	//float fHeight = pTerrain->GetHeight(xmf3PlayerPosition.x, xmf3PlayerPosition.z, bReverseQuad) + 6.0f;

	{ // x, z �࿡�� Height Map�� ����, ���� �����ǰ� ����.
		if (xmf3PlayerPosition.x < 0.0f) xmf3PlayerPosition.x = 0.0f;
		if (xmf3PlayerPosition.x > (xmf4TerrainSize - 1) * xmf3Scale.x) xmf3PlayerPosition.x = (xmf4TerrainSize - 1) * xmf3Scale.x - 1.0f;
		if (xmf3PlayerPosition.z < 0.0f) xmf3PlayerPosition.z = 0.0f;
		if (xmf3PlayerPosition.z > (xmf4TerrainLength - 1) * xmf3Scale.z) xmf3PlayerPosition.z = (xmf4TerrainLength - 1) * xmf3Scale.z - 1.0f;
	}


	float fHeight = 0.0f;
	// ������� ���� üũ
	auto AABB = owner->GetComponent<CAABBCollider>();
	if (AABB) {
		float modelHeight = AABB->GetBoundingBox().Extents.y / 2;

		fHeight = pTerrain->GetHeight(xmf3PlayerPosition.x, xmf3PlayerPosition.z) + modelHeight;
		if (xmf3PlayerPosition.y < fHeight)
		{
			XMFLOAT3 xmf3PlayerVelocity = GetVelocity();
			xmf3PlayerVelocity.y = 0.0f;
			SetVelocity(xmf3PlayerVelocity);
			xmf3PlayerPosition.y = fHeight;
			owner->SetPosition(xmf3PlayerPosition);
		}
	}
	else {

		fHeight = pTerrain->GetHeight(xmf3PlayerPosition.x, xmf3PlayerPosition.z);
		if (xmf3PlayerPosition.y < fHeight)
		{
			XMFLOAT3 xmf3PlayerVelocity = GetVelocity();
			xmf3PlayerVelocity.y = 0.0f;
			SetVelocity(xmf3PlayerVelocity);
			xmf3PlayerPosition.y = fHeight;
			owner->SetPosition(xmf3PlayerPosition);
		}
	}

	std::string output = "Terrain Height: " + std::to_string(fHeight) + "\n";
	OutputDebugStringA(output.c_str());

}

