/////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-28
// GameFramework.cpp : 게임 프레임워크 클래스입니다.
// Version : 0.1
/////////////////////////////////////////////////////////////////////////////
#include "GameFramework.h"

#include "TitleScene.h"

bool g_bRenderCollider = false;

CGameFramework* CGameFramework::pGameFramework = nullptr;

CGameFramework::CGameFramework()
{
	m_hInstance = NULL;
	m_hWnd = NULL;

	m_nWndClientWidth = 0;
	m_nWndClientHeight = 0;

	m_nSwapChainBufferIndex = 0;
	m_nFenceValues[0] = m_nFenceValues[1] = 0;

	m_nMsaa4xQualityLevels = 0;
	m_bMsaa4xEnable = false;

	m_hFenceEvent = nullptr;

	m_pd3dDevice = nullptr;
	m_pdxgiFactory = nullptr;
	m_pdxgiAdapter = nullptr;
	m_pdxgiSwapChain = nullptr;

	m_pd3dCommandQueue = nullptr;

	m_pd3dRtvDescriptorHeap = nullptr;
	m_pd3dDsvDescriptorHeap = nullptr;
	m_pd3dDepthStencilBuffer = nullptr;
	for (int i = 0; i < m_nSwapChainBuffers; ++i) {
		m_ppd3dSwapChainBackBuffers[i] = nullptr;
		m_pd3dCommandAllocator[i] = nullptr;
		m_pd3dCommandList[i] = nullptr;
	}

	m_pd3dFence = nullptr;
}

CGameFramework::~CGameFramework()
{
	OnDestroy();
}

bool CGameFramework::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hMainWnd;

	pGameFramework = this;

	CreateDirect3DDevice();
	CreateCommandQueueAndList();
	CreateRtvAndDsvDescriptorHeap();
	CreateSwapChain();
	CreateRenderTargetViews();
	CreateDepthStencilView();

	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	//CoInitialize(nullptr);

	// 씬들을 생성한다.
	BuildObjects();

	m_GameTimer.Reset();

	return true;
}

void CGameFramework::OnDestroy()
{
	if (!isWorkd) return;
	isWorkd = false;

	// 남은 Command List가 없는지 확인
	::WaitForGpuComplete(m_pd3dCommandQueue.Get(), m_pd3dFence.Get(), ++m_nFenceValues[m_nSwapChainBufferIndex], m_hFenceEvent);

	// Shader 변수들을 해제한다.
	ReleaseShaderVariables();

#ifdef _WITH_DIRECT_WRITE_UI
	if (m_pUILayer) m_pUILayer->ReleaseResources();
	m_pUILayer.reset();
#endif

	// Fence Event 객체를 해제한다.
	::CloseHandle(m_hFenceEvent);

	// Scene들을 해제한다.

	m_Scenes.clear();
	m_pLoadingScene.reset();

	CScene::DestroyFramework();

	// DirectX 12 자원들을 해제한다.
	if (m_pd3dDepthStencilBuffer) m_pd3dDepthStencilBuffer.Reset();
	if (m_pd3dDsvDescriptorHeap) m_pd3dDsvDescriptorHeap.Reset();

	for (int i = 0; i < m_nSwapChainBuffers; ++i) {	if (m_ppd3dSwapChainBackBuffers[i])	m_ppd3dSwapChainBackBuffers[i].Reset();	}
	if (m_pd3dRtvDescriptorHeap) m_pd3dRtvDescriptorHeap.Reset();

	for (int i = 0; i < m_nSwapChainBuffers; ++i)
	{
		if (m_pd3dCommandList[i]) m_pd3dCommandList[i].Reset();
		if (m_pd3dCommandAllocator[i])	m_pd3dCommandAllocator[i].Reset();
	}
	if (m_pd3dCommandQueue)	m_pd3dCommandQueue.Reset();

	if (m_pd3dFence)	m_pd3dFence.Reset();

	m_pdxgiSwapChain->SetFullscreenState(FALSE, NULL);
	if (m_pdxgiSwapChain) m_pdxgiSwapChain.Reset();
	if (m_pd3dDevice) m_pd3dDevice.Reset();
	if (m_pdxgiFactory) m_pdxgiFactory.Reset();
#ifdef _DEBUG
	if (m_pd3dDebugController) m_pd3dDebugController.Reset();
#endif
}

