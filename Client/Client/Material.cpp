///////////////////////////////////////////////////////////////////////////////
// Date: 2025-03-28
// Material.h : CMaterial 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#include "Material.h"
#include "Scene.h"
#include "Shader.h"
#include "GameObject.h"

///////////////////////////////////////////////////////////////////////////////
// static variables
std::shared_ptr<CShader> CMaterial::m_pStandardShader;
std::shared_ptr<CShader> CMaterial::m_pSkinnedAnimationShader;

///////////////////////////////////////////////////////////////////////////////
// 

CMaterial::CMaterial(int nTextures)
{
	m_ppTextures.resize(nTextures);
	m_strTextureNames.resize(nTextures);
}

CMaterial::~CMaterial()
{
	m_ppTextures.clear();
	m_strTextureNames.clear();
	m_nTextures = 0;
	m_nType = 0x00;
	m_pShader = nullptr;
	m_pd3dcbMaterial.Reset();
	m_pcbMappedMaterial = nullptr;
}

// Shader Variables
std::shared_ptr<CTexture> CMaterial::GetTexture(int nIndex) { return m_ppTextures[nIndex]; }
void CMaterial::SetTexture(std::shared_ptr<CTexture> pTexture) { m_ppTextures.clear(); m_ppTextures.push_back(pTexture); }
void CMaterial::SetTexture(std::shared_ptr<CTexture> pTexture, int nIndex) { m_ppTextures[nIndex] = pTexture; }
void CMaterial::AddTexture(std::shared_ptr<CTexture> pTexture) { m_ppTextures.push_back(pTexture); }

void CMaterial::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
#ifdef _USE_OBJECT_MATERIAL_CBV
	// Create Constant Buffer
	UINT ncbElementBytes = ((sizeof(CB_MATERIAL_INFO) + 255) & ~255); //256의 배수
	m_pd3dcbMaterial = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_GENERIC_READ, NULL);

	// Map Constant Buffer
	m_pd3dcbMaterial->Map(0, nullptr, (void**)&m_pcbMappedMaterial);
	ZeroMemory(m_pcbMappedMaterial, sizeof(CB_MATERIAL_INFO));
#endif // _USE_OBJECT_MATERIAL_CBV
}

void CMaterial::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
#ifdef _USE_OBJECT_MATERIAL_CBV
	m_pcbMappedMaterial->m_xmf4Ambient = m_xmf4Ambient;
	m_pcbMappedMaterial->m_xmf4Diffuse = m_xmf4Diffuse;
	m_pcbMappedMaterial->m_xmf4Specular = m_xmf4Specular;
	m_pcbMappedMaterial->m_xmf4Emissive = m_xmf4Emissive;

	m_pcbMappedMaterial->m_nTexturesMask = 0x00;

	if (m_pTexture)
	{
		//pcbMappedObjectInfo->m_nType |= m_pTexture->GetTextureType();
		//m_pTexture->UpdateShaderVariables(pd3dCommandList);
	}
	else {
		m_pcbMappedMaterial->m_nTexturesMask = 0x00;
	}

	D3D12_GPU_VIRTUAL_ADDRESS GPUAddress = m_pd3dcbMaterial->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(ROOT_PARAMETER_MATERIAL, GPUAddress);
#else
	pd3dCommandList->SetGraphicsRoot32BitConstants(ROOT_PARAMETER_MATERIAL, 4, &m_xmf4Ambient, 0);
	pd3dCommandList->SetGraphicsRoot32BitConstants(ROOT_PARAMETER_MATERIAL, 4, &m_xmf4Albedo, 4);
	pd3dCommandList->SetGraphicsRoot32BitConstants(ROOT_PARAMETER_MATERIAL, 4, &m_xmf4Specular, 8);
	pd3dCommandList->SetGraphicsRoot32BitConstants(ROOT_PARAMETER_MATERIAL, 4, &m_xmf4Emissive, 12);

	pd3dCommandList->SetGraphicsRoot32BitConstants(ROOT_PARAMETER_MATERIAL, 1, &m_nType, 16);

	for (auto& pTexture : m_ppTextures) {
		if (pTexture) pTexture->UpdateShaderVariables(pd3dCommandList);
	}
