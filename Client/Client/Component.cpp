///////////////////////////////////////////////////////////////////////////////
// Date: 2025-03-14
// Component.cpp : CComponent 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#include "Component.h"
#include "GameObject.h"
#include "Mesh.h"

CComponent::CComponent()
{
}

CComponent::~CComponent()
{
}

///////////////////////////////////////////////////////////////////////////////
//

void CTransformComponent::SetPosition(float fx, float fy, float fz)
{
	m_xmf3Position = DirectX::XMFLOAT3(fx, fy, fz);

	m_xmf4x4Local._41 = fx;
	m_xmf4x4Local._42 = fy;
	m_xmf4x4Local._43 = fz;

	UpdateTransform(nullptr);
}

void CTransformComponent::SetScale(float fx, float fy, float fz)
{
	m_xmf3Scale = DirectX::XMFLOAT3(fx, fy, fz);

	m_xmf4x4Local._11 = fx;
	m_xmf4x4Local._22 = fy;
	m_xmf4x4Local._33 = fz;

	UpdateTransform(nullptr);
}

void CTransformComponent::Move(DirectX::XMFLOAT3 xmf3Shift)
{
	m_xmf3Position = Vector3::Add(m_xmf3Position, xmf3Shift);

	m_xmf4x4Local._41 = m_xmf3Position.x;
	m_xmf4x4Local._42 = m_xmf3Position.y;
	m_xmf4x4Local._43 = m_xmf3Position.z;

	UpdateTransform(nullptr);
}

void CTransformComponent::MoveStrafe(float fDistance)
{
	DirectX::XMFLOAT3 xmf3Right = GetRight();
	DirectX::XMFLOAT3 xmf3Shift = Vector3::ScalarProduct(xmf3Right, fDistance);
	Move(xmf3Shift);
}

void CTransformComponent::MoveUp(float fDistance)
{
	DirectX::XMFLOAT3 xmf3Up = GetUp();
	DirectX::XMFLOAT3 xmf3Shift = Vector3::ScalarProduct(xmf3Up, fDistance);
	Move(xmf3Shift);
}

void CTransformComponent::MoveForward(float fDistance)
{
	DirectX::XMFLOAT3 xmf3Look = GetLook();
	DirectX::XMFLOAT3 xmf3Shift = Vector3::ScalarProduct(xmf3Look, fDistance);
	Move(xmf3Shift);
}

void CTransformComponent::Rotate(float fPitch, float fYaw, float fRoll)
{
	m_xmf3Rotation.x += fPitch;
	m_xmf3Rotation.y += fYaw;
	m_xmf3Rotation.z += fRoll;

	XMMATRIX xmmtxRotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(fPitch), XMConvertToRadians(fYaw), XMConvertToRadians(fRoll));
	m_xmf4x4Local = Matrix4x4::Multiply(xmmtxRotate, m_xmf4x4Local);

	UpdateTransform(nullptr);
}

void CTransformComponent::Rotate(const XMFLOAT3& pxmf3Axis, float fAngle)
{
	XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&pxmf3Axis), XMConvertToRadians(fAngle));
	m_xmf4x4Local = Matrix4x4::Multiply(xmmtxRotate, m_xmf4x4Local);

	m_xmf3Rotation = ExtractEulerAngles(m_xmf4x4Local, m_xmf3Scale);

	UpdateTransform(nullptr);
}

void CTransformComponent::Rotate(const XMFLOAT4& pxmf4Quaternion)
{
	XMMATRIX xmmtxRotate = XMMatrixRotationQuaternion(XMLoadFloat4(&pxmf4Quaternion));
	m_xmf4x4Local = Matrix4x4::Multiply(xmmtxRotate, m_xmf4x4Local);

	m_xmf3Rotation = ExtractEulerAngles(m_xmf4x4Local, m_xmf3Scale);

	UpdateTransform(nullptr);
}

