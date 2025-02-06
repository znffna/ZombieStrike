///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-28
// Scene.cpp : Scene 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#include "Scene.h"

CScene::CScene()
{
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

void CScene::InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dRootSignature)
{
	// Create Default Lights and Materials
	BuildDefaultLightsAndMaterials();
	// Create Shader Variables
	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	// Create Root Signature
	if (!pd3dRootSignature) m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);
	else m_pd3dGraphicsRootSignature = pd3dRootSignature;

	// Create Objects
	std::shared_ptr<CGameObject> pGameObject;

	std::shared_ptr<CStandardShader> pStandardShader = std::make_shared<CStandardShader>();
	std::shared_ptr<CCubeMesh> pCubeMesh = std::make_shared<CCubeMesh>(pd3dDevice, pd3dCommandList, 1.0f, 1.0f, 1.0f);

	pGameObject = std::make_shared<CRotatingObject>();
	pGameObject->SetMesh(pCubeMesh);
	pGameObject->SetShader(pStandardShader);
	pGameObject->SetPosition(DirectX::XMFLOAT3(0.0f, 0.0f, 10.0f));

	m_ppObjects.push_back(pGameObject);

	m_pCamera = std::make_shared<CCamera>();
	m_pCamera->SetViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	m_pCamera->SetScissorRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	m_pCamera->GenerateProjectionMatrix(((float)WINDOW_WIDTH / (float)WINDOW_HEIGHT), 60.0f, 1.0f, 1000.0f);

}

void CScene::ReleaseObjects()
{
	// Release GameObjects
	m_ppObjects.clear();

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

	// Update GameObjects
	for (auto& pObject : m_ppObjects)
	{
		pObject->Update(deltaTime);
	}

	// Collision Check and Resolve

	// Update Matrix
	for (auto& pObject : m_ppObjects)
	{
		pObject->UpdateWorldMatrix(nullptr);
	}
}

void CScene::PrepareRender(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (false == CheckWorkRendering())
	{
		// Scene is not running or pausing
		return;
	}
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature.Get());
}

bool CScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (false == CheckWorkRendering())
	{
		// Scene is not running or pausing
		return (false);
	}

	// Set Root Signature
	//pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature.Get());

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
	}

	// Update Shader Variables
	UpdateShaderVariables(pd3dCommandList);

	// Render GameObjects [Through Batch Shader]
	for (auto& pObject : m_ppObjects)
	{
		pObject->Render(pd3dCommandList);
	}

	return (true);
}

