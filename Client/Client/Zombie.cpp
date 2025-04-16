#include "Zombie.h"

CZombieCAnimationController::CZombieCAnimationController(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, int nAnimationTracks, std::shared_ptr<CLoadedModelInfo> pModel)
	: CAnimationController(pd3dDevice, pd3dCommandList, nAnimationTracks, pModel)
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

void CZombieObject::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CLoadedModelInfo> pModel, int nAnimationTracks)
{
	CGameObject::Initialize(pd3dDevice, pd3dCommandList);

	// Object Info
	static UINT nGameObjectID = 0;
	m_bActive = true;
	m_nObjectID = nGameObjectID++;

	m_strName = "Zombie_" + std::to_string(m_nObjectID);

	// Model Info
	std::shared_ptr<CLoadedModelInfo> pAngrybotModel = pModel;
	if (!pAngrybotModel) pAngrybotModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, "Model/FuzZombie.bin", NULL);
	SetChild(pAngrybotModel->m_pModelRootObject);

	m_pSkinnedAnimationController = std::make_shared<CAnimationController>(pd3dDevice, pd3dCommandList, nAnimationTracks, pAngrybotModel);

	m_pSkinnedAnimationController->SetTrackAnimationSet(0, 0);
	m_pSkinnedAnimationController->SetTrackAnimationSet(1, 1);
	m_pSkinnedAnimationController->SetTrackEnable(1, false);

	// Component
	std::shared_ptr<CRigidBody> pRigidBody = AddComponent<CRigidBody>(shared_from_this());
	pRigidBody->SetVelocity(XMFLOAT3(0.0f, -9.0f, 0.0f));

}

std::shared_ptr<CZombieObject> CZombieObject::Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CGameObject> pTerrain, std::shared_ptr<CLoadedModelInfo> pModel, int nAnimationTracks)
{
	std::shared_ptr<CZombieObject> pZombie = std::make_shared<CZombieObject>();
	pZombie->Initialize(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pModel, nAnimationTracks);
	pZombie->GetComponent<CRigidBody>()->SetTerrainUpdatedContext(pTerrain.get());

	return pZombie;
}
