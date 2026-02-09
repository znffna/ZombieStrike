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
}

#include "GameFramework.h"
void COnlineScene::Update(float deltaTime)
{
	ProcessReadQueuePacket();

	CGameScene::Update(deltaTime);

	if (m_pScoreInfo)
	{
		auto pTextComp = m_pScoreInfo->GetComponent<CTextComponent>();
		if (pTextComp)
		{
			// 웨이브 정보
			std::wstring waveInfo = L"Wave: " + std::to_wstring(m_currentWave) + L"/" + std::to_wstring(m_totalWaves);

			// 좀비 정보 (현재 웨이브)
			std::wstring zombieInfo = L"  Zombies: " + std::to_wstring(m_waveKilledZombies) + L"/" + std::to_wstring(m_waveTotalZombies);

			// 점수 정보
			std::wstring scoreInfo = L"  Score: " + std::to_wstring(m_stageScore);

			// 탄약 정보
			std::wstring ammoInfo = L"  Ammo: " + std::to_wstring(m_ammoCur) + L"/" + std::to_wstring(m_ammoMax);
			if (m_isReloading) {
				ammoInfo += L" (Reloading...)";
			}

			// 체력 정보
			std::wstring hpInfo = L"";
			if (m_pPlayer) {
				hpInfo = L"  HP: " + std::to_wstring(static_cast<int>(m_pPlayer->GetHealth()));
			}

			// 스테이지 정보
			std::wstring stageInfo = L"  Stage: " + std::to_wstring(m_currentStage) + L"/" + std::to_wstring(m_totalStages);

			// 전체 텍스트 조합
			std::wstring fullText = waveInfo + zombieInfo + scoreInfo + ammoInfo + hpInfo + stageInfo;

			// 클리어 상태 표시
			if (m_stageCleared) {
				fullText += L"\n*** STAGE CLEAR ***";
			}

			pTextComp->SetText(fullText);
		}
	}

	if (m_pAmmoInfo)
	{
		auto pTextComp = m_pAmmoInfo->GetComponent<CTextComponent>();
		if (pTextComp)
		{
			// 탄약 정보
			std::wstring ammoInfo = L"  Ammo: " + std::to_wstring(m_ammoCur) + L"/" + std::to_wstring(m_ammoMax);
			if (m_isReloading) {
				//ammoInfo += L" (Reloading...)";
			}

			// 스테이지 정보
			std::wstring stageInfo = L"  Stage: " + std::to_wstring(m_currentStage) + L"/" + std::to_wstring(m_totalStages);

			// 전체 텍스트 조합
			std::wstring fullText = ammoInfo;

			pTextComp->SetText(fullText);
		}
	}

	// Network Client Update
	if (NetworkingClient::Instance().IsRunning())
	{
		// - 로딩 완료 전엔 상태 전송 금지
		if (!m_sentLoadingFinish)
			return;

		// 즉 클라처리 결과를 서버에 보고
		SendPlayerState();
	}
	else {
		PopScene();
	}
}

bool COnlineScene::ProcessMouseInput(float cxDelta, float cyDelta, float deltaTime)
{
	if (NetworkingClient::Instance().IsRunning() == false)
	{
		PopScene();
		return true;
	}

	CGameScene::ProcessMouseInput(cxDelta, cyDelta, deltaTime);
	return false;
}

bool COnlineScene::ProcessKeyboardInput(const UCHAR pKeysBuffer[256], float deltaTime)
{
	CGameScene::ProcessKeyboardInput(pKeysBuffer, deltaTime);


	return false;
}

void COnlineScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	CGameScene::OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);

	switch (nMessageID)
	{
		case WM_KEYDOWN:
		{
			switch (wParam)
			{
			case VK_F4:
				StageClear();
				break;
			case VK_F5:
			case VK_F6:
			case VK_F7:
				// ChangeMap(wParam - VK_F5);
				break;

			case 'R':
			{
				SendReloadStart();	//리로드 시작 요청
				break;
			}

			default:
				break;
			}
			break;
		}
		break;
	}
}