void CGameFramework::CreateDirect3DDevice()
{
	HRESULT hResult;

	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	// 디버그 레이어 생성
	ComPtr<ID3D12Debug> pd3dDebugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(pd3dDebugController.GetAddressOf()))))
	{
		pd3dDebugController->EnableDebugLayer();

		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	if (FAILED(::CreateDXGIFactory2(dxgiFactoryFlags, __uuidof(IDXGIFactory4), (void**)m_pdxgiFactory.GetAddressOf())))
	{
		return;
	}

	// 비디오 메모리가 가장 큰 Adapter 검색
	size_t maxVideoMemory = 0;

	for (UINT i = 0;; ++i) {
		ComPtr<IDXGIAdapter1> adapter;
		if (m_pdxgiFactory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND) {
			break; // 더 이상 Adapter가 없음
		}

		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		// Software Adapter는 무시
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			continue;
		}

		// 비디오 메모리 확인
		if (desc.DedicatedVideoMemory > maxVideoMemory) {
			maxVideoMemory = desc.DedicatedVideoMemory;
			m_pdxgiAdapter.Reset();
			m_pdxgiAdapter = adapter;
		}
	}

	// 선택된 Adpater가 없을 경우 Warp Adapter 사용
	if (!m_pdxgiAdapter) {
		m_pdxgiFactory->EnumWarpAdapter(_uuidof(IDXGIFactory4), (void**)&m_pdxgiAdapter);
		D3D12CreateDevice(m_pdxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pd3dDevice));
	}
	else {
		// 선택된 Adapter로 Device 생성
		FAILED(D3D12CreateDevice(m_pdxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_pd3dDevice)));
	}

	// MSAA Quality Level 확인
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMsaaQualityLevels.SampleCount = 4;   //Msaa4x 다중 샘플링
	d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMsaaQualityLevels.NumQualityLevels = 0;
	hResult = m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &d3dMsaaQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	
	// 디바이스가 지원하는 다중 샘플의 품질 수준을 확인한다. 
	m_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;

	// 다중 샘플의 품질 수준이 1보다 크면 다중 샘플링을 활성화한다. 
	m_bMsaa4xEnable = (m_nMsaa4xQualityLevels > 1) ? true : false;

	// Fence 생성
	if (SUCCEEDED(m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pd3dFence)))) {
		for (int i = 0; i < m_nFenceValues.size(); ++i)	{
			m_nFenceValues[i] = 0;	// Fence 초기화
		}

		m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		/* 펜스와 동기화를 위한 이벤트 객체를 생성한다(이벤트 객체의 초기값을 FALSE이다). 
		이벤트가 실행되면(Signal) 이벤트의 값을 자동적으로 FALSE가 되도록 생성한다.*/

		if (m_hFenceEvent == nullptr) {
			return;
		}
	}
	else {
		OutputDebugString(L"Failed to create fence\n");
	}

	// 뷰포트를 주 윈도우의 클라이언트 영역 전체로 설정한다. 
	m_d3dViewport.TopLeftX = 0;
	m_d3dViewport.TopLeftY = 0;
	m_d3dViewport.Width = static_cast<float>(m_nWndClientWidth);
	m_d3dViewport.Height = static_cast<float>(m_nWndClientHeight);
	m_d3dViewport.MinDepth = 0.0f;
	m_d3dViewport.MaxDepth = 1.0f;

	// 씨저 사각형을 주 윈도우의 클라이언트 영역 전체로 설정한다. 
	m_d3dScissorRect = { 0, 0, static_cast<long>(m_nWndClientWidth), static_cast<long>(m_nWndClientHeight) };

	// Device의 Descriptor Increment Size
	::gnCbvSrvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	::gnRtvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	::gnDsvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

}

void CGameFramework::CreateCommandQueueAndList()
{
	HRESULT hResult;

	// Command Queue 생성
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc;
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	hResult = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, IID_PPV_ARGS(&m_pd3dCommandQueue));
	if (FAILED(hResult)) {
		OutputDebugString(L"Failed to create command queue\n");
	}

	// Command Allocator 생성
	for (int i = 0; i < m_nSwapChainBuffers; ++i) {
		hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pd3dCommandAllocator[i]));
		if (FAILED(hResult)) {
			OutputDebugString(L"Failed to create command allocator\n");
		}
	}

	// Command List 생성
	for (int i = 0; i < m_nSwapChainBuffers; ++i) {
		hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator[i].Get(), nullptr, IID_PPV_ARGS(&m_pd3dCommandList[i]));
		if (FAILED(hResult)) {
			OutputDebugString(L"Failed to create command list\n");
		}
		// Command List를 닫음.
		CloseCommandList(m_pd3dCommandList[i].Get());
	}

	// Scene 생성용 Command Allocator와 Command List 생성
	{
		hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pd3dSceneMadeCommandAllocator));
		if (FAILED(hResult)) {
			OutputDebugString(L"Failed to create command allocator\n");
		}

		hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dSceneMadeCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_pd3dSceneMadeCommandList));
		if (FAILED(hResult)) {
			OutputDebugString(L"Failed to create command list\n");
		}
		CloseCommandList(m_pd3dSceneMadeCommandList.Get());
	}

}

