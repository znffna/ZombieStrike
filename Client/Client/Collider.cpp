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
}


void CCollider::SetCollider(const BoundingOrientedBox& boundingOrientedBox)
{
	SetCollider(boundingOrientedBox.Center, boundingOrientedBox.Extents, boundingOrientedBox.Orientation);
}

void CCollider::SetCollider(const BoundingBox& boundingOrientedBox)
{
	SetCollider(boundingOrientedBox.Center, boundingOrientedBox.Extents, XMFLOAT4{ 0, 0, 0, 1 });
}

void CCollider::Update(float fTimeElapsed)
{
	/*if (m_pTransform)
	{
		UpdateCollider(m_pTransform->GetWorldMatrix());
	}*/
}

///////////////////////////////////////////////////////////////////////////////
//

void CSphereCollider::SetCollider(std::shared_ptr<CMesh> pMesh)
{
	m_xmBoundingSphere = pMesh->GetBoundingSphere();
}

void CSphereCollider::SetCollider(const XMFLOAT3& xmf3Center, const XMFLOAT3& Extends, const XMFLOAT4& xmf4Orientation)
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
	m_xmBoundingSphere.Transform(m_xmWorldBoundingSphere, XMLoadFloat4x4(&xmf4x4World));
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

XMFLOAT3 CSphereCollider::GetCorrectionVector(std::shared_ptr<CCollider>& pCollider)
{
	return GetCorrectionVector(pCollider.get());
}