void COnlineScene::ProcessReadQueuePacket()
{
	auto& readQueue = NetworkingClient::Instance().GetReadQueue();

	if (false == IsSceneRunning()) return;

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
		m_pPlayer->SetSID(packet->id);
		m_pPlayer->SetSkin(packet->skin_type);
		m_mapGameObjects[packet->id] = m_pPlayer;
		m_mapObjectTypes[packet->id] = ObjectType::PLAYER;

		// // [COnlineScene::ProcessPacket] - S_C_OBJ_INFO 수신 직후 로딩 완료 신호(1회)
		if (!m_sentLoadingFinish) {
			m_sentLoadingFinish = true;
			NetworkingClient::Instance().SendLoadingFinishPacket();
		}

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
			if (m_pPlayer->GetSID() != 0 && packet->id == m_pPlayer->GetSID())
			{
				m_pPlayer->SetPosition(packet->startposition.x, packet->startposition.y, packet->startposition.z);
				m_pPlayer->SetSkin(packet->skin_type); // // S_C_OBJECT_ADD - 스킨 적용(모델 교체 보장)
				m_mapGameObjects[packet->id] = m_pPlayer;
				m_mapObjectTypes[packet->id] = ObjectType::PLAYER;
				m_pPlayer->SetHealthUIPostion(0);
				break;
			}
			auto itExist = m_mapGameObjects.find(packet->id);
			if (itExist != m_mapGameObjects.end())
			{
				CGameObject* existObj = itExist->second;
				CPlayer* existPlayer = (CPlayer*)existObj; // // S_C_OBJECT_ADD - 기존 플레이어 갱신
				existPlayer->SetPosition(packet->startposition.x, packet->startposition.y, packet->startposition.z);
				existPlayer->SetSkin(packet->skin_type);
				m_mapObjectTypes[packet->id] = ObjectType::PLAYER;
				break;
			}
			// 플레이어 오브젝트 추가
			//std::shared_ptr<CPlayer> pPlayer = GetPlayer(packet->skin_type); // GetPlayer(skin_type)로 바꿔야 함
			//std::shared_ptr<CPlayer> pPlayer = GetPlayer(0); // GetPlayer(skin_type)로 바꿔야 함

			auto pPlayer = SpawnPlayer(
				XMFLOAT3{packet->startposition.x, packet->startposition.y, packet->startposition.z },
				std::string(packet->name),
				packet->skin_type,
				packet->starthp,
				packet->act_type,
				packet->move_input,
				m_pTerrain);

			//auto pPlayer = RequestCreateObject(TypeTag<CPlayer>(), packet->skin_type);
			//pPlayer->SetPosition(packet->startposition.x, packet->startposition.y, packet->startposition.z);
			pPlayer->SetSID(packet->id);

			pPlayer->SetSkin(packet->skin_type);
			pPlayer->SetHealthUIPostion(pPlayer->GetID() % 3 + 1); // 다른 플레이어는 [1, 3]번 슬롯에 체력바 출력

			m_mapGameObjects[packet->id] = pPlayer;
			m_mapObjectTypes[packet->id] = ObjectType::PLAYER;

			auto pGun = RequestCreateObject(TypeTag<CGun>(), packet->gun_type);
			pPlayer->SetGun(pGun);

			//int gun_type = packet->gun_type;
			//std::shared_ptr<CGun> pGun = CGun::Create(nullptr, nullptr, nullptr, gun_type);
			//pPlayer->SetGun(pGun);
			//AddObject(pGun);

			//{
			//	std::string DebugOutput = "ObjectType::PLAYER 생성 완료\n";
			//}
			//AddObject(pPlayer);
			break;
		}
		case ObjectType::ZOMBIE:
		{
			auto itExist = m_mapGameObjects.find(packet->id);
			if (itExist != m_mapGameObjects.end())
			{
				itExist->second->SetPosition(packet->startposition.x, packet->startposition.y, packet->startposition.z);
				itExist->second->SetSID(packet->id);
				m_mapObjectTypes[packet->id] = ObjectType::ZOMBIE;
				break;
			}

			// 좀비 오브젝트 추가
			//std::shared_ptr<CGameObject> pZombie = GetZombie(packet->skin_type);
			auto pZombie = RequestCreateObject(TypeTag<CZombieObject>(), packet->skin_type);
			pZombie->SetPosition(packet->startposition.x, packet->startposition.y, packet->startposition.z);
			m_mapGameObjects[packet->id] = pZombie;
			pZombie->SetSID(packet->id);
			{
				std::string DebugOutput = "ObjectType::ZOMBIE 생성 완료\n";
				//OutputDebugStringA(DebugOutput.c_str());
			}
			m_mapObjectTypes[packet->id] = ObjectType::ZOMBIE;

			//AddObject(pZombie);
			break;
		}
		case ObjectType::BULLET:
		{
			// 총알 오브젝트 추가
			/*std::shared_ptr<CGameObject> pBullet = std::make_shared<CBulletParticleObject>();
			pBullet->SetPosition(packet->fixdata.startposition.x, packet->fixdata.startposition.y, packet->fixdata.startposition.z);
			m_mapGameObjects[packet->id] = pBullet;
			*/
			//auto pPlayer = std::dynamic_pointer_cast<CPlayer>(m_mapGameObjects[packet->id]);
			//if(pPlayer) Fire(pPlayer);
			break;
		}
		}
		// 게임 오브젝트 추가
		break;
	}
	case S_C_OBJECT_UPDATE:
	{
		pkt_sc_object_update* updatePkt = reinterpret_cast<pkt_sc_object_update*>(recv_p);

		auto it = m_mapGameObjects.find(updatePkt->id);
		if (it == m_mapGameObjects.end()) break; // - 없는 ID 방어

		CGameObject* obj = it->second;

		const int mySid = (m_pPlayer) ? m_pPlayer->GetSID() : 0;
		const bool isLocalPlayer = (mySid != 0 && updatePkt->id == mySid);

		if (!isLocalPlayer)
		{
			Vec3 position = updatePkt->position;
			Vec3 look = updatePkt->look;

			obj->SetLook(look.x, look.y, look.z);
			obj->SetPosition(position.x, position.y, position.z);

			if (auto pRigidBody = obj->GetComponent<CRigidBody>()) {
				pRigidBody->SetVelocity(updatePkt->velocity.x, updatePkt->velocity.y, updatePkt->velocity.z);
			}

			float fPitch = updatePkt->pitch;
			if (auto pCamera = obj->GetComponent<CCamera>()) pCamera->SetPitch(fPitch);
		}
		else
		{
			// 로컬은 Transform은 클라 권위(예측/입력) 유지
			// HP/피격 표현은 아래 타입 처리에서 따로 다룸
		}

		auto tit = m_mapObjectTypes.find(updatePkt->id);
		if (tit == m_mapObjectTypes.end()) {
			if (!isLocalPlayer) obj->SetState(updatePkt->act_type); 
			break;
		}

		ObjectType type = tit->second;

		if (type == ObjectType::PLAYER)
		{
			CPlayer* pPlayer = (CPlayer*)obj;

			//pPlayer->SetHealth((float)updatePkt->hp);

			if (!isLocalPlayer)
			{
				pPlayer->SetState(updatePkt->act_type);
				pPlayer->SetMoveInput(updatePkt->move_input);
			}
			else
			{
				// 로컬은 여기서 SetState를 호출하면 “HIT 고정” 같은 문제가 생길 수 있음
				// 필요하면 HIT만 1회 트리거로 처리(추가 구현)
				// if (updatePkt->act_type == ActionType::HIT) pPlayer->TriggerHitOnce();
			}
		}
		else if (type == ObjectType::ZOMBIE)
		{
			CZombieObject* pZombie = (CZombieObject*)obj;

			// 좀비 act_type 변경 감지(사운드 1회 트리거)
			const SIZEID zid = updatePkt->id;
			const SIZE1  newAct = (SIZE1)updatePkt->act_type;

			SIZE1 oldAct = 0xFF;
			auto itPrev = m_prevZombieAct.find(zid);
			if (itPrev != m_prevZombieAct.end()) oldAct = itPrev->second;

			const bool actChanged = (oldAct != newAct);
			if (actChanged) m_prevZombieAct[zid] = newAct;

			// 애니 변경은 기존대로 계속 적용
			switch (updatePkt->act_type)
			{
			case ActionType::ZMOVE:  pZombie->SetState((int)ZOMBIE_ANIMATION_POSE::ZOMBIE_RUNNING); break;
			case ActionType::ATTACK: pZombie->SetState((int)ZOMBIE_ANIMATION_POSE::ZOMBIE_ATTACK);  break;
			case ActionType::DEATH:  pZombie->SetState((int)ZOMBIE_ANIMATION_POSE::ZOMBIE_DEATH);   break;
			case ActionType::SCREAM: pZombie->SetState((int)ZOMBIE_ANIMATION_POSE::ZOMBIE_SCREAM);  break;
			case ActionType::HIT:    pZombie->SetState((int)ZOMBIE_ANIMATION_POSE::ZOMBIE_HIT);     break;
			default:                 pZombie->SetState((int)ZOMBIE_ANIMATION_POSE::ZOMBIE_IDLE);    break;
			}

			// 사운드는 "상태가 바뀌는 순간"에만 1회 재생
			if (actChanged)
			{
				if (updatePkt->act_type == ActionType::ATTACK) {
					pZombie->PlayBiteSfx(); // 좀비 무는소리
				}
				else if (updatePkt->act_type == ActionType::SCREAM) {
					pZombie->PlayCrySfx();  // 좀비 우는소리
				}
			}
		}
		else
		{
			if (!isLocalPlayer) obj->SetState(updatePkt->act_type); 
		}

		break;
	}
	
	case S_C_AMMO_INFO:
	{
		auto* p = reinterpret_cast<pkt_sc_ammo_info*>(recv_p);

		// 서버 탄/리로드 스냅샷 적용
		m_ammoCur = p->cur_ammo;
		m_ammoMax = p->max_ammo;
		m_isReloading = (p->reloading != 0);

		// TODO: 여기서 UI 갱신(탄 수 표시/리로드 표시)
		m_pPlayer->SetAmmo(m_ammoCur, m_ammoMax);
		// 예) HUD->SetAmmo(m_ammoCur, m_ammoMax, m_isReloading);

		break;
	}

	case S_C_OBJECT_REMOVE:
	{
		pkt_sc_object_remove* removePkt = reinterpret_cast<pkt_sc_object_remove*>(recv_p);

		m_prevZombieAct.erase(removePkt->id);

		auto it = m_mapGameObjects.find(removePkt->id);
		if (it != m_mapGameObjects.end())
		{
			CGameObject* obj = it->second;                         // obj 로컬로 받기
			if (obj) { RequestDestroyObject(obj->GetID()); }       // nullptr 방어
			m_mapGameObjects.erase(it);
		}
		
		m_mapObjectTypes.erase(removePkt->id);
		break;
	}

	case S_C_STAGE_INFO:
	{
		auto* packet = reinterpret_cast<pkt_sc_stage_info*>(recv_p);

		// 서버 Stage 스냅샷 저장
		m_currentStage = packet->currentStage;
		m_totalStages = packet->totalStages;
		m_timeLeftMs = packet->timeLeft;

		// timeLeft==0이면 Stage Clear(서버 권위 보조 트리거)
		if (!m_stageCleared && m_timeLeftMs <= 0)
		{
			StageClear();

			// TODO: 여기서 클리어 UI / 씬 전환 / 입력 잠금
			// 예) ShowStageClearUI();
			// 예) PushScene(ResultScene);
		}

		break;
	}
	case S_C_SCORE_INFO:
	{
		auto* packet = reinterpret_cast<pkt_sc_score_info*>(recv_p);

		// 기존 Score 스냅샷 저장
		m_stageScore = packet->stage_score;
		m_totalZombies = packet->total_zombies;
		m_killedZombies = packet->killed_zombies;
		m_aliveZombies = packet->alive_zombies;

		// 웨이브 필드 스냅샷 저장(프로토콜 업데이트 반영)
		m_currentWave = packet->current_wave;
		m_totalWaves = packet->total_waves;
		m_waveTotalZombies = packet->wave_total_zombies;
		m_waveKilledZombies = packet->wave_killed_zombies;
		m_waveAliveZombies = packet->wave_alive_zombies;

		// 디버그(
		/*if (g_bNetworkDebugMode) {
			std::string msg;
			msg += "[S_C_SCORE_INFO] score=" + std::to_string(m_stageScore);
			msg += " wave=" + std::to_string(m_currentWave) + "/" + std::to_string(m_totalWaves);
			msg += " alive=" + std::to_string(m_waveAliveZombies) + "/" + std::to_string(m_waveTotalZombies);
			msg += "\n";
			OutputDebugStringA(msg.c_str());
		}*/

		// 클리어 판정: "마지막 웨이브 + waveAlive==0" 기준
		if (!m_stageCleared) {
			const bool isLastWave = (m_currentWave >= m_totalWaves);
			if (isLastWave && m_waveTotalZombies > 0 && m_waveAliveZombies <= 0) {
				StageClear();
			}
		}

		break;
	}

	case PKT_TYPE::S_C_PLAYER_HP_ONLY:
	{
		auto* p = reinterpret_cast<pkt_sc_player_hp_only*>(recv_p);

		auto itObj = m_mapGameObjects.find(p->id);
		if (itObj == m_mapGameObjects.end()) break;

		auto itType = m_mapObjectTypes.find(p->id);
		if (itType == m_mapObjectTypes.end()) break;

		if (itType->second != ObjectType::PLAYER) break;

		CPlayer* pl = static_cast<CPlayer*>(itObj->second);
		pl->SetHealth((float)p->hp); // HP만 갱신

		break;
	}



	default:
		break;
	}
}

