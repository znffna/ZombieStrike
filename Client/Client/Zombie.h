#pragma once

#include "GameObject.h"
#include "AnimationController.h"

class CZombieCAnimationController : public CAnimationController
{
public:
	CZombieCAnimationController();
	~CZombieCAnimationController();
};

class CZombieObject : public CGameObject
{
public:
	CZombieObject();
	virtual ~CZombieObject();

	// Object Initialization
	static std::shared_ptr<CZombieObject> Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CGameObject> pTerrain, std::shared_ptr<CLoadedModelInfo> pModel, int nSkinType);
	
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CLoadedModelInfo> pModel, int nSkinType);
	
	virtual std::string GetDefaultName() override { return "CZombieObject"; }

	virtual void Update(float fTimeElapsed) override;

	// Skin State
	void SetSkinType(int nSkinType);
	int GetSkinType() const;
	void SetSkin(int nSkinType);

	// Died
	bool m_bDied = false; // 좀비가 죽었는지 여부
	float m_fDeathTime = 0.0f; // 죽은 시간
	float m_fMaxDeathTime = 5.0f; // 최대 죽은 시간
	void Died()
	{
		if(false == m_bDied)
		{
			if (m_pSkinnedAnimationController)
			{
				SetState((int)ANIMATION_POSE::ZOMBIE_DEATH);
			}
			m_bDied = true;
		}
	}
private:
	std::vector<std::string> m_strModelName{ "PoliceZombie", "Yaku_J_Ignite", "Zombiegirl_W_Kurniawan"};
	std::vector<std::string> m_strMeshBoneName{ "FuzZombie", "Yaku_zombie", "ZombieGirl_Body"};
	int m_nSkinType = 0;
};