XMFLOAT3 CSphereCollider::GetCorrectionVector(CCollider* pCollider)
{
	if (pCollider->GetColliderType() == ColliderType::OBB)
	{
		return CalculateSphere_MTV(
			GetCenter(), GetExtends(),
			pCollider->GetCenter(), pCollider->GetExtends());
	}
	else if (pCollider->GetColliderType() == ColliderType::AABB)
	{
		return CalculateSphere_MTV(
			GetCenter(), GetExtends(),
			pCollider->GetCenter(), pCollider->GetExtends()
		);
	}
	else {
		return CalculateSphere_MTV(
			GetCenter(), GetExtends(),
			pCollider->GetCenter(), pCollider->GetExtends()
		);
	}
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

///////////////////////////////////////////////////////////////////////////////
//

void CAABBCollider::SetCollider(std::shared_ptr<CMesh> pMesh)
{
	m_xmBoundingBox = pMesh->GetBoundingBox();
}

void CAABBCollider::SetCollider(const XMFLOAT3& xmf3Center, const XMFLOAT3& xmf3Extents, const XMFLOAT4 & xmf4Orientation)
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

BoundingBox CAABBCollider::MergeColliders(std::vector<std::shared_ptr<CCollider>>& pColliders)
{ 
	BoundingBox boundingBox{};
	for (auto pCollider : pColliders)
	{
		BoundingBox::CreateMerged(boundingBox, boundingBox, pCollider->GetBoundingBox());
	}
	return boundingBox;
}

XMFLOAT3 CAABBCollider::GetCorrectionVector(std::shared_ptr<CCollider>& pCollider)
{
	return GetCorrectionVector(pCollider.get());
}

XMFLOAT3 CAABBCollider::GetCorrectionVector(CCollider* pCollider)
{
	if (pCollider->GetColliderType() == ColliderType::OBB)
	{
		return CalculateOBB_MTV(GetCenter(), GetExtends(), GetOrientation(),
			pCollider->GetCenter(), pCollider->GetExtends(), pCollider->GetOrientation());
	}
	else if (pCollider->GetColliderType() == ColliderType::AABB)
	{
		return CalculateAABB_MTV(
			GetCenter(), GetExtends(),
			pCollider->GetCenter(), pCollider->GetExtends()
		);
	}
	else {
		return CalculateSphere_MTV(
			GetCenter(), GetExtends(),
			pCollider->GetCenter(), pCollider->GetExtends()
		);
	}
}

///////////////////////////////////////////////////////////////////////////////
//

void COBBCollider::SetCollider(std::shared_ptr<CMesh> pMesh)
{
	m_xmBoundingOrientedBox = pMesh->GetBoundingOrientedBox(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
}

void COBBCollider::SetCollider(const XMFLOAT3& xmf3Center, const XMFLOAT3& xmf3Extents, const XMFLOAT4 & xmf4Orientation)
{
	m_xmBoundingOrientedBox.Center = xmf3Center;
	m_xmBoundingOrientedBox.Extents = xmf3Extents;
	m_xmBoundingOrientedBox.Orientation = xmf4Orientation;
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

XMFLOAT3 COBBCollider::GetCorrectionVector(std::shared_ptr<CCollider>& pCollider)
{
	return GetCorrectionVector(pCollider.get());
}

XMFLOAT3 COBBCollider::GetCorrectionVector(CCollider* pCollider)
{
	if (pCollider->GetColliderType() == ColliderType::OBB)
	{
		return CalculateOBB_MTV(GetCenter(), GetExtends(), GetOrientation(),
			pCollider->GetCenter(), pCollider->GetExtends(), pCollider->GetOrientation());
	}
	else if (pCollider->GetColliderType() == ColliderType::AABB)
	{
		return CalculateAABB_MTV(
			GetCenter(), GetExtends(),
			pCollider->GetCenter(), pCollider->GetExtends()
		);
	}
	else {
		return CalculateSphere_MTV(
			GetCenter(), GetExtends(),
			pCollider->GetCenter(), pCollider->GetExtends()
		);
	}
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

XMFLOAT3 CalculateSphere_MTV(const XMFLOAT3& centerA, const XMFLOAT3& extentA, const XMFLOAT3& centerB, const XMFLOAT3& extentB)
{
	XMVECTOR aCenter = XMLoadFloat3(&centerA);
	XMVECTOR bCenter = XMLoadFloat3(&centerB);
	XMVECTOR d = bCenter - aCenter;

	float lengthA = XMVectorGetX(XMVector3Length(XMLoadFloat3(&extentA)));
	float lengthB = XMVectorGetX(XMVector3Length(XMLoadFloat3(&extentB)));

	float dist = XMVectorGetX(XMVector3Length(d));
	float overlap = (lengthA + lengthB) - dist;

	if (overlap <= 0.0f || dist == 0.0f) {
		// 충돌하지 않거나 완전히 겹침 (위치 동일)
		return XMFLOAT3(0, 0, 0);
	}

	XMVECTOR mtvDir = XMVector3Normalize(d);
	XMVECTOR mtv = mtvDir * overlap;

	XMFLOAT3 result;
	XMStoreFloat3(&result, mtv);
	return result;
}

// 반환값: MTV 벡터 (겹침 없으면 {0, 0, 0})
XMFLOAT3 CalculateAABB_MTV(const XMFLOAT3& centerA, const XMFLOAT3& extentA,
	const XMFLOAT3& centerB, const XMFLOAT3& extentB)
{
	float dx = centerB.x - centerA.x;
	float px = (extentA.x + extentB.x) - std::abs(dx);
	if (px <= 0) return { 0.0f, 0.0f, 0.0f };

	float dy = centerB.y - centerA.y;
	float py = (extentA.y + extentB.y) - std::abs(dy);
	if (py <= 0) return { 0.0f, 0.0f, 0.0f };

	float dz = centerB.z - centerA.z;
	float pz = (extentA.z + extentB.z) - std::abs(dz);
	if (pz <= 0) return { 0.0f, 0.0f, 0.0f };

	// 최소 겹침 축 선택
	/*if (px < pz) {
		return { dx < 0 ? -px : px, 0.0f, 0.0f };
	}
	else {
		return { 0.0f, 0.0f, dz < 0 ? -pz : pz };
	}*/
	if (px < py && px < pz)
		return { dx < 0 ? px : -px, 0.0f, 0.0f };
	else if (py < pz)
		return { 0.0f, dy < 0 ? py : -py, 0.0f };
	else
		return { 0.0f, 0.0f, dz < 0 ? pz : -pz };
}

XMFLOAT3 CalculateOBB_MTV(const XMFLOAT3& centerA, const XMFLOAT3& extentA, const XMFLOAT4& orientationA,
	const XMFLOAT3& centerB, const XMFLOAT3& extentB, const XMFLOAT4& orientationB)
{
	float minOverlap = FLT_MAX;
	XMVECTOR mtvAxis = XMVectorZero();

	XMVECTOR aCenter = XMLoadFloat3(&centerA);
	XMVECTOR bCenter = XMLoadFloat3(&centerB);
	XMVECTOR d = bCenter - aCenter;

	// 쿼터니언 → 축 변환
	XMVECTOR qA = XMLoadFloat4(&orientationA);
	XMVECTOR qB = XMLoadFloat4(&orientationB);

	XMMATRIX rotA = XMMatrixRotationQuaternion(qA);
	XMMATRIX rotB = XMMatrixRotationQuaternion(qB);

	XMVECTOR aAxes[3] = {
		XMVector3Normalize(rotA.r[0]),
		XMVector3Normalize(rotA.r[1]),
		XMVector3Normalize(rotA.r[2])
	};
	XMVECTOR bAxes[3] = {
		XMVector3Normalize(rotB.r[0]),
		XMVector3Normalize(rotB.r[1]),
		XMVector3Normalize(rotB.r[2])
	};

	XMVECTOR axes[15];
	int axisCount = 0;

	// A의 3축
	for (int i = 0; i < 3; ++i) axes[axisCount++] = aAxes[i];
	// B의 3축
	for (int i = 0; i < 3; ++i) axes[axisCount++] = bAxes[i];
	// A x B 교차축 9개 (너무 작은 벡터 제거)
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			XMVECTOR cross = XMVector3Cross(aAxes[i], bAxes[j]);
			if (XMVectorGetX(XMVector3LengthSq(cross)) > EPSILON) { // 너무 작은 벡터 필터링
				axes[axisCount++] = XMVector3Normalize(cross);
			}
		}
	}

	// SAT 검사 시작
	for (int i = 0; i < axisCount; ++i) {
		XMVECTOR axis = axes[i];
		if (XMVector3Equal(axis, XMVectorZero()))
			continue;

		// Projection lengths
		float projA = 0.0f, projB = 0.0f;
		projA += fabsf(extentA.x * XMVectorGetX(XMVector3Dot(axis, aAxes[0])));
		projA += fabsf(extentA.y * XMVectorGetX(XMVector3Dot(axis, aAxes[1])));
		projA += fabsf(extentA.z * XMVectorGetX(XMVector3Dot(axis, aAxes[2])));

		projB += fabsf(extentB.x * XMVectorGetX(XMVector3Dot(axis, bAxes[0])));
		projB += fabsf(extentB.y * XMVectorGetX(XMVector3Dot(axis, bAxes[1])));
		projB += fabsf(extentB.z * XMVectorGetX(XMVector3Dot(axis, bAxes[2])));

		float centerDist = fabsf(XMVectorGetX(XMVector3Dot(axis, d)));
		float overlap = (projA + projB) - centerDist;

		if (overlap <= 0.0f) {
			return XMFLOAT3(0, 0, 0);
		}

		if (overlap < minOverlap) {
			minOverlap = overlap;
			mtvAxis = XMVector3Normalize(axis) * ((XMVectorGetX(XMVector3Dot(axis, d)) < 0.0f) ? 1.0f : -1.0f);
		}
	}

	// MTV 최종 계산 (NaN 방지)
	if (XMVectorGetX(XMVector3LengthSq(mtvAxis)) > EPSILON) {
		XMVECTOR mtv = XMVector3Normalize(mtvAxis) * minOverlap;
		XMFLOAT3 result;
		XMStoreFloat3(&result, mtv);
		return result;
	}

	return XMFLOAT3(0, 0, 0);
}





