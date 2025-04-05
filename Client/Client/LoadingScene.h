///////////////////////////////////////////////////////////////////////////////
// Date: 2025-04-04
// LoadingScene.h : CLoadingScene 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Scene.h"

class CLoadingScene : public CScene
{
public:
	CLoadingScene();
	virtual ~CLoadingScene();

	// Scene Initialization / Release
	virtual void InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature) override;
	virtual void ReleaseObjects() override;
	virtual void ReleaseUploadBuffers() override;

	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;

	virtual bool Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr) override;

	// Shader Variables
	//void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) override;
	//void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList) override;
	//void ReleaseShaderVariables() override;
};