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

	// Initialization
	virtual void Init(CGameObject* pObject);

	// Clone
	virtual std::shared_ptr<CComponent> Clone() const = 0;  

	// getters  
	virtual int GetColliderType() = 0;  
	virtual XMFLOAT3 GetCenter() = 0;  
	virtual XMFLOAT3 GetExtends() = 0;  
	virtual XMFLOAT4X4 GetColliderMatrix() = 0;  
	virtual BoundingBox GetBoundingBox() { return BoundingBox(GetCenter(), GetExtends()); }  

	XMFLOAT3 GetCorrectionVector(std::shared_ptr<CCollider>& pCollider);  

	// setters  
	virtual void SetCollider(std::shared_ptr<CMesh> pMesh) = 0;  
	virtual void SetCollider(const XMFLOAT3& xmf3Center, const XMFLOAT3& Extends) = 0;  
	virtual void SetCollider(BoundingBox boundingBox) = 0;  

	// methods  
	virtual void Update(float fTimeElapsed) override;
	virtual void UpdateCollider(const XMFLOAT4X4& xmf4x4World) = 0;
	virtual bool IsCollided(CCollider* pCollider) = 0;  
	virtual bool IsCollided(std::shared_ptr<CCollider> pCollider) { return IsCollided(pCollider.get()); };  

	// reference members  
	std::shared_ptr<const CTransform> m_pTransform;  
};  

//////////////////////////////////////////////////////////////////////////  
//  

class CSphereCollider : public CCollider  
{  
public:  
	// Constructor & Destructor  
	CSphereCollider(CGameObject* pObject) : CCollider(pObject) {}  
	virtual ~CSphereCollider() { }  

	// Clone  
	virtual std::shared_ptr<CComponent> Clone() const { return std::make_shared<CSphereCollider>(*this); };  

	// Getters  
	virtual XMFLOAT3 GetCenter() override { return m_xmWorldBoundingSphere.Center; }  
	virtual XMFLOAT3 GetExtends() override { return XMFLOAT3{ m_xmWorldBoundingSphere.Radius, m_xmWorldBoundingSphere.Radius, m_xmWorldBoundingSphere.Radius }; };  
	virtual int GetColliderType() override { return ColliderType::SPHERE; };  
	XMFLOAT4X4 GetColliderMatrix() override;;  
	const BoundingSphere GetBoundingVolume() { return m_xmWorldBoundingSphere; }  

	// Setters  
	virtual void SetCollider(std::shared_ptr<CMesh> pMesh) override;  
	virtual void SetCollider(const XMFLOAT3& xmf3Center, const XMFLOAT3& Extends) override;  
	void SetCollider(const XMFLOAT3& xmf3Center, float fRadius);  
	void SetCollider(BoundingBox boundingBox) override;  

	// Methods  
	virtual void UpdateCollider(const XMFLOAT4X4& xmf4x4World) override;;  
	virtual bool IsCollided(CCollider* pCollider) override;;  

private:  
	BoundingSphere m_xmBoundingSphere;  
	BoundingSphere m_xmWorldBoundingSphere;  
};

class CAABBCollider : public CCollider    
{    
public:    
	CAABBCollider(CGameObject* pObject) : CCollider(pObject) {}    
	virtual ~CAABBCollider() {}    

	// Clone    
	virtual std::shared_ptr<CComponent> Clone() const { return std::make_shared<CAABBCollider>(*this); };    

	// Getters    
	virtual XMFLOAT3 GetCenter() override { return m_xmWorldBoundingBox.Center; };  
	virtual XMFLOAT3 GetExtends() override { return m_xmWorldBoundingBox.Extents; };  
	const BoundingBox GetBoundingVolume() { return m_xmWorldBoundingBox; }  
	int GetColliderType() override { return ColliderType::AABB; }    

	// Setters    
	virtual void SetCollider(std::shared_ptr<CMesh> pMesh) override;    
	virtual void SetCollider(const XMFLOAT3& xmf3Center, const XMFLOAT3& xmf3Extents) override;    
	void SetCollider(BoundingBox boundingBox) override;    

	// Methods    
	virtual void UpdateCollider(const XMFLOAT4X4& xmf4x4World) override;    
	virtual bool IsCollided(CCollider* pCollider) override;    
	XMFLOAT4X4 GetColliderMatrix() override;    

private:    
	BoundingBox m_xmBoundingBox;    
	BoundingBox m_xmWorldBoundingBox;  
};  

class COBBCollider : public CCollider    
{    
public:    
	// Special functions    
	COBBCollider(CGameObject* pObject) : CCollider(pObject) {}    
	virtual ~COBBCollider() {}    

	// Clone
	virtual std::shared_ptr<CComponent> Clone() const { return std::make_shared<COBBCollider>(*this); };    

	// Getters    
	virtual XMFLOAT3 GetCenter() override { return m_xmWorldBoundingOrientedBox.Center; };    
	virtual XMFLOAT3 GetExtends() override { return m_xmWorldBoundingOrientedBox.Extents; };    
	XMFLOAT4X4 GetColliderMatrix() override;    
	const BoundingOrientedBox GetBoundingVolume() { return m_xmWorldBoundingOrientedBox; }    
	int GetColliderType() override { return ColliderType::OBB; }    

	// Setters    
	virtual void SetCollider(std::shared_ptr<CMesh> pMesh) override;    
	virtual void SetCollider(const XMFLOAT3& xmf3Center, const XMFLOAT3& xmf3Extents) override;    
	void SetCollider(const BoundingOrientedBox& OBB) { m_xmBoundingOrientedBox = OBB; };    
	void SetCollider(BoundingBox boundingBox) override;    

	// Methods    
	virtual void UpdateCollider(const XMFLOAT4X4& xmf4x4World) override;    
	virtual bool IsCollided(CCollider* pCollider) override;    

private:    
	BoundingOrientedBox m_xmBoundingOrientedBox;    
	BoundingOrientedBox m_xmWorldBoundingOrientedBox;    
};
