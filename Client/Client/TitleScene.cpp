#include "TitleScene.h"

#include "Sprite.h"

CTitleScene::CTitleScene()
{
}

CTitleScene::~CTitleScene()
{
}

void CTitleScene::InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	if(pd3dRootSignature == nullptr) {
		pd3dRootSignature = m_pd3dGraphicsRootSignature.Get();
	}

	//std::shared_ptr<CMesh> pRectangleMesh = CResourceManager::Instance().GetMesh("UI");
	//
	//std::shared_ptr<CShader> pUIShader = std::make_shared<CTextureToViewportShader>(nullptr);
	//pUIShader->CreateShader(pd3dDevice, pd3dRootSignature);


	//// Title / Background
	//{
	//	std::shared_ptr<CTexture> pTitleTexture = std::make_shared<CTexture>(1, RESOURCE_TEXTURE2D, 1);
	//	pTitleTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Title.dds", RESOURCE_TEXTURE2D, 0);
	//	CScene::CreateShaderResourceViews(pd3dDevice, pTitleTexture.get(), 0, ROOT_PARAMETER_STANDARD_TEXTURES);

	//	std::shared_ptr<CMaterial> pTitleMaterial = std::make_shared<CMaterial>();
	//	pTitleMaterial->SetTexture(pTitleTexture);
	//	pTitleMaterial->SetShader(pUIShader);

	//	std::shared_ptr<CSprite> pBackgroundObject = std::make_shared<CSprite>();
	//	pBackgroundObject->Initialize(pd3dDevice, pd3dCommandList);
	//	pBackgroundObject->SetMesh(pRectangleMesh);
	//	pBackgroundObject->AddMaterial(pTitleMaterial);

	//	pBackgroundObject->SetSize(0.0f, 0.0f, 1.0f, 1.0f);

	//	AddObject(pBackgroundObject);
	//	m_pBackgroundObject = pBackgroundObject;
	//}

	//// Start
	//{
	//	std::shared_ptr<CTexture> pStartTexture = std::make_shared<CTexture>(1, RESOURCE_TEXTURE2D, 1);
	//	pStartTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Start.dds", RESOURCE_TEXTURE2D, 0);
	//	CScene::CreateShaderResourceViews(pd3dDevice, pStartTexture.get(), 0, ROOT_PARAMETER_STANDARD_TEXTURES);

	//	std::shared_ptr<CMaterial> pStartMaterial = std::make_shared<CMaterial>();
	//	pStartMaterial->SetTexture(pStartTexture);
	//	pStartMaterial->SetShader(pUIShader);

	//	std::shared_ptr<CSprite> pStartButton = std::make_shared<CSprite>();
	//	pStartButton->Initialize(pd3dDevice, pd3dCommandList);
	//	pStartButton->SetMesh(pRectangleMesh);
	//	pStartButton->AddMaterial(pStartMaterial);

	//	pStartButton->SetSize(0.6f, 0.7f, 0.4f, 0.5f);

	//	AddObject(pStartButton);
	//	m_pStartButton = pStartButton;
	//}

	//// Start
	//{
	//	std::shared_ptr<CTexture> pExitTexture = std::make_shared<CTexture>(1, RESOURCE_TEXTURE2D, 1);
	//	pExitTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Exit.dds", RESOURCE_TEXTURE2D, 0);
	//	CScene::CreateShaderResourceViews(pd3dDevice, pExitTexture.get(), 0, ROOT_PARAMETER_STANDARD_TEXTURES);

	//	std::shared_ptr<CMaterial> pExitMaterial = std::make_shared<CMaterial>();
	//	pExitMaterial->SetTexture(pExitTexture);
	//	pExitMaterial->SetShader(pUIShader);

	//	std::shared_ptr<CSprite> pExitButton = std::make_shared<CSprite>();
	//	pExitButton->Initialize(pd3dDevice, pd3dCommandList);
	//	pExitButton->SetMesh(pRectangleMesh);
	//	pExitButton->AddMaterial(pExitMaterial);

	//	pExitButton->SetSize(0.6f, 0.2f, 0.4f, 0.5f);

	//	AddObject(pExitButton);
	//	m_pExitButton = pExitButton;
	//}
}

void CTitleScene::ReleaseObjects()
{
}

void CTitleScene::ReleaseUploadBuffers()
{
}

#include "GameFramework.h"

// UTF-16(wstring) → UTF-8(string)
inline std::string WStringToString(const std::wstring& wstr) {
	if (wstr.empty()) return {};
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	std::string result(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), result.data(), size_needed, nullptr, nullptr);
	return result;
}

// UTF-8(string) → UTF-16(wstring)
inline std::wstring StringToWString(const std::string& str) {
	if (str.empty()) return {};
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
	std::wstring result(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), result.data(), size_needed);
	return result;
}

void CTitleScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	// 1. 마우스 위치 (픽셀 좌표)
	int mouseX = LOWORD(lParam);
	int mouseY = HIWORD(lParam);

	// 2. 클라이언트 영역 크기
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	int width = clientRect.right - clientRect.left;
	int height = clientRect.bottom - clientRect.top;

	// 3. 픽셀 좌표를 정규화 좌표로 변환 (-1.0f ~ 1.0f)
	float normalizedX = (2.0f * mouseX / width) - 1.0f;
	float normalizedY = 1.0f - (2.0f * mouseY / height);  // Y는 위가 +이므로 뒤집기

	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
	{
		if (m_pStartButton && m_pStartButton->IsClicked(normalizedX, normalizedY))
		{
			std::thread connectThread([&]() {
				bool ret = NetworkingClient::Instance().Connect();
				if (!ret)
				{
					//TODO : 해당 로직을 TitleScene에서 이미지를 띄우고 지우는 로직으로 변경할 예정
					auto ip = StringToWString(NetworkingClient::Instance().LoadIPAddress());
					std::wstring str = L" 서버에 연결할 수 없습니다. (" + ip + L")";
					OutputDebugStringW(str.c_str());
					return;
				}

				CGameFramework::Instance()->RequestSceneChange(CPushScene{ TypeTag<COnlineScene>{}});

			});
			connectThread.detach();  // 비동기 연결
		}
		else if (m_pExitButton && m_pExitButton->IsClicked(normalizedX, normalizedY))
		{
			PostQuitMessage(0);
		}
		break;
	}
	default:
		break;
	}
}

void CTitleScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID) {
		case WM_KEYDOWN:
		{
			switch (wParam)
			{
			case VK_ESCAPE:
				// ESC 키를 눌러 프로그램 종료
				// TODO : 해당 로직을 TitleScene에서 이미지를 띄우고 지우는 로직으로 변경할 예정
				PostQuitMessage(0);
				break;
			default:
				break;
			}
			break;
		}
	}
}
