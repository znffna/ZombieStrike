///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// GameObject.cpp : GameObject 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#include "GameObject.h"
#include "Shader.h"
#include "Camera.h"

#include "Scene.h"

std::shared_ptr<CShader> CMaterial::m_pStandardShader;
std::shared_ptr<CShader> CMaterial::m_pSkinnedAnimationShader;

CGameObject::CGameObject()
{
	// Object Info
	static UINT nGameObjectID = 0;
	m_bActive = true;
	m_nObjectID = nGameObjectID++;

	// Default Name
	m_strName = "GameObject_" + std::to_string(m_nObjectID);
}

CGameObject::~CGameObject()
{
	// Release Child Object
	m_pChilds.clear();

	// Release Mesh
	m_pMesh.reset();

	// Release Materials
	m_ppMaterials.clear();

	// Release Skinned Animation Controller
	m_pSkinnedAnimationController.reset();

	// Release Shader Variables
	ReleaseShaderVariables();

	// Release Components
	m_pTransform.reset();
	m_pComponents.clear();
}

void CGameObject::SetName(const std::string& strName)
{
	if (strName.length() > 0)
	{
		m_strName = strName;
	}
	else
	{
		m_strName = GetDefaultName() + "_" + std::to_string(m_nObjectID);
	}
}

void CGameObject::Move(DWORD dwDirection, float fDistance, float deltaTime)
{
	// 키보드 입력으로부터 이동 방향을 추출한다.
	XMFLOAT3 xmf3Direction(0.0f, 0.0f, 0.0f);
	if (dwDirection & DIR_FORWARD) xmf3Direction = Vector3::Add(xmf3Direction, GetLookVector());
	if (dwDirection & DIR_BACKWARD) xmf3Direction = Vector3::Add(xmf3Direction, Vector3::ScalarProduct(GetLookVector(), -1.0f));
	if (dwDirection & DIR_RIGHT) xmf3Direction = Vector3::Add(xmf3Direction, GetRightVector());
	if (dwDirection & DIR_LEFT) xmf3Direction = Vector3::Add(xmf3Direction, Vector3::ScalarProduct(GetRightVector(), -1.0f));
	if (dwDirection & DIR_UP) xmf3Direction = Vector3::Add(xmf3Direction, GetUpVector());
	if (dwDirection & DIR_DOWN) xmf3Direction = Vector3::Add(xmf3Direction, Vector3::ScalarProduct(GetUpVector(), -1.0f));

	// 이동 방향으로부터 이동 거리를 계산한다.
	if(auto cRigidbody = GetComponent<CRigidBody>())
	{
		// 물리 엔진을 사용하는 경우
		cRigidbody->AddVelocity(Vector3::ScalarProduct(xmf3Direction, fDistance));
	}
	else
	{
		// 물리 엔진을 사용하지 않는 경우
		xmf3Direction = Vector3::Normalize(xmf3Direction);
		xmf3Direction = Vector3::ScalarProduct(xmf3Direction, fDistance * deltaTime);
		Move(xmf3Direction);
	}
}

void CGameObject::UpdateTransform(const DirectX::XMFLOAT4X4* xmf4x4ParentMatrix)
{ 
	m_pTransform->UpdateTransform(xmf4x4ParentMatrix); 

	for (auto& pChild : m_pChilds) pChild->UpdateTransform(GetWorldMatrix());
}

void CGameObject::UpdateTransform(const DirectX::XMFLOAT4X4& xmf4x4ParentMatrix)
{
	UpdateTransform(&xmf4x4ParentMatrix);
}

void CGameObject::Update(float fTimeElapsed)
{
	// Component Update
	for (auto& pComponent : m_pComponents)
	{
		pComponent.second->Update(fTimeElapsed);
	}

	OnPrepareRender();

	if (m_pSkinnedAnimationController) m_pSkinnedAnimationController->AdvanceTime(fTimeElapsed, this);

	for (auto& pChild : m_pChilds) pChild->Update(fTimeElapsed);
}

void CGameObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (false == m_bActive) return;

	// Render Object

	// Skinned Animation Update
	if (m_pSkinnedAnimationController)
		m_pSkinnedAnimationController->UpdateShaderVariables(pd3dCommandList);

	if (m_pMesh) {
		// Set Shader Variables
		UpdateShaderVariables(pd3dCommandList); // GameObject Matrix Update

		for (int i = 0; i < m_ppMaterials.size(); ++i)
		{
			std::shared_ptr<CMaterial>& pMaterial = m_ppMaterials[i];
			if (pMaterial)
			{
				// Set Pipeline State
				if (pMaterial->m_pShader) pMaterial->m_pShader->OnPrepareRender(pd3dCommandList, 0); // Render(pd3dCommandList, pCamera);
				// Material Update
				pMaterial->UpdateShaderVariables(pd3dCommandList);
			}
			// Render Mesh
			m_pMesh->Render(pd3dCommandList, i);
		}
	}

	// Render Child Object
	for (auto& pChild : m_pChilds)
	{
		pChild->Render(pd3dCommandList, pCamera);
	}
}

void CGameObject::SetShader(std::shared_ptr<CShader> pShader, int nIndex)
{
	if (nIndex < m_ppMaterials.size())
	{
		m_ppMaterials[nIndex]->SetShader(pShader);
	}
	else if(m_ppMaterials.empty()){
		std::shared_ptr<CMaterial> pMaterial= std::make_shared<CMaterial>();
		pMaterial->SetShader(pShader);
		m_ppMaterials.push_back(pMaterial);
	}
	else {
		// Error
		std::wstring DebugString = L"Error : GameObject::SetShader() - nIndex is out of range";
		OutputDebugString(DebugString.c_str());
		throw;
	}
}

void CGameObject::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
#ifdef _USE_OBJECT_MATERIAL_CBV
	// Create Constant Buffer
	UINT ncbElementBytes = ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255); //256의 배수
	m_pd3dcbGameObject = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_GENERIC_READ, NULL);

	// Map Constant Buffer
	m_pd3dcbGameObject->Map(0, nullptr, (void**)&m_pcbMappedObject);
	ZeroMemory(m_pcbMappedObject, sizeof(CB_GAMEOBJECT_INFO));
#endif // _USE_OBJECT_MATERIAL_CBV
}

