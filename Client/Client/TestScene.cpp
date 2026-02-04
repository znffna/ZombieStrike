#include "TestScene.h"
#include "GameFramework.h"

CTestScene::CTestScene()
{
}

CTestScene::~CTestScene()
{
}

void CTestScene::InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	auto pTextObject = AddObject(std::make_unique<CGameObject>());
	auto pTextComponent = pTextObject->CreateComponent<CTextComponent>();
	std::wstring text = L"TestScene " + std::to_wstring(CGameFramework::Instance()->GetSceneSize());
	pTextComponent->SetText(text.data());
	pTextComponent->SetFont(L"Arial");
	pTextComponent->SetColor(D2D1::ColorF::Black);
	pTextComponent->SetFontSize(24.0f);
	pTextComponent->SetSize(100.0f, 100.0f, 400.0f, 50.0f);

	// auto pPlayerObject = AddObject(CPlayer::Create(pd3dDevice, pd3dCommandList, pd3dRootSignature, 0));
	auto pPlayerObject = RequestCreateObject(TypeTag<CPlayer>(), 1);
	// auto pPlayerObject = AddObject(std::make_unique<CPlayer>());
	auto pPlayer = dynamic_cast<CPlayer*>(pPlayerObject);
	// pPlayer->Initialize(pd3dDevice, pd3dCommandList, pd3dRootSignature, 0);
	pPlayerObject->SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
	pPlayerObject->SetLook(XMFLOAT3(0.0f, 0.0f, -1.0f));

	m_pPlayer = dynamic_cast<CPlayer*>(pPlayerObject);

	// Player 다중 실행 테스트
	/*int nPlayerCount = 10;
	for(int i = 0; i < nPlayerCount; ++i){
		auto pPlayerObject = AddObject(std::make_unique<CPlayer>());
		auto pPlayer = dynamic_cast<CPlayer*>(pPlayerObject);
		pPlayer->Initialize(pd3dDevice, pd3dCommandList, pd3dRootSignature, 0);
		pPlayerObject->SetName("Player");
		pPlayerObject->SetPosition(XMFLOAT3(0.5f + 0.5f * i, 0.0f, 0.0f));
		pPlayerObject->SetLook(XMFLOAT3(0.0f, 0.0f, -1.0f));
	}*/

	// SkyBox 생성
	auto pSkyBoxObject = RequestCreateObject(TypeTag<CSkyBox>());

	{
		/*std::wstring wstrHeightMapFilePath, std::wstring wstrMeshFilePath,
			int nWidth, int nLength,
	XMFLOAT3 xmf3Scale(1.0f, 50.0f / 255.0f, 1.0f);
			XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color*/
	}

	// Terrain 생성
	CHeightMapTerrainDesc terrainDesc;
	terrainDesc.wstrHeightMapFilePath = L"Terrain/terrain1.raw";
	terrainDesc.wstrMeshFilePath = L"Terrain/terrain1.bin";
	terrainDesc.nWidth = 257;
	terrainDesc.nLength = 257;
	terrainDesc.xmf3Scale = XMFLOAT3(1.0f, 50.0f / 255.0f, 1.0f);
	terrainDesc.xmf4Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	auto pTerrainObject = RequestCreateObject(TypeTag<CHeightMapTerrain>(), terrainDesc);
	//auto pTerrainObject = RequestCreateObject(TypeTag<CHeightMapTerrain>(), L"Terrain/terrain1.raw", L"Terrain/terrain1.bin", 257, 257,	XMFLOAT3(1.0f, 50.0f / 255.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

	auto mapObject = RequestCreateObject(TypeTag<CMapObject>(), L"Model/Stage1.bin");

}

void CTestScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	CScene::OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);

	switch (nMessageID)
	{
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case VK_ESCAPE:
			CGameFramework::Instance()->RequestSceneChange(CPopScene());
			break;
		case VK_F2:
		{
			auto pObject = RequestCreateObject(TypeTag<CPlayer>());
			pObject->SetPosition(Vector3::Add(GetMainCamera()->GetPosition(), Vector3::ScalarProduct(GetMainCamera()->GetLook(), 3.0f, false)));

#ifdef _DEBUG
			std::string debugMsg = to_string(GetSceneName()) + " - CTestScene::OnProcessingKeyboardMessage: Player Object Created. CID = " + std::to_string(pObject->GetID()) + ", Name = " + pObject->GetName() + "\n";
			OutputDebugStringA(debugMsg.c_str());
#endif
		}
			break;
		case VK_F5:
		{
			for(auto& vector : m_ppLayerView)
			{
				if (vector.first != GAMEOBJECT_LAYER::LAYER_ENEMY) continue;

				for(auto& object : vector.second)
				{
					RequestDestroyObject(object->GetID());
				}
			}
		}
		case '1':
		case '2':
		case '3':
		{
			auto pObject = RequestCreateObject(TypeTag<CPlayer>(), wParam - '1');
			pObject->SetPosition(Vector3::Add(GetMainCamera()->GetPosition(), Vector3::ScalarProduct(GetMainCamera()->GetLook(), 3.0f, false)));

#ifdef _DEBUG
			std::string debugMsg = to_string(GetSceneName()) + " - CTestScene::OnProcessingKeyboardMessage: Player Object Created. CID = " + std::to_string(pObject->GetID()) + ", Name = " + pObject->GetName() + "\n";
			OutputDebugStringA(debugMsg.c_str());
#endif
		}
		break;
		}
	}	
	}
}

bool CTestScene::ProcessMouseInput(float cxDelta, float cyDelta, float deltaTime)
{
	if (cxDelta != 0.0f || cyDelta != 0.0f) {
		m_pPlayer->Rotate(cyDelta, cxDelta, 0.0f);
	}

	return true;
}

bool CTestScene::ProcessKeyboardInput(const UCHAR pKeysBuffer[256], float deltaTime)
{
	DWORD dwDirection = 0;
	if (pKeysBuffer['W'] & 0xF0) dwDirection |= DIR_FORWARD;
	if (pKeysBuffer['S'] & 0xF0) dwDirection |= DIR_BACKWARD;
	if (pKeysBuffer['A'] & 0xF0) dwDirection |= DIR_LEFT;
	if (pKeysBuffer['D'] & 0xF0) dwDirection |= DIR_RIGHT;

	if (false && m_pPlayer) {
		//m_pPlayer->SetMoveInput(dwDirection);
		m_pPlayer->Move(dwDirection, 10.0f, deltaTime);

		if (pKeysBuffer['R'] & 0xF0) {
			if (auto pGun = m_pPlayer->GetGun())
			{
				m_pPlayer->Reload();
			}
		}
		return true;
	}

	return false;
}

void CTestScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_RBUTTONDOWN:
	{
		CGameFramework::Instance()->RequestSceneChange(CPushScene{ TypeTag<CTestScene>{}});
		break;
	}
	}
}

