///////////////////////////////////////////////////////////////////////////////
// Sprite.cpp : Sprite 클래스의 구현 파일
///////////////////////////////////////////////////////////////////////////////

#include "Sprite.h"

CSprite::CSprite() : CGameObject()
{
	SetLayer(LAYER_UI);
}

// 2D Sprite
void CSprite::SetSize(float cx, float cy, float width, float height) {
	m_fLeft = cx - width / 2;
	m_fTop = cy - height / 2;
	m_fRight = cx + width / 2;
	m_fBottom = cy + height / 2;

	if (m_pTransform) {
		m_pTransform->SetPosition(cx, cy, 0.0f);
		m_pTransform->SetScale(width, height, 1.0f);
	}
}

void CSprite::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite) {

	// Set Shader Variables
	UpdateShaderVariables(pd3dCommandList); // GameObject Matrix Update

	// Texture Set
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
