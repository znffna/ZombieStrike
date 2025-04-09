///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-28
// Scene.h : Scene 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"
#include "GameObject.h"
#include "Camera.h"
#include "Shader.h"

class ResourceManager
{
public:
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ID3D12RootSignature* rootsignature) {
		m_d3dDevice = device;
		m_d3dGraphicsCommandList = commandList;
		m_d3dGraphicRootSignature = rootsignature;

		LoadModelList();
	}

	// 모든 리소스 해제
	void ReleaseResources() {

		ModelInfos.clear();
	}

	////////////////////////////////////////////
	// 텍스쳐 정보를 저장
	void SetTexture(const std::string& name, std::shared_ptr<CTexture> texture) {
		TextureInfos[name] = texture;
	}

	std::shared_ptr<CTexture> GetTexture(const std::string& name) {
		if (TextureInfos.find(name) != TextureInfos.end()) {
			// 이미 로드된 모델이 있는 경우
			std::string filepath = name + "이 재사용됨";
			OutputDebugStringA(filepath.c_str());
			OutputDebugStringA("\n");

			return TextureInfos[name];
		}

		return nullptr;
	}
	
	////////////////////////////////////////////
	// 모델 정보를 저장

	void LoadModelList() {
        std::ifstream file("Model/ModelList.txt");
        std::string modelname;
        while (file >> modelname) {
            GetModelInfo(modelname);
        }
	}


	void SetSkinInfo(const std::string& name, std::shared_ptr<CLoadedModelInfo> modelInfo) {
		ModelInfos[name] = modelInfo;
	}

	std::shared_ptr<CLoadedModelInfo> GetModelInfo(const std::string& name) {
		if (ModelInfos.find(name) != ModelInfos.end()) {
			// 이미 로드된 모델이 있는 경우
			if (ModelInfos[name]) return ModelInfos[name];
			else return nullptr;
		}

		ModelInfos[name] = nullptr;

		// 없는경우 바로 불러와서 저장하고 return 한다.
		std::string filepath = "Model/" + name + ".bin";
		OutputDebugStringA(filepath.c_str());
		OutputDebugStringA("\n");

		auto pModelInfo = CGameObject::LoadGeometryAndAnimationFromFile(m_d3dDevice, m_d3dGraphicsCommandList, m_d3dGraphicRootSignature, filepath.c_str(), nullptr);
		if (pModelInfo) {
			SetSkinInfo(name, pModelInfo);
			return pModelInfo;
		}
		else {
			// 로드 실패
			std::string strDebug = "Failed to load model: " + name;
			OutputDebugStringA(strDebug.c_str());
		}
		return nullptr;
	}

private:
	// CGameFramework에서 상속받는다.
	ID3D12Device* m_d3dDevice = nullptr;
	// Resource Manager 전용 CommandList가 필요하다.
	ID3D12GraphicsCommandList* m_d3dGraphicsCommandList = nullptr;
	// CSCene에서 상속받는다. (또는 생성을 CGameFramework에서 하고 넘겨받는다.)
	ID3D12RootSignature* m_d3dGraphicRootSignature = nullptr;

	std::unordered_map<std::string, std::shared_ptr<CLoadedModelInfo>> ModelInfos;
	std::unordered_map<std::string, std::shared_ptr<CTexture>> TextureInfos;
};

#define DIR_FORWARD					0x01
#define DIR_BACKWARD				0x02
#define DIR_LEFT					0x04
#define DIR_RIGHT					0x08
#define DIR_UP						0x10
#define DIR_DOWN					0x20

struct INPUT_PARAMETER
{
	UCHAR pKeysBuffer[256];
	float cxDelta;
	float cyDelta;
};

#define MAX_LIGHTS 16

#define POINT_LIGHT			1
#define SPOT_LIGHT			2
#define DIRECTIONAL_LIGHT	3

struct Light
{
	XMFLOAT4				m_xmf4Ambient;
	XMFLOAT4				m_xmf4Diffuse;
	XMFLOAT4				m_xmf4Specular;
	XMFLOAT3				m_xmf3Position;
	float 					m_fFalloff;
	XMFLOAT3				m_xmf3Direction;
	float 					m_fTheta; //cos(m_fTheta)
	XMFLOAT3				m_xmf3Attenuation;
	float					m_fPhi; //cos(m_fPhi)
	bool					m_bEnable;
	int						m_nType;
	float					m_fRange;
	float					padding;
};

struct CB_LIGHT_INFO
{
	Light					m_pLights[MAX_LIGHTS];
	XMFLOAT4				m_xmf4GlobalAmbient;
	int						m_nLights;
};

class CDescirptorHeap
{
public:
	CDescirptorHeap()
	{
		m_d3dSrvCPUDescriptorStartHandle.ptr = NULL;
		m_d3dSrvGPUDescriptorStartHandle.ptr = NULL;
	};
	virtual ~CDescirptorHeap()
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
};

enum SCENE_STATE
{
	SCENE_STATE_NONE = 0x00, // 초기화되지 않은 상태 [ None ]
	SCENE_STATE_ALLOCING, // 할당 중 [ ALLOC ]
	SCENE_STATE_RUNNING,  // 실행 중 [ Update / Render ]
	SCENE_STATE_PAUSING,  // 일시 중지 중 [ Render ]
	SCENE_STATE_ENDING    // 종료 중 [ Release ]
};

