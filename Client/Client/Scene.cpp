///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-28
// Scene.cpp : Scene 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#include "Scene.h"
#include "GameFramework.h"

std::shared_ptr<CDescirptorHeap> CScene::m_pDescriptorHeap;
ComPtr<ID3D12RootSignature> CScene::m_pd3dGraphicsRootSignature;
ComPtr<ID3D12RootSignature> CScene::m_pd3dComputeRootSignature;

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
	m_pd3dGraphicsRootSignature.Reset();
	m_pd3dComputeRootSignature.Reset();

	// Scene 종료
	m_SceneState = SCENE_STATE_ENDING;
}

void CScene::Init(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	// Scene 초기화
	PreInitializeObjects(pd3dDevice, pd3dCommandList, pd3dRootSignature);
	InitializeObjects(pd3dDevice, pd3dCommandList, pd3dRootSignature);
	PostInitializeObjects();
}

void CScene::PreInitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	m_SceneState = SCENE_STATE_ALLOCING;

	// Create Default Lights and Materials
	BuildDefaultLightsAndMaterials();

	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	CreateRootSignature(pd3dRootSignature, pd3dDevice);
	CreateDescriptorHeap(pd3dDevice);
	CreateStaticShader(pd3dDevice);

	// Fixed Camera
	CreateFixedCamera(pd3dDevice, pd3dCommandList);
}

void CScene::InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{	
}

void CScene::PostInitializeObjects()
{
	// Scene 생성 완료
	m_SceneState = SCENE_STATE_RUNNING;
}

void CScene::CreateFixedCamera(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	m_pCamera = std::make_shared<CCamera>();
	m_pCamera->SetViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	m_pCamera->SetScissorRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	m_pCamera->GenerateViewMatrix(XMFLOAT3(0.0f, 0.0f, -5.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_pCamera->GenerateProjectionMatrix(((float)WINDOW_WIDTH / (float)WINDOW_HEIGHT), 60.0f, 1.0f, 1000.0f);
	m_pCamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CScene::DestroyFramework()
{
	// Release Root Signature
	m_pd3dGraphicsRootSignature.Reset();
	m_pd3dComputeRootSignature.Reset();

	// Release Descriptor Heap
	m_pDescriptorHeap->~CDescirptorHeap();
}

void CScene::CreateRootSignature(ID3D12RootSignature* pd3dRootSignature, ID3D12Device* pd3dDevice)
{
	if (!pd3dRootSignature) m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);
	else m_pd3dGraphicsRootSignature = pd3dRootSignature;
}

void CScene::CreateDescriptorHeap(ID3D12Device* pd3dDevice)
{
	if (!m_pDescriptorHeap)
	{
		m_pDescriptorHeap = std::make_shared<CDescirptorHeap>();
		CreateCbvSrvDescriptorHeaps(pd3dDevice, 100, 100);
	}
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
}

void CScene::ReleaseObjects()
{
	// Release GameObjects
	m_ppObjects.clear();
	m_ppHierarchicalObjects.clear();

	// Release Terrain
	m_pTerrain.reset();

	// Release SkyBox
	m_pSkyBox.reset();

	// Release Lights
	ZeroMemory(m_pLights.data(), sizeof(Light) * MAX_LIGHTS);

	// Release Camera
	m_pCamera.reset();	
}

void CScene::ReleaseUploadBuffers()
{
}

void CScene::BuildDefaultLightsAndMaterials()
{
	ZeroMemory(&m_pLights, sizeof(Light) * MAX_LIGHTS);

	// Global Light
	m_xmf4GlobalAmbient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);

	// Light
	m_pLights[0].m_bEnable = true;
	m_pLights[0].m_nType = POINT_LIGHT;
	m_pLights[0].m_fRange = 1000.0f;
	m_pLights[0].m_xmf4Ambient = XMFLOAT4(0.1f, 0.0f, 0.0f, 1.0f);
	m_pLights[0].m_xmf4Diffuse = XMFLOAT4(0.8f, 0.0f, 0.0f, 1.0f);
	m_pLights[0].m_xmf4Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 0.0f);
	m_pLights[0].m_xmf3Position = XMFLOAT3(30.0f, 30.0f, 30.0f);
	m_pLights[0].m_xmf3Direction = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_pLights[0].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.001f, 0.0001f);

	m_pLights[1].m_bEnable = true;
	m_pLights[1].m_nType = SPOT_LIGHT;
	m_pLights[1].m_fRange = 500.0f;
	m_pLights[1].m_xmf4Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_pLights[1].m_xmf4Diffuse = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	m_pLights[1].m_xmf4Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
	m_pLights[1].m_xmf3Position = XMFLOAT3(0.0f, 0.0f, -100.0f);
	m_pLights[1].m_xmf3Direction = XMFLOAT3(0.0f, 0.0f, 1.0f);
	m_pLights[1].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.01f, 0.0001f);
	m_pLights[1].m_fFalloff = 8.0f;
	m_pLights[1].m_fPhi = (float)cos(XMConvertToRadians(40.0f));
	m_pLights[1].m_fTheta = (float)cos(XMConvertToRadians(20.0f));

	m_pLights[2].m_bEnable = true;
	m_pLights[2].m_nType = DIRECTIONAL_LIGHT;
	m_pLights[2].m_xmf4Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_pLights[2].m_xmf4Diffuse = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	m_pLights[2].m_xmf4Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 0.0f);
	m_pLights[2].m_xmf3Direction = XMFLOAT3(1.0f, 0.0f, 0.0f);

	m_pLights[3].m_bEnable = true;
	m_pLights[3].m_nType = SPOT_LIGHT;
	m_pLights[3].m_fRange = 600.0f;
	m_pLights[3].m_xmf4Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_pLights[3].m_xmf4Diffuse = XMFLOAT4(0.3f, 0.7f, 0.0f, 1.0f);
	m_pLights[3].m_xmf4Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
	m_pLights[3].m_xmf3Position = XMFLOAT3(50.0f, 30.0f, 30.0f);
	m_pLights[3].m_xmf3Direction = XMFLOAT3(0.0f, 1.0f, 1.0f);
	m_pLights[3].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.01f, 0.0001f);
	m_pLights[3].m_fFalloff = 8.0f;
	m_pLights[3].m_fPhi = (float)cos(XMConvertToRadians(90.0f));
	m_pLights[3].m_fTheta = (float)cos(XMConvertToRadians(30.0f));
}