void CGameObject::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
#ifdef _USE_OBJECT_MATERIAL_CBV
	// Update Constant Buffer
	//m_pcbMappedObject->m_xmf4x4World = m_xmf4x4World; // DirectX는 행렬을 전치해서 셰이더에 적용해야 한다.
	XMStoreFloat4x4(&m_pcbMappedObject->m_xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));

#ifdef _DEBUG
	// Debug Output
	std::wstring DebugString = L"GameObject [" + std::to_wstring(m_nObjectID) + L"] - position ("
		+ std::to_wstring(m_xmf4x4World._41) + L", "
		+ std::to_wstring(m_xmf4x4World._42) + L", "
		+ std::to_wstring(m_xmf4x4World._43) + L")\n";
	OutputDebugString(DebugString.c_str());
#endif // _DEBUG

	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = m_pd3dcbGameObject->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(ROOT_PARAMETER_OBJECT, d3dGpuVirtualAddress);
#endif // _USE_OBJECT_MATERIAL_CBV
	// Update Shader Variables
	XMFLOAT4X4 xmf4x4World;

	xmf4x4World = GetWorldMatrix();
	XMStoreFloat4x4(&xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&xmf4x4World)));

	pd3dCommandList->SetGraphicsRoot32BitConstants(ROOT_PARAMETER_OBJECT, 16, &xmf4x4World, 0);
}

void CGameObject::ReleaseShaderVariables()
{
#ifdef _USE_OBJECT_MATERIAL_CBV
	if (m_pd3dcbGameObject) m_pd3dcbGameObject->Unmap(0, nullptr);
	m_pd3dcbGameObject.Reset();
#endif // _USE_OBJECT_MATERIAL_CBV
}

std::shared_ptr<CTexture> CGameObject::FindReplicatedTexture(const _TCHAR* pstrTextureName)
{
	for (UINT i = 0; i < m_nMaterials; i++)
	{
		if (m_ppMaterials[i])
		{
			for (UINT j = 0; j < m_ppMaterials[i]->m_nTextures; j++)
			{
				if (m_ppMaterials[i]->m_ppTextures[j])
				{
					if (!_tcsncmp(m_ppMaterials[i]->m_strTextureNames[j].data(), pstrTextureName, _tcslen(pstrTextureName))) return(m_ppMaterials[i]->m_ppTextures[j]);
				}
			}
		}
	}
	std::shared_ptr<CTexture> pTexture;
	for (auto& pChild : m_pChilds) if (pTexture = pChild->FindReplicatedTexture(pstrTextureName)) return(pTexture);

	return(NULL);
}

void CGameObject::LoadMaterialsFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::shared_ptr<CGameObject> pParent, std::ifstream& File, std::shared_ptr<CShader> pShader)
{
	char pstrToken[64] = { '\0' };
	int nMaterial = 0;
	UINT nReads = 0;

	m_nMaterials = ReadIntegerFromFile(File);

	m_ppMaterials.resize(m_nMaterials);

	std::shared_ptr<CMaterial> pMaterial;

	for (; ; )
	{
		::ReadStringFromFile(File, pstrToken);

		if (!strcmp(pstrToken, "<Material>:"))
		{
			nMaterial = ReadIntegerFromFile(File);

			pMaterial = std::make_shared<CMaterial>(7); //0:Albedo, 1:Specular, 2:Metallic, 3:Normal, 4:Emission, 5:DetailAlbedo, 6:DetailNormal

			if (!pShader)
			{
				UINT nMeshType = GetMeshType();
				if (nMeshType & VERTEXT_NORMAL_TANGENT_TEXTURE)
				{
					if (nMeshType & VERTEXT_BONE_INDEX_WEIGHT)
					{
						pMaterial->SetSkinnedAnimationShader();
					}
					else
					{
						pMaterial->SetStandardShader();
					}
				}
			}
			pMaterial->SetName(GetName());
			SetMaterial(nMaterial, pMaterial);
		}
		else if (!strcmp(pstrToken, "<AlbedoColor>:"))
		{
			XMFLOAT4 m_xmf4AlbedoColor;
			File.read((char*)&(m_xmf4AlbedoColor), sizeof(XMFLOAT4));
			pMaterial->SetAlbedo(m_xmf4AlbedoColor);
			//nReads = (UINT)::fread(&(pMaterial->m_xmf4AlbedoColor), sizeof(float), 4, pInFile);
		}
		else if (!strcmp(pstrToken, "<EmissiveColor>:"))
		{
			XMFLOAT4 m_xmf4EmissiveColor;
			File.read((char*)&(m_xmf4EmissiveColor), sizeof(XMFLOAT4));
			pMaterial->SetEmissive(m_xmf4EmissiveColor);
			//nReads = (UINT)::fread(&(pMaterial->m_xmf4EmissiveColor), sizeof(float), 4, pInFile);
		}
		else if (!strcmp(pstrToken, "<SpecularColor>:"))
		{
			XMFLOAT4 SpecularColor;
			File.read((char*)&(SpecularColor), sizeof(XMFLOAT4));
			pMaterial->SetSpecular(SpecularColor);
			//nReads = (UINT)::fread(&(pMaterial->m_xmf4SpecularColor), sizeof(float), 4, pInFile);
		}
		else if (!strcmp(pstrToken, "<Glossiness>:"))
		{
			float Glossiness;
			Glossiness = ReadFloatFromFile(File);
			pMaterial->SetGlossiness(Glossiness);
			//nReads = (UINT)::fread(&(pMaterial->m_fGlossiness), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<Smoothness>:"))
		{
			float Smoothness;
			Smoothness = ReadFloatFromFile(File);
			pMaterial->SetSmoothness(Smoothness);
			//nReads = (UINT)::fread(&(pMaterial->m_fSmoothness), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<Metallic>:"))
		{
			float Metallic;
			Metallic = ReadFloatFromFile(File);
			pMaterial->SetMetallic(Metallic);
			//nReads = (UINT)::fread(&(pMaterial->m_fMetallic), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<SpecularHighlight>:"))
		{
			float SpecularHighlight;
			SpecularHighlight = ReadFloatFromFile(File);
			pMaterial->SetSpecularHighlight(SpecularHighlight);
			//nReads = (UINT)::fread(&(pMaterial->m_fSpecularHighlight), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<GlossyReflection>:"))
		{
			float GlossyReflection;
			GlossyReflection = ReadFloatFromFile(File);
			pMaterial->SetGlossyReflection(GlossyReflection);
			//nReads = (UINT)::fread(&(pMaterial->m_fGlossyReflection), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<AlbedoMap>:"))
		{
			pMaterial->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_ALBEDO_MAP, ROOT_PARAMETER_ALBEDO_TEXTURE, pMaterial->m_strTextureNames[0], (pMaterial->m_ppTextures[0]), pParent, File, pShader);
		}
		else if (!strcmp(pstrToken, "<SpecularMap>:"))
		{
			m_ppMaterials[nMaterial]->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_SPECULAR_MAP, ROOT_PARAMETER_SPECULAR_TEXTURE, pMaterial->m_strTextureNames[1], (pMaterial->m_ppTextures[1]), pParent, File, pShader);
		}
		else if (!strcmp(pstrToken, "<NormalMap>:"))
		{
			m_ppMaterials[nMaterial]->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_NORMAL_MAP, ROOT_PARAMETER_NORMAL_TEXTURE, pMaterial->m_strTextureNames[2], (pMaterial->m_ppTextures[2]), pParent, File, pShader);
		}
		else if (!strcmp(pstrToken, "<MetallicMap>:"))
		{
			m_ppMaterials[nMaterial]->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_METALLIC_MAP, ROOT_PARAMETER_METALLIC_TEXTURE, pMaterial->m_strTextureNames[3], (pMaterial->m_ppTextures[3]), pParent, File, pShader);
		}
		else if (!strcmp(pstrToken, "<EmissionMap>:"))
		{
			m_ppMaterials[nMaterial]->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_EMISSION_MAP, ROOT_PARAMETER_EMISSION_TEXTURE, pMaterial->m_strTextureNames[4], (pMaterial->m_ppTextures[4]), pParent, File, pShader);
		}
		else if (!strcmp(pstrToken, "<DetailAlbedoMap>:"))
		{
			m_ppMaterials[nMaterial]->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_DETAIL_ALBEDO_MAP, ROOT_PARAMETER_DETAIL_ALBEDO_TEXTURE, pMaterial->m_strTextureNames[5], (pMaterial->m_ppTextures[5]), pParent, File, pShader);
		}
		else if (!strcmp(pstrToken, "<DetailNormalMap>:"))
		{
			m_ppMaterials[nMaterial]->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_DETAIL_NORMAL_MAP, ROOT_PARAMETER_DETAIL_NORMAL_TEXTURE, pMaterial->m_strTextureNames[6], (pMaterial->m_ppTextures[6]), pParent, File, pShader);
		}
		else if (!strcmp(pstrToken, "</Materials>"))
		{
			break;
		}
	}
}

