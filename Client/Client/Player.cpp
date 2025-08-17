#include "Player.h"
#include "Scene.h"

#include "Gun.h"
#include "GaugeBar.h"

///////////////////////////////////////////////////////////////////////////////////
//

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

	// RigidBody 생성
	std::shared_ptr<CRigidBody> pRigidBody = CreateComponent<CRigidBody>(shared_from_this());
	pRigidBody->SetVelocity(XMFLOAT3(0.0f, -9.0f, 0.0f));

	// Camera 생성
	auto pCamera = CreateComponent<CThirdPersonCamera>(shared_from_this());
	pCamera->SetViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	pCamera->SetScissorRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	pCamera->SetOffset(XMFLOAT3(1.0f, 0.7f, -2.5f));
	pCamera->GenerateViewMatrix(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	pCamera->GenerateProjectionMatrix(((float)WINDOW_WIDTH / (float)WINDOW_HEIGHT), 60.0f, 1.0f, 1000.0f);
	pCamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	pCamera->SetActive(true);

	// Collider 생성
	//auto pCollider = CreateComponent<COBBCollider>(shared_from_this());

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
	if (m_pSkinnedAnimationController) UpdateLowerAnimation();

	CGameObject::Update(fTimeElapsed);

	if(m_bReload)
	{
		m_fReloadTime += fTimeElapsed;
		if (m_fReloadTime >= 1.0f) {
			if (m_pGun) m_pGun->Reload();
			m_fReloadTime = 0.0f;
			SetState(CAnimationController::ANIMATION_STATE::IDLE);
		}
	}

	//Move(m_nMoveInput, m_fMoveSpeed, fTimeElapsed);

	if(auto pCamera = GetComponent<CCamera>()) pCamera->Update(GetPosition(), fTimeElapsed);
	//if(auto pCamera = GetComponent<CCamera>()) pCamera->Update(Vector3::Add( GetPosition(), XMFLOAT3(0, m_fCameraLookY,0)), fTimeElapsed);

	if (m_pGun) {
		//m_pGun->Update(fTimeElapsed);
		m_pGun->UpdateTransform(m_pGunSlot->GetWorldMatrix());
	}	
}

int GetAnimationIndex(char nMoveInput)
{
	switch (nMoveInput)
	{
	case 0x00: // 입력 없음
		return (int)CAnimationController::ANIMATION_STATE::IDLE;
	case 0x01: // F
		return (int)CAnimationController::ANIMATION_STATE::WALK_FORWARD;
	case 0x02: // B
		return (int)CAnimationController::ANIMATION_STATE::WALK_BACKWARD;
	case 0x03: // F + B -> 상쇄 -> 없음
		return (int)CAnimationController::ANIMATION_STATE::IDLE;
	case 0x04: // L
		return (int)CAnimationController::ANIMATION_STATE::WALK_LEFT;
	case 0x05: // F + L
		return (int)CAnimationController::ANIMATION_STATE::WALK_FORWARD_LEFT;
	case 0x06: // B + L
		return (int)CAnimationController::ANIMATION_STATE::WALK_BACKWARD_LEFT;
	case 0x07: // F + B + L -> F/B 상쇄 -> L
		return (int)CAnimationController::ANIMATION_STATE::WALK_LEFT;
	case 0x08: // R
		return (int)CAnimationController::ANIMATION_STATE::WALK_RIGHT;
	case 0x09: // F + R
		return (int)CAnimationController::ANIMATION_STATE::WALK_FORWARD_RIGHT;
	case 0x0A: // B + R
		return (int)CAnimationController::ANIMATION_STATE::WALK_BACKWARD_RIGHT;
	case 0x0B: // F + B + R -> F/B 상쇄 -> R
		return (int)CAnimationController::ANIMATION_STATE::WALK_RIGHT;
	case 0x0C: // L + R -> 좌우 상쇄 -> 없음
		return (int)CAnimationController::ANIMATION_STATE::IDLE;
	case 0x0D: // L + R + F -> L/R 상쇄 -> F
		return (int)CAnimationController::ANIMATION_STATE::WALK_FORWARD;
	case 0x0E: // L + R + B -> L/R 상쇄 -> B
		return (int)CAnimationController::ANIMATION_STATE::WALK_BACKWARD;
	case 0x0F: // F + B + L + R -> 상하/좌우 모두 상쇄 -> 없음
		return (int)CAnimationController::ANIMATION_STATE::IDLE;
	default:
		return (int)CAnimationController::ANIMATION_STATE::IDLE;
	}
}

