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

void CScene::FixedUpdate()
{
	for (auto& pGameObject : m_vecpGameObjects)
	{
		//pGameObject->FixedUpdate();
	}
}

void CScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	for (auto& pGameObject : m_vecpGameObjects)
	{
		//pGameObject->Render(pd3dCommandList, pCamera);
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


