/////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-28
// GameFramework.cpp : 게임 프레임워크 클래스입니다.
// Version : 0.1
/////////////////////////////////////////////////////////////////////////////
#include "GameFramework.h"

#include "TitleScene.h"

bool g_bRenderCollider = false;
bool g_bRenderShadowMap = false;

CGameFramework* CGameFramework::pGameFramework = nullptr;

CGameFramework::CGameFramework()
{
	m_hInstance = NULL;
	m_hWnd = NULL;

	m_nWndClientWidth = 0;
	m_nWndClientHeight = 0;

	m_nSwapChainBufferIndex = 0;

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
		m_CommandListContexts[i].pd3dCommandAllocator = nullptr;
		m_CommandListContexts[i].pd3dCommandList = nullptr;
		m_CommandListContexts[i].nFenceValue = 0;

		/*m_ppd3dSwapChainBackBuffers[i] = nullptr;
		m_pd3dCommandAllocator[i] = nullptr;
		m_pd3dCommandList[i] = nullptr;*/
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

	// 전역 변수들 생성 및 초기화
	Sound::Initialize(); // Sound System 초기화
	BuildDefaultObjects();

	isWorked = true;

	// 씬들을 생성한다.
	BuildObjects();

	m_GameTimer.Reset();

	return true;
}

void CGameFramework::BuildLoadingScene(CUploadContext& uploadContext)
{
	m_pLoadingScene = std::move(BuildScene<CLoadingScene>(uploadContext));
}

void CGameFramework::OnDestroy()
{
	if (!isWorked) return;
	isWorked = false;

	Sound::Shutdown(); // Sound System 종료

	// Scene 생성 스레드를 종료한다.
	StopSceneMadeThread();

	// 남은 Command List가 없는지 확인
	::WaitForGpuComplete(m_pd3dCommandQueue.Get(), m_pd3dFence.Get(), m_CommandListContexts[m_nSwapChainBufferIndex].nFenceValue, m_hFenceEvent);

	// Fence Event 객체를 해제한다.
	::CloseHandle(m_hFenceEvent);

	// Scene들을 해제한다.
	m_Scenes.clear();
	m_pLoadingScene.reset();

	// Cursor 관련 요소 제거
	if (m_pCursorSprite) m_pCursorSprite.reset();

	// 전역 리소스 업로드 컨텍스트를 해제한다.
	ReleaseDefaultObjects();

	CScene::DestroyFramework();

	// DirectX 12 자원들을 해제한다.
	if (m_pd3dDepthStencilBuffer) m_pd3dDepthStencilBuffer.Reset();
	if (m_pd3dDsvDescriptorHeap) m_pd3dDsvDescriptorHeap.Reset();

	for (int i = 0; i < m_nSwapChainBuffers; ++i) {	if (m_ppd3dSwapChainBackBuffers[i])	m_ppd3dSwapChainBackBuffers[i].Reset();	}
	if (m_pd3dRtvDescriptorHeap) m_pd3dRtvDescriptorHeap.Reset();

	for (int i = 0; i < m_nSwapChainBuffers; ++i)
	{
		m_CommandListContexts[i].pd3dCommandAllocator.Reset();
		m_CommandListContexts[i].pd3dCommandList.Reset();

		//if (m_pd3dCommandList[i]) m_pd3dCommandList[i].Reset();
		//if (m_pd3dCommandAllocator[i])	m_pd3dCommandAllocator[i].Reset();
	}
	if (m_pd3dCommandQueue)	m_pd3dCommandQueue.Reset();

	if (m_pd3dFence)	m_pd3dFence.Reset();

	m_pdxgiSwapChain->SetFullscreenState(FALSE, NULL);
	if (m_pdxgiSwapChain) m_pdxgiSwapChain.Reset();
	if (m_pd3dDevice) m_pd3dDevice.Reset();
	if (m_pdxgiFactory) m_pdxgiFactory.Reset();
#ifdef _DEBUG
	// ReportLiveObjects();
	if (m_pd3dDebugController) m_pd3dDebugController.Reset();
#endif

	CoUninitialize(); // 추가
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

	IDXGIAdapter1* pd3dAdapter = NULL;

	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_pdxgiFactory->EnumAdapters1(i, &pd3dAdapter); i++)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		pd3dAdapter->GetDesc1(&dxgiAdapterDesc);
		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void**)&m_pd3dDevice))) break;
	}

	if (!m_pd3dDevice)
	{
		hResult = m_pdxgiFactory->EnumWarpAdapter(_uuidof(IDXGIAdapter1), (void**)&pd3dAdapter);
		hResult = D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), (void**)&m_pd3dDevice);
	}

	if (!m_pd3dDevice)
	{
		MessageBox(NULL, L"Direct3D 12 Device Cannot be Created.", L"Error", MB_OK);
		::PostQuitMessage(0);
		return;
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
		for (int i = 0; i < m_CommandListContexts.size(); ++i)	{
			m_CommandListContexts[i].nFenceValue = 0;	// Fence 초기화
			//m_nFenceValues[i] = 0;	// Fence 초기화
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

	// Command Allocator와 Command List 생성
	for (int i = 0; i < m_nSwapChainBuffers; ++i) {
		// Command Allocator 생성
		hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandListContexts[i].pd3dCommandAllocator));
		if (FAILED(hResult)) {
			OutputDebugString(L"Failed to create command allocator\n");
		}
		m_CommandListContexts[i].pd3dCommandAllocator->SetName((L"GameFramework Command Allocator " + std::to_wstring(i)).c_str());

		// Command List 생성
		hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandListContexts[i].pd3dCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandListContexts[i].pd3dCommandList));
		if (FAILED(hResult)) {
			OutputDebugString(L"Failed to create command list\n");
		}
		m_CommandListContexts[i].pd3dCommandList->SetName((L"GameFramework Command List " + std::to_wstring(i)).c_str());
		// Command List를 닫음.
		CloseCommandList(m_CommandListContexts[i].pd3dCommandList.Get());
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
	::WaitForGpuComplete(m_pd3dCommandQueue.Get(), m_pd3dFence.Get(), m_CommandListContexts[m_nSwapChainBufferIndex].nFenceValue, m_hFenceEvent);

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

	ReallocateSwapChain(m_nWndClientWidth, m_nWndClientHeight);
}

