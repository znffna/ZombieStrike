///////////////////////////////////////////////////////////////////////////////
// Date: 2025-04-04
// GameScene.cpp : CGameScene 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#include "GameScene.h"
#include "Sprite.h"
#include "GaugeBar.h"
#include "CollisionChecker.h"
#include "GameFramework.h"

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
	// 생성시 RootSignature 지정
	if (pd3dRootSignature == nullptr) {
		pd3dRootSignature = m_pd3dGraphicsRootSignature.Get();
	}

	// Create Objects
	auto pSkyBoxObject = RequestCreateObject(TypeTag<CSkyBox>());
	
	// Terrain
	ChangeTerrain(0);

	//CHeightMapTerrainDesc terrainDesc;
	//terrainDesc.wstrHeightMapFilePath = L"Terrain/terrain1.raw";
	//terrainDesc.wstrMeshFilePath = L"Terrain/terrain1.bin";
	//terrainDesc.nWidth = 257;
	//terrainDesc.nLength = 257;
	//terrainDesc.xmf3Scale = XMFLOAT3(1.0f, 50.0f / std::numeric_limits<HEIGHTMAPDEPTH>::max(), 1.0f);
	//terrainDesc.xmf4Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	//auto pTerrainObject = RequestCreateObject(TypeTag<CHeightMapTerrain>(), terrainDesc);

	//auto pTerrainObject = RequestCreateObject(TypeTag<CHeightMapTerrain>(), L"Terrain/terrain1.raw", L"Terrain/terrain1.bin", 257, 257,	XMFLOAT3(1.0f, 50.0f / 255.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	
	//m_pTerrain = GetTerrain(0);
	////m_pTerrain = CHeightMapTerrain::InitializeByBinary(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get(), _T("Terrain/terrain1.bin"), _T("Terrain/terrain.raw"), 257, 257, 13, 13, xmf3Scale, xmf4Color);
	////m_pTerrain = CHeightMapTerrain::InitializeByBinary(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get(), _T("Terrain/terrain.bin"), _T("Terrain/terrain.raw"), 1025, 1025, 65, 65, xmf3Scale, xmf4Color);
	//AddObject(m_pTerrain);
	////m_pTerrain = CHeightMapTerrain::Create(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get(), _T("Terrain/terrain.raw"), 257, 257, 13, 13, xmf3Scale, xmf4Color);
	//
	//// <Store GameObjects>
	//StoreZombie(pd3dDevice, pd3dCommandList, pd3dRootSignature, 100);
	//StorePlayer(pd3dDevice, pd3dCommandList, pd3dRootSignature, 3);
	//

	// <Initialize GameObjects>
	// Player 생성
	//auto pPlayer = AddObject(std::make_unique<CPlayer>());

	auto pPlayer = SpawnPlayer(XMFLOAT3(100.0f, 0.0f, 100.0f), "Player1", 0, 100, 0, 0, m_pTerrain);
	m_pPlayer = pPlayer;

	//auto pPlayer = RequestCreateObject(TypeTag<CPlayer>(), 0);
	////pPlayer->SetSkin(0); // 기본 모델 설정
	//pPlayer->SetPosition(DirectX::XMFLOAT3(100.0f, 0.0f, 100.0f));
	//pPlayer->SetTerrain(pTerrainObject);


	// Gun 생성
	auto pGun = RequestCreateObject(TypeTag<CGun>(), 0);
	//auto pGun = dynamic_cast<CGun*>(AddObject(std::make_unique<CGun>()));
	m_pPlayer->SetGun(pGun);

	// Map Load
	
	//auto pMap = resourceManager.GetModelInfo("Stage1");
	//pMap->m_pModelRootObject->UpdateTransform();
	//m_pMap = pMap->m_pModelRootObject;
	//m_pMap->SetLayer(LAYER_ENVIRONMENT);
	//m_pMap->UpdateBBCache();
	//AddObject(m_pMap);

	auto mapObject = RequestCreateObject(TypeTag<CMapObject>(), L"Model/Stage1.bin");

	// Collision Checker
	//auto pCollisionChecker = (CCollisionChecker*)AddObject(std::make_unique<CCollisionChecker>(this));
	auto pCollisionChecker = RequestCreateObject(TypeTag<CCollisionChecker>());
	m_pCollisionChecker = pCollisionChecker;

	// BulletObject
	//auto pBullet = std::make_shared<CBulletParticleObject>(pd3dDevice, pd3dCommandList, pd3dRootSignature, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 65.0f, 0.0f), 20.0f, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(8.0f, 8.0f), MAX_BULLETS);
	auto pBullet = RequestCreateObject(TypeTag<CBulletParticleObject>(), MAX_BULLETS);
	m_pBulletObject = pBullet;
	CGun::m_pBulletObject = pBullet;
	//AddObject(pBullet);

	// UI Object
	{
		auto pAimSprite = RequestCreateObject(TypeTag<CSprite>(), L"Image/aim_cross.dds");
		float aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
		pAimSprite->SetSize(0.0f, 0.0f, 0.04f, 0.04f * aspectRatio);
	}

	//	{
	//		std::shared_ptr<CTexture> pHealthTexture = std::make_shared<CTexture>(1, RESOURCE_TEXTURE2D, 1);
	//		pHealthTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/GaugeBar.dds", RESOURCE_TEXTURE2D, 0);
	//		CScene::CreateShaderResourceViews(pd3dDevice, pHealthTexture.get(), 0, ROOT_PARAMETER_STANDARD_TEXTURES);
	//		
	//		std::shared_ptr<CMaterial> pHealthMaterial = std::make_shared<CMaterial>();
	//		pHealthMaterial->SetTexture(pHealthTexture);
	//		pHealthMaterial->SetShader(pUIShader);
	//		
	//		std::shared_ptr<CGaugeBar> pHealthObject = std::make_shared<CGaugeBar>();
	//		pHealthObject->Initialize(pd3dDevice, pd3dCommandList);
	//		pHealthObject->SetMesh(pRectangleMesh);
	//		pHealthObject->AddMaterial(pHealthMaterial);
	//		pHealthObject->SetName("Health");
	//		
	//		// Health Bar 크기 설정

	//		float aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
	//		pHealthObject->SetGauge(1.0f, -1.0f, -1.0f, 0.3f, 0.1f);

	//		// Health Text 생성
	//		/*pHealthObject->m_pTextBlock = CGameFramework::pGameFramework->GetUILayer()->GetNewTextBlock(2);
	//		pHealthObject->m_pTextBlock->SetText(L"HP :");
	//		pHealthObject->m_pTextBlock->SetActive(false);*/
	//		m_pHealthObject = pHealthObject;

	//		pPlayer->SetHealthObject(pHealthObject);
	//		AddObject(pHealthObject);
	//	}
	//}

	// Score Info
	{
		m_pScoreInfo = RequestCreateObject(TypeTag<CGameObject>());
		m_pScoreInfo->SetName("ScoreInfo");
		auto pTextComp = m_pScoreInfo->CreateComponent<CTextComponent>();
		pTextComp->SetActive(true);
		pTextComp->SetSize(0.0f, 0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, false);
		pTextComp->SetFont(L"Arial");
		pTextComp->SetFontSize(WINDOW_HEIGHT / 35.0f);
		pTextComp->SetBrush(D2D1::ColorF(D2D1::ColorF::Purple, 1.0f));
		pTextComp->SetText(std::to_wstring(m_nScore));
	}

	// Ammo Info
	{
		m_pAmmoInfo = RequestCreateObject(TypeTag<CGameObject>());
		m_pAmmoInfo->SetName("AmmoInfo");
		auto pTextComp = m_pAmmoInfo->CreateComponent<CTextComponent>();
		pTextComp->SetActive(true);
		pTextComp->SetSize((float)WINDOW_WIDTH * 4 / 5, (float)WINDOW_HEIGHT * 9 / 10, (float)WINDOW_WIDTH / 5, (float)WINDOW_HEIGHT / 10, false);
		pTextComp->SetFont(L"Arial");
		pTextComp->SetFontSize(WINDOW_HEIGHT / 35.0f);
		pTextComp->SetBrush(D2D1::ColorF(D2D1::ColorF::Purple, 1.0f));
		pTextComp->SetText(L"");
	}

	
	// Shader
	m_pDepthRenderShader = std::make_shared<CDepthRenderShader>();
	m_pDepthRenderShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get());

	m_pShadowShader = std::make_shared<CShadowMapShader>();
	m_pShadowShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get());
	m_pShadowShader->BuildObjects(pd3dDevice, pd3dCommandList, m_pDepthRenderShader->GetDepthTexture().get());

	m_pShadowMapToViewport = std::make_shared<CShadowToViewportShader>();
	m_pShadowMapToViewport->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get());
	m_pShadowMapToViewport->BuildObjects(pd3dDevice, pd3dCommandList, m_pDepthRenderShader->GetDepthTexture().get());

	/*m_pMinimapShader = std::make_shared<CMinimapShader>();
	m_pMinimapShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get());

	m_pMinimapToViewport = std::make_shared<CMinimapToViewportShader>();
	m_pMinimapToViewport->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get());
	m_pMinimapToViewport->BuildObjects(pd3dDevice, pd3dCommandList, m_pMinimapShader->GetMinimapTexture().get());*/


	// 마지막 모든 Object의 생성이 끝나면 Player의 카메라를 추적
	if (m_pPlayer)
	{
		auto pCamera = m_pPlayer->GetComponent<CCamera>();
		SelectCamera(pCamera);
	}

	Sound::PlayMusic("Sound/bgmusic.mp3");
}

