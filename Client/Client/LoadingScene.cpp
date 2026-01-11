///////////////////////////////////////////////////////////////////////////////
// Date: 2025-04-04
// LoadingScene.cpp : CLoadingScene 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#include "LoadingScene.h"

#include "GameFramework.h"

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
		pText->SetSize((float)WINDOW_WIDTH / 2, (float)WINDOW_HEIGHT / 2, WINDOW_WIDTH, 100.0f, true);

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

		auto loadinfo = CGameFramework::Instance()->GetSceneLoadInfo();
		// CPU Process 계산
		float cpuratio{};
		switch (loadinfo.buildstate)
		{
		case ESceneBuildState::Idle:
			cpuratio = 0.0f;
			break;
		case ESceneBuildState::Requested:
			cpuratio = 20.0f;
			break;
		case ESceneBuildState::Building:
			cpuratio = 50.0f;
			break;
		case ESceneBuildState::CPU_Completed:
			cpuratio = 100.0f;
			break;
		default:
			cpuratio = 100.0f;
			break;
		}

		// GPU Process 계산
		float gpuratio{};
		auto prevloadedCount = loadinfo.GetPreviousLoadedCount();
		auto totalCount = loadinfo.GetTotalResourceCount() - prevloadedCount;
		auto currentLoad = CResourceManager::Instance().m_nUploadMeshCount.load() + CResourceManager::Instance().m_nUploadMaterialCount.load() - prevloadedCount;
		gpuratio = (float)(currentLoad) / (float)(totalCount) * 100.0f;

		if (totalCount == 0) {
			LoadingText += "   0.0%";
		}
		else
		{
			LoadingText += "   " + std::to_string(cpuratio * 0.5f + gpuratio * 0.5f) + "%";
		}

		m_pTextObject->GetComponent<CTextComponent>()->SetText(::to_wstring(LoadingText));
	}
}

bool CLoadingScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	CScene::Render(pd3dCommandList, m_pCamera);

	return true;
}