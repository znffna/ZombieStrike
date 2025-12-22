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
#include "OnlineScene.h"
#include "TitleScene.h"
#include "TestScene.h"

struct CB_FRAMEWORK_INFO
{
	float					m_fCurrentTime;
	float					m_fElapsedTime;
	//float					m_fSecondsPerFirework = 1.0f;
	//int					m_nFlareParticlesToEmit = 300;
	//XMFLOAT3				m_xmf3Gravity = XMFLOAT3(0.0f, -9.8f, 0.0f);
	//int					m_nMaxFlareType2Particles = 150;
	UINT					m_nRenderMode;
	float					m_fBias;
	//float					m_nPadding;
};

class CGameFramework
{
public:
	// Main 함수에서 호출하는 Method들
	CGameFramework();
	~CGameFramework();

	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	void OnDestroy();
	void AdvanceFrame();

	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

private:
	// Framework 초기화 및 게임 루프 관련 Method들
	void CreateDirect3DDevice();
	void CreateCommandQueueAndList();
	void CreateRtvAndDsvDescriptorHeap();
	void CreateSwapChain();
	void CreateRenderTargetViews();
	void CreateDepthStencilView();

	void ChangeSwapChainState(); // 창모드 <-> 전체화면
	void Resize(int width, int height);
	void ReallocateSwapChain(int width, int height);

	void BuildDefaultObjects();
	void ReleaseDefaultObjects();
	void BuildObjects();
	void BuildUILayer();

	void MoveToNextFrame();

	void CreateShaderVariables();
	void UpdateShaderVariables();
	void ReleaseShaderVariables();

	void ProcessInput(CScene* pScene = nullptr);
	void AnimateObjects(CScene* pScene);

	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	bool isWorked = false;

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
	std::array<ComPtr<ID3D12CommandAllocator>, m_nSwapChainBuffers>		m_pd3dCommandAllocator;
	std::array<ComPtr<ID3D12GraphicsCommandList>, m_nSwapChainBuffers>	m_pd3dCommandList;

	ComPtr<ID3D12Fence>										m_pd3dFence;
	UINT64													m_nFenceValueForSignal;
	std::array<UINT64, m_nSwapChainBuffers>					m_nFenceValues;
	HANDLE													m_hFenceEvent;

#if defined(_DEBUG)
	ComPtr<ID3D12Debug>										m_pd3dDebugController;
#endif

	D3D12_VIEWPORT											m_d3dViewport;
	D3D12_RECT												m_d3dScissorRect;

	// Timer
	CGameTimer												m_GameTimer;


public:
	// D12 Resource 접근용 Static Method
	static CGameFramework* Instance() { return pGameFramework; }
	static CGameFramework* pGameFramework;

	POINT m_ptOldCursorPos;

protected:
	// Framework Info (Shader Variable)
	ComPtr<ID3D12Resource> m_pd3dcbFrameworkInfo;
	CB_FRAMEWORK_INFO* m_pcbMappedFrameworkInfo = NULL;
	float m_fBias = 0.007f; // Depth Bias  0.0001f

private:
	// Cursor
	std::shared_ptr<CSprite> m_pCursorSprite; // Cursor Sprite
	POINTF GetTexturePosition(int x, int y);
	void RenderCursor(ID3D12GraphicsCommandList* pd3dCommandList);

	// UI
protected:
	std::shared_ptr<UILayer> m_pUILayer; // UI Layer for DirectWrite
public:
	std::shared_ptr<UILayer> GetUILayer() { return m_pUILayer; }

private:
	// Scene
	std::vector<std::unique_ptr<CScene>>					m_Scenes;
	std::unique_ptr<CScene>									m_pLoadingScene;  // Loading Scene은 Stack이 비었을 경우에만 사용(이는, Render State인 Scene이 없을 때도 포함한다)

	// Scene Creation on Another Thread
	std::thread												m_SceneMadeThread;

	HANDLE													m_hSceneMadeEvent;
	std::atomic_bool										m_SceneThreadRunning{ false };

