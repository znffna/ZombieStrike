#pragma once
#include "GameObject.h"
class CGun;
class CGaugeBar;

class CPlayer : public CGameObject
{
public:
	CPlayer();
	virtual ~CPlayer();
	// Object Initialization
	static std::shared_ptr<CPlayer> Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CGameObject> pTerrain, std::shared_ptr<CLoadedModelInfo> pModel, int nAnimationTracks, int nSkinType = 0);
	
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CLoadedModelInfo> pModel, int nSkinType);
	
	virtual std::string GetDefaultName() override { return "CPlayer"; }
	virtual GAMEOBJECT_LAYER GetLayer() { return m_nLayer = LAYER_PLAYER; }

	virtual void Rotate(float x = 0.0f, float y = 0.0f, float z = 0.0f) override;
	virtual float GetPitch() override { return m_fPitch; }
	virtual float GetYaw() override  { return m_fYaw; }
	virtual float GetRoll() override { return m_fRoll; }

	// Object Update
	virtual void Update(float fTimeElapsed) override;
	void UpdateLowerAnimation();
	virtual void Move(DWORD dwDirection, float fDistance, float deltaTime) override;

	// Object Render
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite = false) override;

	// Skin State
	void SetSkinType(int nSkinType)	{ m_nSkinType = nSkinType % m_ModelName.size();}
	int GetSkinType() const { return m_nSkinType; }
	void SetSkin(int nSkinType);

	// Gun
	void SetGun(const std::shared_ptr<CGun>& pGun) { m_pGun = pGun; }
	std::shared_ptr<CGun> GetGun() const { return m_pGun; }

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

	std::shared_ptr<CGameObject> m_pGunSlot;
	std::shared_ptr<CGun> m_pGun;

	std::shared_ptr<CGaugeBar> m_pHealthGauge;

	// Camera Offset
	float m_fCameraLookY = 0.0f;
	
	float m_fPitch = 0.0f;
	float m_fYaw = 0.0f;
	float m_fRoll = 0.0f;

	int m_nSkinType = 0;

	// Player State
	float m_fHealth = 100.0f; // Player Health
	float m_fMaxHealth = 100.0f; // Max Health
	float m_fMoveSpeed = 10.0f; // Player Move Speed

	// Player Move Input
private:
	char m_nMoveInput;
public:
	void SetMoveInput(char nMoveInput) { m_nMoveInput = nMoveInput; }
	char GetMoveInput() { return m_nMoveInput; }
};

