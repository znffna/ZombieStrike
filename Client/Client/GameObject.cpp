///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// GameObject.cpp : GameObject 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#include "GameObject.h"
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "Transform.h"
#include "Rigidbody.h"
#include "Collider.h"

#include "Scene.h"

#include "GameFramework.h"

UINT CGameObject::m_nObjectIDCounter = 0;

CGameObject::CGameObject()
{
	Init();
}

CGameObject::CGameObject(const std::string& strName)
{
	Init();
	SetName(strName);
}

void CGameObject::ClearMemberVariables()
{
	m_pMesh.reset();
	m_ppMaterials.clear();
	m_pComponents.clear();
	m_pChilds.clear();
}

CGameObject::~CGameObject()
{
	// Release Child Object
	for (auto& pChild : m_pChilds) pChild.reset();
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

	// Debug Output
#ifdef _DEBUG
	std::string debugoutput = "Object Name: " + GetName() + " has Destroyed\n";
	OutputDebugStringA(debugoutput.c_str());
#endif // _DEBUG
}

void CGameObject::Init()
{
	// Object Info
	m_bActive = true;
	m_nObjectID = m_nObjectIDCounter++;

	// Default Name
	m_strName = "Default_GameObject";
	//m_strName = "GameObject_" + std::to_string(m_nObjectID);
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
		if (m_ppMaterials.empty())
		{
			// Render Mesh
			m_pMesh->Render(pd3dCommandList);
		}
	}

	// Render Child Object
	for (auto& pChild : m_pChilds)
	{
		pChild->Render(pd3dCommandList, pCamera);
	}
}

void CGameObject::IsCollided(std::shared_ptr<CGameObject>& pOther)
{
	if (auto pCollider = GetComponent<CCollider>()) {
		if (auto pCollider2 = pOther->GetComponent<CCollider>()) {
			if (pCollider->IsCollided(pCollider2)) {
				OnCollision(pOther);
				pOther->OnCollision(shared_from_this());
			}

			for (auto& pChild : m_pChilds) {
				if (pCollider->IsCollided(pChild->GetComponent<CCollider>())) {
					OnCollision(pChild);
					pChild->OnCollision(shared_from_this());
				}
			}
		}
	}
}

void CGameObject::IsCollided(std::shared_ptr<CGameObject> pGameObject1, std::shared_ptr<CGameObject> pGameObject2)
{
	if (auto pCollider = pGameObject1->GetComponent<CCollider>()) {
		if (auto pCollider2 = pGameObject2->GetComponent<CCollider>()) {
			if (pCollider->IsCollided(pCollider2)) {
				pGameObject1->OnCollision(pGameObject2);
				pGameObject2->OnCollision(pGameObject1);
			}
		}
	}
}

void CGameObject::OnCollision(std::shared_ptr<CGameObject> pGameObject)
{
	// Collision Event
	if (auto pRigidBody = GetComponent<CRigidBody>()) {
		pRigidBody->OnCollision(pGameObject);
	}
}

