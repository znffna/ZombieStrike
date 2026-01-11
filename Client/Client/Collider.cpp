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

// CCollider
void CCollider::Update(float dt)
{
	if (!IsActive()) return;
	UpdateCollider(GetOwner()->GetComponent<CTransform>()->GetWorldMatrix());
}

///////////////////////////////////////////////////////////////////////////////
//

// CSphereCollider
bool CSphereCollider::Intersects(const CCollider* other) const
{
	switch (other->GetType())
	{
	case ColliderType::Sphere:
		return m_world.Intersects(
			static_cast<const CSphereCollider*>(other)->GetWorldSphere());

	case ColliderType::AABB:
		return static_cast<const CAABBCollider*>(other)
			->GetWorldAABB().Intersects(m_world);

	case ColliderType::OBB:
		return static_cast<const COBBCollider*>(other)
			->GetWorldOBB().Intersects(m_world);
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
//

// CAABBCollider
bool CAABBCollider::Intersects(const CCollider* other) const
{
	switch (other->GetType())
	{
	case ColliderType::Sphere:
		return m_world.Intersects(
			static_cast<const CSphereCollider*>(other)->GetWorldSphere());

	case ColliderType::AABB:
		return m_world.Intersects(
			static_cast<const CAABBCollider*>(other)->GetWorldAABB());

	case ColliderType::OBB:
		// BoundingOrientedBox::Intersects(BoundingBox) 또는 BoundingBox::Intersects(OBB) 중 구현에 맞는 것을 사용.
		// CSphereCollider의 OBB 처리와 대칭되도록 OBB 쪽의 Intersects를 호출합니다.
		return static_cast<const COBBCollider*>(other)
			->GetWorldOBB().Intersects(m_world);
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
//

// COBBCollider
bool COBBCollider::Intersects(const CCollider* other) const
{
	switch (other->GetType())
	{
	case ColliderType::Sphere:
		return m_world.Intersects(
			static_cast<const CSphereCollider*>(other)->GetWorldSphere());

	case ColliderType::AABB:
		// OBB::Intersects(BoundingBox) 가 제공되지 않는 경우에는
		// AABB 쪽의 Intersects(OBB)를 호출하도록 변경해도 됩니다.
		return m_world.Intersects(
			static_cast<const CAABBCollider*>(other)->GetWorldAABB());

	case ColliderType::OBB:
		return m_world.Intersects(
			static_cast<const COBBCollider*>(other)->GetWorldOBB());
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
//

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


