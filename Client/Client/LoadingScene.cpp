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

	// Loading Background
	{
		auto pBackground = RequestCreateObject(TypeTag<CSprite>(), L"Image/Loading.dds");
		pBackground->SetSize(0.0f, 0.0f, 1.0f, 1.0f);
	}

	// Create a Quad for Loading Text
	{
		auto pObject = AddObject(std::make_unique<CGameObject>());
		auto pText = pObject->CreateComponent<CTextComponent>();
		pText->SetColor(D2D1::ColorF::White);
		pText->SetFontSize(50.0f);
		pText->SetText(L"Loading...");
		pText->SetSize((float)WINDOW_WIDTH / 2, (float)WINDOW_HEIGHT - 100.0f, WINDOW_WIDTH, 100.0f, true);

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
		std::wstring LoadingText = L"";
		// ResourceManager에서 직접 현재 상태 가져오기
		auto status = CResourceManager::Instance().GetResourceLoadStatus();

		// CPU Process 계산
		auto buildState = CGameFramework::Instance()->GetSceneBuildState();

		switch (buildState)
		{
		case ESceneBuildState::Idle:
			LoadingText = L"대기 중";
			break;

		case ESceneBuildState::Requested:
			LoadingText = L"씬 생성 준비 중";
			break;

		case ESceneBuildState::Building:
			LoadingText = L"씬 생성 중";
			// 애니메이션 점 추가
			for (int i = 0; i < static_cast<int>(m_fTimeElapsed) % 4; ++i)
			{
				LoadingText += L".";
			}
			break;

		case ESceneBuildState::CPU_Completed:
		{
			// 리소스 업로드 진행률 계산
			float gpuProgress = status.GetProgress() * 100.0f;
			LoadingText = L"리소스 로딩 중 : " + std::format(L"{:.1f}", gpuProgress) + L"%";
		}
		break;

		case ESceneBuildState::All_Completed:
			LoadingText = L"로딩 완료!";
			break;

		case ESceneBuildState::Failed:
			LoadingText = L"로딩 실패";
			break;

		default:
			LoadingText = L"알 수 없는 상태";
			break;
		}

		m_pTextObject->GetComponent<CTextComponent>()->SetText(LoadingText);
	}
}

bool CLoadingScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	CScene::Render(pd3dCommandList, m_pCamera);

	return true;
}