void CGameObject::LoadAnimationFromFile(std::ifstream& pInFile, std::shared_ptr<CLoadedModelInfo> pLoadedModel)
{
	char pstrToken[64] = { '\0' };
	UINT nReads = 0;

	int nAnimationSets = 0;

	for (; ; )
	{
		::ReadStringFromFile(pInFile, pstrToken);
		if (!strcmp(pstrToken, "<AnimationSets>:"))
		{
			nAnimationSets = ::ReadIntegerFromFile(pInFile);
			pLoadedModel->m_pAnimationSets = std::make_shared<CAnimationSets>(nAnimationSets);
		}
		else if (!strcmp(pstrToken, "<FrameNames>:"))
		{
			pLoadedModel->m_pAnimationSets->m_nBoneFrames = ::ReadIntegerFromFile(pInFile);
			pLoadedModel->m_pAnimationSets->m_ppBoneFrameCaches.resize(pLoadedModel->m_pAnimationSets->m_nBoneFrames);

			for (int j = 0; j < pLoadedModel->m_pAnimationSets->m_nBoneFrames; j++)
			{
				::ReadStringFromFile(pInFile, pstrToken);
				pLoadedModel->m_pAnimationSets->m_ppBoneFrameCaches[j] = pLoadedModel->m_pModelRootObject->FindFrame(pstrToken);

#ifdef _WITH_DEBUG_SKINNING_BONE
				TCHAR pstrDebug[256] = { 0 };
				TCHAR pwstrAnimationBoneName[64] = { 0 };
				TCHAR pwstrBoneCacheName[64] = { 0 };
				size_t nConverted = 0;
				mbstowcs_s(&nConverted, pwstrAnimationBoneName, 64, pstrToken, _TRUNCATE);
				mbstowcs_s(&nConverted, pwstrBoneCacheName, 64, pLoadedModel->m_pAnimationSets->m_ppBoneFrameCaches[j]->m_strName.c_str(), _TRUNCATE);
				_stprintf_s(pstrDebug, 256, _T("AnimationBoneFrame:: Cache(%s) AnimationBone(%s)\n"), pwstrBoneCacheName, pwstrAnimationBoneName);
				OutputDebugString(pstrDebug);
#endif
			}
		}
		else if (!strcmp(pstrToken, "<AnimationSet>:"))
		{
			int nAnimationSet = ::ReadIntegerFromFile(pInFile);

			::ReadStringFromFile(pInFile, pstrToken); //Animation Set Name

			float fLength = ::ReadFloatFromFile(pInFile);
			int nFramesPerSecond = ::ReadIntegerFromFile(pInFile);
			int nKeyFrames = ::ReadIntegerFromFile(pInFile);

			pLoadedModel->m_pAnimationSets->m_pAnimationSets[nAnimationSet] = std::make_shared <CAnimationSet>(fLength, nFramesPerSecond, nKeyFrames, pLoadedModel->m_pAnimationSets->m_nBoneFrames, pstrToken);

			for (int i = 0; i < nKeyFrames; i++)
			{
				::ReadStringFromFile(pInFile, pstrToken);
				if (!strcmp(pstrToken, "<Transforms>:"))
				{
					std::shared_ptr <CAnimationSet> pAnimationSet = pLoadedModel->m_pAnimationSets->m_pAnimationSets[nAnimationSet];

					int nKey = ::ReadIntegerFromFile(pInFile); //i
					float fKeyTime = ::ReadFloatFromFile(pInFile);

#ifdef _WITH_ANIMATION_SRT
					m_pfKeyFrameScaleTimes[i] = fKeyTime;
					m_pfKeyFrameRotationTimes[i] = fKeyTime;
					m_pfKeyFrameTranslationTimes[i] = fKeyTime;
					nReads = (UINT)::fread(pAnimationSet->m_ppxmf3KeyFrameScales[i], sizeof(XMFLOAT3), pLoadedModel->m_pAnimationSets->m_nBoneFrames, pInFile);
					nReads = (UINT)::fread(pAnimationSet->m_ppxmf4KeyFrameRotations[i], sizeof(XMFLOAT4), pLoadedModel->m_pAnimationSets->m_nBoneFrames, pInFile);
					nReads = (UINT)::fread(pAnimationSet->m_ppxmf3KeyFrameTranslations[i], sizeof(XMFLOAT3), pLoadedModel->m_pAnimationSets->m_nBoneFrames, pInFile);
#else
					pAnimationSet->m_pfKeyFrameTimes[i] = fKeyTime;
					pInFile.read((char*)(pAnimationSet->m_ppxmf4x4KeyFrameTransforms[i].data()), sizeof(XMFLOAT4X4) * pLoadedModel->m_pAnimationSets->m_nBoneFrames);
#endif
				}
			}
		}
		else if (!strcmp(pstrToken, "</AnimationSets>"))
		{
			break;
		}
	}
}

