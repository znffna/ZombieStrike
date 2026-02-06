#include "CollisionChecker.h"
#include "Scene.h"

CCollisionChecker::CCollisionChecker()
	:CGameObject()
{
	SetLayer(LAYER_CONTROLLER);
}

CCollisionChecker::~CCollisionChecker()
{
}

void CCollisionChecker::Initialize()
{
	std::vector<std::pair<GAMEOBJECT_LAYER, GAMEOBJECT_LAYER>> ppObjectLayerPairs{
	{ LAYER_PLAYER, LAYER_ENEMY,},
	{ LAYER_BULLET, LAYER_ENEMY },
	{ LAYER_PLAYER, LAYER_ENVIRONMENT },
	//{ LAYER_ENEMY, LAYER_ENVIRONMENT },
	{ LAYER_BULLET, LAYER_ENVIRONMENT }
	};
	m_ppObjectLayerPairs = ppObjectLayerPairs;
}

void CCollisionChecker::Update(float fTimeElapsed)
{
	// Collision Check
	CollisionCheckFromLayers(m_ppObjectLayerPairs);
}

void CCollisionChecker::CollisionCheckFromLayers(std::vector<std::pair<GAMEOBJECT_LAYER, GAMEOBJECT_LAYER>>& ppObjectLayerPairs)
{
	auto& ppObjects = m_pScene->GetLayerViews();	

	std::vector<CollisionInfo> ppCollidedPairs;
	for (auto& ppLayerPair : ppObjectLayerPairs) {
		auto& pObjectsA = ppObjects[ppLayerPair.first];
		auto& pObjectsB = ppObjects[ppLayerPair.second];
		for (auto pObjectA : pObjectsA) {
			//pObjectA->UpdateTransform();
			for (auto pObjectB : pObjectsB) {
				// 여기서의 Object는 RootObject임을 기억.
				//pObjectB->UpdateTransform();

				// 먼저 model Bound AABB로 체크
				auto pMergedA = pObjectA->GetMergedMeshBound();
				auto pMergedB = pObjectB->GetMergedMeshBound();

				// AABB 충돌이 아니면 패스
				if (false == pMergedA.Intersects(pMergedB)) continue;

				// 그 이후, Collider 를 가져와 체크
				auto& pCollidersA = pObjectA->GetCachedColliders();
				auto& pCollidersB = pObjectB->GetCachedColliders();

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
		ppCollisionInfo.pObjectA->OnCollision(ppCollisionInfo.pObjectB, ppCollisionInfo.pColliderA, ppCollisionInfo.pColliderB);
		ppCollisionInfo.pObjectB->OnCollision(ppCollisionInfo.pObjectA, ppCollisionInfo.pColliderB, ppCollisionInfo.pColliderA);
	}
}

void CCollisionChecker::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite)
{
}

bool CCollisionChecker::IsCollided(CCollider* colliderA, CCollider* colliderB)
{
	return colliderA->Intersects(colliderB);
}

bool CCollisionChecker::IsCollided(CCollider& colliderA, CCollider& colliderB)
{
	return colliderA.Intersects(&colliderB);
}

RESULT_RAYCAST CCollisionChecker::CheckBulletCollision(const XMFLOAT3& xmf3Position, const XMFLOAT3& xmf3Direction, float fRange)
{
	// return 값
	RESULT_RAYCAST resultRaycast;
	bool& isCollided = resultRaycast.isCollided;
	float& fImpactDistance = resultRaycast.fImpactDistance;
	resultRaycast.fImpactDistance = fRange;
	resultRaycast.nHitObjectType = HIT_TYPE_NONE; // 초기값은 충돌 없음

	// 총알 충돌 체크
	auto& ppObjects = m_pScene->GetLayerViews();
	auto& pMaps = ppObjects[LAYER_ENVIRONMENT];
	auto& pEnemies = ppObjects[LAYER_ENEMY];

	XMVECTOR xmv3Position = XMLoadFloat3(&xmf3Position);
	XMVECTOR xmv3Direction = XMLoadFloat3(&xmf3Direction);
	xmv3Direction = XMVector3Normalize(xmv3Direction);

	float tempRange;

	/*
	auto& pTerrain = ppObjects[CGameObject::LAYER_TERRAIN];
	for(auto& pTerrainObject : pTerrain)
	{
		std::vector<std::shared_ptr<CCollider>> pColliders = pTerrainObject->m_pCachesColliders;
		
	}
	*/

	//for (auto& pObject : pMaps)
	//{
	//	auto& pColliders = pObject->GetCachedColliders();
	//	for (auto& pCollider : pColliders) {
	//		if (auto result = pCollider->RayCast(xmv3Position, xmv3Direction, tempRange)) {
	//			isCollided = true;
	//			if (tempRange < fImpactDistance) {
	//				fImpactDistance = tempRange;
	//				resultRaycast.nHitObjectType = HIT_TYPE_ENVIRONMENT; // Environment
	//			}
	//		}
	//	}
	//}

	//for (auto& pEnemy : pEnemies) {
	//	auto& pColliders = pEnemy->GetCachedColliders();
	//	for (auto& pCollider : pColliders) {
	//		if (auto result = pCollider->RayCast(xmv3Position, xmv3Direction, tempRange)) {
	//			isCollided = true;
	//			if (tempRange < fImpactDistance) {
	//				fImpactDistance = tempRange;
	//				resultRaycast.nHitObjectType = HIT_TYPE_ENEMY; // Enemy
	//			}
	//		}
	//	}
	//}
	
	return resultRaycast;
}


