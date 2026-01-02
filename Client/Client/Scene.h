///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-28
// Scene.h : Scene 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include "GameObject.h"
#include "Zombie.h" 
#include "Player.h"
#include "Gun.h"
#include "CollisionChecker.h"
#include "Sprite.h"

#include "Camera.h"
#include "Shader.h"

#include "ResourceManager.h"

#define DIR_FORWARD					0x01
#define DIR_BACKWARD				0x02
#define DIR_LEFT					0x04
#define DIR_RIGHT					0x08
#define DIR_UP						0x10
#define DIR_DOWN					0x20

template<typename T>
struct TypeTag
{
	using type = T;
};

enum class ESceneBuildState : uint8_t
{
	Idle,
	Requested,
	Building,
	Completed,
	Failed
};

constexpr const std::string to_string(const ESceneBuildState& type)
{
	switch (type)
	{
	case ESceneBuildState::Idle:  return "Idle";
	case ESceneBuildState::Requested:  return "Requested";
	case ESceneBuildState::Building:   return "Building";
	case ESceneBuildState::Completed:   return "Completed";
	case ESceneBuildState::Failed: return "Failed";
	default:                 return "Unknown";
	}
}

constexpr const std::wstring to_wstring(const ESceneBuildState& type)
{
	switch (type)
	{
	case ESceneBuildState::Idle:  return L"Idle";
	case ESceneBuildState::Requested:  return L"Requested";
	case ESceneBuildState::Building:   return L"Building";
	case ESceneBuildState::Completed:   return L"Completed";
	case ESceneBuildState::Failed: return L"Failed";
	default:                 return L"Unknown";
	}
}


enum class ESceneRequestState
{
	Idle,        // 요청 없음
	Pending,     // 요청 대기 (아직 처리 안 함)
	Processing   // Scene 생성/전환 중
};

constexpr const std::string to_string(const ESceneRequestState& type)
{
	switch (type)
	{
	case ESceneRequestState::Idle:       return "Idle";
	case ESceneRequestState::Pending:    return "Pending";
	case ESceneRequestState::Processing: return "Processing";
	default:                             return "Unknown";
	}
}

constexpr const std::wstring to_wstring(const ESceneRequestState& type)
{
	switch (type)
	{
	case ESceneRequestState::Idle:       return L"Idle";
	case ESceneRequestState::Pending:    return L"Pending";
	case ESceneRequestState::Processing: return L"Processing";
	default:                             return L"Unknown";
	}
}


class CLoadingScene;
class CGameScene;
class CTitleScene;
class COnlineScene;
class CTestScene;

using SceneTypeTag = std::variant<
	TypeTag<CLoadingScene>,
	TypeTag<CGameScene>,
	TypeTag<CTitleScene>,
	TypeTag<COnlineScene>,
	TypeTag<CTestScene>
>;

struct CPushScene
{
	SceneTypeTag SceneTag;
};

struct CPopScene
{
};

using SceneRequest = std::variant<
	CPushScene,
	CPopScene
>;


struct INPUT_PARAMETER
{
	UCHAR pKeysBuffer[256];
	float cxDelta;
	float cyDelta;
};

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
		m_d3dCbvCPUDescriptorStartHandle.ptr = NULL;
		m_d3dCbvGPUDescriptorStartHandle.ptr = NULL;
		m_d3dCbvCPUDescriptorNextHandle.ptr = NULL;
		m_d3dCbvGPUDescriptorNextHandle.ptr = NULL;
		m_d3dSrvCPUDescriptorNextHandle.ptr = NULL;
		m_d3dSrvGPUDescriptorNextHandle.ptr = NULL;
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

extern std::vector<std::string> g_vecSceneStateNames;

enum SCENE_STATE
{
	SCENE_STATE_RUNNING = 0x00,  // 실행 중 [ Update / Render ]
	SCENE_STATE_PAUSE,			 // 일시 중지 중 [ Render ]
};

