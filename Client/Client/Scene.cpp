///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-28
// Scene.cpp : Scene 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#include "Scene.h"
#include "GameFramework.h"

ComPtr<ID3D12RootSignature> CScene::m_pd3dGraphicsRootSignature;
ComPtr<ID3D12RootSignature> CScene::m_pd3dComputeRootSignature;

std::vector<std::string> g_vecSceneStateNames{
	"None",
	"Allocing",
	"ReadyToStart",
	"Running",
	"Pausing",
	"Ending"
}; 

CScene::CScene()
{
	ZeroMemory(m_pLights.data(), sizeof(Light) * MAX_LIGHTS);
}

CScene::~CScene()
{
	// Release Objects
	ReleaseObjects();

	// Release Shader Variables
	ReleaseShaderVariables();

	// Release Root Signature
	//m_pd3dGraphicsRootSignature.Reset();
	//m_pd3dComputeRootSignature.Reset();

}

void CScene::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	// Scene 초기화
	PreInitializeObjects(pd3dDevice, pd3dCommandList, pd3dRootSignature);
	InitializeObjects(pd3dDevice, pd3dCommandList, pd3dRootSignature);
	PostInitializeObjects(pd3dDevice, pd3dCommandList, pd3dRootSignature);
}

void CScene::PreInitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	// Create Default Lights and Materials
	BuildDefaultLightsAndMaterials();
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
	CreateDefaultCamera(pd3dDevice, pd3dCommandList);
}

void CScene::InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{	
}

void CScene::PostInitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	// Fixed Camera
	SetSceneState(SCENE_STATE_RUNNING);
}

void CScene::CreateDefaultCamera(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_pCamera) return;

	auto pCameraObject = AddObject(std::make_unique<CGameObject>());
	auto pcameracomponent = pCameraObject->CreateComponent<CCamera>();
	pcameracomponent->SetViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	pcameracomponent->SetScissorRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	pcameracomponent->GenerateViewMatrix(XMFLOAT3(0.0f, 0.0f, -5.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	pcameracomponent->GenerateProjectionMatrix(((float)WINDOW_WIDTH / (float)WINDOW_HEIGHT), 60.0f, 1.0f, 1000.0f);
	pcameracomponent->CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CScene::DestroyFramework()
{
	// Release Root Signature
	m_pd3dGraphicsRootSignature.Reset();
	m_pd3dComputeRootSignature.Reset();

}

void CScene::CreateRootSignature(ID3D12RootSignature* pd3dRootSignature, ID3D12Device* pd3dDevice)
{
	if (!pd3dRootSignature) m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);
	else m_pd3dGraphicsRootSignature = pd3dRootSignature;
}

void CScene::CreateStaticShader(ID3D12Device* pd3dDevice)
{
	if (CMaterial::m_pStandardShader == nullptr)
	{
		CMaterial::m_pStandardShader = std::make_shared<CStandardShader>();
		CMaterial::m_pStandardShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature.Get());
	}
	if (CMaterial::m_pSkinnedAnimationShader == nullptr)
	{
		CMaterial::m_pSkinnedAnimationShader = std::make_shared<CSkinnedAnimationStandardShader>();
		CMaterial::m_pSkinnedAnimationShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature.Get());
	}
	if (CMaterial::m_pColliderShader == nullptr)
	{
		CMaterial::m_pColliderShader = std::make_shared<CColliderShader>();
		CMaterial::m_pColliderShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature.Get());
	}
}

void CScene::CreateStaticMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CResourceManager::Instance().SetMesh("CCubeMesh", std::make_shared<CCubeMesh>(pd3dDevice, pd3dCommandList, 1.0f, 1.0f, 1.0f));
	//CResourceManager::Instance().SetMesh("SphereMesh", std::make_shared<CSphereMesh>(pd3dDevice, pd3dCommandList));
}

void CScene::ReleaseObjects()
{
	// Release GameObjects (m_ppGameObjects가 소유권을 관리)
	m_ppGameObjects.clear();

	// Clear LayerView (포인터들은 소유권이 사라짐)
	m_ppLayerView.clear();

	// Reset special IDs / observers
	m_SkyBoxID = 0;
	m_TerrainID = 0;
	m_MapID = 0;
	m_PlayerID = 0;
	m_pPlayer = nullptr;

	// Release Lights
	ZeroMemory(m_pLights.data(), sizeof(Light) * MAX_LIGHTS);

	// Release Camera
	m_pCamera = nullptr;	
}

void CScene::ReleaseUploadBuffers()
{
	// Release Shader Variables
	for (auto& pGameObject : m_ppLayerView)
	{
		for (auto& pObject : pGameObject.second)
		{
			pObject->ReleaseUploadBuffers();
		}
	}
}

