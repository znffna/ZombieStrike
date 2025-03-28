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