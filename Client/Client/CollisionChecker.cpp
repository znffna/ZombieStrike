#include "CollisionChecker.h"
#include "Scene.h"

CCollisionChecker::CCollisionChecker(CScene* pScene)
	:CGameObject(), m_pScene(pScene)
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
				// 여기서의 Object는 RootObject임을 기억.

				// 먼저 model Bound AABB로 체크
				auto pMergedA = pObjectA->GetMergedMeshBound();
				auto pMergedB = pObjectB->GetMergedMeshBound();
				if (!pMergedA.Intersects(pMergedB)) continue;

				// 그 이후, Collider 를 가져와 체크
				std::vector<std::shared_ptr<CCollider>> pCollidersA;
				std::vector<std::shared_ptr<CCollider>> pCollidersB;

				pObjectA->GetComponentsInChildren<CCollider>(pCollidersA);
				pObjectB->GetComponentsInChildren<CCollider>(pCollidersB);
				// TODO : 이떄 IsCollided를 CollisionChecker의 멤버함수로 작성
				// TODO : 이때 로직 수행을 바로 하지 않고 따로 pair를 저장한 이후 batch 처리 생각할 것.
				for (auto& pColliderA : pCollidersA) {
					for (auto& pColliderB : pCollidersB) {
						if (IsCollided(pColliderA, pColliderB)) {
							pObjectA->OnCollision(pColliderB);
							pObjectB->OnCollision(pColliderA);
						}
					}
				}
			}
		}
	}
}

void CCollisionChecker::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
}

bool CCollisionChecker::IsCollided(std::shared_ptr<CCollider>& colliderA, std::shared_ptr<CCollider>& colliderB)
{
	bool result = colliderA->IsCollided(colliderB);
	if(result)
	{
		std::string DebugOutput = "Collision Detected: " + colliderA->gameObject->GetName() + " <-> " + colliderB->gameObject->GetName() + "\n";
		OutputDebugStringA(DebugOutput.c_str());
	}
	return result;
}


