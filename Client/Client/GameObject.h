///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// GameObject.h : CGameObject 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"
#include "Mesh.h"

#include "Component.h"
#include "Transform.h"

class CGameObject;
class CTexture;
class CShader;
class CCamera;

////////////////////////////////////////////////////////////////////////////////////////
//

enum RESOURCE_TYPE
{
	RESOURCE_TEXTURE1D = 0x01,
	RESOURCE_TEXTURE2D = 0x02,
	RESOURCE_TEXTURE2D_ARRAY = 0x03, //[]
	RESOURCE_TEXTURE2DARRAY = 0x04,
	RESOURCE_TEXTURE_CUBE = 0x05,
	RESOURCE_BUFFER = 0x06,
	RESOURCE_STRUCTURED_BUFFER = 0x07
};

class CTexture
{
public:
	CTexture() {};
	CTexture(int nTextures, UINT nTextureType, int nRootParameters);;
	virtual ~CTexture()
	{
		m_pd3dTextures.clear();
		m_pd3dTextureUploadBuffers.clear();
		for (auto& name : m_strTextureNames)
		{
			std::wstring debugoutput = L"Texture Name: " + name + L" has destroyed\n";
			OutputDebugString(debugoutput.c_str());
		}
		m_strTextureNames.clear();
		m_nResourceTypes.clear();
		m_pdxgiBufferFormats.clear();
		m_nBufferElements.clear();
		m_nBufferStrides.clear();
		m_nRootParameterIndices.clear();
		m_d3dSrvGpuDescriptorHandles.clear();

	};

	// Texture Name
	std::string GetName() { return m_strName; }
	void SetName(std::string strName) { m_strName = strName; }

	// Texture Type
	UINT GetTextureType() { return m_nTextureType; }
	void SetTextureType(UINT nTextureType) { m_nTextureType = nTextureType; }

	// Texture
	ComPtr<ID3D12Resource> GetTexture(int nIndex = 0) { return m_pd3dTextures[nIndex]; }
	void SetTexture(ComPtr<ID3D12Resource> pd3dTexture, int nIndex = 0) { m_pd3dTextures[nIndex] = pd3dTexture; }

	// Getter / Setter
	void SetRootParameterIndex(int nIndex, UINT nRootParameterIndex) {m_nRootParameterIndices[nIndex] = nRootParameterIndex;}
	void SetGpuDescriptorHandle(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSrvGpuDescriptorHandle) {m_d3dSrvGpuDescriptorHandles[nIndex] = d3dSrvGpuDescriptorHandle;}

	int GetRootParameters() { return(m_nRootParameters); }
	int GetTextures() { return((int)m_pd3dTextures.size()); }
	std::wstring GetTextureName(int nIndex) { return(m_strTextureNames[nIndex]); }
	ID3D12Resource* GetResource(int nIndex) { return(m_pd3dTextures[nIndex].Get()); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(int nIndex) { return(m_d3dSrvGpuDescriptorHandles[nIndex]); }
	int GetRootParameter(int nIndex) { return(m_nRootParameterIndices[nIndex]); }

	UINT GetTextureType(int nIndex) { return(m_nResourceTypes[nIndex]); }
	DXGI_FORMAT GetBufferFormat(int nIndex) { return(m_pdxgiBufferFormats[nIndex]); }
	int GetBufferElements(int nIndex) { return(m_nBufferElements[nIndex]); }

	D3D12_SHADER_RESOURCE_VIEW_DESC GetShaderResourceViewDesc(int nIndex);;

	// Shader Variables
	void UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, int nParameterIndex, int nTextureIndex);;
	void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);;

	// Load Texture
	void LoadTextureFromDDSFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::wstring strTextureName, UINT nResourceType, UINT nIndex);;
	void LoadTextureFromWICFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::wstring strTextureName, UINT nResourceType, UINT nIndex);;
	void LoadBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nElements, UINT nStride, DXGI_FORMAT ndxgiFormat, UINT nIndex);;
	ComPtr<ID3D12Resource> CreateTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, UINT nIndex, UINT nResourceType, UINT nWidth, UINT nHeight, UINT nElements, UINT nMipLevels, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS d3dResourceFlags, D3D12_RESOURCE_STATES d3dResourceStates, D3D12_CLEAR_VALUE* pd3dClearValue);;

