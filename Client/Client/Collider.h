///////////////////////////////////////////////////////////////////////////////
// Date: 2025-03-28
// Collider.h : CCollider 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Component.h"

class CCamera;

class CCollider;
class CSphereCollider;
class CAABBCollider;
class COBBCollider;

using DefaultCollider = CAABBCollider; // Alias for easier usage

class CMesh;
class CTransform;

const enum ColliderType { AABB, OBB, SPHERE };

class CCollider : public CComponent
{
public:
	CCollider(CGameObject* pObject) : CComponent(pObject) { }
	virtual ~CCollider() { }

	virtual void Init(CGameObject* pObject);
	virtual std::shared_ptr<CComponent> Clone() const = 0;

	virtual void Update(float fTimeElapsed) override;

	virtual int GetColliderType() = 0;
	virtual XMFLOAT3 GetCenter() = 0;
	virtual XMFLOAT3 GetExtends() = 0;
	virtual XMFLOAT4X4 GetColliderMatrix() = 0;

	XMFLOAT3 GetCorrectionVector(std::shared_ptr<CCollider>& pCollider);

	virtual void SetCollider(std::shared_ptr<CMesh> pMesh) = 0;
	virtual void SetCollider(const XMFLOAT3& xmf3Center, const XMFLOAT3& Extends) = 0;


	virtual void UpdateCollider(const XMFLOAT4X4& xmf4x4World) = 0;
	//virtual void RenderCollider(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera) = 0;

	virtual bool IsCollided(CCollider* pCollider) = 0;
	virtual bool IsCollided(std::shared_ptr<CCollider> pCollider) { return IsCollided(pCollider.get()); };

	std::shared_ptr<const CTransform> m_pTransform;
};

//////////////////////////////////////////////////////////////////////////
//

class CSphereCollider : public CCollider
{
public:
	CSphereCollider(CGameObject* pObject) : CCollider(pObject) {}
	virtual ~CSphereCollider() { }

	virtual std::shared_ptr<CComponent> Clone() const { return std::make_shared<CSphereCollider>(*this); };

	virtual XMFLOAT3 GetCenter() override { return m_xmWorldBoundingSphere.Center; }
	virtual XMFLOAT3 GetExtends() override { return XMFLOAT3{ m_xmWorldBoundingSphere.Radius,m_xmWorldBoundingSphere.Radius,m_xmWorldBoundingSphere.Radius }; };

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

	virtual bool IsCollided(CCollider* pCollider) override;;

	XMFLOAT4X4 GetColliderMatrix() override
	{
		XMFLOAT4X4 xmf4x4box = Matrix4x4::TransformMatrix(
			XMFLOAT3(m_xmWorldBoundingSphere.Radius * 2, m_xmWorldBoundingSphere.Radius * 2, m_xmWorldBoundingSphere.Radius * 2),
			XMFLOAT4(0.0f,0.0f,0.0f,1.0f),
			m_xmWorldBoundingSphere.Center
		);
		return xmf4x4box;
	};

	int GetColliderType() override { return ColliderType::SPHERE; };
	const BoundingSphere GetBoundingSphere() { return m_xmWorldBoundingSphere; }
private:
	BoundingSphere m_xmBoundingSphere;
	BoundingSphere m_xmWorldBoundingSphere;
};

class CAABBCollider : public CCollider
{
public:
	CAABBCollider(CGameObject* pObject) : CCollider(pObject) {}
	virtual ~CAABBCollider() { }

	virtual std::shared_ptr<CComponent> Clone() const { return std::make_shared<CAABBCollider>(*this); };

	virtual XMFLOAT3 GetCenter() override { return m_xmWorldBoundingBox.Center; };
	virtual XMFLOAT3 GetExtends() override { return m_xmWorldBoundingBox.Extents; };

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

	virtual bool IsCollided(CCollider* pCollider) override;;

	XMFLOAT4X4 GetColliderMatrix() override
	{
		XMFLOAT4X4 xmf4x4box = Matrix4x4::TransformMatrix(
			Vector3::ScalarProduct(m_xmWorldBoundingBox.Extents, 2.0f, false),
			XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),
			m_xmWorldBoundingBox.Center
		);
		return xmf4x4box;
	};

	const BoundingBox GetBoundingBox() { return m_xmWorldBoundingBox; }
	int GetColliderType() override { return ColliderType::AABB; }

private:
	BoundingBox m_xmBoundingBox;
	BoundingBox m_xmWorldBoundingBox;
};

class COBBCollider : public CCollider
{
public:
	COBBCollider(CGameObject* pObject) : CCollider(pObject) {}
	virtual ~COBBCollider() { }

	virtual std::shared_ptr<CComponent> Clone() const { return std::make_shared<COBBCollider>(*this); };

	virtual XMFLOAT3 GetCenter() override { return m_xmWorldBoundingOrientedBox.Center; };
	virtual XMFLOAT3 GetExtends() override { return m_xmWorldBoundingOrientedBox.Extents; };

	virtual void SetCollider(std::shared_ptr<CMesh> pMesh) override;
	virtual void SetCollider(const XMFLOAT3& xmf3Center, const XMFLOAT3& xmf3Extents) override
	{
		m_xmBoundingOrientedBox.Center = xmf3Center;
		m_xmBoundingOrientedBox.Extents = xmf3Extents;
	};
	void SetCollider(const BoundingOrientedBox& OBB) { m_xmBoundingOrientedBox = OBB; };

	virtual void UpdateCollider(const XMFLOAT4X4& xmf4x4World) override
	{
		m_xmBoundingOrientedBox.Transform(m_xmWorldBoundingOrientedBox, XMLoadFloat4x4(&xmf4x4World));
	};

	virtual bool IsCollided(CCollider* pCollider) override;;

	XMFLOAT4X4 GetColliderMatrix() override
	{
		XMFLOAT4X4 xmf4x4box = Matrix4x4::TransformMatrix(
			Vector3::ScalarProduct(m_xmWorldBoundingOrientedBox.Extents, 2.0f, false),
			m_xmWorldBoundingOrientedBox.Orientation,
			m_xmWorldBoundingOrientedBox.Center
		);
		return xmf4x4box;
	};

	const BoundingOrientedBox GetBoundingOrientedBox() { return m_xmWorldBoundingOrientedBox; }
	int GetColliderType() override { return ColliderType::OBB; }

private:

	BoundingOrientedBox m_xmBoundingOrientedBox;
	BoundingOrientedBox m_xmWorldBoundingOrientedBox;
};