std::shared_ptr<CGameObject> CGameObject::LoadFrameHierarchyFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CGameObject> pParent, std::ifstream& file, std::shared_ptr<CShader> pShader, int* pnSkinnedMeshes)
{
	char pstrToken[64] = { '\0' };
	UINT nReads = 0;

	int nFrame = 0, nTextures = 0;

	std::shared_ptr<CGameObject> pGameObject = std::make_shared<CGameObject>();

	for (; ; )
	{
		::ReadStringFromFile(file, pstrToken);
		if (!strcmp(pstrToken, "<Frame>:"))
		{
			nFrame = ::ReadIntegerFromFile(file);
			nTextures = ::ReadIntegerFromFile(file);

			::ReadStringFromFile(file, pGameObject->m_strName);
		}
		else if (!strcmp(pstrToken, "<Transform>:"))
		{
			XMFLOAT3 xmf3Position, xmf3Rotation, xmf3Scale;
			XMFLOAT4 xmf4Rotation;
			file.read((char*)&xmf3Position, sizeof(float) * 3); 
			file.read((char*)&xmf3Rotation, sizeof(float) * 3); //Euler Angle
			file.read((char*)&xmf3Scale, sizeof(float) * 3);
			file.read((char*)&xmf4Rotation, sizeof(float) * 4); //Quaternion
		}
		else if (!strcmp(pstrToken, "<TransformMatrix>:"))
		{
			XMFLOAT4X4 xmf4x4Matrix;
			file.read((char*)&xmf4x4Matrix, sizeof(float) * 16);
			pGameObject->SetLocalMatrix(xmf4x4Matrix);
		}
		else if (!strcmp(pstrToken, "<Mesh>:"))
		{
			std::shared_ptr<CStandardMesh> pMesh = std::make_shared<CStandardMesh>(pd3dDevice, pd3dCommandList);
			pMesh->LoadMeshFromFile(pd3dDevice, pd3dCommandList, file);
			pGameObject->SetMesh(pMesh);
		}
		else if (!strcmp(pstrToken, "<SkinningInfo>:"))
		{
			if (pnSkinnedMeshes) (*pnSkinnedMeshes)++;

			std::shared_ptr<CSkinnedMesh> pSkinnedMesh = std::make_shared<CSkinnedMesh>(pd3dDevice, pd3dCommandList);
			pSkinnedMesh->LoadSkinInfoFromFile(pd3dDevice, pd3dCommandList, file);
			pSkinnedMesh->CreateShaderVariables(pd3dDevice, pd3dCommandList);

			::ReadStringFromFile(file, pstrToken); //<Mesh>:
			if (!strcmp(pstrToken, "<Mesh>:")) pSkinnedMesh->LoadMeshFromFile(pd3dDevice, pd3dCommandList, file);

			pGameObject->SetMesh(pSkinnedMesh);
		}
		else if (!strcmp(pstrToken, "<Materials>:"))
		{
			pGameObject->LoadMaterialsFromFile(pd3dDevice, pd3dCommandList, pParent, file, pShader);
		}
		else if (!strcmp(pstrToken, "<Children>:"))
		{
			int nChilds = ::ReadIntegerFromFile(file);
			if (nChilds > 0)
			{
				for (int i = 0; i < nChilds; i++)
				{
					std::shared_ptr<CGameObject> pChild = CGameObject::LoadFrameHierarchyFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pGameObject, file, pShader, pnSkinnedMeshes);
					if (pChild) pGameObject->SetChild(pChild);
#ifdef _WITH_DEBUG_FRAME_HIERARCHY
					TCHAR pstrDebug[256] = { 0 };
					_stprintf_s(pstrDebug, 256, "(Frame: %p) (Parent: %p)\n"), pChild, pGameObject);
					OutputDebugString(pstrDebug);
#endif
				}
			}
		}
		else if (!strcmp(pstrToken, "</Frame>"))
		{
			break;
		}
	}
	return(pGameObject);
}

std::shared_ptr<CLoadedModelInfo> CGameObject::LoadGeometryAndAnimationFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, const char* pstrFileName, std::shared_ptr<CShader> pShader)
{
	std::ifstream pInFile(pstrFileName, std::ios::binary);
	//::fopen_s(&pInFile, pstrFileName, "rb");
	//::rewind(pInFile);

	std::shared_ptr<CLoadedModelInfo> pLoadedModel = std::make_shared<CLoadedModelInfo>();

	char pstrToken[64] = { '\0' };

	for (; ; )
	{
		if (::ReadStringFromFile(pInFile, pstrToken))
		{
			if (!strcmp(pstrToken, "<Hierarchy>:"))
			{
				pLoadedModel->m_pModelRootObject = CGameObject::LoadFrameHierarchyFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, NULL, pInFile, pShader, &pLoadedModel->m_nSkinnedMeshes);
				::ReadStringFromFile(pInFile, pstrToken); //"</Hierarchy>"
			}
			else if (!strcmp(pstrToken, "<Animation>:"))
			{
				CGameObject::LoadAnimationFromFile(pInFile, pLoadedModel);
				pLoadedModel->PrepareSkinning();
			}
			else if (!strcmp(pstrToken, "</Animation>:"))
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

#ifdef _WITH_DEBUG_FRAME_HIERARCHY
	TCHAR pstrDebug[256] = { 0 };
	_stprintf_s(pstrDebug, 256, "Frame Hierarchy\n"));
	OutputDebugString(pstrDebug);

	CGameObject::PrintFrameInfo(pGameObject, NULL);
#endif

	return(pLoadedModel);
}

