#include "OnlineScene.h"


COnlineScene::COnlineScene()
{	
}

COnlineScene::~COnlineScene()
{
	NetworkingClient::Instance().Logout();
}

void COnlineScene::InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	CGameScene::InitializeObjects(pd3dDevice, pd3dCommandList, pd3dRootSignature);
}

void COnlineScene::PostInitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	NetworkingClient::Instance().StartRecvLoop();
}

void COnlineScene::ReleaseUploadBuffers()
{
}

void COnlineScene::StartScene()
{ 
	CScene::StartScene();
}

#include "GameFramework.h"
void COnlineScene::Update(float deltaTime)
{
	ProcessReadQueuePacket();

	CGameScene::Update(deltaTime);

	// Network Client Update
	if (NetworkingClient::Instance().IsRunning())
	{
		// Network Client Update
		// 즉 클라처리 결과를 서버에 보고
		SendPlayerState();
	}
	else {
		PopScene();
	}
}

bool COnlineScene::ProcessInput(const INPUT_PARAMETER& pBuffer, float deltaTime)
{
	CGameScene::ProcessInput(pBuffer, deltaTime);

	if (pBuffer.pKeysBuffer[VK_ESCAPE] & 0xF0)
	{
		PopScene();
		return true;
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

void COnlineScene::ProcessReadQueuePacket()
{
	auto& readQueue = NetworkingClient::Instance().GetReadQueue();

	if (false == CheckWorkUpdating()) return;

	for (auto& packet : readQueue) {
		ProcessPacket(packet.header());
	}
}

void COnlineScene::ProcessPacket(PacketHeader* recv_p)
{	
	PKT_TYPE type = recv_p->type; // 패킷 타입  

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
		float fPitch = updatePkt->pitch;
		if(auto pCamera = m_mapGameObjects[updatePkt->id]->GetComponent<CCamera>()) pCamera->SetPitch(fPitch);
	
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

		NetworkingClient::Instance().send_packet((char*)&packet);

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

	NetworkingClient::Instance().send_packet((char*)&packet);
}

FIRE_INFO COnlineScene::Fire(const std::shared_ptr<CPlayer>& pPlayer)
{
	auto fireInfo = CGameScene::Fire(pPlayer);

	if (fireInfo.fRange <= 0.0f) return fireInfo; // 총알이 발사되지 않은 경우

	if (m_pPlayer == pPlayer) SendFirePacket(fireInfo);

	return fireInfo;
}
