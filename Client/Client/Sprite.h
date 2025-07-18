#pragma once

#include "GameObject.h"

class CSprite : public CGameObject
{
	float m_fLeft = 0.0f;
	float m_fTop = 0.0f;
	float m_fRight = 0.0f;
	float m_fBottom = 0.0f;

public:
	// 2D Sprite
	virtual void SetSize(float px, float py, float width, float height) override;

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite = false) override;

	bool IsClicked(float x, float y) const
	{
		return (x >= m_fLeft && x <= m_fRight && y >= m_fTop && y <= m_fBottom);
	}
};

