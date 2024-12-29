///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-28
// Scene.h : Scene Ŭ������ ��� ����
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"
#include "GameObject.h"

class CCamera;

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
	bool CheckWorkRendering() { return (m_SceneState == SCENE_STATE_RUNNING)||(m_SceneState == SCENE_STATE_PAUSING); }
	bool CheckWorkUpdating() { return (m_SceneState == SCENE_STATE_RUNNING); }
	SCENE_STATE GetSceneState() { return m_SceneState; }
	void SetSceneState(SCENE_STATE SceneState) { m_SceneState = SceneState; }

	// Scene Method
	virtual void FixedUpdate(float deltaTime);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL);

	// Object Management
	void AddGameObject(std::shared_ptr<CGameObject>& pGameObject);
	void RemoveGameObject(std::shared_ptr<CGameObject>& pGameObject);

	// Shader Variables
	void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseShaderVariables();

private:
	SCENE_STATE m_SceneState; // Scene State

	std::vector<std::shared_ptr<CGameObject>> m_vecpGameObjects; // ���� ��ü���� �迭
	

};