private:
	std::string m_strName; // Texture Name

	UINT m_nTextureType = 0x00; // Texture Type

	// Texture Variables
	UINT m_nTextures;
	std::vector<ComPtr<ID3D12Resource>> m_pd3dTextures;
	std::vector<ComPtr<ID3D12Resource>> m_pd3dTextureUploadBuffers;
	std::vector<std::wstring> m_strTextureNames;

	std::vector<UINT> m_nResourceTypes;

	std::vector<DXGI_FORMAT> m_pdxgiBufferFormats;
	std::vector<int> m_nBufferElements;
	std::vector<int> m_nBufferStrides;

	int m_nRootParameters = 0;
	std::vector<int> m_nRootParameterIndices;
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_d3dSrvGpuDescriptorHandles;

};

struct CB_GAMEOBJECT_INFO
{
	XMFLOAT4X4						m_xmf4x4World;
};

struct CB_MATERIAL_INFO
{
	XMFLOAT4						m_xmf4Ambient;
	XMFLOAT4						m_xmf4Diffuse;
	XMFLOAT4						m_xmf4Specular;
	XMFLOAT4						m_xmf4Emissive;
	UINT							m_nTexturesMask;
};

////////////////////////////////////////////////////////////////////////////////////////
//

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
	CMaterial(int nTextures = 0);;
	virtual ~CMaterial()
	{
		m_ppTextures.clear();
		m_strTextureNames.clear();
		m_nTextures = 0;
		m_nType = 0x00;
		m_pShader = nullptr;
		m_pd3dcbMaterial.Reset();
		m_pcbMappedMaterial = nullptr;
	};

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
	
	std::shared_ptr<CTexture> GetTexture(int nIndex = 0) { return m_ppTextures[nIndex]; }
	void SetTexture(std::shared_ptr<CTexture> pTexture) { m_ppTextures.clear(); m_ppTextures.push_back(pTexture); }
	void SetTexture(std::shared_ptr<CTexture> pTexture, int nIndex) { m_ppTextures[nIndex] = pTexture; }
	void AddTexture(std::shared_ptr<CTexture> pTexture) { m_ppTextures.push_back(pTexture); }

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

	void SetStandardShader() { CMaterial::SetShader(m_pStandardShader); };
	void SetSkinnedAnimationShader() { CMaterial::SetShader(m_pSkinnedAnimationShader); };
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct CALLBACKKEY
{
	float  							m_fTime = 0.0f;
	void* m_pCallbackData = NULL;
};

#define _WITH_ANIMATION_INTERPOLATION

class CAnimationCallbackHandler
{
public:
	CAnimationCallbackHandler() { }
	~CAnimationCallbackHandler() { }

public:
	virtual void HandleCallback(void* pCallbackData, float fTrackPosition) { }
};

//#define _WITH_ANIMATION_SRT

class CAnimationSet
{
public:
	CAnimationSet(float fLength, int nFramesPerSecond, int nKeyFrameTransforms, int nSkinningBones, char* pstrName)
	{
		m_fLength = fLength;
		m_nFramesPerSecond = nFramesPerSecond;
		m_pstrAnimationSetName = pstrName;

		m_nKeyFrames = nKeyFrameTransforms;
		m_pfKeyFrameTimes.resize(nKeyFrameTransforms);
		m_ppxmf4x4KeyFrameTransforms.resize(nKeyFrameTransforms);
		for (int i = 0; i < nKeyFrameTransforms; i++) m_ppxmf4x4KeyFrameTransforms[i].resize(nSkinningBones);
	};
	
	~CAnimationSet()
	{
		m_pfKeyFrameTimes.clear();
		for (int i = 0; i < m_nKeyFrames; i++) m_ppxmf4x4KeyFrameTransforms[i].clear();
		m_ppxmf4x4KeyFrameTransforms.clear();
	};

public:
	std::string							m_pstrAnimationSetName;

	float							m_fLength = 0.0f;
	int								m_nFramesPerSecond = 0; //m_fTicksPerSecond

