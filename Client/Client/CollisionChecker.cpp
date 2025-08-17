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
	CollisionCheckFromLayers(m_ppObjectLayerPairs);
}

void CCollisionChecker::CollisionCheckFromLayers(std::vector<std::pair<CGameObject::GAMEOBJECT_LAYER, CGameObject::GAMEOBJECT_LAYER>>& ppObjectLayerPairs)
{
	auto& ppObjects = m_pScene->GetObjects();	

	std::vector<CollisionInfo> ppCollidedPairs;
	for (auto& ppLayerPair : ppObjectLayerPairs) {
		auto& pObjectsA = ppObjects[ppLayerPair.first];
		auto& pObjectsB = ppObjects[ppLayerPair.second];
		for (auto& pObjectA : pObjectsA) {
			//pObjectA->UpdateTransform();
			for (auto& pObjectB : pObjectsB) {
				// 여기서의 Object는 RootObject임을 기억.
				//pObjectB->UpdateTransform();

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
		ppCollisionInfo.pObjectA->OnCollision(ppCollisionInfo.pObjectB, ppCollisionInfo.pColliderA, ppCollisionInfo.pColliderB);
	}
}

void CCollisionChecker::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite)
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

RESULT_RAYCAST CCollisionChecker::CheckBulletCollision(const XMFLOAT3& xmf3Position, const XMFLOAT3& xmf3Direction, float fRange)
{
	// return 값
	RESULT_RAYCAST resultRaycast;
	bool& isCollided = resultRaycast.isCollided;
	float& fImpactDistance = resultRaycast.fImpactDistance;
	resultRaycast.fImpactDistance = fRange;

	// 총알 충돌 체크
	auto& ppObjects = m_pScene->GetObjects();
	auto& pMaps = ppObjects[CGameObject::LAYER_ENVIRONMENT];
	auto& pEnemies = ppObjects[CGameObject::LAYER_ENEMY];

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

	for (auto& pObject : pMaps)
	{
		std::vector<std::shared_ptr<CCollider>> pColliders = pObject->m_pCachesColliders;
		for (auto& pCollider : pColliders) {
			if (auto result = pCollider->RayCast(xmv3Position, xmv3Direction, tempRange)) {
				isCollided = true;
				if (tempRange < fImpactDistance) {
					fImpactDistance = tempRange;
					resultRaycast.nHitObjectType = 1; // Environment
				}
			}
		}
	}

	for (auto& pEnemy : pEnemies) {
		std::vector<std::shared_ptr<CCollider>> pColliders = pEnemy->m_pCachesColliders;
		for (auto& pCollider : pColliders) {
			if (auto result = pCollider->RayCast(xmv3Position, xmv3Direction, tempRange)) {
				isCollided = true;
				if (tempRange < fImpactDistance) {
					fImpactDistance = tempRange;
					resultRaycast.nHitObjectType = 2; // Enemy
				}
			}
		}
	}
	
	return resultRaycast;
}


