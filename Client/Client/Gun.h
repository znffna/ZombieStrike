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
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr) override;

	// Getters and Setters
	void SetGunType(int type);
	int GetGunType() { return m_nGunType; }

	// Methods
	void Fire(const XMFLOAT3& xmf3Direction);
	void Fire(const XMFLOAT3& xmf3Position, const XMFLOAT3& xmf3Direction);

	static std::shared_ptr<CBulletObject> m_pBulletObject; // 총알 오브젝트
private:

	const std::vector<std::string> m_strGunName{ "M16" }; // 총 이름

	int m_nGunType = 0; // 0: Assault Rifle, 1: Shotgun

	float m_fFireRate = 12.5f; // 초당 발사 횟수
	float m_fCoolTime = 0.0f; // 발사 대기 시간
	float m_fBulletRange = 100.0f; // 총알 속도
	float m_fReloadTime = 2.0f; // 재장전 시간
	int m_nMaxAmmo = 30; // 최대 탄약 수
	int m_nCurrentAmmo = 30; // 현재 탄약 수
};