	int								m_nKeyFrames = 0;
	std::vector<float> m_pfKeyFrameTimes;
	std::vector<std::vector<XMFLOAT4X4>> m_ppxmf4x4KeyFrameTransforms;

#ifdef _WITH_ANIMATION_SRT
	int								m_nKeyFrameScales = 0;
	float* m_pfKeyFrameScaleTimes = NULL;
	XMFLOAT3** m_ppxmf3KeyFrameScales = NULL;
	int								m_nKeyFrameRotations = 0;
	float* m_pfKeyFrameRotationTimes = NULL;
	XMFLOAT4** m_ppxmf4KeyFrameRotations = NULL;
	int								m_nKeyFrameTranslations = 0;
	float* m_pfKeyFrameTranslationTimes = NULL;
	XMFLOAT3** m_ppxmf3KeyFrameTranslations = NULL;
#endif

public:
	XMFLOAT4X4 GetSRT(int nBone, float fPosition)
	{
		XMFLOAT4X4 xmf4x4Transform = Matrix4x4::Identity();

		for (int i = 0; i < (m_nKeyFrames - 1); i++)
		{
			if ((m_pfKeyFrameTimes[i] <= fPosition) && (fPosition < m_pfKeyFrameTimes[i + 1]))
			{
				float t = (fPosition - m_pfKeyFrameTimes[i]) / (m_pfKeyFrameTimes[i + 1] - m_pfKeyFrameTimes[i]);
				xmf4x4Transform = Matrix4x4::Interpolate(m_ppxmf4x4KeyFrameTransforms[i][nBone], m_ppxmf4x4KeyFrameTransforms[i + 1][nBone], t);
				break;
			}
		}
		if (fPosition >= m_pfKeyFrameTimes[m_nKeyFrames - 1]) xmf4x4Transform = m_ppxmf4x4KeyFrameTransforms[m_nKeyFrames - 1][nBone];

		return(xmf4x4Transform);
	};
};

class CAnimationSets
{
public:
	CAnimationSets(int nAnimationSets)
	{
		m_nAnimationSets = nAnimationSets;
		m_pAnimationSets.resize(nAnimationSets);
	};
	~CAnimationSets() {};

public:
	int	m_nAnimationSets = 0;
	std::vector<std::shared_ptr<CAnimationSet>> m_pAnimationSets;

	int	m_nBoneFrames = 0;
	std::vector<std::shared_ptr<CGameObject>> m_ppBoneFrameCaches; //[m_nBoneFrames]

};

class CAnimationTrack
{
public:
	CAnimationTrack() { };
	~CAnimationTrack() { };

public:
	BOOL 							m_bEnable = true;
	float 							m_fSpeed = 1.0f;
	float 							m_fPosition = -ANIMATION_CALLBACK_EPSILON;
	float 							m_fWeight = 1.0f;

	int 							m_nAnimationSet = 0; //AnimationSet Index

	int 							m_nType = ANIMATION_TYPE_LOOP; //Once, Loop, PingPong

	int 							m_nCallbackKeys = 0;
	std::vector<CALLBACKKEY> m_pCallbackKeys;

	std::shared_ptr<CAnimationCallbackHandler> m_pAnimationCallbackHandler;

public:
	void SetAnimationSet(int nAnimationSet) { m_nAnimationSet = nAnimationSet; }

	void SetEnable(bool bEnable) { m_bEnable = bEnable; }
	void SetSpeed(float fSpeed) { m_fSpeed = fSpeed; }
	void SetWeight(float fWeight) { m_fWeight = fWeight; }

	void SetPosition(float fPosition) { m_fPosition = fPosition; }
	float UpdatePosition(float fTrackPosition, float fElapsedTime, float fAnimationLength)
	{
		float fTrackElapsedTime = fElapsedTime * m_fSpeed;
		switch (m_nType)
		{
		case ANIMATION_TYPE_LOOP:
		{
			if (m_fPosition < 0.0f) m_fPosition = 0.0f;
			else
			{
				m_fPosition = fTrackPosition + fTrackElapsedTime;
				if (m_fPosition > fAnimationLength)
				{
					m_fPosition = -ANIMATION_CALLBACK_EPSILON;
					return(fAnimationLength);
				}
			}
			//			m_fPosition = fmod(fTrackPosition, m_pfKeyFrameTimes[m_nKeyFrames-1]); // m_fPosition = fTrackPosition - int(fTrackPosition / m_pfKeyFrameTimes[m_nKeyFrames-1]) * m_pfKeyFrameTimes[m_nKeyFrames-1];
			//			m_fPosition = fmod(fTrackPosition, m_fLength); //if (m_fPosition < 0) m_fPosition += m_fLength;
			//			m_fPosition = fTrackPosition - int(fTrackPosition / m_fLength) * m_fLength;
			break;
		}
		case ANIMATION_TYPE_ONCE:
			m_fPosition = fTrackPosition + fTrackElapsedTime;
			if (m_fPosition > fAnimationLength) m_fPosition = fAnimationLength;
			break;
		case ANIMATION_TYPE_PINGPONG:
			break;
		}

		return(m_fPosition);
	};

