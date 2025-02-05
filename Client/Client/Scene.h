///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-28
// Scene.h : Scene 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"
#include "GameObject.h"
#include "Camera.h"
#include "Shader.h"

enum SCENE_STATE
{
	SCENE_STATE_NONE = 0x00, // 초기화되지 않은 상태 [ None ]
	SCENE_STATE_ALLOCING, // 할당 중 [ ALLOC ]
	SCENE_STATE_RUNNING,  // 실행 중 [ Update / Render ]
	SCENE_STATE_PAUSING,  // 일시 중지 중 [ Render ]
	SCENE_STATE_ENDING    // 종료 중 [ Release ]
};

class CScene
{
public:
	CScene();
	virtual ~CScene();

	// Scene Initialization / Release
	virtual void InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseObjects();
	virtual void ReleaseUploadBuffers();

	// Scene Management
	bool CheckWorkRendering() { return (m_SceneState == SCENE_STATE_RUNNING) || (m_SceneState == SCENE_STATE_PAUSING); }
	bool CheckWorkUpdating() { return (m_SceneState == SCENE_STATE_RUNNING); }
	SCENE_STATE GetSceneState() { return m_SceneState; }
	void SetSceneState(SCENE_STATE SceneState) { m_SceneState = SceneState; }

	// Scene Method
	virtual void FixedUpdate(float deltaTime);
	virtual bool Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr);

	// Shader method
	ComPtr<ID3D12RootSignature> CreateGraphicsRootSignature(ID3D12Device* pd3dDevice);

	// Shader Variables
	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();

protected:
	SCENE_STATE m_SceneState = SCENE_STATE_NONE; // Scene State

	// Shader Variables
	ComPtr<ID3D12RootSignature> m_pd3dGraphicsRootSignature;
	ComPtr<ID3D12RootSignature> m_pd3dComputeRootSignature;
};

////////////////////////////////////////////////////////////////////////////////////////////
//

class LoadingScene : public CScene
{
public:
	LoadingScene();
	virtual ~LoadingScene();

	// Scene Initialization / Release
	virtual void InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) override;
	virtual void ReleaseObjects() override;
	virtual void ReleaseUploadBuffers() override;

	//virtual bool Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr) override;

	// Shader Variables
	//void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) override;
	//void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList) override;
	//void ReleaseShaderVariables() override;
};