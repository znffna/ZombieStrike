///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-28
// Scene.h : Scene 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include "GameObject.h"
#include "MapObject.h"
#include "Skybox.h"
#include "HeightMapTerrain.h"
#include "Sprite.h"

#include "Zombie.h" 
#include "Player.h"
#include "Gun.h"
#include "CollisionChecker.h"
#include "BulletObject.h"

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
	CPU_Completed,
	All_Completed,
	Failed
};

constexpr const std::string to_string(const ESceneBuildState& type)
{
	switch (type)
	{
	case ESceneBuildState::Idle:  return "Idle";
	case ESceneBuildState::Requested:  return "Requested";
	case ESceneBuildState::Building:   return "Building";
	case ESceneBuildState::All_Completed:   return "Completed";
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
	case ESceneBuildState::All_Completed:   return L"Completed";
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
class CVictoryScene;

using SceneTypeTag = std::variant<
	TypeTag<CLoadingScene>,
	TypeTag<CGameScene>,
	TypeTag<CTitleScene>,
	TypeTag<COnlineScene>,
	TypeTag<CTestScene>,
	TypeTag<CVictoryScene>
>;

struct CPushScene
{
	SceneTypeTag SceneTag;
};

struct CPopScene
{
};

struct CPopAllScene
{
};

using SceneRequest = std::variant<
	CPushScene,
	CPopScene,
	CPopAllScene
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
	//float					padding;
	int 					m_nShadowStartIndex;
};

struct CB_LIGHT_INFO
{
	Light					m_pLights[MAX_LIGHTS];
	XMFLOAT4				m_xmf4GlobalAmbient;
	int						m_nLights;
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
	void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature = nullptr);
	virtual void ReleaseObjects();
	virtual void ReleaseUploadBuffers();

protected:
	virtual void PreInitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature = nullptr);
	virtual void InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature = nullptr);
	virtual void PostInitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature);

public:
	static void InitStaticMembers(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature = nullptr);

	void BuildDefaultLightsAndMaterials();

	virtual void CreateDefaultCamera(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	
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

	template<typename T>
	T* AddObject(std::unique_ptr<T> object);

	// 생성 요청 (지연 생성)
	template<typename T, typename... Args>
	T* RequestCreateObject(TypeTag<T> tag, Args&&... args);

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

	std::string GetPlayerInfo()
	{
		if (m_pPlayer)
		{
			auto pos = m_pPlayer->GetPosition();

			std::string ret;
			ret += "Player ID: " + std::to_string(m_pPlayer->GetID());
			ret += ", Position: (" + std::to_string(pos.x) + ", " + std::to_string(pos.y) + ", " + std::to_string(pos.z) + ")";
			return ret;
		}
		return "No Player";
	}

public:
	// Scene Update
	virtual void Update(float deltaTime);

	// Camera registry
	void RegisterCamera(CCamera* pCamera);
	void UnregisterCamera(CCamera* pCamera);

	// 유효한 카메라가 있으면 우선순위에 따라 반환, 없으면 기존 m_pCamera(기본 카메라) 반환
	CCamera* GetMainCamera();

	std::string GetCameraInfo()
	{
		if (m_nSelectedCamera >= 0 && m_nSelectedCamera < static_cast<int>(m_CameraRegistry.size()))
		{
			auto camera = m_CameraRegistry[m_nSelectedCamera];
			return "Camera Index: " + std::to_string(m_nSelectedCamera) + ", Position: (" +
				std::to_string(camera->GetPosition().x) + ", " +
				std::to_string(camera->GetPosition().y) + ", " +
				std::to_string(camera->GetPosition().z) + ")";
		}
		return "Default Camera";
	}

	// 사용할 카메라 선택
	void SelectCamera(int nIndex);
	void SelectCamera(CCamera* pCamera);

protected:
	// ----------------------------------------
	// 내부 멤버
	// ----------------------------------------
	std::vector<CCamera*> m_CameraRegistry;

	// 기존에 있던 기본 카메라
	int m_nSelectedCamera = -1;  // m_CameraRegistry 내에서 선택된 카메라 인덱스

	CGameObject* m_pDefaultCameraObject = nullptr;
	CCamera* m_pCamera = nullptr; // 이건 현재 카메라가 아닌 기본 카메라

public:
	// ----------------------------------------
	// Render
	// ----------------------------------------

	// Light 정보 갱신
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
	static void CreateStaticShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
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
	// ----------------------------------------
	// TextBlock
	// ----------------------------------------
	const std::vector<TextBlock*> GetTextBlocks()
	{
		return m_TextBlock;
	}

	void RegisterText(TextBlock* ptextblock)
	{
		// 중복 등록 방지
		auto it = std::find(m_TextBlock.begin(), m_TextBlock.end(), ptextblock);
		if (it != m_TextBlock.end()) return; // 이미 등록되어 있음
		
		{
			std::string debugMsg = "Register TextBlock: " + std::to_string((uintptr_t)ptextblock) + "\n";
			OutputDebugStringA(debugMsg.c_str());
		}
		m_TextBlock.push_back(ptextblock);
	}

	void UnregisterText(TextBlock* ptextblock)
	{
		// 카메라 레지스트리에서 제거
		auto it = std::find(m_TextBlock.begin(), m_TextBlock.end(), ptextblock);
		if (it != m_TextBlock.end()) m_TextBlock.erase(it);

		{
			std::string debugMsg = "Unregister TextBlock: " + std::to_string((uintptr_t)ptextblock) + "\n";
			OutputDebugStringA(debugMsg.c_str());
		}
	}

protected:
	std::vector<TextBlock*> m_TextBlock;


public:
	// Shadow Map
	std::shared_ptr<CDepthRenderShader> m_pDepthRenderShader;

	std::shared_ptr<CShadowMapShader> m_pShadowShader;
	std::shared_ptr<CShadowToViewportShader> m_pShadowMapToViewport;

	std::shared_ptr<CMinimapShader> m_pMinimapShader;
	std::shared_ptr<CMinimapToViewportShader> m_pMinimapToViewport;


	BoundingBox CalculateBoundingBox();
	std::array<Light, MAX_LIGHTS> GetLights() const { return m_pLights; }

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

template<typename T, typename... Args>
T* CScene::RequestCreateObject(TypeTag<T> tag, Args&&... args)
{
	static_assert(std::is_base_of_v<CGameObject, T>,
		"T must derive from CGameObject");

	auto object = std::make_unique<T>();
	object->SetID(m_NextGameObjectID++);
	object->SetScene(this);
	object->SetActive(false);
	object->Initialize(std::forward<Args>(args)...);

	T* rawPtr = object.get();
	m_CreateQueue.push_back(std::move(object));
	
	CResourceManager::Instance().RegisterGameObjectResources(rawPtr);

	return rawPtr; // 아직 Scene에 등록되지 않은 객체
}

template<typename T>
T* CScene::AddObject(std::unique_ptr<T> object)
{
	static_assert(std::is_base_of_v<CGameObject, T>,
		"T must derive from CGameObject");

	if (!object)
		return nullptr;

	object->SetID(m_NextGameObjectID++);
	object->SetScene(this);

	auto rawPtr = object.get();

	RegisterLayerView(rawPtr);
	m_ppGameObjects.emplace(rawPtr->GetID(), std::move(object));

	{
		std::string debugMsg = to_string(GetSceneName()) + " - CScene::AddObject: Object Added. CID = " + std::to_string(rawPtr->GetID()) + ", Name = " + rawPtr->GetName() + "\n";
		OutputDebugStringA(debugMsg.c_str());
	}
	return rawPtr;
}
