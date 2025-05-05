#include "OnlineScene.h"

bool g_bNetworkDebugMode = true;

COnlineScene::COnlineScene()
{	
	if(g_bNetworkDebugMode)
	{
		std::string debugOutput = "\nOnlineScene 생성됨";
		debugOutput += "m_NetworkClient Address : ";
		debugOutput += std::to_string(reinterpret_cast<uintptr_t>(&m_NetworkClient));
		debugOutput += "\n";
		debugOutput += "m_NetworkClient.Overlapped Address : ";
		debugOutput += std::to_string(reinterpret_cast<uintptr_t>(&m_NetworkClient.recv_over)) + "\n";
		OutputDebugStringA(debugOutput.c_str());

		if (reinterpret_cast<uintptr_t>(&m_NetworkClient) == reinterpret_cast<uintptr_t>(&m_NetworkClient.recv_over)) {
			OutputDebugStringA("m_NetworkClient과 m_NetworkClient.recv_over의 주소가 같습니다.\n");
		}
		else
		{
			OutputDebugStringA("m_NetworkClient과 m_NetworkClient.recv_over의 주소가 다릅니다.\n");
		}
	}
}

COnlineScene::~COnlineScene()
{
}

void COnlineScene::InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	CGameScene::InitializeObjects(pd3dDevice, pd3dCommandList, pd3dRootSignature);

	m_NetworkClient.Connect();
}

void COnlineScene::ReleaseObjects()
{
	m_NetworkClient.Logout();
}


void COnlineScene::ReleaseUploadBuffers()
{
}

void COnlineScene::Update(float deltaTime)
{
	m_NetworkClient.startRecvLoop();

	#define MAX_PACKET_PER_FRAME 10

	// 얼만큼 반복하나 찍어보자.
	int count{ 0 };
	for (int i = 0; i < MAX_PACKET_PER_FRAME; i++)
	{
		// SleepEx(0, TRUE) : 네트워크 I/O 콜백 처리
		DWORD ret = SleepEx(0, TRUE);
		//DWORD ret = SleepEx(0, TRUE);
		if (ret != WAIT_IO_COMPLETION) // 비동기 작업이 없다면 즉시 중단
			break;
		++count;
	}

	if (g_bNetworkDebugMode)
	{
		std::string debugOutput = "COnlineScene::Update() - SleepEx() 을 통한 I/O 횟수 : " + std::to_string(count) + "\n";
		OutputDebugStringA(debugOutput.c_str());
	}

	CGameScene::Update(deltaTime);

	// Network Client Update
	if (m_NetworkClient.is_running)
	{
		// Network Client Update
		// 즉 클라처리 결과를 서버에 보고
		// SendPlayerState();
	}
}

bool COnlineScene::ProcessInput(const INPUT_PARAMETER& pBuffer, float deltaTime)
{
	DWORD dwDirection = 0;
	if (pBuffer.pKeysBuffer[VK_UP] & 0xF0)dwDirection |= DIR_FORWARD;
	if (pBuffer.pKeysBuffer[VK_DOWN] & 0xF0)dwDirection |= DIR_BACKWARD;
	if (pBuffer.pKeysBuffer[VK_LEFT] & 0xF0)dwDirection |= DIR_LEFT;
	if (pBuffer.pKeysBuffer[VK_RIGHT] & 0xF0)dwDirection |= DIR_RIGHT;
	if (pBuffer.pKeysBuffer[VK_PRIOR] & 0xF0)dwDirection |= DIR_UP;
	if (pBuffer.pKeysBuffer[VK_NEXT] & 0xF0)dwDirection |= DIR_DOWN;

	if (dwDirection || pBuffer.cxDelta != 0.0f || pBuffer.cyDelta != 0.0f)
	{
		if (m_pPlayer)
		{
			m_pPlayer->Rotate(pBuffer.cyDelta, pBuffer.cxDelta, 0.0f);
			m_pPlayer->Move(dwDirection, 10.0f, deltaTime);
		}

		/*if (m_pCamera)
		{
			m_pCamera->Rotate(pBuffer.cyDelta, pBuffer.cxDelta, 0.0f);
			m_pCamera->RegenerateViewMatrix();
		}*/
	}

	return true;
}

void COnlineScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
}

void COnlineScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
}

