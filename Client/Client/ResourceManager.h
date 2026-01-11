#pragma once

#include "GameObject.h"

class CLoadedModelInfo;
class CGameObject;
class CMaterial;
class CTexture;
class CShader;

class CUploadContext
{
public:
	// Singleton
	CUploadContext() {};
	CUploadContext(std::string name): m_strName(name) {};
	~CUploadContext() {};

	// 전역 리소스 업로드 컨텍스트
	static CUploadContext& Instance()
	{
		static CUploadContext instance{"Static Instance"};
		return instance;
	}

	void Create(ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Fence* pd3dFence, HANDLE hFenceEvent)
	{
		if (m_bIsCreated) return;

		m_pd3dDevice = pd3dDevice;
		m_pd3dCommandQueue = pd3dCommandQueue;
		m_pd3dFence = pd3dFence;

		//m_hFenceEvent = hFenceEvent;
		
		// Create Fence Event
		m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

		HRESULT hResult;
		// Command Allocator 생성
		hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&m_pd3dCommandAllocator);
		hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&m_pd3dGraphicCommandList);

		hResult = m_pd3dGraphicCommandList->Close();

		m_pd3dCommandAllocator->Reset();
		m_pd3dGraphicCommandList->Reset(m_pd3dCommandAllocator, nullptr);

		{
			std::string debugname = m_strName + "CUploadContext Created\n";
			OutputDebugStringA(debugname.c_str());
		}

		m_bIsCreated = true;
	}
	void OnDestroy()
	{
		::WaitForGpuComplete(m_pd3dCommandQueue, m_pd3dFence, m_nFenceValue, m_hFenceEvent);

		m_pd3dGraphicCommandList->Release();
		m_pd3dCommandAllocator->Release();

		m_pd3dCommandQueue = nullptr;
		m_pd3dDevice = nullptr;
		m_pd3dFence = nullptr;
		CloseHandle(m_hFenceEvent);

		m_bIsCreated = false;
	}

	void ExecuteAndReset()
	{
		if (!m_bIsCreated) return;

		// Command List를 닫고 Execute, Signal 및 wait까지 함.
		::ExecuteCommandList(m_pd3dGraphicCommandList, m_pd3dCommandQueue, m_pd3dFence, m_nFenceValue, m_hFenceEvent);
		
		// Command Allocator와 Command List를 재설정함.
		HRESULT hResult = m_pd3dCommandAllocator->Reset();
		hResult = m_pd3dGraphicCommandList->Reset(m_pd3dCommandAllocator, nullptr);

		{
			std::string debugname = m_strName + " CUploadContext ExecuteAndReset - FenceValue :" + std::to_string(m_nFenceValue) + "\n";
			OutputDebugStringA(debugname.c_str());
		}
	}

	bool m_bIsCreated = false;
	ID3D12Device* m_pd3dDevice = nullptr;
	ID3D12CommandQueue* m_pd3dCommandQueue = nullptr;
	ID3D12CommandAllocator* m_pd3dCommandAllocator = nullptr;
	ID3D12GraphicsCommandList* m_pd3dGraphicCommandList = nullptr;
	ID3D12Fence* m_pd3dFence = nullptr;
	UINT64 m_nFenceValue = 0;
	HANDLE m_hFenceEvent = nullptr;

private:
	std::string m_strName{"UploadContext"};

};

class CResourceManager
{
private:
	CResourceManager() {}
	~CResourceManager() {}

public:
	static CResourceManager& Instance() {
		static CResourceManager instance;
		return instance;
	}

	void Initialize(ID3D12RootSignature* rootsignature);
	void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList, ID3D12RootSignature* rootsignature);

	// 모든 리소스 해제
	void ReleaseResources();

	////////////////////////////////////////////
	// 텍스쳐 정보를 저장
	void SetTexture(const std::string& name, std::shared_ptr<CTexture> texture);
	std::shared_ptr<CTexture> GetTexture(const std::string& name);

	////////////////////////////////////////////
	// 모델 정보를 저장
	void LoadModelList(std::string filepath = "Model/ModelList.txt");
	void LoadModelList(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList, std::string filepath = "Model/ModelList.txt");

	void SetSkinInfo(const std::string& name, std::shared_ptr<CLoadedModelInfo> modelInfo);
	CLoadedModelInfo* GetModelInfo(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList, const std::string& name);
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

public:
	// ----------------------------------------
	// Register To Upload List
	// ----------------------------------------

	std::mutex m_UploadMutex; // Upload List에 대한 Mutex

	void CollectRegister(int maxcount)
	{
		std::lock_guard<std::mutex> lock(m_UploadMutex);
		// Mesh Upload List 일부 추출
		//std::swap(m_MeshRegisterBuffer, m_MeshUploadList);
		{
			int count{};
			while (false == m_MeshRegisterBuffer.empty() && count < maxcount)
			{
				m_MeshUploadList.push_back(m_MeshRegisterBuffer.back());
				m_MeshRegisterBuffer.pop_back();
				++count;
			}

			m_nUploadMeshCount.fetch_add(count);
		}
		// Material Upload List 교체
		//std::swap(m_MaterialRegisterBuffer, m_MaterialUploadList);
		{
			int count{};
			while (false == m_MaterialRegisterBuffer.empty() && count < maxcount)
			{
				m_MaterialUploadList.push_back(m_MaterialRegisterBuffer.back());
				m_MaterialRegisterBuffer.pop_back();
				++count;
			}
			m_nUploadMaterialCount.fetch_add(count);
		}
	}
	void ProcessRegistries(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
	{
		CollectRegister(8);
		ProcessMeshUpload(pd3dDevice, pd3dCommandList);
		ProcessMaterialUpload(pd3dDevice, pd3dCommandList);
	}
	void ReleaseUploadBuffers()
	{
		ReleaseMeshUploadBuffers();
		ReleaseMaterialUploadBuffers();
	}

	// ----------------------------------------
	// Mesh Upload 처리
	// ----------------------------------------
	void RegisterMeshUpload(CMesh* pMesh);
	void ProcessMeshUpload(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseMeshUploadBuffers();

	std::atomic<UINT> m_nRegisterMeshCount = 0;
	std::atomic<UINT> m_nUploadMeshCount = 0;
	std::deque<CMesh*> m_MeshRegisterBuffer;
	std::vector<CMesh*> m_MeshUploadList;
	
	// ----------------------------------------
	// Material Upload 처리
	// ----------------------------------------
	void RegisterMaterialUpload(CMaterial* pMaterial);
	void ProcessMaterialUpload(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseMaterialUploadBuffers();

	std::atomic<UINT> m_nRegisterMaterialCount = 0;
	std::atomic<UINT> m_nUploadMaterialCount = 0;
	std::deque<CMaterial*> m_MaterialRegisterBuffer;
	std::vector<CMaterial*> m_MaterialUploadList;
};
