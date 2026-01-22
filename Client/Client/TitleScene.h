#pragma once

#include "Scene.h"
#include "Sprite.h"

class CTitleScene : public CScene
{
public:
	CTitleScene();
	virtual ~CTitleScene();
	
	// Scene Initialization / Release
	virtual void InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature) override;
	virtual void ReleaseObjects() override;
	virtual void ReleaseUploadBuffers() override;

	// UI Interaction
	virtual void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;
	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;

private:
	CSprite* m_pBackgroundObject;
	CSprite* m_pStartButton;
	CSprite* m_pExitButton;
};