void CScene::InitStaticMembers(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	CreateRootSignature(pd3dRootSignature, pd3dDevice);
	CreateStaticShader(pd3dDevice);
}

void CScene::BuildDefaultLightsAndMaterials()
{
	ZeroMemory(&m_pLights, sizeof(Light) * MAX_LIGHTS);

	// Global Light
	m_xmf4GlobalAmbient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);

	// Light
	int nIndex = 0;
	m_pLights[nIndex].m_bEnable = true;
	m_pLights[nIndex].m_nType = DIRECTIONAL_LIGHT;
	m_pLights[nIndex].m_fRange = 2000.0f;
	m_pLights[nIndex].m_xmf4Ambient = XMFLOAT4(0.2f, 0.0f, 0.0f, 1.0f);
	m_pLights[nIndex].m_xmf4Diffuse = XMFLOAT4(0.73f, 0.73f, 0.73f, 1.0f);
	m_pLights[nIndex].m_xmf4Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
	m_pLights[nIndex].m_xmf3Position = XMFLOAT3(513 * 1.5f, 450.0f, 0);
	m_pLights[nIndex].m_xmf3Direction = XMFLOAT3(-1.0f, -1.0f, 0.0f);
	nIndex++;

	m_pLights[nIndex].m_bEnable = false;
	m_pLights[nIndex].m_nType = DIRECTIONAL_LIGHT;
	m_pLights[nIndex].m_fRange = 2000.0f;
	m_pLights[nIndex].m_xmf4Ambient = XMFLOAT4(0.2f, 0.0f, 0.0f, 1.0f);
	m_pLights[nIndex].m_xmf4Diffuse = XMFLOAT4(0.73f, 0.73f, 0.73f, 1.0f);
	m_pLights[nIndex].m_xmf4Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
	m_pLights[nIndex].m_xmf3Position = XMFLOAT3(513.0f, 450.0f, 0);
	m_pLights[nIndex].m_xmf3Direction = XMFLOAT3(-1.0f, -1.0f, 0.0f);
	nIndex++;


	/*m_pLights[1].m_bEnable = false;
	m_pLights[1].m_nType = SPOT_LIGHT;
	m_pLights[1].m_fRange = 1000.0f;
	m_pLights[1].m_xmf4Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_pLights[1].m_xmf4Diffuse = XMFLOAT4(0.54f, 0.54f, 0.54f, 1.0f);
	m_pLights[1].m_xmf4Specular = XMFLOAT4(0.13f, 0.13f, 0.13f, 0.0f);
	m_pLights[1].m_xmf3Position = XMFLOAT3(-50.0f, 120.0f, -5.0f);
	m_pLights[1].m_xmf3Direction = XMFLOAT3(0.0f, -1.0f, 1.0f);
	m_pLights[1].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.1f, 0.001f);
	m_pLights[1].m_fFalloff = 16.0f;
	m_pLights[1].m_fPhi = (float)cos(XMConvertToRadians(60.0f));
	m_pLights[1].m_fTheta = (float)cos(XMConvertToRadians(30.0f));

	m_pLights[2].m_bEnable = false;
	m_pLights[2].m_nType = SPOT_LIGHT;
	m_pLights[2].m_fRange = 500.0f;
	m_pLights[2].m_xmf4Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_pLights[2].m_xmf4Diffuse = XMFLOAT4(0.85f, 0.85f, 0.85f, 1.0f);
	m_pLights[2].m_xmf4Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	m_pLights[2].m_xmf3Position = XMFLOAT3(0.0f, 256.0f, 0.0f);
	m_pLights[2].m_xmf3Direction = XMFLOAT3(+1.0f, -1.0f, 0.0f);
	m_pLights[2].m_xmf3Attenuation = XMFLOAT3(0.5f, 0.01f, 0.0001f);
	m_pLights[2].m_fFalloff = 4.0f;
	m_pLights[2].m_fPhi = (float)cos(XMConvertToRadians(60.0f));
	m_pLights[2].m_fTheta = (float)cos(XMConvertToRadians(30.0f));

	m_pLights[3].m_bEnable = false;
	m_pLights[3].m_nType = DIRECTIONAL_LIGHT;
	m_pLights[3].m_fRange = 1000.0f;
	m_pLights[3].m_xmf4Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_pLights[3].m_xmf4Diffuse = XMFLOAT4(0.83f, 0.83f, 0.83f, 1.0f);
	m_pLights[3].m_xmf4Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
	m_pLights[3].m_xmf3Position = XMFLOAT3(0.0f, 128.0f, 0.0f);
	m_pLights[3].m_xmf3Direction = XMFLOAT3(+1.0f, -1.0f, 0.0f);*/
}

void CScene::ResetScene()
{
	//TODO : Scene의 모든 Object를 삭제가 아닌 초기화
	//ReleaseObjects(); <- 이게 아님
}

