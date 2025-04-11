/////////////////////////////////////////////////////////////////////////////
// 2024-12-28
// GameFramework.h : 게임 프레임워크 클래스입니다.
// Version : 0.1
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"
#include "GameTimer.h"
#include "Scene.h"

#include "GameScene.h"
#include "LoadingScene.h"


struct CB_FRAMEWORK_INFO
{
	float					m_fCurrentTime;
	float					m_fElapsedTime;
	//float					m_fSecondsPerFirework = 1.0f;
	//int					m_nFlareParticlesToEmit = 300;
	//XMFLOAT3				m_xmf3Gravity = XMFLOAT3(0.0f, -9.8f, 0.0f);
	//int					m_nMaxFlareType2Particles = 150;
	UINT					m_nRenderMode;
	float					m_nPadding;
};

class CGameFramework
{
public:
	CGameFramework();
	~CGameFramework();

	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	void OnDestroy();

	void CreateDirect3DDevice();
	void CreateCommandQueueAndList();
	void CreateRtvAndDsvDescriptorHeap();
	void CreateSwapChain();
	void CreateRenderTargetView();
	void CreateDepthStencilView();

	void BuildObjects();

	void AdvanceFrame();
	void OMSetBackBuffer();
	void ClearRtvAndDsv();

	void WaitForGpuComplete();
	void MoveToNextFrame();

	void CreateShaderVariables();
	void UpdateShaderVariables();
	void ReleaseShaderVariables();

	void ProcessInput();

	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

private:
	HINSTANCE m_hInstance;
	HWND m_hWnd;

	UINT m_nWndClientWidth;
	UINT m_nWndClientHeight;

	ComPtr<ID3D12Device>		m_pd3dDevice;
	ComPtr<IDXGIFactory4>		m_pdxgiFactory;
	ComPtr<IDXGIAdapter1>		m_pdxgiAdapter;
	ComPtr<IDXGISwapChain3>		m_pdxgiSwapChain;

	bool						m_bMsaa4xEnable = false;
	UINT						m_nMsaa4xQualityLevels = 0;

	static const UINT			m_nSwapChainBuffers = 2;
	UINT						m_nSwapChainBufferIndex;

	std::array<ComPtr<ID3D12Resource>, m_nSwapChainBuffers> m_ppd3dSwapChainBackBuffers;
	ComPtr<ID3D12DescriptorHeap>							m_pd3dRtvDescriptorHeap;

	ComPtr<ID3D12Resource>									m_pd3dDepthStencilBuffer;
	ComPtr<ID3D12DescriptorHeap>							m_pd3dDsvDescriptorHeap;

	ComPtr<ID3D12CommandQueue>								m_pd3dCommandQueue;
	ComPtr<ID3D12CommandAllocator>							m_pd3dCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList>						m_pd3dCommandList;

	ComPtr<ID3D12Fence>										m_pd3dFence;
	std::array<UINT64, m_nSwapChainBuffers>					m_nFenceValues;
	HANDLE													m_hFenceEvent;

#if defined(_DEBUG)
	ComPtr<ID3D12Debug>										m_pd3dDebugController;
#endif

	D3D12_VIEWPORT											m_d3dViewport;
	D3D12_RECT												m_d3dScissorRect;

	// Timer
	CGameTimer												m_GameTimer;

	// Scene
	std::list<std::unique_ptr<CScene>>						m_Scenes;
	std::unique_ptr<CScene>									m_pLoadingScene;  // Loading Scene은 Stack이 비었을 경우에만 사용(이는, Render State인 Scene이 없을 때도 포함한다)
	/*
	Scene을 Stack으로 관리하여 가장 마지막 Scene에 대해서 Input 처리를 수행.
	나머지 Scene에 대해서는 Scene의 State에 따라 Update / Render를 수행.
	*/

	POINT m_ptOldCursorPos;


protected:
	ComPtr<ID3D12Resource> m_pd3dcbFrameworkInfo;
	CB_FRAMEWORK_INFO* m_pcbMappedFrameworkInfo = NULL;
};