void CPlayer::UpdateLowerAnimation()
{
	//if(false) {// 현재 Look과 Velocity를 비교하여 애니메이션 상태 변경
	//	XMFLOAT3 xmf3Look = GetComponent<CRigidBody>()->GetVelocity();
	//	if (Vector3::IsZero(xmf3Look))
	//	{
	//		m_pSkinnedAnimationController->ChangeState(CAnimationController::ANIMATION_STATE::IDLE);
	//		return;
	//	}

	//	// 현재 Look 방향(x,z평면 기준)기준 Right, Forward 벡터를 구한다.
	//	float fRight = Vector3::DotProduct(m_pTransform->GetRight(), xmf3Look);
	//	float fForward = Vector3::DotProduct(m_pTransform->GetLook(), xmf3Look);
	//	float angle = atan2(fForward, fRight);
	//	if (angle < 0.0f) angle += XM_PI * 2.0f; // 각도를 [-PI,PI) 에서 [0, 2PI)로 변환
	//	float degree = XMConvertToDegrees(angle); // degree로 변환[0 ~ 2Pi) => [0, 360)
	//	int nDirection = (int)CAnimationController::ANIMATION_STATE::WALK_RIGHT + static_cast<int>(round(degree / 45.0f)) % 8; // 8방향으로 나누기
	//	float fLength = sqrtf(xmf3Look.x * xmf3Look.x + xmf3Look.z * xmf3Look.z);

	//	if (false == ::IsZero(fLength))
	//	{
	//		m_pSkinnedAnimationController->ChangeState(nDirection);
	//	}
	//}

	int nDirection = GetAnimationIndex(m_nMoveInput);
	m_pSkinnedAnimationController->SetLowerState(nDirection);
}

void CPlayer::Move(DWORD dwDirection, float fDistance, float deltaTime)
{
	if(dwDirection) CGameObject::Move(dwDirection, fDistance, deltaTime);
	SetMoveInput((char)dwDirection);

	
}

void CPlayer::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite)
{
	CGameObject::Render(pd3dCommandList, pCamera, bDepthWrite);

	if (m_pHealthGauge) {
		m_pHealthGauge->SetGauge(m_fHealth / m_fMaxHealth);
	}

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
		if(i != 0) m_pSkinnedAnimationController->SetTrackMask(i, ANIMATION_MASK_FULL, false);
	}

	// 바뀐 Model에 맞춰 Component 변경
	for (auto& pComponent : m_pComponents)
	{
		pComponent->Init(this);
	}

	//auto pCollider = GetComponent<COBBCollider>();
	//pCollider->SetCollider(FindFrame(m_MeshBoneName[m_nSkinType])->GetMeshBound());

	// 바뀐 Model에 맞춰 PrepareSkinning
	m_pGunSlot = FindFrame("GunSlot");
	auto bound = FindFrame(m_MeshBoneName[m_nSkinType])->GetMeshBound();
	m_fCameraLookY = bound.Center.y + bound.Extents.y;

	Update(0.0f);
	UpdateTransform();
}

bool CPlayer::Fire(FIRE_INFO* pFireInfo)
{ 
	if (m_pGun) {
		UpdateTransform();
		m_pGun->UpdateTransform(m_pGunSlot->GetWorldMatrix());
		bool ret =  m_pGun->Fire(GetComponent<CCamera>()->GetLook(), pFireInfo);
		if (ret) {
			SetState(CAnimationController::ANIMATION_STATE::FIRE);
		}
		return ret;
	} 
	return false;
}

void CPlayer::Reload()
{
	SetState(CAnimationController::ANIMATION_STATE::RELOAD);
	m_bReload = true;
	m_fReloadTime = 0.0f;
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

