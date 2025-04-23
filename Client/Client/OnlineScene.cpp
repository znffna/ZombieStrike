#include "OnlineScene.h"

COnlineScene::COnlineScene()
{	
	std::string debugOutput = "OnlineScene 생성됨";
	debugOutput += "m_NetworkClient Address : ";
	debugOutput += std::to_string(reinterpret_cast<uintptr_t>(&m_NetworkClient));
	debugOutput += "\n";
	debugOutput += "m_NetworkClient.Overlapped Address : ";
	debugOutput += std::to_string(reinterpret_cast<uintptr_t>(&m_NetworkClient.recv_over));
	OutputDebugStringA(debugOutput.c_str());
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
	CGameScene::Update(deltaTime);
	// Update Camera Position
	if (m_pCamera)
	{
		// Camera Follow Zombie
		if (m_ppHierarchicalObjects.size() > 0)
		{
			XMFLOAT3 xmf3CameraPosition = Vector3::Add(m_ppHierarchicalObjects[0]->GetPosition(), XMFLOAT3(0.0f, 0.0f, -10.0f));
			m_pCamera->SetPosition(xmf3CameraPosition);
			m_pCamera->RegenerateViewMatrix();
		}
	}

	// Network Client Update
	if (m_NetworkClient.is_running)
	{
		// Network Client Update
		SendPlayerState();
	}
}

bool COnlineScene::ProcessInput(const INPUT_PARAMETER& pBuffer, float deltaTime)
{
	return false;
}

void COnlineScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
}

void COnlineScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
}

void COnlineScene::ProcessPacket(char* recv_p)
{	
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
		XMFLOAT3 direction = m_pPlayer->GetLookVector();
		memcpy(&packet.obj.meta.position, &position, sizeof(XMFLOAT3)); // 현재 위치
		memcpy(&packet.obj.meta.direction, &direction, sizeof(XMFLOAT3)); // 이동 방향

		packet.obj.meta.speed = 5.0f; // 이동 속도
		packet.obj.meta.hp = 100; // 체력

		m_NetworkClient.send_packet((char*)&packet);
	}
}