void CScene::FixedUpdate(float deltaTime)
{
	if (false == CheckWorkUpdating())
	{
		// Scene is not running
		return;
	}

	m_fElapsedTime = deltaTime;

	// Update GameObjects
	for (auto& pObject : m_ppObjects)
	{
		pObject->Update(deltaTime);
	}

	// Collision Check and Resolve

	// Update Matrix
	for (auto& pObject : m_ppObjects)
	{
		pObject->UpdateTransform(nullptr);
	}
}

bool CScene::PrepareRender(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (false == CheckWorkRendering())
	{
		// Scene is not running or pausing
		return false;
	}
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature.Get());
	return true;
}

bool CScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (false == CheckWorkRendering())
	{
		// Scene is not running or pausing
		return (false);
	}

	// Set Root Signature
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature.Get());

	// Set Descriptor Heap
	ID3D12DescriptorHeap* ppHeaps[] = { m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap.Get() };
	pd3dCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// Set Viewport and Scissor & Update Camera Variables
	if (pCamera)
	{
		pCamera->SetViewportsAndScissorRects(pd3dCommandList);
		pCamera->UpdateShaderVariables(pd3dCommandList);
	}
	else {
		// Set Default Viewport and Scissor
		m_pCamera->SetViewportsAndScissorRects(pd3dCommandList);
		m_pCamera->UpdateShaderVariables(pd3dCommandList);

		pCamera = m_pCamera.get();
	}

	// Update Shader Variables
	UpdateShaderVariables(pd3dCommandList);

	// Render Terrain
	if (m_pTerrain)
	{
		m_pTerrain->Render(pd3dCommandList, pCamera);
	}

	// Render GameObjects 
	for (auto& pObject : m_ppObjects)
	{
		pObject->Render(pd3dCommandList, pCamera);
	}

	for (auto& pObject : m_ppHierarchicalObjects)
	{
		pObject->Update(m_fElapsedTime);
		if (!pObject->m_pSkinnedAnimationController) pObject->UpdateTransform(NULL);
		pObject->Render(pd3dCommandList, pCamera);
	}

	// Render SkyBox
	if (m_pSkyBox)
	{
		m_pSkyBox->Render(pd3dCommandList, pCamera);
	}

	return (true);
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

	// Root Parameter 
	std::vector<D3D12_ROOT_PARAMETER> pd3dRootParameters(9);

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
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].Constants.Num32BitValues = 16;
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].Constants.ShaderRegister = ROOT_PARAMETER_OBJECT; // b0 : GameObject
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].Constants.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
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
	pd3dRootParameters.resize(pd3dRootParameters.size() + 6);

	pd3dRootParameters[ROOT_PARAMETER_ALBEDO_TEXTURE].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_ALBEDO_TEXTURE].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_ALBEDO_TEXTURE].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[0];
	pd3dRootParameters[ROOT_PARAMETER_ALBEDO_TEXTURE].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[ROOT_PARAMETER_SPECULAR_TEXTURE].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_SPECULAR_TEXTURE].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_SPECULAR_TEXTURE].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[1];
	pd3dRootParameters[ROOT_PARAMETER_SPECULAR_TEXTURE].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[ROOT_PARAMETER_NORMAL_TEXTURE].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_NORMAL_TEXTURE].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_NORMAL_TEXTURE].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[2];
	pd3dRootParameters[ROOT_PARAMETER_NORMAL_TEXTURE].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[ROOT_PARAMETER_METALLIC_TEXTURE].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_METALLIC_TEXTURE].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_METALLIC_TEXTURE].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[3];
	pd3dRootParameters[ROOT_PARAMETER_METALLIC_TEXTURE].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[ROOT_PARAMETER_EMISSION_TEXTURE].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_EMISSION_TEXTURE].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_EMISSION_TEXTURE].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[4];
	pd3dRootParameters[ROOT_PARAMETER_EMISSION_TEXTURE].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[ROOT_PARAMETER_DETAIL_ALBEDO_TEXTURE].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_DETAIL_ALBEDO_TEXTURE].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_DETAIL_ALBEDO_TEXTURE].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[5];
	pd3dRootParameters[ROOT_PARAMETER_DETAIL_ALBEDO_TEXTURE].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[ROOT_PARAMETER_DETAIL_NORMAL_TEXTURE].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_DETAIL_NORMAL_TEXTURE].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_DETAIL_NORMAL_TEXTURE].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[6];
	pd3dRootParameters[ROOT_PARAMETER_DETAIL_NORMAL_TEXTURE].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
