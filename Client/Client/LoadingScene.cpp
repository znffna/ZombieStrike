///////////////////////////////////////////////////////////////////////////////
// Date: 2025-04-04
// LoadingScene.cpp : CLoadingScene 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#include "LoadingScene.h"


CLoadingScene::CLoadingScene()
{
}

CLoadingScene::~CLoadingScene()
{
}

void CLoadingScene::InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	// Create Objects

	// Create a Quad for Loading Text
	{
		auto pObject = AddObject(std::make_unique<CGameObject>());
		auto pText = pObject->CreateComponent<CTextComponent>();
		pText->SetColor(D2D1::ColorF::White);
		pText->SetFontSize(50.0f);
		pText->SetText(L"Loading...");

		m_pTextObject = pObject;
	}

}

void CLoadingScene::ReleaseObjects()
{
}

void CLoadingScene::ReleaseUploadBuffers()
{
}

void CLoadingScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
		case WM_KEYDOWN:
		{
			switch (wParam)
			{
			case VK_ESCAPE:
				// Loading 씬에서는 ESC 키를 누르면 프로그램 종료
				PostQuitMessage(0);
				break;
			}
		}
	}
}

void CLoadingScene::Update(float fTimeElapsed)
{
	CScene::Update(fTimeElapsed);

	m_fTimeElapsed += fTimeElapsed;

	{
		std::string LoadingText = "Loading";
		for (int i = 0; i < static_cast<int>(m_fTimeElapsed) % 4; ++i)
		{
			LoadingText += ".";
		}
		m_pTextObject->GetComponent<CTextComponent>()->SetText(::to_wstring(LoadingText));
	}
}

bool CLoadingScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	CScene::Render(pd3dCommandList, m_pCamera);

	return true;
}