#include "ResourceManager.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

void CResourceManager::Initialize(ID3D12RootSignature* rootsignature) {
	m_d3dGraphicRootSignature = rootsignature;

	LoadModelList();
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

void CResourceManager::SetSkinInfo(const std::string& name, std::shared_ptr<CLoadedModelInfo> modelInfo) {
	ModelInfos[name] = modelInfo;
}

inline CLoadedModelInfo* CResourceManager::GetModelInfo(const std::string& name) {
	if (ModelInfos.find(name) != ModelInfos.end()) {
		// 이미 로드된 모델이 있는 경우
		if (ModelInfos[name]) return ModelInfos[name].get();
		else return nullptr;
	}

	ModelInfos[name] = nullptr;

	// 없는경우 바로 불러와서 저장하고 return 한다.
	std::string filepath = "Model/" + name + ".bin";
	/*OutputDebugStringA(filepath.c_str());
	OutputDebugStringA("\n");*/

	auto& uploadContext = CUploadContext::Instance();

	auto pModelInfo = CGameObject::LoadGeometryAndAnimationFromFile(uploadContext.m_pd3dDevice, uploadContext.m_pd3dGraphicCommandList, m_d3dGraphicRootSignature, filepath.c_str(), nullptr);
	if (pModelInfo) {
		SetSkinInfo(name, pModelInfo);
		return pModelInfo.get();
	}
	else {
		// 로드 실패
		/*	std::string strDebug = "Failed to load model: " + name;
		OutputDebugStringA(strDebug.c_str());
		*/
	}

	uploadContext.ExecuteUploadCommandList();
	return nullptr;
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

