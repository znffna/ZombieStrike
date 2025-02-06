/////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-28
// GameFramework.cpp : 게임 프레임워크 클래스입니다.
// Version : 0.1
/////////////////////////////////////////////////////////////////////////////
#include "GameFramework.h"

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
	m_pd3dCommandAllocator = nullptr;
	m_pd3dCommandList = nullptr;

	m_pd3dRtvDescriptorHeap = nullptr;
	m_pd3dDsvDescriptorHeap = nullptr;
	m_pd3dDepthStencilBuffer = nullptr;
	for (int i = 0; i < m_nSwapChainBuffers; ++i) {
		m_ppd3dSwapChainBackBuffers[i] = nullptr;
	}

	m_pd3dFence = nullptr;
}

CGameFramework::~CGameFramework()
{
	

	// DirectX 12 자원들을 해제한다.
	if (m_hFenceEvent != nullptr) {
		CloseHandle(m_hFenceEvent);
	}
	if (m_pd3dFence)	m_pd3dFence.Reset();

	if (m_pd3dCommandList)	m_pd3dCommandList.Reset();
	if (m_pd3dCommandAllocator)	m_pd3dCommandAllocator.Reset();
	if (m_pd3dCommandQueue)	m_pd3dCommandQueue.Reset();

	if (m_pd3dDsvDescriptorHeap) m_pd3dDsvDescriptorHeap.Reset();
	if (m_pd3dDepthStencilBuffer) m_pd3dDepthStencilBuffer.Reset();
	if (m_pd3dRtvDescriptorHeap) m_pd3dRtvDescriptorHeap.Reset();
	for (int i = 0; i < m_nSwapChainBuffers; ++i) {
		if (m_ppd3dSwapChainBackBuffers[i])	m_ppd3dSwapChainBackBuffers[i].Reset();
	}

	if (m_pdxgiSwapChain)	m_pdxgiSwapChain.Reset();
	if (m_pdxgiAdapter)	m_pdxgiAdapter.Reset();
	if (m_pdxgiFactory)	m_pdxgiFactory.Reset();

	if (m_pd3dDevice)	m_pd3dDevice.Reset();
}

bool CGameFramework::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hMainWnd;

	CreateDirect3DDevice();
	CreateCommandQueueAndList();
	CreateRtvAndDsvDescriptorHeap();
	CreateSwapChain();
	CreateRenderTargetView();
	CreateDepthStencilView();

	BuildObjects();

	m_GameTimer.Reset();

	return true;
}

void CGameFramework::OnDestroy()
{
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
		hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pd3dCommandAllocator));
		if (FAILED(hResult)) {
			OutputDebugString(L"Failed to create command allocator\n");
		}
	}

	// Command List 생성
	hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_pd3dCommandList));
	if (FAILED(hResult)) {
		OutputDebugString(L"Failed to create command list\n");
	}

	// Command List를 초기화
	m_pd3dCommandList->Close();
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
	dxgiSwapChainDesc.Flags = 0;

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
}

void CGameFramework::CreateRenderTargetView()
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

void CGameFramework::BuildObjects()
{
	m_pd3dCommandAllocator->Reset();
	m_pd3dCommandList->Reset(m_pd3dCommandAllocator.Get(), nullptr);

	// Framework 정보 생성 (Shader에 전달할 정보)
	CreateShaderVariables();

	// Scene 생성
	std::unique_ptr<CScene> scene = std::make_unique<CLoadingScene>();
	scene->InitializeObjects(m_pd3dDevice.Get(), m_pd3dCommandList.Get());

	m_Scenes.push_back(std::move(scene));

	// Command List에 대한 명령들을 종료
	m_pd3dCommandList->Close();

	// Command Queue에 Command List를 추가
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList.Get() };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	// Command Queue의 명령들이 모두 실행될 때까지 대기
	WaitForGpuComplete();
}

