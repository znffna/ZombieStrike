#pragma once

#include "stdafx.h"
#include "NetworkClient.h"
#include "GameScene.h"


class COnlineScene : public CGameScene
{
public:
	COnlineScene();
	virtual ~COnlineScene();

	virtual const std::wstring& GetSceneName() const override { static std::wstring scenename = L"COnlineScene"; return scenename; }

	// Scene Override
	virtual void InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature) override;
	virtual void PostInitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature) override;
	virtual void ReleaseUploadBuffers() override;

	virtual void StartScene();
	virtual void PopScene() {
		NetworkingClient::Instance().Logout();
		CScene::PopScene();
		m_mapGameObjects.clear();
	}

	virtual void Update(float deltaTime) override;

	virtual bool ProcessMouseInput(float cxDelta, float cyDelta, float deltaTime) override;
	virtual bool ProcessKeyboardInput(const UCHAR pKeysBuffer[256], float deltaTime) override;
	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;

	// Network Override
	void ProcessReadQueuePacket();
	virtual void ProcessPacket(PacketHeader* recv_p); // Recv 내용 처리 (m_NetworkClient로 부터	호출됨)

	void SendPlayerState();
	void SendFirePacket(const FIRE_INFO fireInfo);

	virtual bool Fire(CPlayer* pPlayer, FIRE_INFO* pFireInfo);
	virtual bool Fire(CPlayer* pPlayer);

private:
	std::unordered_map<int, std::shared_ptr<CGameObject>> m_mapGameObjects;

	char m_nMoveInput;
};

