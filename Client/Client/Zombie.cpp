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
	//pCollider->SetBoundingBox(pModel->m_MeshBoundingBox);
	pCollider->SetBoundingBox(pModel->m_pModelRootObject->GetComponent<COBBCollider>()->GetBroadPhaseAABB());

	// Model Info
	SetSkin(m_nSkinType);

	m_bInitialized = true;
}


void CZombieObject::Update(float fTimeElapsed)
{
	// 좀비 사운드 쿨타임 감소
	if (m_fCrySfxCooldown > 0.0f)  m_fCrySfxCooldown -= fTimeElapsed;
	if (m_fBiteSfxCooldown > 0.0f) m_fBiteSfxCooldown -= fTimeElapsed;


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

static constexpr float ZOMBIE_SFX_MIN_DIST = 2.0f;
static constexpr float ZOMBIE_SFX_MAX_DIST = 40.0f;

void CZombieObject::PlayCrySfx()
{
	// 울음소리(3D) 1회 재생
	if (m_fCrySfxCooldown > 0.0f) return;
	m_fCrySfxCooldown = 1.2f; // 너무 도배되지 않게(원하면 조절)

	Sound::SetSoundVolume(0.35f);
	Sound::Play3DSound("Sound/zombie_cry.wav", GetPosition(), ZOMBIE_SFX_MIN_DIST, ZOMBIE_SFX_MAX_DIST);
}

void CZombieObject::PlayBiteSfx()
{
	// 무는소리(3D) 1회 재생
	if (m_fBiteSfxCooldown > 0.0f) return;
	m_fBiteSfxCooldown = 0.4f; // 공격 연타 대비(원하면 조절)

	Sound::SetSoundVolume(0.45f);
	Sound::Play3DSound("Sound/zombie_bite.wav", GetPosition(), ZOMBIE_SFX_MIN_DIST, ZOMBIE_SFX_MAX_DIST);
}