std::shared_ptr<CGameObject> CGameObject::FindFrame(std::string strFrameName)
{
	std::shared_ptr<CGameObject> pFrameObject;
	if (m_strName == strFrameName) return (shared_from_this());

	for (auto& pChild : m_pChilds) if (pFrameObject = pChild->FindFrame(strFrameName)) return(pFrameObject);

	return(NULL);
}

///////////////////////////////////////////////////////////////////////////////
//

CRotatingObject::CRotatingObject()
	: CGameObject()
{
}

CRotatingObject::~CRotatingObject()
{
}

void CRotatingObject::Initialize(ID3D12Device* pd3dDevice, ID3D12CommandList* pd3dCommandlist)
{
	CGameObject::Initialize(pd3dDevice, pd3dCommandlist);

	m_fRotationSpeed = 30.0f;
	m_xmf3RotationAxis = XMFLOAT3(0.0f, 1.0f, 0.0f);
}

std::shared_ptr<CRotatingObject> CRotatingObject::Create(ID3D12Device* pd3dDevice, ID3D12CommandList* pd3dCommandList)
{
	std::shared_ptr<CRotatingObject> pRotatingObject = std::make_shared<CRotatingObject>();
	pRotatingObject->Initialize(pd3dDevice, pd3dCommandList);

	return pRotatingObject;
}

void CRotatingObject::Update(float fTimeElapsed)
{
	Rotate(m_xmf3RotationAxis, m_fRotationSpeed * fTimeElapsed);
}

///////////////////////////////////////////////////////////////////////////////
//

CSkyBox::CSkyBox()
{
}

CSkyBox::~CSkyBox()
{
}

void CSkyBox::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	std::shared_ptr<CMesh> pSkyBoxMesh = std::make_shared<CSkyBoxMesh>(pd3dDevice, pd3dCommandList, 20.0f, 20.0f, 20.0f);
	SetMesh(pSkyBoxMesh);

	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	std::shared_ptr<CTexture> pSkyBoxTexture = std::make_shared<CTexture>(1, RESOURCE_TEXTURE_CUBE, 1);
	pSkyBoxTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"SkyBox/SkyBox_0.dds", RESOURCE_TEXTURE_CUBE, 0);
	// pSkyBoxTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"SkyBox/SkyBox_1.dds", RESOURCE_TEXTURE_CUBE, 0);

	std::shared_ptr<CShader> pSkyBoxShader = std::make_shared<CSkyBoxShader>();
	pSkyBoxShader->CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
	pSkyBoxShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	// pSkyBoxShader->CreateCbvSrvDescriptorHeaps(pd3dDevice, 0, 1);
	CScene::CreateShaderResourceViews(pd3dDevice, pSkyBoxTexture.get(), 0, ROOT_PARAMETER_SKYBOX);

	std::shared_ptr<CMaterial> pSkyBoxMaterial = std::make_shared<CMaterial>();
	pSkyBoxMaterial->SetTexture(pSkyBoxTexture);
	pSkyBoxMaterial->SetShader(pSkyBoxShader);

	m_ppMaterials.resize(1);
	SetMaterial(0, pSkyBoxMaterial);
}

std::shared_ptr<CSkyBox> CSkyBox::Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	std::shared_ptr<CSkyBox> pSkyBox = std::make_shared<CSkyBox>();
	pSkyBox->Initialize(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);

	return pSkyBox;
}

void CSkyBox::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if(pCamera)
	{
		XMFLOAT3 xmf3CameraPos = pCamera->GetPosition();
		SetPosition(xmf3CameraPos.x, xmf3CameraPos.y, xmf3CameraPos.z);
	}
	UpdateTransform();

	CGameObject::Render(pd3dCommandList, pCamera);
}

///////////////////////////////////////////////////////////////////////////////
//

CHeightMapTerrain::CHeightMapTerrain()
{
}

CHeightMapTerrain::~CHeightMapTerrain()
{
}

std::shared_ptr<CHeightMapTerrain> CHeightMapTerrain::Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color)
{
	std::shared_ptr<CHeightMapTerrain> pHeightMapTerrain = std::make_shared<CHeightMapTerrain>();
	pHeightMapTerrain->Initialize(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pFileName, nWidth, nLength, nBlockWidth, nBlockLength, xmf3Scale, xmf4Color);

	return pHeightMapTerrain;
}

