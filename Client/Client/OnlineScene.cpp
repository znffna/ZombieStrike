#include "OnlineScene.h"

COnlineScene::COnlineScene()
{	
	std::string debugOutput = "OnlineScene »ý¼ºµÊ";
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