void CGameFramework::CreateRtvAndDsvDescriptorHeap()
{
	HRESULT hResult;

	// SwapChain의 Back Buffer 개수만큼 RTV를 위한 Descriptor Heap 생성
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	d3dDescriptorHeapDesc.NumDescriptors = m_nSwapChainBuffers;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, IID_PPV_ARGS(&m_pd3dRtvDescriptorHeap));

	// Depth/Stencil Buffer를 위한 Descriptor Heap 생성
	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, IID_PPV_ARGS(&m_pd3dDsvDescriptorHeap));
}

void CGameFramework::CreateSwapChain()
{
	// Window Client 영역의 크기를 얻는다.
	RECT rcClient;
	::GetClientRect(m_hWnd, &rcClient);
	m_nWndClientWidth = rcClient.right - rcClient.left;
	m_nWndClientHeight = rcClient.bottom - rcClient.top;

	// Swap Chain 생성
	DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));
	dxgiSwapChainDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.Scaling = DXGI_SCALING_NONE;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	//dxgiSwapChainDesc.Flags = 0;
	//전체화면 모드에서 바탕화면의 해상도를 스왑체인(후면버퍼)의 크기에 맞게 변경한다. 
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Full Screen 모드 설정
	DXGI_SWAP_CHAIN_FULLSCREEN_DESC dxgiSwapChainFullScreenDesc;
	::ZeroMemory(&dxgiSwapChainFullScreenDesc, sizeof(DXGI_SWAP_CHAIN_FULLSCREEN_DESC));
	dxgiSwapChainFullScreenDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainFullScreenDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainFullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Windowed = TRUE;
	
	// Swap Chain 생성
	m_pdxgiFactory->CreateSwapChainForHwnd(m_pd3dCommandQueue.Get(), m_hWnd,&dxgiSwapChainDesc, &dxgiSwapChainFullScreenDesc, NULL, (IDXGISwapChain1**)m_pdxgiSwapChain.GetAddressOf());

	// Full Screen 모드에서 Alt+Enter 키를 통한 전체 화면 전환을 사용하지 않도록 설정
	m_pdxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);
		
	// Swap Chain의 Back Buffer 인덱스를 얻는다.
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE
	CreateRenderTargetViews();
#endif
}

void CGameFramework::CreateRenderTargetViews()
{
	// Swap Chain의 Back Buffer 개수만큼 RTV 생성
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < m_nSwapChainBuffers; i++)
	{
		m_pdxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_ppd3dSwapChainBackBuffers[i]));
		m_pd3dDevice->CreateRenderTargetView(m_ppd3dSwapChainBackBuffers[i].Get(), NULL, d3dRtvCPUDescriptorHandle);
		d3dRtvCPUDescriptorHandle.ptr += ::gnRtvDescriptorIncrementSize;
	}
}

void CGameFramework::CreateDepthStencilView()
{
	// Depth/Stencil Buffer 생성
	D3D12_RESOURCE_DESC d3dResourceDesc;
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = m_nWndClientWidth;
	d3dResourceDesc.Height = m_nWndClientHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dResourceDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	d3dResourceDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	::ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapProperties.CreationNodeMask = 1;
	d3dHeapProperties.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;

	m_pd3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dClearValue, IID_PPV_ARGS(&m_pd3dDepthStencilBuffer));

	// Depth/Stencil Buffer를 DSV로 사용
	D3D12_DEPTH_STENCIL_VIEW_DESC d3dDepthStencilViewDesc;
	::ZeroMemory(&d3dDepthStencilViewDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
	d3dDepthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dDepthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	d3dDepthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer.Get(), &d3dDepthStencilViewDesc, d3dDsvCPUDescriptorHandle);
}

void CGameFramework::ChangeSwapChainState()
{
	::WaitForGpuComplete(m_pd3dCommandQueue.Get(), m_pd3dFence.Get(), ++m_nFenceValues[m_nSwapChainBufferIndex], m_hFenceEvent);
	
	BOOL bFullScreenState = FALSE;
	m_pdxgiSwapChain->GetFullscreenState(&bFullScreenState, NULL);
	m_pdxgiSwapChain->SetFullscreenState(!bFullScreenState, NULL);

	DXGI_MODE_DESC dxgiTargetParameters;
	dxgiTargetParameters.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiTargetParameters.Width = m_nWndClientWidth;
	dxgiTargetParameters.Height = m_nWndClientHeight;
	dxgiTargetParameters.RefreshRate.Numerator = 60;
	dxgiTargetParameters.RefreshRate.Denominator = 1;
	dxgiTargetParameters.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiTargetParameters.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	m_pdxgiSwapChain->ResizeTarget(&dxgiTargetParameters);

	for (int i = 0; i < m_nSwapChainBuffers; i++) if (m_ppd3dSwapChainBackBuffers[i]) m_ppd3dSwapChainBackBuffers[i].Reset();

	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	m_pdxgiSwapChain->ResizeBuffers(m_nSwapChainBuffers, m_nWndClientWidth,
		m_nWndClientHeight, dxgiSwapChainDesc.BufferDesc.Format, dxgiSwapChainDesc.Flags);

	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	CreateRenderTargetViews();

}