	void SetCallbackKeys(int nCallbackKeys)
	{
		m_nCallbackKeys = nCallbackKeys;
		m_pCallbackKeys.resize(nCallbackKeys);
	};
	void SetCallbackKey(int nKeyIndex, float fTime, void* pData)
	{
		m_pCallbackKeys[nKeyIndex].m_fTime = fTime;
		m_pCallbackKeys[nKeyIndex].m_pCallbackData = pData;
	};
	void SetAnimationCallbackHandler(std::shared_ptr<CAnimationCallbackHandler> pCallbackHandler)
	{
		m_pAnimationCallbackHandler = pCallbackHandler;
	};

	void HandleCallback()
	{
		if (m_pAnimationCallbackHandler)
		{
			for (int i = 0; i < m_nCallbackKeys; i++)
			{
				if (::IsEqual(m_pCallbackKeys[i].m_fTime, m_fPosition, ANIMATION_CALLBACK_EPSILON))
				{
					if (m_pCallbackKeys[i].m_pCallbackData) m_pAnimationCallbackHandler->HandleCallback(m_pCallbackKeys[i].m_pCallbackData, m_fPosition);
					break;
				}
			}
		}
	};
};

class CLoadedModelInfo
{
public:
	CLoadedModelInfo() { };
	~CLoadedModelInfo()	{ };

	std::shared_ptr<CGameObject> m_pModelRootObject;

	int m_nSkinnedMeshes = 0;
	std::vector <std::shared_ptr<CSkinnedMesh>> m_ppSkinnedMeshes; //[SkinnedMeshes], Skinned Mesh Cache

	std::shared_ptr<CAnimationSets> m_pAnimationSets;

public:
	void PrepareSkinning();;
};

class CAnimationController
{
public:
	CAnimationController(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, int nAnimationTracks, std::shared_ptr<CLoadedModelInfo> pModel)
	{
		m_nAnimationTracks = nAnimationTracks;
		m_pAnimationTracks.resize(nAnimationTracks);

		m_pModelRootObject = pModel->m_pModelRootObject;
		m_nSkinnedMeshes = pModel->m_nSkinnedMeshes;
		m_ppSkinnedMeshes = pModel->m_ppSkinnedMeshes;

		m_pAnimationSets = pModel->m_pAnimationSets;

		m_ppd3dcbSkinningBoneTransforms.resize(m_nSkinnedMeshes);
		m_ppcbxmf4x4MappedSkinningBoneTransforms.resize(m_nSkinnedMeshes);

		UINT ncbElementBytes = (((sizeof(XMFLOAT4X4) * SKINNED_ANIMATION_BONES) + 255) & ~255); //256의 배수
		for (int i = 0; i < m_nSkinnedMeshes; i++)
		{
			m_ppd3dcbSkinningBoneTransforms[i] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
			m_ppd3dcbSkinningBoneTransforms[i]->Map(0, NULL, (void**)&m_ppcbxmf4x4MappedSkinningBoneTransforms[i]);

			std::wstring name = L"Skinning Bone Transforms [" + std::to_wstring(i) + L"]";
			m_ppd3dcbSkinningBoneTransforms[i]->SetName(name.c_str());
		}

		for (int i = 0; i < m_nAnimationTracks; i++)
		{
			m_pAnimationTracks[i].SetAnimationSet(0);
			m_pAnimationTracks[i].SetCallbackKeys(0);
			m_pAnimationTracks[i].SetAnimationCallbackHandler(NULL);
		}
	};
	
	~CAnimationController() 
	{
		for (int i = 0; i < m_nSkinnedMeshes; i++)
		{
			if (m_ppd3dcbSkinningBoneTransforms[i]) {
				m_ppd3dcbSkinningBoneTransforms[i]->Unmap(0, NULL);
				m_ppd3dcbSkinningBoneTransforms[i].Reset();
			}
			if (m_ppcbxmf4x4MappedSkinningBoneTransforms[i]) m_ppcbxmf4x4MappedSkinningBoneTransforms[i] = NULL;
		}
	};

public:
	float 							m_fTime = 0.0f;

	int 							m_nAnimationTracks = 0;
	std::vector<CAnimationTrack> m_pAnimationTracks;

	std::shared_ptr<CAnimationSets> m_pAnimationSets;