void CGameFramework::Resize(int width, int height)
{
	if (m_pdxgiSwapChain == nullptr) return;

	// ReallocateSwapChain(width, height);
}

void CGameFramework::ReallocateSwapChain(int width, int height)
{
	// 1) GPU 동기화
	::WaitForGpuComplete(m_pd3dCommandQueue.Get(), m_pd3dFence.Get(), m_CommandListContexts[m_nSwapChainBufferIndex].nFenceValue, m_hFenceEvent);

	m_nWndClientWidth = width;
	m_nWndClientHeight = height;

	// 2) 기존 BackBuffers 해제
#ifdef _WITH_DIRECT_WRITE_UI
	if (m_pUILayer) m_pUILayer.reset();
#endif

	for (int i = 0; i < m_nSwapChainBuffers; i++)
		if (m_ppd3dSwapChainBackBuffers[i])
			m_ppd3dSwapChainBackBuffers[i].Reset();

	// 3) ResizeBuffers
	DXGI_SWAP_CHAIN_DESC desc;
	m_pdxgiSwapChain->GetDesc(&desc);

	m_pdxgiSwapChain->ResizeBuffers(
		m_nSwapChainBuffers,
		width,
		height,
		desc.BufferDesc.Format,
		desc.Flags
	);

	// 4) BackBuffer Index 갱신
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	// 5) RTV 재생성
	CreateRenderTargetViews();

#ifdef _WITH_DIRECT_WRITE_UI
	BuildUILayer();
#endif
}

