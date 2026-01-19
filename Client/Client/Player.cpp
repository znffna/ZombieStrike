#include "Player.h"
#include "Scene.h"

#include "Gun.h"
#include "GaugeBar.h"

///////////////////////////////////////////////////////////////////////////////////
//

CPlayer::CPlayer()

{
	SetLayer(LAYER_PLAYER);
}

CPlayer::~CPlayer()
{
}

void CPlayer::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nSkinIndex)
{
	CGameObject::Initialize(pd3dDevice, pd3dCommandList);

	// Object Info
	SetName("Player_" + std::to_string(GetID()));

	SetRotationAxisLock(true, false, true);

	// <Components>
	// Animation Controller
	auto pSkinnedAnimationController = CreateComponent<CAnimationController>();
	auto pModel = CResourceManager::Instance().GetModelInfo(m_ModelName[nSkinIndex]);
	pSkinnedAnimationController->SetModel(pModel);

	// RigidBody 생성
	auto pRigidBody = CreateComponent<CRigidBody>();
	pRigidBody->SetGravity(XMFLOAT3(0.0f, 0.0f, 0.0f));
	//pRigidBody->SetVelocity(XMFLOAT3(0.0f, -9.0f, 0.0f));

	// Camera 생성
	auto pCamera = CreateComponent<CThirdPersonCamera>();
	pCamera->SetViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	pCamera->SetScissorRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	pCamera->SetOffset(XMFLOAT3(1.0f, 0.7f, -2.5f));
	pCamera->GenerateViewMatrix(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	pCamera->GenerateProjectionMatrix(((float)WINDOW_WIDTH / (float)WINDOW_HEIGHT), 60.0f, 1.0f, 1000.0f);
	pCamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	pCamera->SetActive(true);

	// Collider 생성
	//auto pCollider = CreateComponent<COBBCollider>();

	//Update(0.0f);
	//UpdateTransform();
}

void CPlayer::Update(float fTimeElapsed)
{
	if (auto pSkinnedAnimationController = CreateComponent<CAnimationController>()) UpdateLowerAnimation();

	CGameObject::Update(fTimeElapsed);

	if(m_bReload)
	{
		m_fReloadTime += fTimeElapsed;
		if (m_fReloadTime >= 1.0f) {
			if (m_pGun) m_pGun->Reload();
			m_fReloadTime = 0.0f;
			SetState(ANIMATION_POSE::IDLE);
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
		return (int)ANIMATION_POSE::IDLE;
	case 0x01: // F
		return (int)ANIMATION_POSE::WALK_FORWARD;
	case 0x02: // B
		return (int)ANIMATION_POSE::WALK_BACKWARD;
	case 0x03: // F + B -> 상쇄 -> 없음
		return (int)ANIMATION_POSE::IDLE;
	case 0x04: // L
		return (int)ANIMATION_POSE::WALK_LEFT;
	case 0x05: // F + L
		return (int)ANIMATION_POSE::WALK_FORWARD_LEFT;
	case 0x06: // B + L
		return (int)ANIMATION_POSE::WALK_BACKWARD_LEFT;
	case 0x07: // F + B + L -> F/B 상쇄 -> L
		return (int)ANIMATION_POSE::WALK_LEFT;
	case 0x08: // R
		return (int)ANIMATION_POSE::WALK_RIGHT;
	case 0x09: // F + R
		return (int)ANIMATION_POSE::WALK_FORWARD_RIGHT;
	case 0x0A: // B + R
		return (int)ANIMATION_POSE::WALK_BACKWARD_RIGHT;
	case 0x0B: // F + B + R -> F/B 상쇄 -> R
		return (int)ANIMATION_POSE::WALK_RIGHT;
	case 0x0C: // L + R -> 좌우 상쇄 -> 없음
		return (int)ANIMATION_POSE::IDLE;
	case 0x0D: // L + R + F -> L/R 상쇄 -> F
		return (int)ANIMATION_POSE::WALK_FORWARD;
	case 0x0E: // L + R + B -> L/R 상쇄 -> B
		return (int)ANIMATION_POSE::WALK_BACKWARD;
	case 0x0F: // F + B + L + R -> 상하/좌우 모두 상쇄 -> 없음
		return (int)ANIMATION_POSE::IDLE;
	default:
		return (int)ANIMATION_POSE::IDLE;
	}
}

void CPlayer::UpdateLowerAnimation()
{
	//if(false) {// 현재 Look과 Velocity를 비교하여 애니메이션 상태 변경
	//	XMFLOAT3 xmf3Look = GetComponent<CRigidBody>()->GetVelocity();
	//	if (Vector3::IsZero(xmf3Look))
	//	{
	//		m_pSkinnedAnimationController->ChangeState(ANIMATION_POSE::IDLE);
	//		return;
	//	}

	//	// 현재 Look 방향(x,z평면 기준)기준 Right, Forward 벡터를 구한다.
	//	float fRight = Vector3::DotProduct(m_pTransform->GetRight(), xmf3Look);
	//	float fForward = Vector3::DotProduct(m_pTransform->GetLook(), xmf3Look);
	//	float angle = atan2(fForward, fRight);
	//	if (angle < 0.0f) angle += XM_PI * 2.0f; // 각도를 [-PI,PI) 에서 [0, 2PI)로 변환
	//	float degree = XMConvertToDegrees(angle); // degree로 변환[0 ~ 2Pi) => [0, 360)
	//	int nDirection = (int)ANIMATION_POSE::WALK_RIGHT + static_cast<int>(round(degree / 45.0f)) % 8; // 8방향으로 나누기
	//	float fLength = sqrtf(xmf3Look.x * xmf3Look.x + xmf3Look.z * xmf3Look.z);

	//	if (false == ::IsZero(fLength))
	//	{
	//		m_pSkinnedAnimationController->ChangeState(nDirection);
	//	}
	//}

	int nDirection = GetAnimationIndex(m_nMoveInput);
	//m_pSkinnedAnimationController->SetLowerState(nDirection);
}

void CPlayer::Move(DWORD dwDirection, float fDistance, float deltaTime)
{
	if(dwDirection) CGameObject::Move(dwDirection, fDistance, deltaTime);
	SetMoveInput((char)dwDirection);

	
}

void CPlayer::OnPrepareAnimate()
{
	if (auto pSkinnedAnimationController = CreateComponent<CAnimationController>())
	{
		m_pGunSlot = pSkinnedAnimationController->m_pModelRootObject->FindFrame("GunSlot");
		if (m_pGunSlot) {
			auto bound = pSkinnedAnimationController->m_pModelRootObject->FindFrame(m_MeshBoneName[m_nSkinType])->GetMeshBound();
			m_fCameraLookY = bound.Center.y + bound.Extents.y;
		}
	}
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
	auto pPlayerModel = CResourceManager::Instance().GetModelInfo(m_ModelName[m_nSkinType]);

	// 바뀐 Model에 맞춰 Component 변경
	auto animatoncontroller = GetComponent<CAnimationController>();
	animatoncontroller->SetModel(pPlayerModel);

	//auto pCollider = GetComponent<COBBCollider>();
	//pCollider->SetCollider(FindFrame(m_MeshBoneName[m_nSkinType])->GetMeshBound());

	// 바뀐 Model에 맞춰 PrepareSkinning
	OnPrepareAnimate();

	//Update(0.0f);
	//UpdateTransform();
}

bool CPlayer::Fire(FIRE_INFO* pFireInfo)
{ 
	if (m_pGun) {
		UpdateTransform();
		m_pGun->UpdateTransform(m_pGunSlot->GetWorldMatrix());
		auto pCamera = GetComponent<CCamera>();
		bool ret =  m_pGun->Fire(pCamera->GetPosition(), pCamera->GetLook(), pFireInfo);
		if (ret) {
			SetState(ANIMATION_POSE::FIRE);
		}
		return ret;
	} 
	return false;
}

void CPlayer::Reload()
{
	SetState(ANIMATION_POSE::RELOAD);
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

	/*if (m_bPitchLock) x = 0.0f;
	if (m_bYawLock) y = 0.0f;
	if (m_bRollLock) z = 0.0f;

	m_pTransform->Rotate(x, y, z);*/
}

