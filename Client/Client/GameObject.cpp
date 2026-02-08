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

CGameObject::CGameObject()	
{
	m_pTransform = std::make_unique<CTransform>(this);
}

CGameObject::CGameObject(const std::string& strName)
{
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

	// Release Shader Variables
	ReleaseShaderVariables();

	// Release Components
	m_pTransform.reset();
	m_pComponents.clear();

	// Debug Output
#ifdef _DEBUG
	/*std::string debugoutput = "Object Name: " + GetName() + " has Destroyed\n";
	OutputDebugStringA(debugoutput.c_str());*/
#endif // _DEBUG

}

void CGameObject::Initialize()
{
	
}

void CGameObject::DeepCopyFromGameObject(CGameObject* rhs)
{
	// 복사 할당 연산자 호출 (ID는 복제되면 안된다)
	//*this = *rhs.get();

	// Object Info
	m_strName = rhs->m_strName;

	/// Resource Copy (shallow copy)
	// Copy Mesh 
	if (rhs->m_pMesh) m_pMesh = rhs->m_pMesh;

	// Copy Materials 
	m_ppMaterials = rhs->m_ppMaterials;

	/// Components Copy (Deep Copy)
	// Copy Transform
	m_bPitchLock = rhs->m_bPitchLock;
	m_bYawLock = rhs->m_bYawLock;
	m_bRollLock = rhs->m_bRollLock;
	*m_pTransform = *rhs->m_pTransform;

	// Copy Components (owner-aware / assignment patterns)
	for (auto& pComponent : rhs->m_pComponents)
	{
		auto prawptr = pComponent.get();
		auto pclone = pComponent->Clone(this);
		m_pComponents.push_back(std::move(pclone));
	}

	// Copy Childs
	for (auto& pChild : rhs->m_pChilds)
	{
		auto pnewChild = std::make_unique<CGameObject>();
		pnewChild->DeepCopyFromGameObject(pChild.get());
		SetChild(std::move(pnewChild)); 
	}
}

bool CGameObject::IsGPUInitialized() 
{
	if (m_bInitialized) return true;

	if(m_pMesh)
	{
		if(!m_pMesh->IsGPUInitialized())
		{
			return false;
		}
	}

	for(auto& pComponent : m_pComponents)
	{
		if(!pComponent->IsGPUInitialized())
		{
			return false;
		}
	}

	for(auto& pChild : m_pChilds)
	{
		if(!pChild->IsGPUInitialized())
		{
			return false;
		}
	}

	m_bInitialized = true;
	return true;
}

void CGameObject::SetName(const std::string& strName)
{
	if (strName.length() > 0)
	{
		m_strName = strName;
	}
	else
	{
		m_strName = GetDefaultName() + "_" + std::to_string(m_nID);
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
		cRigidbody->SetVelocity(Vector3::ScalarProduct(xmf3Direction, fDistance));	// 여기는 초당속도로 갱신시키면 된다.
		//cRigidbody->PrintVelocity();
	}
	else
	{
		// 물리 엔진을 사용하지 않는 경우
		xmf3Direction = Vector3::Normalize(xmf3Direction);
		xmf3Direction = Vector3::ScalarProduct(xmf3Direction, fDistance * deltaTime);
		Move(xmf3Direction);
	}
}

void CGameObject::SetRotationAxisLock(bool bPitchLock, bool bYawLock, bool bRollLock)
{
	m_bPitchLock = bPitchLock;
	m_bYawLock = bYawLock;
	m_bRollLock = bRollLock;
}

void CGameObject::Rotate(float fPitch, float fYaw, float fRoll)
{
	if (m_bPitchLock) fPitch = 0.0f;
	if (m_bYawLock) fYaw = 0.0f;
	if (m_bRollLock) fRoll = 0.0f;

	m_pTransform->Rotate(fPitch, fYaw, fRoll); 
}

void CGameObject::UpdateTransform(const DirectX::XMFLOAT4X4* xmf4x4ParentMatrix)
{ 
	m_pTransform->UpdateTransform(xmf4x4ParentMatrix); 

	auto pColliders = GetComponents<CCollider>();
	if (pColliders.size())
	{
		XMFLOAT4X4 xmf4x4WorldMatrix = m_pTransform->GetWorldMatrix();
		for (auto& pCollider : pColliders)
		{
			pCollider->UpdateCollider(xmf4x4WorldMatrix);
		}
	}

	for (auto& pChild : m_pChilds) pChild->UpdateTransform(GetWorldMatrix());
}