void CGameFramework::BuildDefaultObjects()
{
#ifdef _WITH_DIRECT_WRITE_UI
	BuildUILayer();
#endif
	// BuildDefaultObjects용 리소스 업로드 컨텍스트를 생성한다.
	CUploadContext uploadContext{ "Only For BuildDefaultObjects" };
	uploadContext.Create(m_pd3dDevice.Get(), m_pd3dCommandQueue.Get(), m_pd3dFence.Get(), m_hFenceEvent);

	CScene::InitStaticMembers(uploadContext.m_pd3dDevice, uploadContext.m_pd3dGraphicCommandList);

	CreateShaderVariables(uploadContext.m_pd3dDevice, uploadContext.m_pd3dGraphicCommandList);

	// Bond Transform Manager를 초기화한다.
	CResourceManager::Instance().Initialize(uploadContext.m_pd3dDevice, uploadContext.m_pd3dGraphicCommandList, CScene::GetGraphicRootSignature());
	CGlobalBoneTransformManager::Instance().Initialize(m_pd3dDevice.Get(), uploadContext.m_pd3dGraphicCommandList);

	// Scene 생성 스레드를 시작한다.
	BuildSceneMadeThread();

	// 로딩 Scene을 생성한다.
	BuildLoadingScene(uploadContext);

	// 업로드 커맨드 리스트를 실행하고 리셋한다.
	uploadContext.ExecuteAndReset();
	// ::ExecuteCommandList(uploadContext.m_pd3dGraphicCommandList, m_pd3dCommandQueue.Get(), m_pd3dFence.Get(), m_CommandListContexts[m_nSwapChainBufferIndex].nFenceValue, m_hFenceEvent);
	uploadContext.OnDestroy();

	// 디버그용 텍스트 오브젝트들을 생성한다.
	CreateDebugTextObjects();
}

void CGameFramework::ReleaseDefaultObjects()
{
#ifdef _WITH_DIRECT_WRITE_UI
	m_pUILayer.reset();
#endif

	CGlobalBoneTransformManager::Instance().Shutdown();

	// Shader 변수들을 해제한다.
	ReleaseShaderVariables();

	CResourceManager::Instance().ReleaseResources();

	CUploadContext::Instance().OnDestroy();
}

void CGameFramework::BuildObjects()
{
	//SceneRequest newReq{ CPushScene(TypeTag<CGameScene>{})};

	SceneRequest newReq{ CPushScene(TypeTag<CTitleScene>{})};
	//SceneRequest newReq{ CPushScene(TypeTag<CTestScene>{})};
	RequestSceneChange(newReq);
}


void CGameFramework::BuildUILayer()
{
	std::vector< ID3D12Resource*> ppd3dRenderTargets;
	for (UINT i = 0; i < m_nSwapChainBuffers; i++) {
		ppd3dRenderTargets.push_back(m_ppd3dSwapChainBackBuffers[i].Get());
	}
	m_pUILayer = std::make_shared<UILayer>(m_nSwapChainBuffers, m_pd3dDevice.Get(), m_pd3dCommandQueue.Get(), ppd3dRenderTargets.data(), m_nWndClientWidth, m_nWndClientHeight);

	/*{
		ComPtr<ID2D1SolidColorBrush> pd2dBrush = m_pUILayer->GetBrush(D2D1::ColorF(D2D1::ColorF::Purple, 1.0f));
		ComPtr<IDWriteTextFormat> pdwTextFormat = m_pUILayer->GetTextFormat(L"궁서체", m_nWndClientHeight / 15.0f);
		D2D1_RECT_F d2dRect = D2D1::RectF(0.0f, 0.0f, (float)m_nWndClientWidth, (float)m_nWndClientHeight);

		pd2dBrush = m_pUILayer->GetBrush(D2D1::ColorF(D2D1::ColorF::BlueViolet, 1.0f));
		pdwTextFormat = m_pUILayer->GetTextFormat(L"Arial", m_nWndClientHeight / 35.0f);
		d2dRect = D2D1::RectF(0.0f, m_nWndClientHeight - 45.0f, (float)300.0f, (float)m_nWndClientHeight);

		pd2dBrush = m_pUILayer->GetBrush(D2D1::ColorF(D2D1::ColorF::Black, 1.0f));
		pdwTextFormat = m_pUILayer->GetTextFormat(L"Bahnschrift Condensed", m_nWndClientHeight / 25.0f);
	}*/
}

