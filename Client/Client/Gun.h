#pragma once

#include "GameObject.h"

class CGun : public CGameObject
{
public:
	CGun();
	virtual ~CGun();

	// GameObject Override
	static std::shared_ptr<CGun> Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature);

	virtual GAMEOBJECT_LAYER GetLayer() override { return LAYER_GUN; }

	// Object Update
	virtual void Update(float fTimeElapsed) override;

	// Object Render
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr) override;

	// Getters and Setters
	void SetGunType(int type) { m_nGunType = type; }
	int GetGunType() { return m_nGunType; }

	// Methods
	void Fire(XMFLOAT3 xmf3Direction);

private:
	const std::vector<std::string> m_strGunName{ "DefaultGun" }; // 총 이름

	int m_nGunType = 0; // 0: Pistol, 1: Shotgun, 2: Rifle

	float m_fFireRate = 0.5f; // 초당 발사 횟수
	float m_fBulletSpeed = 100.0f; // 총알 속도
	float m_fReloadTime = 2.0f; // 재장전 시간
	int m_nMaxAmmo = 30; // 최대 탄약 수
	int m_nCurrentAmmo = 30; // 현재 탄약 수
};

