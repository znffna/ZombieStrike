///////////////////////////////////////////////////////////////////////////////
// Date: 2025-04-04
// GameScene.h : CGameScene 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Scene.h"



class CGameScene : public CScene
{
public:
	CGameScene();
	virtual ~CGameScene();

	virtual const std::wstring& GetSceneName() const override { static std::wstring scenename = L"CGameScene"; return scenename; }
	
	// Scene Initialization / Release
	virtual void InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature) override;
	void CreateFreeCamera(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void PostInitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature) override;
	
	virtual void CreateDefaultCamera(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) override;

	virtual void ReleaseObjects() override;
	virtual void ReleaseUploadBuffers() override;

	virtual void SetCursor() override { g_bEnableCursor = false; }

	virtual void Update(float deltaTime) override;
	virtual void UpdateLights() override;

	virtual void OnPostRender(ID3D12GraphicsCommandList *pd3dCommandList) override;

	virtual bool ProcessMouseInput(float cxDelta, float cyDelta, float deltaTime) override;
	virtual bool ProcessKeyboardInput(const UCHAR pKeysBuffer[256], float deltaTime) override;
	virtual void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;
	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;

	virtual void ChangeMap(int nMapIndex);

	virtual bool Fire(CPlayer* pPlayer, FIRE_INFO* pFireInfo);
	virtual bool Fire(CPlayer* pPlayer, XMFLOAT3 cameraPos, XMFLOAT3 cameraDir);

	// Shader Variables
	//void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) override;
	//void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList) override;
	//void ReleaseShaderVariables() override;

public:
	CPlayer* SpawnPlayer(XMFLOAT3 xmf3Position, std::string name, int nSkinIndex, short starthp, char actType, char move_input, void * pTerrain = nullptr);
	CZombieObject* SpawnZombie(XMFLOAT3 xmf3Position, std::string name, int nSkinIndex, short starthp, char actType, char move_input);

	CHeightMapTerrain* ChangeTerrain(int nMapIndex);

public:
	// Map FileName
	const std::vector<std::string> m_strStageNames = { "Stage1", "Stage2", "Stage3" };
	int m_nStageIndex = 0;

	// Cache Control object
	CBulletParticleObject* m_pBulletObject = nullptr;
	CCollisionChecker* m_pCollisionChecker = nullptr;
	CHeightMapTerrain* m_pTerrain = nullptr;

	void BuildFiredBullets();

	bool m_bPrintObjectCount = false; // 디버그 출력용

	std::unique_ptr<CGameObject> m_pHealthObject;

	// Free Camera 참조용
	CGameObject* m_pFreeCamera = nullptr;

public:
	void SetScore(int nScore) {};
	int GetScore() const { return m_nScore; }

	// Score
	int m_nScore = 0;
	int m_nWave = 1;

	CGameObject* m_pScoreInfo = nullptr;
	CGameObject* m_pAmmoInfo = nullptr;
};