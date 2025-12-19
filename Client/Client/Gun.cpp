#include "Gun.h"


std::shared_ptr<CBulletParticleObject> CGun::m_pBulletObject; // 총알 오브젝트

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
	SetGunType(nWeaponType);

	// Initialize Ammo
	m_nCurrentAmmo = m_nMaxAmmo;
	m_fFireRate = 0.5f;
	m_fBulletRange = 100.0f;
	m_fReloadTime = 2.0f;

	DeepCopyFromModel(CResourceManager::Instance().GetModelInfo(m_strGunName[m_nGunType]));
}

void CGun::Update(float fTimeElapsed)
{
	m_fCoolTime -= fTimeElapsed;
}

void CGun::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite)
{
	CGameObject::Render(pd3dCommandList, pCamera, bDepthWrite);
}

// Getters and Setters
void CGun::SetGunType(int type)
{ 
	m_nGunType = type; 

	switch (m_nGunType)
	{
	case 0: // Assault Rifle
		SetFireTime(12.5f); // 초당 12.5발
		m_fBulletRange = 200.0f;
		break;
	case 1: // Shotgun
		SetFireTime(1.2f); // 초당 12.5발
		m_fBulletRange = 100.0f;
		break;
	}

	auto pModel = CResourceManager::Instance().GetModelInfo(m_strGunName[m_nGunType]);
	DeepCopyFromModel(pModel);
}

void CGun::UpdateTransform(const DirectX::XMFLOAT4X4* xmf4x4ParentMatrix)
{
	if (nullptr == xmf4x4ParentMatrix) return;

	CGameObject::UpdateTransform(xmf4x4ParentMatrix);

}

bool CGun::Fire(const XMFLOAT3& xmf3Direction, FIRE_INFO* pFireInfo)
{
	if (CanFire() == false) return false; // 총이 발사 가능한 상태가 아닐 경우

	m_fCoolTime = m_fFireRate;
	m_nCurrentAmmo--; //일단 Reload 없이 총 발사 간격만 적용.

	XMFLOAT3 direction = xmf3Direction;
	XMFLOAT3 position = FindFrame(m_strMuzzleName[m_nGunType])->GetPosition(); // 총구 위치

	FIRE_INFO fireInfo;
	fireInfo.xmf3Position = position;
	fireInfo.xmf3Look = direction;
	fireInfo.xmf3MuzzlePosition = position;
	fireInfo.nBulletType = m_nGunType; // 총알 타입 설정
	fireInfo.fRange = GetRange();
	fireInfo.fspeed = GetSpeed();
	m_pBulletObject->AddFireInfo(fireInfo);

	{
		std::string debug = "Gun Fire \n";
		debug += "Position: " + std::to_string(position.x) + ", " + std::to_string(position.y) + ", " + std::to_string(position.z) + "\n";
		debug += "Direction: " + std::to_string(direction.x) + ", " + std::to_string(direction.y) + ", " + std::to_string(direction.z) + "\n";
		OutputDebugStringA(debug.c_str());
	}

	if (pFireInfo) {
		*pFireInfo = fireInfo; // 발사 정보 전달
	}

	return true;
	//return Fire(position, Vector3::ScalarProduct(direction, m_fBulletRange, false));
}

// ----------------------------------------------
// 아래 Fire는 실제 카메라의 위치와 방향을 통해 조준점 위주 판정 이후 피격위치를 향해 궤적 생성하는 방식
// 으로 구현할때 사용할 함수.
// ----------------------------------------------
bool CGun::Fire(const XMFLOAT3& xmf3Position, const XMFLOAT3& xmf3Direction, FIRE_INFO* pFireInfo)
{
	if (CanFire() == false) return false; // 총이 발사 가능한 상태가 아닐 경우

	XMFLOAT3 Muzzleposition = FindFrame(m_strMuzzleName[m_nGunType])->GetPosition(); // 총구 위치

	FIRE_INFO fireInfo;
	fireInfo.xmf3Position = xmf3Position;
	fireInfo.xmf3Look = xmf3Direction;
	fireInfo.xmf3MuzzlePosition = Muzzleposition;
	fireInfo.nBulletType = m_nGunType; // 총알 타입 설정
	fireInfo.fRange = GetRange();
	fireInfo.fspeed = GetSpeed();
	m_pBulletObject->AddFireInfo(fireInfo);

	if (pFireInfo) {
		*pFireInfo = fireInfo; // 발사 정보 전달
	}
	
	return true; // 발사 성공
}
