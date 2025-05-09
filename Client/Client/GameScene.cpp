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
	CResourceManager& resourceManager = CResourceManager::GetInstance();

	// <Environment>

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

	// <Store GameObjects>
	StoreZombie(pd3dDevice, pd3dCommandList, pd3dRootSignature, 300);
	StorePlayer(pd3dDevice, pd3dCommandList, pd3dRootSignature, 2);

	// <Initialize GameObjects>

	// Cube
	//std::shared_ptr<CGameObject> pGameObject;
	//pGameObject = CCubeObject::Create(pd3dDevice, pd3dCommandList, pd3dRootSignature);
	//pGameObject->SetPosition(DirectX::XMFLOAT3(0.0f, 0.0f, 10.0f));
	//AddObject(pGameObject);

	// Player 생성
	std::shared_ptr<CPlayer> pPlayer = GetPlayer();
	//std::shared_ptr<CPlayer> pPlayer = CPlayer::Create(pd3dDevice, pd3dCommandList, pd3dRootSignature, m_pTerrain, nullptr, 2, 0);
	pPlayer->SetPosition(DirectX::XMFLOAT3(0.0f, 100.0f, 0.0f));
	AddObject(pPlayer);
	m_pPlayer = pPlayer;

	// Gun 생성
	std::shared_ptr<CGun> pGun = CGun::Create(pd3dDevice, pd3dCommandList, pd3dRootSignature, 0);
	m_pPlayer->SetGun(pGun);

	//pGun = CGun::Create(pd3dDevice, pd3dCommandList, pd3dRootSignature, 0);
	//pGun->SetPosition(DirectX::XMFLOAT3(0.0f, 15.0f, 0.0f));
	//AddObject(pGun);

	// Zombie 생성
	/*std::shared_ptr<CZombieObject> pZombie = GetZombie();
	pZombie->SetPosition(DirectX::XMFLOAT3(0.0f, 100.0f, 5.0f));
	AddObject(pZombie);*/

	
	// Map Load
	auto pMap = resourceManager.GetModelInfo("Stage_1");
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

	SetSceneState(SCENE_STATE_READY_TO_START);
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
	if (pBuffer.pKeysBuffer['W'] & 0xF0)dwDirection |= DIR_FORWARD;
	if (pBuffer.pKeysBuffer['S'] & 0xF0)dwDirection |= DIR_BACKWARD;
	if (pBuffer.pKeysBuffer['A'] & 0xF0)dwDirection |= DIR_LEFT;
	if (pBuffer.pKeysBuffer['D'] & 0xF0)dwDirection |= DIR_RIGHT;
	//if (pBuffer.pKeysBuffer[VK_PRIOR] & 0xF0)dwDirection |= DIR_UP;
	//if (pBuffer.pKeysBuffer[VK_NEXT] & 0xF0)dwDirection |= DIR_DOWN;

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
	/*switch (nMessageID) {
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		default:
			break;
		}
	}
	}*/
}

void CGameScene::StoreZombie(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature, int nZombieCount)
{
	m_pZombiePool.reserve(nZombieCount);
	for (int i = 0; i < nZombieCount; ++i)
	{
		std::shared_ptr<CZombieObject> pZombie = CZombieObject::Create(pd3dDevice, pd3dCommandList, pd3dRootSignature, m_pTerrain, nullptr, i);
		pZombie->GetComponent<CRigidBody>()->SetTerrainUpdatedContext(m_pTerrain.get());
		pZombie->SetActive(false);
		m_pZombiePool.push_back(pZombie);
	}
}

std::shared_ptr<CZombieObject> CGameScene::GetZombie(int nSkinType)
{
	for (auto& pZombie : m_pZombiePool)
	{
		if (false == pZombie->IsActive())
		{
			pZombie->SetSkin(nSkinType);
			pZombie->SetActive(true);
			return pZombie;
		}
	}
	return nullptr;
}

void CGameScene::StorePlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature, int nPlayerCount)
{
	m_pPlayerObjects.reserve(nPlayerCount);
	for (int i = 0; i < nPlayerCount; ++i)
	{
		std::shared_ptr<CPlayer> pPlayer = CPlayer::Create(pd3dDevice, pd3dCommandList, pd3dRootSignature, m_pTerrain, nullptr, 2, i);
		pPlayer->GetComponent<CRigidBody>()->SetTerrainUpdatedContext(m_pTerrain.get());
		pPlayer->SetActive(false);
		m_pPlayerObjects.push_back(pPlayer);
	}
}

std::shared_ptr<CPlayer> CGameScene::GetPlayer(int nSkinType)
{
	for (auto& pPlayer : m_pPlayerObjects)
	{
		if (false == pPlayer->IsActive())
		{
			pPlayer->SetSkin(nSkinType);	
			pPlayer->SetActive(true);
			return pPlayer;
		}
	}
	return nullptr;
}
