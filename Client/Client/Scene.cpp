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
}

void CScene::InitializeObjects()
{
}

void CScene::InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
}

void CScene::ReleaseObjects()
{
}

void CScene::ReleaseUploadBuffers()
{
}

void CScene::FixedUpdate(float deltaTime)
{
	if (false == CheckWorkUpdating())
	{
		// Scene is not running
		return;
	}

	// Update GameObjects
}

bool CScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (false == CheckWorkRendering())
	{
		// Scene is not running or pausing
		return (false);
	}

	// Set Root Signature

	// Set Viewport and Scissor

	// Update Camera Variables

	// Update Shader Variables

	// Render GameObjects [Through Batch Shader]

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

	pd3dRootParameters[ROOT_PARAMETER_OBJECT].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].Descriptor.ShaderRegister = ROOT_PARAMETER_OBJECT; // b0 : cbGameObjectInfo
	pd3dRootParameters[ROOT_PARAMETER_OBJECT].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	pd3dRootParameters[ROOT_PARAMETER_MATERIAL].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[ROOT_PARAMETER_MATERIAL].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_MATERIAL].Descriptor.ShaderRegister = ROOT_PARAMETER_MATERIAL; // b1 : cbGameObjectInfo
	pd3dRootParameters[ROOT_PARAMETER_MATERIAL].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	pd3dRootParameters[ROOT_PARAMETER_CAMERA].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[ROOT_PARAMETER_CAMERA].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_CAMERA].Descriptor.ShaderRegister = ROOT_PARAMETER_CAMERA; // b2 : cbCamera
	pd3dRootParameters[ROOT_PARAMETER_CAMERA].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		
	pd3dRootParameters[ROOT_PARAMETER_FRAMEWORK].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[ROOT_PARAMETER_FRAMEWORK].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_FRAMEWORK].Descriptor.ShaderRegister = ROOT_PARAMETER_FRAMEWORK; // b3 : cbFramework
	pd3dRootParameters[ROOT_PARAMETER_FRAMEWORK].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[ROOT_PARAMETER_LIGHT].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[ROOT_PARAMETER_LIGHT].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[ROOT_PARAMETER_LIGHT].Descriptor.ShaderRegister = ROOT_PARAMETER_LIGHT; // b4 : cbLight
	pd3dRootParameters[ROOT_PARAMETER_LIGHT].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[ROOT_PARAMETER_TEXTURES].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[ROOT_PARAMETER_TEXTURES].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[ROOT_PARAMETER_TEXTURES].DescriptorTable.pDescriptorRanges = &d3dDescriptorRanges[0];
	pd3dRootParameters[ROOT_PARAMETER_TEXTURES].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

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

}

void CScene::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{

}

void CScene::ReleaseShaderVariables()
{

}

///////////////////////////////////////////////////////////////////////////////
// LoadingScene.cpp : LoadingScene 클래스의 구현 파일

LoadingScene::LoadingScene()
{
}

LoadingScene::~LoadingScene()
{
}

void LoadingScene::InitializeObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	// Create Shader Variables
	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	// Create Root Signature
	if(!m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);
	
	// Create Objects
	std::shared_ptr<CGameObject> pGameObject;

	std::shared_ptr<CStandardShader> pStandardShader = std::make_shared<CStandardShader>();
	std::shared_ptr<CCubeMesh> pCubeMesh = std::make_shared<CCubeMesh>(pd3dDevice, 1.0f, 1.0f, 1.0f);

	pGameObject = std::make_shared<CRotatingObject>();
	pGameObject->SetMesh(pCubeMesh);
	pGameObject->SetShader(pStandardShader);
	pGameObject->SetPosition(DirectX::XMFLOAT3(0.0f, 0.0f, 10.0f));


}

void LoadingScene::ReleaseObjects()
{
}

void LoadingScene::ReleaseUploadBuffers()
{
}