void CScene::PopScene()
{
	CGameFramework::Instance()->RequestSceneChange(CPopScene());
}

// --------------------------------------------
// 즉시 추가 (디버그 / 테스트)
// --------------------------------------------
CGameObject* CScene::AddObject(std::unique_ptr<CGameObject> object)
{
	if (!object)
		return nullptr;

	object->SetID(m_NextGameObjectID++);
	object->SetScene(this);

	CGameObject* rawPtr = object.get();

	RegisterLayerView(rawPtr);
	m_ppGameObjects.emplace(rawPtr->GetID(), std::move(object));

	{
		std::string debugMsg = to_string(GetSceneName()) + " - CScene::AddObject: Object Added. CID = " + std::to_string(rawPtr->GetID()) + ", Name = " + rawPtr->GetName() + "\n";
		OutputDebugStringA(debugMsg.c_str());
	}
	return rawPtr;
}

// --------------------------------------------
// 삭제 요청
// --------------------------------------------
void CScene::RequestDestroyObject(uint32_t id)
{
	m_RemoveQueue.push_back(id);
}

// --------------------------------------------
// Pending Request 처리
// --------------------------------------------

// Object 생성 / 삭제 요청 처리
void CScene::ProcessPendingRequest()
{
#ifdef _DEBUG
	if(m_CreateQueue.size() + m_RemoveQueue.size() > 0)
	{
		std::string debugMsg = to_string(GetSceneName()) + " - CScene::ProcessPendingRequest: Creating " + std::to_string(m_CreateQueue.size()) + " Objects, Removing " + std::to_string(m_RemoveQueue.size()) + " Objects.\n";
		OutputDebugStringA(debugMsg.c_str());
	}
#endif

	for (auto& object : m_CreateQueue)
	{
		// TODO : Initialize 시점에 ID3D12 요소	전달 필요 또는 별도로 가져오는 Init 함수 필요
		object->SetScene(this);

		CGameObject* rawPtr = object.get();
		RegisterLayerView(rawPtr);

		m_ppGameObjects.emplace(rawPtr->GetID(), std::move(object));
	}
	m_CreateQueue.clear();

	for (uint32_t id : m_RemoveQueue)
	{
		auto it = m_ppGameObjects.find(id);
		if (it != m_ppGameObjects.end())
		{
			UnregisterLayerView(it->second.get());
			m_ppGameObjects.erase(it);
		}
	}
	m_RemoveQueue.clear();
}

// --------------------------------------------
// Object 검색
// --------------------------------------------
CGameObject* CScene::FindObject(uint32_t id) const
{
	auto it = m_ppGameObjects.find(id);
	return (it != m_ppGameObjects.end()) ? it->second.get() : nullptr;
}

// --------------------------------------------
// Layer View 관리
// --------------------------------------------
void CScene::RegisterLayerView(CGameObject* object)
{
	if (!object)
		return;

	m_ppLayerView[object->GetLayer()].push_back(object);
}

void CScene::UnregisterLayerView(CGameObject* object)
{
	if (!object)
		return;

	auto& vec = m_ppLayerView[object->GetLayer()];
	vec.erase(
		std::remove(vec.begin(), vec.end(), object),
		vec.end()
	);
}

// Scene Update
void CScene::Update(float deltaTime)
{
	if (false == IsSceneRunning()) return;

	m_fElapsedTime = deltaTime;

	// Update GameObjects
	for (auto& pvecObjects : m_ppLayerView) for(auto& pObject : pvecObjects.second) pObject->Update(deltaTime);

	// Update Matrix
	for (auto& pvecObjects : m_ppLayerView) for (auto& pObject : pvecObjects.second)  pObject->UpdateTransform();

	UpdateLights();

	ProcessPendingRequest();
}

// Camera registry
void CScene::RegisterCamera(CCamera* pCamera)
{
	// 중복 등록 방지
	for (auto& camera : m_CameraRegistry)
	{
		if (camera == pCamera) return; // 이미 등록되어 있음
	}
	m_CameraRegistry.push_back(pCamera);

	if (m_nSelectedCamera == -1)
	{
		m_nSelectedCamera = 0; // 첫 번째 등록된 카메라를 기본으로 선택
		m_pCamera = pCamera;
	}

	// 리소스 매니저에도 등록
	CResourceManager::Instance().RegisterCamera(pCamera);
}


void CScene::UnregisterCamera(CCamera* pCamera)
{
	// 카메라 레지스트리에서 제거
	auto it = std::find(m_CameraRegistry.begin(), m_CameraRegistry.end(), pCamera);
	if (it != m_CameraRegistry.end()) m_CameraRegistry.erase(it);

	// 선택된 카메라가 제거된 경우, 인덱스를 재조정
	if (m_nSelectedCamera >= static_cast<int>(m_CameraRegistry.size()))
	{
		m_nSelectedCamera = static_cast<int>(m_CameraRegistry.size()) - 1;
	}
}

