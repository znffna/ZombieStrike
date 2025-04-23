#pragma once

#include "stdafx.h"
#include "NetworkClient.h"
#include "GameScene.h"

class COnlineScene : public CGameScene
{
public:
	COnlineScene();
	virtual ~COnlineScene();

	// Scene Override
	virtual void InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature) override;
	virtual void ReleaseObjects() override;
	virtual void ReleaseUploadBuffers() override;

	virtual void Update(float deltaTime) override;

	virtual bool ProcessInput(const INPUT_PARAMETER& pBuffer, float deltaTime) override;
	virtual void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;
	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;

	// Network Override
	virtual void ProcessPacket(char* recv_p);

	void SendPlayerState()
	{
		if(m_pPlayer)
		{
			pkt_cs_update packet{};
			packet.header.size = sizeof(packet);
			packet.header.type = PKT_TYPE::C_S_UPDATE;
			packet.obj.level = 1; // 레벨
			packet.obj.score = 0; // 점수
			packet.obj.damage = 0; // 공격력

			XMFLOAT3 position = m_pPlayer->GetPosition();
			XMFLOAT3 direction = m_pPlayer->GetLookVector();
			memcpy(&packet.obj.meta.position, &position, sizeof(XMFLOAT3)); // 현재 위치
			memcpy(&packet.obj.meta.direction, &direction, sizeof(XMFLOAT3)); // 이동 방향

			packet.obj.meta.speed = 5.0f; // 이동 속도
			packet.obj.meta.hp = 100; // 체력

			m_NetworkClient.send_packet((char*)&packet);
		}
	}

private:
	NetworkingClient m_NetworkClient;
};

