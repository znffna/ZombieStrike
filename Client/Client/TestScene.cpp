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
	auto pPlayerObject = RequestCreateObject(TypeTag<CPlayer>());
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

		case 'W': case 'w':
		{
			auto camera = GetMainCamera();
			camera->Move(XMFLOAT3(0.0f, 0.0f, 0.1f));
			camera->RegenerateViewMatrix();
		}
			break;
		case 'S': case 's':
		{
			auto camera = GetMainCamera();
			camera->Move(XMFLOAT3(0.0f, 0.0f, -0.1f));
			camera->RegenerateViewMatrix();
		}
			break;
		case 'A': case 'a':
		{
			auto camera = GetMainCamera();
			camera->Move(XMFLOAT3(-0.1f, 0.0f, 0.0f));
			camera->RegenerateViewMatrix();
		}
			break;
		case 'D': case 'd':
		{
			auto camera = GetMainCamera();
			camera->Move(XMFLOAT3(0.1f, 0.0f, 0.0f));
			camera->RegenerateViewMatrix();
		}
			break;
		case VK_SPACE:
			{
			auto camera = GetMainCamera();
			camera->Move(XMFLOAT3(0.0f, 0.1f, 0.0f));
			camera->RegenerateViewMatrix();
		}
			break;
		case VK_LSHIFT:
		{
			auto camera = GetMainCamera();
			camera->Move(XMFLOAT3(0.0f, -0.1f, 0.0f));
			camera->RegenerateViewMatrix();
		}
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
		case VK_F3 :
		{
			auto pObject = RequestCreateObject(TypeTag<CZombieObject>());
			pObject->SetPosition(Vector3::Add(GetMainCamera()->GetPosition(), Vector3::ScalarProduct(GetMainCamera()->GetLook(), 3.0f, false)));

#ifdef _DEBUG
			std::string debugMsg = to_string(GetSceneName()) + " - CTestScene::OnProcessingKeyboardMessage: Zombie Object Created. CID = " + std::to_string(pObject->GetID()) + ", Name = " + pObject->GetName() + "\n";
			OutputDebugStringA(debugMsg.c_str());
#endif
		}
			break;
		}
		break;
	}	
	}
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

