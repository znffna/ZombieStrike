#include "Gun.h"

CGun::CGun()
{
}

CGun::~CGun()
{
}

std::shared_ptr<CGun> CGun::Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	std::shared_ptr<CGun> pGun = std::make_shared<CGun>();
	pGun->Initialize(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);
	return pGun;
}

void CGun::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	CGameObject::Initialize(pd3dDevice, pd3dCommandList);

	// Initialize Gun Type
	m_nGunType = 0; // Default to Pistol

	// Initialize Ammo
	m_nCurrentAmmo = m_nMaxAmmo;
	m_fFireRate = 0.5f;
	m_fBulletSpeed = 100.0f;
	m_fReloadTime = 2.0f;



}

void CGun::Update(float fTimeElapsed)
{
	CGameObject::Update(fTimeElapsed);
}

void CGun::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	CGameObject::Render(pd3dCommandList, pCamera);
}

void CGun::Fire(XMFLOAT3 xmf3Direction)
{

}