void CHeightMapTerrain::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color)
{
	//지형에 사용할 높이 맵의 가로, 세로의 크기이다. 
	m_nWidth = nWidth;
	m_nLength = nLength;

	/*지형 객체는 격자 메쉬들의 배열로 만들 것이다.
	nBlockWidth, nBlockLength는 격자 메쉬 하나의 가로, 세로 크기이다.
	cxQuadsPerBlock, czQuadsPerBlock은 격자 메쉬의 가로 방향과 세로 방향 사각형의 개수이다.*/
	int cxQuadsPerBlock = nBlockWidth - 1;
	int czQuadsPerBlock = nBlockLength - 1;

	long cxBlocks = (nWidth - 1) / cxQuadsPerBlock;
	long czBlocks = (nLength - 1) / czQuadsPerBlock;

	//xmf3Scale는 지형을 실제로 몇 배 확대할 것인가를 나타낸다. 
	m_xmf3Scale = xmf3Scale;

	//지형에 사용할 높이 맵을 생성한다. 
	m_pHeightMapImage = std::make_shared<CHeightMapImage>(pFileName, nWidth, nLength, xmf3Scale);

	if (nWidth == nBlockWidth && nLength == nBlockLength) {
		std::shared_ptr<CMesh> pHeightMapGridMesh;
		pHeightMapGridMesh = std::make_shared<CHeightMapGridMesh>(pd3dDevice, pd3dCommandList, 0, 0, nBlockWidth, nBlockLength, xmf3Scale, xmf4Color, m_pHeightMapImage.get());
		SetMesh(pHeightMapGridMesh);
	}
	else
	{
		std::shared_ptr<CMesh> pHeightMapGridMesh;
		std::shared_ptr<CGameObject> pHeightMapGameObject;
		for (int z = 0, zStart = 0; z < czBlocks; z++)
		{
			for (int x = 0, xStart = 0; x < cxBlocks; x++)
			{
				pHeightMapGameObject = std::make_shared<CGameObject>();
				pHeightMapGameObject->MaterialResize(0);
				xStart = x * (nBlockWidth - 1);
				zStart = z * (nBlockLength - 1);
				pHeightMapGridMesh = std::make_shared<CHeightMapGridMesh>(pd3dDevice, pd3dCommandList, xStart, zStart, nBlockWidth, nBlockLength, xmf3Scale, xmf4Color, m_pHeightMapImage.get());
				pHeightMapGameObject->SetMesh(pHeightMapGridMesh);
				SetChild(pHeightMapGameObject);
			}
		}
	}
	//{

	//	//지형 전체를 표현하기 위한 격자 메쉬에 대한 포인터 배열을 생성한다. 
	//	CHeightMapGridMesh* pHeightMapGridMesh = NULL;
	//	//지형의 일부분을 나타내는 격자 메쉬를 생성하여 지형 메쉬에 저장한다. 
	//	pHeightMapGridMesh = new CHeightMapGridMesh(pd3dDevice, pd3dCommandList, 0,
	//		0, nBlockWidth, nBlockLength, xmf3Scale, xmf4Color, m_pHeightMapImage);

	//	SetMesh(pHeightMapGridMesh);
	//}

	//지형을 렌더링하기 위한 셰이더를 생성한다. 

#ifdef _WITH_TERRAIN_TESSELATION
	CTerrainTessellationShader* pShader = new CTerrainTessellationShader();
#else
	std::shared_ptr<CShader> pShader = std::make_shared<CTerrainShader>();
#endif
	pShader->CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
	pShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	SetShader(pShader);

	// 이 define 은 stdafx.h 에서 정의되어 있다.
	std::shared_ptr<CTexture> pTexture = std::make_shared<CTexture>(2, RESOURCE_TEXTURE2D, 2);
	pTexture->LoadTextureFromWICFile(pd3dDevice, pd3dCommandList, L"Image/Stone01.jpg", RESOURCE_TEXTURE2D, 0);
	pTexture->LoadTextureFromWICFile(pd3dDevice, pd3dCommandList, L"Image/Grass.jpg", RESOURCE_TEXTURE2D, 1);

	CScene::CreateShaderResourceViews(pd3dDevice, pTexture.get(), 0, ROOT_PARAMETER_STANDARD_TEXTURES);

	m_ppMaterials.resize(1);
	m_ppMaterials[0]->SetTexture(pTexture);
}

void CHeightMapTerrain::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	UpdateTransform();

	CGameObject::Render(pd3dCommandList, pCamera);
}

///////////////////////////////////////////////////////////////////////////////
//

CTexture::CTexture(int nTextures, UINT nTextureType, int nRootParameters)
{
	m_nTextures = nTextures;
	m_pd3dTextures.resize(m_nTextures);
	m_pd3dTextureUploadBuffers.resize(m_nTextures);
	m_strTextureNames.resize(m_nTextures);

	m_nResourceTypes.resize(m_nTextures);

	m_pdxgiBufferFormats.resize(m_nTextures);
	m_nBufferElements.resize(m_nTextures);
	m_nBufferStrides.resize(m_nTextures);

	m_d3dSrvGpuDescriptorHandles.resize(m_nTextures);

	m_nRootParameters = nRootParameters;
	m_nRootParameterIndices.resize(m_nRootParameters);
}

D3D12_SHADER_RESOURCE_VIEW_DESC CTexture::GetShaderResourceViewDesc(int nIndex)
{
	ID3D12Resource* pShaderResource = GetResource(nIndex);
	D3D12_RESOURCE_DESC d3dResourceDesc = pShaderResource->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc;
	d3dShaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	int nTextureType = GetTextureType(nIndex);
	switch (nTextureType)
	{
	case RESOURCE_TEXTURE2D: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize == 1)
	case RESOURCE_TEXTURE2D_ARRAY: //[]
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		d3dShaderResourceViewDesc.Texture2D.MipLevels = -1;
		d3dShaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.Texture2D.PlaneSlice = 0;
		d3dShaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;
	case RESOURCE_TEXTURE2DARRAY: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize != 1)
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		d3dShaderResourceViewDesc.Texture2DArray.MipLevels = -1;
		d3dShaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.Texture2DArray.PlaneSlice = 0;
		d3dShaderResourceViewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		d3dShaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;
		d3dShaderResourceViewDesc.Texture2DArray.ArraySize = d3dResourceDesc.DepthOrArraySize;
		break;
	case RESOURCE_TEXTURE_CUBE: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize == 6)
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		d3dShaderResourceViewDesc.TextureCube.MipLevels = 1;
		d3dShaderResourceViewDesc.TextureCube.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		break;
	case RESOURCE_BUFFER: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		d3dShaderResourceViewDesc.Format = m_pdxgiBufferFormats[nIndex];
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		d3dShaderResourceViewDesc.Buffer.FirstElement = 0;
		d3dShaderResourceViewDesc.Buffer.NumElements = m_nBufferElements[nIndex];
		d3dShaderResourceViewDesc.Buffer.StructureByteStride = 0;
		d3dShaderResourceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		break;
	}
	return(d3dShaderResourceViewDesc);
}

// Shader Variables

void CTexture::UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, int nParameterIndex, int nTextureIndex)
{
	pd3dCommandList->SetGraphicsRootDescriptorTable(m_nRootParameterIndices[nParameterIndex], m_d3dSrvGpuDescriptorHandles[nTextureIndex]);
}

void CTexture::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_nRootParameters == m_pd3dTextures.size())
	{
		for (int i = 0; i < m_nRootParameters; i++)
		{
			if (m_d3dSrvGpuDescriptorHandles[i].ptr && (m_nRootParameterIndices[i] != -1))
				pd3dCommandList->SetGraphicsRootDescriptorTable(m_nRootParameterIndices[i], m_d3dSrvGpuDescriptorHandles[i]);
		}
	}
	else
	{
		if (m_d3dSrvGpuDescriptorHandles[0].ptr) pd3dCommandList->SetGraphicsRootDescriptorTable(m_nRootParameterIndices[0], m_d3dSrvGpuDescriptorHandles[0]);
	}
}

