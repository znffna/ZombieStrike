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

void CSphereCollider::SetCollider(const XMFLOAT3& xmf3Center, const XMFLOAT3& Extends)
{
	m_xmBoundingSphere.Center = xmf3Center;
	m_xmBoundingSphere.Radius = Vector3::Length(Extends);
}

void CSphereCollider::SetCollider(const XMFLOAT3& xmf3Center, float fRadius)
{
	m_xmBoundingSphere.Center = xmf3Center;
	m_xmBoundingSphere.Radius = fRadius;
}

void CSphereCollider::UpdateCollider(const XMFLOAT4X4& xmf4x4World)
{
	m_xmWorldBoundingSphere.Center = Vector3::TransformCoord(m_xmBoundingSphere.Center, xmf4x4World);
	m_xmWorldBoundingSphere.Radius = m_xmBoundingSphere.Radius;

}
bool CSphereCollider::IsCollided(CCollider* pCollider)
{
	switch (pCollider->GetColliderType())
	{
	case ColliderType::SPHERE:
	{
		CSphereCollider* pSphereCollider = dynamic_cast<CSphereCollider*>(pCollider);
		return m_xmWorldBoundingSphere.Intersects(pSphereCollider->GetBoundingVolume());
	}
	case ColliderType::AABB:
	{
		CAABBCollider* pAABBBoxCollider = dynamic_cast<CAABBCollider*>(pCollider);
		return m_xmWorldBoundingSphere.Intersects(pAABBBoxCollider->GetBoundingVolume());
	}
	case ColliderType::OBB:
	{
		COBBCollider* pOBBBoxCollider = dynamic_cast<COBBCollider*>(pCollider);
		return m_xmWorldBoundingSphere.Intersects(pOBBBoxCollider->GetBoundingVolume());
	}
	}
	return false;
}

XMFLOAT4X4 CSphereCollider::GetColliderMatrix()
{
	XMFLOAT4X4 xmf4x4box = Matrix4x4::TransformMatrix(
		XMFLOAT3(m_xmWorldBoundingSphere.Radius * 2, m_xmWorldBoundingSphere.Radius * 2, m_xmWorldBoundingSphere.Radius * 2),
		XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),
		m_xmWorldBoundingSphere.Center
	);
	return xmf4x4box;
}

void CSphereCollider::SetCollider(BoundingBox boundingBox)
{
	SetCollider(boundingBox.Center, boundingBox.Extents);
}

///////////////////////////////////////////////////////////////////////////////
//

void CAABBCollider::SetCollider(std::shared_ptr<CMesh> pMesh)
{
	m_xmBoundingBox = pMesh->GetBoundingBox();
}

void CAABBCollider::SetCollider(const XMFLOAT3& xmf3Center, const XMFLOAT3& xmf3Extents)
{
	m_xmBoundingBox.Center = xmf3Center;
	m_xmBoundingBox.Extents = xmf3Extents;
}

void CAABBCollider::UpdateCollider(const XMFLOAT4X4& xmf4x4World)
{
	m_xmBoundingBox.Transform(m_xmWorldBoundingBox, XMLoadFloat4x4(&xmf4x4World));
}
bool CAABBCollider::IsCollided(CCollider* pCollider)
{
	switch (pCollider->GetColliderType())
	{
	case ColliderType::SPHERE:
	{
		CSphereCollider* pSphereCollider = dynamic_cast<CSphereCollider*>(pCollider);
		return m_xmWorldBoundingBox.Intersects(pSphereCollider->GetBoundingVolume());
	}
	case ColliderType::AABB:
	{
		CAABBCollider* pAABBBoxCollider = dynamic_cast<CAABBCollider*>(pCollider);
		return m_xmWorldBoundingBox.Intersects(pAABBBoxCollider->GetBoundingVolume());
	}
	case ColliderType::OBB:
	{
		COBBCollider* pOBBBoxCollider = dynamic_cast<COBBCollider*>(pCollider);
		return m_xmWorldBoundingBox.Intersects(pOBBBoxCollider->GetBoundingVolume());
	}
	}
	return false;
}

XMFLOAT4X4 CAABBCollider::GetColliderMatrix()
{
	XMFLOAT4X4 xmf4x4box = Matrix4x4::TransformMatrix(
		Vector3::ScalarProduct(m_xmWorldBoundingBox.Extents, 2.0f, false),
		XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),
		m_xmWorldBoundingBox.Center
	);
	return xmf4x4box;
}

