#pragma once

#include "GameObject.h"

class CSprite : public CGameObject
{
protected:
	float m_fLeft = 0.0f;
	float m_fBottom = 0.0f;
	float m_fWidth = 0.0f;
	float m_fHeight = 0.0f;

public:
	// 2D Sprite
	CSprite();

	void Initialize(std::wstring wstrFilepath);

	virtual void SetSize(float cx, float cy, float width, float height) override;
	virtual void SetSizeLT(float left, float top, float width, float height);

	CMaterial* GetMaterial() { return m_ppMaterials[0].get(); }

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite = false) override;

	bool IsClicked(float x, float y) const;
};

