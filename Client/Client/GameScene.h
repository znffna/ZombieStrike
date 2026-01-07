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

	virtual void ChangeMap(int nMapIndex);;

	bool m_bIschambered = false; // 총알 장전 여부
	virtual bool Fire(CPlayer* pPlayer, FIRE_INFO* pFireInfo);

	// Shader Variables
	//void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) override;
	//void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList) override;
	//void ReleaseShaderVariables() override;

public:
	const std::vector<std::string> m_strStageNames = { "Stage1", "Stage2", "Stage3" };
	int m_nStageIndex = 0;

	// ObjectPool
	std::vector<std::shared_ptr<CZombieObject>> m_pZombiePool;
	void StoreZombie(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature, int nZombieCount);
	std::shared_ptr<CZombieObject> GetZombie(int nSkinType = 0);

	
	std::vector<std::shared_ptr<CPlayer>> m_pPlayerObjects;
	void StorePlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature, int nPlayerCount);
	std::shared_ptr<CPlayer> GetPlayer(int nSkinType = 0);
	
	std::vector<std::shared_ptr<CGameObject>> m_pTerrainObjects;
	void StoreTerrain(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature, int nTerrainCount);
	std::shared_ptr<CGameObject> GetTerrain(int nSkinType = 0)
	{
		return m_pTerrainObjects[nSkinType % (int)m_pTerrainObjects.size()];
	};

	std::shared_ptr<CBulletParticleObject> m_pBulletObject;
	CCollisionChecker* m_pCollisionChecker;
	void BuildFiredBullets();


	bool m_bPrintObjectCount = false; // 디버그 출력용

	std::unique_ptr<CGameObject> m_pHealthObject;

	// Free Camera 참조용
	CGameObject* m_pFreeCamera;
};