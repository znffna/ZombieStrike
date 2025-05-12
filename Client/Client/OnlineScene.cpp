#include "OnlineScene.h"


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
	bool isComplelte = m_NetworkClient.Connect();
	if (!isComplelte)
	{
		std::string debugOutput = "COnlineScene::InitializeObjects() - NetworkClient Connect 실패\n";
		OutputDebugStringA(debugOutput.c_str());
		return;
	}

	CGameScene::InitializeObjects(pd3dDevice, pd3dCommandList, pd3dRootSignature);
}

void COnlineScene::PostInitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	if (false == m_NetworkClient.IsConnect())
	{
		SetSceneState(SCENE_STATE_ENDING);
		return;
	}
	SetSceneState(SCENE_STATE_READY_TO_START);
}

void COnlineScene::ReleaseObjects()
{
	m_NetworkClient.Logout();
}


void COnlineScene::ReleaseUploadBuffers()
{
}

void COnlineScene::StartScene()
{ 
	{
		std::string debugOutput = "COnlineScene::StartScene() - StartRecvLoop() 호출됨\n";
		OutputDebugStringA(debugOutput.c_str());
	}
	m_NetworkClient.StartRecvLoop();

	CScene::StartScene();
}

void COnlineScene::Update(float deltaTime)
{
	#define MAX_PACKET_PER_FRAME 3

	// 얼만큼 반복하나 찍어보자.
	int count{ 0 };
	for (int i = 0; i < MAX_PACKET_PER_FRAME; ++i)
	{
		// SleepEx(0, TRUE) : 네트워크 I/O 콜백 처리
		DWORD ret = SleepEx(0, TRUE);
		if (ret != WAIT_IO_COMPLETION) // 비동기 작업이 없다면 즉시 중단
			break;
		++count;
	}
	//SleepEx(0, TRUE); // 네트워크 I/O 콜백 처리

	CGameScene::Update(deltaTime);

	// Network Client Update
	if (m_NetworkClient.IsConnect())
	{
		// Network Client Update
		// 즉 클라처리 결과를 서버에 보고
		SendPlayerState();
	}
}

bool COnlineScene::ProcessInput(const INPUT_PARAMETER& pBuffer, float deltaTime)
{
	CGameScene::ProcessInput(pBuffer, deltaTime);

	if (pBuffer.pKeysBuffer[VK_ESCAPE] & 0xF0)
	{
		SetSceneState(SCENE_STATE_ENDING);
	}

	if (pBuffer.pKeysBuffer[VK_F5] & 0xF0)
	{
		ChangeMap(0);
	}
	else if (pBuffer.pKeysBuffer[VK_F6] & 0xF0)
	{
		ChangeMap(1);
	}
	else if (pBuffer.pKeysBuffer[VK_F7] & 0xF0)
	{
		ChangeMap(2);
	}

	return true;
}

void COnlineScene::ProcessPacket(PacketHeader* recv_p)
{	
	PKT_TYPE type = recv_p->type; // 패킷 타입  

	//if (g_bNetworkDebugMode) 
	{
		std::string DebugOutput = "COnlineScene::ProcessPacket() - Packet Type : " + std::to_string(type) + "\n";
		//OutputDebugStringA(DebugOutput.c_str());
	}

	switch (type) {
	case S_C_PLAYER_INFO:
	{
		pkt_sc_player_info* packet = reinterpret_cast<pkt_sc_player_info*>(recv_p);
		if (g_bNetworkDebugMode) {
			std::string DebugOutput = "S_C_PLAYER_INFO 패킷 수신\n";
			DebugOutput += "position : (" + std::to_string(packet->fixdata.startposition.x) + ", " + std::to_string(packet->fixdata.startposition.y) + ", " + std::to_string(packet->fixdata.startposition.z) + ")\n";
			OutputDebugStringA(DebugOutput.c_str());
		}

		m_pPlayer->SetPosition(packet->fixdata.startposition.x, packet->fixdata.startposition.y, packet->fixdata.startposition.z);
		m_mapGameObjects[packet->id] = m_pPlayer;
		break;
	}
	case S_C_OBJECT_ADD:
	{
		pkt_sc_object_add* packet = reinterpret_cast<pkt_sc_object_add*>(recv_p);
		//if (g_bNetworkDebugMode)
		{
			std::string DebugOutput = "S_C_OBJECT_ADD 패킷 수신\n";
			DebugOutput += "position : (" + std::to_string(packet->fixdata.startposition.x) + ", " + std::to_string(packet->fixdata.startposition.y) + ", " + std::to_string(packet->fixdata.startposition.z) + ")\n";
			DebugOutput += "ObjectType : " + std::to_string(packet->fixdata.obj_type) + "\n";
			//OutputDebugStringA(DebugOutput.c_str());
		}

		switch (packet->fixdata.obj_type)
		{
		case ObjectType::PLAYER:
		{
			// 플레이어 오브젝트 추가
			std::shared_ptr<CPlayer> pPlayer = GetPlayer(); // GetPlayer(skin_type)로 바꿔야 함
			pPlayer->SetPosition(packet->fixdata.startposition.x, packet->fixdata.startposition.y, packet->fixdata.startposition.z);
			m_mapGameObjects[packet->id] = pPlayer;

			int gun_type = packet->fixdata.gun_type;
			std::shared_ptr<CGun> pGun = CGun::Create(nullptr, nullptr, nullptr, gun_type);
			pPlayer->SetGun(pGun);

			{
				std::string DebugOutput = "ObjectType::PLAYER 생성 완료\n";
				//OutputDebugStringA(DebugOutput.c_str());
			}
			AddObject(pPlayer);
			break;
		}
		case ObjectType::ZOMBIE:
		{
			// 좀비 오브젝트 추가
			std::shared_ptr<CGameObject> pZombie = GetZombie(packet->fixdata.skin_type);
			pZombie->SetPosition(packet->fixdata.startposition.x, packet->fixdata.startposition.y, packet->fixdata.startposition.z);
			m_mapGameObjects[packet->id] = pZombie;
			{
				std::string DebugOutput = "ObjectType::ZOMBIE 생성 완료\n";
				//OutputDebugStringA(DebugOutput.c_str());
			}
			AddObject(pZombie);
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

		//std::string DebugOutput = "COnlineScene::SendPlayerState() - Player State 전송\n";
		//DebugOutput += "position : (" + std::to_string(position.x) + ", " + std::to_string(position.y) + ", " + std::to_string(position.z) + ")\n";
		//OutputDebugStringA(DebugOutput.c_str());
	}
}