	std::optional<SceneTypeTag>								m_PendingSceneTag;
	std::atomic<ESceneBuildState>							m_SceneBuildState{ ESceneBuildState::Idle };

	std::unique_ptr<CScene> m_BuiltScene;
	std::mutex m_BuiltSceneMutex;

	// Scene Transition
	std::atomic<ESceneRequestState>							 m_RequestState{ ESceneRequestState::Idle };
	std::optional<SceneRequest>								 m_PendingRequest;


public:
	// Scene 변경 요청 등록
	void RequestSceneChange(SceneRequest newReq)
	{
		// 이미 처리 중이면 병합
		if (m_RequestState.load() != ESceneRequestState::Idle)
		{
			// 기존 요청 덮어쓰기 (마지막 입력만 유지)
			m_PendingRequest = newReq;
			return;
		}

		// 새 요청 등록
		m_PendingRequest = newReq;
		m_RequestState.store(ESceneRequestState::Pending);
	}

	int GetSceneSize() const { return static_cast<int>(m_Scenes.size()); }

private:
	// Scene Management 
	void ProcessSceneRequest()
	{
		const auto& PendingRequest = *m_PendingRequest;

		// 요청 처리
		std::visit([this](auto&& arg)
			{
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, CPushScene>)
				{
					// Push Scene
					RequestBuildScene(arg.SceneTag);
					m_RequestState.store(ESceneRequestState::Processing);
					// 시간이 걸리기에 별도 스레드에서 처리, 다음 요청은 대기상태
				}
				else if constexpr (std::is_same_v<T, CPopScene>)
				{
					// Pop Scene
					PopScene();
					ClearSceneRequest();
				}
			}
		, PendingRequest);

		// 요청 처리 완료
		m_PendingRequest.reset();
	}
	template<typename T>
	void PushScene()
	{
		// 아래는 Request를 처리할때의 코드.
		SceneTypeTag tag = TypeTag<T>{};
		RequestBuildScene(tag);
	}
	template<typename T>
	void RequestBuildScene()
	{
		SceneTypeTag tag = TypeTag<T>{};
		RequestBuildScene(tag);
	}
	void RequestBuildScene(const SceneTypeTag& tag)
	{
		if (m_SceneBuildState.load() != ESceneBuildState::Idle)
			return;

		m_PendingSceneTag = tag;
		m_SceneBuildState.store(ESceneBuildState::Requested, std::memory_order_release);

		OutputDebugString(L"Scene 생성 요청이 접수되었습니다.\n");

		SetEvent(m_hSceneMadeEvent);
	}
	void PopScene();

	void ClearSceneRequest()
	{
		// 씬 생성 상태 변경
		m_SceneBuildState.store(ESceneBuildState::Idle, std::memory_order_release);
		// 다시 요청 대기 상태로 변경
		m_PendingRequest.reset();
		m_RequestState.store(ESceneRequestState::Idle);
	}

	template <typename T>
	std::unique_ptr<T> BuildScene(CUploadContext& uploadContext)
	{
		// 여기서 Scene을 및 각종 오브젝트들을 생성한다.
		auto pScene = std::make_unique<T>();
		pScene->Init(uploadContext.m_pd3dDevice, uploadContext.m_pd3dGraphicCommandList);

		// ------------- 
		uploadContext.ExecuteAndReset();
		pScene->ReleaseUploadBuffers();
		return pScene;
	}

	CScene* GetCurrentScene() { if (m_Scenes.size()) return m_Scenes.back().get(); else return m_pLoadingScene.get(); }
	
	void BuildSceneMadeThread(); // Scene 생성 스레드 함수
	void StopSceneMadeThread();  // Scene 생성 스레드 종료
	void UpdateSceneTransition();
	void HandleSceneBuildState(); // Scene 생성상태 처리

private:
	// 디버그용 텍스트 오브젝트들
	std::vector<CTextObject> m_DebugTextObjects; 

	void CreateDebugTextObjects();
	void UpdateDebugTextObjects();
	void ReleaseDebugTextObjects();
};

