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
	std::vector<std::pair<CGameObject::GAMEOBJECT_LAYER, CGameObject::GAMEOBJECT_LAYER>> ppObjectLayerPairs{
	{ CGameObject::LAYER_PLAYER, CGameObject::LAYER_ENEMY,},
	{ CGameObject::LAYER_BULLET, CGameObject::LAYER_ENEMY },
	{ CGameObject::LAYER_PLAYER, CGameObject::LAYER_ENVIRONMENT },
	//{ CGameObject::LAYER_ENEMY, CGameObject::LAYER_ENVIRONMENT },
	{ CGameObject::LAYER_BULLET, CGameObject::LAYER_ENVIRONMENT }
	};
	m_ppObjectLayerPairs = ppObjectLayerPairs;
}


void CCollisionChecker::Update(float fTimeElapsed)
{
	// Collision Check
	CollisonCheckFromLayers(m_ppObjectLayerPairs);
}

void CCollisionChecker::CollisonCheckFromLayers(std::vector<std::pair<CGameObject::GAMEOBJECT_LAYER, CGameObject::GAMEOBJECT_LAYER>>& ppObjectLayerPairs)
{
	auto& ppObjects = m_pScene->GetObjects();	

	std::vector<CollisionInfo> ppCollidedPairs;
	for (auto& ppLayerPair : ppObjectLayerPairs) {
		auto& pObjectsA = ppObjects[ppLayerPair.first];
		auto& pObjectsB = ppObjects[ppLayerPair.second];
		for (auto& pObjectA : pObjectsA) {
			pObjectA->UpdateTransform();
			for (auto& pObjectB : pObjectsB) {
				// 여기서의 Object는 RootObject임을 기억.
				pObjectB->UpdateTransform();

				// 먼저 model Bound AABB로 체크
				auto pMergedA = pObjectA->GetMergedMeshBound();
				auto pMergedB = pObjectB->GetMergedMeshBound();
				if (!pMergedA.Intersects(pMergedB)) continue;

				// 그 이후, Collider 를 가져와 체크
				std::vector<std::shared_ptr<CCollider>> pCollidersA = pObjectA->m_pCachesColliders;
				std::vector<std::shared_ptr<CCollider>> pCollidersB = pObjectB->m_pCachesColliders;

				//pObjectA->GetComponentsInChildren<CCollider>(pCollidersA);
				//pObjectB->GetComponentsInChildren<CCollider>(pCollidersB);

				for (auto& pColliderA : pCollidersA) {
					for (auto& pColliderB : pCollidersB) {		
						if (IsCollided(pColliderA, pColliderB)) {
							ppCollidedPairs.emplace_back(pObjectA, pObjectB, pColliderA, pColliderB);
							//pObjectA->OnCollision(pColliderB);
							//pObjectB->OnCollision(pColliderA);
						}
					}
				}
			}
		}
	}
	for (auto& ppCollisionInfo : ppCollidedPairs)
	{
		// TODO : 이떄 UpdateTransform을 하지 않고, Model이 가진 모든 BB를 가져와서 복사할당후, 변위값만으로 갱신시키는 코드 작성 필요.
		// TODO : 충돌처리시 TransformUpdate를 계속 호출시 많은 부하 발생(실제 좀비렌더링에 렉도 충돌체크때문임을 체크.
		// TODO : 즉, UpdateTransform은 충돌체크전 1번, 렌더링 전 1번 으로 한프레임에 2번만으로 바꾸어야 함.
		//ppCollisionInfo.pObjectA->UpdateTransform();
		//ppCollisionInfo.pObjectB->UpdateTransform();

		ppCollisionInfo.pObjectA->OnCollision(ppCollisionInfo.pObjectB, ppCollisionInfo.pColliderA, ppCollisionInfo.pColliderB);
	}
}

void CCollisionChecker::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
}

bool CCollisionChecker::IsCollided(std::shared_ptr<CCollider>& colliderA, std::shared_ptr<CCollider>& colliderB)
{
	return colliderA->IsCollided(colliderB);
}

bool CCollisionChecker::IsCollided(CCollider& colliderA, CCollider& colliderB)
{
	return colliderA.IsCollided(&colliderB);
}


