#pragma once

#include "GameObject.h"

class CScene;

struct CollisionInfo {
	CGameObject* pObjectA;
	CGameObject* pObjectB;
	CCollider* pColliderA;
	CCollider* pColliderB;
};

struct RESULT_RAYCAST {
	bool isCollided;
	float fImpactDistance;
	int nHitObjectType; 
};

class CCollisionChecker : public CGameObject
{
public:
	CCollisionChecker();
	virtual ~CCollisionChecker();

	// Object Initialization
	virtual void Initialize() override;
	
	// Object Update
	virtual void Update(float fTimeElapsed) override;
	void CollisionCheckFromLayers(std::vector<std::pair<GAMEOBJECT_LAYER, GAMEOBJECT_LAYER>>& ppObjectLayerPairs);
	
	// Object Render
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr, bool bDepthWrite = false) override;
	
	// Object Collision
	virtual bool IsCollided(CCollider* colliderA, CCollider* colliderB);
	virtual bool IsCollided(CCollider& colliderA, CCollider& colliderB);

	virtual RESULT_RAYCAST CheckBulletCollision(const XMFLOAT3& xmf3Position, const XMFLOAT3& xmf3Direction, float fRange);

private:
	std::vector<std::pair<GAMEOBJECT_LAYER, GAMEOBJECT_LAYER>> m_ppObjectLayerPairs;
};