void CGameObject::UpdateTransform(const DirectX::XMFLOAT4X4& xmf4x4ParentMatrix)
{
	UpdateTransform(&xmf4x4ParentMatrix);
}

void CGameObject::UpdateTransform(std::shared_ptr<CGameObject>& pGameobject)
{
	UpdateTransform(pGameobject->GetWorldMatrix());
}

void CGameObject::Update(float fTimeElapsed)
{
	if (GetLayer() == GAMEOBJECT_LAYER::LAYER_ENVIRONMENT)
	{
		// UI Layer는 Update 하지 않음
		return;
	}

	// Component Update 
	// TODO : 순서좀 생각해야 될 듯?
	for (auto& pComponent : m_pComponents)
	{
		pComponent->Update(fTimeElapsed);
	}

	OnPrepareRender();

	if (auto pSkinnedAnimationController = GetComponent<CAnimationController>()) pSkinnedAnimationController->AdvanceTime(fTimeElapsed, this);

	for (auto& pChild : m_pChilds) pChild->Update(fTimeElapsed);

	UpdateBBCache();
}

void CGameObject::UpdateBBCache()
{
	// 부모가 없을 경우, 모든 OBB를 Copy
	if (!m_pParent)
	{
		// 기존 Collider 파괴
		m_pCachesColliders.clear();

		// 현재 모든 자식내 Collider를 가져옴
		std::vector<CCollider*> pCachesColliders;
		GetComponentsInChildren<CCollider>(pCachesColliders);

		// Pointer 가아닌 실제 Value를 복사
		std::vector<CCollider*> pColliders;
		for (auto& pCollider : pCachesColliders)
		{
			pColliders.push_back(pCollider);
		}

		m_pCachesColliders = std::move(pColliders);
	}
}

void CGameObject::OnCollision(CGameObject* pOther, CCollider* pColliderA, CCollider* pColliderB)
{
	// Collision Event
	if (nullptr == pColliderA) return;

	auto rigidBody = GetComponent<CRigidBody>();
	auto pOtherRigidBody = pOther->GetComponent<CRigidBody>();
	
	/*{
		std::string debugoutput = "Collision Root Object Occured: " + GetName() + " - " + pOther->GetName() + " / ";
		debugoutput += "Collision Collider Object Occured: " + pColliderA->gameObject->GetName() + " - " + pColliderB->gameObject->GetName() + "\n";
		OutputDebugStringA(debugoutput.c_str());
	}*/

	// 최소 거리 측정
	//XMFLOAT3 mtv = pColliderA->GetCorrectionVector(pColliderB);
	XMFLOAT3 mtv{0,0,0};

	// 충돌 Normal을 통한 Y축 보정 추가
	float yAngle = mtv.y / Vector3::Length(mtv);
	if (yAngle > 0.7f) // y축과 mtv사이의 각도 X일때 cos(X) > 0.7f 경우를 의미.
	{
		mtv = XMFLOAT3(0.0f, mtv.y, 0.0f);
	}

	// 계단처리 수행
	// TODO : Model의 Extend * 0.66f보다 MTV의 Y가 작을경우, 이는 계단으로 판단하고, X, Z축 MTV값을 없앤다.


	if (rigidBody)
	{
		if (pOtherRigidBody)
		{
			mtv = Vector3::ScalarProduct(mtv, 0.5f);
			rigidBody->ApplyCorrection(mtv);
			pOtherRigidBody->ApplyCorrection(Vector3::ScalarProduct(mtv, -0.5f));
		}	
		else {
			rigidBody->ApplyCorrection(mtv);
		}

		// Cache Collider 갱신
		/*for (auto& pCollider : m_pCachesColliders) {
			pCollider->Move(mtv);
		}*/
	}
}

BoundingBox CGameObject::GetMergedCollider() const
{
	std::vector<CCollider*> pColliders;
	GetComponentsInChildren<CCollider>(pColliders);

	BoundingBox mergedBoundingBox{};
	for (auto& pCollider : pColliders)
	{
		BoundingBox colliderAABB = pCollider->GetBroadPhaseAABB();
		BoundingBox::CreateMerged(mergedBoundingBox, mergedBoundingBox, colliderAABB);
	}

	return mergedBoundingBox;
}

