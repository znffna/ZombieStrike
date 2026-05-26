#pragma once

#include <typeindex>
#include "GameObject.h"
class CLoadedModelInfo;
class CGameObject;
class CMaterial;
class CTexture;
class CShader;

// ----------------------------------------
// 리소스 로딩 현황 구조체
// ----------------------------------------
struct ResourceLoadStatus
{
	// GameObject
	UINT nRegisteredGameObjects;
	UINT nCollectedGameObjects;
	UINT nUploadedGameObjects;

	// Mesh
	UINT nRegisteredMeshes;
	UINT nCollectedMeshes;
	UINT nUploadedMeshes;

	// Material
	UINT nRegisteredMaterials;
	UINT nCollectedMaterials;
	UINT nUploadedMaterials;

	// Texture
	UINT nRegisteredTextures;
	UINT nCollectedTextures;
	UINT nUploadedTextures;

	// Shader
	UINT nRegisteredShaders;
	UINT nCollectedShaders;
	UINT nCreatedShaders;

	// 전체 등록된 작업 수
	UINT GetTotalRegistered() const
	{
		return nRegisteredGameObjects + nRegisteredMeshes + nRegisteredMaterials + 
		       nRegisteredTextures + nRegisteredShaders;
	}

	// 전체 Collect된 작업 수
	UINT GetTotalCollected() const
	{
		return nCollectedGameObjects + nCollectedMeshes + nCollectedMaterials + 
		       nCollectedTextures + nCollectedShaders;
	}

	// 전체 Upload 완료된 작업 수
	UINT GetTotalUploaded() const
	{
		return nUploadedGameObjects + nUploadedMeshes + nUploadedMaterials + 
		       nUploadedTextures + nCreatedShaders;
	}

	// 모든 리소스가 업로드 완료되었는지 확인
	bool IsAllUploaded() const
	{
		return (nUploadedGameObjects >= nRegisteredGameObjects) &&
		       (nUploadedMeshes >= nRegisteredMeshes) &&
		       (nUploadedMaterials >= nRegisteredMaterials) &&
		       (nUploadedTextures >= nRegisteredTextures) &&
		       (nCreatedShaders >= nRegisteredShaders);
	}

	// 진행률 (0.0 ~ 1.0)
	float GetProgress() const
	{
		UINT total = GetTotalRegistered();
		if (total == 0) return 1.0f;
		return static_cast<float>(GetTotalUploaded()) / static_cast<float>(total);
	}
};

class CUploadContext
{
public:
	// Singleton
	CUploadContext() {};
	CUploadContext(std::string name): m_strName(name) {};
	~CUploadContext() { OnDestroy(); };

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
		m_pd3dCommandAllocator->SetName((to_wstring(m_strName) + L" UploadContext Command Allocator").c_str());
		hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&m_pd3dGraphicCommandList);
		m_pd3dGraphicCommandList->SetName((to_wstring(m_strName) + L" UploadContext Graphic Command List").c_str());

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
		if (false == m_bIsCreated) return;

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

class CDescriptorHeap
{
public:
	CDescriptorHeap()
	{
		m_d3dSrvCPUDescriptorStartHandle.ptr = NULL;
		m_d3dSrvGPUDescriptorStartHandle.ptr = NULL;
		m_d3dCbvCPUDescriptorStartHandle.ptr = NULL;
		m_d3dCbvGPUDescriptorStartHandle.ptr = NULL;
		m_d3dCbvCPUDescriptorNextHandle.ptr = NULL;
		m_d3dCbvGPUDescriptorNextHandle.ptr = NULL;
		m_d3dSrvCPUDescriptorNextHandle.ptr = NULL;
		m_d3dSrvGPUDescriptorNextHandle.ptr = NULL;
	};
	virtual ~CDescriptorHeap()
	{
		if (m_pd3dCbvSrvDescriptorHeap) m_pd3dCbvSrvDescriptorHeap.Reset();
	};

	ComPtr<ID3D12DescriptorHeap> m_pd3dCbvSrvDescriptorHeap;

