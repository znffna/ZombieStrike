#include "BulletObject.h"

///////////////////////////////////////////////////////////////////////////////
//

CBulletParticleObject::CBulletParticleObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, XMFLOAT3 xmf3Position, XMFLOAT3 xmf3Look, float fLifetime, XMFLOAT3 xmf3Acceleration, XMFLOAT3 xmf3Color, XMFLOAT2 xmf2Size, UINT nMaxParticles)
{
	//std::shared_ptr<CBulletMesh> pMesh = std::make_shared<CBulletMesh>(pd3dDevice, pd3dCommandList, xmf3Position, xmf3Look, fLifetime, xmf3Acceleration, xmf3Color, xmf2Size, nMaxParticles);
	//SetMesh(pMesh);

	//std::shared_ptr<CMaterial> pMaterial = std::make_shared<CMaterial>();
	//AddMaterial(pMaterial);

	//TextureRecipe particleTextureRecipe;
	//particleTextureRecipe.source = TEXTURE_SOURCE_FILE;
	//particleTextureRecipe.name = L"CBulletParticleObject";
	//particleTextureRecipe.filePath = L"Image/BulletTrail.dds";
	//particleTextureRecipe.type = RESOURCE_TEXTURE2D;
	//particleTextureRecipe.rootparameterindex = ROOT_PARAMETER_ALBEDO_TEXTURE;

	//pMaterial->AddTexture(std::make_shared<CTexture>(particleTextureRecipe));

	//particleTextureRecipe.filePath = L"Image/Spark.dds";
	//particleTextureRecipe.rootparameterindex++;
	//pMaterial->AddTexture(std::make_shared<CTexture>(particleTextureRecipe));

	//particleTextureRecipe.filePath = L"Image/Blood.dds";
	//particleTextureRecipe.rootparameterindex++;
	//pMaterial->AddTexture(std::make_shared<CTexture>(particleTextureRecipe));

	//particleTextureRecipe.filePath = L"Image/StoneFragment.dds";
	//particleTextureRecipe.rootparameterindex++;
	//pMaterial->AddTexture(std::make_shared<CTexture>(particleTextureRecipe));


	///*
	//std::shared_ptr<CTexture> pParticleTexture = std::make_shared<CTexture>(4, RESOURCE_TEXTURE2D, 4);
	//pParticleTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/BulletTrail.dds", RESOURCE_TEXTURE2D, 0);
	//pParticleTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Spark.dds", RESOURCE_TEXTURE2D, 1);
	//pParticleTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Blood.dds", RESOURCE_TEXTURE2D, 2);
	//pParticleTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/StoneFragment.dds", RESOURCE_TEXTURE2D, 3);
	//pParticleTexture->SetName("CBulletParticleObject");
	//CResourceManager::Instance().CreateShaderResourceViews(pd3dDevice, pParticleTexture.get(), 0, ROOT_PARAMETER_ALBEDO_TEXTURE);
	//*/
	//
	////pMaterial->SetTexture(pParticleTexture);

	//srand((unsigned)time(NULL));

	//XMFLOAT4* pxmf4RandomValues = new XMFLOAT4[1024];
	//for (int i = 0; i < 1024; i++) { pxmf4RandomValues[i].x = float((rand() % 10000) - 5000) / 5000.0f; pxmf4RandomValues[i].y = float((rand() % 10000) - 5000) / 5000.0f; pxmf4RandomValues[i].z = float((rand() % 10000) - 5000) / 5000.0f; pxmf4RandomValues[i].w = float((rand() % 10000) - 5000) / 5000.0f; }

	////	m_pRandowmValueTexture = new CTexture(1, RESOURCE_TEXTURE1D, 0, 1);

	//TextureRecipe RandomValueTextureRecipe;
	//RandomValueTextureRecipe.source = TEXTURE_SOURCE_BUFFER;
	//RandomValueTextureRecipe.name = L"RandomValueTexture";
	//RandomValueTextureRecipe.pData = pxmf4RandomValues;
	//RandomValueTextureRecipe.width = 1024;
	//RandomValueTextureRecipe.nElements = 1024;
	//RandomValueTextureRecipe.nStride = sizeof(XMFLOAT4);
	//RandomValueTextureRecipe.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	//RandomValueTextureRecipe.heapType = D3D12_HEAP_TYPE_DEFAULT;
	//RandomValueTextureRecipe.initialState = D3D12_RESOURCE_STATE_GENERIC_READ;

	//m_pRandowmValueTexture = std::make_shared<CTexture>(1, RESOURCE_BUFFER, 1);
	//m_pRandowmValueTexture->CreateBuffer(pd3dDevice, pd3dCommandList, pxmf4RandomValues, 1024, sizeof(XMFLOAT4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_GENERIC_READ, 0);

	//m_pRandowmValueOnSphereTexture = std::make_shared<CTexture>(1, RESOURCE_BUFFER, 1);
	//m_pRandowmValueOnSphereTexture->CreateBuffer(pd3dDevice, pd3dCommandList, pxmf4RandomValues, 256, sizeof(XMFLOAT4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_GENERIC_READ, 0);

	//CGameObject::CreateShaderVariables(pd3dDevice, pd3dCommandList);

	//CResourceManager::Instance().CreateShaderResourceViews(pd3dDevice, m_pRandowmValueTexture.get(), 0, ROOT_PARAMETER_RANDOMBUFFER);
	//CResourceManager::Instance().CreateShaderResourceViews(pd3dDevice, m_pRandowmValueOnSphereTexture.get(), 0, ROOT_PARAMETER_RANDOM_SPHERE_BUFFER);

	//std::shared_ptr<CBulletShader> pShader = std::make_shared<CBulletShader>();
	//pShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);
	//pShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	//pMaterial->SetShader(pShader);
	//SetMaterial(0, pMaterial);
}

