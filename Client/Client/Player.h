#pragma once
#include "GameObject.h"

class CPlayer : public CGameObject
{
public:
	CPlayer();
	virtual ~CPlayer();
	// Object Initialization
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CLoadedModelInfo> pModel, int nAnimationTracks);
	
	virtual std::string GetDefaultName() override { return "CPlayer"; }
	static std::shared_ptr<CPlayer> Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CGameObject> pTerrain, std::shared_ptr<CLoadedModelInfo> pModel, int nAnimationTracks);
	// Object Update
	virtual void Update(float fTimeElapsed) override;

	virtual void Rotate(float x = 0.0f, float y = 0.0f, float z = 0.0f) override;

private:
	std::vector<std::string> m_ModelName{ "Ch18_nonPBR", "Ch35_nonPBR" };
};