void CAABBCollider::SetCollider(BoundingBox boundingBox)
{
	SetCollider(boundingBox.Center, boundingBox.Extents);
}

///////////////////////////////////////////////////////////////////////////////
//

void COBBCollider::SetCollider(std::shared_ptr<CMesh> pMesh)
{
	m_xmBoundingOrientedBox = pMesh->GetBoundingOrientedBox(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
}

void COBBCollider::SetCollider(const XMFLOAT3& xmf3Center, const XMFLOAT3& xmf3Extents)
{
	m_xmBoundingOrientedBox.Center = xmf3Center;
	m_xmBoundingOrientedBox.Extents = xmf3Extents;
}

void COBBCollider::UpdateCollider(const XMFLOAT4X4& xmf4x4World)
{
	m_xmBoundingOrientedBox.Transform(m_xmWorldBoundingOrientedBox, XMLoadFloat4x4(&xmf4x4World));
}

bool COBBCollider::IsCollided(CCollider* pCollider)
{
	switch (pCollider->GetColliderType())
	{
	case ColliderType::SPHERE:
	{
		CSphereCollider* pSphereCollider = dynamic_cast<CSphereCollider*>(pCollider);
		return m_xmWorldBoundingOrientedBox.Intersects(pSphereCollider->GetBoundingVolume());
	}
	case ColliderType::AABB:
	{
		CAABBCollider* pAABBBoxCollider = dynamic_cast<CAABBCollider*>(pCollider);
		return m_xmWorldBoundingOrientedBox.Intersects(pAABBBoxCollider->GetBoundingVolume());
	}
	case ColliderType::OBB:
	{
		COBBCollider* pOBBBoxCollider = dynamic_cast<COBBCollider*>(pCollider);
		return m_xmWorldBoundingOrientedBox.Intersects(pOBBBoxCollider->GetBoundingVolume());
	}
	}
	return false;
}

XMFLOAT4X4 COBBCollider::GetColliderMatrix()
{
	XMFLOAT4X4 xmf4x4box = Matrix4x4::TransformMatrix(
		Vector3::ScalarProduct(m_xmWorldBoundingOrientedBox.Extents, 2.0f, false),
		m_xmWorldBoundingOrientedBox.Orientation,
		m_xmWorldBoundingOrientedBox.Center
	);
	return xmf4x4box;
}

void COBBCollider::SetCollider(BoundingBox boundingBox)
{
	SetCollider(boundingBox.Center, boundingBox.Extents);
}

// 반환값: MTV 벡터 (겹침 없으면 {0, 0, 0})
XMFLOAT3 GetAABB_MTV(const XMFLOAT3& centerA, const XMFLOAT3& extentA,
	const XMFLOAT3& centerB, const XMFLOAT3& extentB)
{
	float dx = centerB.x - centerA.x;
	float px = (extentA.x + extentB.x) - std::abs(dx);
	if (px <= 0) return { 0.0f, 0.0f, 0.0f };

	/*float dy = centerB.y - centerA.y;
	float py = (extentA.y + extentB.y) - std::abs(dy);
	if (py <= 0) return { 0.0f, 0.0f, 0.0f };*/

	float dz = centerB.z - centerA.z;
	float pz = (extentA.z + extentB.z) - std::abs(dz);
	if (pz <= 0) return { 0.0f, 0.0f, 0.0f };

	// 최소 겹침 축 선택
	if (px < pz) {
		return { dx < 0 ? -px : px, 0.0f, 0.0f };
	}
	else {
		return { 0.0f, 0.0f, dz < 0 ? -pz : pz };
	}
	/*if (px < py && px < pz)
		return { dx < 0 ? -px : px, 0.0f, 0.0f };
	else if (py < pz)
		return { 0.0f, dy < 0 ? -py : py, 0.0f };
	else
		return { 0.0f, 0.0f, dz < 0 ? -pz : pz };*/
}

XMFLOAT3 CCollider::GetCorrectionVector(std::shared_ptr<CCollider>& pCollider)
{
	XMFLOAT3 xmf3Center = GetCenter();
	XMFLOAT3 xmf3OtherCenter = pCollider->GetCenter();

	// 일단 거리기반으로 통일
	return GetAABB_MTV(
		xmf3Center, GetExtends(),
		xmf3OtherCenter, pCollider->GetExtends()
	);
}

