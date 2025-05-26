#pragma once

#include "GameObject.h"

class CScene;

struct CollisionInfo {
	std::shared_ptr<CGameObject> pObjectA;
	std::shared_ptr<CGameObject> pObjectB;
	std::shared_ptr<CCollider> pColliderA;
	std::shared_ptr<CCollider> pColliderB;
};

struct RESULT_RAYCAST {
	bool isCollided;
	float fImpactDistance;
};

class CCollisionChecker : public CGameObject
{
public:
	CCollisionChecker(CScene* pScene);
	virtual ~CCollisionChecker();

	virtual GAMEOBJECT_LAYER GetLayer() { return m_nLayer = LAYER_CONTROLLER; }

	// Object Initialization
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)  override;
	// Object Update
	virtual void Update(float fTimeElapsed) override;
	void CollisionCheckFromLayers(std::vector<std::pair<CGameObject::GAMEOBJECT_LAYER, CGameObject::GAMEOBJECT_LAYER>>& ppObjectLayerPairs);
	// Object Render
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr) override;
	// Object Collision
	virtual bool IsCollided(std::shared_ptr<CCollider>& colliderA, std::shared_ptr<CCollider>& colliderB);
	virtual bool IsCollided(CCollider& colliderA, CCollider& colliderB);

	virtual RESULT_RAYCAST CheckBulletCollision(const XMFLOAT3& xmf3Position, const XMFLOAT3& xmf3Direction, float fRange);

private:
	CScene* m_pScene;

	std::vector<std::pair<CGameObject::GAMEOBJECT_LAYER, CGameObject::GAMEOBJECT_LAYER>> m_ppObjectLayerPairs;
};

