#include "Gun.h"


std::shared_ptr<CBulletObject> CGun::m_pBulletObject; // 총알 오브젝트

CGun::CGun()
{
}

CGun::~CGun()
{
}

std::shared_ptr<CGun> CGun::Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nWeaponType)
{
	std::shared_ptr<CGun> pGun = std::make_shared<CGun>();
	pGun->Initialize(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, nWeaponType);
	return pGun;
}

void CGun::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature, int nWeaponType)
{
	CGameObject::Initialize(pd3dDevice, pd3dCommandList);

	// Initialize Gun Type
	m_nGunType = nWeaponType; // Default to Pistol

	// Initialize Ammo
	m_nCurrentAmmo = m_nMaxAmmo;
	m_fFireRate = 0.5f;
	m_fBulletRange = 100.0f;
	m_fReloadTime = 2.0f;

	DeepCopyFromModel(CResourceManager::GetInstance().GetModelInfo(m_strGunName[m_nGunType]));
}

void CGun::Update(float fTimeElapsed)
{
	m_fCoolTime -= fTimeElapsed;
}

void CGun::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	CGameObject::Render(pd3dCommandList, pCamera);
}

// Getters and Setters
void CGun::SetGunType(int type)
{ 
	m_nGunType = type; 

	auto pModel = CResourceManager::GetInstance().GetModelInfo(m_strGunName[m_nGunType]);
	DeepCopyFromModel(pModel);
}

void CGun::Fire()
{
	XMFLOAT3 direction = FindFrame("M16_4_low")->GetUpVector();
	XMFLOAT3 position = FindFrame("M16_4_low")->GetPosition();
	Fire(position, Vector3::ScalarProduct(direction, m_fBulletRange, false));
}

void CGun::Fire(const XMFLOAT3& xmf3Position, const XMFLOAT3& xmf3Direction)
{
	if (m_fCoolTime < 0.0f)
	{
		CGun::m_pBulletObject->AddBullet(xmf3Position, xmf3Direction);
		m_fCoolTime = m_fFireRate;
	}
}
