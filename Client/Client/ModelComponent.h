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
	CModelComponent(CGameObject* pObject, CLoadedModelInfo* pModel) : CComponent(pObject), m_pModel(pModel){};
	virtual ~CModelComponent() {};

	virtual std::shared_ptr<CComponent> Clone(CGameObject* newOwner) const override
	{
		return std::make_shared<CModelComponent>(newOwner, m_pModel);
	};

	virtual void Update(float fTimeElapsed) override {};
	void SetModel(CLoadedModelInfo* pModel) { m_pModel = pModel; }
	CLoadedModelInfo* GetModel() const { return m_pModel; }

private:
	CLoadedModelInfo* m_pModel;
};

