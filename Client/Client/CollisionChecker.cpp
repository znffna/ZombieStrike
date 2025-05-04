#include "CollisionChecker.h"
#include "Scene.h"

CCollisionChecker::CCollisionChecker(std::shared_ptr<CScene>& pScene)
	: CGameObject(), m_pScene(pScene.get())
{
}

CCollisionChecker::~CCollisionChecker()
{
}

void CCollisionChecker::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	std::vector<std::pair<CGameObject::Layer, CGameObject::Layer>> ppObjectLayerPairs{
	{ CGameObject::LAYER_PLAYER, CGameObject::LAYER_ENEMY,},
	{ CGameObject::LAYER_BULLET, CGameObject::LAYER_ENEMY },
	{ CGameObject::LAYER_PLAYER, CGameObject::LAYER_DEFUALT },
	{ CGameObject::LAYER_ENEMY, CGameObject::LAYER_DEFUALT },
	{ CGameObject::LAYER_BULLET, CGameObject::LAYER_DEFUALT }
	};
	m_ppObjectLayerPairs = ppObjectLayerPairs;
}


void CCollisionChecker::Update(float fTimeElapsed)
{
	// Collision Check
	CollisonCheckFromLayers(m_ppObjectLayerPairs);
}

void CCollisionChecker::CollisonCheckFromLayers(std::vector<std::pair<CGameObject::Layer, CGameObject::Layer>>& ppObjectLayerPairs)
{
	auto& ppObjects = m_pScene->GetObjects();
	for (auto& ppLayerPair : ppObjectLayerPairs) {
		auto& pObjectsA = ppObjects[ppLayerPair.first];
		auto& pObjectsB = ppObjects[ppLayerPair.second];
		for (auto& pObjectA : pObjectsA) {
			for (auto& pObjectB : pObjectsB) {
				auto pCollidersA = pObjectA->GetComponents<CCollider>();
				auto pCollidersB = pObjectB->GetComponents<CCollider>();
				// TODO : 이떄 IsCollided를 CollisionChecker의 멤버함수로 작성
				// TODO : 이때 로직 수행을 바로 하지 않고 따로 pair를 저장한 이후 batch 처리 생각할 것.
				/*
				if (pObjectA->IsCollided(pObjectB)) {
					pObjectA->OnCollision(pObjectB);
					pObjectB->OnCollision(pObjectA);
				}
				*/
			}
		}
	}
}

void CCollisionChecker::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
}

bool CCollisionChecker::IsCollided(std::shared_ptr<CGameObject>& pGameObject, UINT nDepth)
{
	return false;
}
