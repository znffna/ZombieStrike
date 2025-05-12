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
#ifdef _DEBUG
#endif

	// Create Objects
	CResourceManager& resourceManager = CResourceManager::GetInstance();

	if (pd3dRootSignature == nullptr) {
		pd3dRootSignature = m_pd3dGraphicsRootSignature.Get();
	}

	// <Environment>
	StoreTerrain(pd3dDevice, pd3dCommandList, pd3dRootSignature, 3);

	// Skybox
	m_pSkyBox = CSkyBox::Create(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get());
	AddObject(m_pSkyBox);
	
	// Terrain
	XMFLOAT3 xmf3Scale(1.0f, 50.0f / 255.0f, 1.0f);
	XMFLOAT4 xmf4Color(0.0f, 0.2f, 0.3f, 0.0f);
	m_pTerrain = GetTerrain(0);
	//m_pTerrain = CHeightMapTerrain::InitializeByBinary(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get(), _T("Terrain/terrain1.bin"), _T("Terrain/terrain.raw"), 257, 257, 13, 13, xmf3Scale, xmf4Color);
	//m_pTerrain = CHeightMapTerrain::InitializeByBinary(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get(), _T("Terrain/terrain.bin"), _T("Terrain/terrain.raw"), 1025, 1025, 65, 65, xmf3Scale, xmf4Color);
	AddObject(m_pTerrain);
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
	pPlayer->SetPosition(DirectX::XMFLOAT3(100.0f, 0.0f, 100.0f));
	AddObject(pPlayer);
	m_pPlayer = pPlayer;

	// Gun 생성
	std::shared_ptr<CGun> pGun = CGun::Create(pd3dDevice, pd3dCommandList, pd3dRootSignature, 0);
	m_pPlayer->SetGun(pGun);
	AddObject(pGun);

	// Zombie 생성
	/*std::shared_ptr<CZombieObject> pZombie = GetZombie();
	pZombie->SetPosition(DirectX::XMFLOAT3(0.0f, 100.0f, 5.0f));
	AddObject(pZombie);*/

	
	// Map Load
	auto pMap = resourceManager.GetModelInfo("Stage1");
	pMap->m_pModelRootObject->UpdateTransform();
	m_pMap = pMap->m_pModelRootObject;
	m_pMap->SetLayer(CGameObject::LAYER_ENVIRONMENT);
	m_pMap->Update(0.0f);
	AddObject(m_pMap);
	//AddObject(m_pMap);

	// Collision Checker
	auto pCollisionChecker = std::make_shared<CCollisionChecker>(this);
	pCollisionChecker->Initialize(pd3dDevice, pd3dCommandList);
	AddObject(pCollisionChecker);

	// BulletObject
	//std::shared_ptr<CBulletObject> pBullet = std::make_shared<CBulletObject>(pd3dDevice, pd3dCommandList, pd3dRootSignature, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 65.0f, 0.0f), 20.0f, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(8.0f, 8.0f), MAX_BULLETS);
	//m_pBulletObject = pBullet;
	//CGun::m_pBulletObject = pBullet;
	//AddObject(pBullet);

	// 마지막 모든 Object의 생성이 끝나면 Player의 카메라를 추적
	if (m_pPlayer)
	{
		m_pCamera = m_pPlayer->GetComponent<CCamera>();
	}
}

void CGameScene::PostInitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
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

void CGameScene::OnPostRender()
{
	if(m_pBulletObject) m_pBulletObject->OnPostRender();
}

bool CGameScene::ProcessInput(const INPUT_PARAMETER& pBuffer, float deltaTime)
{
	// 키보드 입력의 정보 압축
	DWORD dwDirection = 0;
	if (pBuffer.pKeysBuffer['W'] & 0xF0) dwDirection |= DIR_FORWARD;
	if (pBuffer.pKeysBuffer['S'] & 0xF0) dwDirection |= DIR_BACKWARD;
	if (pBuffer.pKeysBuffer['A'] & 0xF0) dwDirection |= DIR_LEFT;
	if (pBuffer.pKeysBuffer['D'] & 0xF0) dwDirection |= DIR_RIGHT;
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

	if (m_bMouseLButtonDown) {
		if (m_pPlayer && m_pBulletObject)
		{
			m_pPlayer->Fire();
		}
	}

	return true;
}

void CGameScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	CScene::OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);

	switch (nMessageID)
	{
	default:
		break;
	}
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

void CGameScene::ChangeMap(int nMapIndex)
{
	m_nStageIndex = nMapIndex % m_strStageNames.size();

	if (m_pTerrain) RemoveObject(m_pTerrain);
	m_pTerrain = GetTerrain(m_nStageIndex);
	AddObject(m_pTerrain);

	for (auto& pObject : m_pZombiePool)
	{
		if (pObject) pObject->GetComponent<CRigidBody>()->SetTerrainUpdatedContext(m_pTerrain.get());
	}
	for (auto& pObject : m_pPlayerObjects)
	{
		if (pObject) pObject->GetComponent<CRigidBody>()->SetTerrainUpdatedContext(m_pTerrain.get());
	}
	//if (m_pPlayer) m_pPlayer->GetComponent<CRigidBody>()->SetTerrainUpdatedContext(m_pTerrain.get());

	if (m_pMap) {
		RemoveObject(m_pMap);
	}

	auto pMap = CResourceManager::GetInstance().GetModelInfo(m_strStageNames[m_nStageIndex]);
	pMap->m_pModelRootObject->UpdateTransform();
	m_pMap = pMap->m_pModelRootObject;
	m_pMap->Update(0.0f);
	m_pMap->SetLayer(CGameObject::LAYER_ENVIRONMENT);
	AddObject(m_pMap);
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
		if(auto pCamera = pPlayer->GetComponent<CCamera>())
			pCamera->SetTerrainUpdatedContext(m_pTerrain.get());
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

void CGameScene::StoreTerrain(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature, int nTerrainCount)
{
	m_pTerrainObjects.reserve(nTerrainCount);

	XMFLOAT3 xmf3Scale(1.0f, 50.0f / 255.0f, 1.0f);
	XMFLOAT4 xmf4Color(0.0f, 0.2f, 0.3f, 0.0f);

	auto pTerrain1 = CHeightMapTerrain::InitializeByBinary(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get(), _T("Terrain/terrain1.bin"), _T("Terrain/terrain.raw"), 257, 257, 13, 13, xmf3Scale, xmf4Color);
	auto pTerrain2 = CHeightMapTerrain::InitializeByBinary(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get(), _T("Terrain/terrain2.bin"), _T("Terrain/terrain.raw"), 257, 257, 13, 13, xmf3Scale, xmf4Color);
	auto pTerrain3 = CHeightMapTerrain::InitializeByBinary(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get(), _T("Terrain/terrain3.bin"), _T("Terrain/terrain.raw"), 257, 257, 13, 13, xmf3Scale, xmf4Color);

	m_pTerrainObjects.push_back(pTerrain1);
	m_pTerrainObjects.push_back(pTerrain2);
	m_pTerrainObjects.push_back(pTerrain3);
}
