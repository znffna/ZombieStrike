#include "ResourceManager.h"

void ResourceManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ID3D12RootSignature* rootsignature) {
	m_d3dDevice = device;

	m_d3dGraphicsCommandList = commandList;
	m_d3dGraphicRootSignature = rootsignature;

	LoadModelList();
}

// 모든 리소스 해제
void ResourceManager::ReleaseResources() {
	ModelInfos.clear();
}

////////////////////////////////////////////
// 텍스쳐 정보를 저장
void ResourceManager::SetTexture(const std::string& name, std::shared_ptr<CTexture> texture) {
	if (texture == nullptr) return;
	TextureInfos[name] = texture;
}

std::shared_ptr<CTexture> ResourceManager::GetTexture(const std::string& name) {
	if (TextureInfos.find(name) != TextureInfos.end()) {
		// 이미 로드된 모델이 있는 경우
		return TextureInfos[name];
	}
	return nullptr;
}

void ResourceManager::LoadModelList(std::string filepath) {
	std::ifstream file(filepath);
	std::string modelname;
	while (file >> modelname) {
		GetModelInfo(modelname);
	}
}

void ResourceManager::SetSkinInfo(const std::string& name, std::shared_ptr<CLoadedModelInfo> modelInfo) {
	ModelInfos[name] = modelInfo;
}

inline std::shared_ptr<CLoadedModelInfo> ResourceManager::GetModelInfo(const std::string& name) {
	if (ModelInfos.find(name) != ModelInfos.end()) {
		// 이미 로드된 모델이 있는 경우
		if (ModelInfos[name]) return ModelInfos[name];
		else return nullptr;
	}

	ModelInfos[name] = nullptr;

	// 없는경우 바로 불러와서 저장하고 return 한다.
	std::string filepath = "Model/" + name + ".bin";
	/*OutputDebugStringA(filepath.c_str());
	OutputDebugStringA("\n");*/

	auto pModelInfo = CGameObject::LoadGeometryAndAnimationFromFile(m_d3dDevice, m_d3dGraphicsCommandList, m_d3dGraphicRootSignature, filepath.c_str(), nullptr);
	if (pModelInfo) {
		SetSkinInfo(name, pModelInfo);
		return pModelInfo;
	}
	else {
		// 로드 실패
		/*	std::string strDebug = "Failed to load model: " + name;
		OutputDebugStringA(strDebug.c_str());
		*/
	}
	return nullptr;
}

// 메쉬 정보를 저장
void ResourceManager::SetMesh(const std::string& name, std::shared_ptr<CMesh> pMesh) {
	MeshInfos[name] = pMesh;
}

std::shared_ptr<CMesh> ResourceManager::GetMesh(const std::string& name) {
	if (MeshInfos.find(name) != MeshInfos.end()) {
		return MeshInfos[name];
	}
	return nullptr;
}
