#pragma once

#include "GameObject.h"

class CGun : public CGameObject
{
public:
	CGun();
	virtual ~CGun();

	// GameObject Override
	static std::shared_ptr<CGun> Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nWeaponType);
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature, int nWeaponType);

	virtual GAMEOBJECT_LAYER GetLayer() override { return m_nLayer = LAYER_GUN; }

	// Object Update
	virtual void Update(float fTimeElapsed) override;

	// Object Render
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr, bool bDepthWrite = false) override;

	// Getters and Setters
	void SetGunType(int type);
	int GetGunType() { return m_nGunType; }

	float GetRange() const { return m_fBulletRange; }
	float GetSpeed() const { return m_fBulletSpeed; } // 총알 속도

	using CGameObject::UpdateTransform; 
	virtual void UpdateTransform(const DirectX::XMFLOAT4X4* xmf4x4ParentMatrix = nullptr);

	// Methods
	bool CanFire() {
		if (m_fCoolTime <= 0.0f && m_nCurrentAmmo > 0) {
			m_fCoolTime = m_fFireRate;
			{
				std::string debug = "CGun::CanFire() - Gun Type: " + std::to_string(m_nGunType) + ", Ammo: " + std::to_string(m_nCurrentAmmo) + "\n";
				OutputDebugStringA(debug.c_str());
			}
			//m_nCurrentAmmo--; 일단 Reload 없이 총 발사 간격만 적용.
			return true; // 발사 성공
		}
		return false; // 발사 실패
	}
	
	void Fire(const XMFLOAT3& xmf3Direction);
	void Fire(const XMFLOAT3& xmf3Position, const XMFLOAT3& xmf3Direction);

	static std::shared_ptr<CBulletParticleObject> m_pBulletObject; // 총알 오브젝트
private:
	void SetFireTime(float fFireTimePerSecond)
	{ 
		m_fFireTimePerSecond = fFireTimePerSecond;
		m_fFireRate = 1.0f / fFireTimePerSecond; 
	}

	const std::vector<std::string> m_strGunName{ "M16" }; // 총 이름

	int m_nGunType = 0; // 0: Assault Rifle, 1: Shotgun

	float m_fFireTimePerSecond = 10.0f; // 초당 발사 횟수
	float m_fFireRate = 1.0f / 10.0f; // 발당 시간 (초당 발사 횟수의 역수)
	float m_fCoolTime = 0.0f; // 발사 대기 시간
	float m_fBulletSpeed = 300.0f; // 총알 속도 (미터/초)
	float m_fBulletRange = 300.0f; // 총알 최대 사거리
	float m_fReloadTime = 2.0f; // 재장전 시간
	int m_nMaxAmmo = 30; // 최대 탄약 수
	int m_nCurrentAmmo = 30; // 현재 탄약 수
};

