#pragma once

#include "Scene.h"

class CTestScene : public CScene
{
public:
	CTestScene();
	virtual ~CTestScene();

	virtual const std::wstring& GetSceneName() const override { static std::wstring scenename = L"CTestScene"; return scenename; }

	virtual void InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature) override;

	virtual void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	virtual bool ProcessMouseInput(float cxDelta, float cyDelta, float deltaTime) override;
	virtual bool ProcessKeyboardInput(const UCHAR pKeysBuffer[256], float deltaTime) override;

};