// 유효한 카메라가 있으면 우선순위에 따라 반환, 없으면 기존 m_pCamera(기본 카메라) 반환
CCamera* CScene::GetMainCamera()
{
	if (m_nSelectedCamera >= 0 && m_nSelectedCamera < static_cast<int>(m_CameraRegistry.size()))
	{
		return m_CameraRegistry[m_nSelectedCamera];
	}
	return m_pCamera;
}

// 사용할 카메라 선택
void CScene::SelectCamera(int nIndex)
{
	if (nIndex >= 0 && nIndex < static_cast<int>(m_CameraRegistry.size()))
	{
		m_nSelectedCamera = nIndex;
	}
}

void CScene::SelectCamera(CCamera* pCamera)
{
	auto it = std::find(m_CameraRegistry.begin(), m_CameraRegistry.end(), pCamera);
	if (it != m_CameraRegistry.end())
	{
		m_nSelectedCamera = static_cast<int>(std::distance(m_CameraRegistry.begin(), it));
	}
}

// Scene Render

bool CScene::PrepareRender(ID3D12GraphicsCommandList* pd3dCommandList)
{
	// Scene is not running or pausing
	if (false == IsSceneRunning()){	return false;}
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature.Get());

	// Set Descriptor Heap
	CResourceManager::Instance().PrepareRender(pd3dCommandList);

	UpdateShaderVariables(pd3dCommandList);

	return true;
}

bool CScene::OnPreRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (m_pDepthRenderShader) {
		m_pDepthRenderShader->PrepareShadowMap(pd3dCommandList, pCamera, this);
		return true;
	}
	return false;
}

bool CScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	// Scene is not running or pausing
	if (false == IsSceneRunning())	{return (false);}

	// Set Descriptor Heap
	/*ID3D12DescriptorHeap* ppHeaps[] = { m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap.Get() };
	pd3dCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);*/

	if(m_pDepthRenderShader) m_pDepthRenderShader->UpdateShaderVariables(pd3dCommandList);

	// Set Viewport and Scissor & Update Camera Variables
	if (nullptr == pCamera)
	{
		pCamera = GetMainCamera();
	}

	// if(m_pPlayer) pCamera->Update(m_pPlayer->GetPosition(), 0.0f);

	pCamera->SetViewportsAndScissorRects(pd3dCommandList);
	pCamera->UpdateShaderVariables(pd3dCommandList);

	// Update Shader Variables
	UpdateShaderVariables(pd3dCommandList);

	//if (m_pShadowShader) m_pShadowShader->Render(pd3dCommandList, pCamera);
	//if (m_pShadowMapToViewport) m_pShadowMapToViewport->Render(pd3dCommandList, pCamera);

	// Render GameObjects 
	for (auto& pvecObjects : m_ppLayerView)
	{
		for (auto& pObject : pvecObjects.second)
		{
			// pObject->Update(0.0f);
			pObject->Render(pd3dCommandList, pCamera, false);
		}
	}

	if (m_pShadowMapToViewport)
	{
		// Render Shadow Map to Viewport
		m_pShadowMapToViewport->Render(pd3dCommandList, pCamera);
	}

	return (true);
}

bool CScene::RenderUI(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	auto UIObjects = m_ppLayerView.find(GAMEOBJECT_LAYER::LAYER_UI);
	if(UIObjects != m_ppLayerView.end())
	{
		for (auto& pObject : UIObjects->second)
		{
			pObject->Render(pd3dCommandList, pCamera, false);
		}
		return true;
	}
	return false;
}

void CScene::RenderDepthWrite(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	std::set<GAMEOBJECT_LAYER> layer = {
		GAMEOBJECT_LAYER::LAYER_TERRAIN,
		GAMEOBJECT_LAYER::LAYER_ENVIRONMENT,
		GAMEOBJECT_LAYER::LAYER_PLAYER,
		GAMEOBJECT_LAYER::LAYER_ENEMY,
		GAMEOBJECT_LAYER::LAYER_GUN,
	};

	// Render GameObjects 
	for (auto& pvecObjects : m_ppLayerView)
	{
		if (false == layer.contains(pvecObjects.first))
			continue;
		for (auto& pObject : pvecObjects.second)
		{
			pObject->Update(0.0f);
			pObject->Render(pd3dCommandList, pCamera, true);
		}
	}
}

