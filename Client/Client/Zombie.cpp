#include "Zombie.h"
#include "Scene.h"

CZombieCAnimationController::CZombieCAnimationController()
	: CAnimationController()
{
}

CZombieCAnimationController::~CZombieCAnimationController()
{
}

///////////////////////////////////////////////////////////////////////////////
//

CZombieObject::CZombieObject()
{
}

CZombieObject::~CZombieObject()
{
}

void CZombieObject::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CLoadedModelInfo> pModel, int nSkinType)
{
	CGameObject::Initialize(pd3dDevice, pd3dCommandList);

	// Object Info
	Init();

	m_strName = "Zombie_" + std::to_string(m_nObjectID);

	SetRotationAxisLock(true, false, true);

	// <Components>
	std::shared_ptr<CRigidBody> pRigidBody = CreateComponent<CRigidBody>(shared_from_this());
	pRigidBody->SetGravity(XMFLOAT3(0.0f, -9.0f, 0.0f));

	m_pSkinnedAnimationController = std::make_shared<CAnimationController>();

	

	// Model Info
	SetSkinType(nSkinType);
	SetSkin(m_nSkinType);

	Update(0.0f);
	UpdateTransform();
}

std::shared_ptr<CZombieObject> CZombieObject::Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CGameObject> pTerrain, std::shared_ptr<CLoadedModelInfo> pModel, int nSkinType)
{
	std::shared_ptr<CZombieObject> pZombie = std::make_shared<CZombieObject>();
	pZombie->Initialize(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pModel, nSkinType);
	return pZombie;
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

	auto pZombieModel = CResourceManager::GetInstance().GetModelInfo(m_strModelName[m_nSkinType]);
	SetChild(pZombieModel->m_pModelRootObject);

	m_pSkinnedAnimationController->SettingByModel(pZombieModel);
	for (int i = 0; i < m_pSkinnedAnimationController->m_nAnimationTracks; i++)
	{
		m_pSkinnedAnimationController->SetTrackAnimationSet(i, i);
		if (i != 0) m_pSkinnedAnimationController->SetTrackEnable(i, false);
	}

	//auto pCollider = GetComponent<COBBCollider>();
	//pCollider->SetCollider(FindFrame(m_strMeshBoneName[m_nSkinType])->GetMeshBound());
}