XMFLOAT3 CTransformComponent::ExtractEulerAngles(const XMFLOAT4X4& worldMatrix, const XMFLOAT3& scale)
{
	// 스케일을 제거한 회전 행렬을 얻기 위해 각 축을 정규화
	XMFLOAT3X3 rotationMatrix;
	rotationMatrix._11 = worldMatrix._11 / scale.x;
	rotationMatrix._12 = worldMatrix._12 / scale.x;
	rotationMatrix._13 = worldMatrix._13 / scale.x;

	rotationMatrix._21 = worldMatrix._21 / scale.y;
	rotationMatrix._22 = worldMatrix._22 / scale.y;
	rotationMatrix._23 = worldMatrix._23 / scale.y;

	rotationMatrix._31 = worldMatrix._31 / scale.z;
	rotationMatrix._32 = worldMatrix._32 / scale.z;
	rotationMatrix._33 = worldMatrix._33 / scale.z;

	// 회전 행렬을 XMVECTOR로 변환
	XMMATRIX rotMat = XMLoadFloat3x3(&rotationMatrix);

	// Euler 각을 추출
	float pitch, yaw, roll;
	yaw = atan2f(rotMat.r[0].m128_f32[2], rotMat.r[2].m128_f32[2]) * RAD_TO_DEG;
	pitch = asinf(-rotMat.r[1].m128_f32[2]) * RAD_TO_DEG;
	roll = atan2f(rotMat.r[1].m128_f32[0], rotMat.r[1].m128_f32[1]) * RAD_TO_DEG;

	return XMFLOAT3(pitch, yaw, roll); // 각도(x,y,z) 값 반환
}

void CTransformComponent::UpdateTransform(const DirectX::XMFLOAT4X4* xmf4x4ParentMatrix)
{
	m_xmf4x4World = (xmf4x4ParentMatrix) ? Matrix4x4::Multiply(m_xmf4x4Local, *xmf4x4ParentMatrix) : m_xmf4x4Local;

#ifdef _WITH_TRANSFORM_HIERARCHY
	for (auto& pChild : m_vecChildTransforms)
	{
		pChild->UpdateTransform(&m_xmf4x4World);
	}
#endif
}

void CTransformComponent::UpdateTransform(const std::shared_ptr<CGameObject> pParentObject)
{
	SetWorldMatrix((pParentObject) ? Matrix4x4::Multiply(m_xmf4x4Local, (pParentObject->GetWorldMatrix())) : m_xmf4x4Local);
#ifdef	_WITH_TRANSFORM_HIERARCHY
	for (auto& pChild : m_vecChildTransforms)
	{
		pChild->UpdateTransform(&m_xmf4x4World);
	}
#endif
}

#ifdef	_WITH_TRANSFORM_HIERARCHY

void CTransformComponent::SetParent(std::shared_ptr<CTransform> pParentTransform)
{
	m_pParentTransform = pParentTransform;
	pParentTransform->SetChild(shared_from_this());
}

void CTransformComponent::SetChild(std::shared_ptr<CTransform> pChildTransform)
{
	m_vecChildTransforms.push_back(pChildTransform);
	pChildTransform->SetParent(shared_from_this());
}

void CTransformComponent::RemoveChild(std::shared_ptr<CTransform> pChildTransform)
{
	auto iter = std::find(m_vecChildTransforms.begin(), m_vecChildTransforms.end(), pChildTransform);
	if (iter != m_vecChildTransforms.end())
	{
		m_vecChildTransforms.erase(iter);
	}
}

#endif


///////////////////////////////////////////////////////////////////////////////
//