void CGameScene::CreateFreeCamera(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	// 임시 카메라
	m_pFreeCamera = AddObject(std::make_unique<CGameObject>());

	// Camera 생성
	auto pcameracomponent = m_pFreeCamera->CreateComponent<CCamera>();
	pcameracomponent->SetViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	pcameracomponent->SetScissorRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	pcameracomponent->SetOffset(XMFLOAT3(1.0f, 0.7f, -2.5f));
	pcameracomponent->GenerateViewMatrix(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	pcameracomponent->GenerateProjectionMatrix(((float)WINDOW_WIDTH / (float)WINDOW_HEIGHT), 60.0f, 1.0f, 1000.0f);
	pcameracomponent->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	m_pFreeCamera->SetActive(true);
}

void CGameScene::PostInitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
}

void CGameScene::CreateDefaultCamera(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_pCamera) return;

	auto pCameraObject = AddObject(std::make_unique<CGameObject>());
	pCameraObject->SetName("DefaultCamera");
	auto pcameracomponent = pCameraObject->CreateComponent<CThirdPersonCamera>();
	pcameracomponent->SetViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	pcameracomponent->SetScissorRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	pcameracomponent->GenerateViewMatrix(XMFLOAT3(0.0f, 0.0f, -5.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	pcameracomponent->GenerateProjectionMatrix(((float)WINDOW_WIDTH / (float)WINDOW_HEIGHT), 60.0f, 1.0f, 1000.0f);
	pcameracomponent->CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CGameScene::ReleaseObjects()
{
}

void CGameScene::ReleaseUploadBuffers()
{
}

void CGameScene::Update(float deltaTime)
{
	CScene::Update(deltaTime); // Collider check 포함

	BuildFiredBullets();

	if(m_pScoreInfo)
	{
		auto pTextComp = m_pScoreInfo->GetComponent<CTextComponent>();
		if (pTextComp)
		{
			pTextComp->SetText(L"Score: " + std::to_wstring(m_nScore) + L"  Wave: " + std::to_wstring(m_nWave));
		}
	}

	if (m_pAmmoInfo) {
		auto pTextComp = m_pAmmoInfo->GetComponent<CTextComponent>();
		if (pTextComp && m_pPlayer)
		{
			if (auto pGun = m_pPlayer->GetGun())
			{
				pTextComp->SetText(L"Ammo: " + std::to_wstring(pGun->GetCurrentAmmo()) + L" / " + std::to_wstring(pGun->GetMaxAmmo()));
			}
		}
	}
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
		pBulletVertex.m_xmf3Position = pBullet.xmf3MuzzlePosition;
		pBulletVertex.m_xmf3Destination = Vector3::Add(pBullet.xmf3Position, Vector3::ScalarProduct(pBullet.xmf3Look, pBullet.fRange));
		pBulletVertex.m_xmf3Velocity = Vector3::ScalarProduct(pBullet.xmf3Look, pBullet.fspeed);
		
		// 총알 궤적 출력 시간 설정
		pBulletVertex.m_fLifetime = 1.0f;
		pBulletVertex.m_nBulletType = pBullet.nBulletType;
		pBulletVertex.m_nHitObjectType = result.nHitObjectType;
		pBulletVertices.push_back(pBulletVertex);

#ifdef _DEBUG
		{
			std::string debugOutput = "CGameScene::BuildFiredBullets() - Bullet Position: " + std::to_string(pBullet.xmf3Position.x) + ", " + std::to_string(pBullet.xmf3Position.y) + ", " + std::to_string(pBullet.xmf3Position.z) + "\n";
			debugOutput += "Velocity: " + std::to_string(pBulletVertex.m_xmf3Velocity.x) + ", " + std::to_string(pBulletVertex.m_xmf3Velocity.y) + ", " + std::to_string(pBulletVertex.m_xmf3Velocity.z) + "\n";
			debugOutput += "Impact Distance: " + std::to_string(pBullet.fRange) + "\n";
			debugOutput += "Hit Object Type: ";
			switch (pBulletVertex.m_nHitObjectType) {
				case HIT_TYPE_NONE:
					debugOutput += "None";
					break;
				case HIT_TYPE_ENVIRONMENT:
					debugOutput += "Environment";
					break;
				case HIT_TYPE_ENEMY:
					debugOutput += "Enemy";
					break;
			}
			debugOutput += "\n";
			
			OutputDebugStringA(debugOutput.c_str());
		}
#endif
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
			SelectCamera(m_pPlayer->GetComponent<CCamera>());
			break;
		case 'C':
			SelectCamera(m_pFreeCamera->GetComponent<CCamera>());
			break;
		case VK_F1:
			g_bRenderCollider = !g_bRenderCollider;
			break; 
		case VK_F2:
			g_bRenderShadowMap = !g_bRenderShadowMap;
			break;
		case VK_ESCAPE:
			PopScene();
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

	//if (m_pTerrain) RemoveObject(m_pTerrain);
	//m_pTerrain = GetTerrain(m_nStageIndex);
	//AddObject(m_pTerrain);

	//for (auto& pObject : m_pZombiePool)
	//{
	//	if (pObject) pObject->GetComponent<CRigidBody>()->SetTerrainUpdatedContext(m_pTerrain.get());
	//}
	//for (auto& pObject : m_pPlayerObjects)
	//{
	//	if (pObject) pObject->GetComponent<CRigidBody>()->SetTerrainUpdatedContext(m_pTerrain.get());
	//}
	////if (m_pPlayer) m_pPlayer->GetComponent<CRigidBody>()->SetTerrainUpdatedContext(m_pTerrain.get());

	//if (m_pMap) {
	//	RemoveObject(m_pMap);
	//}

	//auto pMap = CResourceManager::Instance().GetModelInfo(m_strStageNames[m_nStageIndex]);
	//pMap->m_pModelRootObject->UpdateTransform();
	//m_pMap = pMap->m_pModelRootObject;
	//m_pMap->Update(0.0f);
	//m_pMap->SetLayer(LAYER_ENVIRONMENT);
	//AddObject(m_pMap);
}

bool CGameScene::Fire(CPlayer* pPlayer, FIRE_INFO* pFireInfo)
{
	bool ret = false;
	ret = pPlayer->Fire(pFireInfo);
	return ret;
}

bool CGameScene::Fire(CPlayer* pPlayer, XMFLOAT3 cameraPos, XMFLOAT3 cameraDir)
{
	bool ret = false;
	FIRE_INFO fireInfo;
	fireInfo.xmf3Position = cameraPos;
	fireInfo.xmf3Look = cameraDir;
	fireInfo.fRange = pPlayer->GetGun()->GetRange();
	fireInfo.fspeed = pPlayer->GetGun()->GetSpeed();
	fireInfo.nBulletType = pPlayer->GetGun()->GetGunType();
	ret = pPlayer->Fire(&fireInfo);
	return false;
}

CPlayer* CGameScene::SpawnPlayer(XMFLOAT3 xmf3Position, std::string name, int nSkinIndex, short starthp, char actType, char move_input, void* pTerrain)
{
	auto pPlayer = RequestCreateObject(TypeTag<CPlayer>(), nSkinIndex);
	//pPlayer->SetSkin(0); // 기본 모델 설정
	pPlayer->SetPosition(xmf3Position);

	if(pTerrain != nullptr)
	{
		pPlayer->SetTerrain((pTerrain));
	}
	else
	{
		pPlayer->SetTerrain(m_ppLayerView[LAYER_TERRAIN][0]);
	}

	pPlayer->SetHealth(starthp);
	pPlayer->SetMaxHealth(starthp);

	// 행동 유형 설정
	pPlayer->GetComponent<CAnimationController>()->SetUpperPose(actType);
	pPlayer->SetMoveInput(move_input);

	return pPlayer;
}

CZombieObject* CGameScene::SpawnZombie(XMFLOAT3 xmf3Position, std::string name, int nSkinIndex, short starthp, char actType, char move_input)
{
	auto pZombie = RequestCreateObject(TypeTag<CZombieObject>(), nSkinIndex);
	//pPlayer->SetSkin(0); // 기본 모델 설정
	pZombie->SetPosition(xmf3Position);
	pZombie->SetTerrain(m_ppLayerView[LAYER_TERRAIN][0]);

	//pZombie->SetHealth(starthp);
	//pZombie->SetMaxHealth(starthp);

	// 행동 유형 설정
	pZombie->GetComponent<CAnimationController>()->SetUpperPose(actType);
	return pZombie;
}

CHeightMapTerrain* CGameScene::ChangeTerrain(int nMapIndex)
{
	std::vector<std::wstring> heightmappath = 
	{
		L"Terrain/terrain1.raw",
		L"Terrain/terrain2.raw",
		L"Terrain/terrain3.raw"
	};

	std::vector<std::wstring> meshhpath =
	{
		L"Terrain/terrain1.bin",
		L"Terrain/terrain2.bin",
		L"Terrain/terrain3.bin"
	};

	assert(nMapIndex >= 0 && nMapIndex < heightmappath.size());

	// 이미 지형이 존재할 경우 지운다.
	if(!m_ppLayerView[LAYER_TERRAIN].empty()) RequestDestroyObject(m_ppLayerView[LAYER_TERRAIN][0]->GetID());

	// 새로운 지형 생성
	CHeightMapTerrainDesc terrainDesc;
	terrainDesc.wstrHeightMapFilePath = heightmappath[nMapIndex];
	terrainDesc.wstrMeshFilePath = meshhpath[nMapIndex];
	//terrainDesc.wstrMeshFilePath = L"null";
	terrainDesc.nWidth = 257;
	terrainDesc.nLength = 257;
	terrainDesc.nBlockWidth = 13;
	terrainDesc.nBlockLength = 13;
	terrainDesc.xmf3Scale = XMFLOAT3(1.0f, 50.0f / std::numeric_limits<HEIGHTMAPDEPTH>::max(), 1.0f);
	terrainDesc.xmf4Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	auto pTerrainObject = RequestCreateObject(TypeTag<CHeightMapTerrain>(), terrainDesc);

	// 모든 물리 객체에 새로운 지형 정보를 설정
	auto pPlayers = m_ppLayerView[LAYER_PLAYER];
	for(auto& pPlayer : pPlayers)
	{
		if (pPlayer)
		{
			pPlayer->GetComponent<CRigidBody>()->SetTerrainUpdatedContext(pTerrainObject);
		}
	}

	auto pZombies = m_ppLayerView[LAYER_ENEMY];
	for (auto& pZombie : pZombies)
	{
		if (pZombie)
		{
			pZombie->GetComponent<CRigidBody>()->SetTerrainUpdatedContext(pTerrainObject);
		}
	}

	// GameScene 자체적으로 Cache
	m_pTerrain = pTerrainObject;

	return pTerrainObject;
}



