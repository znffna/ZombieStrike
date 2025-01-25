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

	// Update GameObjects
}

bool CScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (false == CheckWorkRendering())
	{
		// Scene is not running or pausing
		return (false);
	}

	// Set Root Signature

	// Set Viewport and Scissor

	// Update Camera Variables

	// Update Shader Variables

	// Render GameObjects [Through Batch Shader]

	return (true);
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