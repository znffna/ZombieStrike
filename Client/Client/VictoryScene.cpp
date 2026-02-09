#include "VictoryScene.h"

#include "GameFramework.h"

CVictoryScene::CVictoryScene() : CScene()
{
}

CVictoryScene::~CVictoryScene()
{
}

void CVictoryScene::InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	if (pd3dRootSignature == nullptr) {
		pd3dRootSignature = m_pd3dGraphicsRootSignature.Get();
	}
	// Victory / Background
	{
		auto pBackgroundObject = RequestCreateObject(TypeTag<CSprite>(), L"Image/Victory.dds");
		pBackgroundObject->SetSize(0.0f, 0.0f, 1.0f, 1.0f);
	}
}

void CVictoryScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID) {
	case WM_LBUTTONDOWN:
		{
			// Victory 씬에서 마우스 왼쪽 버튼 클릭 시 Title 씬으로 전환
			CGameFramework::Instance()->RequestSceneChange(CPopScene());
		}
		break;
	}
}

void CVictoryScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID) {
	case WM_KEYDOWN:
		{
			switch (wParam)
			{
			case VK_ESCAPE:
				// Victory 씬에서 ESC 키를 누르면 Title 씬으로 전환
				CGameFramework::Instance()->RequestSceneChange(CPopAllScene());
				break;
			default:
				break;
			}
		}
		break;
	}
}
