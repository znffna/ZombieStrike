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
		exit(1);
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
	case S_C_OBJ_INFO:
	{
		pkt_sc_obj_info* packet = reinterpret_cast<pkt_sc_obj_info*>(recv_p);
		if (g_bNetworkDebugMode) {
			std::string DebugOutput = "S_C_PLAYER_INFO 패킷 수신\n";
			DebugOutput += "position : (" + std::to_string(packet->startposition.x) + ", " + std::to_string(packet->startposition.y) + ", " + std::to_string(packet->startposition.z) + ")\n";
			OutputDebugStringA(DebugOutput.c_str());
		}

		m_pPlayer->SetPosition(packet->startposition.x, packet->startposition.y, packet->startposition.z);
		m_mapGameObjects[packet->id] = m_pPlayer;
		m_pPlayer->SetServerID(packet->id);
		break;
	}
	case S_C_OBJECT_ADD:
	{
		pkt_sc_object_add* packet = reinterpret_cast<pkt_sc_object_add*>(recv_p);
		if (g_bNetworkDebugMode)
		{
			std::string DebugOutput = "S_C_OBJECT_ADD 패킷 수신\n";
			DebugOutput += "position : (" + std::to_string(packet->startposition.x) + ", " + std::to_string(packet->startposition.y) + ", " + std::to_string(packet->startposition.z) + ")\n";
			DebugOutput += "ObjectType : " + std::to_string(packet->obj_type) + "\n";
			//OutputDebugStringA(DebugOutput.c_str());
		}

		switch (packet->obj_type)
		{
		case ObjectType::PLAYER:
		{
			// 플레이어 오브젝트 추가
			//std::shared_ptr<CPlayer> pPlayer = GetPlayer(packet->skin_type); // GetPlayer(skin_type)로 바꿔야 함
			std::shared_ptr<CPlayer> pPlayer = GetPlayer(0); // GetPlayer(skin_type)로 바꿔야 함
			pPlayer->SetPosition(packet->startposition.x, packet->startposition.y, packet->startposition.z);
			m_mapGameObjects[packet->id] = pPlayer;
			pPlayer->SetServerID(packet->id);

			int gun_type = packet->gun_type;
			std::shared_ptr<CGun> pGun = CGun::Create(nullptr, nullptr, nullptr, gun_type);
			pPlayer->SetGun(pGun);
			AddObject(pGun);

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
			std::shared_ptr<CGameObject> pZombie = GetZombie(packet->skin_type);
			pZombie->SetPosition(packet->startposition.x, packet->startposition.y, packet->startposition.z);
			m_mapGameObjects[packet->id] = pZombie;
			pZombie->SetServerID(packet->id);
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
			Fire(std::dynamic_pointer_cast<CPlayer>(m_mapGameObjects[packet->id]));
			break;
		}
		}
		// 게임 오브젝트 추가
		break;
	}
	case S_C_OBJECT_UPDATE:
	{
		pkt_sc_object_update* updatePkt = reinterpret_cast<pkt_sc_object_update*>(recv_p);
		Vec3 position = updatePkt->position;
		Vec3 look = updatePkt->look;
		m_mapGameObjects[updatePkt->id]->SetLook(look.x, look.y, look.z);
		m_mapGameObjects[updatePkt->id]->SetPosition(position.x, position.y, position.z);
		if (auto pRigidBody = m_mapGameObjects[updatePkt->id]->GetComponent<CRigidBody>()) {
			pRigidBody->SetVelocity(updatePkt->velocity.x, updatePkt->velocity.y, updatePkt->velocity.z);
		}
		m_mapGameObjects[updatePkt->id]->SetState(updatePkt->act_type);

	
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
		packet.level = 1; // 레벨
		packet.score = 0; // 점수
		packet.damage = 0; // 공격력

		XMFLOAT3 position = m_pPlayer->GetPosition();
		XMFLOAT3 velocity = m_pPlayer->GetComponent<CRigidBody>()->GetVelocity();
		XMFLOAT3 look = m_pPlayer->GetLookVector();
		memcpy(&packet.position, &position, sizeof(XMFLOAT3)); // 현재 위치
		memcpy(&packet.velocity, &velocity, sizeof(XMFLOAT3)); // 이동 방향
		memcpy(&packet.look, &look, sizeof(XMFLOAT3)); // 이동 방향
		float pitch = m_pPlayer->GetComponent<CCamera>()->GetPitch();
		packet.pitch = pitch; // 피치

		packet.hp = 100; // 체력

		m_NetworkClient.send_packet((char*)&packet);

	}
}

void COnlineScene::SendFirePacket(const FIRE_INFO fireInfo)
{
	//struct pkt_cs_shoot {
	//	PacketHeader header{ sizeof(*this), PKT_TYPE::C_S_SHOOT };
	//	SIZEID id;                      // 누가 쐈는지
	//	SIZE1 GunType;                  // 총 종류
	//	//int hitZombieId;
	//	float bulletPos[3];
	//	float bulletDir[3];
	//};

	pkt_cs_shoot packet{};
	memcpy(&packet.bulletPos, &fireInfo.xmf3Position, sizeof(XMFLOAT3)); // 총알 위치
	memcpy(&packet.bulletDir, &fireInfo.xmf3Velocity, sizeof(XMFLOAT3)); // 총알 방향 * 거리

	m_NetworkClient.send_packet((char*)&packet);
}

FIRE_INFO COnlineScene::Fire(const std::shared_ptr<CPlayer>& pPlayer)
{
	auto fireInfo = CGameScene::Fire(pPlayer);

	if (m_pPlayer == pPlayer) SendFirePacket(fireInfo);

	return fireInfo;
}