void CGameFramework::AdvanceFrame()
{
	// 타이머 업데이트
	m_GameTimer.Tick(60.0f);

	// Scene 업데이트

	int bRenderScene = 0;

	// PreRendering [ Swap Chain Back Buffer를 렌더 타겟으로 사용하기 전 렌더링 단계 ]
	// Shadow Map, Reflection Map, Refraction Map, Deferred Shading, G-buffer 등
	// PreRendering 단계에서는 자체적으로 SetOM과 ExecuteCommandLists를 호출.
	// 따라서 이미 Command List에 대한 명령들이 실행된 후, Close 상태여야 함.


	// Command Allocator 재사용
	m_pd3dCommandAllocator->Reset();

	// Command List 재사용
	m_pd3dCommandList->Reset(m_pd3dCommandAllocator.Get(), nullptr);

	// Swap Chain의 Back Buffer를 렌더 타겟으로 사용
	OMSetBackBuffer();

	// Scene 업데이트
	for (auto& scene : m_Scenes)
	{
		if (scene->GetSceneState() == SCENE_STATE_RUNNING)
		{
			scene->PrepareRender(m_pd3dCommandList.Get());

			// Framework 정보 업데이트
			UpdateShaderVariables();

			// Scene 정보 업데이트 및 렌더링
			scene->FixedUpdate(m_GameTimer.DeltaTime());
			bRenderScene += scene->Render(m_pd3dCommandList.Get(), nullptr)? 1 : 0;
		}
	}
	if (0 == bRenderScene) {
		// 렌더링된 Scene이 없는 경우 Loading Scene을 출력

	}

	// Command List에 대한 명령들을 종료
	D3D12_RESOURCE_BARRIER d3dResourceBarrier;
	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource = m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex].Get();
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	m_pd3dCommandList->Close();

	// Command Queue에 Command List를 추가
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList.Get() };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	// Command Queue의 명령들이 모두 실행될 때까지 대기
	WaitForGpuComplete();

	// Swap Chain의 Back Buffer를 화면에 표시
	m_pdxgiSwapChain->Present(0, 0);

	// 다음 Frame으로 이동 (Fence Value를 각 Swap Chain Buffer Index마다 생성하였기에,
	// 이전 스왑체인 버퍼의 Present이 끝나기를 기다린다.)
	MoveToNextFrame();

	// FPS 출력
	std::wstring time = L"Time: " + std::to_wstring(m_GameTimer.GameTime());
	std::wstring fps = L"FPS: " + std::to_wstring(m_GameTimer.calculateAverageFPS());
	std::wstring text = time + L" " + fps;
	::SetWindowText(m_hWnd, text.c_str());
}

void CGameFramework::OMSetBackBuffer()
{
	// Swap Chain의 Back Buffer를 렌더 타겟으로 사용
	D3D12_RESOURCE_BARRIER d3dResourceBarrier;
	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource = m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex].Get();
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	d3dRtvCPUDescriptorHandle.ptr += m_nSwapChainBufferIndex * ::gnRtvDescriptorIncrementSize;

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	m_pd3dCommandList->OMSetRenderTargets(1, &d3dRtvCPUDescriptorHandle, TRUE, &d3dDsvCPUDescriptorHandle);

	// Clear Back Buffer
	m_pd3dCommandList->ClearRenderTargetView(d3dRtvCPUDescriptorHandle, Colors::SteelBlue, 0, nullptr);
	m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
}

void CGameFramework::WaitForGpuComplete()
{
	const UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence.Get(), nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void CGameFramework::MoveToNextFrame()
{
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

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
	m_pd3dcbFrameworkInfo = ::CreateBufferResource(m_pd3dDevice.Get(), m_pd3dCommandList.Get(), NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_GENERIC_READ, NULL);

	m_pd3dcbFrameworkInfo->Map(0, NULL, (void**)&m_pcbMappedFrameworkInfo);
	ZeroMemory(m_pcbMappedFrameworkInfo, sizeof(CB_FRAMEWORK_INFO));
}

void CGameFramework::UpdateShaderVariables()
{
	m_pcbMappedFrameworkInfo->m_fCurrentTime = m_GameTimer.GameTime();
	m_pcbMappedFrameworkInfo->m_fElapsedTime = m_GameTimer.DeltaTime();
	m_pcbMappedFrameworkInfo->m_nRenderMode = 0;

	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = m_pd3dcbFrameworkInfo->GetGPUVirtualAddress();
	m_pd3dCommandList->SetGraphicsRootConstantBufferView(ROOT_PARAMETER_FRAMEWORK, d3dGpuVirtualAddress);
}

void CGameFramework::ReleaseShaderVariables()
{
	if (m_pd3dcbFrameworkInfo)
	{
		m_pd3dcbFrameworkInfo->Unmap(0, NULL);
		m_pd3dcbFrameworkInfo.Reset();
	}
}
