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
	std::shared_ptr<CMaterial> pMaterial = std::make_shared<CMaterial>();
	pMaterial->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	std::shared_ptr<CShader> pStandardShader = std::make_shared<CStandardShader>();
	pStandardShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature.Get());
	std::shared_ptr<CMesh> pCubeMesh = std::make_shared<CCubeMesh>(pd3dDevice, pd3dCommandList, 1.0f, 1.0f, 1.0f);

	std::shared_ptr<CRotatingObject> pRotateGameObject;
	pRotateGameObject = CRotatingObject::Create(pd3dDevice, pd3dCommandList);
	pRotateGameObject->SetMesh(pCubeMesh);
	pRotateGameObject->AddMaterial(pMaterial);
	pMaterial->SetShader(pStandardShader);
	pRotateGameObject->SetPosition(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));
	pRotateGameObject->SetRotationSpeed(50.0f);
	pRotateGameObject->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	m_ppObjects.push_back(pRotateGameObject);
}

void CLoadingScene::ReleaseObjects()
{
}

void CLoadingScene::ReleaseUploadBuffers()
{
}

void CLoadingScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{

}

bool CLoadingScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (false == CheckWorkRendering())
	{
		// Scene is not running or pausing
		return (false);
	}

	CScene::Render(pd3dCommandList, m_pCamera.get());

	return true;
}