void CGameFramework::AdvanceFrame()
{
	// 타이머 업데이트
	m_GameTimer.Tick(60.0f);

	// Input 업데이트
	ProcessInput(GetCurrentScene());

	// 이번 프레임에 작업할	Scene 결정
	UpdateSceneTransition();

	// 현재 Scene 요청이 들어온 경우 로딩 Scene을 현재 Scene으로 설정
	CScene* pCurrentScene{};
	const ESceneRequestState requestState = m_RequestState.load(std::memory_order_acquire);
	requestState == ESceneRequestState::Processing ? pCurrentScene = m_pLoadingScene.get() : pCurrentScene = GetCurrentScene();

	// 현재 Scene이 없고, 생성중이 아닐 시 종료
	if (m_Scenes.empty() && requestState == ESceneRequestState::Idle)
	{
		{
			// AdvanceFrame 종료 진단 로그
			OutputDebugStringA("[AdvanceFrame] No current scene. Exiting.\n");
		}
		PostQuitMessage(0);
		return;
	}

	// Update
	AnimateObjects(pCurrentScene);

	// Command List 재사용
	auto& pCommandListContexts = m_CommandListContexts[m_nSwapChainBufferIndex];
	ID3D12CommandAllocator* pCommandAllocator = pCommandListContexts.pd3dCommandAllocator.Get();
	ID3D12GraphicsCommandList* pd3dCommandList = pCommandListContexts.pd3dCommandList.Get();

	/*ID3D12CommandAllocator* pCommandAllocator = m_pd3dCommandAllocator[m_nSwapChainBufferIndex].Get();
	ID3D12GraphicsCommandList* pd3dCommandList = m_CommandListContexts[m_nSwapChainBufferIndex].pd3dCommandList.Get();*/

	pCommandAllocator->Reset();
	pd3dCommandList->Reset(pCommandAllocator, nullptr);

	// 전역 리소스 매니저 처리
	CResourceManager::Instance().ProcessRegistries(m_pd3dDevice.Get(), pd3dCommandList);

	pCurrentScene->PrepareRender(pd3dCommandList);
	CGlobalBoneTransformManager::Instance().PrepareRender(pd3dCommandList);

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
#ifndef _WITH_DIRECT_WRITE_UI
	::SynchronizeResourceTransition(pd3dCommandList, m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
#endif
	::ExecuteCommandList(pd3dCommandList, m_pd3dCommandQueue.Get(), m_pd3dFence.Get(), m_CommandListContexts[m_nSwapChainBufferIndex].nFenceValue, m_hFenceEvent);
	

#ifdef _WITH_DIRECT_WRITE_UI
	// Text 출력
	std::vector<TextBlock*> textBlocks = pCurrentScene->GetTextBlocks();
	if (g_bWindowActive && g_bEnableCursor)
	{
		// 마우스 커서 위치 출력
		if (auto pText = m_pCursorSprite->GetComponent<CTextComponent>()) textBlocks.push_back(pText->GetTextBlock());
	}
	if (g_bDebugOutput) {
		// 디버그 텍스트 출력
		UpdateDebugTextObjects();
		for (auto& pDebugText : m_DebugTextBlocks)
		{
			textBlocks.push_back(pDebugText);
		}
	}
	m_pUILayer->Render(m_nSwapChainBufferIndex, textBlocks);

	/*if (g_bWindowActive && g_bEnableCursor)
	{
		std::vector<std::shared_ptr<CGameObject>> textBlock = { m_pCursorSprite };
		m_pUILayer->Render(m_nSwapChainBufferIndex, textBlock);
	}*/
#endif

	pCurrentScene->OnPostRender(nullptr);

	// Swap Chain의 Back Buffer를 화면에 표시
	m_pdxgiSwapChain->Present(0, 0);
	++g_nFrameCount;

	// 다음 Frame으로 이동
	CResourceManager::Instance().ReleaseUploadBuffers();

	MoveToNextFrame();

	// Time / FPS 출력
	std::wstring time = L"Time: " + std::to_wstring(m_GameTimer.GameTime());
	std::wstring fps = L"FPS: " + std::to_wstring(m_GameTimer.calculateAverageFPS());
	std::wstring text = time + L" " + fps;

	::SetWindowText(m_hWnd, text.c_str());

	// Sound 갱신
	Sound::Update();
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
		m_pCursorSprite = std::make_unique<CSprite>();
		auto pUIShader = CResourceManager::Instance().GetShader<CTextureToViewportShader>();
		std::shared_ptr<CMesh> pRectangleMesh = CResourceManager::Instance().GetMesh("UI");
		auto pCursorMaterial = m_pCursorSprite->GetMaterial();

		TextureRecipe cursorTextureRecipe;
		cursorTextureRecipe.source = TEXTURE_SOURCE_FILE;
		cursorTextureRecipe.name = L"Cursor";
		cursorTextureRecipe.filePath = L"Image/cursor.dds";
		cursorTextureRecipe.type = RESOURCE_TEXTURE2D;
		cursorTextureRecipe.rootparameterindex = ROOT_PARAMETER_STANDARD_TEXTURES;

		auto cursorTexture = std::make_shared<CTexture>(cursorTextureRecipe);
		//cursorTexture = std::make_shared<CTexture>(1, RESOURCE_TEXTURE2D, 1);
		//cursorTexture->LoadTextureFromDDSFile(m_pd3dDevice.Get(), pd3dCommandList, L"Image/cursor.dds", RESOURCE_TEXTURE2D, 0);
		//CResourceManager::Instance().CreateShaderResourceViews(m_pd3dDevice.Get(), cursorTexture.get(), 0, ROOT_PARAMETER_STANDARD_TEXTURES);
		//CResourceManager::Instance().SetTexture(L"Image/cursor.dds", cursorTexture);
		cursorTexture->CreateShaderVariables(m_pd3dDevice.Get(), pd3dCommandList);

		pCursorMaterial->SetTexture(cursorTexture);
		pCursorMaterial->SetShader(pUIShader);

		m_pCursorSprite->SetMesh(pRectangleMesh);

#ifdef _WITH_DIRECT_WRITE_UI
#ifdef _DEBUG
		auto ptextcomponent = m_pCursorSprite->CreateComponent<CTextComponent>();
		ptextcomponent->SetText(L"Cursor Position: (0, 0)");
		ptextcomponent->SetActive(true);
		ptextcomponent->SetSize(0.0f, 0.0f, (float)m_nWndClientWidth, (float)m_nWndClientHeight, false);
		ptextcomponent->SetFont(L"Arial");
		ptextcomponent->SetFontSize(m_nWndClientHeight / 35.0f);
		ptextcomponent->SetBrush(D2D1::ColorF(D2D1::ColorF::Purple, 1.0f));
#endif
#endif
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
#ifdef _WITH_DIRECT_WRITE_UI
	std::wstring cursorText = L"Cursor Position: (" + std::to_wstring(ptCursorPos.x) + L", " + std::to_wstring(ptCursorPos.y) + L")";
	if ( auto ptext = m_pCursorSprite->GetComponent<CTextComponent>()) {
		ptext->SetText(cursorText);
	}

#endif

	m_pCursorSprite->Render(pd3dCommandList, nullptr);
}

void CGameFramework::MoveToNextFrame()
{
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
	//m_nSwapChainBufferIndex = (m_nSwapChainBufferIndex + 1) % m_nSwapChainBuffers;

	UINT64 nFenceValue = ++m_CommandListContexts[m_nSwapChainBufferIndex].nFenceValue;

	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence.Get(), nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void CGameFramework::CreateShaderVariables(CUploadContext& uploadcontext)
{
	CreateShaderVariables(uploadcontext.m_pd3dDevice, uploadcontext.m_pd3dGraphicCommandList);
}

void CGameFramework::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(CB_FRAMEWORK_INFO) + 255) & ~255); //256의 배수
	m_pd3dcbFrameworkInfo = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_GENERIC_READ, NULL);

	m_pd3dcbFrameworkInfo->Map(0, NULL, (void**)&m_pcbMappedFrameworkInfo);
	ZeroMemory(m_pcbMappedFrameworkInfo, sizeof(CB_FRAMEWORK_INFO));
}


