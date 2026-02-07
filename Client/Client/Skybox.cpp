#include "Skybox.h"

///////////////////////////////////////////////////////////////////////////////
//

CSkyBox::CSkyBox()
{
}

CSkyBox::~CSkyBox()
{
}

void CSkyBox::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	std::shared_ptr<CMesh> pSkyBoxMesh = std::make_shared<CSkyBoxMesh>(pd3dDevice, pd3dCommandList, 20.0f, 20.0f, 20.0f);
	SetMesh(pSkyBoxMesh);

	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	TextureRecipe skyboxTextureRecipe;
	skyboxTextureRecipe.source = TEXTURE_SOURCE_FILE;
	skyboxTextureRecipe.name = L"SkyBoxTexture";
	skyboxTextureRecipe.filePath = L"SkyBox/SkyBox.dds";
	skyboxTextureRecipe.type = RESOURCE_TEXTURE_CUBE;
	skyboxTextureRecipe.rootparameterindex = ROOT_PARAMETER_ALBEDO_TEXTURE;

	std::shared_ptr<CTexture> pSkyBoxTexture = std::make_shared<CTexture>(skyboxTextureRecipe);
	//std::shared_ptr<CTexture> pSkyBoxTexture = std::make_shared<CTexture>(1, RESOURCE_TEXTURE_CUBE, 1);
	//pSkyBoxTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"SkyBox/SkyBox.dds", RESOURCE_TEXTURE_CUBE, 0);

	std::shared_ptr<CShader> pSkyBoxShader = std::make_shared<CSkyBoxShader>();
	pSkyBoxShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);
	// pSkyBoxShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	// pSkyBoxShader->CreateCbvSrvDescriptorHeaps(pd3dDevice, 0, 1);

	/*CResourceManager::Instance().CreateShaderResourceViews(pd3dDevice, pSkyBoxTexture.get(), 0, ROOT_PARAMETER_SKYBOX);
	{
		std::string strDebugName = "After CScene::CreateShaderResourceViews\n";
		OutputDebugStringA(strDebugName.c_str());
	}*/

	std::shared_ptr<CMaterial> pSkyBoxMaterial = std::make_shared<CMaterial>();
	pSkyBoxMaterial->SetTexture(pSkyBoxTexture);
	pSkyBoxMaterial->SetShader(pSkyBoxShader);

	//SetMaterialSize(1);
	//SetMaterial(0, pSkyBoxMaterial);
	AddMaterial(pSkyBoxMaterial);
}

void CSkyBox::Initialize()
{
	// Set Mesh
	std::shared_ptr<CMesh> pSkyBoxMesh = std::make_shared<CSkyBoxMesh>(20.0f, 20.0f, 20.0f);
	SetMesh(pSkyBoxMesh);

	// Set Material
	auto pSkyBoxMaterial = std::make_shared<CMaterial>();
	pSkyBoxMaterial->SetName("SkyBox Material");

	// Set Texture
	TextureRecipe skyboxTextureRecipe;
	skyboxTextureRecipe.source = TEXTURE_SOURCE_FILE;
	skyboxTextureRecipe.name = L"SkyBoxTexture";
	skyboxTextureRecipe.filePath = L"SkyBox/SkyBox.dds";
	skyboxTextureRecipe.type = RESOURCE_TEXTURE_CUBE;
	skyboxTextureRecipe.rootparameterindex = ROOT_PARAMETER_SKYBOX;

	auto pSkyBoxTexture = std::make_shared<CTexture>(skyboxTextureRecipe);
	pSkyBoxMaterial->SetTexture(pSkyBoxTexture);

	// Set Shader
	auto pSkyBoxShader = CResourceManager::Instance().GetOrCreate<CSkyBoxShader>();
	pSkyBoxMaterial->SetShader(pSkyBoxShader);

	AddMaterial(pSkyBoxMaterial);
}

void CSkyBox::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite)
{
	if (pCamera)
	{
		XMFLOAT3 xmf3CameraPos = pCamera->GetPosition();
		SetPosition(xmf3CameraPos.x, xmf3CameraPos.y, xmf3CameraPos.z);
	}
	UpdateTransform();

	CGameObject::Render(pd3dCommandList, pCamera, bDepthWrite);
}