	int m_nSkinnedMeshes = 0;
	std::vector<std::shared_ptr<CSkinnedMesh>> m_ppSkinnedMeshes; //[SkinnedMeshes], Skinned Mesh Cache

	std::vector<ComPtr<ID3D12Resource>> m_ppd3dcbSkinningBoneTransforms; //[SkinnedMeshes]
	std::vector<XMFLOAT4X4*> m_ppcbxmf4x4MappedSkinningBoneTransforms; //[SkinnedMeshes]

public:
	void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
	{
		for (int i = 0; i < m_nSkinnedMeshes; i++)
		{
			m_ppSkinnedMeshes[i]->m_pd3dcbSkinningBoneTransforms = m_ppd3dcbSkinningBoneTransforms[i];
			m_ppSkinnedMeshes[i]->m_pcbxmf4x4MappedSkinningBoneTransforms = m_ppcbxmf4x4MappedSkinningBoneTransforms[i];
		}
	};

	void SetTrackAnimationSet(int nAnimationTrack, int nAnimationSet) {	if (!m_pAnimationTracks.empty()) m_pAnimationTracks[nAnimationTrack].m_nAnimationSet = nAnimationSet;};

	void SetTrackEnable(int nAnimationTrack, bool bEnable) { if (!m_pAnimationTracks.empty()) m_pAnimationTracks[nAnimationTrack].SetEnable(bEnable); };
	void SetTrackPosition(int nAnimationTrack, float fPosition) { if (!m_pAnimationTracks.empty()) m_pAnimationTracks[nAnimationTrack].SetPosition(fPosition); };
	void SetTrackSpeed(int nAnimationTrack, float fSpeed) { if (!m_pAnimationTracks.empty()) m_pAnimationTracks[nAnimationTrack].SetSpeed(fSpeed); };
	void SetTrackWeight(int nAnimationTrack, float fWeight) { if (!m_pAnimationTracks.empty()) m_pAnimationTracks[nAnimationTrack].SetWeight(fWeight); };

	void SetCallbackKeys(int nAnimationTrack, int nCallbackKeys) { if (!m_pAnimationTracks.empty()) m_pAnimationTracks[nAnimationTrack].SetCallbackKeys(nCallbackKeys); };
	void SetCallbackKey(int nAnimationTrack, int nKeyIndex, float fTime, void* pData) { if (!m_pAnimationTracks.empty()) m_pAnimationTracks[nAnimationTrack].SetCallbackKey(nKeyIndex, fTime, pData); };
	void SetAnimationCallbackHandler(int nAnimationTrack, std::shared_ptr<CAnimationCallbackHandler> pCallbackHandler) { if (!m_pAnimationTracks.empty()) m_pAnimationTracks[nAnimationTrack].SetAnimationCallbackHandler(pCallbackHandler); };

	void AdvanceTime(float fElapsedTime, CGameObject* pRootGameObject);;

public:
	bool							m_bRootMotion = false;
	std::shared_ptr<CGameObject> m_pModelRootObject;