#endif // _USE_OBJECT_MATERIAL_CBV
}

void CMaterial::ReleaseShaderVariables()
{
#ifdef _USE_OBJECT_MATERIAL_CBV
	if (m_pd3dcbMaterial) m_pd3dcbMaterial->Unmap(0, nullptr);
	m_pd3dcbMaterial.Reset();
#endif // _USE_OBJECT_MATERIAL_CBV
}

void LoadTextureFromFile(std::shared_ptr<CTexture>& ppTexture, ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::wstring& pwstrTexturePath, char  pstrTextureName[64], UINT nRootParameter)
{
	ppTexture = std::make_shared <CTexture>(1, RESOURCE_TEXTURE2D, 1);
	
	(ppTexture)->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, pwstrTexturePath, RESOURCE_TEXTURE2D, 0);
	ppTexture->SetName(pstrTextureName);
	
	CScene::CreateShaderResourceViews(pd3dDevice, ppTexture.get(), 0, nRootParameter);
}

void CMaterial::LoadTextureFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, UINT nType, UINT nRootParameter, std::wstring& pwstrTextureName, std::shared_ptr<CTexture>& ppTexture, std::shared_ptr<CGameObject> pParent, std::ifstream& File, std::shared_ptr<CShader> pShader)
{
	char pstrTextureName[64] = { '\0' };

	BYTE nStrLength = 64;
	//UINT nReads;
	File.read((char*)&nStrLength, sizeof(BYTE));
	File.read((char*)&pstrTextureName, sizeof(char) * nStrLength);
	pstrTextureName[nStrLength] = '\0';

	bool bDuplicated = false;
	if (strcmp(pstrTextureName, "null"))
	{
		SetMaterialType(nType);

		char pstrFilePath[64] = { '\0' };
		strcpy_s(pstrFilePath, 64, "Model/Textures/");

		bDuplicated = (pstrTextureName[0] == '@');
		strcpy_s(pstrFilePath + 15, 64 - 15, (bDuplicated) ? (pstrTextureName + 1) : pstrTextureName);
		strcpy_s(pstrFilePath + 15 + ((bDuplicated) ? (nStrLength - 1) : nStrLength), 64 - 15 - ((bDuplicated) ? (nStrLength - 1) : nStrLength), ".dds");

		size_t nConverted = 0;

		wchar_t pwstrName[64] = { '\0' };
		mbstowcs_s(&nConverted, pwstrName, 64, pstrFilePath, _TRUNCATE);
		pwstrTextureName = pwstrName;
		//#define _WITH_DISPLAY_TEXTURE_NAME

#ifdef _WITH_DISPLAY_TEXTURE_NAME
		static int nTextures = 0, nRepeatedTextures = 0;
		TCHAR pstrDebug[256] = { 0 };
		_stprintf_s(pstrDebug, 256, _T("Texture Name: %d %c %s\n"), (pstrTextureName[0] == '@') ? nRepeatedTextures++ : nTextures++, (pstrTextureName[0] == '@') ? '@' : ' ', pwstrTextureName);
		OutputDebugString(pstrDebug);
#endif
		if (!bDuplicated)
		{
			ppTexture = CScene::GetTexture(pstrTextureName);
			if (nullptr == ppTexture)
			{
				::LoadTextureFromFile(ppTexture, pd3dDevice, pd3dCommandList, pwstrTextureName, pstrTextureName, nRootParameter);
				CScene::StoreTexture(pstrTextureName, ppTexture);
			}
		}
		else
		{
			if (pParent)
			{
				while (pParent)
				{
					std::shared_ptr<CGameObject> pGrandParent = pParent->GetParent();
					if (!pGrandParent) break;
					pParent = pGrandParent;
				}
				std::shared_ptr<CGameObject> pRootGameObject = pParent;
				ppTexture = pRootGameObject->FindReplicatedTexture(pwstrTextureName.data());
			}
		}
	}
}

void CMaterial::SetStandardShader() { CMaterial::SetShader(m_pStandardShader); }
void CMaterial::SetSkinnedAnimationShader() { CMaterial::SetShader(m_pSkinnedAnimationShader); }
