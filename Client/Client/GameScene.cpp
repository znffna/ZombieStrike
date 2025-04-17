///////////////////////////////////////////////////////////////////////////////
// Date: 2025-04-04
// GameScene.cpp : CGameScene 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#include "GameScene.h"

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

	// Terrain
	XMFLOAT3 xmf3Scale(1.0f, 32.0f / 255.0f, 1.0f);
	XMFLOAT4 xmf4Color(0.0f, 0.2f, 0.3f, 0.0f);
	m_pTerrain = CHeightMapTerrain::InitializeByBinary(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get(), _T("Terrain/terrain.bin"), _T("Terrain/terrain.raw"), 1025, 1025, 65, 65, xmf3Scale, xmf4Color);
	//m_pTerrain = CHeightMapTerrain::Create(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get(), _T("Terrain/terrain.raw"), 1025, 1025, 65, 65, xmf3Scale, xmf4Color);
	//m_pTerrain = CHeightMapTerrain::Create(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get(), _T("Terrain/terrain.raw"), 257, 257, 13, 13, xmf3Scale, xmf4Color);

	// Cube
	std::shared_ptr<CGameObject> pGameObject;
	pGameObject = CCubeObject::Create(pd3dDevice, pd3dCommandList, pd3dRootSignature);
	pGameObject->SetPosition(DirectX::XMFLOAT3(0.0f, 0.0f, 10.0f));
	m_ppObjects.push_back(pGameObject);

	// Zombie Object
	std::shared_ptr<CLoadedModelInfo> pModel = resourceManager.GetModelInfo("FuzZombie");
	if (!pModel)
	{
		pModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, pd3dRootSignature, "Model/FuzZombie.bin", nullptr);
		resourceManager.SetSkinInfo("FuzZombie", pModel);
	}

	std::shared_ptr<CZombieObject> pZombie = CZombieObject::Create(pd3dDevice, pd3dCommandList, pd3dRootSignature, m_pTerrain, pModel, 2);
	pZombie->SetPosition(DirectX::XMFLOAT3(0.0f, 100.0f, 0.0f));
	m_ppHierarchicalObjects.push_back(pZombie);

	// Map Load
	auto pMap = resourceManager.GetModelInfo("Map");
	pMap->m_pModelRootObject->UpdateTransform();
	m_pMap = pMap->m_pModelRootObject;
	m_pMap->Update(0.0f);

	// Default Camera 위치 수정
	m_pCamera->SetPosition(Vector3::Add(pZombie->GetPosition(), XMFLOAT3(0.0f, 0.0f, -10.0f)));
	m_pCamera->RegenerateViewMatrix();
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

	// Update Camera Position
	if (m_pCamera)
	{
		// Camera Follow Zombie
		if (m_ppHierarchicalObjects.size() > 0)
		{
			//XMFLOAT3 xmf3CameraPosition = Vector3::Add(m_ppHierarchicalObjects[0]->GetPosition(), XMFLOAT3(0.0f, 0.0f, -10.0f));
			XMFLOAT3 xmf3CameraPosition = Vector3::Add(m_ppHierarchicalObjects[0]->GetPosition(), Vector3::ScalarProduct(m_pCamera->GetLook(), -10.0f));
			m_pCamera->SetPosition(xmf3CameraPosition);
			m_pCamera->RegenerateViewMatrix();
		}
	}
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
		if (m_ppHierarchicalObjects.size() > 0)
		{
			m_ppHierarchicalObjects[0]->Move(dwDirection, 10.0f, deltaTime);
		}

		if (m_pCamera)
		{
			m_pCamera->Rotate(pBuffer.cyDelta, pBuffer.cxDelta, 0.0f);
			m_pCamera->RegenerateViewMatrix();
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