void COnlineScene::StageClear()
{
	if (m_stageCleared) return;
	m_stageCleared = true;
	CGameFramework::Instance()->RequestSceneChange(CPushScene{ TypeTag<CVictoryScene>{} });
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
		packet.move_input = m_pPlayer->GetMoveInput(); // 이동 입력
		packet.act_type = 0;

		XMFLOAT3 position = m_pPlayer->GetPosition();
		XMFLOAT3 velocity = m_pPlayer->GetComponent<CRigidBody>()->GetVelocity();
		XMFLOAT3 look = m_pPlayer->GetLookVector();
		memcpy(&packet.position, &position, sizeof(XMFLOAT3)); // 현재 위치
		memcpy(&packet.velocity, &velocity, sizeof(XMFLOAT3)); // 이동 방향
		memcpy(&packet.look, &look, sizeof(XMFLOAT3)); // 이동 방향
		float pitch = m_pPlayer->GetComponent<CCamera>()->GetPitch();
		packet.pitch = pitch; // 피치

		//packet.hp = 100; // 체력
		packet.hp = (uint16_t)m_pPlayer->GetHealth();

		NetworkingClient::Instance().send_packet((char*)&packet);
	}
}

void COnlineScene::SendReloadStart()
{
	if (!m_pPlayer || !m_pPlayer->GetGun()) return;

	m_pPlayer->Reload();

	pkt_cs_reload pkt{};
	pkt.header.size = sizeof(pkt);
	pkt.header.type = PKT_TYPE::C_S_RELOAD;
	pkt.gun_type = static_cast<GunType>(m_pPlayer->GetGun()->GetGunType());  // 현재 총

	NetworkingClient::Instance().send_packet((char*)&pkt);

}

