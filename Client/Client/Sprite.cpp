///////////////////////////////////////////////////////////////////////////////
// Sprite.cpp : Sprite 클래스의 구현 파일
///////////////////////////////////////////////////////////////////////////////

#include "Sprite.h"

CSprite::CSprite() : CGameObject()
{
	SetLayer(LAYER_UI);

	auto pUIShader = CResourceManager::Instance().GetShader<CTextureToViewportShader>();
	std::shared_ptr<CMaterial> pMaterial = std::make_shared<CMaterial>(1);
	pMaterial->SetShader(pUIShader);
	m_ppMaterials.push_back(pMaterial);
}

void CSprite::Initialize(std::wstring wstrFilepath)
{
	TextureRecipe titleTextureRecipe;
	titleTextureRecipe.filePath = wstrFilepath;
	titleTextureRecipe.type = RESOURCE_TEXTURE2D;
	titleTextureRecipe.source = TEXTURE_SOURCE_FILE;
	titleTextureRecipe.name = L" Sprite Image";
	titleTextureRecipe.rootparameterindex = ROOT_PARAMETER_ALBEDO_TEXTURE;

	m_ppMaterials[0]->SetTexture(std::make_shared<CTexture>(titleTextureRecipe));
	CResourceManager::Instance().RegisterMaterialUpload(m_ppMaterials[0].get());
}


// 2D Sprite
void CSprite::SetSize(float cx, float cy, float width, float height) {
	m_fLeft = cx - width / 2;
	m_fBottom = cy - height / 2;
	m_fWidth = width;
	m_fHeight = height;

	if (m_pTransform) {
		m_pTransform->SetPosition(cx, cy, 0.0f);
		m_pTransform->SetScale(width, height, 1.0f);
	}
}

// SetSizeLT: Set size using left-top corner coordinates
void CSprite::SetSizeLT(float left, float top, float width, float height) {
	m_fLeft = left;
	m_fBottom = top - height;
	m_fWidth = width;
	m_fHeight = height;

	if (m_pTransform) {
		m_pTransform->SetPosition(left + width / 2, m_fBottom + height / 2, 0.0f);
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

bool CSprite::IsClicked(float x, float y) const
{
	return (x >= m_fLeft && x <= m_fLeft + m_fWidth && y >= m_fBottom && y <= m_fBottom + m_fHeight);
}