	std::shared_ptr<CGameObject> m_pRootMotionObject;
	XMFLOAT3 m_xmf3FirstRootMotionPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);

	void SetRootMotion(bool bRootMotion) { m_bRootMotion = bRootMotion; }

	virtual void OnRootMotion(CGameObject* pRootGameObject) { }
	virtual void OnAnimationIK(CGameObject* pRootGameObject) { }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CGameObject : public std::enable_shared_from_this<CGameObject>
{
public:
	CGameObject();
	virtual ~CGameObject();

	// Object Initialization
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12CommandList* pd3dCommandList) {
		// Transform Owner Setting
		m_pTransform->SetOwner(shared_from_this());
	};

	static std::shared_ptr<CGameObject> CreateObject() { return std::make_shared<CGameObject>(); }

	// Active Flag
	bool IsActive() { return m_bActive; }
	void SetActive(bool bActive) { m_bActive = bActive; }

	// Object ID
	UINT GetObjectID() { return m_nObjectID; }
	void SetObjectID(UINT nObjectID) { m_nObjectID = nObjectID; }

	// Object Name
	std::string GetName() { return m_strName; }
	void SetName(const std::string& strName);
	virtual std::string GetDefaultName() { return "CGameObject"; }

	// Transform
	DirectX::XMFLOAT3 GetPosition() { return m_pTransform->GetPosition(); }
	DirectX::XMFLOAT3 GetRightVector() { return m_pTransform->GetRight(); }
	DirectX::XMFLOAT3 GetUpVector() { return m_pTransform->GetUp(); }
	DirectX::XMFLOAT3 GetLookVector() { return m_pTransform->GetLook(); }
	DirectX::XMFLOAT3 GetScale() { return m_pTransform->GetScale(); }

	DirectX::XMFLOAT3 GetRotation() { return m_pTransform->GetRotation(); }
	float GetPitch() { return m_pTransform->GetRotation().x; } // X 축을	기준으로 회전
	float GetYaw() { return m_pTransform->GetRotation().y; } // Y 축을 기준으로 회전
	float GetRoll() { return m_pTransform->GetRotation().z; } // Z 축을 기준으로 회전

	DirectX::XMFLOAT4X4 GetLocalMatrix() { return m_pTransform->GetLocalMatrix(); }
	DirectX::XMFLOAT4X4 GetWorldMatrix() { return m_pTransform->GetWorldMatrix(); }

	DirectX::XMFLOAT3 GetLocalPosition() { return m_pTransform->GetLocalPosition(); };

	std::shared_ptr<CTransform> GetTransform() { return m_pTransform; }

	void SetPosition(DirectX::XMFLOAT3 xmf3Position) { m_pTransform->SetPosition(xmf3Position); }
	void SetPosition(float fx, float fy, float fz) {  m_pTransform->SetPosition(fx, fy, fz);  }
	void SetScale(DirectX::XMFLOAT3 xmf3Scale) { m_pTransform->SetPosition(xmf3Scale); };
	void SetScale(float fx, float fy, float fz) { m_pTransform->SetPosition(fx, fy, fz); };

	void Move(DirectX::XMFLOAT3 xmf3Shift) { m_pTransform->Move(xmf3Shift); } ;
	void Move(float x, float y, float z) { Move(DirectX::XMFLOAT3(x, y, z)); }

	void Move(DWORD dwDirection, float fDistance, float deltaTime);

	void MoveStrafe(float fDistance = 1.0f) { m_pTransform->MoveStrafe(fDistance); };
	void MoveUp(float fDistance = 1.0f) { m_pTransform->MoveUp(fDistance); };
	void MoveForward(float fDistance = 1.0f) { m_pTransform->MoveForward(fDistance); };

	void Rotate(float fPitch = 10.0f, float fYaw = 10.0f, float fRoll = 10.0f) { m_pTransform->Rotate(fPitch, fYaw, fRoll); }
	void Rotate(const XMFLOAT3& pxmf3Axis, float fAngle) { m_pTransform->Rotate(pxmf3Axis, fAngle); }
	void Rotate(const XMFLOAT4& pxmf4Quaternion) { m_pTransform->Rotate(pxmf4Quaternion); }

	void SetLocalMatrix(DirectX::XMFLOAT4X4 xmf4x4Local) { m_pTransform->SetLocalMatrix(xmf4x4Local); }
	void SetLocalTransform(DirectX::XMFLOAT4X4 xmf4x4Local) { SetLocalMatrix(xmf4x4Local); }
	void SetWorldMatrix(DirectX::XMFLOAT4X4 xmf4x4World) { m_pTransform->SetWorldMatrix(xmf4x4World); }
	void SetWorldTransform(DirectX::XMFLOAT4X4 xmf4x4World) { SetWorldTransform(xmf4x4World); }

	void UpdateTransform(const DirectX::XMFLOAT4X4* xmf4x4ParentMatrix = nullptr);
	void UpdateTransform(const DirectX::XMFLOAT4X4& xmf4x4ParentMatrix);
	void UpdateTransform(std::shared_ptr<CGameObject>& pGameobject)
	{
		m_pTransform->UpdateTransform(pGameobject); 

		// Update Child
		for (auto& pChild : m_pChilds) pChild->UpdateTransform(GetWorldMatrix());
	}

	// 상속 관계
	std::shared_ptr<CGameObject> GetParent() { return m_pParent.lock(); }
	std::vector<std::shared_ptr<CGameObject>> GetChilds() { return m_pChilds; }
	std::shared_ptr<CGameObject> GetChild(int nIndex) { return m_pChilds[nIndex]; }

	void SetParent(std::shared_ptr<CGameObject> pParent) { m_pParent = pParent; };
	void SetChild(std::shared_ptr<CGameObject> pChild) { m_pChilds.push_back(pChild); pChild->SetParent(shared_from_this()); };
	
	// Object Update
	virtual void Update(float fTimeElapsed);

	// Object Render
	virtual void OnPrepareRender() { }
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr);

	// Component
	template <typename T>
	std::shared_ptr<T> AddComponent(std::shared_ptr<CGameObject> pOwner)
	{
		std::shared_ptr<T> pComponent = std::make_shared<T>();
		pComponent->SetOwner(pOwner);
		m_pComponents[typeid(T).name()] = pComponent;
		return pComponent;
	};

	std::shared_ptr<CTransformComponent> GetComponent()
	{
		return m_pTransform;
	};

	template <typename T>
	std::shared_ptr<T> GetComponent()
	{
		if constexpr (std::is_same_v<T, CTransform>) return m_pTransform;
		
		auto iter = m_pComponents.find(typeid(T).name());
		if (iter != m_pComponents.end()) return std::dynamic_pointer_cast<T>(iter->second);
		return nullptr;
	}

	// Object Collision
	//virtual void OnCollision(CGameObject* pGameObject) {}

	// Mesh
	void SetMesh(std::shared_ptr<CMesh> pMesh) { m_pMesh = pMesh; }
	UINT GetMeshType() { return((m_pMesh) ? m_pMesh->GetType() : 0x00); }

	// Material
	void MaterialResize(int nMaterials) { m_ppMaterials.resize(nMaterials); }
	void AddMaterial(std::shared_ptr<CMaterial> pMaterial) { m_ppMaterials.push_back(pMaterial); }
	void SetMaterial(int nIndex, std::shared_ptr<CMaterial> pMaterial) { if(m_ppMaterials.size() <= nIndex) m_ppMaterials.resize(nIndex + 1); m_ppMaterials[nIndex] = pMaterial; }

	// Shader
	void SetShader(std::shared_ptr<CShader> pShader, int nIndex = 0);

	// Shader Variables
	void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseShaderVariables();