#else
	pd3dRootParameters[ROOT_PARAMETER_STANDARD_TEXTURES].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_STANDARD_TEXTURES].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_STANDARD_TEXTURES].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[0];
	pd3dRootParameters[ROOT_PARAMETER_STANDARD_TEXTURES].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
#endif
	// Skybox
	pd3dRootParameters[ROOT_PARAMETER_SKYBOX].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_SKYBOX].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_SKYBOX].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[ROOT_PARAMETER_SKYBOX - ROOT_PARAMETER_LIGHT - 1];
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

	// Static Sampler
	std::vector<D3D12_STATIC_SAMPLER_DESC> pd3dStaticSamplerDescs(1);

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

	// Root Signature Flags
	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | 
	//	D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT |
	//	D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
	//	D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	
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

	// Create Root Signature
	hResult = pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)pd3dRootSignature.GetAddressOf());

	// Release Blob
	if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
	if (pd3dErrorBlob) pd3dErrorBlob->Release();

	return (pd3dRootSignature);
}

void CScene::CreateCbvSrvDescriptorHeaps(ID3D12Device* pd3dDevice, int nConstantBufferViews, int nShaderResourceViews)
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	d3dDescriptorHeapDesc.NumDescriptors = nConstantBufferViews + nShaderResourceViews; //CBVs + SRVs 
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap);

	m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle = m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle = m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	m_pDescriptorHeap->m_d3dSrvCPUDescriptorStartHandle.ptr = m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * nConstantBufferViews);
	m_pDescriptorHeap->m_d3dSrvGPUDescriptorStartHandle.ptr = m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * nConstantBufferViews);

	m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle;
	m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle;
	m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dSrvCPUDescriptorStartHandle;
	m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dSrvGPUDescriptorStartHandle;

}

