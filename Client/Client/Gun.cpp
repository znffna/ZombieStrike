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

	DeepCopyFromModel(CResourceManager::GetInstance().GetModelInfo(m_strGunName[m_nGunType]));
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

	auto pModel = CResourceManager::GetInstance().GetModelInfo(m_strGunName[m_nGunType]);
	DeepCopyFromModel(pModel);
}

void CGun::UpdateTransform(const DirectX::XMFLOAT4X4* xmf4x4ParentMatrix)
{
	if (nullptr == xmf4x4ParentMatrix) return;

	CGameObject::UpdateTransform(xmf4x4ParentMatrix);

	if(g_bDebugOutput){
		std::string debug = "Gun UpdateTransform \n";
		debug += "Position: " + std::to_string(GetPosition().x) + ", " + std::to_string(GetPosition().y) + ", " + std::to_string(GetPosition().z) + "\n";
		OutputDebugStringA(debug.c_str());
		if (GetPosition().x + GetPosition().y + GetPosition().z < 1.0f) {
			std::cout << 1;
		}
	}
}

void CGun::Fire(const XMFLOAT3& xmf3Direction)
{
	XMFLOAT3 direction = xmf3Direction;
	XMFLOAT3 position = FindFrame("M16_4_low")->GetPosition(); // 총구 위치

	Fire(position, Vector3::ScalarProduct(direction, m_fBulletRange, false));
}

void CGun::Fire(const XMFLOAT3& xmf3Position, const XMFLOAT3& xmf3Direction)
{
	while (m_fCoolTime < 0.0f)
	{
		m_fCoolTime += m_fFireRate;

		CBulletVertex pBulletVertice;
		pBulletVertice.m_xmf3Position = xmf3Position;
		pBulletVertice.m_xmf3Velocity = xmf3Direction;
		pBulletVertice.m_fLifetime = 0.6f;
		pBulletVertice.m_nBulletType = m_nGunType;

		CGun::m_pBulletObject->AddBullet(pBulletVertice);

		{
			std::string debug = "Gun Fire \n";
			debug += "Position: " + std::to_string(xmf3Position.x) + ", " + std::to_string(xmf3Position.y) + ", " + std::to_string(xmf3Position.z) + "\n";
			debug += "Direction: " + std::to_string(xmf3Direction.x) + ", " + std::to_string(xmf3Direction.y) + ", " + std::to_string(xmf3Direction.z) + "\n";
			OutputDebugStringA(debug.c_str());
		}
	}
}
