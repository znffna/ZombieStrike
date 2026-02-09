#pragma once

#include "GameObject.h"
#include "AnimationController.h"

enum ZOMBIE_ANIMATION_POSE : int // Number == Animation Track Index
{
	/// Zombie Animation States
	ZOMBIE_IDLE = 0,
	ZOMBIE_RUNNING,
	ZOMBIE_ATTACK,
	ZOMBIE_DEATH,
	ZOMBIE_SCREAM,
	ZOMBIE_HIT,

};

class CZombieCAnimationController : public CAnimationController
{
public:
	CZombieCAnimationController(CGameObject* pOwner);
	~CZombieCAnimationController();
};

class CZombieObject : public CGameObject
{
public:
	CZombieObject();
	virtual ~CZombieObject();

	// Object Initialization
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nSkinType);
	virtual void Initialize();
	virtual void Initialize(int nSkinType);
	
	virtual std::string GetDefaultName() override { return "CZombieObject"; }

	virtual void Update(float fTimeElapsed) override;

	// Skin State
	void	SetSkinType(int nSkinType);
	int		GetSkinType() const;
	void	SetSkin(int nSkinType);

	// Died
	bool	m_bDied = false; // 좀비가 죽었는지 여부
	float	m_fDeathTime = 0.0f; // 죽은 시간
	float	m_fMaxDeathTime = 5.0f; // 최대 죽은 시간

	void Died()
	{
		if (m_bDied) return;

		m_bDied = true;
		m_fDeathTime = 0.0f;

		if (auto pAnim = GetComponent<CAnimationController>())  
		{
			pAnim->SetBasePose((int)ZOMBIE_ANIMATION_POSE::ZOMBIE_DEATH);
		}

		
	}
	/*void Died()
	{
		if(false == m_bDied)
		{
			if (auto pSkinnedAnimationController = CreateComponent<CAnimationController>())
			{
				SetState((int)ZoMBIE_ANIMATION_POSE::ZOMBIE_DEATH);
			}
			m_bDied = true;
		}
	}*/

	void ApplyNetState( const XMFLOAT3& pos, const XMFLOAT3& vel, int actType, int hp)
	{
		SetPosition(pos);

		if (auto rb = GetComponent<CRigidBody>())
			rb->SetVelocity(vel);

		m_xmf3NetVelocity = vel;
		m_nNetActType = actType;
		m_nNetHP = hp;

		if (hp <= 0)
			Died();
	}


	const XMFLOAT3& GetNetVelocity() const { return m_xmf3NetVelocity; }
	int  GetNetActType() const { return m_nNetActType; }
	int  GetNetHP() const { return m_nNetHP; }

	void PlayCrySfx();   // 울음소리 재생
	void PlayBiteSfx();  // 무는소리 재생


private:
	std::vector<std::string> m_strModelName{ "PoliceZombie", "Yaku_J_Ignite", "Zombiegirl_W_Kurniawan"};
	std::vector<std::string> m_strMeshBoneName{ "FuzZombie", "Yaku_zombie", "ZombieGirl_Body"};
	int m_nSkinType = 0;

	float m_fCrySfxCooldown = 0.0f;     //  울음소리 쿨타임
	float m_fBiteSfxCooldown = 0.0f;    //  무는소리 쿨타임

private:

	XMFLOAT3 m_xmf3NetVelocity = XMFLOAT3(0.f, 0.f, 0.f); // 서버 속도
	int      m_nNetActType = 0;                           // 서버 액션 타입
	int      m_nNetHP = 0;                                // 서버 HP

};