void CGameFramework::BuildObjects()
{
#ifdef _WITH_DIRECT_WRITE_UI
	m_pUILayer = std::make_shared<UILayer>(m_nSwapChainBuffers, 3, m_pd3dDevice.Get(), m_pd3dCommandQueue.Get(), m_ppd3dSwapChainBackBuffers.data()->GetAddressOf(), m_nWndClientWidth, m_nWndClientHeight);

	ComPtr<ID2D1SolidColorBrush> pd2dBrush = m_pUILayer->CreateBrush(D2D1::ColorF(D2D1::ColorF::Purple, 1.0f));
	ComPtr<IDWriteTextFormat> pdwTextFormat = m_pUILayer->CreateTextFormat(L"궁서체", m_nWndClientHeight / 15.0f);
	D2D1_RECT_F d2dRect = D2D1::RectF(0.0f, 0.0f, (float)m_nWndClientWidth, (float)m_nWndClientHeight);

	m_pUILayer->StorePoolTextBlock(0, NULL, &d2dRect, pdwTextFormat.Get(), pd2dBrush.Get());

	pd2dBrush = m_pUILayer->CreateBrush(D2D1::ColorF(D2D1::ColorF::BlueViolet, 1.0f));
	pdwTextFormat = m_pUILayer->CreateTextFormat(L"Arial", m_nWndClientHeight / 25.0f);
	d2dRect = D2D1::RectF(0.0f, m_nWndClientHeight - 75.0f, (float)m_nWndClientWidth, (float)m_nWndClientHeight);

	m_pUILayer->StorePoolTextBlock(1, NULL, &d2dRect, pdwTextFormat.Get(), pd2dBrush.Get());

	pd2dBrush = m_pUILayer->CreateBrush(D2D1::ColorF(D2D1::ColorF::Black, 1.0f));
	pdwTextFormat = m_pUILayer->CreateTextFormat(L"Bahnschrift Condensed", m_nWndClientHeight / 25.0f);
	m_pUILayer->StorePoolTextBlock(2, NULL, &d2dRect, pdwTextFormat.Get(), pd2dBrush.Get());
#endif

	m_pd3dCommandAllocator[m_nSwapChainBufferIndex]->Reset();
	m_pd3dCommandList[m_nSwapChainBufferIndex]->Reset(m_pd3dCommandAllocator[m_nSwapChainBufferIndex].Get(), nullptr);

	// 모든 씬이 공유할 요소 생성
	CScene::InitStaticMembers(m_pd3dDevice.Get(), m_pd3dCommandList[m_nSwapChainBufferIndex].Get());
	CResourceManager::GetInstance().Initialize(m_pd3dDevice.Get(), m_pd3dCommandList[m_nSwapChainBufferIndex].Get(), nullptr);
	CResourceManager::GetInstance().SetCommandList(m_pd3dSceneMadeCommandList.Get());
	// Title Scene 생성
	std::shared_ptr<CScene> pTitleScene = std::make_unique<CTitleScene>();
	pTitleScene->Init(m_pd3dDevice.Get(), m_pd3dCommandList[m_nSwapChainBufferIndex].Get());
	pTitleScene->SetSceneState(SCENE_STATE_RUNNING);
	m_Scenes.push_back(std::move(pTitleScene));

	// Command List에 대한 명령들을 종료
	CloseCommandList(m_pd3dCommandList[m_nSwapChainBufferIndex].Get());

	{
		// Command Queue에 Command List를 추가
		ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList[m_nSwapChainBufferIndex].Get() };
		m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);
	}

	// Command Queue의 명령들이 모두 실행될 때까지 대기
	::WaitForGpuComplete(m_pd3dCommandQueue.Get(), m_pd3dFence.Get(), ++m_nFenceValues[m_nSwapChainBufferIndex], m_hFenceEvent);

	m_pd3dCommandAllocator[m_nSwapChainBufferIndex]->Reset();
	m_pd3dCommandList[m_nSwapChainBufferIndex]->Reset(m_pd3dCommandAllocator[m_nSwapChainBufferIndex].Get(), nullptr);
	
	// Framework 정보 생성 (Shader에 전달할 정보)
	CreateShaderVariables();

	// GameScene 생성
	//AddScene("CGameScene");

	// LoadingScene 생성
	std::unique_ptr<CScene> pLoadingScene = std::make_unique<CLoadingScene>();
	pLoadingScene->Init(m_pd3dDevice.Get(), m_pd3dCommandList[m_nSwapChainBufferIndex].Get());
	pLoadingScene->SetSceneState(SCENE_STATE_RUNNING);
	m_pLoadingScene = std::move(pLoadingScene);
	

	// Command List에 대한 명령들을 종료
	CloseCommandList(m_pd3dCommandList[m_nSwapChainBufferIndex].Get());

	// Command Queue에 Command List를 추가
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList[m_nSwapChainBufferIndex].Get() };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	// Command Queue의 명령들이 모두 실행될 때까지 대기
	::WaitForGpuComplete(m_pd3dCommandQueue.Get(), m_pd3dFence.Get(), ++m_nFenceValues[m_nSwapChainBufferIndex], m_hFenceEvent);

	if (m_pLoadingScene) m_pLoadingScene->ReleaseUploadBuffers();
}