BoundingBox CGameObject::GetMergedMeshBound(BoundingBox* pVolume)
{
	if (nullptr == pVolume)
	{
		BoundingBox boundingBox{XMFLOAT3{0.0f,0.0f,0.0f}, XMFLOAT3{0.0f,0.0f,0.0f}};

		if (m_pMesh)
		{
			boundingBox = m_pMesh->GetBoundingBox(m_pTransform->GetWorldMatrix());
			//BoundingBox::CreateMerged(boundingBox, boundingBox, m_pMesh->GetBoundingBox(m_pTransform->GetWorldMatrix()));
		}

		for (auto& pChild : m_pChilds)
		{
			pChild->GetMergedMeshBound(&boundingBox);
		}

		return boundingBox;
	}
	else {
		if (m_pMesh)
		{
			if (pVolume->Extents.x == 0.0f && 
				pVolume->Extents.y == 0.0f &&
				pVolume->Extents.z == 0.0f
				)
			{
				*pVolume = m_pMesh->GetBoundingBox(m_pTransform->GetWorldMatrix());
			}
			else
			{
				BoundingBox::CreateMerged(*pVolume, *pVolume, m_pMesh->GetBoundingBox(m_pTransform->GetWorldMatrix()));
			}
		}

		for (auto& pChild : m_pChilds)
		{
			pChild->GetMergedMeshBound(pVolume);
		}

		return *pVolume;
	}
}

void CGameObject::UpdateLocalBoundingBox(const XMFLOAT4X4& pParentTransform)
{
	// TODO : m_pModelCollider의 LocalBoundingBox를 업데이트
	// 이때 업데이트는 부모의 Inverse를 WorldMatrix에 적용해 오직 현 Object space를 기준으로 한 local 좌표로 얻는다.

	for (auto& pChild : m_pChilds)
	{
		pChild->UpdateLocalBoundingBox(Matrix4x4::Inverse(m_pTransform->GetWorldMatrix()));
	}
}

void CGameObject::OnPrepareRender()
{
	if (auto pCollider = GetComponent<CCollider>())
	{
		pCollider->UpdateCollider(GetWorldMatrix());
	}
}

void CGameObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite)
{
	if (false == m_bActive) return;
	if (false == IsGPUInitialized()) return;

	// Render Object

	// Skinned Animation Update / Render
	if (auto panimationcontroller = GetComponent<CAnimationController>()) {
		panimationcontroller->UpdateShaderVariables(pd3dCommandList);
		panimationcontroller->m_pModelRootObject->Render(pd3dCommandList, pCamera, bDepthWrite);
	}

	if (m_pMesh) {
		// Set Shader Variables
		UpdateShaderVariables(pd3dCommandList); // GameObject Matrix Update

		for (int i = 0; i < m_ppMaterials.size(); ++i)
		{
			std::shared_ptr<CMaterial>& pMaterial = m_ppMaterials[i];
			if (pMaterial)
			{
				// Set Pipeline State
				if (pMaterial->m_pShader) {
					//if (pMaterial->m_pShader->GetAllowShadow() == false) continue; // 그림자 허용 여부 확인
					pMaterial->m_pShader->OnPrepareRender(pd3dCommandList, 0, bDepthWrite); // Render(pd3dCommandList, pCamera);
				}
				// Material Update
				if (!bDepthWrite) pMaterial->UpdateShaderVariables(pd3dCommandList);
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

	if (g_bRenderCollider && !bDepthWrite) {
		RenderCollider(pd3dCommandList);
	}

	// Render Child Object
	for (auto& pChild : m_pChilds)
	{
		pChild->Render(pd3dCommandList, pCamera, bDepthWrite);
	}
}

void CGameObject::RenderCollider(ID3D12GraphicsCommandList* pd3dCommandList)
{
	auto pColliders = GetComponents<CCollider>();

	for (auto pCollider : pColliders)
	{
		// Update Shader Variables
		XMFLOAT4X4 xmf4x4World;

		xmf4x4World = pCollider->GetColliderMatrix();
		XMStoreFloat4x4(&xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&xmf4x4World)));

		pd3dCommandList->SetGraphicsRoot32BitConstants(ROOT_PARAMETER_OBJECT, 16, &xmf4x4World, 0);

		// Use Collider Shader
		CMaterial::m_pColliderShader->OnPrepareRender(pd3dCommandList, 0, false);

		CResourceManager::Instance().GetMesh("Cube")->Render(pd3dCommandList);
	}
}

// Mesh
void CGameObject::SetMesh(std::shared_ptr<CMesh> pMesh)
{
	m_pMesh = pMesh; 
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

	// Component Shader Variables
	for(auto& pComponent : m_pComponents)
	{
		// Component Shader Variables
		pComponent->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	}

	for(auto& pChild : m_pChilds)
	{
		pChild->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	}
}

void CGameObject::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
#ifdef _USE_OBJECT_MATERIAL_CBV
	// Update Constant Buffer
	//m_pcbMappedObject->m_xmf4x4World = m_xmf4x4World; // DirectX는 행렬을 전치해서 셰이더에 적용해야 한다.
	XMStoreFloat4x4(&m_pcbMappedObject->m_xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));

