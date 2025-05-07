#pragma once

#include "stdafx.h"
#include "NetworkClient.h"
#include "GameScene.h"

extern bool g_bNetworkDebugMode;

class COnlineScene : public CGameScene
{
public:
	COnlineScene();
	virtual ~COnlineScene();

	// Scene Override
	virtual void InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature) override;
	virtual void ReleaseObjects() override;
	virtual void ReleaseUploadBuffers() override;

	virtual void StartScene() { m_NetworkClient.StartRecvLoop(); CScene::SetSceneState(SCENE_STATE_RUNNING); }

	virtual void Update(float deltaTime) override;

	virtual bool ProcessInput(const INPUT_PARAMETER& pBuffer, float deltaTime) override;
	virtual void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;
	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;


	// Network Override
	virtual void ProcessPacket(PacketHeader* recv_p); // Recv 내용 처리 (m_NetworkClient로 부터	호출됨)

	void SendPlayerState();

	NetworkingClient* GetClient() { return &m_NetworkClient; }//
private:
	NetworkingClient m_NetworkClient{ this };
	std::unordered_map<int, std::shared_ptr<CGameObject>> m_mapGameObjects;
};