ComPtr<ID3D12RootSignature> CScene::CreateGraphicsRootSignature(ID3D12Device* pd3dDevice)
{
	ComPtr<ID3D12RootSignature> pd3dRootSignature = nullptr;

	// Descriptor Table
	std::vector<D3D12_DESCRIPTOR_RANGE> d3dDescriptorRanges;

	d3dDescriptorRanges.reserve(2);

	D3D12_DESCRIPTOR_RANGE d3dDescriptorRange;
	// Standard Texture
	{
		d3dDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		d3dDescriptorRange.NumDescriptors = 7;
		d3dDescriptorRange.BaseShaderRegister = 6; //t6 ~ t12: gtxtStandardTextures[7]
		d3dDescriptorRange.RegisterSpace = 0;
		d3dDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		d3dDescriptorRanges.push_back(d3dDescriptorRange);
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
	std::vector<D3D12_ROOT_PARAMETER> pd3dRootParameters(7);

	// 0
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].Descriptor.ShaderRegister = ROOT_PARAMETER_OBJECT; // b0 : cbGameObjectInfo
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	// 1
	pd3dRootParameters[ROOT_PARAMETER_MATERIAL].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[ROOT_PARAMETER_MATERIAL].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_MATERIAL].Descriptor.ShaderRegister = ROOT_PARAMETER_MATERIAL; // b1 : cbGameObjectInfo
	pd3dRootParameters[ROOT_PARAMETER_MATERIAL].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	// 2
	pd3dRootParameters[ROOT_PARAMETER_CAMERA].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[ROOT_PARAMETER_CAMERA].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_CAMERA].Descriptor.ShaderRegister = ROOT_PARAMETER_CAMERA; // b2 : cbCamera
	pd3dRootParameters[ROOT_PARAMETER_CAMERA].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	// 3	
	pd3dRootParameters[ROOT_PARAMETER_FRAMEWORK].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[ROOT_PARAMETER_FRAMEWORK].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_FRAMEWORK].Descriptor.ShaderRegister = ROOT_PARAMETER_FRAMEWORK; // b3 : cbFramework
	pd3dRootParameters[ROOT_PARAMETER_FRAMEWORK].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	// 4
	pd3dRootParameters[ROOT_PARAMETER_LIGHT].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[ROOT_PARAMETER_LIGHT].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_LIGHT].Descriptor.ShaderRegister = ROOT_PARAMETER_LIGHT; // b4 : cbLight
	pd3dRootParameters[ROOT_PARAMETER_LIGHT].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	// 5
	pd3dRootParameters[ROOT_PARAMETER_TEXTURES].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_TEXTURES].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_TEXTURES].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[0];
	pd3dRootParameters[ROOT_PARAMETER_TEXTURES].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	// 6
	pd3dRootParameters[ROOT_PARAMETER_SKYBOX].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_SKYBOX].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_SKYBOX].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[1];
	pd3dRootParameters[ROOT_PARAMETER_SKYBOX].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

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
	d3dRootSignatureDesc.NumParameters = pd3dRootParameters.size();
	d3dRootSignatureDesc.pParameters = pd3dRootParameters.data();
	d3dRootSignatureDesc.NumStaticSamplers = pd3dStaticSamplerDescs.size();
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
	m_pcbMappedLights->m_nLights = m_pLights.size();

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
	// Create Default Lights and Materials
	BuildDefaultLightsAndMaterials();

	// Create Shader Variables
	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	// Create Root Signature
	if(!pd3dRootSignature) m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);
	else m_pd3dGraphicsRootSignature = pd3dRootSignature;

	// Create Objects

	std::shared_ptr<CMaterial> pMaterial = std::make_shared<CMaterial>();
	pMaterial->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	std::shared_ptr<CShader> pStandardShader = std::make_shared<CStandardShader>();
	pStandardShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature.Get());
	std::shared_ptr<CMesh> pCubeMesh = std::make_shared<CCubeMesh>(pd3dDevice, pd3dCommandList, 1.0f, 1.0f, 1.0f);

	std::shared_ptr<CRotatingObject> pGameObject;
	pGameObject = std::make_shared<CRotatingObject>();
	pGameObject->SetMesh(pCubeMesh);
	pGameObject->AddMaterial(pMaterial);
	pMaterial->SetShader(pStandardShader);
	pGameObject->SetPosition(DirectX::XMFLOAT3(0.0f, 0.0f, 10.0f));
	pGameObject->SetRotationSpeed(50.0f);
	pGameObject->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	m_ppObjects.push_back(pGameObject);

	m_pCamera = std::make_shared<CCamera>();
	m_pCamera->SetViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	m_pCamera->SetScissorRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	m_pCamera->GenerateViewMatrix(XMFLOAT3(0.0f, 0.0f, -5.0f),XMFLOAT3(0.0f, 0.0f, 1.0f),XMFLOAT3(0.0f,1.0f,0.0f));
	m_pCamera->GenerateProjectionMatrix(((float)WINDOW_WIDTH / (float)WINDOW_HEIGHT), 60.0f, 1.0f, 1000.0f);
	m_pCamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	// Scene 생성 완료
	m_SceneState = SCENE_STATE_RUNNING;
}

void CLoadingScene::ReleaseObjects()
{
}

void CLoadingScene::ReleaseUploadBuffers()
{
}


