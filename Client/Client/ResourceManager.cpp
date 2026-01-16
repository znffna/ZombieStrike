#include "ResourceManager.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

void CResourceManager::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList, ID3D12RootSignature* rootsignature) {
	m_d3dGraphicRootSignature = rootsignature;
	CreateDefaultMesh(pd3dDevice, pd3dCommnadList);
	LoadModelList(pd3dDevice, pd3dCommnadList);
}

void CResourceManager::CreateDefaultMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList)
{
	CreateDefualtQuad(pd3dDevice, pd3dCommnadList);
	CreateDefaultCube(pd3dDevice, pd3dCommnadList);
	CreateDefualtSphere(pd3dDevice, pd3dCommnadList);
}

void CResourceManager::CreateDefualtQuad(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList)
{
	// Quad Mesh 생성
	auto pMesh = std::make_shared<CQuadMesh>(pd3dDevice, pd3dCommnadList);
	pMesh->SetName("Quad");
	SetMesh("Quad", pMesh);
	CResourceManager::Instance().RegisterMeshUpload(pMesh.get());
}

void CResourceManager::CreateDefaultCube(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList)
{
	// Cube Mesh 생성
	auto pMesh = std::make_shared<CCubeMesh>(pd3dDevice, pd3dCommnadList);
	pMesh->SetName("Cube");
	SetMesh("Cube", pMesh);
	CResourceManager::Instance().RegisterMeshUpload(pMesh.get());
}

void CResourceManager::CreateDefualtSphere(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList)
{
	auto pMesh = std::make_shared<CSphereMesh>(pd3dDevice, pd3dCommnadList);
	pMesh->SetName("Sphere");
	SetMesh("Sphere", pMesh);
	CResourceManager::Instance().RegisterMeshUpload(pMesh.get());
}

// 모든 리소스 해제
void CResourceManager::ReleaseResources() {
	ModelInfos.clear();
	TextureInfos.clear();
	MeshInfos.clear();

	// Shader Resource Release
	ShaderInfos.clear();
}

// ----------------------------------------
// 텍스쳐 정보를 저장
// ----------------------------------------
void CResourceManager::SetTexture(const std::wstring& path, std::shared_ptr<CTexture> texture) {
	if (texture == nullptr) return;
	TextureInfos[path] = texture;
}

std::shared_ptr<CTexture> CResourceManager::LoadOrCreateTexture(const std::wstring& path) {
	if (TextureInfos.find(path) != TextureInfos.end()) {
		// 이미 로드된 모델이 있는 경우
		return TextureInfos[path];
	}
	return nullptr;
}

void CResourceManager::LoadModelList(std::string filepath) {
	std::ifstream file(filepath);
	std::string modelname;
	while (file >> modelname) {
		GetModelInfo(modelname);
	}
}

void CResourceManager::LoadModelList(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList, std::string filepath) {
	std::ifstream file(filepath);
	std::string modelname;
	while (file >> modelname) {
		GetModelInfo(pd3dDevice, pd3dCommnadList, modelname);
	}
}

void CResourceManager::SetSkinInfo(const std::string& name, std::shared_ptr<CLoadedModelInfo> modelInfo) {
	ModelInfos[name] = modelInfo;
}

CLoadedModelInfo* CResourceManager::GetModelInfo(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList, const std::string& name)
{
	if (ModelInfos.find(name) != ModelInfos.end()) {
		// 이미 로드된 모델이 있는 경우
		if (ModelInfos[name]) return ModelInfos[name].get();
		else return nullptr;
	}

	ModelInfos[name] = nullptr;

	// 없는경우 바로 불러와서 저장하고 return 한다.
	std::string filepath = "Model/" + name + ".bin";

	auto pModelInfo = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommnadList, m_d3dGraphicRootSignature, filepath.c_str(), nullptr);
	if (pModelInfo) {
		SetSkinInfo(name, pModelInfo);
		std::string strDebug = "Success to load model: " + name + "\n";
		OutputDebugStringA(strDebug.c_str());
		return pModelInfo.get();
	}
	else {
		// 로드 실패
		std::string strDebug = "Failed to load model: " + name + "\n";
		OutputDebugStringA(strDebug.c_str());
	}

	return nullptr;
}

inline CLoadedModelInfo* CResourceManager::GetModelInfo(const std::string& name)
{
	// 이 함수는 이미 Model이 Load된 상태에서만 호출되어야 한다.
	CLoadedModelInfo* ret = nullptr;
	auto uploadcontext = CUploadContext::Instance();
	ret = GetModelInfo(uploadcontext.m_pd3dDevice, uploadcontext.m_pd3dGraphicCommandList, name);
	return ret;
}

// ----------------------------------------
// 메쉬 정보를 저장
// ----------------------------------------
void CResourceManager::SetMesh(const std::string& name, std::shared_ptr<CMesh> pMesh) {
	MeshInfos[name] = pMesh;
}

std::shared_ptr<CMesh> CResourceManager::GetMesh(const std::string& name) {
	if (MeshInfos.find(name) != MeshInfos.end()) {
		return MeshInfos[name];
	}
	return nullptr;
}

// ----------------------------------------
// 셰이더 정보를 저장
// ----------------------------------------