// Load Texture

void CTexture::LoadTextureFromDDSFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::wstring strTextureName, UINT nResourceType, UINT nIndex)
{
	if (nIndex >= m_pd3dTextures.size()) {
		OutputDebugString(L"Texture index out of range\n");
		return;
	};

	m_nResourceTypes[nIndex] = nResourceType;
	m_strTextureNames[nIndex] = strTextureName;
	m_pd3dTextures[nIndex] = ::CreateTextureResourceFromDDSFile(pd3dDevice, pd3dCommandList, strTextureName.c_str(), m_pd3dTextureUploadBuffers[nIndex].GetAddressOf(), D3D12_RESOURCE_STATE_GENERIC_READ/*D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE*/);
	m_pd3dTextures[nIndex]->SetName(strTextureName.c_str());
}

void CTexture::LoadTextureFromWICFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::wstring strTextureName, UINT nResourceType, UINT nIndex)
{
	if (nIndex >= m_pd3dTextures.size()) {
		OutputDebugString(L"Texture index out of range\n");
		return;
	};

	m_nResourceTypes[nIndex] = nResourceType;
	m_strTextureNames[nIndex] = strTextureName;
	m_pd3dTextures[nIndex] = ::CreateTextureResourceFromWICFile(pd3dDevice, pd3dCommandList, strTextureName.c_str(), m_pd3dTextureUploadBuffers[nIndex].GetAddressOf(), D3D12_RESOURCE_STATE_GENERIC_READ/*D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE*/);
	m_pd3dTextures[nIndex]->SetName(strTextureName.c_str());
}

void CTexture::LoadBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nElements, UINT nStride, DXGI_FORMAT ndxgiFormat, UINT nIndex)
{
	if (nIndex >= m_pd3dTextures.size()) {
		OutputDebugString(L"Texture index out of range\n");
		return;
	};

	m_nResourceTypes[nIndex] = RESOURCE_BUFFER;
	m_nBufferElements[nIndex] = nElements;
	m_nBufferStrides[nIndex] = nStride;
	m_pdxgiBufferFormats[nIndex] = ndxgiFormat;
	m_pd3dTextures[nIndex] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pData, nElements * nStride, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dTextureUploadBuffers[nIndex]);
}

ComPtr<ID3D12Resource> CTexture::CreateTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, UINT nIndex, UINT nResourceType, UINT nWidth, UINT nHeight, UINT nElements, UINT nMipLevels, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS d3dResourceFlags, D3D12_RESOURCE_STATES d3dResourceStates, D3D12_CLEAR_VALUE* pd3dClearValue)
{
	if (nIndex >= m_pd3dTextures.size()) {
		OutputDebugString(L"Texture index out of range\n");
		return nullptr;
	};

	m_nResourceTypes[nIndex] = nResourceType;
	//m_pd3dTextures[nIndex] = ::CreateTexture2DResource(pd3dDevice, pd3dCommandList, nWidth, nHeight, nElements, nMipLevels, dxgiFormat, d3dResourceFlags, d3dResourceStates, pd3dClearValue);
	return(m_pd3dTextures[nIndex]);
}

///////////////////////////////////////////////////////////////////////////////
//

inline CMaterial::CMaterial(int nTextures)
{
	m_ppTextures.resize(nTextures);
	m_strTextureNames.resize(nTextures);
}

// Shader Variables

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
		if(pTexture) pTexture->UpdateShaderVariables(pd3dCommandList);
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
			ppTexture = std::make_shared <CTexture>(1, RESOURCE_TEXTURE2D, 1);
			(ppTexture)->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, pwstrTextureName, RESOURCE_TEXTURE2D, 0);

			CScene::CreateShaderResourceViews(pd3dDevice, ppTexture.get(), 0, nRootParameter);
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

///////////////////////////////////////////////////////////////////////////////
//

void CLoadedModelInfo::PrepareSkinning()
{
	int nSkinnedMesh = 0;
	m_ppSkinnedMeshes.resize(m_nSkinnedMeshes);
	m_pModelRootObject->FindAndSetSkinnedMesh(m_ppSkinnedMeshes, &nSkinnedMesh);

	for (int i = 0; i < m_nSkinnedMeshes; i++) m_ppSkinnedMeshes[i]->PrepareSkinning(m_pModelRootObject);
}

///////////////////////////////////////////////////////////////////////////////
//

CZombieObject::CZombieObject()
{
}

CZombieObject::~CZombieObject()
{
}

void CZombieObject::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CLoadedModelInfo> pModel, int nAnimationTracks)
{
	CGameObject::Initialize(pd3dDevice, pd3dCommandList);

	// Object Info
	static UINT nGameObjectID = 0;
	m_bActive = true;
	m_nObjectID = nGameObjectID++;

	m_strName = "Zombie_" + std::to_string(m_nObjectID);

	// Model Info
	std::shared_ptr<CLoadedModelInfo> pAngrybotModel = pModel;
	if (!pAngrybotModel) pAngrybotModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, "Model/FuzZombie.bin", NULL);
	SetChild(pAngrybotModel->m_pModelRootObject);

	m_pSkinnedAnimationController = std::make_shared<CAnimationController>(pd3dDevice, pd3dCommandList, nAnimationTracks, pAngrybotModel);

	m_pSkinnedAnimationController->SetTrackAnimationSet(0, 0);
	m_pSkinnedAnimationController->SetTrackAnimationSet(1, 1);
	m_pSkinnedAnimationController->SetTrackEnable(1, false);

	// Component
	std::shared_ptr<CRigidBody> pRigidBody = AddComponent<CRigidBody>(shared_from_this());
	pRigidBody->SetVelocity(XMFLOAT3(0.0f, -9.0f, 0.0f));

}

std::shared_ptr<CZombieObject> CZombieObject::Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CGameObject> pTerrain, std::shared_ptr<CLoadedModelInfo> pModel, int nAnimationTracks)
{
	std::shared_ptr<CZombieObject> pZombie = std::make_shared<CZombieObject>();
	pZombie->Initialize(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pModel, nAnimationTracks);
	pZombie->GetComponent<CRigidBody>()->SetTerrainUpdatedContext(pTerrain.get());

	return pZombie;
}