	D3D12_CPU_DESCRIPTOR_HANDLE			m_d3dCbvCPUDescriptorStartHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE			m_d3dCbvGPUDescriptorStartHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE			m_d3dSrvCPUDescriptorStartHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE			m_d3dSrvGPUDescriptorStartHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE			m_d3dCbvCPUDescriptorNextHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE			m_d3dCbvGPUDescriptorNextHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE			m_d3dSrvCPUDescriptorNextHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE			m_d3dSrvGPUDescriptorNextHandle;

	
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() { return(m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() { return(m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUCbvDescriptorStartHandle() { return(m_d3dCbvCPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCbvDescriptorStartHandle() { return(m_d3dCbvGPUDescriptorStartHandle); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSrvDescriptorStartHandle() { return(m_d3dSrvCPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSrvDescriptorStartHandle() { return(m_d3dSrvGPUDescriptorStartHandle); }

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUAddressOfIndex(int nIndex) const
	{
		D3D12_GPU_DESCRIPTOR_HANDLE d3dGpuDescriptorHandle;
		d3dGpuDescriptorHandle.ptr = m_d3dSrvGPUDescriptorStartHandle.ptr + nIndex * gnCbvSrvDescriptorIncrementSize;
		return d3dGpuDescriptorHandle;
	}
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUAddressOfIndex(int nIndex) const
	{
		D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuDescriptorHandle;
		d3dCpuDescriptorHandle.ptr = m_d3dSrvCPUDescriptorStartHandle.ptr + nIndex * gnCbvSrvDescriptorIncrementSize;
		return d3dCpuDescriptorHandle;
	}
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

	void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList, ID3D12RootSignature* rootsignature);
	void CreateDefaultMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList);

	void CreateDefualtQuad(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList);
	void CreateDefaultCube(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList);
	void CreateDefualtSphere(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList);


	// 모든 리소스 해제
	void ReleaseResources();

private:
	// CSCene에서 받아온다. (또는 원래는 CGameFramework에서 하고 넘겨받는다.)
	ID3D12RootSignature* m_d3dGraphicRootSignature = nullptr;

public:
	// ----------------------------------------
	// 리소스 로딩 현황 조회
	// ----------------------------------------
	ResourceLoadStatus GetResourceLoadStatus() const
	{
		ResourceLoadStatus status{};

		// GameObject
		{
			std::lock_guard<std::mutex> lock(m_RegisterGameObjectMutex);
			status.nRegisteredGameObjects = static_cast<UINT>(m_GameObjectResourceRegisterList.size());
			status.nCollectedGameObjects = static_cast<UINT>(m_GameObjectToProcessList.size());
		}
		status.nUploadedGameObjects = status.nCollectedGameObjects; // GameObject는 Process되면 즉시 Upload

		// Mesh
		status.nRegisteredMeshes = m_nRegisterMeshCount.load();
		{
			std::lock_guard<std::mutex> lock(m_UploadMutex);
			status.nCollectedMeshes = static_cast<UINT>(m_MeshUploadList.size());
		}
		status.nUploadedMeshes = m_nUploadMeshCount.load();

		// Material
		status.nRegisteredMaterials = m_nRegisterMaterialCount.load();
		{
			std::lock_guard<std::mutex> lock(m_UploadMutex);
			status.nCollectedMaterials = static_cast<UINT>(m_MaterialUploadList.size());
		}
		status.nUploadedMaterials = m_nUploadMaterialCount.load();

		// Texture
		status.nRegisteredTextures = m_nRegisterTextureCount.load();
		{
			std::lock_guard<std::mutex> lock(m_UploadMutex);
			status.nCollectedTextures = static_cast<UINT>(m_TextureUploadList.size());
		}
		status.nUploadedTextures = m_nUploadTextureCount.load();

		// Shader
		{
			std::lock_guard<std::mutex> lock(m_UploadMutex);
			status.nRegisteredShaders = static_cast<UINT>(m_ShaderRegisterBuffer.size());
			status.nCollectedShaders = static_cast<UINT>(m_ShaderToCreateList.size());
			// Shader는 CreateList가 비면 모두 생성된 것
			if (m_ShaderRegisterBuffer.empty() && m_ShaderToCreateList.empty())
			{
				status.nCreatedShaders = status.nRegisteredShaders;
			}
			else
			{
				// 등록된 것 - 아직 처리 안된 것 = 생성 완료된 것
				status.nCreatedShaders = status.nRegisteredShaders - 
				                         static_cast<UINT>(m_ShaderRegisterBuffer.size());
			}
		}

		return status;
	}

	// ----------------------------------------
	// 디스크립터 힙 (Descriptor Heap) 관리
	// ----------------------------------------
	void CreateDescriptorHeap(ID3D12Device* pd3dDevice);

	void CreateCbvSrvDescriptorHeaps(ID3D12Device* pd3dDevice, int nConstantBufferViews, int nShaderResourceViews);
	void CreateConstantBufferViews(ID3D12Device* pd3dDevice, int nConstantBufferViews, ID3D12Resource* pd3dConstantBuffers, UINT nStride);
	D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferView(ID3D12Device* pd3dDevice, ID3D12Resource* pd3dConstantBuffer, UINT nStride);
	D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferView(ID3D12Device* pd3dDevice, D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress, UINT nStride);
	void CreateShaderResourceViews(ID3D12Device* pd3dDevice, CTexture* pTexture, UINT nDescriptorHeapIndex, UINT nRootParameterStartIndex);
	void CreateShaderResourceView(ID3D12Device* pd3dDevice, CTexture* pTexture, int nIndex, UINT nRootParameterStartIndex);
	void CreateShaderResourceView(ID3D12Device* pd3dDevice, CTexture* pTexture, int nIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE CreateShaderResourceView(ID3D12Device* pd3dDevice, ID3D12Resource* pResource, D3D12_SHADER_RESOURCE_VIEW_DESC& desc);

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() { return(m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() { return(m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUCbvDescriptorStartHandle() { return(m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCbvDescriptorStartHandle() { return(m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCbvDescriptorNextHandle() { return(m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSrvDescriptorStartHandle() { return(m_pDescriptorHeap->m_d3dSrvCPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSrvDescriptorStartHandle() { return(m_pDescriptorHeap->m_d3dSrvGPUDescriptorStartHandle); }

	void PrepareRender(ID3D12GraphicsCommandList* pd3dCommnadList);

private:
	std::unique_ptr<CDescriptorHeap> m_pDescriptorHeap;

public:
	// ----------------------------------------
	// 텍스처 리소스 관리
	// ----------------------------------------
	struct TextureInfo {
		ComPtr<ID3D12Resource> texture;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
		UINT rootParameterIndex;
	};
	void SetTexture(const std::wstring& path, std::shared_ptr<CTexture> texture);
	void SetTexture(const std::wstring& path, TextureInfo texture);
	std::shared_ptr<CTexture> LoadOrCreateTexture(const std::wstring& path);
	TextureInfo GetTexture(const std::wstring& path)
	{
		if (Textures.find(path) != Textures.end())
		{
			return Textures[path];
		}

		return TextureInfo{nullptr, 0, 0};
	};

private:
	std::unordered_map<std::wstring, std::shared_ptr<CTexture>> TextureInfos;
	std::unordered_map<std::wstring, TextureInfo> Textures;

public:
	// ----------------------------------------
	// 모델 리소스 관리
	// ----------------------------------------
	void LoadModelList(std::string filepath = "Model/ModelList.txt");
	void LoadModelList(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList, std::string filepath = "Model/ModelList.txt");

	void SetSkinInfo(const std::string& name, std::shared_ptr<CLoadedModelInfo> modelInfo);
	CLoadedModelInfo* GetModelInfo(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList, const std::string& name);
	CLoadedModelInfo* GetModelInfo(const std::string& name);

private:
	std::unordered_map<std::string, std::shared_ptr<CLoadedModelInfo>> ModelInfos;

public:
	// ----------------------------------------
	// 메시 리소스 관리
	// ----------------------------------------
	void SetMesh(const std::string& name, std::shared_ptr<CMesh> pMesh);
	std::shared_ptr<CMesh> GetMesh(const std::string& name);

private:
	std::unordered_map<std::string, std::shared_ptr<CMesh>> MeshInfos;

public:
	// ----------------------------------------
	// 셰이더 리소스 관리
	// ----------------------------------------
	template <typename TShader>
	std::shared_ptr<CShader> GetShader()
	{
		static_assert(std::is_base_of_v<CShader, TShader>);
		return GetOrCreate<TShader>();
	}

	template <typename TShader>
	std::shared_ptr<CShader> GetOrCreate()
	{
		auto findIt = ShaderInfos.find(std::type_index(typeid(TShader)));
		if (findIt != ShaderInfos.end())
		{
			return findIt->second;
		}
		return CreateShader<TShader>();
	}

	template <typename TShader>
	std::shared_ptr<CShader> CreateShader()
	{
		static_assert(std::is_base_of_v<CShader, TShader>);

		auto shader = std::make_shared<TShader>();
		ShaderInfos[std::type_index(typeid(TShader))] = shader;
		RegisterShaderUpload(shader.get());

		return shader;
	}

private:
	std::unordered_map<std::type_index, std::shared_ptr<CShader>> ShaderInfos;

public:
	void RegisterGameObjectResources(CGameObject* pGameObject);
	void CollectGameObjectRequest(int maxcount);
	void ProcessGameObjectUpload(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	mutable std::mutex m_RegisterGameObjectMutex;
	std::queue<CGameObject*> m_GameObjectResourceRegisterList;
	std::vector<CGameObject*> m_GameObjectToProcessList;

public:
	// ----------------------------------------
	// Register To Upload List
	// ----------------------------------------

	mutable std::mutex m_UploadMutex;

	void CollectRegister(int maxcount)
	{
		CollectGameObjectRequest(maxcount);

		std::lock_guard<std::mutex> lock(m_UploadMutex);

		// Mesh Upload List 일부 수집
		CollectMeshRegister(maxcount);
		// Material Upload List 전체
		CollectMaterialRegister(maxcount);
		// Shader Create List 전체
		CollectShaderRegister();
		// Texture Upload List 전체
		CollectTextureRegister(maxcount);
	}

	void ProcessRegistries(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
	{
		CollectRegister(8);
		ProcessGameObjectUpload(pd3dDevice, pd3dCommandList);
		ProcessMeshUpload(pd3dDevice, pd3dCommandList);
		ProcessMaterialUpload(pd3dDevice, pd3dCommandList);
		ProcessShaderCreate(pd3dDevice, pd3dCommandList);
		ProcessTextureUpload(pd3dDevice, pd3dCommandList);
	}
	void ReleaseUploadBuffers()
	{
		ReleaseMeshUploadBuffers();
		ReleaseMaterialUploadBuffers();
		ReleaseTextureUploadBuffers();
	}

	// ----------------------------------------
	// Mesh Upload 처리
	// ----------------------------------------
	void RegisterMeshUpload(CMesh* pMesh);
	void CollectMeshRegister(int maxcount = INT_MAX);
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
	void CollectMaterialRegister(int maxcount = INT_MAX);
	void ProcessMaterialUpload(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseMaterialUploadBuffers();

	std::atomic<UINT> m_nRegisterMaterialCount = 0;
	std::atomic<UINT> m_nUploadMaterialCount = 0;
	std::deque<CMaterial*> m_MaterialRegisterBuffer;
	std::vector<CMaterial*> m_MaterialUploadList;

	// ----------------------------------------
	// Texture Upload 처리
	// ----------------------------------------
	void RegisterTextureUpload(CTexture* pTexture);
	void CollectTextureRegister(int maxcount = INT_MAX);
	void ProcessTextureUpload(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseTextureUploadBuffers();

	std::atomic<UINT> m_nRegisterTextureCount = 0;
	std::atomic<UINT> m_nUploadTextureCount = 0;
	std::deque<CTexture*> m_TextureRegisterBuffer;
	std::vector<CTexture*> m_TextureUploadList;

	// ----------------------------------------
	// Create Shader
	// ----------------------------------------
	void RegisterShaderUpload(CShader* pShader);
	void CollectShaderRegister(int maxcount = INT_MAX);
	void ProcessShaderCreate(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	std::deque<CShader*> m_ShaderRegisterBuffer;
	std::vector<CShader*> m_ShaderToCreateList;

	// ----------------------------------------
	// Create Shader Variables For Camera
	// ----------------------------------------
	void RegisterCamera(CCamera* pCamera);
	void CollectCameraRegister(int maxcount = INT_MAX);
	void ProcessCameraCreate(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	std::deque<CCamera*> m_CameraRegisterBuffer;
	std::vector<CCamera*> m_CameraToCreateList;
};
