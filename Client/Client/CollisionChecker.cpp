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
	//{ LAYER_PLAYER, LAYER_ENEMY,},
	//{ LAYER_BULLET, LAYER_ENEMY },
	{ LAYER_PLAYER, LAYER_ENVIRONMENT },
	//{ LAYER_ENEMY, LAYER_ENVIRONMENT },
	//{ LAYER_BULLET, LAYER_ENVIRONMENT }
	};
	m_ppObjectLayerPairs = ppObjectLayerPairs;
}

void CCollisionChecker::Update(float fTimeElapsed)
{
	// Collision Check
	for (auto& pair : m_ppObjectLayerPairs)
	{
		CollisionCheckFromLayer(pair.first, pair.second);
	}
	//CollisionCheckFromLayers(m_ppObjectLayerPairs);
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

bool CCollisionChecker::CheckMeshBoundCollision(CGameObject* pObjectA, CGameObject* pObjectB)
{
	// 지형과의 체크는 무조건 통과
	if (pObjectA->GetLayer() == GAMEOBJECT_LAYER::LAYER_TERRAIN || pObjectB->GetLayer() == GAMEOBJECT_LAYER::LAYER_TERRAIN) return true;

	// Root Object에서 트리구조의 모든 MeshBound를 합친 AABB를 가져옴
	auto pMergedA = pObjectA->GetMergedMeshBound();
	auto pMergedB = pObjectB->GetMergedMeshBound();

	// AABB 충돌이 아니면 패스
	if (false == pMergedA.Intersects(pMergedB)) { return false; };
	return true;
}

void CCollisionChecker::CollisionCheckFromLayer(GAMEOBJECT_LAYER first, GAMEOBJECT_LAYER second)
{
	auto& ppObjects = m_pScene->GetLayerViews();
	std::vector<CollisionInfo> ppCollidedPairs;

	auto& pObjectsA = ppObjects[first];
	auto& pObjectsB = ppObjects[second];

	for (auto pObjectA : pObjectsA) {
		for (auto pObjectB : pObjectsB) {
			// 먼저 model Bound AABB로 체크
			//bool ret = CheckMeshBoundCollision(pObjectA, pObjectB);
			//if (ret == false) continue;

			// 그 이후, Collider 를 가져와 체크
			// 이떄 CachedCOllider는 생성시 또는 변화시 한번씩 갱신.
			auto& pCollidersA = pObjectA->GetCachedColliders();
			auto& pCollidersB = pObjectB->GetCachedColliders();

			//pObjectA->GetComponentsInChildren<CCollider>(pCollidersA);
			//pObjectB->GetComponentsInChildren<CCollider>(pCollidersB);

			for (auto& pColliderA : pCollidersA) {
				for (auto& pColliderB : pCollidersB) {
					if (IsCollided(pColliderA, pColliderB)) {
						ppCollidedPairs.emplace_back(pObjectA, pObjectB, pColliderA, pColliderB);
						/*{
							std::string debugoutput = "Collision Detected Between: " + pObjectA->GetName() + " - " + pObjectB->GetName() + "\n";
							OutputDebugStringA(debugoutput.c_str());
						}*/
						//pObjectA->OnCollision(pColliderB);
						//pObjectB->OnCollision(pColliderA);
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
	resultRaycast.isCollided = false;
	resultRaycast.fImpactDistance = fRange;
	resultRaycast.nHitObjectType = HIT_TYPE_NONE; // 초기값은 충돌 없음

	// 총알 충돌 체크
	auto& ppObjects = m_pScene->GetLayerViews();
	auto& pMaps = ppObjects[LAYER_ENVIRONMENT];
	auto& pEnemies = ppObjects[LAYER_ENEMY];

	XMVECTOR xmv3Position = XMLoadFloat3(&xmf3Position);
	XMVECTOR xmv3Direction = XMLoadFloat3(&xmf3Direction);
	xmv3Direction = XMVector3Normalize(xmv3Direction);

	float tempRange = 0.0f; // 초기화 추가

	// Terrain과의 충돌은 미정
	/*auto& pTerrain = ppObjects[GAMEOBJECT_LAYER::LAYER_TERRAIN];
	for(auto& pTerrainObject : pTerrain)
	{
		std::vector<std::shared_ptr<CCollider>> pColliders = pTerrainObject->m_pCachesColliders;
		
	}*/
	

	for (auto& pObject : pMaps)
	{
		auto& pColliders = pObject->GetCachedColliders();
		for (auto& pCollider : pColliders) {
			if (pCollider->RayCast(xmv3Position, xmv3Direction, tempRange)) {
				// 사거리 체크 및 가장 가까운 충돌 지점으로 기록
				if (tempRange <= fRange && tempRange < fImpactDistance) {
					isCollided = true;
					fImpactDistance = tempRange;
					resultRaycast.nHitObjectType = HIT_TYPE_ENVIRONMENT; // Environment
				}
			}
		}
	}

	for (auto& pEnemy : pEnemies) {
		auto& pColliders = pEnemy->GetCachedColliders();
		for (auto& pCollider : pColliders) {
			if (pCollider->RayCast(xmv3Position, xmv3Direction, tempRange)) {
				// 사거리 체크 및 가장 가까운 충돌 지점으로 기록
				if (tempRange <= fRange && tempRange < fImpactDistance) {
					isCollided = true;
					fImpactDistance = tempRange;
					resultRaycast.nHitObjectType = HIT_TYPE_ENEMY; // Enemy
				}
			}
		}
	}
	
	return resultRaycast;
}


