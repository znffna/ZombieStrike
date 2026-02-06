#pragma once
#include "GameObject.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CSkyBox : public CGameObject
{
public:
	CSkyBox();
	virtual ~CSkyBox();

	virtual GAMEOBJECT_LAYER GetLayer() override { return GAMEOBJECT_LAYER::LAYER_SKYBOX; }

	// Object Initialization
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
	virtual void Initialize();
	virtual std::string GetDefaultName() override { return "CSkyBox"; }

	// Object Render
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite = false) override;
}; // CSkyBox