///////////////////////////////////////////////////////////////////////////////
//

void CAnimationController::AdvanceTime(float fElapsedTime, CGameObject* pRootGameObject)
{
	m_fTime += fElapsedTime;
	if (!m_pAnimationTracks.empty())
	{
#ifdef _WITH_OBJECT_TRANSFORM
		for (int j = 0; j < m_pAnimationSets->m_nBoneFrames; j++) m_pAnimationSets->m_ppBoneFrameCaches[j]->m_xmf4x4Local = Matrix4x4::Zero();
#else
		for (int j = 0; j < m_pAnimationSets->m_nBoneFrames; j++) m_pAnimationSets->m_ppBoneFrameCaches[j]->SetLocalMatrix(Matrix4x4::Zero());
#endif

		for (int k = 0; k < m_nAnimationTracks; k++)
		{
			if (m_pAnimationTracks[k].m_bEnable)
			{
				std::shared_ptr<CAnimationSet> pAnimationSet = m_pAnimationSets->m_pAnimationSets[m_pAnimationTracks[k].m_nAnimationSet];
				float fPosition = m_pAnimationTracks[k].UpdatePosition(m_pAnimationTracks[k].m_fPosition, fElapsedTime, pAnimationSet->m_fLength);
				for (int j = 0; j < m_pAnimationSets->m_nBoneFrames; j++)
				{
#ifdef _WITH_OBJECT_TRANSFORM
					XMFLOAT4X4 xmf4x4Transform = m_pAnimationSets->m_ppBoneFrameCaches[j]->m_xmf4x4Local;
#else
					XMFLOAT4X4 xmf4x4Transform = m_pAnimationSets->m_ppBoneFrameCaches[j]->GetLocalMatrix();
#endif
					XMFLOAT4X4 xmf4x4TrackTransform = pAnimationSet->GetSRT(j, fPosition);
					xmf4x4Transform = Matrix4x4::Add(xmf4x4Transform, Matrix4x4::Scale(xmf4x4TrackTransform, m_pAnimationTracks[k].m_fWeight));
#ifdef _WITH_OBJECT_TRANSFORM
					m_pAnimationSets->m_ppBoneFrameCaches[j]->m_xmf4x4Local = xmf4x4Transform;
#else
					m_pAnimationSets->m_ppBoneFrameCaches[j]->SetLocalMatrix(xmf4x4Transform);
#endif
				}
				m_pAnimationTracks[k].HandleCallback();
			}
		}
#ifdef _WITH_DEBUG_ANIMATION_UPDATE
#ifdef _WITH_OBJECT_TRANSFORM
		for (int j = 0; j < m_pAnimationSets->m_nBoneFrames; j++)
		{
			XMFLOAT4X4 xmf4x4Transform = m_pAnimationSets->m_ppBoneFrameCaches[j]->m_xmf4x4Local;

			// Print Matrix
			TCHAR pstrDebug[256] = { 0 };
			_stprintf_s(pstrDebug, 256, _T("Bone Frame %d\n"), j);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("%f %f %f %f\n"), xmf4x4Transform._11, xmf4x4Transform._12, xmf4x4Transform._13, xmf4x4Transform._14);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("%f %f %f %f\n"), xmf4x4Transform._21, xmf4x4Transform._22, xmf4x4Transform._23, xmf4x4Transform._24);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("%f %f %f %f\n"), xmf4x4Transform._31, xmf4x4Transform._32, xmf4x4Transform._33, xmf4x4Transform._34);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("%f %f %f %f\n"), xmf4x4Transform._41, xmf4x4Transform._42, xmf4x4Transform._43, xmf4x4Transform._44);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("----------------\n"));
		}
#else
		for (int j = 0; j < m_pAnimationSets->m_nBoneFrames; j++)
		{
			XMFLOAT4X4 xmf4x4Transform = m_pAnimationSets->m_ppBoneFrameCaches[j]->GetLocalMatrix();

			// Print Matrix
			TCHAR pstrDebug[256] = { 0 };
			_stprintf_s(pstrDebug, 256, _T("Bone Frame %d\n"), j);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("%f %f %f %f\n"), xmf4x4Transform._11, xmf4x4Transform._12, xmf4x4Transform._13, xmf4x4Transform._14);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("%f %f %f %f\n"), xmf4x4Transform._21, xmf4x4Transform._22, xmf4x4Transform._23, xmf4x4Transform._24);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("%f %f %f %f\n"), xmf4x4Transform._31, xmf4x4Transform._32, xmf4x4Transform._33, xmf4x4Transform._34);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("%f %f %f %f\n"), xmf4x4Transform._41, xmf4x4Transform._42, xmf4x4Transform._43, xmf4x4Transform._44);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("----------------\n"));
	}
#endif // _WITH_OBJECT_TRANSFORM/

#endif // _WITH_DEBUG_ANIMATION_UPDATE/

		pRootGameObject->UpdateTransform(NULL);

		OnRootMotion(pRootGameObject);
		OnAnimationIK(pRootGameObject);
	}
}

///////////////////////////////////////////////////////////////////////////////
//

CZombieAnimationController::CZombieAnimationController(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, int nAnimationTracks, std::shared_ptr<CLoadedModelInfo> pModel)
	: CAnimationController(pd3dDevice, pd3dCommandList, nAnimationTracks, pModel)
{
}

CZombieAnimationController::~CZombieAnimationController()
{
}

///////////////////////////////////////////////////////////////////////////////
//

CCubeObject::CCubeObject()
	: CRotatingObject()
{

}

CCubeObject::~CCubeObject()
{
}

void CCubeObject::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	// Mesh
	std::shared_ptr<CCubeMesh> pCubeMesh = std::make_shared<CCubeMesh>(pd3dDevice, pd3dCommandList, 1.0f, 1.0f, 1.0f);
	SetMesh(pCubeMesh);

	// Material
	std::shared_ptr<CMaterial> pMaterial = std::make_shared<CMaterial>();
	pMaterial->SetStandardShader();
	SetMaterial(0, pMaterial);
}

std::shared_ptr<CCubeObject> CCubeObject::Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	std::shared_ptr<CCubeObject> pCube = std::make_shared<CCubeObject>();
	pCube->Initialize(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);

	return pCube;
}