protected:
	bool m_bActive; // Active Flag

	UINT m_nObjectID; // Object ID
	std::string m_strName;  // Object Name

	std::shared_ptr<CMesh> m_pMesh; // Object Mesh

	// CMaterial
	UINT m_nMaterials = 0;
	std::vector<std::shared_ptr<CMaterial>> m_ppMaterials; // Object CMaterial
public:
	// Transform
	std::shared_ptr<CTransform> m_pTransform = std::make_shared<CTransform>();

	// Component
	std::unordered_map<std::string, std::shared_ptr<CComponent>> m_pComponents;

	// Shader Variables
	ComPtr<ID3D12Resource> m_pd3dcbGameObject;
	CB_GAMEOBJECT_INFO* m_pcbMappedObject = nullptr;

protected:
	// Parent
	std::weak_ptr<CGameObject> m_pParent;

	// Child
	std::vector<std::shared_ptr<CGameObject>> m_pChilds; // Child Object
public:

	std::shared_ptr<CAnimationController> m_pSkinnedAnimationController;
	// Load Model
	void LoadMaterialsFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::shared_ptr<CGameObject> pParent, std::ifstream& File, std::shared_ptr<CShader> pShader);
	std::shared_ptr<CTexture> FindReplicatedTexture(const _TCHAR* pstrTextureName);
	
	void FindAndSetSkinnedMesh(std::vector<std::shared_ptr<CSkinnedMesh>>& ppSkinnedMeshes, int* pnSkinnedMesh)
	{
		if (m_pMesh && (m_pMesh->GetType() & VERTEXT_BONE_INDEX_WEIGHT)) ppSkinnedMeshes[(*pnSkinnedMesh)++] = std::dynamic_pointer_cast<CSkinnedMesh>(m_pMesh) ;
		
		for (auto& pChild : m_pChilds) pChild->FindAndSetSkinnedMesh(ppSkinnedMeshes, pnSkinnedMesh);
	};

	static void LoadAnimationFromFile(std::ifstream& pInFile, std::shared_ptr<CLoadedModelInfo> pLoadedModel);
	static std::shared_ptr<CGameObject> LoadFrameHierarchyFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CGameObject> pParent, std::ifstream& file, std::shared_ptr<CShader> pShader, int* pnSkinnedMeshes);
	static std::shared_ptr<CLoadedModelInfo> LoadGeometryAndAnimationFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, const char* pstrFileName, std::shared_ptr<CShader> pShader);

	std::shared_ptr<CGameObject> FindFrame(std::string strFrameName);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CZombieAnimationController : public CAnimationController
{
public:
	CZombieAnimationController(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, int nAnimationTracks, std::shared_ptr<CLoadedModelInfo> pModel);
	virtual ~CZombieAnimationController();
};

