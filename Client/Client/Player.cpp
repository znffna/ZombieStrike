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

	// <Components>
	// Animation Controller
	m_pSkinnedAnimationController = std::make_shared<CAnimationController>();

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
	auto pCollider = CreateComponent<COBBCollider>(shared_from_this());

	// Model Info
	SetSkin(m_nSkinType);

	Update(0.0f);
	UpdateTransform();
}

std::shared_ptr<CPlayer> CPlayer::Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CGameObject> pTerrain, std::shared_ptr<CLoadedModelInfo> pModel, int nAnimationTracks, int nSkinType)
{
	std::shared_ptr<CPlayer> pPlayer = std::make_shared<CPlayer>();
	pPlayer->SetSkinType(nSkinType);
	pPlayer->Initialize(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pModel, nAnimationTracks);

	return pPlayer;
}

void CPlayer::Update(float fTimeElapsed)
{
	CGameObject::Update(fTimeElapsed);

	if(auto pCamera = GetComponent<CCamera>()) pCamera->Update(GetPosition(), fTimeElapsed);
}

void CPlayer::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	CGameObject::Render(pd3dCommandList, pCamera);
}

void CPlayer::SetSkin(int nSkinType)
{
	SetSkinType(nSkinType);

	m_pChilds.clear();

	auto pPlayerModel = CResourceManager::GetInstance().GetModelInfo(m_ModelName[m_nSkinType]);
	SetChild(pPlayerModel->m_pModelRootObject);
	m_pSkinnedAnimationController->SettingByModel(pPlayerModel);

	for (int i = 0; i < m_pSkinnedAnimationController->m_nAnimationTracks; i++)
	{
		m_pSkinnedAnimationController->SetTrackAnimationSet(i, i);
		if(i != 0) m_pSkinnedAnimationController->SetTrackEnable(i, false);
	}

	auto pCollider = GetComponent<COBBCollider>();
	pCollider->SetCollider(FindFrame(m_MeshBoneName[m_nSkinType])->GetMeshBound());

	Update(0.0f);
	UpdateTransform();
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