#ifdef _DEBUG
	// Debug Output
	std::wstring DebugString = L"GameObject [" + std::to_wstring(m_nID) + L"] - position ("
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
	pd3dCommandList->SetGraphicsRoot32BitConstants(ROOT_PARAMETER_OBJECT, 4, &m_xmf4Color, 16);
}

void CGameObject::ReleaseShaderVariables()
{
#ifdef _USE_OBJECT_MATERIAL_CBV
	if (m_pd3dcbGameObject) m_pd3dcbGameObject->Unmap(0, nullptr);
	m_pd3dcbGameObject.Reset();
#endif // _USE_OBJECT_MATERIAL_CBV
}

void CGameObject::CollectShaderVariables()
{
	if (m_pMesh)
	{
		CResourceManager::Instance().RegisterMeshUpload(m_pMesh.get());
	}

	for (auto& pMaterial : m_ppMaterials)
	{
		CResourceManager::Instance().RegisterMaterialUpload(pMaterial.get());
	}

	for(auto& pComponent : m_pComponents)
	{
		// Component Shader Variables
		//pComponent->CollectShaderVariables();
	}

	for (auto& pChild : m_pChilds)
	{
		pChild->CollectShaderVariables();
	}
}

void CGameObject::ReleaseUploadBuffers()
{
	if (m_pMesh)
	{
		m_pMesh->ReleaseUploadBuffers();
	}

	if (false == m_ppMaterials.empty()) {
		for (UINT i = 0; i < (UINT)m_ppMaterials.size(); i++)
		{
			if (m_ppMaterials[i]) m_ppMaterials[i]->ReleaseUploadBuffers();
		}
	}
}

std::shared_ptr<CTexture> CGameObject::FindReplicatedTexture(const std::wstring pstrTextureName)
{
	std::string strTextureName = to_string(pstrTextureName);
	for (UINT i = 0; i < (UINT)m_ppMaterials.size(); i++)
	{
		if (m_ppMaterials[i])
		{
			for (UINT j = 0; j < m_ppMaterials[i]->m_nTextures; j++)
			{
				if (m_ppMaterials[i]->m_ppTextures[j])
				{
					if (m_ppMaterials[i]->m_strTextureNames[j] == strTextureName) return(m_ppMaterials[i]->m_ppTextures[j]);
					// if (!_tcsncmp(m_ppMaterials[i]->m_strTextureNames[j], pstrTextureName, _tcslen(pstrTextureName))) return(m_ppMaterials[i]->m_ppTextures[j]);
				}
			}
		}
	}
	for (auto& pChild : m_pChilds) if (auto pTexture = pChild->FindReplicatedTexture(pstrTextureName)) return(pTexture);

	return(NULL);
}

void CGameObject::FindAndSetSkinnedMesh(std::vector<std::shared_ptr<CSkinnedMesh>>& ppSkinnedMeshes, int* pnSkinnedMesh)
{
	if (m_pMesh && (m_pMesh->GetType() & VERTEXT_BONE_INDEX_WEIGHT)) ppSkinnedMeshes[(*pnSkinnedMesh)++] = std::dynamic_pointer_cast<CSkinnedMesh>(m_pMesh);

	for (auto& pChild : m_pChilds) pChild->FindAndSetSkinnedMesh(ppSkinnedMeshes, pnSkinnedMesh);
}

