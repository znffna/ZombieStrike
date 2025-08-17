///////////////////////////////////////////////////////////////////////////////
// Date: 2025-04-04
// GameScene.cpp : CGameScene 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#include "GameScene.h"
#include "Sprite.h"
#include "GaugeBar.h"
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
	StoreZombie(pd3dDevice, pd3dCommandList, pd3dRootSignature, 100);
	StorePlayer(pd3dDevice, pd3dCommandList, pd3dRootSignature, 3);
	

	// <Initialize GameObjects>
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

	CreateFreeCamera(pd3dDevice, pd3dCommandList);

	// Map Load
	auto pMap = resourceManager.GetModelInfo("Stage1");
	pMap->m_pModelRootObject->UpdateTransform();
	m_pMap = pMap->m_pModelRootObject;
	m_pMap->SetLayer(CGameObject::LAYER_ENVIRONMENT);
	m_pMap->UpdateBBCache();
	AddObject(m_pMap);

	// Collision Checker
	auto pCollisionChecker = std::make_shared<CCollisionChecker>(this);
	pCollisionChecker->Initialize(pd3dDevice, pd3dCommandList);
	m_pCollisionChecker = pCollisionChecker;
	AddObject(pCollisionChecker);

	// BulletObject
	std::shared_ptr<CBulletParticleObject> pBullet = std::make_shared<CBulletParticleObject>(pd3dDevice, pd3dCommandList, pd3dRootSignature, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 65.0f, 0.0f), 20.0f, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(8.0f, 8.0f), MAX_BULLETS);
	m_pBulletObject = pBullet;
	CGun::m_pBulletObject = pBullet;
	AddObject(pBullet);

	// UI Object
	{
		std::shared_ptr<CMesh> pRectangleMesh = CResourceManager::GetInstance().GetMesh("UI");

		std::shared_ptr<CShader> pUIShader = std::make_shared<CTextureToViewportShader>(nullptr);
		pUIShader->CreateShader(pd3dDevice, pd3dRootSignature);
		{
			std::shared_ptr<CTexture> pAimTexture = std::make_shared<CTexture>(1, RESOURCE_TEXTURE2D, 1);
			pAimTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/aim_cross.dds", RESOURCE_TEXTURE2D, 0);
			CScene::CreateShaderResourceViews(pd3dDevice, pAimTexture.get(), 0, ROOT_PARAMETER_STANDARD_TEXTURES);

			std::shared_ptr<CMaterial> pTitleMaterial = std::make_shared<CMaterial>();
			pTitleMaterial->SetTexture(pAimTexture);
			pTitleMaterial->SetShader(pUIShader);

			std::shared_ptr<CSprite> pAimObject = std::make_shared<CSprite>();
			pAimObject->Initialize(pd3dDevice, pd3dCommandList);
			pAimObject->SetMesh(pRectangleMesh);
			pAimObject->AddMaterial(pTitleMaterial);
			pAimObject->SetName("Aim");

			float aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
			pAimObject->SetSize(0.0f, 0.0f, 0.05f, 0.05f * aspectRatio);
			AddObject(pAimObject);
		}

		{
			std::shared_ptr<CTexture> pHealthTexture = std::make_shared<CTexture>(1, RESOURCE_TEXTURE2D, 1);
			pHealthTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/GaugeBar.dds", RESOURCE_TEXTURE2D, 0);
			CScene::CreateShaderResourceViews(pd3dDevice, pHealthTexture.get(), 0, ROOT_PARAMETER_STANDARD_TEXTURES);
			
			std::shared_ptr<CMaterial> pHealthMaterial = std::make_shared<CMaterial>();
			pHealthMaterial->SetTexture(pHealthTexture);
			pHealthMaterial->SetShader(pUIShader);
			
			std::shared_ptr<CGaugeBar> pHealthObject = std::make_shared<CGaugeBar>();
			pHealthObject->Initialize(pd3dDevice, pd3dCommandList);
			pHealthObject->SetMesh(pRectangleMesh);
			pHealthObject->AddMaterial(pHealthMaterial);
			pHealthObject->SetName("Health");
			
			float aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
			pHealthObject->SetGauge(1.0f, -1.0f, -1.0f, 0.3f, 0.1f);

			pPlayer->SetHealthObject(pHealthObject);
			AddObject(pHealthObject);
		}
	}
	
	// Shader
	m_pDepthRenderShader = std::make_shared<CDepthRenderShader>(this);
	m_pDepthRenderShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature.Get());
	m_pDepthRenderShader->BuildObjects(pd3dDevice, pd3dCommandList);

	m_pShadowShader = std::make_shared<CShadowMapShader>(this);
	m_pShadowShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature.Get());
	m_pShadowShader->BuildObjects(pd3dDevice, pd3dCommandList, m_pDepthRenderShader->GetDepthTexture());

	m_pShadowMapToViewport = std::make_shared<CShadowToViewportShader>(this);
	m_pShadowMapToViewport->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature.Get());
	m_pShadowMapToViewport->BuildObjects(pd3dDevice, pd3dCommandList, m_pDepthRenderShader->GetDepthTexture());

	// 마지막 모든 Object의 생성이 끝나면 Player의 카메라를 추적
	if (m_pPlayer)
	{
		m_pCamera = m_pPlayer->GetComponent<CCamera>();
	}
}