void CGameFramework::UpdateShaderVariables()
{
	m_pcbMappedFrameworkInfo->m_fCurrentTime = m_GameTimer.GameTime();
	m_pcbMappedFrameworkInfo->m_fElapsedTime = m_GameTimer.DeltaTime();
	m_pcbMappedFrameworkInfo->m_nRenderMode = g_bRenderCollider ? 1 : 0;
	m_pcbMappedFrameworkInfo->m_fBias = m_fBias;
	m_pcbMappedFrameworkInfo->m_nShadowmapIndex = m_nShadowmapIndex;

	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = m_pd3dcbFrameworkInfo->GetGPUVirtualAddress();
	//->SetGraphicsRootConstantBufferView(ROOT_PARAMETER_FRAMEWORK, d3dGpuVirtualAddress);
	m_CommandListContexts[m_nSwapChainBufferIndex].pd3dCommandList->SetGraphicsRootConstantBufferView(ROOT_PARAMETER_FRAMEWORK, d3dGpuVirtualAddress);
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
	if(auto scene = GetCurrentScene()){
		if(scene != nullptr && scene->IsSceneRunning()){
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
			g_bDebugOutput = !g_bDebugOutput;
			break;
		case VK_F9:
			//“F9” 키가 눌려지면 윈도우 모드와 전체화면 모드의 전환을 처리한다.
			ChangeSwapChainState();
			break;
		case VK_OEM_PLUS:
			m_nShadowmapIndex = (m_nShadowmapIndex + 1) % MAX_DEPTH_TEXTURES;
			{
				std::string debug = "[OnProcessingKeyboardMessage] Shadowmap Index = " + std::to_string(m_nShadowmapIndex) + "\n";
				OutputDebugStringA(debug.c_str());
			}
			break;
		case VK_OEM_MINUS:
			m_nShadowmapIndex = (m_nShadowmapIndex - 1 + MAX_DEPTH_TEXTURES) % MAX_DEPTH_TEXTURES;
			{
				std::string debug = "[OnProcessingKeyboardMessage] Shadowmap Index = " + std::to_string(m_nShadowmapIndex) + "\n";
				OutputDebugStringA(debug.c_str());
			}
			break;
		default:
			break;
		}
		break;
	}
	}
}

