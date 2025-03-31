/////////////////////////////////////////////////////////////////////////////
// 2024-12-28
// GameFramework.h : ���� �����ӿ�ũ Ŭ�����Դϴ�.
// Version : 0.1
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"
#include "GameTimer.h"
#include "Scene.h"

class ResourceManager
{
public:
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ID3D12RootSignature* rootsignature) {
		m_d3dDevice = device;
		m_d3dGraphicsCommandList = commandList;
		m_d3dGraphicRootSignature = rootsignature;
	}

	// ��� ���ҽ� ����
	void ReleaseResources() {

		ModelInfos.clear();
	}

	////////////////////////////////////////////

	// �� ������ ����

	void SetSkinInfo(const std::string& name, std::shared_ptr<CLoadedModelInfo> modelInfo) {
		ModelInfos[name] = modelInfo;
	}

	std::shared_ptr<CLoadedModelInfo> GetModelInfo(const std::string& name) {
		if (ModelInfos.find(name) != ModelInfos.end()) {
			return ModelInfos[name];
		}

		// ���°�� �ٷ� �ҷ��ͼ� �����ϰ� return �Ѵ�.
		std::string filepath = "Model/" + name + ".bin";
		auto pModelInfo = CGameObject::LoadGeometryAndAnimationFromFile(m_d3dDevice, m_d3dGraphicsCommandList, m_d3dGraphicRootSignature, filepath.c_str(), nullptr);
		if (pModelInfo) {
			SetSkinInfo(name, pModelInfo);
			return pModelInfo;
		}
		else {
			// �ε� ����
			std::string strDebug = "Failed to load model: " + name;
			OutputDebugStringA(strDebug.c_str());
		}
		return nullptr;
	}

private:
	// CGameFramework���� ��ӹ޴´�.
	ID3D12Device* m_d3dDevice = nullptr;
	// Resource Manager ���� CommandList�� �ʿ��ϴ�.
	ID3D12GraphicsCommandList* m_d3dGraphicsCommandList = nullptr;
	// CSCene���� ��ӹ޴´�. (�Ǵ� ������ CGameFramework���� �ϰ� �Ѱܹ޴´�.)
	ID3D12RootSignature* m_d3dGraphicRootSignature = nullptr;

	std::unordered_map<std::string, std::shared_ptr<CLoadedModelInfo>> ModelInfos;
};

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
	std::unique_ptr<CScene>									m_pLoadingScene;  // Loading Scene�� Stack�� ����� ��쿡�� ���(�̴�, Render State�� Scene�� ���� ���� �����Ѵ�)
	/*
	Scene�� Stack���� �����Ͽ� ���� ������ Scene�� ���ؼ� Input ó���� ����.
	������ Scene�� ���ؼ��� Scene�� State�� ���� Update / Render�� ����.
	*/

	POINT m_ptOldCursorPos;


protected:
	ComPtr<ID3D12Resource> m_pd3dcbFrameworkInfo;
	CB_FRAMEWORK_INFO* m_pcbMappedFrameworkInfo = NULL;

public:
	// ResourceManager ���ҽ� ����
	static ResourceManager& GetResourceManager() {
		static ResourceManager instance; // ���� ���� ����
		return instance;
	}

	static ComPtr<ID3D12Resource> GetResource(const std::string& name)
	{
		return GetResourceManager().GetResource(name);
	};

	static void StoreResource(const std::string& filename, ComPtr<ID3D12Resource> pResource)
	{
		GetResourceManager().SetResource(filename, pResource);
	};

	static void StoreSkinInfo(const std::string& filename, std::shared_ptr<CLoadedModelInfo> pSkinInfo)
	{
		GetResourceManager().SetSkinInfo(filename, pSkinInfo);
	};

	static std::shared_ptr<CLoadedModelInfo> GetModelInfo(const std::string& objectname)
	{
		return GetResourceManager().GetModelInfo(objectname);
	};

	static void ReleaseResources()
	{
		GetResourceManager().ReleaseResources();
	};
};