ComPtr<ID3D12RootSignature> CScene::CreateGraphicsRootSignature(ID3D12Device* pd3dDevice)
{
	ComPtr<ID3D12RootSignature> pd3dRootSignature = nullptr;

	// Descriptor Table
	std::vector<D3D12_DESCRIPTOR_RANGE> d3dDescriptorRanges;

	d3dDescriptorRanges.reserve(8);

	D3D12_DESCRIPTOR_RANGE d3dDescriptorRange;
	// Standard Texture
	{	
#ifdef _WITH_STANDARD_TEXTURE_MULTIPLE_PARAMETERS
		d3dDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		d3dDescriptorRange.NumDescriptors = 1;
		d3dDescriptorRange.BaseShaderRegister = 6;  //t6: gtxtAlbedoTexture
		d3dDescriptorRange.RegisterSpace = 0;
		d3dDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		d3dDescriptorRanges.push_back(d3dDescriptorRange);

		d3dDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		d3dDescriptorRange.NumDescriptors = 1;
		d3dDescriptorRange.BaseShaderRegister = 7; //t7: gtxtSpecularTexture
		d3dDescriptorRange.RegisterSpace = 0;
		d3dDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		d3dDescriptorRanges.push_back(d3dDescriptorRange);

		d3dDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		d3dDescriptorRange.NumDescriptors = 1;
		d3dDescriptorRange.BaseShaderRegister = 8; //t8: gtxtNormalTexture
		d3dDescriptorRange.RegisterSpace = 0;
		d3dDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		d3dDescriptorRanges.push_back(d3dDescriptorRange);

		d3dDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		d3dDescriptorRange.NumDescriptors = 1;
		d3dDescriptorRange.BaseShaderRegister = 9; //t9: gtxtMetallicTexture
		d3dDescriptorRange.RegisterSpace = 0;
		d3dDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		d3dDescriptorRanges.push_back(d3dDescriptorRange);

		d3dDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		d3dDescriptorRange.NumDescriptors = 1;
		d3dDescriptorRange.BaseShaderRegister = 10; //t10: gtxtEmissionTexture
		d3dDescriptorRange.RegisterSpace = 0;
		d3dDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		d3dDescriptorRanges.push_back(d3dDescriptorRange);

		d3dDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		d3dDescriptorRange.NumDescriptors = 1;
		d3dDescriptorRange.BaseShaderRegister = 11; //t11: gtxtDetailAlbedoTexture
		d3dDescriptorRange.RegisterSpace = 0;
		d3dDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		d3dDescriptorRanges.push_back(d3dDescriptorRange);

		d3dDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		d3dDescriptorRange.NumDescriptors = 1;
		d3dDescriptorRange.BaseShaderRegister = 12; //t12: gtxtDetailNormalTexture
		d3dDescriptorRange.RegisterSpace = 0;
		d3dDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		d3dDescriptorRanges.push_back(d3dDescriptorRange);
#else
		// gtxtStandardTextures[7] 로 사용시
		d3dDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		d3dDescriptorRange.NumDescriptors = 7;
		d3dDescriptorRange.BaseShaderRegister = 6; //t6 ~ t12: gtxtStandardTextures[7]
		d3dDescriptorRange.RegisterSpace = 0;
		d3dDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		d3dDescriptorRanges.push_back(d3dDescriptorRange);
#endif
	}

	// SkyBox Texture
	{
		d3dDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		d3dDescriptorRange.NumDescriptors = 1;
		d3dDescriptorRange.BaseShaderRegister = 13; // t13: gtxtSkyCubeTexture
		d3dDescriptorRange.RegisterSpace = 0;
		d3dDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		d3dDescriptorRanges.push_back(d3dDescriptorRange);
	}

	// Depth Write Texture
	{
		d3dDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		d3dDescriptorRange.NumDescriptors = MAX_DEPTH_TEXTURES;
		d3dDescriptorRange.BaseShaderRegister = 14; // t14: Depth Buffer
		d3dDescriptorRange.RegisterSpace = 0;
		d3dDescriptorRange.OffsetInDescriptorsFromTableStart = 0;

		d3dDescriptorRanges.push_back(d3dDescriptorRange);
	}

	// Random Buffer
	{
		d3dDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		d3dDescriptorRange.NumDescriptors = 1;
		d3dDescriptorRange.BaseShaderRegister = 30; //t30: gtxtRandomTexture
		d3dDescriptorRange.RegisterSpace = 0;
		d3dDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		d3dDescriptorRanges.push_back(d3dDescriptorRange);

		d3dDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		d3dDescriptorRange.NumDescriptors = 1;
		d3dDescriptorRange.BaseShaderRegister = 31; //t31: gtxtRandomOnSphereTexture
		d3dDescriptorRange.RegisterSpace = 0;
		d3dDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		d3dDescriptorRanges.push_back(d3dDescriptorRange);
	}

	// Root Parameter 
	std::vector<D3D12_ROOT_PARAMETER> pd3dRootParameters(11); // 11 = Object ~ Light (5) + Texture(1) + Skybox(1) + Skinning(2) + Depth write(1) + ToLight(1)

	int nDescriptorIndexCounter = 0;

#ifdef _USE_OBJECT_MATERIAL_CBV
	// 0
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].Descriptor.ShaderRegister = ROOT_PARAMETER_OBJECT; // b0 : cbGameObjectInfo
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	// 1
	pd3dRootParameters[ROOT_PARAMETER_MATERIAL].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[ROOT_PARAMETER_MATERIAL].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_MATERIAL].Descriptor.ShaderRegister = ROOT_PARAMETER_MATERIAL; // b1 : cbMaterialInfo
	pd3dRootParameters[ROOT_PARAMETER_MATERIAL].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
