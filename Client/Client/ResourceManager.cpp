#include "ResourceManager.h"


inline CResource& CResource::Use() {
	isUsed = true;
	return *this;
}

// 리소스 해제
void CResource::Release() {
	if (resource) {
		resource->Unmap(0, NULL);
		isUsed = false;
		CResourceManager::GetInstance().ReleaseSkinningBoneTransform(*this);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

void CResourceManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ID3D12RootSignature* rootsignature) {
	m_d3dDevice = device;

	m_d3dGraphicsCommandList = commandList;
	m_d3dGraphicRootSignature = rootsignature;

	CreateSkinnedTransformBuffer();
	LoadModelList();
}

// 모든 리소스 해제
void CResourceManager::ReleaseResources() {
	ModelInfos.clear();
	TextureInfos.clear();
	MeshInfos.clear();
	for (auto& resource : m_ppd3dcbSkinningBoneTransforms) {
		resource.Release();
		resource.resource.Reset();
	}
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

inline std::shared_ptr<CLoadedModelInfo> CResourceManager::GetModelInfo(const std::string& name) {
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
void CResourceManager::SetMesh(const std::string& name, std::shared_ptr<CMesh> pMesh) {
	MeshInfos[name] = pMesh;
}

std::shared_ptr<CMesh> CResourceManager::GetMesh(const std::string& name) {
	if (MeshInfos.find(name) != MeshInfos.end()) {
		return MeshInfos[name];
	}
	return nullptr;
}

void CResourceManager::CreateSkinnedTransformBuffer() {
	m_ppd3dcbSkinningBoneTransforms.resize(SKINNED_TRANSFORM_GPU_BUFFER);
	UINT ncbElementBytes = (((sizeof(XMFLOAT4X4) * SKINNED_ANIMATION_BONES) + 255) & ~255); //256의 배수

	for (int i = 0; i < SKINNED_TRANSFORM_GPU_BUFFER; i++)
	{
		m_ppd3dcbSkinningBoneTransforms[i].index = i;
		m_ppd3dcbSkinningBoneTransforms[i].isUsed = false;
		m_ppd3dcbSkinningBoneTransforms[i].resource = ::CreateBufferResource(m_d3dDevice, m_d3dGraphicsCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
	}
}

CResource& CResourceManager::GetSkinningBoneTransforms() {
	for (int i = 0; i < m_ppd3dcbSkinningBoneTransforms.size(); ++i) {
		if (false == m_ppd3dcbSkinningBoneTransforms[i].isUsed) {
			
			m_ppd3dcbSkinningBoneTransforms[i].isUsed = true;
			++m_nSkinningBoneTransformsCount;
			/*{
				std::string debugName = "GetSkinningBoneTransforms() - Skinning Bone Transforms [" + std::to_string(i) + "] is return, now SKinningBoneTransforms Uses : " + std::to_string(m_nSkinningBoneTransformsCount) + "\n";
				OutputDebugStringA(debugName.c_str());
			}*/
			return m_ppd3dcbSkinningBoneTransforms[i];
		}
	}

	// 이 아래 코드가 실행되면, 리소스가 부족한 경우이다.
	// 리소스가 부족한경우 아래코드는 Resource Container를 조작하기에 기존 &로 수행되는 코드가 터지게 된다.
	// 따라서, 아래 코드가 진행될 경우, SKINNED_TRANSFORM_GPU_BUFFER 을 높여서 다시 실행하자.
	{
		CResource resource;
		resource.isUsed = true;
		resource.index = m_ppd3dcbSkinningBoneTransforms.size();
		UINT ncbElementBytes = (((sizeof(XMFLOAT4X4) * SKINNED_ANIMATION_BONES) + 255) & ~255); //256의 배수
		resource.resource = ::CreateBufferResource(m_d3dDevice, m_d3dGraphicsCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
		m_ppd3dcbSkinningBoneTransforms.push_back(resource);
		return m_ppd3dcbSkinningBoneTransforms.back();
	}
}

void CResourceManager::ReleaseSkinningBoneTransform(const CResource& cResource) {
	// count를 위해 만든 Method.
	--m_nSkinningBoneTransformsCount;
	/*{
		std::string debugName = "ReleaseSkinningBoneTransform() - Skinning Bone Transforms [" + std::to_string(cResource.index) + "] is return, now SKinningBoneTransforms Uses : " + std::to_string(m_nSkinningBoneTransformsCount) + "\n";
		OutputDebugStringA(debugName.c_str());
	}*/
}