void CGameScene::CreateFreeCamera(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	// 임시 카메라
	m_pFreeCamera = std::make_shared<CCamera>();
	// Camera 생성
	m_pFreeCamera->SetViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	m_pFreeCamera->SetScissorRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	m_pFreeCamera->SetOffset(XMFLOAT3(1.0f, 0.7f, -2.5f));
	m_pFreeCamera->GenerateViewMatrix(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_pFreeCamera->GenerateProjectionMatrix(((float)WINDOW_WIDTH / (float)WINDOW_HEIGHT), 60.0f, 1.0f, 1000.0f);
	m_pFreeCamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	m_pFreeCamera->SetActive(true);
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

	static const std::map<CGameObject::GAMEOBJECT_LAYER, std::string> LayerToString = {
	{ CGameObject::LAYER_DEFAULT, "LAYER_DEFAULT" },
	{ CGameObject::LAYER_SKYBOX, "LAYER_SKYBOX" },
	{ CGameObject::LAYER_TERRAIN, "LAYER_TERRAIN" },
	{ CGameObject::LAYER_ENVIRONMENT, "LAYER_ENVIRONMENT" },
	{ CGameObject::LAYER_ENEMY, "LAYER_ENEMY" },
	{ CGameObject::LAYER_PLAYER, "LAYER_PLAYER" },
	{ CGameObject::LAYER_GUN, "LAYER_GUN" },
	{ CGameObject::LAYER_BULLET, "LAYER_BULLET" },
	{ CGameObject::LAYER_CONTROLLER, "LAYER_CONTROLLER" },
	{ CGameObject::LAYER_UI, "LAYER_UI" },
	};

	if (m_bPrintObjectCount) {
		for(auto& pvecObject : m_ppGameObjects)
		{
			std::string debugOutput = "CGameScene::Update() - Layer: " + LayerToString.at(pvecObject.first) + ", Object Count: " + std::to_string(pvecObject.second.size()) + "\n";
			OutputDebugStringA(debugOutput.c_str());
		}

		std::string debugOutput = "////////////////////////////////" + std::to_string(g_nFrameCount) + "////////////////////////////////\n";
		OutputDebugStringA(debugOutput.c_str());

		m_bPrintObjectCount = false;
	}

	BuildFiredBullets();
}

void CGameScene::UpdateLights()
{
	if (m_pDepthRenderShader)
	{
		/*auto xmf3CameraPosition = m_pCamera->GetPosition();
		for (int i = 1; i < 2; ++i) {
			if (m_pLights[i].m_bEnable == false) continue;
			if (m_pLights[i].m_nType != DIRECTIONAL_LIGHT) continue;
			float fLightRange = m_pLights[i].m_fRange;
			m_pLights[i].m_xmf3Position = Vector3::Add(xmf3CameraPosition, Vector3::ScalarProduct(m_pLights[i].m_xmf3Direction, -fLightRange));
		}*/
	}
}

void CGameScene::BuildFiredBullets()
{
	// 일단 Scene에서 하되, 나중에 BulletParticleObject로 옮길 예정
	// Scene에서 하는 이유 : CollisionChecker가 Scene에 있기 때문(나중에 구조 수정 예정) => BulletParticleObject가 Scene에 의존성이 생김
	auto pFireInfos = m_pBulletObject->GetFireInfos();

	if(pFireInfos.empty())
	{
		// 총알 발사 정보가 없으면 아무것도 하지 않음
		return;
	}

	std::vector<CBulletVertex> pBulletVertices;
	pBulletVertices.reserve(pFireInfos.size());

	for (auto& pBullet : pFireInfos)
	{
		auto result = m_pCollisionChecker->CheckBulletCollision(pBullet.xmf3Position, pBullet.xmf3Look, pBullet.fRange);
		//m_pBulletObject->AddBullet(pBullet.xmf3Position, pBullet.xmf3Look, result.fImpactDistance);
		
		CBulletVertex pBulletVertex;
		pBulletVertex.m_xmf3Position = pBullet.xmf3Position;
		pBulletVertex.m_xmf3Destination = Vector3::Add(pBullet.xmf3Position, Vector3::ScalarProduct(pBullet.xmf3Look, pBullet.fRange));
		pBulletVertex.m_xmf3Velocity = Vector3::ScalarProduct(pBullet.xmf3Look, pBullet.fspeed);
		
		// 총알 궤적 출력 시간 설정
		pBulletVertex.m_fLifetime = 1.0f;
		pBulletVertex.m_nBulletType = pBullet.nBulletType;
		pBulletVertex.m_nHitObjectType = result.nHitObjectType;
		pBulletVertices.push_back(pBulletVertex);

		//if(g_bDebugOutput)
		{
			std::string debugOutput = "CGameScene::BuildFiredBullets() - Bullet Position: " + std::to_string(pBullet.xmf3Position.x) + ", " + std::to_string(pBullet.xmf3Position.y) + ", " + std::to_string(pBullet.xmf3Position.z) + "\n";
			debugOutput += "Velocity: " + std::to_string(pBulletVertex.m_xmf3Velocity.x) + ", " + std::to_string(pBulletVertex.m_xmf3Velocity.y) + ", " + std::to_string(pBulletVertex.m_xmf3Velocity.z) + "\n";
			debugOutput += "Impact Distance: " + std::to_string(pBullet.fRange) + "\n";
			
			OutputDebugStringA(debugOutput.c_str());
		}
	}
	m_pBulletObject->UpdateBulletVertices(pBulletVertices);
	m_pBulletObject->ClearFireInfos();
}

void CGameScene::OnPostRender(ID3D12GraphicsCommandList *pd3dCommandList)
{
	if(m_pBulletObject) m_pBulletObject->OnPostRender();
}

bool CGameScene::ProcessMouseInput(float cxDelta, float cyDelta, float deltaTime)
{
	if (cxDelta != 0.0f || cyDelta != 0.0f) {
		m_pPlayer->Rotate(cyDelta, cxDelta, 0.0f);
	}

	if(m_bMouseLButtonDown)
	{
		if (m_pPlayer && m_pPlayer->GetGun())
		{
			Fire(m_pPlayer, nullptr);
		}
	}
	return false;
}

bool CGameScene::ProcessKeyboardInput(const UCHAR pKeysBuffer[256], float deltaTime)
{
	DWORD dwDirection = 0;
	if (pKeysBuffer['W'] & 0xF0) dwDirection |= DIR_FORWARD;
	if (pKeysBuffer['S'] & 0xF0) dwDirection |= DIR_BACKWARD;
	if (pKeysBuffer['A'] & 0xF0) dwDirection |= DIR_LEFT;
	if (pKeysBuffer['D'] & 0xF0) dwDirection |= DIR_RIGHT;

	if (m_pPlayer) {
		//m_pPlayer->SetMoveInput(dwDirection);
		m_pPlayer->Move(dwDirection, 10.0f, deltaTime);

		if (pKeysBuffer['R'] & 0xF0) {
			if (auto pGun = m_pPlayer->GetGun())
			{
				m_pPlayer->Reload();
			}
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
	switch (nMessageID) {
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case 'P':
			m_bFreeCamera = false;
			m_pCamera = m_pPlayer->GetComponent<CCamera>();
			break;
		case 'C':
			m_bFreeCamera = true;
			m_pCamera = m_pFreeCamera;
			break;
		case VK_F1:
			m_bPrintObjectCount = true;
			break;
		default:
			break;
		}
	}
	}
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

bool CGameScene::Fire(const std::shared_ptr<CPlayer>& pPlayer, FIRE_INFO* pFireInfo)
{
	bool ret = pPlayer->Fire(pFireInfo);
	return ret;
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