#else
	// Object
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	//pd3dRootParameters[ROOT_PARAMETER_OBJECT].Constants.Num32BitValues = 16;
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].Constants.Num32BitValues = 21;
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].Constants.ShaderRegister = ROOT_PARAMETER_OBJECT; // b0 : GameObject
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].Constants.RegisterSpace = 0;
	//pd3dRootParameters[ROOT_PARAMETER_OBJECT].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	// Material
	pd3dRootParameters[ROOT_PARAMETER_MATERIAL].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[ROOT_PARAMETER_MATERIAL].Constants.Num32BitValues = 17;
	pd3dRootParameters[ROOT_PARAMETER_MATERIAL].Constants.ShaderRegister = ROOT_PARAMETER_MATERIAL; // b1 : Material
	pd3dRootParameters[ROOT_PARAMETER_MATERIAL].Constants.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_MATERIAL].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
#endif // _USE_OBJECT_MATERIAL_CBV
	// Camera
	pd3dRootParameters[ROOT_PARAMETER_CAMERA].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[ROOT_PARAMETER_CAMERA].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_CAMERA].Descriptor.ShaderRegister = ROOT_PARAMETER_CAMERA; // b2 : cbCamera
	pd3dRootParameters[ROOT_PARAMETER_CAMERA].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	// Framework
	pd3dRootParameters[ROOT_PARAMETER_FRAMEWORK].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[ROOT_PARAMETER_FRAMEWORK].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_FRAMEWORK].Descriptor.ShaderRegister = ROOT_PARAMETER_FRAMEWORK; // b3 : cbFramework
	pd3dRootParameters[ROOT_PARAMETER_FRAMEWORK].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	// Light
	pd3dRootParameters[ROOT_PARAMETER_LIGHT].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[ROOT_PARAMETER_LIGHT].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_LIGHT].Descriptor.ShaderRegister = ROOT_PARAMETER_LIGHT; // b4 : cbLight
	pd3dRootParameters[ROOT_PARAMETER_LIGHT].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	// Textures
#ifdef _WITH_STANDARD_TEXTURE_MULTIPLE_PARAMETERS
	// 추가될 파라미터 수 만큼 resize
	pd3dRootParameters.resize(pd3dRootParameters.size() + 6 + 2);

	pd3dRootParameters[ROOT_PARAMETER_ALBEDO_TEXTURE].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_ALBEDO_TEXTURE].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_ALBEDO_TEXTURE].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[nDescriptorIndexCounter++];
	pd3dRootParameters[ROOT_PARAMETER_ALBEDO_TEXTURE].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[ROOT_PARAMETER_SPECULAR_TEXTURE].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_SPECULAR_TEXTURE].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_SPECULAR_TEXTURE].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[nDescriptorIndexCounter++];
	pd3dRootParameters[ROOT_PARAMETER_SPECULAR_TEXTURE].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[ROOT_PARAMETER_NORMAL_TEXTURE].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_NORMAL_TEXTURE].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_NORMAL_TEXTURE].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[nDescriptorIndexCounter++];
	pd3dRootParameters[ROOT_PARAMETER_NORMAL_TEXTURE].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[ROOT_PARAMETER_METALLIC_TEXTURE].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_METALLIC_TEXTURE].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_METALLIC_TEXTURE].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[nDescriptorIndexCounter++];
	pd3dRootParameters[ROOT_PARAMETER_METALLIC_TEXTURE].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[ROOT_PARAMETER_EMISSION_TEXTURE].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_EMISSION_TEXTURE].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_EMISSION_TEXTURE].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[nDescriptorIndexCounter++];
	pd3dRootParameters[ROOT_PARAMETER_EMISSION_TEXTURE].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[ROOT_PARAMETER_DETAIL_ALBEDO_TEXTURE].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_DETAIL_ALBEDO_TEXTURE].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_DETAIL_ALBEDO_TEXTURE].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[nDescriptorIndexCounter++];
	pd3dRootParameters[ROOT_PARAMETER_DETAIL_ALBEDO_TEXTURE].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[ROOT_PARAMETER_DETAIL_NORMAL_TEXTURE].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_DETAIL_NORMAL_TEXTURE].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_DETAIL_NORMAL_TEXTURE].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[nDescriptorIndexCounter++];
	pd3dRootParameters[ROOT_PARAMETER_DETAIL_NORMAL_TEXTURE].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