LRESULT CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if(isWorked == false)
		return DefWindowProc(hWnd, nMessageID, wParam, lParam);;

	switch (nMessageID)
	{
	case WM_ACTIVATE:
	{
		if (LOWORD(wParam) == WA_INACTIVE) 
		{
			//m_GameTimer.Stop();
			g_bWindowActive = false;
		}
		else 
		{
			//m_GameTimer.Start();
			g_bWindowActive = true;
		}
		break;
	}
	case WM_SIZE:
	{
		int width = LOWORD(lParam);
		int height = HIWORD(lParam);

		if (width != 0 && height != 0)
		{
			OutputDebugStringA("WM_SIZE received\n");
			//Resize(width, height);
		}
	}
	break;
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

void CGameFramework::PopScene()
{
	// TODO : 씬 전환 시 로딩 씬 처리
	if (!m_Scenes.empty())
	{
		m_Scenes.pop_back();
	}
}

void CGameFramework::BuildSceneMadeThread()
{
	// 이미 Scene 생성 스레드가 실행 중이면 반환
	if (m_SceneThreadRunning.load())
	{
		OutputDebugString(L"Scene 생성 스레드가 이미 실행 중입니다.\n");
		return;
	}

	OutputDebugString(L"Scene 생성 스레드 시작.\n");

	// Scene 생성 스레드 시작
	m_SceneMadeThread = std::thread([this]() {

		m_SceneThreadRunning.store(true);
		// 1. Scene 생성 스레드용 Event 생성
		m_hSceneMadeEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		// 2. Scene 생성 컨텍스트 생성
		CUploadContext sceneUploadContext{"SceneMadeThread"};
		sceneUploadContext.Create(m_pd3dDevice.Get(), m_pd3dCommandQueue.Get(), m_pd3dFence.Get(), m_hFenceEvent);

		//CResourceManager::Instance().LoadModelList(sceneUploadContext.m_pd3dDevice, sceneUploadContext.m_pd3dGraphicCommandList);

		// Scene 생성 루프
		while (m_SceneThreadRunning.load()) {
			// Scene 생성 신호 대기
			WaitForSingleObject(m_hSceneMadeEvent, INFINITE);

			if (false == isWorked) break;

			// Scene 생성 작업 수행
			m_SceneBuildState.store(
				ESceneBuildState::Building,
				std::memory_order_release);

			if (false == isWorked) break;

			std::unique_ptr<CScene> newScene;
			try
			{
				// 3. TypeTag 기반 Scene 생성
				std::visit([&](auto tag)
					{
						using T = typename decltype(tag)::type;
						newScene = BuildScene<T>(sceneUploadContext); // Upload 포함
					}, *m_PendingSceneTag);
			}
			catch (...)
			{
				m_SceneBuildState.store(
					ESceneBuildState::Failed,
					std::memory_order_release);
				continue;
			}

			if (false == isWorked) break;

			// -------------
			sceneUploadContext.ExecuteAndReset();

			if (false == isWorked) break;

			// GPU까지 끝났기 때문에 씬을 변경한다.
			{
				std::lock_guard<std::mutex> lock(m_BuiltSceneMutex);
				m_BuiltScene = std::move(newScene);
			}

			m_SceneBuildState.store(
				ESceneBuildState::CPU_Completed,
				std::memory_order_release);

			OutputDebugString(L"Scene 생성 스레드 완료.\n");

			if (false == isWorked) break;
		}

		CloseHandle(m_hSceneMadeEvent);

		sceneUploadContext.OnDestroy();
		m_SceneThreadRunning.store(false);
		});
}

void CGameFramework::StopSceneMadeThread()
{
	// Scene 생성 스레드 종료 신호
	if (m_SceneThreadRunning.load())
	{
		if (m_hSceneMadeEvent)
		{
			// Scene 생성 신호를 보내 스레드를 깨운다.
			SetEvent(m_hSceneMadeEvent);

			// 스레드 종료 대기
			if (m_SceneMadeThread.joinable())
			{
				m_SceneMadeThread.join();
			}
		}
	}
}

void CGameFramework::UpdateSceneTransition()
{
	// 씬 빌드 상태에 따른 처리
	HandleSceneBuildState();

	// 씬 전환 상태에 따른 처리
	const ESceneRequestState requestState = m_RequestState.load(std::memory_order_acquire);

	switch (requestState)
	{
	case ::ESceneRequestState::Idle:
		// 아무 일 없음
		break;
	case ::ESceneRequestState::Pending:
		// 요청을 수리
		ProcessSceneRequest();
		break;
	case ::ESceneRequestState::Processing:
		// 요청 처리 중 (현 요청은 처리완료후 체크)
		break;

	}

}

void CGameFramework::HandleSceneBuildState()
{
	const ESceneBuildState state = m_SceneBuildState.load(std::memory_order_acquire);
	switch (state)
	{
	case ESceneBuildState::Idle:
		// 아무 일 없음
		break;

	case ESceneBuildState::Requested:
		// worker thread가 처리 중
		break;

	case ESceneBuildState::Building:
		// 로딩 씬 출력
		break;

	case ESceneBuildState::CPU_Completed:
	{
		// ResourceManager에서 직접 현재 상태 확인
		auto status = CResourceManager::Instance().GetResourceLoadStatus();

		// 디버그 출력
		{
			std::wstring debug = L"[HandleSceneBuildState] Upload Progress:\n";
			debug += L"  GameObject: " + std::to_wstring(status.nUploadedGameObjects) + L" / " + std::to_wstring(status.nRegisteredGameObjects) + L"\n";
			debug += L"  Mesh: " + std::to_wstring(status.nUploadedMeshes) + L" / " + std::to_wstring(status.nRegisteredMeshes) + L"\n";
			debug += L"  Material: " + std::to_wstring(status.nUploadedMaterials) + L" / " + std::to_wstring(status.nRegisteredMaterials) + L"\n";
			debug += L"  Texture: " + std::to_wstring(status.nUploadedTextures) + L" / " + std::to_wstring(status.nRegisteredTextures) + L"\n";
			debug += L"  Shader: " + std::to_wstring(status.nCreatedShaders) + L" / " + std::to_wstring(status.nRegisteredShaders) + L"\n";
			OutputDebugString(debug.c_str());
		}

		// 모든 리소스가 업로드 완료되었는지 확인
		if (status.IsAllUploaded())
		{
			OutputDebugString(L"[HandleSceneBuildState] All resources uploaded. Transitioning to All_Completed.\n");

			m_SceneBuildState.store(
				ESceneBuildState::All_Completed,
				std::memory_order_release);
			[[fallthrough]];
		}
		else break;
	}
	case ESceneBuildState::All_Completed:
		{
			std::wstring debug = L"[HandleSceneBuildState] " + m_BuiltScene->GetSceneName() + L" 로드 완료.\n";
			OutputDebugString(debug.data());
		}

		m_Scenes.push_back(std::move(m_BuiltScene));
		ClearSceneRequest();
		break;
	case ESceneBuildState::Failed:
		m_BuiltScene.reset();
		ClearSceneRequest();
		break;
	default:
		break;
	}
}

void CGameFramework::CreateDebugTextObjects()
{
	int nDebugTextObjects = 30;
	m_DebugTextBlocks.reserve(nDebugTextObjects);
	int FontSize = (int)(m_nWndClientHeight / 50.0f);
	for (int i = 0; i < nDebugTextObjects; i++) {
		auto pDebugTextObject = new TextBlock;
		pDebugTextObject->SetText(L"Debug Info");
		pDebugTextObject->SetActive(true);
		pDebugTextObject->SetSize(0.0f, (FontSize) * i, (float)m_nWndClientWidth * 0.5f, (float)m_nWndClientHeight , false);
		pDebugTextObject->SetFont(L"Consolas");
		pDebugTextObject->SetFontSize(FontSize);
		pDebugTextObject->SetBrush(D2D1::ColorF(D2D1::ColorF::Black, 1.0f));
		m_DebugTextBlocks.push_back(pDebugTextObject);
	}

	int nIndex = 0;
	auto debugOutputFunc = [&](const std::wstring& debugString) {
		if (nIndex < m_DebugTextBlocks.size()) {
			m_DebugTextBlocks[nIndex]->SetText(debugString + std::to_wstring(nIndex));
			nIndex++;
		}
	};
}

void CGameFramework::UpdateDebugTextObjects()
{
	int index = 0;
	m_DebugTextBlocks[index++]->SetText(L"Debug Info");
	m_DebugTextBlocks[index++]->SetText(L"Scene Size :" + std::to_wstring(m_Scenes.size()));

	// 
	auto requestState = m_RequestState.load(std::memory_order_acquire);
	m_DebugTextBlocks[index++]->SetText(L" Scene Request State:" + to_wstring(requestState));

	//
	auto sceneBuildState = m_SceneBuildState.load(std::memory_order_acquire);
	m_DebugTextBlocks[index++]->SetText(L" Scene Build State :" + to_wstring(sceneBuildState));

	if (sceneBuildState == ESceneBuildState::CPU_Completed)
	{
		auto status = CResourceManager::Instance().GetResourceLoadStatus();
		float progress = status.GetProgress() * 100.0f;
		m_DebugTextBlocks[index++]->SetText(L" Loading Progress: " + std::to_wstring((int)progress) + L"%");

		m_DebugTextBlocks[index++]->SetText(
			L"  GameObject: " + std::to_wstring(status.nUploadedGameObjects) + L" / " + std::to_wstring(status.nRegisteredGameObjects));
		m_DebugTextBlocks[index++]->SetText(
			L"  Mesh: " + std::to_wstring(status.nUploadedMeshes) + L" / " + std::to_wstring(status.nRegisteredMeshes));
		m_DebugTextBlocks[index++]->SetText(
			L"  Material: " + std::to_wstring(status.nUploadedMaterials) + L" / " + std::to_wstring(status.nRegisteredMaterials));
		m_DebugTextBlocks[index++]->SetText(
			L"  Texture: " + std::to_wstring(status.nUploadedTextures) + L" / " + std::to_wstring(status.nRegisteredTextures));
		m_DebugTextBlocks[index++]->SetText(
			L"  Shader: " + std::to_wstring(status.nCreatedShaders) + L" / " + std::to_wstring(status.nRegisteredShaders));
	}

	m_DebugTextBlocks[index++]->SetText(L" Bullets : " + std::to_wstring(gnCurrentBullets));
	m_DebugTextBlocks[index++]->SetText(to_wstring(GetCurrentScene()->GetCameraInfo()));
	m_DebugTextBlocks[index++]->SetText(to_wstring(GetCurrentScene()->GetPlayerInfo()));
	m_DebugTextBlocks[index++]->SetText(L"Bone Matrix : " + std::to_wstring(CGlobalBoneTransformManager::Instance().GetLastAlloactedIndex()) + L" / " + std::to_wstring(CGlobalBoneTransformManager::Instance().GetMaxIndex()));
	m_DebugTextBlocks[index++]->SetText(GetCurrentScene()->to_wstring());


	for(; index < m_DebugTextBlocks.size(); index++)
	{
		m_DebugTextBlocks[index]->SetText(L"");
	}


}

void CGameFramework::ReleaseDebugTextObjects()
{
	m_DebugTextBlocks.clear();
}
