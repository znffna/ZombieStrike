/////////////////////////////////////////////////////////////////////////////
// 2024-12-28
// GameFramework.h : 게임 프레임워크 클래스입니다.
// Version : 0.1
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"
#include "GameTimer.h"
#include "Scene.h"

class CGameFramework
{
public:
	CGameFramework();
	~CGameFramework();

	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);

	void CreateDirect3DDevice();
	void CreateCommandQueueAndList();
	void CreateRtvAndDsvDescriptorHeap();
	void CreateSwapChain();
	void CreateRenderTargetView();
	void CreateDepthStencilView();

	void FixedUpdate();

	void WaitForGpuComplete();
	void MoveToNextFrame();

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

	// Scene Management
	std::vector<std::shared_ptr<CScene>> m_vecpScenes;

};