// ----------------------------------------
// Mesh Upload 처리
// ----------------------------------------
void CResourceManager::RegisterMeshUpload(CMesh* pMesh)
{
	// 등록 갯수 증가
	m_nRegisterMeshCount.fetch_add(1);

	std::lock_guard<std::mutex> lock(m_UploadMutex);
	m_MeshRegisterBuffer.push_back(pMesh);
}

void CResourceManager::CollectMeshRegister(int maxcount)
{
	{
		int count{};
		while (false == m_MeshRegisterBuffer.empty() && count < maxcount)
		{
			m_MeshUploadList.push_back(m_MeshRegisterBuffer.front());
			m_MeshRegisterBuffer.pop_front();
			++count;
		}

		m_nUploadMeshCount.fetch_add(count);
	}
}

void CResourceManager::ProcessMeshUpload(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	for (auto pMesh : m_MeshUploadList)
	{
		{
			std::string debugname = "Processing Mesh Upload: " + pMesh->GetName() + "\n";
			OutputDebugStringA(debugname.c_str());
		}

		pMesh->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	}
}

void CResourceManager::ReleaseMeshUploadBuffers()
{
	// Release Mesh Upload Buffers
	for (auto& pMesh : m_MeshUploadList)
	{
		pMesh->ReleaseUploadBuffers();
	}
	m_MeshUploadList.clear();
}

// ----------------------------------------
// Material Upload 처리
// ----------------------------------------
void CResourceManager::RegisterMaterialUpload(CMaterial* pMaterial)
{
	// 등록 갯수 증가
	m_nRegisterMaterialCount.fetch_add(1);

	std::lock_guard<std::mutex> lock(m_UploadMutex);
	m_MaterialRegisterBuffer.push_back(pMaterial);
}

void CResourceManager::CollectMaterialRegister(int maxcount)
{
	{
		int count{};
		while (false == m_MaterialRegisterBuffer.empty() && count < maxcount)
		{
			m_MaterialUploadList.push_back(m_MaterialRegisterBuffer.front());
			m_MaterialRegisterBuffer.pop_front();
			++count;
		}
		m_nUploadMaterialCount.fetch_add(count);
	}
}

void CResourceManager::ProcessMaterialUpload(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	for (auto& pMaterial : m_MaterialUploadList)
	{
		/*{
			std::string debugname = "Processing Material Upload: " + pMaterial->GetName() + "\n";
			OutputDebugStringA(debugname.c_str());
		}*/
		pMaterial->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	}
}

void CResourceManager::ReleaseMaterialUploadBuffers()
{
	// Release Material Upload Buffers
	for (auto pMaterial : m_MaterialUploadList)
	{
		pMaterial->ReleaseUploadBuffers();
	}
	m_MaterialUploadList.clear();
}

// ----------------------------------------
// Texture Upload 처리
// ----------------------------------------
void CResourceManager::RegisterTextureUpload(CTexture* pTexture)
{
	// 등록 갯수 증가
	m_nRegisterTextureCount.fetch_add(1);

	std::lock_guard<std::mutex> lock(m_UploadMutex);
	m_TextureRegisterBuffer.push_back(pTexture);
}

void CResourceManager::CollectTextureRegister(int maxcount)
{
	{
		int count{};
		while (false == m_TextureRegisterBuffer.empty() && count < maxcount)
		{
			m_TextureUploadList.push_back(m_TextureRegisterBuffer.front());
			m_TextureRegisterBuffer.pop_front();
			++count;
		}
		m_nUploadTextureCount.fetch_add(count);
	}
}

void CResourceManager::ProcessTextureUpload(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	for (auto& pTexture : m_TextureUploadList)
	{
		/*{
			std::string debugname = "Processing Texture Upload: " + pTexture->GetName() + "\n";
			OutputDebugStringA(debugname.c_str());
		}*/
		pTexture->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	}
}

void CResourceManager::ReleaseTextureUploadBuffers()
{
	// Release Texture Upload Buffers
	for (auto pTexture : m_TextureUploadList)
	{
		pTexture->ReleaseUploadBuffers();
	}
	m_TextureUploadList.clear();
}

// ----------------------------------------
// Shader Upload 처리
// ----------------------------------------
void CResourceManager::RegisterShaderUpload(CShader* pShader)
{
	std::lock_guard<std::mutex> lock(m_UploadMutex);
	m_ShaderRegisterBuffer.push_back(pShader);
}

void CResourceManager::CollectShaderRegister(int maxcount)
{
	{
		int count{};
		while (false == m_ShaderRegisterBuffer.empty() && count < maxcount)
		{
			m_ShaderToCreateList.push_back(m_ShaderRegisterBuffer.front());
			m_ShaderRegisterBuffer.pop_front();
			++count;
		}
	}
}

void CResourceManager::ProcessShaderCreate(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	for (auto& pShader : m_ShaderToCreateList)
	{
		{
			std::string debugname = "Processing Shader Upload: " + to_string(pShader->GetShaderName()) + "\n";
			OutputDebugStringA(debugname.c_str());
		}
		pShader->CreateShader(pd3dDevice, m_d3dGraphicRootSignature);
	}

	m_ShaderToCreateList.clear();
}

