#include "GaugeBar.h"

CGaugeBar::CGaugeBar()
{
}

CGaugeBar::~CGaugeBar()
{
}


void CGaugeBar::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite)
{
	CSprite::Render(pd3dCommandList, pCamera, bDepthWrite);

	// Ratio만큼 줄어든 CSprite Render
	{
		XMFLOAT4X4 xmf4x4World;
		xmf4x4World = m_pGaugeTransform.GetWorldMatrix();
		XMStoreFloat4x4(&xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&xmf4x4World)));
		pd3dCommandList->SetGraphicsRoot32BitConstants(ROOT_PARAMETER_OBJECT, 16, &xmf4x4World, 0);
		pd3dCommandList->SetGraphicsRoot32BitConstants(ROOT_PARAMETER_OBJECT, 4, &m_xmf4GaugeColor, 16);
	}

	for (int i = 0; i < m_ppMaterials.size(); ++i)
	{
		std::shared_ptr<CMaterial>& pMaterial = m_ppMaterials[i];
		if (pMaterial)
		{
			// Set Pipeline State
			if (pMaterial->m_pShader) {
				pMaterial->m_pShader->OnPrepareRender(pd3dCommandList, 0, bDepthWrite); // Render(pd3dCommandList, pCamera);
			}
			// Material Update
			if (!bDepthWrite) pMaterial->UpdateShaderVariables(pd3dCommandList);
		}

		// Render Mesh
		pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pd3dCommandList->DrawInstanced(6, 1, 0, 0);
	}
}

void CGaugeBar::SetGauge(float ratio, float left, float bottom, float width, float height)
{
	CSprite::SetSize(left + width / 2, bottom + height / 2, width, height);
	SetGauge(ratio);
}

void CGaugeBar::SetGauge(float ratio)
{
	m_fRatio = ratio;

	m_pGaugeTransform.SetPosition(m_fLeft + m_fWidth * m_fRatio / 2, m_fBottom + m_fHeight / 2, 0.0f);
	m_pGaugeTransform.SetScale(m_fWidth * ratio, m_fHeight, 1.0f);
}