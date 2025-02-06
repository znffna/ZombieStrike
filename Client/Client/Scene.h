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

#define MAX_LIGHTS 16

#define POINT_LIGHT			1
#define SPOT_LIGHT			2
#define DIRECTIONAL_LIGHT	3

struct Light
{
	XMFLOAT4				m_xmf4Ambient;
	XMFLOAT4				m_xmf4Diffuse;
	XMFLOAT4				m_xmf4Specular;
	XMFLOAT3				m_xmf3Position;
	float 					m_fFalloff;
	XMFLOAT3				m_xmf3Direction;
	float 					m_fTheta; //cos(m_fTheta)
	XMFLOAT3				m_xmf3Attenuation;
	float					m_fPhi; //cos(m_fPhi)
	bool					m_bEnable;
	int						m_nType;
	float					m_fRange;
	float					padding;
};

struct CB_LIGHT_INFO
{
	Light					m_pLights[MAX_LIGHTS];
	XMFLOAT4				m_xmf4GlobalAmbient;
	int						m_nLights;
};

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
	virtual void InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature = nullptr);
	virtual void ReleaseObjects();
	virtual void ReleaseUploadBuffers();

	void BuildDefaultLightsAndMaterials();

	// Scene Management
	bool CheckWorkRendering() { return (m_SceneState == SCENE_STATE_RUNNING) || (m_SceneState == SCENE_STATE_PAUSING); }
	bool CheckWorkUpdating() { return (m_SceneState == SCENE_STATE_RUNNING); }
	SCENE_STATE GetSceneState() { return m_SceneState; }
	void SetSceneState(SCENE_STATE SceneState) { m_SceneState = SceneState; }

	// Scene Method
	virtual void FixedUpdate(float deltaTime);
	void PrepareRender(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual bool Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr);

	// Shader method
	ComPtr<ID3D12RootSignature> CreateGraphicsRootSignature(ID3D12Device* pd3dDevice);

	// Shader Variables
	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();

	// Input Method
	virtual void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) {}
	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) {}

protected:
	SCENE_STATE m_SceneState = SCENE_STATE_NONE; // Scene State

	// Shader Variables
	ComPtr<ID3D12RootSignature> m_pd3dGraphicsRootSignature;
	ComPtr<ID3D12RootSignature> m_pd3dComputeRootSignature;

	// Light
	XMFLOAT4 m_xmf4GlobalAmbient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);

	std::array<Light, MAX_LIGHTS> m_pLights;
	ComPtr<ID3D12Resource> m_pd3dcbLights;
	CB_LIGHT_INFO* m_pcbMappedLights = nullptr;

	// GameObjects
	std::vector<std::shared_ptr<CGameObject>> m_ppObjects;

	// Camera
	std::shared_ptr<CCamera> m_pCamera;
};

////////////////////////////////////////////////////////////////////////////////////////////
//

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


	//virtual bool Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr) override;

	// Shader Variables
	//void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) override;
	//void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList) override;
	//void ReleaseShaderVariables() override;
};