///////////////////////////////////////////////////////////////////////////////
// Date: 2025-04-04
// GameScene.cpp : CGameScene 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#include "GameScene.h"
#include "CollisionChecker.h"

CGameScene::CGameScene()
{
}

CGameScene::~CGameScene()
{
}

void CGameScene::InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	// Create Objects
	ResourceManager& resourceManager = GetResourceManager();

	// Skybox
	m_pSkyBox = CSkyBox::Create(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get());
	AddObject(m_pSkyBox);

	// Terrain
	XMFLOAT3 xmf3Scale(1.0f, 32.0f / 255.0f, 1.0f);
	XMFLOAT4 xmf4Color(0.0f, 0.2f, 0.3f, 0.0f);
	m_pTerrain = CHeightMapTerrain::InitializeByBinary(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get(), _T("Terrain/terrain.bin"), _T("Terrain/terrain.raw"), 1025, 1025, 65, 65, xmf3Scale, xmf4Color);
	AddObject(m_pTerrain);
	//m_pTerrain = CHeightMapTerrain::Create(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get(), _T("Terrain/terrain.raw"), 1025, 1025, 65, 65, xmf3Scale, xmf4Color);
	//m_pTerrain = CHeightMapTerrain::Create(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get(), _T("Terrain/terrain.raw"), 257, 257, 13, 13, xmf3Scale, xmf4Color);

	// Cube
	std::shared_ptr<CGameObject> pGameObject;
	pGameObject = CCubeObject::Create(pd3dDevice, pd3dCommandList, pd3dRootSignature);
	pGameObject->SetPosition(DirectX::XMFLOAT3(0.0f, 0.0f, 10.0f));
	AddObject(pGameObject);

	// Player 생성
	std::shared_ptr<CPlayer> pPlayer = CPlayer::Create(pd3dDevice, pd3dCommandList, pd3dRootSignature, m_pTerrain, nullptr, 2, 0);
	pPlayer->SetPosition(DirectX::XMFLOAT3(0.0f, 100.0f, 0.0f));
	AddObject(pPlayer);
	m_pPlayer = pPlayer;

	// Zombie Object
	StoreZombie(pd3dDevice, pd3dCommandList, pd3dRootSignature, 1000);

	// Map Load
	auto pMap = resourceManager.GetModelInfo("Map");
	pMap->m_pModelRootObject->UpdateTransform();
	m_pMap = pMap->m_pModelRootObject;
	m_pMap->Update(0.0f);
	AddObjects(m_pMap->GetChilds());
	//AddObject(m_pMap);

	// Collision Checker
	auto pCollisionChecker = std::make_shared<CCollisionChecker>(this);
	pCollisionChecker->Initialize(pd3dDevice, pd3dCommandList);
	AddObject(pCollisionChecker);
}

void CGameScene::PostInitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	if(m_pPlayer)
	{
		m_pCamera = m_pPlayer->GetComponent<CCamera>();
	}
}

void CGameScene::CreateFixedCamera(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_pCamera) return;
	m_pCamera = std::make_shared<CThirdPersonCamera>();
	m_pCamera->SetViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	m_pCamera->SetScissorRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	m_pCamera->SetOffset(XMFLOAT3(0.0f, 0.0f, -5.0f));
	m_pCamera->GenerateViewMatrix(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_pCamera->GenerateProjectionMatrix(((float)WINDOW_WIDTH / (float)WINDOW_HEIGHT), 60.0f, 1.0f, 1000.0f);
	m_pCamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CGameScene::ReleaseObjects()
{
}

void CGameScene::ReleaseUploadBuffers()
{
}

void CGameScene::Update(float deltaTime)
{
	CScene::Update(deltaTime);
}

bool CGameScene::ProcessInput(const INPUT_PARAMETER& pBuffer, float deltaTime)
{
	// 키보드 입력의 정보 압축
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
			m_pPlayer->Move(dwDirection, 10.0f, deltaTime);
			m_pPlayer->Rotate(pBuffer.cyDelta, pBuffer.cxDelta, 0.0f);
		}
	}

	return true;
}

void CGameScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
}

void CGameScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID) {
	case WM_KEYDOWN: {
		switch (wParam)
		{
			//case VK_LEFT:
			//{
			//	//m_pCamera->Move(-1.0f,0.0f,0.0f);
			//	m_pCamera->Rotate(0.0f, -10.0f, 0.0f);
			//	m_pCamera->RegenerateViewMatrix();
			//	break;
			//}
			//case VK_RIGHT:
			//{
			//	//m_pCamera->Move(1.0f, 0.0f, 0.0f);
			//	m_pCamera->Rotate(0.0f, 10.0f, 0.0f);
			//	m_pCamera->RegenerateViewMatrix();
			//	break;
			//}
			//case VK_UP:
			//{
			//	//m_pCamera->Move(0.0f, 0.0f, 1.0f);
			//	m_pCamera->Rotate(-10.0f, 0.0f, 0.0f);
			//	m_pCamera->RegenerateViewMatrix();
			//	break;
			//}
			//case VK_DOWN:
			//{
			//	//m_pCamera->Move(0.0f, 0.0f, -1.0f);
			//	m_pCamera->Rotate(10.0f, 0.0f, 0.0f);
			//	m_pCamera->RegenerateViewMatrix();
			//	break;
			//}
			/*
			case VK_SPACE:
			{
				m_pCamera->Move(0.0f, 10.0f, 0.0f);
				m_pCamera->RegenerateViewMatrix();
				break;
			}
			case VK_SHIFT:
			{
				m_pCamera->Move(0.0f, -10.0f, 0.0f);
				m_pCamera->RegenerateViewMatrix();
				break;
			}
			*/
			/*
			case 'W': case 'w':
			{
				m_ppHierarchicalObjects[0]->Move(0.0f, 0.0f, 1.0f);
				break;
			}
			case 'S': case 's':
			{
				m_ppHierarchicalObjects[0]->Move(0.0f, 0.0f, -1.0f);
				break;
			}
			case 'A': case 'a':
			{
				m_ppHierarchicalObjects[0]->Move(-1.0f, 0.0f, 0.0f);
				break;
			}
			case 'D': case 'd':
			{
				m_ppHierarchicalObjects[0]->Move(1.0f, 0.0f, 0.0f);
				break;
			}
			}
			break;
			}
			*/
		default:
			break;
		}
	}
	}
}

