#pragma once
#include "Scene.h"
class CVictoryScene : public CScene
{
public:
	CVictoryScene();
	virtual ~CVictoryScene();

	virtual const std::wstring& GetSceneName() const override { static std::wstring scenename = L"CVictoryScene"; return scenename; }

	virtual void InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature) override;

	virtual void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

};

