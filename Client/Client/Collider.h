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

// MTV: Minimum Translation Vector
// 
// Use : Sphere - Sphere, Sphere - AABB, Sphere - OBB
extern XMFLOAT3 CalculateSphere_MTV(const XMFLOAT3& centerA, const XMFLOAT3& extentA, const XMFLOAT3& centerB, const XMFLOAT3& extentB);
// Use : AABB - AABB
extern XMFLOAT3 CalculateAABB_MTV(const XMFLOAT3& centerA, const XMFLOAT3& extentA, const XMFLOAT3& centerB, const XMFLOAT3& extentB);
// Use : OBB - OBB, OBB - AABB
extern XMFLOAT3 CalculateOBB_MTV(const XMFLOAT3& centerA, const XMFLOAT3& extentA, const XMFLOAT4& orientationA, const XMFLOAT3& centerB, const XMFLOAT3& extentB, const XMFLOAT4& orientationB);


enum class ColliderType
{
	Sphere,
	AABB,
	OBB
};

class CCollider : public CComponent
{
public:
	CCollider(CGameObject* owner)
		: CComponent(owner) {}

	virtual ~CCollider() = default;

	// 반드시 구현해야 하는 것들
	virtual ColliderType GetType() const = 0;

	// BroadPhase용 AABB (모든 Collider는 AABB로 변환 가능)
	virtual BoundingBox GetBroadPhaseAABB() const = 0;

	// NarrowPhase
	virtual bool Intersects(const CCollider* other) const = 0;

	// World Transform 반영
	virtual void UpdateCollider(const XMFLOAT4X4& world) = 0;

    // Component
	void Update(float dt) override;
};


//////////////////////////////////////////////////////////////////////////  
//  

class CSphereCollider : public CCollider
{
public:
    CSphereCollider(CGameObject* owner)
        : CCollider(owner) {
    }
	CSphereCollider(const CSphereCollider& rhs) : CCollider(nullptr), m_local(rhs.m_local), m_world(rhs.m_world) { };

    virtual std::unique_ptr<CComponent> Clone(CGameObject* newOwner) const { auto ret = std::make_unique<CSphereCollider>(*this); ret->SetOwnerInternal(newOwner); return (ret); };


    ColliderType GetType() const override
    {
        return ColliderType::Sphere;
    }

    void SetLocalSphere(const BoundingSphere& sphere)
    {
        m_local = sphere;
        m_world = sphere;
    }

    void UpdateCollider(const XMFLOAT4X4& world) override
    {
        m_local.Transform(m_world, XMLoadFloat4x4(&world));
    }

    BoundingBox GetBroadPhaseAABB() const override
    {
        BoundingBox box;
        BoundingBox::CreateFromSphere(box, m_world);
        return box;
    }

    bool Intersects(const CCollider* other) const override;

    const BoundingSphere& GetWorldSphere() const { return m_world; }

private:
    BoundingSphere m_local;
    BoundingSphere m_world;
};

class CAABBCollider : public CCollider
{
public:
    CAABBCollider(CGameObject* owner)
        : CCollider(owner) {
    }
    CAABBCollider(const CAABBCollider& rhs) : CCollider(nullptr), m_local(rhs.m_local), m_world(rhs.m_world) {};

    virtual std::unique_ptr<CComponent> Clone(CGameObject* newOwner) const { auto ret = std::make_unique<CAABBCollider>(*this); ret->SetOwnerInternal(newOwner); return (ret); };

    ColliderType GetType() const override { return ColliderType::AABB; }

    void UpdateCollider(const XMFLOAT4X4& world) override
    {
        m_local.Transform(m_world, XMLoadFloat4x4(&world));
    }

    BoundingBox GetBroadPhaseAABB() const override
    {
        return m_world;
    }

    bool Intersects(const CCollider* other) const override;

    const BoundingBox& GetWorldAABB() const { return m_world; }


private:
    BoundingBox m_local;
    BoundingBox m_world;
};

class COBBCollider : public CCollider
{
public:
    COBBCollider(CGameObject* owner)
        : CCollider(owner) {
    }
    COBBCollider(const COBBCollider& rhs) : CCollider(nullptr), m_local(rhs.m_local), m_world(rhs.m_world) {};

    virtual std::unique_ptr<CComponent> Clone(CGameObject* newOwner) const { auto ret = std::make_unique<COBBCollider>(*this); ret->SetOwnerInternal(newOwner); return (ret); };

    ColliderType GetType() const override { return ColliderType::OBB; }

    void SetCenter(const XMFLOAT3& center)
    {
        m_local.Center = center;
	}

    void SetExtents(const XMFLOAT3& extents)
    {
        m_local.Extents = extents;
    }

    void UpdateCollider(const XMFLOAT4X4& world) override
    {
        m_local.Transform(m_world, XMLoadFloat4x4(&world));
    }

    BoundingBox GetBroadPhaseAABB() const override
    {
        BoundingBox box(m_world.Center, m_world.Extents);
        return box;
    }

    bool Intersects(const CCollider* other) const override;

    const BoundingOrientedBox& GetWorldOBB() const { return m_world; }

private:
    BoundingOrientedBox m_local;
    BoundingOrientedBox m_world;
};
