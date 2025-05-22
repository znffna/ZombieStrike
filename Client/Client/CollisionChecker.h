#pragma once

#include "GameObject.h"

class CScene;

struct CollisionInfo {
	std::shared_ptr<CGameObject> pObjectA;
	std::shared_ptr<CGameObject> pObjectB;
	std::shared_ptr<CCollider> pColliderA;
	std::shared_ptr<CCollider> pColliderB;
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
	void CollisonCheckFromLayers(std::vector<std::pair<CGameObject::GAMEOBJECT_LAYER, CGameObject::GAMEOBJECT_LAYER>>& ppObjectLayerPairs);
	// Object Render
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr) override;
	// Object Collision
	virtual bool IsCollided(std::shared_ptr<CCollider>& colliderA, std::shared_ptr<CCollider>& colliderB);
	virtual bool IsCollided(CCollider& colliderA, CCollider& colliderB);

private:
	CScene* m_pScene;

	std::vector<std::pair<CGameObject::GAMEOBJECT_LAYER, CGameObject::GAMEOBJECT_LAYER>> m_ppObjectLayerPairs;
};