void COnlineScene::ProcessPacket(PacketHeader* recv_p)
{	
	PKT_TYPE type = recv_p->type; // 패킷 타입  

	switch (type) {
	case S_C_PLAYER_INFO:
	{
		pkt_sc_player_info* packet = reinterpret_cast<pkt_sc_player_info*>(recv_p);

		m_pPlayer->SetPosition(packet->fixdata.startposition.x, packet->fixdata.startposition.y, packet->fixdata.startposition.z);
		m_mapGameObjects[packet->id] = m_pPlayer;

		if(g_bNetworkDebugMode){
			std::string DebugOutput = "S_C_PLAYER_INFO 패킷 수신\n";
			DebugOutput += "position : (" + std::to_string(packet->fixdata.startposition.x) + ", " + std::to_string(packet->fixdata.startposition.y) + ", " + std::to_string(packet->fixdata.startposition.z) + ")\n";
			OutputDebugStringA(DebugOutput.c_str());
		}
		break;
	}
	case S_C_OBJECT_ADD:
	{
		pkt_sc_object_add* packet = reinterpret_cast<pkt_sc_object_add*>(recv_p);
		switch (packet->fixdata.obj_type)
		{
		case ObjectType::PLAYER:
		{
			// 플레이어 오브젝트 추가
			std::shared_ptr<CGameObject> pPlayer = GetZombie();
			pPlayer->SetPosition(packet->fixdata.startposition.x, packet->fixdata.startposition.y, packet->fixdata.startposition.z);
			m_mapGameObjects[packet->id] = pPlayer;
			//AddObject(pZombie);
			break;
		}
		case ObjectType::ZOMBIE:
		{
			// 좀비 오브젝트 추가
			std::shared_ptr<CGameObject> pZombie = GetZombie(packet->fixdata.skin_type);
			pZombie->SetPosition(packet->fixdata.startposition.x, packet->fixdata.startposition.y, packet->fixdata.startposition.z);
			m_mapGameObjects[packet->id] = pZombie;
			AddObject(pZombie);
			{
				std::string DebugOutput = "S_C_OBJECT_ADD 패킷 수신 => Zombie 생성\n";
				DebugOutput += "position : (" + std::to_string(packet->fixdata.startposition.x) + ", " + std::to_string(packet->fixdata.startposition.y) + ", " + std::to_string(packet->fixdata.startposition.z) + ")\n";
				OutputDebugStringA(DebugOutput.c_str());
			}

			break;
		}
		case ObjectType::BULLET:
		{
			// 총알 오브젝트 추가
			/*std::shared_ptr<CGameObject> pBullet = std::make_shared<CBulletObject>();
			pBullet->SetPosition(packet->fixdata.startposition.x, packet->fixdata.startposition.y, packet->fixdata.startposition.z);
			m_mapGameObjects[packet->id] = pBullet;*/
			break;
		}
		}
		// 게임 오브젝트 추가
		break;
	}
	case S_C_OBJECT_UPDATE:
	{
		pkt_sc_object_update* updatePkt = reinterpret_cast<pkt_sc_object_update*>(recv_p);
		Vec3 position = updatePkt->obj.meta.position;
		m_mapGameObjects[updatePkt->id]->SetPosition(position.x, position.y, position.z);
		if (g_bNetworkDebugMode) {
			std::string DebugOutput = "S_C_OBJECT_UPDATE[" + std::to_string(updatePkt->id) + "] ";
			DebugOutput += "position : (" + std::to_string(position.x) + ", " + std::to_string(position.y) + ", " + std::to_string(position.z) + ")\n";
			OutputDebugStringA(DebugOutput.c_str());
		}
		break;
	}
	case S_C_OBJECT_REMOVE:
	{
		pkt_sc_object_remove* removePkt = reinterpret_cast<pkt_sc_object_remove*>(recv_p);
		break;
	}

	case S_C_STAGE_INFO:
	{
		break;
	}
	case S_C_SCORE_INFO:
	{
		break;

	}
	default:
		break;
	}
}

void COnlineScene::SendPlayerState()
{
	if (m_pPlayer)
	{
		pkt_cs_update packet{};
		packet.header.size = sizeof(packet);
		packet.header.type = PKT_TYPE::C_S_UPDATE;
		packet.obj.level = 1; // 레벨
		packet.obj.score = 0; // 점수
		packet.obj.damage = 0; // 공격력

		XMFLOAT3 position = m_pPlayer->GetPosition();
		XMFLOAT3 direction = m_pPlayer->GetComponent<CRigidBody>()->GetVelocity();
		memcpy(&packet.obj.meta.position, &position, sizeof(XMFLOAT3)); // 현재 위치
		memcpy(&packet.obj.meta.direction, &direction, sizeof(XMFLOAT3)); // 이동 방향

		packet.obj.meta.speed = 5.0f; // 이동 속도
		packet.obj.meta.hp = 100; // 체력

		m_NetworkClient.send_packet((char*)&packet);
	}
}