void CGameFramework::CreateSceneOnAnotherThread(std::string sceneName)
{
	std::thread thread([this, sceneName]() mutable {
		ComPtr<ID3D12Fence>		pd3dFence;
		UINT64					nFenceValue;
		HANDLE					hFenceEvent;

		nFenceValue = 0;
		hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		if (SUCCEEDED(m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pd3dFence)))) {}

		//std::shared_ptr<CScene> pCreatedScene = std::make_unique<CGameScene>();
		std::shared_ptr<CScene> pCreatedScene;
		if(sceneName == "CGameScene")
		{
			pCreatedScene = std::make_shared<CGameScene>();
		}
		else if (sceneName == "COnlineScene")
		{
			pCreatedScene = std::make_shared<COnlineScene>();
		}
		else {
			return; // 알 수 없는 씬 이름인 경우
		}
		//std::shared_ptr<CScene> pCreatedScene = std::make_unique<COnlineScene>();
		m_Scenes.push_back(pCreatedScene);

		m_pd3dSceneMadeCommandAllocator->Reset();
		m_pd3dSceneMadeCommandList->Reset(m_pd3dSceneMadeCommandAllocator.Get(), nullptr);

		pCreatedScene->Init(m_pd3dDevice.Get(), m_pd3dSceneMadeCommandList.Get());

		{
			std::string debugstr = "m_pd3dSceneMadeCommandList->Close\n";
			OutputDebugStringA(debugstr.c_str());
		}
		CloseCommandList(m_pd3dSceneMadeCommandList.Get());

		ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dSceneMadeCommandList.Get() };
		m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

		//WaitGpuWithoutPresent();
		++nFenceValue;
		HRESULT hResult = m_pd3dCommandQueue->Signal(pd3dFence.Get(), nFenceValue);

		if (pd3dFence->GetCompletedValue() < nFenceValue)
		{
			hResult = pd3dFence->SetEventOnCompletion(nFenceValue, hFenceEvent);
			HRESULT hResult = pd3dFence->SetEventOnCompletion(nFenceValue, hFenceEvent);
			::WaitForSingleObject(hFenceEvent, INFINITE);
		}

		CloseHandle(hFenceEvent);
		pd3dFence.Reset();

		if (pCreatedScene) pCreatedScene->ReleaseUploadBuffers();

		pCreatedScene->PostInitializeObjects(nullptr, nullptr, nullptr);
		pCreatedScene->SetSceneState(SCENE_STATE_RUNNING);
		});
	thread.detach();
}

