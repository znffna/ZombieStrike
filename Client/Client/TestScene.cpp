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

