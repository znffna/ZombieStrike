#pragma once

#include "GameObject.h"

class CLoadedModelInfo;
class CGameObject;
class CTexture;
class CShader;

class CResource {
public:
	bool isUsed{ false };
	int index{ -1 };
	ComPtr<ID3D12Resource> resource;

	CResource& Use() {
		isUsed = true;
		return *this;
	}

	// 리소스 해제
	void Release() {
		if (resource) {
			resource->Unmap(0, NULL);
			isUsed = false;
		}
	}
};

class CResourceManager
{
private:
	CResourceManager() {}
	~CResourceManager() {}

public:
	static CResourceManager& GetInstance() {
		static CResourceManager instance;
		return instance;
	}

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ID3D12RootSignature* rootsignature);

	// 모든 리소스 해제
	void ReleaseResources();

	////////////////////////////////////////////
	// 텍스쳐 정보를 저장
	void SetTexture(const std::string& name, std::shared_ptr<CTexture> texture);
	std::shared_ptr<CTexture> GetTexture(const std::string& name);

	////////////////////////////////////////////
	// 모델 정보를 저장
	void LoadModelList(std::string filepath = "Model/ModelList.txt");

	void SetSkinInfo(const std::string& name, std::shared_ptr<CLoadedModelInfo> modelInfo);
	std::shared_ptr<CLoadedModelInfo> GetModelInfo(const std::string& name);

	// 메쉬 정보를 저장
	void SetMesh(const std::string& name, std::shared_ptr<CMesh> pMesh);
	std::shared_ptr<CMesh> GetMesh(const std::string& name);

private:
	// CGameFramework에서 상속받는다.
	ID3D12Device* m_d3dDevice = nullptr;
	// Resource Manager 전용 CommandList가 필요하다.
	ID3D12GraphicsCommandList* m_d3dGraphicsCommandList = nullptr;
	// CSCene에서 상속받는다. (또는 생성을 CGameFramework에서 하고 넘겨받는다.)
	ID3D12RootSignature* m_d3dGraphicRootSignature = nullptr;

	std::unordered_map<std::string, std::shared_ptr<CLoadedModelInfo>> ModelInfos;
	std::unordered_map<std::string, std::shared_ptr<CTexture>> TextureInfos;
	std::unordered_map<std::string, std::shared_ptr<CMesh>> MeshInfos;

	std::vector<CResource> m_ppd3dcbSkinningBoneTransforms;
public:
	void CreateSkinnedTransformBuffer();

	CResource& GetSkinningBoneTransforms();

};