void CScene::CreateConstantBufferViews(ID3D12Device* pd3dDevice, int nConstantBufferViews, ID3D12Resource* pd3dConstantBuffers, UINT nStride)
{
	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = pd3dConstantBuffers->GetGPUVirtualAddress();
	D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCBVDesc;
	d3dCBVDesc.SizeInBytes = nStride;
	for (int j = 0; j < nConstantBufferViews; j++)
	{
		d3dCBVDesc.BufferLocation = d3dGpuVirtualAddress + (nStride * j);
		pd3dDevice->CreateConstantBufferView(&d3dCBVDesc, m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
		m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	}
}

D3D12_GPU_DESCRIPTOR_HANDLE CScene::CreateConstantBufferView(ID3D12Device* pd3dDevice, ID3D12Resource* pd3dConstantBuffer, UINT nStride)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCBVDesc;
	d3dCBVDesc.SizeInBytes = nStride;
	d3dCBVDesc.BufferLocation = pd3dConstantBuffer->GetGPUVirtualAddress();
	pd3dDevice->CreateConstantBufferView(&d3dCBVDesc, m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dCbvGPUDescriptorHandle = m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle;
	m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

	return(d3dCbvGPUDescriptorHandle);
}

D3D12_GPU_DESCRIPTOR_HANDLE CScene::CreateConstantBufferView(ID3D12Device* pd3dDevice, D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress, UINT nStride)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCBVDesc;
	d3dCBVDesc.SizeInBytes = nStride;
	d3dCBVDesc.BufferLocation = d3dGpuVirtualAddress;
	pd3dDevice->CreateConstantBufferView(&d3dCBVDesc, m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dCbvGPUDescriptorHandle = m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle;
	m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	
	return(d3dCbvGPUDescriptorHandle);
}

void CScene::CreateShaderResourceViews(ID3D12Device* pd3dDevice, CTexture* pTexture, UINT nDescriptorHeapIndex, UINT nRootParameterStartIndex)
{
	m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += (::gnCbvSrvDescriptorIncrementSize * nDescriptorHeapIndex);
	m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += (::gnCbvSrvDescriptorIncrementSize * nDescriptorHeapIndex);

	int nTextures = pTexture->GetTextures();
	for (int i = 0; i < nTextures; i++)
	{
		ID3D12Resource* pShaderResource = pTexture->GetResource(i);
		D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = pTexture->GetShaderResourceViewDesc(i);
		pd3dDevice->CreateShaderResourceView(pShaderResource, &d3dShaderResourceViewDesc, m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

		pTexture->SetGpuDescriptorHandle(i, m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

	}
	int nRootParameters = pTexture->GetRootParameters();
	for (int i = 0; i < nRootParameters; i++) pTexture->SetRootParameterIndex(i, nRootParameterStartIndex + i);
}

void CScene::CreateShaderResourceView(ID3D12Device* pd3dDevice, CTexture* pTexture, int nIndex, UINT nRootParameterStartIndex)
{
	ID3D12Resource* pShaderResource = pTexture->GetResource(nIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dGpuDescriptorHandle = pTexture->GetGpuDescriptorHandle(nIndex);
	if (pShaderResource && !d3dGpuDescriptorHandle.ptr)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = pTexture->GetShaderResourceViewDesc(nIndex);
		pd3dDevice->CreateShaderResourceView(pShaderResource, &d3dShaderResourceViewDesc, m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

		pTexture->SetGpuDescriptorHandle(nIndex, m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

		pTexture->SetRootParameterIndex(nIndex, nRootParameterStartIndex + nIndex);
	}

}

void CScene::CreateShaderResourceView(ID3D12Device* pd3dDevice, CTexture* pTexture, int nIndex)
{
	ID3D12Resource* pShaderResource = pTexture->GetResource(nIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dGpuDescriptorHandle = pTexture->GetGpuDescriptorHandle(nIndex);
	if (pShaderResource && !d3dGpuDescriptorHandle.ptr)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = pTexture->GetShaderResourceViewDesc(nIndex);
		pd3dDevice->CreateShaderResourceView(pShaderResource, &d3dShaderResourceViewDesc, m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

		pTexture->SetGpuDescriptorHandle(nIndex, m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

	}

}

void CScene::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	// Light
	// Create Constant Buffer
	UINT ncbElementBytes = ((sizeof(CB_LIGHT_INFO) + 255) & ~255); //256의 배수
	m_pd3dcbLights= ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_GENERIC_READ, NULL);

	// Map Constant Buffer
	m_pd3dcbLights->Map(0, nullptr, (void**)&m_pcbMappedLights);
	ZeroMemory(m_pcbMappedLights, sizeof(CB_LIGHT_INFO));
}

void CScene::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
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
	if (m_pd3dcbLights) m_pd3dcbLights->Unmap(0, nullptr);
	m_pd3dcbLights.Reset();
	m_pcbMappedLights = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
// CLoadingScene.cpp : CLoadingScene 클래스의 구현 파일

CLoadingScene::CLoadingScene()
{
}

CLoadingScene::~CLoadingScene()
{
}

void CLoadingScene::InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
		// Create Objects
	std::shared_ptr<CMaterial> pMaterial = std::make_shared<CMaterial>();
	pMaterial->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	std::shared_ptr<CShader> pStandardShader = std::make_shared<CStandardShader>();
	pStandardShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature.Get());
	std::shared_ptr<CMesh> pCubeMesh = std::make_shared<CCubeMesh>(pd3dDevice, pd3dCommandList, 1.0f, 1.0f, 1.0f);

	std::shared_ptr<CRotatingObject> pRotateGameObject;
	pRotateGameObject = CRotatingObject::Create(pd3dDevice, pd3dCommandList);
	pRotateGameObject->SetMesh(pCubeMesh);
	pRotateGameObject->AddMaterial(pMaterial);
	pMaterial->SetShader(pStandardShader);
	pRotateGameObject->SetPosition(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));
	pRotateGameObject->SetRotationSpeed(50.0f);
	pRotateGameObject->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	m_ppObjects.push_back(pRotateGameObject);	
}

void CLoadingScene::ReleaseObjects()
{
}

void CLoadingScene::ReleaseUploadBuffers()
{
}

void CLoadingScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	
}

bool CLoadingScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (false == CheckWorkRendering())
	{
		// Scene is not running or pausing
		return (false);
	}

	CScene::Render(pd3dCommandList, m_pCamera.get());

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//

CGameScene::CGameScene()
{
}

CGameScene::~CGameScene()
{
}

void CGameScene::InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	// Create Objects
	ResourceManager& resourceManager = CGameFramework::GetResourceManager();

	// Skybox
	m_pSkyBox = CSkyBox::Create(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get());

	// Terrain
	//지형을 확대할 스케일 벡터이다. x-축과 z-축은 8배, y-축은 2배 확대한다. 
	XMFLOAT3 xmf3Scale(8.0f, 1.0f, 8.0f);
	XMFLOAT4 xmf4Color(0.0f, 0.2f, 0.3f, 0.0f);
	m_pTerrain = CHeightMapTerrain::Create(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get(), _T("Terrain/terrain.raw"), 257, 257, 257, 257, xmf3Scale, xmf4Color);
	//m_pTerrain = CHeightMapTerrain::Create(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature.Get(), _T("Terrain/terrain.raw"), 257, 257, 13, 13, xmf3Scale, xmf4Color);

	// Cube
	std::shared_ptr<CGameObject> pGameObject;
	pGameObject = CCubeObject::Create(pd3dDevice, pd3dCommandList, pd3dRootSignature);
	pGameObject->SetPosition(DirectX::XMFLOAT3(0.0f, 0.0f, 10.0f));
	m_ppObjects.push_back(pGameObject);

	// Zombie Object
	std::shared_ptr<CLoadedModelInfo> pModel = resourceManager.GetSkinInfo(L"Model/FuzZombie.bin");
	if (!pModel)
	{
		pModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, pd3dRootSignature, "Model/FuzZombie.bin", nullptr);
		resourceManager.SetSkinInfo(L"Model/FuzZombie.bin", pModel);
	}

	std::shared_ptr<CZombieObject> pZombie = CZombieObject::Create(pd3dDevice, pd3dCommandList, pd3dRootSignature, m_pTerrain, pModel, 2);
	pZombie->SetPosition(DirectX::XMFLOAT3(100.0f, 50.0f, 100.0f));
	m_ppHierarchicalObjects.push_back(pZombie);

	// Default Camera 위치 수정
	m_pCamera->SetPosition(Vector3::Add(pZombie->GetPosition(), XMFLOAT3(0.0f, 0.0f, -10.0f)));
	m_pCamera->RegenerateViewMatrix();
}

