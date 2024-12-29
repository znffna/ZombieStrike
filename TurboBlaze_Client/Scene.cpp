///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-28
// Scene.cpp : Scene 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#include "Scene.h"

CScene::CScene()
{
}

CScene::~CScene()
{
}

void CScene::InitializeObjects()
{
}

void CScene::ReleaseObjects()
{
}

void CScene::ReleaseUploadBuffers()
{
}

void CScene::FixedUpdate(float deltaTime)
{
	if (false == CheckWorkUpdating())
	{
		// Scene is not running
		return;
	}

	for (auto& pGameObject : m_vecpGameObjects)
	{
		pGameObject->FixedUpdate(deltaTime);
	}
}

void CScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (false == CheckWorkRendering())
	{
		// Scene is not running or pausing
		return;
	}

	// Set Root Signature

	// Set Viewport and Scissor

	// Update Camera Variables

	// Update Shader Variables

	// Render GameObjects [Through Batch Shader]

	for (auto& pGameObject : m_vecpGameObjects)
	{
		pGameObject->Render(pd3dCommandList);
	}
}

void CScene::AddGameObject(std::shared_ptr<CGameObject>& pGameObject)
{
	m_vecpGameObjects.push_back(pGameObject);
}

void CScene::RemoveGameObject(std::shared_ptr<CGameObject>& pGameObject)
{
	auto iter = std::find(m_vecpGameObjects.begin(), m_vecpGameObjects.end(), pGameObject);
	if (iter != m_vecpGameObjects.end())
	{
		m_vecpGameObjects.erase(iter);
	}
}

void CScene::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{

}

void CScene::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{

}

void CScene::ReleaseShaderVariables()
{

}


