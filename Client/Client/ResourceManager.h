#pragma once

#include "GameObject.h"

class CLoadedModelInfo;
class CGameObject;
class CTexture;
class CShader;

class CUploadContext
{
public:
	// Singleton
	CUploadContext() {};
	~CUploadContext() {};

	static CUploadContext& Instance()
	{
		static CUploadContext instance;
		return instance;
	}

	void Create(ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue)
	{
		m_pd3dDevice = pd3dDevice;
		m_pd3dCommandQueue = pd3dCommandQueue;

		HRESULT hResult;
		// Command Allocator 생성
		hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&m_pd3dCommandAllocator);
		hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&m_pd3dGraphicCommandList);

		hResult = m_pd3dGraphicCommandList->Close();

		m_pd3dCommandAllocator->Reset();
		m_pd3dGraphicCommandList->Reset(m_pd3dCommandAllocator, nullptr);

		hResult = pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pd3dFence));

		// Fence 생성
		if (SUCCEEDED(hResult)) {
			m_nFenceValue = 0;	// Fence 초기화
			m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			if (m_hFenceEvent == nullptr) { return; }
		}

		m_bIsCreated = true;
	}
	void Destroy()
	{
		::WaitForGpuComplete(m_pd3dCommandQueue, m_pd3dFence, m_nFenceValue, m_hFenceEvent);

		if (m_hFenceEvent) {
			CloseHandle(m_hFenceEvent);
			m_hFenceEvent = nullptr;
		}

		m_pd3dGraphicCommandList->Release();
		m_pd3dCommandAllocator->Release();
		m_pd3dFence->Release();

		m_pd3dCommandQueue = nullptr;
		m_pd3dDevice = nullptr;

		m_bIsCreated = false;
	}

	void ExecuteUploadCommandList()
	{
		if (!m_bIsCreated) return;

		// Command List를 닫음.
		HRESULT hResult = m_pd3dGraphicCommandList->Close();
		// Command List를 실행함.
		ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dGraphicCommandList };
		m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);
		// Fence에 신호를 보냄.
		hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, ++m_nFenceValue);
		// GPU가 명령을 완료할 때까지 대기함.
		::WaitForGpuComplete(m_pd3dCommandQueue, m_pd3dFence, m_nFenceValue, m_hFenceEvent);
		// Command Allocator와 Command List를 재설정함.
		hResult = m_pd3dCommandAllocator->Reset();
		hResult = m_pd3dGraphicCommandList->Reset(m_pd3dCommandAllocator, nullptr);
	}

	bool m_bIsCreated = false;
	ID3D12Device* m_pd3dDevice = nullptr;
	ID3D12CommandQueue* m_pd3dCommandQueue = nullptr;
	ID3D12CommandAllocator* m_pd3dCommandAllocator = nullptr;
	ID3D12GraphicsCommandList* m_pd3dGraphicCommandList = nullptr;
	ID3D12Fence* m_pd3dFence = nullptr;
	UINT64 m_nFenceValue = 0;
	HANDLE m_hFenceEvent = nullptr;
};

class CResourceManager
{
private:
	CResourceManager() {}
	~CResourceManager() {}

public:
	static CResourceManager& GetInstance() {
		static CResourceManager instance;
		return instance;
	}

	void Initialize(ID3D12RootSignature* rootsignature);

	// 모든 리소스 해제
	void ReleaseResources();

	////////////////////////////////////////////
	// 텍스쳐 정보를 저장
	void SetTexture(const std::string& name, std::shared_ptr<CTexture> texture);
	std::shared_ptr<CTexture> GetTexture(const std::string& name);

	////////////////////////////////////////////
	// 모델 정보를 저장
	void LoadModelList(std::string filepath = "Model/ModelList.txt");

	void SetSkinInfo(const std::string& name, std::shared_ptr<CLoadedModelInfo> modelInfo);
	CLoadedModelInfo* GetModelInfo(const std::string& name);

	// 메쉬 정보를 저장
	void SetMesh(const std::string& name, std::shared_ptr<CMesh> pMesh);
	std::shared_ptr<CMesh> GetMesh(const std::string& name);

private:
	// CSCene에서 상속받는다. (또는 생성을 CGameFramework에서 하고 넘겨받는다.)
	ID3D12RootSignature* m_d3dGraphicRootSignature = nullptr;

	std::unordered_map<std::string, std::shared_ptr<CLoadedModelInfo>> ModelInfos;
	std::unordered_map<std::string, std::shared_ptr<CTexture>> TextureInfos;
	std::unordered_map<std::string, std::shared_ptr<CMesh>> MeshInfos;

};
