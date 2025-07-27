#pragma once

#include "Sprite.h"

class CGaugeBar : public CSprite
{
	float m_fRatio = 1.0f;

	CTransform m_pGaugeTransform{this};
	XMFLOAT4 m_xmf4GaugeColor = XMFLOAT4(0.7f, 0.0f, 0.0f, 1.0f); // Default color is green
public:
	CGaugeBar();
	virtual ~CGaugeBar();

	// Object Render
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr, bool bDepthWrite = false) override;

	virtual void SetGauge(float ratio, float left, float bottom, float width, float height);
	virtual void SetGauge(float ratio);
};

