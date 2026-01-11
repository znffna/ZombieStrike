#include "ResourceManager.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

void CResourceManager::Initialize(ID3D12RootSignature* rootsignature) {
	m_d3dGraphicRootSignature = rootsignature;
	LoadModelList();
}

void CResourceManager::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList, ID3D12RootSignature* rootsignature) {
	m_d3dGraphicRootSignature = rootsignature;
	LoadModelList(pd3dDevice, pd3dCommnadList);
}

// 모든 리소스 해제
void CResourceManager::ReleaseResources() {
	ModelInfos.clear();
	TextureInfos.clear();
	MeshInfos.clear();
}

////////////////////////////////////////////
// 텍스쳐 정보를 저장
void CResourceManager::SetTexture(const std::string& name, std::shared_ptr<CTexture> texture) {
	if (texture == nullptr) return;
	TextureInfos[name] = texture;
}

std::shared_ptr<CTexture> CResourceManager::GetTexture(const std::string& name) {
	if (TextureInfos.find(name) != TextureInfos.end()) {
		// 이미 로드된 모델이 있는 경우
		return TextureInfos[name];
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

// 메쉬 정보를 저장
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
// Mesh Upload 처리
// ----------------------------------------
void CResourceManager::RegisterMeshUpload(CMesh* pMesh)
{
	std::lock_guard<std::mutex> lock(m_UploadMutex);
	m_MeshRegisterBuffer.push_back(pMesh);
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
	std::lock_guard<std::mutex> lock(m_UploadMutex);
	m_MaterialRegisterBuffer.push_back(pMaterial);
}

void CResourceManager::ProcessMaterialUpload(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	for (auto& pMaterial : m_MaterialUploadList)
	{
		{
			std::string debugname = "Processing Material Upload: " + pMaterial->GetName() + "\n";
			OutputDebugStringA(debugname.c_str());
		}
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

