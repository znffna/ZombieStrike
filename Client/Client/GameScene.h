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

	// Shader Variables
	//void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) override;
	//void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList) override;
	//void ReleaseShaderVariables() override;

public:
	// ObjectPool
	std::vector<std::shared_ptr<CZombieObject>> m_pZombiePool;
	void StoreZombie(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature, int nZombieCount);
	std::shared_ptr<CZombieObject> GetZombie(int nSkinType = 0);

	
	std::vector<std::shared_ptr<CPlayer>> m_pPlayerObjects;
	void StorePlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature, int nPlayerCount);
	std::shared_ptr<CPlayer> GetPlayer(int nSkinType = 0);
	
};