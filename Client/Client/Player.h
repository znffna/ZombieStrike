#pragma once
#include "GameObject.h"
class CGun;
struct FIRE_INFO;
class CGaugeBar;

enum PLAYER_ANIMATION_POSE : int // Number == Animation Track Index
{
	/// Player Animation States
	// IDLE(Aiming)
	PLAYER_IDLE = 0,
	// WALK
	WALK_RIGHT,
	WALK_FORWARD_RIGHT,
	WALK_FORWARD,
	WALK_FORWARD_LEFT,
	WALK_LEFT,
	WALK_BACKWARD_LEFT,
	WALK_BACKWARD,
	WALK_BACKWARD_RIGHT,
	// FIRE
	FIRE,
	// Reload
	RELOAD,
	// Hitted
	HITTED,
};

class CPlayer : public CGameObject
{
public:
	CPlayer();
	virtual ~CPlayer();
	// Object Initialization
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nSkinIndex);
	
	virtual void Initialize(); // Component 만을 생성. 실제 DX 관련 초기화는 CResourceManager에 Register하여 일괄 처리
	virtual void Initialize(int nSkinIndex); // Component 만을 생성. 실제 DX 관련 초기화는 CResourceManager에 Register하여 일괄 처리
	
	virtual std::string GetDefaultName() override { return "CPlayer"; }

	virtual void Rotate(float x = 0.0f, float y = 0.0f, float z = 0.0f) override;
	virtual float GetPitch() override { return m_fPitch; }
	virtual float GetYaw() override  { return m_fYaw; }
	virtual float GetRoll() override { return m_fRoll; }

	// Object Update
	virtual void Update(float fTimeElapsed) override;
	void UpdateBaseAnimation();
	void UpdateUpperAnimation();
	virtual void Move(DWORD dwDirection, float fDistance, float deltaTime) override;

	virtual void OnPrepareAnimate();

	// Object Render
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite = false) override;

	// Skin State
	void SetSkinType(int nSkinType) { m_nSkinType = nSkinType % m_ModelName.size(); } // 여기선 Index만 설정 
	int GetSkinType() const { return m_nSkinType; }
	void SetSkin(int nSkinType);

	// Gun
	void SetGun(CGun* pGun) { m_pGun = pGun; }
	CGun* GetGun() const { return m_pGun; }

	bool Fire(FIRE_INFO* pFireInfo);

	// Reload
	bool m_bReload = false; // 총알 재장전 여부
	float m_fReloadTime = 0.0f;
	void Reload();

	// UI
	void SetHealthObject(const std::shared_ptr<CGaugeBar>& pHealthGauge) { m_pHealthGauge = pHealthGauge; }

private:

	std::vector<std::string> m_ModelName{ "Ch18_nonPBR", "Ch35_nonPBR" };
	std::vector<std::string> m_MeshBoneName{ "Ch18", "Ch35" };

	// Gun Slot Bone
	CGameObject* m_pGunSlot = nullptr;

	CGun* m_pGun = nullptr ;

	std::shared_ptr<CGaugeBar> m_pHealthGauge;

	// Camera Offset
	float m_fCameraLookY = 0.0f;
	
	float m_fPitch = 0.0f;
	float m_fYaw = 0.0f;
	float m_fRoll = 0.0f;

	int m_nSkinType = 0;

	// Player State
private:
	float m_fHealth = 100.0f; // Player Health
	float m_fMaxHealth = 100.0f; // Max Health
	float m_fMoveSpeed = 10.0f; // Player Move Speed

public:
	void SetHealth(float fHealth) { m_fHealth = fHealth; }
	float GetHealth() const { return m_fHealth; }
	void SetMaxHealth(float fMaxHealth) { m_fMaxHealth = fMaxHealth; }
	float GetMaxHealth() const { return m_fMaxHealth; }
	void SetMoveSpeed(float fMoveSpeed) { m_fMoveSpeed = fMoveSpeed; }
	float GetMoveSpeed() const { return m_fMoveSpeed; }

	// Player Move Input
private:
	char m_nMoveInput = 0;
	int	 m_nState = 0;


private:
	XMFLOAT3	m_xmf3NetVelocity = XMFLOAT3(0.f, 0.f, 0.f);
	int			m_nNetActType = 0;
	bool		m_bHasNetAnimState = false;

public:
	void SetNetVelocity(XMFLOAT3& v) { m_xmf3NetVelocity = v; m_bHasNetAnimState = true; }
	XMFLOAT3& GetNetVelocity() { return m_xmf3NetVelocity; }

	void SetNetActType(int actType) { m_nNetActType = actType; m_bHasNetAnimState = true; }
	int GetNetActType() { return m_nNetActType; }

	bool HasNetAnimState() { return m_bHasNetAnimState; }


public:
	void SetMoveInput(char nMoveInput) { m_nMoveInput = nMoveInput; }
	char GetMoveInput() { return m_nMoveInput; }
};

