///////////////////////////////////////////////////////////////////////////////
// Date: 2025-03-28
// Collider.cpp : CCollider 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#include "Collider.h"
#include "Transform.h"
#include "GameObject.h"
#include "Mesh.h"

///////////////////////////////////////////////////////////////////////////////
//

void CCollider::Init(CGameObject* pObject)
{
	m_pTransform = pObject->GetComponent<CTransform>();
}


void CCollider::Update(float fTimeElapsed)
{
	if (m_pTransform)
	{
		UpdateCollider(m_pTransform->GetWorldMatrix());
	}
}

///////////////////////////////////////////////////////////////////////////////
//

void CSphereCollider::SetCollider(std::shared_ptr<CMesh> pMesh)
{
	m_xmBoundingSphere = pMesh->GetBoundingSphere();
}
bool CSphereCollider::IsCollided(CCollider* pCollider)
{
	switch (pCollider->GetColliderType())
	{
	case ColliderType::SPHERE:
	{
		CSphereCollider* pSphereCollider = dynamic_cast<CSphereCollider*>(pCollider);
		return m_xmWorldBoundingSphere.Intersects(pSphereCollider->GetBoundingSphere());
	}
	case ColliderType::AABB:
	{
		CAABBCollider* pAABBBoxCollider = dynamic_cast<CAABBCollider*>(pCollider);
		return m_xmWorldBoundingSphere.Intersects(pAABBBoxCollider->GetBoundingBox());
	}
	case ColliderType::OBB:
	{
		COBBCollider* pOBBBoxCollider = dynamic_cast<COBBCollider*>(pCollider);
		return m_xmWorldBoundingSphere.Intersects(pOBBBoxCollider->GetBoundingOrientedBox());
	}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
//

void CAABBCollider::SetCollider(std::shared_ptr<CMesh> pMesh)
{
	m_xmBoundingBox = pMesh->GetBoundingBox();
}
bool CAABBCollider::IsCollided(CCollider* pCollider)
{
	switch (pCollider->GetColliderType())
	{
	case ColliderType::SPHERE:
	{
		CSphereCollider* pSphereCollider = dynamic_cast<CSphereCollider*>(pCollider);
		return m_xmWorldBoundingBox.Intersects(pSphereCollider->GetBoundingSphere());
	}
	case ColliderType::AABB:
	{
		CAABBCollider* pAABBBoxCollider = dynamic_cast<CAABBCollider*>(pCollider);
		return m_xmWorldBoundingBox.Intersects(pAABBBoxCollider->GetBoundingBox());
	}
	case ColliderType::OBB:
	{
		COBBCollider* pOBBBoxCollider = dynamic_cast<COBBCollider*>(pCollider);
		return m_xmWorldBoundingBox.Intersects(pOBBBoxCollider->GetBoundingOrientedBox());
	}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
//

void COBBCollider::SetCollider(std::shared_ptr<CMesh> pMesh)
{
	m_xmBoundingOrientedBox = pMesh->GetBoundingOrientedBox(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
}
bool COBBCollider::IsCollided(CCollider* pCollider)
{
	switch (pCollider->GetColliderType())
	{
	case ColliderType::SPHERE:
	{
		CSphereCollider* pSphereCollider = dynamic_cast<CSphereCollider*>(pCollider);
		return m_xmWorldBoundingOrientedBox.Intersects(pSphereCollider->GetBoundingSphere());
	}
	case ColliderType::AABB:
	{
		CAABBCollider* pAABBBoxCollider = dynamic_cast<CAABBCollider*>(pCollider);
		return m_xmWorldBoundingOrientedBox.Intersects(pAABBBoxCollider->GetBoundingBox());
	}
	case ColliderType::OBB:
	{
		COBBCollider* pOBBBoxCollider = dynamic_cast<COBBCollider*>(pCollider);
		return m_xmWorldBoundingOrientedBox.Intersects(pOBBBoxCollider->GetBoundingOrientedBox());
	}
	}
	return false;
}

XMFLOAT3 CCollider::GetCorrectionVector(std::shared_ptr<CCollider>& pCollider)
{
	XMFLOAT3 xmf3Center = GetCenter();
	XMFLOAT3 xmf3OtherCenter = pCollider->GetCenter();

	// 일단 거리기반으로 통일
	XMFLOAT3 xmf3CorrectionVector = Vector3::Subtract(xmf3Center, xmf3OtherCenter);
	Vector3::Normalize(xmf3CorrectionVector);

	return xmf3CorrectionVector;
}
