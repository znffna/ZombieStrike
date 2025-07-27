#include "Sprite.h"

// 2D Sprite
void CSprite::SetSize(float px, float py, float width, float height) {
	m_fLeft = px - width / 2;
	m_fTop = py - height / 2;
	m_fRight = px + width / 2;
	m_fBottom = py + height / 2;

	if (m_pTransform) {
		m_pTransform->SetPosition(px, py, 0.0f);
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