void CGameObject::LoadMaterialsFromFile(std::ifstream& File, std::shared_ptr<CShader> pShader)
{
	char pstrToken[64] = { '\0' };
	int nMaterial = 0;
	UINT nReads = 0;

	int nMaterials = ReadIntegerFromFile(File);

	m_ppMaterials.resize(nMaterials);

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

			// 실제 Material Upload 등록
			CResourceManager::Instance().RegisterMaterialUpload(pMaterial.get());
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
			// TODO : 여기에선 순수 텍스처 이름만 읽어오고, 텍스처 로드는 나중에 수행하도록 변경 필요
			pMaterial->LoadTextureFromFile(MATERIAL_ALBEDO_MAP, File);
			// pMaterial->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_ALBEDO_MAP, ROOT_PARAMETER_ALBEDO_TEXTURE, pMaterial->m_strTextureNames[0], (pMaterial->m_ppTextures[0]), pParent, File, pShader);
		}
		else if (!strcmp(pstrToken, "<SpecularMap>:"))
		{
			m_ppMaterials[nMaterial]->LoadTextureFromFile(MATERIAL_SPECULAR_MAP, File);
			//m_ppMaterials[nMaterial]->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_SPECULAR_MAP, ROOT_PARAMETER_SPECULAR_TEXTURE, pMaterial->m_strTextureNames[1], (pMaterial->m_ppTextures[1]), pParent, File, pShader);
		}
		else if (!strcmp(pstrToken, "<NormalMap>:"))
		{
			m_ppMaterials[nMaterial]->LoadTextureFromFile(MATERIAL_NORMAL_MAP, File);
			//m_ppMaterials[nMaterial]->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_NORMAL_MAP, ROOT_PARAMETER_NORMAL_TEXTURE, pMaterial->m_strTextureNames[2], (pMaterial->m_ppTextures[2]), pParent, File, pShader);
		}
		else if (!strcmp(pstrToken, "<MetallicMap>:"))
		{
			m_ppMaterials[nMaterial]->LoadTextureFromFile(MATERIAL_METALLIC_MAP, File);
			//m_ppMaterials[nMaterial]->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_METALLIC_MAP, ROOT_PARAMETER_METALLIC_TEXTURE, pMaterial->m_strTextureNames[3], (pMaterial->m_ppTextures[3]), pParent, File, pShader);
		}
		else if (!strcmp(pstrToken, "<EmissionMap>:"))
		{
			m_ppMaterials[nMaterial]->LoadTextureFromFile(MATERIAL_EMISSION_MAP, File);
			//m_ppMaterials[nMaterial]->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_EMISSION_MAP, ROOT_PARAMETER_EMISSION_TEXTURE, pMaterial->m_strTextureNames[4], (pMaterial->m_ppTextures[4]), pParent, File, pShader);
		}
		else if (!strcmp(pstrToken, "<DetailAlbedoMap>:"))
		{
			m_ppMaterials[nMaterial]->LoadTextureFromFile(MATERIAL_DETAIL_ALBEDO_MAP, File);
			//m_ppMaterials[nMaterial]->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_DETAIL_ALBEDO_MAP, ROOT_PARAMETER_DETAIL_ALBEDO_TEXTURE, pMaterial->m_strTextureNames[5], (pMaterial->m_ppTextures[5]), pParent, File, pShader);
		}
		else if (!strcmp(pstrToken, "<DetailNormalMap>:"))
		{
			m_ppMaterials[nMaterial]->LoadTextureFromFile(MATERIAL_DETAIL_NORMAL_MAP, File);
			//m_ppMaterials[nMaterial]->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_DETAIL_NORMAL_MAP, ROOT_PARAMETER_DETAIL_NORMAL_TEXTURE, pMaterial->m_strTextureNames[6], (pMaterial->m_ppTextures[6]), pParent, File, pShader);
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
	std::string strAnimationRootName;

	for (; ; )
	{
		::ReadStringFromFile(pInFile, pstrToken);
		if (!strcmp(pstrToken, "<RootObject>:"))
		{
			char pstrRootName[64] = { '\0' };
			::ReadStringFromFile(pInFile, pstrRootName);
			strAnimationRootName = pstrRootName;
		}
		else if (!strcmp(pstrToken, "<AnimationSets>:"))
		{
			nAnimationSets = ::ReadIntegerFromFile(pInFile);
			pLoadedModel->m_pAnimationSets = std::make_shared<CAnimationSets>(nAnimationSets);
		}
		else if (!strcmp(pstrToken, "<FrameNames>:"))
		{
			pLoadedModel->m_pAnimationSets->m_nBoneFrames = ::ReadIntegerFromFile(pInFile);
			pLoadedModel->m_pAnimationSets->m_ppBoneFrameName.resize(pLoadedModel->m_pAnimationSets->m_nBoneFrames);
			pLoadedModel->m_pAnimationSets->m_ppBoneFrameCaches.resize(pLoadedModel->m_pAnimationSets->m_nBoneFrames);

			for (int j = 0; j < pLoadedModel->m_pAnimationSets->m_nBoneFrames; j++)
			{
				::ReadStringFromFile(pInFile, pstrToken);
				pLoadedModel->m_pAnimationSets->m_ppBoneFrameName[j] = pstrToken;
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

bool CGameObject::DeepCopyFromModel(const std::string& strModelName, CGameObject* pGameObject)
{
	if (auto pModel = CResourceManager::Instance().GetModelInfo(strModelName)) {
		return DeepCopyFromModel(pModel, pGameObject);
	}
	return false;
}

bool CGameObject::DeepCopyFromModel(const CLoadedModelInfo* pLoadModel,CGameObject* pGameObject)
{
	if (pLoadModel) {
		pGameObject->DeepCopyFromGameObject(pLoadModel->m_pModelRootObject.get());
		return true;
	}
	return false;
}

bool CGameObject::DeepCopyFromModel(const std::string& strModelName)
{
	return DeepCopyFromModel(strModelName, this); 
}

std::unique_ptr<CGameObject> CGameObject::LoadFrameHierarchyFromFile(CGameObject* pParent, std::ifstream& file, std::shared_ptr<CShader> pShader, int* pnSkinnedMeshes, int nDepth)
{
	char pstrToken[64] = { '\0' };
	UINT nReads = 0;

	int nFrame = 0, nTextures = 0;

	auto pGameObject = std::make_unique<CGameObject>();
	pGameObject->SetParent(pParent);

	bool isGetModel = false;

	XMFLOAT4 xmf4Rotation {0,0,0,1};
	for (; ; )
	{
		::ReadStringFromFile(file, pstrToken);
		if (!strcmp(pstrToken, "<Frame>:"))
		{
			nFrame = ::ReadIntegerFromFile(file);
			nTextures = ::ReadIntegerFromFile(file);

			::ReadStringFromFile(file, pGameObject->m_strName);

			// Test용
			// isGetModel = DeepCopyFromModel(pGameObject->m_strName, pGameObject.get());
		}
		else if (!strcmp(pstrToken, "<Tag>:")) {
			::ReadStringFromFile(file, pGameObject->m_strTag);
		}
		else if (!strcmp(pstrToken, "<Transform>:"))
		{
			XMFLOAT3 xmf3Position, xmf3Rotation, xmf3Scale;
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
			std::shared_ptr<CStandardMesh> pMesh = std::make_shared<CStandardMesh>();
			pMesh->LoadMeshFromFile(file);
			pGameObject->SetMesh(pMesh);			
		}
		else if (!strcmp(pstrToken, "<SkinningInfo>:"))
		{
			if (pnSkinnedMeshes) (*pnSkinnedMeshes)++;

			std::shared_ptr<CSkinnedMesh> pSkinnedMesh = std::make_shared<CSkinnedMesh>();
			pSkinnedMesh->LoadSkinInfoFromFile(file);
			//pSkinnedMesh->CreateShaderVariables(pd3dDevice, pd3dCommandList);

			::ReadStringFromFile(file, pstrToken); //<Mesh>:
			if (!strcmp(pstrToken, "<Mesh>:")) pSkinnedMesh->LoadMeshFromFile(file);

			pGameObject->SetMesh(pSkinnedMesh);
		}
		else if (!strcmp(pstrToken, "<Materials>:"))
		{
			pGameObject->LoadMaterialsFromFile(file, pShader);
		}
		else if (!strcmp(pstrToken, "<Colliders>:"))
		{
			int nCollider = ::ReadIntegerFromFile(file);

			XMFLOAT3 xmf3Center, xmf3Extents;
			for (int i = 0; i < nCollider; i++)
			{
				file.read((char*)&xmf3Center, sizeof(float) * 3);
				file.read((char*)&xmf3Extents, sizeof(float) * 3);
				xmf3Extents = Vector3::ScalarProduct(xmf3Extents, 0.5f, false);

				auto pCollider = pGameObject->CreateComponent<COBBCollider>();
				pCollider->SetCenter(xmf3Center);
				pCollider->SetExtents(xmf3Extents);
			}
		}
		else if (!strcmp(pstrToken, "<Children>:"))
		{
			int nChilds = ::ReadIntegerFromFile(file);
			if (nChilds > 0)
			{
				pGameObject->m_pChilds.reserve(nChilds); 
				for (int i = 0; i < nChilds; i++)
				{
					auto pChild = CGameObject::LoadFrameHierarchyFromFile(pGameObject.get(), file, pShader, pnSkinnedMeshes, nDepth + 1);
					if (pChild) pGameObject->SetChild(std::move(pChild));
				}
			}
		}
		else if (!strcmp(pstrToken, "<ModelName>"))
		{
			std::string strModelName;
			::ReadStringFromFile(file, strModelName);

			if (isGetModel) continue;
			isGetModel = DeepCopyFromModel(strModelName, pGameObject.get());
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
	pLoadedModel->m_strFileName = pstrFileName;

	char pstrToken[500] = { '\0' };

	for (; ; )
	{
		if (::ReadStringFromFile(pInFile, pstrToken))
		{
			if (!strcmp(pstrToken, "<Hierarchy>:"))
			{
				pLoadedModel->m_pModelRootObject = CGameObject::LoadFrameHierarchyFromFile(nullptr, pInFile, pShader, &pLoadedModel->m_nSkinnedMeshes, 0);
				CResourceManager::Instance().RegisterGameObjectResources(pLoadedModel->m_pModelRootObject.get());
				::ReadStringFromFile(pInFile, pstrToken); //"</Hierarchy>"

				// 모델의 Root Transform을 초기화	
				pLoadedModel->m_pModelRootObject->SetLocalMatrix(Matrix4x4::Identity());

				// Model BoundingVolume 계산
				pLoadedModel->m_pModelRootObject->Update(0.0f);
				pLoadedModel->m_pModelRootObject->UpdateTransform();
				pLoadedModel->m_MeshBoundingBox = pLoadedModel->m_pModelRootObject->GetMergedMeshBound();
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

	return(pLoadedModel);
}

CGameObject* CGameObject::FindFrame(std::string strFrameName)
{
	if (m_strName == strFrameName) return this;

	for (auto& pChild : m_pChilds) if (auto pFrameObject = pChild->FindFrame(strFrameName)) return (pFrameObject);

	return(nullptr);
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

std::string to_string(GAMEOBJECT_LAYER type)
{
	std::string ret;
	switch (type)
	{
	case LAYER_DEFAULT:     ret = "Default";     break;
	case LAYER_TERRAIN:     ret = "Terrain";     break;
	case LAYER_ENVIRONMENT: ret = "Environment"; break;
	case LAYER_ENEMY:       ret = "Enemy";       break;
	case LAYER_PLAYER:      ret = "Player";      break;
	case LAYER_GUN:         ret = "Gun";         break;
	case LAYER_BULLET:      ret = "Bullet";      break;
	case LAYER_SKYBOX:      ret = "SkyBox";      break;
	case LAYER_CONTROLLER:  ret = "Controller";  break;
	case LAYER_UI:          ret = "UI";          break;
	case LAYER_TEXT:        ret = "Text";        break;
	default:                             ret = "Unknown";     break;
	}
	return ret;
}

std::wstring to_wstring(GAMEOBJECT_LAYER type)
{
	std::wstring ret;
	switch (type)
	{
	case LAYER_DEFAULT:     ret = L"Default";     break;
	case LAYER_TERRAIN:     ret = L"Terrain";     break;
	case LAYER_ENVIRONMENT: ret = L"Environment"; break;
	case LAYER_ENEMY:       ret = L"Enemy";       break;
	case LAYER_PLAYER:      ret = L"Player";      break;
	case LAYER_GUN:         ret = L"Gun";         break;
	case LAYER_BULLET:      ret = L"Bullet";      break;
	case LAYER_SKYBOX:      ret = L"SkyBox";      break;
	case LAYER_CONTROLLER:  ret = L"Controller";  break;
	case LAYER_UI:          ret = L"UI";          break;
	case LAYER_TEXT:        ret = L"Text";        break;
	default:                             ret = L"Unknown";     break;
	}
	return ret;
}

