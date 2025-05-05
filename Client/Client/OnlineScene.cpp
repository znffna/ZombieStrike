#include "OnlineScene.h"

bool g_bNetworkDebugMode = true;

COnlineScene::COnlineScene()
{	
	if(g_bNetworkDebugMode)
	{
		std::string debugOutput = "OnlineScene ª˝º∫µ ";
		debugOutput += "m_NetworkClient Address : ";
		debugOutput += std::to_string(reinterpret_cast<uintptr_t>(&m_NetworkClient));
		debugOutput += "\n";
		debugOutput += "m_NetworkClient.Overlapped Address : ";
		debugOutput += std::to_string(reinterpret_cast<uintptr_t>(&m_NetworkClient.recv_over));
		OutputDebugStringA(debugOutput.c_str());
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
	#define MAX_PACKET_PER_FRAME 10

	for (int i = 0; i < MAX_PACKET_PER_FRAME; i++)
	{
		// SleepEx(0, TRUE) : ÔøΩÔøΩ∆ÆÔøΩÔøΩ≈© I/O ÔøΩ›πÔøΩ √≥ÔøΩÔøΩ
		DWORD ret = SleepEx(0, TRUE);
		if (ret == WAIT_IO_COMPLETION) // ÔøΩÒµø±ÔøΩ ÔøΩ€æÔøΩÔøΩÔøΩ ÔøΩÔøΩÔøΩŸ∏ÔøΩ ÔøΩÔøΩÔøΩ ÔøΩﬂ¥ÔøΩ
			break;
	}

	CGameScene::Update(deltaTime);

	// Network Client Update
	if (m_NetworkClient.is_running)
	{
		// Network Client Update
		SendPlayerState();
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
	PKT_TYPE type = recv_p->type; // ∆–≈∂ ≈∏¿‘  

	switch (type) {
	case S_C_PLAYER_INFO:
	{
		pkt_sc_player_info* packet = reinterpret_cast<pkt_sc_player_info*>(recv_p);

		m_pPlayer->SetPosition(packet->fixdata.startposition.x, packet->fixdata.startposition.y, packet->fixdata.startposition.z);
		m_mapGameObjects[packet->id] = m_pPlayer;

		if(g_bNetworkDebugMode){
			std::string DebugOutput = "S_C_PLAYER_INFO ∆–≈∂ ºˆΩ≈\n";
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
			// «√∑π¿ÃæÓ ø¿∫Í¡ß∆Æ √ﬂ∞°
			std::shared_ptr<CGameObject> pPlayer = GetZombie();
			pPlayer->SetPosition(packet->fixdata.startposition.x, packet->fixdata.startposition.y, packet->fixdata.startposition.z);
			m_mapGameObjects[packet->id] = pPlayer;
			//AddObject(pZombie);
			break;
		}
		case ObjectType::ZOMBIE:
		{
			// ¡ª∫Ò ø¿∫Í¡ß∆Æ √ﬂ∞°
			std::shared_ptr<CGameObject> pZombie = GetZombie(packet->fixdata.skin_type);
			pZombie->SetPosition(packet->fixdata.startposition.x, packet->fixdata.startposition.y, packet->fixdata.startposition.z);
			m_mapGameObjects[packet->id] = pZombie;
			AddObject(pZombie);
			break;
		}
		case ObjectType::BULLET:
		{
			// √—æÀ ø¿∫Í¡ß∆Æ √ﬂ∞°
			/*std::shared_ptr<CGameObject> pBullet = std::make_shared<CBulletObject>();
			pBullet->SetPosition(packet->fixdata.startposition.x, packet->fixdata.startposition.y, packet->fixdata.startposition.z);
			m_mapGameObjects[packet->id] = pBullet;*/
			break;
		}
		}
		// ∞‘¿” ø¿∫Í¡ß∆Æ √ﬂ∞°
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
		packet.obj.level = 1; // ∑π∫ß
		packet.obj.score = 0; // ¡°ºˆ
		packet.obj.damage = 0; // ∞¯∞›∑¬

		XMFLOAT3 position = m_pPlayer->GetPosition();
		XMFLOAT3 direction = m_pPlayer->GetComponent<CRigidBody>()->GetVelocity();
		memcpy(&packet.obj.meta.position, &position, sizeof(XMFLOAT3)); // «ˆ¿Á ¿ßƒ°
		memcpy(&packet.obj.meta.direction, &direction, sizeof(XMFLOAT3)); // ¿Ãµø πÊ«‚

		packet.obj.meta.speed = 5.0f; // ¿Ãµø º”µµ
		packet.obj.meta.hp = 100; // √º∑¬

		m_NetworkClient.send_packet((char*)&packet);
	}
}