inline std::string to_string(SCENE_STATE type)
{
	std::string ret;
	switch (type)
	{
	case SCENE_STATE_RUNNING: ret = "Running"; break;
	case SCENE_STATE_PAUSE:   ret = "Pause";   break;
	default:                  ret = "Unknown"; break;
	}
	return ret;
}

inline std::wstring to_wstring(SCENE_STATE type)
{
	std::wstring ret;
	switch (type)
	{
	case SCENE_STATE_RUNNING: ret = L"Running"; break;
	case SCENE_STATE_PAUSE:   ret = L"Pause";   break;
	default:                  ret = L"Unknown"; break;
	}
	return ret;
}


class CScene
{
public:
	CScene();
	virtual ~CScene();

	virtual const std::wstring& GetSceneName() const { static std::wstring scenename = L"CScene"; return scenename; }

	// Scene Initialization / Release
	void Init(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature = nullptr);
	virtual void ReleaseObjects();
	virtual void ReleaseUploadBuffers();

protected:
	virtual void PreInitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature = nullptr);
	virtual void InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature = nullptr);
	virtual void PostInitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature);

public:
	static void InitStaticMembers(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature = nullptr);

	void BuildDefaultLightsAndMaterials();

	virtual void CreateFixedCamera(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	
	// static member variable
	static void DestroyFramework();
	static ID3D12RootSignature* GetGraphicRootSignature() {return m_pd3dGraphicsRootSignature.Get();	};
	
	// Scene Management
	template<typename T>
	void PushScene()
	{
		CGameFramework::Instance()->RequestSceneChange(CPushScene{ TypeTag<T>{} });
	};

	virtual void ResetScene();
	virtual void PopScene();

	SCENE_STATE GetSceneState() { return m_SceneState; }
	void SetSceneState(SCENE_STATE UpperPose) { m_SceneState = UpperPose; }
	bool IsSceneRunning() const { return m_SceneState == SCENE_STATE_RUNNING; }

	// Cursor Management
	virtual void SetCursor() { g_bEnableCursor = true; }

public:
	// ----------------------------------------
	// Object Management
	// ----------------------------------------
	// 디버그 / 테스트용 즉시 추가 (소유권 이전 강제)
	CGameObject* AddObject(std::unique_ptr<CGameObject> object);

	// 생성 요청 (지연 생성)
	template<typename T>
	CGameObject* RequestCreateObject(TypeTag<T> tag);

	// 삭제 요청 (지연 삭제)
	void RequestDestroyObject(uint32_t id);

	// 프레임 경계에서 호출
	void ProcessPendingRequest();

    // Object 조회
	CGameObject* FindObject(uint32_t id) const;
	std::map<GAMEOBJECT_LAYER, std::vector<CGameObject*>>& GetLayerViews() { return m_ppLayerView; }

	// Player 세팅(현 클라이언트 입력처리를 수행할 오브젝트)
	void SetPlayer(std::unique_ptr<CPlayer>& pPlayer);

protected:
	// ----------------------------------------
	// 내부 헬퍼
	// ----------------------------------------
	void RegisterLayerView(CGameObject* object);
	void UnregisterLayerView(CGameObject* object);

	// ----------------------------------------
	// Object Containers
	// ----------------------------------------
	std::map<uint32_t, std::unique_ptr<CGameObject>> m_ppGameObjects;      // 실제 오브젝트 소유권 보유
	std::map<GAMEOBJECT_LAYER, std::vector<CGameObject*>> m_ppLayerView;   // 레이어별 오브젝트 뷰 (포인터만 보유)
	std::list<std::unique_ptr<CGameObject>> m_CreateQueue;				   // Request로 받은 Object를 담아둘 리스트
	std::list<uint32_t> m_RemoveQueue;									   // Remove 요청 받은 Object ID를 담아둘 리스트

	// ID 발급기
	uint32_t m_NextGameObjectID = 1;

	// Special objects: owned by m_ppGameObjects; store IDs + observer pointer for fast access
	uint32_t m_SkyBoxID = 0;
	uint32_t m_TerrainID = 0;
	uint32_t m_MapID = 0;
	uint32_t m_PlayerID = 0;

	// Observer pointer (lifetime: owned by m_ppGameObjects)
	// - 빠른 접근 용도. 실제 소유권은 m_ppGameObjects에 있음.
	CPlayer* m_pPlayer = nullptr;

public:
	// Observer getter (null if not present)
	CPlayer* GetPlayer() { return m_pPlayer; }
public:
	// Scene Method
	virtual void Update(float deltaTime);
	void LateUpdate();
	virtual void UpdateLights() {};

	bool PrepareRender(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual bool Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr);
	virtual bool RenderUI(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr);
	virtual void RenderDepthWrite(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr);
	virtual bool OnPreRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr);
	virtual void OnPostRender(ID3D12GraphicsCommandList *pd3dCommandList) {} ;

	// static method
	static ComPtr<ID3D12RootSignature> CreateGraphicsRootSignature(ID3D12Device* pd3dDevice);
	static void CreateRootSignature(ID3D12RootSignature* pd3dRootSignature, ID3D12Device* pd3dDevice);
	static void CreateDescriptorHeap(ID3D12Device* pd3dDevice);
	static void CreateStaticShader(ID3D12Device* pd3dDevice);
	static void CreateStaticMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	static ComPtr<ID3D12RootSignature> GetGraphicsRootSignature() { return m_pd3dGraphicsRootSignature; }

	// Shader Variables
	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();

	void CreateLightShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void UpdateLightShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseLightShaderVariables();

	// Input Method
	virtual bool ProcessMouseInput(float cxDelta, float cyDelta, float deltaTime) { return false; };
	virtual bool ProcessKeyboardInput(const UCHAR pKeysBuffer[256], float deltaTime) { return false; };
	virtual void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) {}

