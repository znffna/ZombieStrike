///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-28
// Scene.h : Scene 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"
#include "GameObject.h"

class CCamera;

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

	std::vector<std::shared_ptr<CGameObject>> m_vecpGameObjects; // 게임 객체들의 배열
	

};