void COnlineScene::SendReloadFinish()
{
	if (!m_pPlayer) return;

	pkt_cs_reload_finish pkt{};
	pkt.header.size = sizeof(pkt);
	pkt.header.type = PKT_TYPE::C_S_RELOAD_FINISH;
	pkt.gun_type = static_cast<GunType>(m_pPlayer->GetGun()->GetGunType());  // 현재 총

	NetworkingClient::Instance().send_packet((char*)&pkt);

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
	packet.header.size = sizeof(packet);              
	packet.header.type = PKT_TYPE::C_S_SHOOT;

	memcpy(&packet.bulletPos, &fireInfo.xmf3Position, sizeof(XMFLOAT3)); // 총알 위치
	memcpy(&packet.bulletDir, &fireInfo.xmf3Look, sizeof(XMFLOAT3)); // 총알 방향 * 거리

	NetworkingClient::Instance().send_packet((char*)&packet);
}

bool COnlineScene::Fire(CPlayer* pPlayer, FIRE_INFO* pFireInfo)
{
	FIRE_INFO fireInfo{};
	auto ret = CGameScene::Fire(pPlayer, &fireInfo);

	// 로컬 정보였을 경우
	if (ret && m_pPlayer == pPlayer) {
		SendFirePacket(fireInfo);

		/*if(m_pPlayer->GetGun()->GetCurrentAmmo() <= 0)
		{
			m_pPlayer->Reload();
		}*/
	}

	return ret;
}

bool COnlineScene::Fire(CPlayer* pPlayer)
{
	auto ret = Fire(pPlayer, nullptr);
	return ret;
}