#else
	pd3dRootParameters[ROOT_PARAMETER_STANDARD_TEXTURES].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_STANDARD_TEXTURES].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_STANDARD_TEXTURES].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[nDescriptorIndexCounter++];
	pd3dRootParameters[ROOT_PARAMETER_STANDARD_TEXTURES].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
#endif
	// Skybox
	pd3dRootParameters[ROOT_PARAMETER_SKYBOX].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_SKYBOX].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_SKYBOX].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[nDescriptorIndexCounter++];
	pd3dRootParameters[ROOT_PARAMETER_SKYBOX].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// Skin Mesh Bone
	pd3dRootParameters[ROOT_PARAMETER_SKINNED_BONE_OFFSETS].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[ROOT_PARAMETER_SKINNED_BONE_OFFSETS].Descriptor.ShaderRegister = 7; //Skinned Bone Offsets
	pd3dRootParameters[ROOT_PARAMETER_SKINNED_BONE_OFFSETS].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_SKINNED_BONE_OFFSETS].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	pd3dRootParameters[ROOT_PARAMETER_SKINNED_BONE_TRANSFORM].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[ROOT_PARAMETER_SKINNED_BONE_TRANSFORM].Descriptor.ShaderRegister = 8; //Skinned Bone Transforms
	pd3dRootParameters[ROOT_PARAMETER_SKINNED_BONE_TRANSFORM].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_SKINNED_BONE_TRANSFORM].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// Depth Write
	pd3dRootParameters[ROOT_PARAMETER_DEPTH_WRITE].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_DEPTH_WRITE].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_DEPTH_WRITE].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[nDescriptorIndexCounter++];
	pd3dRootParameters[ROOT_PARAMETER_DEPTH_WRITE].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// ToLight
	pd3dRootParameters[ROOT_PARAMETER_TO_LIGHT].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[ROOT_PARAMETER_TO_LIGHT].Descriptor.ShaderRegister = 5; // b5 : cbToLight
	pd3dRootParameters[ROOT_PARAMETER_TO_LIGHT].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_TO_LIGHT].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Random Buffer
	pd3dRootParameters[ROOT_PARAMETER_RANDOMBUFFER].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_RANDOMBUFFER].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_RANDOMBUFFER].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[nDescriptorIndexCounter++];
	pd3dRootParameters[ROOT_PARAMETER_RANDOMBUFFER].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[ROOT_PARAMETER_RANDOM_SPHERE_BUFFER].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_RANDOM_SPHERE_BUFFER].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_RANDOM_SPHERE_BUFFER].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[nDescriptorIndexCounter++];
	pd3dRootParameters[ROOT_PARAMETER_RANDOM_SPHERE_BUFFER].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Static Sampler
	std::vector<D3D12_STATIC_SAMPLER_DESC> pd3dStaticSamplerDescs(4);

	// WRAP Sampler [0.0 ~ 1.0)
	pd3dStaticSamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dStaticSamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dStaticSamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dStaticSamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dStaticSamplerDescs[0].MipLODBias = 0;
	pd3dStaticSamplerDescs[0].MaxAnisotropy = 1;
	pd3dStaticSamplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dStaticSamplerDescs[0].MinLOD = 0;
	pd3dStaticSamplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dStaticSamplerDescs[0].ShaderRegister = 0;
	pd3dStaticSamplerDescs[0].RegisterSpace = 0;
	pd3dStaticSamplerDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dStaticSamplerDescs[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dStaticSamplerDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dStaticSamplerDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dStaticSamplerDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dStaticSamplerDescs[1].MipLODBias = 0;
	pd3dStaticSamplerDescs[1].MaxAnisotropy = 1;
	pd3dStaticSamplerDescs[1].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dStaticSamplerDescs[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	pd3dStaticSamplerDescs[1].MinLOD = 0;
	pd3dStaticSamplerDescs[1].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dStaticSamplerDescs[1].ShaderRegister = 1;
	pd3dStaticSamplerDescs[1].RegisterSpace = 0;
	pd3dStaticSamplerDescs[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dStaticSamplerDescs[2].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	pd3dStaticSamplerDescs[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dStaticSamplerDescs[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dStaticSamplerDescs[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dStaticSamplerDescs[2].MipLODBias = 0.0f;
	pd3dStaticSamplerDescs[2].MaxAnisotropy = 1;
	pd3dStaticSamplerDescs[2].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; //D3D12_COMPARISON_FUNC_LESS
	pd3dStaticSamplerDescs[2].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE; // D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	pd3dStaticSamplerDescs[2].MinLOD = 0;
	pd3dStaticSamplerDescs[2].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dStaticSamplerDescs[2].ShaderRegister = 2;
	pd3dStaticSamplerDescs[2].RegisterSpace = 0;
	pd3dStaticSamplerDescs[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dStaticSamplerDescs[3].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dStaticSamplerDescs[3].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dStaticSamplerDescs[3].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dStaticSamplerDescs[3].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dStaticSamplerDescs[3].MipLODBias = 0.0f;
	pd3dStaticSamplerDescs[3].MaxAnisotropy = 1;
	pd3dStaticSamplerDescs[3].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dStaticSamplerDescs[3].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	pd3dStaticSamplerDescs[3].MinLOD = 0;
	pd3dStaticSamplerDescs[3].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dStaticSamplerDescs[3].ShaderRegister = 3;
	pd3dStaticSamplerDescs[3].RegisterSpace = 0;
	pd3dStaticSamplerDescs[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// Root Signature Flags
	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
	//	D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
	//	D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
	//D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT;

	// Root Signature Description
	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
	::ZeroMemory(&d3dRootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
	d3dRootSignatureDesc.NumParameters = (UINT)pd3dRootParameters.size();
	d3dRootSignatureDesc.pParameters = pd3dRootParameters.data();
	d3dRootSignatureDesc.NumStaticSamplers = (UINT)pd3dStaticSamplerDescs.size();
	d3dRootSignatureDesc.pStaticSamplers = pd3dStaticSamplerDescs.data();
	d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;

	// Serialize Root Signature
	ID3DBlob* pd3dSignatureBlob = NULL;
	ID3DBlob* pd3dErrorBlob = NULL;
	HRESULT hResult = D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);

	if (FAILED(hResult))
	{
		if (pd3dErrorBlob) OutputDebugStringA((char*)pd3dErrorBlob->GetBufferPointer());
		return (nullptr);
	}

	// Create Root Signature
	hResult = pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)pd3dRootSignature.GetAddressOf());

	// Release Blob
	if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
	if (pd3dErrorBlob) pd3dErrorBlob->Release();

	return (pd3dRootSignature);
}

void CScene::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	// Light
	CreateLightShaderVariables(pd3dDevice, pd3dCommandList);
}

void CScene::CreateLightShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	// Create Constant Buffer
	UINT ncbElementBytes = ((sizeof(CB_LIGHT_INFO) + 255) & ~255); //256의 배수
	m_pd3dcbLights = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_GENERIC_READ, NULL);

	// Map Constant Buffer
	m_pd3dcbLights->Map(0, nullptr, (void**)&m_pcbMappedLights);
	ZeroMemory(m_pcbMappedLights, sizeof(CB_LIGHT_INFO));
}

void CScene::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	UpdateLightShaderVariables(pd3dCommandList);
}

void CScene::UpdateLightShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	// Light 
	memcpy(&m_pcbMappedLights->m_pLights, m_pLights.data(), sizeof(Light) * m_pLights.size());
	m_pcbMappedLights->m_xmf4GlobalAmbient = m_xmf4GlobalAmbient;
	m_pcbMappedLights->m_nLights = (UINT)m_pLights.size();

	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = m_pd3dcbLights->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(ROOT_PARAMETER_LIGHT, d3dGpuVirtualAddress);
}

