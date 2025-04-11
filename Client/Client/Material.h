///////////////////////////////////////////////////////////////////////////////
// Date: 2025-03-28
// Material.h : CMaterial 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once
#include "stdafx.h"
#include "Shader.h"

class CTexture;
class CGameObject;

struct CB_MATERIAL_INFO
{
	XMFLOAT4						m_xmf4Ambient;
	XMFLOAT4						m_xmf4Diffuse;
	XMFLOAT4						m_xmf4Specular;
	XMFLOAT4						m_xmf4Emissive;
	UINT							m_nTexturesMask;
};

#define MATERIAL_ALBEDO_MAP				0x01
#define MATERIAL_SPECULAR_MAP			0x02
#define MATERIAL_NORMAL_MAP				0x04
#define MATERIAL_METALLIC_MAP			0x08
#define MATERIAL_EMISSION_MAP			0x10
#define MATERIAL_DETAIL_ALBEDO_MAP		0x20
#define MATERIAL_DETAIL_NORMAL_MAP		0x40

class CMaterial
{
public:
	CMaterial(int nTextures = 0);
	virtual ~CMaterial();

	// CMaterial Name
	std::string GetName() { return m_strMaterialName; }
	void SetName(std::string strName) { m_strMaterialName = strName; }

	void SetMaterialType(UINT nType) { m_nType |= nType; }

	// CMaterial Color
	DirectX::XMFLOAT4 GetAmbient() { return m_xmf4Ambient; }
	void SetAmbient(DirectX::XMFLOAT4 xmf4Ambient) { m_xmf4Ambient = xmf4Ambient; }

	DirectX::XMFLOAT4 GetDiffuse() { return m_xmf4Albedo; }
	void SetAlbedo(DirectX::XMFLOAT4 xmf4Diffuse) { m_xmf4Albedo = xmf4Diffuse; }

	DirectX::XMFLOAT4 GetSpecular() { return m_xmf4Specular; }
	void SetSpecular(DirectX::XMFLOAT4 xmf4Specular) { m_xmf4Specular = xmf4Specular; }

	DirectX::XMFLOAT4 GetEmissive() { return m_xmf4Emissive; }
	void SetEmissive(DirectX::XMFLOAT4 xmf4Emissive) { m_xmf4Emissive = xmf4Emissive; }

	float GetGlossiness() { return m_fGlossiness; }
	void SetGlossiness(float fGlossiness) { m_fGlossiness = fGlossiness; }

	float GetSmoothness() { return m_fSmoothness; }
	void SetSmoothness(float fSmoothness) { m_fSmoothness = fSmoothness; }

	float GetSpecularHighlight() { return m_fSpecularHighlight; }
	void SetSpecularHighlight(float fSpecularHighlight) { m_fSpecularHighlight = fSpecularHighlight; }

	float GetMetallic() { return m_fMetallic; }
	void SetMetallic(float fMetallic) { m_fMetallic = fMetallic; }

	float GetGlossyReflection() { return m_fGlossyReflection; }
	void SetGlossyReflection(float fGlossyReflection) { m_fGlossyReflection = fGlossyReflection; }

	// Texture
	//std::shared_ptr<CTexture> GetTexture() { return m_pTexture; }
	//void SetTexture(std::shared_ptr<CTexture> pTexture) { m_pTexture = pTexture; }

	std::shared_ptr<CTexture> GetTexture(int nIndex = 0);
	void SetTexture(std::shared_ptr<CTexture> pTexture);
	void SetTexture(std::shared_ptr<CTexture> pTexture, int nIndex);
	void AddTexture(std::shared_ptr<CTexture> pTexture);

	// Shader
	void SetShader(std::shared_ptr<CShader> pShader) { m_pShader = pShader; }

	// Shader Variables
	void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseShaderVariables();

	void LoadTextureFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, UINT nType, UINT nRootParameter, std::wstring& pwstrTextureName, std::shared_ptr<CTexture>& ppTexture, std::shared_ptr<CGameObject> pParent, std::ifstream& File, std::shared_ptr<CShader> pShader);

private:
	std::string m_strMaterialName; // CMaterial Name

	DirectX::XMFLOAT4 m_xmf4Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f); // Ambient Color
	DirectX::XMFLOAT4 m_xmf4Albedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // Diffuse Color
	DirectX::XMFLOAT4 m_xmf4Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f); // Specular Color
	DirectX::XMFLOAT4 m_xmf4Emissive = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f); // Emissive Color

private:
	float							m_fGlossiness = 0.0f;
	float							m_fSmoothness = 0.0f;
	float							m_fSpecularHighlight = 0.0f;
	float							m_fMetallic = 0.0f;
	float							m_fGlossyReflection = 0.0f;

public:
	UINT m_nType = 0x00; // Texture Mask

	// Shader Variables
	ComPtr<ID3D12Resource> m_pd3dcbMaterial;
	CB_MATERIAL_INFO* m_pcbMappedMaterial = nullptr;
public:
	UINT m_nTextures = 0;
	std::vector<std::wstring> m_strTextureNames; // Texture Name
	std::vector<std::shared_ptr<CTexture>> m_ppTextures; // Texture
	std::shared_ptr<CShader> m_pShader; // Shader

public:
	static std::shared_ptr<CShader> m_pStandardShader;
	static std::shared_ptr<CShader> m_pSkinnedAnimationShader;
	static std::shared_ptr<CShader> m_pColliderShader;	

	void SetStandardShader();
	void SetSkinnedAnimationShader();
};

void LoadTextureFromFile(std::shared_ptr<CTexture>& ppTexture, ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::wstring& pwstrTextureName, char  pstrTextureName[64], UINT nRootParameter);