CBulletParticleObject::~CBulletParticleObject()
{

}

void CBulletParticleObject::Initialize(UINT nMaxParticles)
{
	// Mesh 생성 및 설정
	auto pMesh = std::make_shared<CBulletMesh>();
	pMesh->Initialize(nMaxParticles);
	SetMesh(pMesh);

	// Material 생성 및 설정
	SetMaterialSize(1);
	auto pMaterial = std::make_shared<CMaterial>();

	TextureRecipe particleTextureRecipe;
	particleTextureRecipe.source = TEXTURE_SOURCE_FILE;
	particleTextureRecipe.name = L"CBulletParticleObject";
	particleTextureRecipe.filePath = L"Image/BulletTrail.dds";
	particleTextureRecipe.type = RESOURCE_TEXTURE2D;
	particleTextureRecipe.rootparameterindex = ROOT_PARAMETER_ALBEDO_TEXTURE;

	pMaterial->AddTexture(std::make_shared<CTexture>(particleTextureRecipe));

	particleTextureRecipe.filePath = L"Image/Spark.dds";
	particleTextureRecipe.rootparameterindex++;
	pMaterial->AddTexture(std::make_shared<CTexture>(particleTextureRecipe));

	particleTextureRecipe.filePath = L"Image/Blood.dds";
	particleTextureRecipe.rootparameterindex++;
	pMaterial->AddTexture(std::make_shared<CTexture>(particleTextureRecipe));

	particleTextureRecipe.filePath = L"Image/StoneFragment.dds";
	particleTextureRecipe.rootparameterindex++;
	pMaterial->AddTexture(std::make_shared<CTexture>(particleTextureRecipe));

	// Shader
	auto pShader = CResourceManager::Instance().GetOrCreate<CBulletShader>();
	pMaterial->SetShader(pShader);
	SetMaterial(0, pMaterial);
}

void CBulletParticleObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite)
{
	if (!IsGPUInitialized()) return;

	OnPrepareRender();

	for (auto& pMaterial : m_ppMaterials)
	{
		if (pMaterial->m_pShader) pMaterial->m_pShader->OnPrepareRender(pd3dCommandList, 0, false);
		for (auto& pTexture : pMaterial->m_ppTextures)
		{
			if (pTexture) pTexture->UpdateShaderVariables(pd3dCommandList);
		}
	}
	if (m_pRandowmValueTexture) m_pRandowmValueTexture->UpdateShaderVariables(pd3dCommandList);
	if (m_pRandowmValueOnSphereTexture) m_pRandowmValueOnSphereTexture->UpdateShaderVariables(pd3dCommandList);


	UpdateShaderVariables(pd3dCommandList);

	m_pMesh->PreRender(pd3dCommandList, 0); //Stream Output
	m_pMesh->Render(pd3dCommandList, 0); //Stream Output
	m_pMesh->PostRender(pd3dCommandList, 0); //Stream Output

	for (auto& pMaterial : m_ppMaterials)
	{
		if (pMaterial->m_pShader) pMaterial->m_pShader->OnPrepareRender(pd3dCommandList, 1, false);
	}

	m_pMesh->PreRender(pd3dCommandList, 1); //Draw
	m_pMesh->Render(pd3dCommandList, 1); //Draw
}

void CBulletParticleObject::OnPostRender()
{
	m_pMesh->OnPostRender(0); //Read Stream Output Buffer Filled Size
}

void CBulletParticleObject::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CGameObject::CreateShaderVariables(pd3dDevice, pd3dCommandList);

	XMFLOAT4* pxmf4RandomValues = new XMFLOAT4[1024];
	for (int i = 0; i < 1024; i++) { pxmf4RandomValues[i].x = float((rand() % 10000) - 5000) / 5000.0f; pxmf4RandomValues[i].y = float((rand() % 10000) - 5000) / 5000.0f; pxmf4RandomValues[i].z = float((rand() % 10000) - 5000) / 5000.0f; pxmf4RandomValues[i].w = float((rand() % 10000) - 5000) / 5000.0f; }

	m_pRandowmValueTexture = std::make_shared<CTexture>(1, RESOURCE_BUFFER, 1);
	m_pRandowmValueTexture->CreateBuffer(pd3dDevice, pd3dCommandList, pxmf4RandomValues, 1024, sizeof(XMFLOAT4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_GENERIC_READ, 0);

	m_pRandowmValueOnSphereTexture = std::make_shared<CTexture>(1, RESOURCE_BUFFER, 1);
	m_pRandowmValueOnSphereTexture->CreateBuffer(pd3dDevice, pd3dCommandList, pxmf4RandomValues, 256, sizeof(XMFLOAT4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_GENERIC_READ, 0);

	CResourceManager::Instance().CreateShaderResourceViews(pd3dDevice, m_pRandowmValueTexture.get(), 0, ROOT_PARAMETER_RANDOMBUFFER);
	CResourceManager::Instance().CreateShaderResourceViews(pd3dDevice, m_pRandowmValueOnSphereTexture.get(), 0, ROOT_PARAMETER_RANDOM_SPHERE_BUFFER);
}

void CBulletParticleObject::AddBullet(const XMFLOAT3& pOrigin, const XMFLOAT3& xmf3Look, float fRange)
{
	CBulletVertex pBulletVertex;
	pBulletVertex.m_xmf3Position = pOrigin;
	pBulletVertex.m_xmf3Destination = Vector3::Add(pOrigin, Vector3::ScalarProduct(xmf3Look, fRange));
	pBulletVertex.m_xmf3Velocity = xmf3Look;
	// 총알 궤적 출력 시간 설정
	pBulletVertex.m_fLifetime = 0.5f;
	//pBulletVertex.m_fLifetime = fRange / Vector3::Length(xmf3Look);

	AddBullet(pBulletVertex);
}

void CBulletParticleObject::AddBullet(const CBulletVertex& pBulletVertex)
{
	if(auto pMesh = std::dynamic_pointer_cast<CBulletMesh>(m_pMesh)) pMesh->AddBullet(pBulletVertex);
}
