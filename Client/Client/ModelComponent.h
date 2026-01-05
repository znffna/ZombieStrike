#pragma once

#include "Component.h"

#include "AnimationSet.h"
#include "Mesh.h"

class CGameObject;
class CTexture;
class CShader;
class CCamera;

class CLoadedModelInfo
{
public:
	CLoadedModelInfo() {};
	~CLoadedModelInfo() {};

	std::string m_strFileName{};

	std::shared_ptr<CGameObject> m_pModelRootObject;

	int m_nSkinnedMeshes = 0;
	std::vector <std::shared_ptr<CSkinnedMesh>> m_ppSkinnedMeshes; //[SkinnedMeshes], Skinned Mesh Cache

	std::shared_ptr<CAnimationSets> m_pAnimationSets;

	BoundingBox m_MeshBoundingBox;
public:
	void PrepareSkinning();
};

class CModelComponent : public CComponent
{
public:
	CModelComponent(CGameObject* pObject) : CComponent(pObject) {};
	CModelComponent(const CModelComponent& pModel) : CComponent(nullptr), m_pModel(pModel.m_pModel) {};
	virtual ~CModelComponent() {};

	virtual std::unique_ptr<CComponent> Clone(CGameObject* newOwner) const { auto ret = std::make_unique<CModelComponent>(*this); ret->SetOwnerInternal(newOwner); return (ret); };

	virtual void Update(float fTimeElapsed) override {};
	void SetModel(CLoadedModelInfo* pModel);
	CLoadedModelInfo* GetModel() const { return m_pModel; }

private:
	CLoadedModelInfo* m_pModel;
};