// Mesh
void CGameObject::SetMesh(std::shared_ptr<CMesh> pMesh)
{
	m_pMesh = pMesh; 
	if (m_pMesh->GetType() && VERTEXT_POSITION) {
		auto pCollider = AddComponent<DefaultCollider>(shared_from_this());
		pCollider->SetCollider(m_pMesh);
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

bool CGameObject::CloneByModel(std::string& strModelName, std::shared_ptr<CGameObject>& pGameObject)
{
	if (auto pModel = CScene::GetResourceManager().GetModelInfo(strModelName)) {
		pGameObject->GetResourcesAndComponents(pModel->m_pModelRootObject);
		//pGameObject->SetChild(pModel->m_pModelRootObject);
		return true;
	}
	return false;
}

std::shared_ptr<CGameObject> CGameObject::LoadFrameHierarchyFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CGameObject> pParent, std::ifstream& file, std::shared_ptr<CShader> pShader, int* pnSkinnedMeshes, int nDepth)
{
	char pstrToken[64] = { '\0' };
	UINT nReads = 0;

	int nFrame = 0, nTextures = 0;

	std::shared_ptr<CGameObject> pGameObject = std::make_shared<CGameObject>();
	pGameObject->SetParent(pParent);

	bool isGetModel = false;

	for (; ; )
	{
		::ReadStringFromFile(file, pstrToken);
		if (!strcmp(pstrToken, "<Frame>:"))
		{
			nFrame = ::ReadIntegerFromFile(file);
			nTextures = ::ReadIntegerFromFile(file);

			::ReadStringFromFile(file, pGameObject->m_strName);

#ifdef _DEBUG
			pGameObject->nLoadFrames = nFrame;
			std::string debugoutput = pGameObject->m_strName + " " + std::to_string(nFrame)+ ", Stack Depth = " + std::to_string(nDepth) + "\n";
			OutputDebugStringA(debugoutput.c_str());
#endif
			// Test용
			isGetModel = CloneByModel(pGameObject->m_strName, pGameObject);
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
				pGameObject->m_pChilds.reserve(nChilds); 
				for (int i = 0; i < nChilds; i++)
				{
					std::shared_ptr<CGameObject> pChild = CGameObject::LoadFrameHierarchyFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pGameObject, file, pShader, pnSkinnedMeshes, nDepth + 1);
					if (pChild) pGameObject->SetChild(pChild);

#ifdef _WITH_DEBUG_FRAME_HIERARCHY
					TCHAR pstrDebug[256] = { 0 };
					_stprintf_s(pstrDebug, 256, "(Frame: %p) (Parent: %p)\n"), pChild, pGameObject);
					OutputDebugString(pstrDebug);
#endif
				}
			}
		}
		else if (!strcmp(pstrToken, "<ModelName>"))
		{
			std::string strModelName;
			::ReadStringFromFile(file, strModelName);
			//if (strModelName == pGameObject->m_strName) continue;
			if (isGetModel) continue;
			CloneByModel(strModelName, pGameObject);
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
	if (!pInFile.is_open()) return nullptr;

	//::fopen_s(&pInFile, pstrFileName, "rb");
	//::rewind(pInFile);

	std::shared_ptr<CLoadedModelInfo> pLoadedModel = std::make_shared<CLoadedModelInfo>();

	char pstrToken[500] = { '\0' };

	for (; ; )
	{
		if (::ReadStringFromFile(pInFile, pstrToken))
		{
			if (!strcmp(pstrToken, "<Hierarchy>:"))
			{
				pLoadedModel->m_pModelRootObject = CGameObject::LoadFrameHierarchyFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, NULL, pInFile, pShader, &pLoadedModel->m_nSkinnedMeshes, 0);
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

		m_pChilds.reserve(cxBlocks * czBlocks); //지형을 표현하기 위한 격자 메쉬의 개수이다.

		for (int z = 0, zStart = 0; z < czBlocks; z++)
		{
			for (int x = 0, xStart = 0; x < cxBlocks; x++)
			{
				pHeightMapGameObject = std::make_shared<CGameObject>("HeightMapSub");
				pHeightMapGameObject->MaterialResize(0);
				xStart = x * (nBlockWidth - 1);
				zStart = z * (nBlockLength - 1);
				pHeightMapGridMesh = std::make_shared<CHeightMapGridMesh>(pd3dDevice, pd3dCommandList, xStart, zStart, nBlockWidth, nBlockLength, xmf3Scale, xmf4Color, m_pHeightMapImage.get());
				pHeightMapGameObject->SetMesh(pHeightMapGridMesh);
				SetChild(pHeightMapGameObject);

				std::string debugoutput =
					pHeightMapGameObject->GetName()
					+ " " + std::to_string(xStart) 
					+ ", " + std::to_string(zStart) 
					+ ", cxBlocks = " + std::to_string(cxBlocks)
					+ ", czBlocks = " + std::to_string(czBlocks)
					+ "\n";
				OutputDebugStringA(debugoutput.c_str());
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

std::shared_ptr<CHeightMapTerrain> CHeightMapTerrain::InitializeByBinary(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, LPCTSTR pBinFileName, LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color)
{
	std::shared_ptr<CHeightMapTerrain> pHeightMapTerrain = std::make_shared<CHeightMapTerrain>();

	//지형에 사용할 높이 맵의 가로, 세로의 크기이다. 
	pHeightMapTerrain->m_nWidth = nWidth;
	pHeightMapTerrain->m_nLength = nLength;

	/*지형 객체는 격자 메쉬들의 배열로 만들 것이다.
	nBlockWidth, nBlockLength는 격자 메쉬 하나의 가로, 세로 크기이다.
	cxQuadsPerBlock, czQuadsPerBlock은 격자 메쉬의 가로 방향과 세로 방향 사각형의 개수이다.*/
	int cxQuadsPerBlock = nBlockWidth - 1;
	int czQuadsPerBlock = nBlockLength - 1;

	long cxBlocks = (nWidth - 1) / cxQuadsPerBlock;
	long czBlocks = (nLength - 1) / czQuadsPerBlock;

	//xmf3Scale는 지형을 실제로 몇 배 확대할 것인가를 나타낸다. 
	pHeightMapTerrain->m_xmf3Scale = xmf3Scale;

	//지형에 사용할 높이 맵을 생성한다. 
	pHeightMapTerrain->m_pHeightMapImage = std::make_shared<CHeightMapImage>(pFileName, nWidth, nLength, xmf3Scale);

	{
		std::ifstream in(pBinFileName, std::ios::binary);
		if (!in.is_open()) return nullptr;

		UINT vertexCount = 0;
		UINT indexCount = 0;

		in.read(reinterpret_cast<char*>(&vertexCount), sizeof(UINT));
		in.read(reinterpret_cast<char*>(&indexCount), sizeof(UINT));

		std::vector<CTerrainVertex> outVertices;
		std::vector<UINT> outIndices;

		outVertices.resize(vertexCount);
		outIndices.resize(indexCount);

		in.read(reinterpret_cast<char*>(outVertices.data()), vertexCount * sizeof(CTerrainVertex));
		in.read(reinterpret_cast<char*>(outIndices.data()), indexCount * sizeof(UINT));

		in.close();

		std::cout << "Import successful.\n";
		std::cout << "Vertices: " << vertexCount << ", Indices: " << indexCount << "\n";

		std::shared_ptr<CMesh> pHeightMapGridMesh;
		pHeightMapGridMesh = std::make_shared<CHeightMapGridMesh>(pd3dDevice, pd3dCommandList, outVertices, outIndices);
		pHeightMapTerrain->SetMesh(pHeightMapGridMesh);

		pHeightMapTerrain->m_pVertices = outVertices;
		pHeightMapTerrain->m_pIndices = outIndices;

		pHeightMapTerrain->isBinary = true;
	}


#ifdef _WITH_TERRAIN_TESSELATION
	CTerrainTessellationShader* pShader = new CTerrainTessellationShader();
#else
	std::shared_ptr<CShader> pShader = std::make_shared<CTerrainShader>();
#endif
	pShader->CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
	pShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	pHeightMapTerrain->SetShader(pShader);

	// 이 define 은 stdafx.h 에서 정의되어 있다.
	std::shared_ptr<CTexture> pTexture = std::make_shared<CTexture>(2, RESOURCE_TEXTURE2D, 2);
	pTexture->LoadTextureFromWICFile(pd3dDevice, pd3dCommandList, L"Image/Stone01.jpg", RESOURCE_TEXTURE2D, 0);
	pTexture->LoadTextureFromWICFile(pd3dDevice, pd3dCommandList, L"Image/Grass.jpg", RESOURCE_TEXTURE2D, 1);

	CScene::CreateShaderResourceViews(pd3dDevice, pTexture.get(), 0, ROOT_PARAMETER_STANDARD_TEXTURES);

	pHeightMapTerrain->m_ppMaterials.resize(1);
	pHeightMapTerrain->m_ppMaterials[0]->SetTexture(pTexture);

	return pHeightMapTerrain;
}

void CHeightMapTerrain::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	UpdateTransform();

	CGameObject::Render(pd3dCommandList, pCamera);
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
