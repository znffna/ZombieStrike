#pragma once
#include "GameObject.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CMapObject : public CGameObject
{
public:
	CMapObject() :CGameObject() {};

	virtual GAMEOBJECT_LAYER GetLayer() override { return GAMEOBJECT_LAYER::LAYER_ENVIRONMENT; }

	void Initialize(std::wstring wstrMapFilePath);
	void LoadGeometryAndAnimationFromFile(std::wstring wstrMapFilePath);

	void CollectMeshBound();
};
