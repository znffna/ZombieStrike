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

	void SendReloadStart();		// 리로드 시작 패킷 전송 
	void SendReloadFinish();	// 리로드 종료 패킷 전송 

private:
	std::unordered_map< int, CGameObject* > m_mapGameObjects;
	std::unordered_map< int, ObjectType > m_mapObjectTypes; // - id별 obj_type 저장
	std::unordered_map<SIZEID, SIZE1> m_prevZombieAct;	// 좀비 이전 act_type 저장

	// LoadingFinish 패킷 1회 전송 보장용
	bool m_sentLoadingFinish = false;

	// Stage/Score UI용 상태값(서버 스냅샷)
	int m_currentStage = 1;
	int m_totalStages = 1;
	int m_timeLeftMs = 0;

	int m_stageScore = 0;
	int m_totalZombies = 0;
	int m_killedZombies = 0;
	int m_aliveZombies = 0;
	

	// S_C_SCORE_INFO 웨이브 스냅샷(프로토콜 업데이트 반영)
	int m_currentWave = 1;
	int m_totalWaves = 1;
	int m_waveTotalZombies = 0;
	int m_waveKilledZombies = 0;
	int m_waveAliveZombies = 0;


	// 클리어 중복 처리 방지
	bool m_stageCleared = false;

	// 탄/리로드 UI용(서버 스냅샷)
	int  m_ammoCur = 0;
	int  m_ammoMax = 0;
	bool m_isReloading = false;

	char m_nMoveInput;
};

