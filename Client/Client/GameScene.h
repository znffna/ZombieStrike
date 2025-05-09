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

	// Scene Initialization / Release
	virtual void InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature) override;
	virtual void PostInitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature) override;
	
	virtual void CreateFixedCamera(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) override;

	virtual void ReleaseObjects() override;
	virtual void ReleaseUploadBuffers() override;

	virtual void Update(float deltaTime) override;

	virtual bool ProcessInput(const INPUT_PARAMETER& pBuffer, float deltaTime) override;
	virtual void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;
	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;

	virtual void ChangeMap(int nMapIndex)
	{
		m_nStageIndex = nMapIndex % m_strStageNames.size();

		if (m_pTerrain) RemoveObject(m_pTerrain);
		m_pTerrain = GetTerrain(m_nStageIndex);
		AddObject(m_pTerrain);
		
		if (m_pMap) {
			for (auto& pObject : m_pMap->GetChilds())
			{
				RemoveObject(pObject);
			}
		}

		m_pMap = m_pTerrain->GetChilds()[0];

		auto pMap = CResourceManager::GetInstance().GetModelInfo(m_strStageNames[m_nStageIndex]);
		pMap->m_pModelRootObject->UpdateTransform();
		m_pMap = pMap->m_pModelRootObject;
		m_pMap->Update(0.0f);
		m_pMap->SetLayer(CGameObject::LAYER_ENVIRONMENT);
		AddObject(m_pMap);
	};

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
};