void CGameScene::ReleaseObjects()
{
}

void CGameScene::ReleaseUploadBuffers()
{
}

void CGameScene::FixedUpdate(float deltaTime)
{
	CScene::FixedUpdate(deltaTime);

	// Update Camera Position
	if (m_pCamera)
	{
		// Camera Follow Zombie
		if (m_ppHierarchicalObjects.size() > 0)
		{
			//XMFLOAT3 xmf3CameraPosition = Vector3::Add(m_ppHierarchicalObjects[0]->GetPosition(), XMFLOAT3(0.0f, 0.0f, -10.0f));
			XMFLOAT3 xmf3CameraPosition = Vector3::Add(m_ppHierarchicalObjects[0]->GetPosition(), Vector3::ScalarProduct(m_pCamera->GetLook(), -10.0f));
			m_pCamera->SetPosition(xmf3CameraPosition);
			m_pCamera->RegenerateViewMatrix();
		}
	}
}

bool CGameScene::ProcessInput(const INPUT_PARAMETER& pBuffer, float deltaTime)
{
	// 키보드 입력의 정보 압축
	DWORD dwDirection = 0;
	if (pBuffer.pKeysBuffer[VK_UP] & 0xF0)dwDirection |= DIR_FORWARD;
	if (pBuffer.pKeysBuffer[VK_DOWN] & 0xF0)dwDirection |= DIR_BACKWARD;
	if (pBuffer.pKeysBuffer[VK_LEFT] & 0xF0)dwDirection |= DIR_LEFT;
	if (pBuffer.pKeysBuffer[VK_RIGHT] & 0xF0)dwDirection |= DIR_RIGHT;
	if (pBuffer.pKeysBuffer[VK_PRIOR] & 0xF0)dwDirection |= DIR_UP;
	if (pBuffer.pKeysBuffer[VK_NEXT] & 0xF0)dwDirection |= DIR_DOWN;

	if (dwDirection || pBuffer.cxDelta != 0.0f || pBuffer.cyDelta != 0.0f)
	{
		if (m_ppHierarchicalObjects.size() > 0)
		{
			m_ppHierarchicalObjects[0]->Move(dwDirection, 10.0f, deltaTime);
		}

		if (m_pCamera)
		{
			m_pCamera->Rotate(pBuffer.cyDelta, pBuffer.cxDelta, 0.0f);
			m_pCamera->RegenerateViewMatrix();
		}
	}

	return true;
}

void CGameScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
}

void CGameScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)	{
	case WM_KEYDOWN: {
		switch (wParam)
		{
			//case VK_LEFT:
			//{
			//	//m_pCamera->Move(-1.0f,0.0f,0.0f);
			//	m_pCamera->Rotate(0.0f, -10.0f, 0.0f);
			//	m_pCamera->RegenerateViewMatrix();
			//	break;
			//}
			//case VK_RIGHT:
			//{
			//	//m_pCamera->Move(1.0f, 0.0f, 0.0f);
			//	m_pCamera->Rotate(0.0f, 10.0f, 0.0f);
			//	m_pCamera->RegenerateViewMatrix();
			//	break;
			//}
			//case VK_UP:
			//{
			//	//m_pCamera->Move(0.0f, 0.0f, 1.0f);
			//	m_pCamera->Rotate(-10.0f, 0.0f, 0.0f);
			//	m_pCamera->RegenerateViewMatrix();
			//	break;
			//}
			//case VK_DOWN:
			//{
			//	//m_pCamera->Move(0.0f, 0.0f, -1.0f);
			//	m_pCamera->Rotate(10.0f, 0.0f, 0.0f);
			//	m_pCamera->RegenerateViewMatrix();
			//	break;
			//}
			/*
			case VK_SPACE:
			{
				m_pCamera->Move(0.0f, 10.0f, 0.0f);
				m_pCamera->RegenerateViewMatrix();
				break;
			}
			case VK_SHIFT:
			{
				m_pCamera->Move(0.0f, -10.0f, 0.0f);
				m_pCamera->RegenerateViewMatrix();
				break;
			}
			*/
			/*
			case 'W': case 'w':
			{
				m_ppHierarchicalObjects[0]->Move(0.0f, 0.0f, 1.0f);
				break;
			}
			case 'S': case 's':
			{
				m_ppHierarchicalObjects[0]->Move(0.0f, 0.0f, -1.0f);
				break;
			}
			case 'A': case 'a':
			{
				m_ppHierarchicalObjects[0]->Move(-1.0f, 0.0f, 0.0f);
				break;
			}
			case 'D': case 'd':
			{
				m_ppHierarchicalObjects[0]->Move(1.0f, 0.0f, 0.0f);
				break;
			}
			}
			break;
			}
			*/
		default: 
			break;
		}
	}
	}
}