void CGameFramework::AdvanceFrame()
{
	// 타이머 업데이트
	m_GameTimer.Tick(60.0f);

	// 이번 프레임에 작업할	Scene 결정
	CScene* pCurrentScene{ m_pLoadingScene.get() };
	if (m_Scenes.empty() == false && m_Scenes.back()->CheckWorkUpdating()) {
		pCurrentScene = m_Scenes.back().get();
	}
	
	// Input 업데이트
	ProcessInput(pCurrentScene);

	// Scene Container 업데이트
	for (auto it = m_Scenes.begin(); it != m_Scenes.end(); ) {
		if (it->get()->GetSceneState() == SCENE_STATE_ENDING) {
			{
				std::string debug = typeid(*it).name();
				debug += "[AdvanceFrame] Scene Ending\n";
				OutputDebugStringA(debug.c_str());
			}
			it = m_Scenes.erase(it);  // erase는 다음 유효 반복자를 반환

			if (m_Scenes.empty() == false && m_Scenes.back()->CheckWorkUpdating()) {
				pCurrentScene = m_Scenes.back().get();
			}
			else {
				pCurrentScene = m_pLoadingScene.get();
			}
		}
		else {
			++it;
		}
	}

	// 현재 Scene이 없으면 종료
	if(m_Scenes.empty())
	{
		PostQuitMessage(0);
		return;
	}

	// Command List 재사용
	ID3D12CommandAllocator* pCommandAllocator = m_pd3dCommandAllocator[m_nSwapChainBufferIndex].Get();
	ID3D12GraphicsCommandList* pd3dCommandList = m_pd3dCommandList[m_nSwapChainBufferIndex].Get();

	pCommandAllocator->Reset();
	pd3dCommandList->Reset(pCommandAllocator, nullptr);

	// Update
	AnimateObjects(pCurrentScene);

	pCurrentScene->PrepareRender(pd3dCommandList);
	pCurrentScene->OnPreRender(pd3dCommandList);

	// Framework 정보 업데이트
	UpdateShaderVariables();

	// Swap Chain의 Back Buffer를 렌더 타겟으로 사용
	::SynchronizeResourceTransition(pd3dCommandList, m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Swap Chain의 Back Buffer를 렌더 타겟으로 사용
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	d3dRtvCPUDescriptorHandle.ptr += m_nSwapChainBufferIndex * ::gnRtvDescriptorIncrementSize;
	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	pd3dCommandList->OMSetRenderTargets(1, &d3dRtvCPUDescriptorHandle, TRUE, &d3dDsvCPUDescriptorHandle);
	pd3dCommandList->ClearRenderTargetView(d3dRtvCPUDescriptorHandle, Colors::SteelBlue, 0, nullptr);
	pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Scene 렌더링
	pCurrentScene->Render(pd3dCommandList, nullptr);

	// UI 렌더링
	pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	pCurrentScene->RenderUI(pd3dCommandList, nullptr);

	// 커서 렌더링
	if (g_bWindowActive && g_bEnableCursor) { RenderCursor(pd3dCommandList); }

	// Command List에 대한 명령들을 종료
	::SynchronizeResourceTransition(pd3dCommandList, m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	::ExecuteCommandList(pd3dCommandList, m_pd3dCommandQueue.Get(), m_pd3dFence.Get(), ++m_nFenceValues[m_nSwapChainBufferIndex], m_hFenceEvent);
	
#ifdef _WITH_DIRECT_WRITE_UI
	m_pUILayer->Render(m_nSwapChainBufferIndex);
#endif

	pCurrentScene->OnPostRender(nullptr);

	// Swap Chain의 Back Buffer를 화면에 표시
	m_pdxgiSwapChain->Present(0, 0);
	++g_nFrameCount;

	// 다음 Frame으로 이동
	MoveToNextFrame();

	// Time / FPS 출력
	std::wstring time = L"Time: " + std::to_wstring(m_GameTimer.GameTime());
	std::wstring fps = L"FPS: " + std::to_wstring(m_GameTimer.calculateAverageFPS());

	std::string playerPostion = "Player Position: ";
	std::wstring text = time + L" " + fps;

	if (auto pPlayer = pCurrentScene->GetPlayer()) {
		XMFLOAT3 playerPosition = pPlayer->GetPosition();
		char move_input = pPlayer->GetMoveInput();
		text += L"( " + std::to_wstring(playerPosition.x) + L", " + std::to_wstring(playerPosition.y) + L", " + std::to_wstring(playerPosition.z) + L")";
		text += L"GetMoveInput( " + std::to_wstring(move_input & DIR_FORWARD? 1 : 0) + L", " + std::to_wstring(move_input & DIR_BACKWARD ? 1 : 0) + L", " + std::to_wstring(move_input & DIR_LEFT ? 1 : 0) + L", " + std::to_wstring(move_input & DIR_RIGHT ? 1 : 0) + L")";
	}

	::SetWindowText(m_hWnd, text.c_str());
}

void CGameFramework::AnimateObjects(CScene* pScene)
{
	// Scene 정보 업데이트
	pScene->Update(m_GameTimer.DeltaTime());
}

POINTF CGameFramework::GetTexturePosition(int x, int y) {
	POINTF pt;
	pt.x = (float)x / (float)m_nWndClientWidth * 2.0f - 1.0f;
	pt.y = (float)y / (float)m_nWndClientHeight * 2.0f - 1.0f;
	
	return pt;
}

void CGameFramework::RenderCursor(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (!m_pCursorSprite) {
		m_pCursorSprite = std::make_shared<CSprite>();
		m_pCursorSprite->Initialize(m_pd3dDevice.Get(), pd3dCommandList);

		std::shared_ptr<CShader> pUIShader = std::make_shared<CTextureToViewportShader>(nullptr);
		pUIShader->CreateShader(m_pd3dDevice.Get(), CScene::GetGraphicsRootSignature().Get());

		std::shared_ptr<CMesh> pRectangleMesh = CResourceManager::GetInstance().GetMesh("UI");

		auto cursorTexture = CResourceManager::GetInstance().GetTexture("Cursor");
		if(nullptr == cursorTexture)
		{
			cursorTexture = std::make_shared<CTexture>(1, RESOURCE_TEXTURE2D, 1);
			cursorTexture->LoadTextureFromDDSFile(m_pd3dDevice.Get(), pd3dCommandList, L"Image/cursor.dds", RESOURCE_TEXTURE2D, 0);
			CScene::CreateShaderResourceViews(m_pd3dDevice.Get(), cursorTexture.get(), 0, ROOT_PARAMETER_STANDARD_TEXTURES);
			CResourceManager::GetInstance().SetTexture("Cursor", cursorTexture);
		}

		std::shared_ptr<CMaterial> pCursorMaterial = std::make_shared<CMaterial>();
		pCursorMaterial->SetTexture(cursorTexture);
		pCursorMaterial->SetShader(pUIShader);

		m_pCursorSprite->SetMesh(pRectangleMesh);
		m_pCursorSprite->MaterialResize(1);
		m_pCursorSprite->SetMaterial(0, pCursorMaterial);
	}

	POINT ptCursorPos;
	GetCursorPos(&ptCursorPos);
	ScreenToClient(m_hWnd, &ptCursorPos);
	POINTF p = GetTexturePosition(ptCursorPos.x, ptCursorPos.y);
	m_pCursorSprite->SetSize(p.x, -p.y, 0.05f, 0.05f);

	if(GetCapture() == m_hWnd)
	{
		// 마우스 커서가 캡처된 상태일 때
		m_pCursorSprite->SetColor({ 0.5f, 0.5f, 0.5f, 1.0f });
	}
	else
	{
		m_pCursorSprite->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}

	m_pCursorSprite->Render(pd3dCommandList, nullptr);
}

void CGameFramework::MoveToNextFrame()
{
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
	//m_nSwapChainBufferIndex = (m_nSwapChainBufferIndex + 1) % m_nSwapChainBuffers;

	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence.Get(), nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void CGameFramework::CreateShaderVariables()
{
	UINT ncbElementBytes = ((sizeof(CB_FRAMEWORK_INFO) + 255) & ~255); //256의 배수
	m_pd3dcbFrameworkInfo = ::CreateBufferResource(m_pd3dDevice.Get(), m_pd3dCommandList[m_nSwapChainBufferIndex].Get(), NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_GENERIC_READ, NULL);

	m_pd3dcbFrameworkInfo->Map(0, NULL, (void**)&m_pcbMappedFrameworkInfo);
	ZeroMemory(m_pcbMappedFrameworkInfo, sizeof(CB_FRAMEWORK_INFO));
}

void CGameFramework::UpdateShaderVariables()
{
	m_pcbMappedFrameworkInfo->m_fCurrentTime = m_GameTimer.GameTime();
	m_pcbMappedFrameworkInfo->m_fElapsedTime = m_GameTimer.DeltaTime();
	m_pcbMappedFrameworkInfo->m_nRenderMode = g_bRenderCollider ? 1 : 0;
	m_pcbMappedFrameworkInfo->m_fBias = m_fBias;

	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = m_pd3dcbFrameworkInfo->GetGPUVirtualAddress();
	m_pd3dCommandList[m_nSwapChainBufferIndex]->SetGraphicsRootConstantBufferView(ROOT_PARAMETER_FRAMEWORK, d3dGpuVirtualAddress);
}

void CGameFramework::ReleaseShaderVariables()
{
	if (m_pd3dcbFrameworkInfo)
	{
		m_pd3dcbFrameworkInfo->Unmap(0, NULL);
		m_pd3dcbFrameworkInfo.Reset();
	}
}

void CGameFramework::ProcessInput(CScene* pScene)
{
	static INPUT_PARAMETER pInputBuffer;
	static UCHAR pKeysBuffer[256];
	float cxDelta = 0.0f, cyDelta = 0.0f;
	bool bProcessedByScene = false;

	if (false == g_bWindowActive) return;
	ZeroMemory(&pKeysBuffer, sizeof(UCHAR) * 256);

	pScene->SetCursor();

	GetKeyboardState(pKeysBuffer);
	if (nullptr != pScene) bProcessedByScene = pScene->ProcessKeyboardInput(pKeysBuffer, m_GameTimer.DeltaTime()) ? true : false;
	
	// 마우스 입력 처리
	{
		POINT ptCursorPos;
		if (GetCapture() == m_hWnd)
		{
			SetCursor(NULL);			
		}

		GetCursorPos(&ptCursorPos);
		cxDelta = (float)(ptCursorPos.x - m_ptOldCursorPos.x) / 3.0f;
		cyDelta = (float)(ptCursorPos.y - m_ptOldCursorPos.y) / 3.0f;
		if (g_bEnableCursor)
		{
			m_ptOldCursorPos = ptCursorPos;
		}
		else {
			m_ptOldCursorPos = WindowCursor::GetClientCenter(m_hWnd);
			SetCursorPos(m_ptOldCursorPos.x, m_ptOldCursorPos.y);
		}

		if (nullptr != pScene) bProcessedByScene = pScene->ProcessMouseInput(cxDelta, cyDelta, m_GameTimer.DeltaTime()) ? true : false;
	}

	if (false == bProcessedByScene) {
		// Keyboard 입력 처리s
		DWORD dwDirection = 0;
		if (pInputBuffer.pKeysBuffer[VK_UP] & 0xF0)dwDirection |= DIR_FORWARD;
		if (pInputBuffer.pKeysBuffer[VK_DOWN] & 0xF0)dwDirection |= DIR_BACKWARD;
		if (pInputBuffer.pKeysBuffer[VK_LEFT] & 0xF0)dwDirection |= DIR_LEFT;
		if (pInputBuffer.pKeysBuffer[VK_RIGHT] & 0xF0)dwDirection |= DIR_RIGHT;
		if (pInputBuffer.pKeysBuffer[VK_PRIOR] & 0xF0)dwDirection |= DIR_UP;
		if (pInputBuffer.pKeysBuffer[VK_NEXT] & 0xF0)dwDirection |= DIR_DOWN;

		if ((dwDirection != 0) || (pInputBuffer.cxDelta != 0.0f) || (pInputBuffer.cyDelta != 0.0f))
		{
			/*if (pInputBuffer.cxDelta || pInputBuffer.cyDelta)
			{
				if (pKeysBuffer[VK_RBUTTON] & 0xF0)
					m_pPlayer->Rotate(pInputBuffer.cyDelta, 0.0f, -pInputBuffer.cxDelta);
				else
					m_pPlayer->Rotate(pInputBuffer.cyDelta, pInputBuffer.cxDelta, 0.0f);
			}
			if (dwDirection) m_pPlayer->Move(dwDirection, 50.0f * m_GameTimer.GetTimeElapsed(), true);*/
		}
	}
}

void CGameFramework::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	// 마우스 메시지 처리
	{
		auto& scene = m_Scenes.back();
		if(scene != nullptr && scene->CheckWorkUpdating()){
			scene->OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
		}
		else {
			m_pLoadingScene->OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
		}
	}
	
	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	{
		::SetFocus(hWnd);
		::SetCapture(hWnd);
		::GetCursorPos(&m_ptOldCursorPos);
		break;
	}
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	{
		::ReleaseCapture();
		break;
	}
	case WM_MOUSEMOVE:
		break;
	default:
		break;
	};
}

void CGameFramework::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	bool bProcessed = false;
	// 키보드 메시지 처리
	if(m_Scenes.size()){
		m_Scenes.back()->OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
	}

	if (m_Scenes.empty() || bProcessed) {
		m_pLoadingScene->OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
		return;
	}

	switch (nMessageID) {
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case VK_F1:
			g_bRenderCollider = !g_bRenderCollider;
			break;
		case VK_F9:
			//“F9” 키가 눌려지면 윈도우 모드와 전체화면 모드의 전환을 처리한다.
			ChangeSwapChainState();
			break;
		/*case VK_F5:
			m_fBias += 0.001f;
			{
				std::string debug = "[OnProcessingKeyboardMessage] Bias = " + std::to_string(m_fBias) + "\n";
				OutputDebugStringA(debug.c_str());
			}
			break;
		case VK_F6:
			m_fBias -= 0.001f;
			if (m_fBias < 0.0f) m_fBias = 0.0f;
			{
				std::string debug = "[OnProcessingKeyboardMessage] Bias = " + std::to_string(m_fBias) + "\n";
				OutputDebugStringA(debug.c_str());
			}
			break;*/
		default:
			break;
		}
		break;
	}
	}
}

LRESULT CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{

	switch (nMessageID)
	{
	case WM_ACTIVATE:
	{
		if (LOWORD(wParam) == WA_INACTIVE) 
		{
			//m_GameTimer.Stop();
			g_bWindowActive = false;
			WindowCursor::SetCursorVisibility(true);
		}
		else 
		{
			//m_GameTimer.Start();
			g_bWindowActive = true;
			WindowCursor::SetCursorVisibility(false);
		}
		break;
	}
	case WM_SIZE:
		break;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
		OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
		break;
	}
	return(0);
}

void CGameFramework::AddScene(std::string sceneName)
{
	CreateSceneOnAnotherThread(sceneName);
}

void CGameFramework::PopScene()
{
	if (!m_Scenes.empty())
	{
		m_Scenes.back()->SetSceneState(SCENE_STATE_ENDING);
	}
}