void CColliderComponent::Update(float fTimeElapsed)
{
	auto pOwner = GetOwner();
	if (pOwner)
	{
		auto pTransform = pOwner->GetComponent<CTransform>();
		if (pTransform)
		{
			UpdateCollider(pTransform->GetWorldMatrix());
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//

void CSphereCollider::SetCollider(std::shared_ptr<CMesh> pMesh)
{
	m_xmBoundingSphere = pMesh->GetBoundingSphere();
}
bool CSphereCollider::IsCollided(CColliderComponent* pCollider)
{
	switch (pCollider->GetColliderType())
	{
	case ColliderType::Sphere:
	{
		CSphereCollider* pSphereCollider = dynamic_cast<CSphereCollider*>(pCollider);
		return m_xmWorldBoundingSphere.Intersects(pSphereCollider->GetBoundingSphere());
	}
	case ColliderType::AABB:
	{
		CAABBBoxCollider* pAABBBoxCollider = dynamic_cast<CAABBBoxCollider*>(pCollider);
		return m_xmWorldBoundingSphere.Intersects(pAABBBoxCollider->GetBoundingBox());
	}
	case ColliderType::OBB:
	{
		COBBBoxCollider* pOBBBoxCollider = dynamic_cast<COBBBoxCollider*>(pCollider);
		return m_xmWorldBoundingSphere.Intersects(pOBBBoxCollider->GetBoundingOrientedBox());
	}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
//

void CAABBBoxCollider::SetCollider(std::shared_ptr<CMesh> pMesh)
{
	m_xmBoundingBox = pMesh->GetBoundingBox();
}
bool CAABBBoxCollider::IsCollided(CColliderComponent* pCollider)
{
	switch (pCollider->GetColliderType())
	{
	case ColliderType::Sphere:
	{
		CSphereCollider* pSphereCollider = dynamic_cast<CSphereCollider*>(pCollider);
		return m_xmWorldBoundingBox.Intersects(pSphereCollider->GetBoundingSphere());
	}
	case ColliderType::AABB:
	{
		CAABBBoxCollider* pAABBBoxCollider = dynamic_cast<CAABBBoxCollider*>(pCollider);
		return m_xmWorldBoundingBox.Intersects(pAABBBoxCollider->GetBoundingBox());
	}
	case ColliderType::OBB:
	{
		COBBBoxCollider* pOBBBoxCollider = dynamic_cast<COBBBoxCollider*>(pCollider);
		return m_xmWorldBoundingBox.Intersects(pOBBBoxCollider->GetBoundingOrientedBox());
	}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
//

void COBBBoxCollider::SetCollider(std::shared_ptr<CMesh> pMesh)
{
	m_xmBoundingOrientedBox = pMesh->GetBoundingOrientedBox(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
}
bool COBBBoxCollider::IsCollided(CColliderComponent* pCollider)
{
	switch (pCollider->GetColliderType())
	{
	case ColliderType::Sphere:
	{
		CSphereCollider* pSphereCollider = dynamic_cast<CSphereCollider*>(pCollider);
		return m_xmWorldBoundingOrientedBox.Intersects(pSphereCollider->GetBoundingSphere());
	}
	case ColliderType::AABB:
	{
		CAABBBoxCollider* pAABBBoxCollider = dynamic_cast<CAABBBoxCollider*>(pCollider);
		return m_xmWorldBoundingOrientedBox.Intersects(pAABBBoxCollider->GetBoundingBox());
	}
	case ColliderType::OBB:
	{
		COBBBoxCollider* pOBBBoxCollider = dynamic_cast<COBBBoxCollider*>(pCollider);
		return m_xmWorldBoundingOrientedBox.Intersects(pOBBBoxCollider->GetBoundingOrientedBox());
	}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
//

void CRigidBodyComponent::UpdateRigidBody(float fTimeElapsed)
{
	auto owner = GetOwner();

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

	{ // 이동 후처치(속도 감소(정지까지만))
		float fLength = Vector3::Length(m_xmf3Velocity);
		float fDeceleration = (m_fFriction * fTimeElapsed);
		if (fDeceleration > fLength) fDeceleration = fLength;
		m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, Vector3::ScalarProduct(m_xmf3Velocity, -fDeceleration, true));
	}
}

void CRigidBodyComponent::UpdateVelocity(float fTimeElapsed)
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
	auto pTransform = GetOwner()->GetComponent<CTransformComponent>();
	pTransform->Move(xmf3Velocity);
}

void CRigidBodyComponent::OnTerrainUpdateCallback(float fTimeElapsed)
{
	CHeightMapTerrain* pTerrain = (CHeightMapTerrain*)m_pTerrainUpdatedContext;
	XMFLOAT3 xmf3Scale = pTerrain->GetScale();
	int xmf4TerrainSize = pTerrain->GetHeightMapWidth();

	auto owner = GetOwner();

	XMFLOAT3 xmf3PlayerPosition = owner->GetPosition();
	int z = (int)(xmf3PlayerPosition.z / xmf3Scale.z);
	bool bReverseQuad = ((z % 2) != 0);
	//float fHeight = pTerrain->GetHeight(xmf3PlayerPosition.x, xmf3PlayerPosition.z, bReverseQuad) + 6.0f;

	// 지상과의 높이 체크
	auto AABB = owner->GetComponent<CAABBBoxCollider>();
	if (AABB) {
		float modelHeight = AABB->GetBoundingBox().Extents.y / 2;

		float fHeight = pTerrain->GetHeight(xmf3PlayerPosition.x, xmf3PlayerPosition.z) + modelHeight;
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

		float fHeight = pTerrain->GetHeight(xmf3PlayerPosition.x, xmf3PlayerPosition.z);
		if (xmf3PlayerPosition.y < fHeight)
		{
			XMFLOAT3 xmf3PlayerVelocity = GetVelocity();
			xmf3PlayerVelocity.y = 0.0f;
			SetVelocity(xmf3PlayerVelocity);
			xmf3PlayerPosition.y = fHeight;
			owner->SetPosition(xmf3PlayerPosition);
		}
	}
}

