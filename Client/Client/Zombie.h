#pragma once

#include "GameObject.h"
#include "AnimationController.h"

class CZombieCAnimationController : public CAnimationController
{
public:
	CZombieCAnimationController(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, int nAnimationTracks, std::shared_ptr<CLoadedModelInfo> pModel);
	~CZombieCAnimationController();
};

class CZombieObject : public CGameObject
{
public:
	CZombieObject();
	virtual ~CZombieObject();

	// Object Initialization
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CLoadedModelInfo> pModel, int nAnimationTracks);
	virtual std::string GetDefaultName() override { return "CZombieObject"; }

	static std::shared_ptr<CZombieObject> Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CGameObject> pTerrain, std::shared_ptr<CLoadedModelInfo> pModel, int nAnimationTracks);
};