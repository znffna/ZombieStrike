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
std::shared_ptr<CShader> CMaterial::m_pColliderShader;

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

	// Material이 가지고 있는 텍스쳐 로드
	std::string strTextureDirectory = { "Model/Textures/" }; // 텍스쳐 폴더 경로
	int pstrDirectoryPath = strTextureDirectory.size();

	for (int idx = 0; idx < m_ppTextures.size(); ++idx)
	{
		if (m_strTextureNames[idx].empty()) continue;

		if(auto pTexture = CResourceManager::Instance().GetTexture(m_strTextureNames[idx]))
		{
			m_ppTextures[idx] = pTexture;
			continue;
		}

		std::string strFilePath = strTextureDirectory;
		strFilePath += m_strTextureNames[idx] + ".dds";

		::LoadTextureFromFile(m_ppTextures[idx], pd3dDevice, pd3dCommandList, to_wstring(strFilePath), m_strTextureNames[idx], ROOT_PARAMETER_ALBEDO_TEXTURE + idx);
		CResourceManager::Instance().SetTexture(m_strTextureNames[idx], m_ppTextures[idx]);
	}
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

void CMaterial::ReleaseUploadBuffers()
{
	if (false == m_ppTextures.empty()) { 
		for (auto& pTexture : m_ppTextures) {
			if (pTexture) pTexture->ReleaseUploadBuffers();
		}
	}

}

inline uint32_t MaterialFlagToIndex(uint32_t flag)
{
	return std::countr_zero(flag);
}

void LoadTextureFromFile(std::shared_ptr<CTexture>& ppTexture, ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::wstring& pwstrTexturePath, std::string& strTextureName, UINT nRootParameter)
{
	ppTexture = std::make_shared <CTexture>(1, RESOURCE_TEXTURE2D, 1);
	
	(ppTexture)->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, pwstrTexturePath, RESOURCE_TEXTURE2D, 0);
	
	CScene::CreateShaderResourceViews(pd3dDevice, ppTexture.get(), 0, nRootParameter);
}

void CMaterial::LoadTextureFromFile(UINT nType, std::ifstream& File)
{
	// Load Texture Name
	std::string strTextureName;
	ReadStringFromFile(File, strTextureName);

	// 실제 텍스쳐가 있을때만 추가.
	if (strTextureName != "null")
	{
		SetMaterialType(nType);

		bool bDuplicated = (strTextureName[0] == '@');
		strTextureName = (bDuplicated) ? (strTextureName.substr(1)) : strTextureName;

		int nTextureIndex = MaterialFlagToIndex(nType);
		m_strTextureNames[nTextureIndex] = strTextureName;
	}
}

void CMaterial::LoadTextureFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, UINT nType, UINT nRootParameter, std::wstring& pwstrTextureName, std::shared_ptr<CTexture>& ppTexture, CGameObject* pParent, std::ifstream& File, std::shared_ptr<CShader> pShader)
{
	//char pstrTextureName[64] = { '\0' };

	//BYTE nStrLength = 64;
	//UINT nReads;

	std::string strTextureName;

	ReadStringFromFile(File, strTextureName);
	//File.read((char*)&nStrLength, sizeof(BYTE));
	//File.read((char*)&pstrTextureName, sizeof(char) * nStrLength);
	//pstrTextureName[nStrLength] = '\0';

	bool bDuplicated = false;
	if (strTextureName != "null")
	//if (strcmp(pstrTextureName, "null"))
	{
		SetMaterialType(nType);

		std::string strTextureDirectory = { "Model/Textures/" }; // 텍스쳐 폴더 경로
		int pstrDirectoryPath = strTextureDirectory.size();

		std::string strFilePath = strTextureDirectory;

		// 복제여부 확인 및 복제 시 @ 제거
		bDuplicated = (strTextureName[0] == '@');
		strTextureName = (bDuplicated) ? (strTextureName.substr(1)) : strTextureName;

		strFilePath += strTextureName + ".dds";

		pwstrTextureName = to_wstring(strFilePath);

		#define _WITH_DISPLAY_TEXTURE_NAME

#ifdef _WITH_DISPLAY_TEXTURE_NAME
		static int nTextures = 0, nRepeatedTextures = 0;
		TCHAR pstrDebug[256] = { 0 };
		_stprintf_s(pstrDebug, 256, _T("Texture Name: %d %c %s\n"), (bDuplicated) ? nRepeatedTextures++ : nTextures++, (bDuplicated) ? '@' : ' ', pwstrTextureName.data());
		OutputDebugString(pstrDebug);
#endif
		if (!bDuplicated)
		{
			ppTexture = CResourceManager::Instance().GetTexture(strTextureName);
			if (nullptr == ppTexture)
			{
				::LoadTextureFromFile(ppTexture, pd3dDevice, pd3dCommandList, pwstrTextureName, strTextureName, nRootParameter);
				CResourceManager::Instance().SetTexture(strTextureName, ppTexture);
			}
		}
		else
		{
			if (pParent)
			{
				while (pParent)
				{
					auto pGrandParent = pParent->GetParent();
					if (!pGrandParent) break;
					pParent = pGrandParent;
				}
				auto pRootGameObject = pParent;
				ppTexture = pRootGameObject->FindReplicatedTexture(pwstrTextureName);
			}
		}
	}
}

void CMaterial::SetStandardShader() { CMaterial::SetShader(m_pStandardShader); }
void CMaterial::SetSkinnedAnimationShader() { CMaterial::SetShader(m_pSkinnedAnimationShader); }
