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

void CZombieObject::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, int nSkinType)
{
	CGameObject::Initialize(pd3dDevice, pd3dCommandList);

	SetName("Zombie_" + std::to_string(GetID()));

	SetRotationAxisLock(true, false, true);

	// <Components>
	auto pRigidBody = CreateComponent<CRigidBody>();
	pRigidBody->SetGravity(XMFLOAT3(0.0f, -9.0f, 0.0f));

	auto pSkinnedAnimationController = CreateComponent<CAnimationController>();
	// m_pSkinnedAnimationController = std::make_shared<CAnimationController>();

	auto pmodel = CreateComponent<CModelComponent>();
	pmodel->SetModel(pModel);

	// Model Info
	SetSkinType(nSkinType);
	SetSkin(m_nSkinType);

	Update(0.0f);
	UpdateTransform();
}

std::unique_ptr<CZombieObject> CZombieObject::Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CGameObject> pTerrain, CLoadedModelInfo* pModel, int nSkinType)
{
	auto pZombie = std::unique_ptr<CZombieObject>();
	pZombie->Initialize(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pModel, nSkinType);
	return pZombie;
}

void CZombieObject::Update(float fTimeElapsed)
{
	CGameObject::Update(fTimeElapsed);

	if(m_bDied)
	{
		m_fDeathTime += fTimeElapsed;
		if (m_fDeathTime > m_fMaxDeathTime)
		{
			m_bDied = false; // 좀비가 죽은 상태를 초기화
			m_fDeathTime = 0.0f; // 죽은 시간 초기화

			SetActive(false); // 좀비 오브젝트 비활성화
		}
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
		panimationcontroller->SettingByModel(pZombieModel);

	m_fMaxDeathTime = 3.0f;
	//m_fMaxDeathTime = m_pSkinnedAnimationController->m_pAnimationSets->m_pAnimationSets[(int)CAnimationController::ANIMATION_POSE::ZOMBIE_DEATH]->m_fLength + 3.0f; // 좀비가 죽은 후 사라지기까지의 시간

	//auto pCollider = GetComponent<COBBCollider>();
	//pCollider->SetCollider(FindFrame(m_strMeshBoneName[m_nSkinType])->GetMeshBound());
}