class CScene
{
public:
	CScene();
	virtual ~CScene();

	// Scene Initialization / Release
	void Init(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature = nullptr);
	void PreInitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature = nullptr);
	virtual void InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature = nullptr);
	void PostInitializeObjects();
	virtual void ReleaseObjects();
	virtual void ReleaseUploadBuffers();

	static void InitStaticMembers(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature = nullptr);

	void BuildDefaultLightsAndMaterials();

	void CreateFixedCamera(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	// static member variable
	static void DestroyFramework();
	static ID3D12RootSignature* GetGraphicRootSignature() {return m_pd3dGraphicsRootSignature.Get();	};
	
	// Scene Management
	bool CheckWorkRendering() { return (m_SceneState == SCENE_STATE_RUNNING) || (m_SceneState == SCENE_STATE_PAUSING); }
	bool CheckWorkUpdating() { return (m_SceneState == SCENE_STATE_RUNNING); }
	SCENE_STATE GetSceneState() { return m_SceneState; }
	void SetSceneState(SCENE_STATE SceneState) { m_SceneState = SceneState; }

	// Scene Method
	virtual void Update(float deltaTime);
	bool PrepareRender(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual bool Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr);

	// static method
	static ComPtr<ID3D12RootSignature> CreateGraphicsRootSignature(ID3D12Device* pd3dDevice);
	static void CreateRootSignature(ID3D12RootSignature* pd3dRootSignature, ID3D12Device* pd3dDevice);
	static void CreateDescriptorHeap(ID3D12Device* pd3dDevice);
	static void CreateStaticShader(ID3D12Device* pd3dDevice);

	// Descriptor Heap
	static void CreateCbvSrvDescriptorHeaps(ID3D12Device* pd3dDevice, int nConstantBufferViews, int nShaderResourceViews);
	static void CreateConstantBufferViews(ID3D12Device* pd3dDevice, int nConstantBufferViews, ID3D12Resource* pd3dConstantBuffers, UINT nStride);
	static D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferView(ID3D12Device* pd3dDevice, ID3D12Resource* pd3dConstantBuffer, UINT nStride);
	static D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferView(ID3D12Device* pd3dDevice, D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress, UINT nStride);
	static void CreateShaderResourceViews(ID3D12Device* pd3dDevice, CTexture* pTexture, UINT nDescriptorHeapIndex, UINT nRootParameterStartIndex);
	static void CreateShaderResourceView(ID3D12Device* pd3dDevice, CTexture* pTexture, int nIndex, UINT nRootParameterStartIndex);
	static void CreateShaderResourceView(ID3D12Device* pd3dDevice, CTexture* pTexture, int nIndex);

	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() { return(m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()); }
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() { return(m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()); }

	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUCbvDescriptorStartHandle() { return(m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle); }
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCbvDescriptorStartHandle() { return(m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle); }
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCbvDescriptorNextHandle() { return(m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle); }
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSrvDescriptorStartHandle() { return(m_pDescriptorHeap->m_d3dSrvCPUDescriptorStartHandle); }
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSrvDescriptorStartHandle() { return(m_pDescriptorHeap->m_d3dSrvGPUDescriptorStartHandle); }
	
	// Shader Variables
	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();

	// Input Method
	virtual bool ProcessInput(const INPUT_PARAMETER& pBuffer, float deltaTime) { return false; };
	virtual void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) {}
	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) {}

protected:
	// DescriptorHeap
	static std::shared_ptr<CDescirptorHeap> m_pDescriptorHeap;

	// Scene State
	SCENE_STATE m_SceneState = SCENE_STATE_NONE;

	// RootSignature
	static ComPtr<ID3D12RootSignature> m_pd3dGraphicsRootSignature;
	static ComPtr<ID3D12RootSignature> m_pd3dComputeRootSignature;

	// Light
	XMFLOAT4 m_xmf4GlobalAmbient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);

	std::array<Light, MAX_LIGHTS> m_pLights;
	ComPtr<ID3D12Resource> m_pd3dcbLights;
	CB_LIGHT_INFO* m_pcbMappedLights = nullptr;

	// Animation
	float								m_fElapsedTime = 0.0f;

	// GameObjects
	std::vector<std::shared_ptr<CGameObject>> m_ppObjects;
	std::vector<std::shared_ptr<CGameObject>> m_ppHierarchicalObjects;

	// SkyBox
	std::shared_ptr<CGameObject> m_pSkyBox;

	// Terrain
	std::shared_ptr<CGameObject> m_pTerrain;

	// Camera
	std::shared_ptr<CCamera> m_pCamera;

public:
	// ResourceManager 리소스 관리
	static ResourceManager& GetResourceManager() {
		static ResourceManager instance; // 정적 지역 변수
		return instance;
	}

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

	static void StoreTexture(const std::string& name, std::shared_ptr<CTexture> texture)
	{
		GetResourceManager().SetTexture(name, texture);
	};

	static std::shared_ptr<CTexture> GetTexture(const std::string& name)
	{
		return GetResourceManager().GetTexture(name);
	};
};



