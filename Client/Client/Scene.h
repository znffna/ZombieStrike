///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-28
// Scene.h : Scene Ŭ������ ��� ����
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"

class CCamera;
class CGameObject;

enum SCENE_STATE
{
	SCENE_STATE_NONE = 0x00, // �ʱ�ȭ���� ���� ���� [ None ]
	SCENE_STATE_ALLOCING, // �Ҵ� �� [ ALLOC ]
	SCENE_STATE_RUNNING,  // ���� �� [ Update / Render ]
	SCENE_STATE_PAUSING,  // �Ͻ� ���� �� [ Render ]
	SCENE_STATE_ENDING    // ���� �� [ Release ]
};

class CScene
{
public:
	CScene();
	virtual ~CScene();

	// Scene Initialization / Release
	virtual void InitializeObjects();
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
	ComPtr<ID3D12RootSignature> CreateRootSignature(ID3D12Device* pd3dDevice);

	// Shader Variables
	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();

private:
	SCENE_STATE m_SceneState = SCENE_STATE_NONE; // Scene State
};

////////////////////////////////////////////////////////////////////////////////////////////
//

class LoadingScene : public CScene
{
public:
	LoadingScene();
	virtual ~LoadingScene();

	// Scene Initialization / Release
	virtual void InitializeObjects() override;
	virtual void ReleaseObjects() override;
	virtual void ReleaseUploadBuffers() override;

	//virtual bool Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr) override;

	// Shader Variables
	//void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) override;
	//void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList) override;
	//void ReleaseShaderVariables() override;
};