////////////////////////////////////////////////////////////////////////////////////////
//

class CRotatingObject : public CGameObject
{
public:
	CRotatingObject();
	virtual ~CRotatingObject();

	// Object Initialization
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12CommandList* pd3dCommandList);
	virtual std::string GetDefaultName() override { return "CRotatingObject"; } 

	static std::shared_ptr<CRotatingObject> Create(ID3D12Device* pd3dDevice, ID3D12CommandList* pd3dCommandList);

	// Object Update
	virtual void Update(float fTimeElapsed) override;

	// Set Rotation Speed
	void SetRotationSpeed(float fRotationSpeed) { m_fRotationSpeed = fRotationSpeed; }

	// Set Rotation Axis
	void SetRotationAxis(DirectX::XMFLOAT3 xmf3RotationAxis) { m_xmf3RotationAxis = xmf3RotationAxis; }

private:
	float m_fRotationSpeed = 90.0f; // 초당 회전 속도
	XMFLOAT3 m_xmf3RotationAxis = XMFLOAT3(0.0f, 1.0f, 0.0f); // 회전 축

};

////////////////////////////////////////////////////////////////////////////////////////
//

class CCubeObject : public CRotatingObject
{
public:
	CCubeObject();
	virtual ~CCubeObject();

	// Object Initialization
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
	virtual std::string GetDefaultName() override { return "CCubeObject"; }

	static std::shared_ptr<CCubeObject> Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CHeightMapTerrain;

class CZombieObject : public CGameObject
{
public:
	CZombieObject();
	virtual ~CZombieObject();

	// Object Initialization
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CLoadedModelInfo> pModel, int nAnimationTracks);
	virtual std::string GetDefaultName() override { return "CZombieObject"; }

	static std::shared_ptr<CZombieObject> Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CGameObject> pTerrain, std::shared_ptr<CLoadedModelInfo> pModel, int nAnimationTracks);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CSkyBox : public CGameObject
{
public:
	CSkyBox();
	virtual ~CSkyBox();

	// Object Initialization
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
	virtual std::string GetDefaultName() override { return "CSkyBox"; }

	static std::shared_ptr<CSkyBox> Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);

	// Object Render
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CHeightMapTerrain : public CGameObject
{
public:
	CHeightMapTerrain();
	virtual ~CHeightMapTerrain();

	// Object Initialization
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature,
		LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength,
		XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color);
	virtual std::string GetDefaultName() override { return "CHeightMapTerrain"; }

	static std::shared_ptr<CHeightMapTerrain> Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature,
		LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength,
		XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color);

	// Object Render
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera) override;

	//지형의 높이를 계산하는 함수이다(월드 좌표계). 높이 맵의 높이에 스케일의 y를 곱한 값이다. 
	float GetHeight(float x, float z) { return(m_pHeightMapImage->GetHeight(x / m_xmf3Scale.x, z / m_xmf3Scale.z) * m_xmf3Scale.y); }
	
	//지형의 법선 벡터를 계산하는 함수이다(월드 좌표계). 높이 맵의 법선 벡터를 사용한다. 
	XMFLOAT3 GetNormal(float x, float z) {
		return(m_pHeightMapImage->GetHeightMapNormal(int(x / m_xmf3Scale.x),
			int(z / m_xmf3Scale.z)));
	}

	int GetHeightMapWidth() { return(m_pHeightMapImage->GetHeightMapWidth()); }
	int GetHeightMapLength() { return(m_pHeightMapImage->GetHeightMapLength()); }

	XMFLOAT3 GetScale() { return(m_xmf3Scale); }

	//지형의 크기(가로/세로)를 반환한다. 높이 맵의 크기에 스케일을 곱한 값이다. 
	float GetWidth() { return(m_nWidth * m_xmf3Scale.x); }
	float GetLength() { return(m_nLength * m_xmf3Scale.z); }

private:
	//지형의 높이 맵으로 사용할 이미지이다. 
	std::shared_ptr<CHeightMapImage> m_pHeightMapImage;

	//높이 맵의 가로와 세로 크기이다. 
	int m_nWidth;
	int m_nLength;

	//지형을 실제로 몇 배 확대할 것인가를 나타내는 스케일 벡터이다. 
	XMFLOAT3 m_xmf3Scale;
};

