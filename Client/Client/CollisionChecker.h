#pragma once

#include "GameObject.h"

class CScene;

class CCollisionChecker : public CGameObject
{
public:
	CCollisionChecker(CScene* pScene);
	virtual ~CCollisionChecker();

	virtual Layer GetLayer() { return LAYER_CONTROLLER; }

	// Object Initialization
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)  override;
	// Object Update
	virtual void Update(float fTimeElapsed) override;
	void CollisonCheckFromLayers(std::vector<std::pair<CGameObject::Layer, CGameObject::Layer>>& ppObjectLayerPairs);
	// Object Render
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr) override;
	// Object Collision
	virtual bool IsCollided(std::shared_ptr<CCollider>& colliderA, std::shared_ptr<CCollider>& colliderB);

private:
	CScene* m_pScene;

	std::vector<std::pair<CGameObject::Layer, CGameObject::Layer>> m_ppObjectLayerPairs;
};