void CScene::ReleaseShaderVariables()
{
	// Light 
	ReleaseLightShaderVariables();
}

void CScene::ReleaseLightShaderVariables()
{
	if (m_pd3dcbLights) m_pd3dcbLights->Unmap(0, nullptr);
	m_pd3dcbLights.Reset();
	m_pcbMappedLights = nullptr;
}

void CScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		m_bMouseLButtonDown = true;
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		m_bMouseLButtonDown = false;
		break;
	}
}

BoundingBox CScene::CalculateBoundingBox()
{
	BoundingBox ret;
	for (auto& pvecObject : m_ppLayerView) {
		for (auto& pObject : pvecObject.second) {
			BoundingBox box = pObject->GetMergedMeshBound();
			ret.CreateMerged(ret, box, ret);
		}
	}
	return ret;
}

void CScene::SetPlayer(std::unique_ptr<CPlayer>& pPlayer)
{
	if (!pPlayer) return;

	// AddObject은 ID 부여 및 LayerView 등록을 수행함
	CGameObject* rawPtr = AddObject(std::move(pPlayer)); // 소유권이 m_ppGameObjects로 이동
	if (rawPtr)
	{
		m_PlayerID = rawPtr->GetID();
		m_pPlayer = dynamic_cast<CPlayer*>(rawPtr); // observer pointer (소유권 없음)
	}
	else
	{
		m_PlayerID = 0;
		m_pPlayer = nullptr;
	}
}