protected:
	bool m_bMouseLButtonDown = false;

	// DescriptorHeap
	static std::unique_ptr<CDescirptorHeap> m_pDescriptorHeap;

	// Scene State
	SCENE_STATE m_SceneState = SCENE_STATE_RUNNING;

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

	

public:
	const std::vector<CTextObject*> GetTextBlocks()
	{
		std::vector<CTextObject*> ppVector;
		ppVector.reserve(m_ppLayerView[GAMEOBJECT_LAYER::LAYER_TEXT].size());
		for(auto& obj : m_ppLayerView[GAMEOBJECT_LAYER::LAYER_TEXT])
		{
			if (CTextObject* textObj = dynamic_cast<CTextObject*>(obj))
				ppVector.push_back(textObj);
		}
		return ppVector;
	}

protected:
	// Camera
	std::shared_ptr<CCamera> m_pCamera;

public:
	// Shadow Map
	std::shared_ptr<CDepthRenderShader> m_pDepthRenderShader;

	std::shared_ptr<CShadowMapShader> m_pShadowShader;
	std::shared_ptr<CShadowToViewportShader> m_pShadowMapToViewport;

	BoundingBox CalculateBoundingBox();
	std::array<Light, MAX_LIGHTS> GetLights() const { return m_pLights; }

public:
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


public:
	// For Debug
	std::wstring to_wstring() const
	{
		std::wstring ret;
		ret += L"Scene Name: " + GetSceneName() + L"\n";
		ret += L"Scene State: " + ::to_wstring(m_SceneState) + L"\n";
		ret += L"Number of GameObjects Layer: " + std::to_wstring(m_ppLayerView.size()) + L"\n";
		for(auto& [layer, objects] : m_ppLayerView)
		{
			ret += L"  Layer " + ::to_wstring(layer) + L": " + std::to_wstring(objects.size()) + L" objects\n";
		}
		return ret;
	}
};

template<typename T>
CGameObject* CScene::RequestCreateObject(TypeTag<T> tag)
{
	static_assert(std::is_base_of_v<CGameObject, T>,
		"T must derive from CGameObject");

	auto object = std::make_unique<T>();
	object->SetID(m_NextGameObjectID++);

	CGameObject* rawPtr = object.get();
	m_CreateQueue.push_back(std::move(object));

	return rawPtr; // 아직 Scene에 등록되지 않은 객체
}

