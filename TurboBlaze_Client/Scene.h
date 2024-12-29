///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-28
// Scene.h : Scene 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"

class CCamera;
class CGameObject;

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
	virtual void FixedUpdate();
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL);

	// Object Management
	void AddGameObject(std::shared_ptr<CGameObject>& pGameObject);
	void RemoveGameObject(std::shared_ptr<CGameObject>& pGameObject);

	// Shader Variables
	void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseShaderVariables();

private:
	std::vector<std::shared_ptr<CGameObject>> m_vecpGameObjects; // 게임 객체들의 배열
	

};

