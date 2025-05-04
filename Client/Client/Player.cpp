#include "Player.h"
#include "Scene.h"

CPlayer::CPlayer()
{
}

CPlayer::~CPlayer()
{
}

void CPlayer::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CLoadedModelInfo> pModel, int nAnimationTracks)
{
	CGameObject::Initialize(pd3dDevice, pd3dCommandList);

	// Object Info
	Init();

	m_strName = "Player_" + std::to_string(m_nObjectID);

	SetRotationAxisLock(true, false, true);

	// Model Info
	std::shared_ptr<CLoadedModelInfo> pPlayerModel;
	if (pModel) {
		pPlayerModel = pModel;
		//if (!pPlayerModel) pPlayerModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, "Model/PoliceZombie.bin", NULL);
		SetChild(pPlayerModel->m_pModelRootObject);
	}
	else {
		pPlayerModel = CScene::GetResourceManager().GetModelInfo(m_ModelName[0]);
		SetChild(pPlayerModel->m_pModelRootObject);
		//pPlayerModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, "Model/Ch35_nonPBR.bin", NULL);
		//SetChild(pPlayerModel->m_pModelRootObject);
	}
	
	// Animation Controller
	if(pPlayerModel != nullptr)
	{
		m_pSkinnedAnimationController = std::make_shared<CAnimationController>(pd3dDevice, pd3dCommandList, nAnimationTracks, pPlayerModel);

		m_pSkinnedAnimationController->SetTrackAnimationSet(0, 0);
		m_pSkinnedAnimationController->SetTrackAnimationSet(1, 1);
		m_pSkinnedAnimationController->SetTrackEnable(1, false);
	}

	// RigidBody 持失
	std::shared_ptr<CRigidBody> pRigidBody = CreateComponent<CRigidBody>(shared_from_this());
	pRigidBody->SetVelocity(XMFLOAT3(0.0f, -9.0f, 0.0f));

	// Camera 持失
	auto pCamera = CreateComponent<CThirdPersonCamera>(shared_from_this());
	pCamera->SetViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	pCamera->SetScissorRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	pCamera->SetOffset(XMFLOAT3(0.0f, 0.0f, -5.0f));
	pCamera->GenerateViewMatrix(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	pCamera->GenerateProjectionMatrix(((float)WINDOW_WIDTH / (float)WINDOW_HEIGHT), 60.0f, 1.0f, 1000.0f);
	pCamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	pCamera->SetActive(true);

	// Collider 持失
	auto pCollider = CreateComponent<CAABBCollider>(shared_from_this());
	pCollider->SetCollider(pPlayerModel->m_MeshBoundingBox.Center, pPlayerModel->m_MeshBoundingBox.Extents);

	Update(0.0f);
	UpdateTransform();
}

std::shared_ptr<CPlayer> CPlayer::Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CGameObject> pTerrain, std::shared_ptr<CLoadedModelInfo> pModel, int nAnimationTracks)
{
	std::shared_ptr<CPlayer> pPlayer = std::make_shared<CPlayer>();
	pPlayer->Initialize(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pModel, nAnimationTracks);
	pPlayer->GetComponent<CRigidBody>()->SetTerrainUpdatedContext(pTerrain.get());

	return pPlayer;
}

void CPlayer::Update(float fTimeElapsed)
{
	CGameObject::Update(fTimeElapsed);

	if(auto pCamera = GetComponent<CCamera>()) pCamera->Update(GetPosition(), fTimeElapsed);
}

void CPlayer::Rotate(float x, float y, float z)
{
	if (x != 0.0f)
	{
		m_fPitch += x;
		if (m_fPitch > +89.0f) { x -= (m_fPitch - 89.0f); m_fPitch = +89.0f; }
		if (m_fPitch < -89.0f) { x -= (m_fPitch + 89.0f); m_fPitch = -89.0f; }
	}
	if (y != 0.0f)
	{
		m_fYaw += y;
		if (m_fYaw > 360.0f) m_fYaw -= 360.0f;
		if (m_fYaw < 0.0f) m_fYaw += 360.0f;
	}
	if (z != 0.0f)
	{
		m_fRoll += z;
		if (m_fRoll > +20.0f) { z -= (m_fRoll - 20.0f); m_fRoll = +20.0f; }
		if (m_fRoll < -20.0f) { z -= (m_fRoll + 20.0f); m_fRoll = -20.0f; }
	}

	if (auto pCamera = GetComponent<CCamera>())
	{
		pCamera->Rotate(x, y, z);
	}

	if (m_bPitchLock) x = 0.0f;
	if (m_bYawLock) y = 0.0f;
	if (m_bRollLock) z = 0.0f;

	m_pTransform->Rotate(x, y, z);
}
