#include "Zombie.h"
#include "Scene.h"
#include "GameFramework.h"

CZombieCAnimationController::CZombieCAnimationController(CGameObject* pOwner)
	: CAnimationController(pOwner)
{
}

CZombieCAnimationController::~CZombieCAnimationController()
{
}

///////////////////////////////////////////////////////////////////////////////
//

CZombieObject::CZombieObject()
{
	SetLayer(LAYER_ENEMY);
}

CZombieObject::~CZombieObject()
{
}

void CZombieObject::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nSkinType)
{
	CGameObject::Initialize(pd3dDevice, pd3dCommandList);

	SetName("Zombie_" + std::to_string(GetID()));

	SetRotationAxisLock(true, false, true);

	// <Components>
	auto pRigidBody = CreateComponent<CRigidBody>();
	//pRigidBody->SetGravity(XMFLOAT3(0.0f, -9.0f, 0.0f));

	auto pModel = CResourceManager::Instance().GetModelInfo(m_strModelName[nSkinType]);
	auto pSkinnedAnimationController = CreateComponent<CAnimationController>();
	pSkinnedAnimationController->SetModel(pModel);
	// m_pSkinnedAnimationController = std::make_shared<CAnimationController>();

	// Model Info
	SetSkinType(nSkinType);
	SetSkin(m_nSkinType);
}

void CZombieObject::Initialize()
{
	if (IsInitialized()) return;

	CGameObject::Initialize();

	SetName("Zombie_" + std::to_string(GetID()));

	SetRotationAxisLock(true, false, true);

	// <Components>
	auto pModel = CResourceManager::Instance().GetModelInfo(m_strModelName[m_nSkinType]);
	auto pSkinnedAnimationController = CreateComponent<CAnimationController>();
	pSkinnedAnimationController->SetModel(pModel);
	// m_pSkinnedAnimationController = std::make_shared<CAnimationController>();

	auto pRigidBody = CreateComponent<CRigidBody>();
	pRigidBody->SetGravity(XMFLOAT3(0.0f, 0.0f, 0.0f));
	//pRigidBody->SetGravity(XMFLOAT3(0.0f, -9.0f, 0.0f));

	auto pCollider = CreateComponent<COBBCollider>();
	pCollider->SetBoundingBox(pModel->m_MeshBoundingBox);

	// Model Info
	SetSkin(m_nSkinType);

	m_bInitialized = true;
}

void CZombieObject::Initialize(int nSkinType)
{
	if (IsInitialized()) return;

	CGameObject::Initialize();

	SetName("Zombie_" + std::to_string(GetID()));

	SetRotationAxisLock(true, false, true);

	// <Components>
	m_nSkinType = nSkinType;
	auto pModel = CResourceManager::Instance().GetModelInfo(m_strModelName[m_nSkinType]);
	auto pSkinnedAnimationController = CreateComponent<CAnimationController>();
	pSkinnedAnimationController->SetModel(pModel);
	// m_pSkinnedAnimationController = std::make_shared<CAnimationController>();

	auto pRigidBody = CreateComponent<CRigidBody>();
	pRigidBody->SetGravity(XMFLOAT3(0.0f, 0.0f, 0.0f));
	//pRigidBody->SetGravity(XMFLOAT3(0.0f, -9.0f, 0.0f));

	auto pCollider = CreateComponent<COBBCollider>();
	pCollider->SetBoundingBox(pModel->m_MeshBoundingBox);

	// Model Info
	SetSkin(m_nSkinType);

	m_bInitialized = true;
}


void CZombieObject::Update(float fTimeElapsed)
{
	UpdateAnimation();  

	CGameObject::Update(fTimeElapsed);

	if (!m_bDied) return;

	m_fDeathTime += fTimeElapsed;
	if (m_fDeathTime > m_fMaxDeathTime)
	{
		m_bDied = false;
		m_fDeathTime = 0.f;
		SetActive(false);
	}
}

// Skin State
void CZombieObject::SetSkinType(int nSkinType)
{
	m_nSkinType = nSkinType % m_strModelName.size(); 
}

int CZombieObject::GetSkinType() const { return m_nSkinType; }

void CZombieObject::SetSkin(int nSkinType)
{
	SetSkinType(nSkinType);

	m_pChilds.clear();

	auto pZombieModel = CResourceManager::Instance().GetModelInfo(m_strModelName[m_nSkinType]);
	// SetChild(pZombieModel->m_pModelRootObject);
	if (auto panimationcontroller = GetComponent<CAnimationController>())
		panimationcontroller->SetModel(pZombieModel);

	m_fMaxDeathTime = 3.0f;
	//m_fMaxDeathTime = m_pSkinnedAnimationController->m_pAnimationSets->m_pAnimationSets[(int)CAnimationController::ANIMATION_POSE::ZOMBIE_DEATH]->m_fLength + 3.0f; // 좀비가 죽은 후 사라지기까지의 시간

	//auto pCollider = GetComponent<COBBCollider>();
	//pCollider->SetCollider(FindFrame(m_strMeshBoneName[m_nSkinType])->GetMeshBound());
}

void CZombieObject::UpdateAnimation()
{
	auto pAnim = GetComponent<CAnimationController>();
	if (!pAnim) return;

	// 죽음 최우선
	if (m_bDied || m_nNetHP <= 0 || (ActionType)m_nNetActType == ActionType::DEAD)
	{
		pAnim->SetBasePose((int)ZOMBIE_ANIMATION_POSE::ZOMBIE_DEATH);
		return;
	}

	int newBase = (int)ZOMBIE_ANIMATION_POSE::ZOMBIE_IDLE;

	switch ((ActionType)m_nNetActType)
	{
	case ActionType::ATTACK:
		newBase = (int)ZOMBIE_ANIMATION_POSE::ZOMBIE_ATTACK;
		break;

	case ActionType::HIT:
		newBase = (int)ZOMBIE_ANIMATION_POSE::ZOMBIE_HIT;
		break;

	case ActionType::RANGED:
		newBase = (int)ZOMBIE_ANIMATION_POSE::ZOMBIE_SCREAM;
		break;

	default:
	{
		const float speedXZ = sqrtf(m_xmf3NetVelocity.x * m_xmf3NetVelocity.x +
			m_xmf3NetVelocity.z * m_xmf3NetVelocity.z);

		newBase = (speedXZ > 0.05f)
			? (int)ZOMBIE_ANIMATION_POSE::ZOMBIE_RUNNING
			: (int)ZOMBIE_ANIMATION_POSE::ZOMBIE_IDLE;
	}
	break;
	}

	pAnim->SetBasePose(newBase);
}
