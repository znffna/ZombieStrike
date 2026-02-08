///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// GameObject.h : CGameObject 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once

// Component
#include "Component.h"
#include "Transform.h"
#include "Rigidbody.h"
#include "Collider.h"
#include "AnimationController.h"
#include "TextComponent.h"

#include "ResourceManager.h"

// Resource
#include "Mesh.h"
#include "Texture.h"
#include "Shader.h"
#include "Material.h"

#include "Camera.h" // CCamera 타입을 사용하므로 헤더 포함 (기존 전방선언 대신 안전하게 포함)

#define COMPONENT_KEY(T) typeid(T).name()

class CGameObject;
class CTexture;
class CShader;
class CCamera;

class CScene; // forward

////////////////////////////////////////////////////////////////////////////////////////
//

class CLoadedModelInfo
{
public:
	CLoadedModelInfo() {};
	~CLoadedModelInfo() {};

	std::string m_strFileName{};

	std::shared_ptr<CGameObject> m_pModelRootObject;

	int m_nSkinnedMeshes = 0;
	std::vector <std::shared_ptr<CSkinnedMesh>> m_ppSkinnedMeshes; //[SkinnedMeshes], Skinned Mesh Cache

	std::shared_ptr<CAnimationSets> m_pAnimationSets;

	BoundingBox m_MeshBoundingBox;
public:
	void PrepareSkinning();
};


struct CB_GAMEOBJECT_INFO
{
	XMFLOAT4X4						m_xmf4x4World;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

enum GAMEOBJECT_LAYER {
	LAYER_DEFAULT = 0,
	LAYER_TERRAIN,
	LAYER_ENVIRONMENT,
	LAYER_ENEMY,
	LAYER_PLAYER,
	LAYER_GUN,
	LAYER_BULLET,
	LAYER_SKYBOX,
	LAYER_CONTROLLER,
	LAYER_UI,
	LAYER_TEXT,
};

std::string to_string(GAMEOBJECT_LAYER type);
std::wstring to_wstring(GAMEOBJECT_LAYER type);

class CGameObject
{
public:
	CGameObject();
	CGameObject(const std::string& strName);
	virtual ~CGameObject();

	// --------------------------------------------
	// Object Initialization
	// --------------------------------------------
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) {};
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int) {};
	virtual void DeepCopyFromGameObject(CGameObject* rhs);
	void ClearMemberVariables();
	virtual void Initialize();

	// --------------------------------------------
	// Object methods
	// --------------------------------------------
	virtual void Update(float fTimeElapsed);
	virtual void LateUpdate() {};
	void UpdateBBCache();

	virtual void OnPrepareRender();
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr, bool bDepthWrite = false);
	void RenderCollider(ID3D12GraphicsCommandList* pd3dCommandList);
public:
	// --------------------------------------------
	// Object Name
	// --------------------------------------------
	std::string GetName() { return m_strName; }
	void SetName(const std::string& strName);
	virtual std::string GetDefaultName() { return "CGameObject"; }
private:
	std::string m_strName = "GameObject";  // Object Name

public:
	// --------------------------------------------
	// Active Flag
	// --------------------------------------------
	bool IsActive() { return m_bActive; }
	void SetActive(bool bActive) { m_bActive = bActive; }

private:
	bool m_bActive = true; // Active Flag

public:
	// --------------------------------------------
	// Object ID
	// --------------------------------------------
	UINT GetID() { return m_nID; }
	void SetID(UINT nObjectID) { m_nID = nObjectID; }
private:
	UINT m_nID = 0; 

public:
	// --------------------------------------------
	// Server ID
	// --------------------------------------------
	UINT GetSID() { return m_nSID; } // 서버 ID는 Object ID와 동일하게 사용
	void SetSID(UINT nServerID) { m_nSID = nServerID; } // 서버 ID는 Object ID와 동일하게 사용

private:
	UINT m_nSID = 0; // Object Server ID

public:
	// --------------------------------------------
	// Initialize Flag
	// --------------------------------------------
	bool IsInitialized() const { return m_bInitialized; }
protected:
	bool m_bInitialized = false;
	
	bool IsGPUInitialized();

public:
	// --------------------------------------------
	// Layer
	// --------------------------------------------
	virtual void SetLayer(GAMEOBJECT_LAYER layer) { m_nLayer = layer; }
	virtual GAMEOBJECT_LAYER GetLayer() { return m_nLayer; }
	
private:
	GAMEOBJECT_LAYER m_nLayer; // Object Layer

public:
	// --------------------------------------------
	// State
	// --------------------------------------------
	void SetState(int BasePose) {
		{
			//std::string debugOutput = "SetState called : " + std::to_string(BasePose) + " on object: " + m_strName + "\n";
			//OutputDebugStringA(debugOutput.c_str());
		}
		/*if (m_pSkinnedAnimationController) {
			m_pSkinnedAnimationController->ChangeState(UpperPose);
		}*/
		if (auto pAnim = GetComponent<CAnimationController>())
		{
			pAnim->SetBasePose(BasePose);
		}
	}

public:
	// --------------------------------------------
	// 소속 Scene 참조
	// --------------------------------------------
	void SetScene(CScene* pScene)
	{
		m_pScene = pScene;

		// 자식들에게도 전파
		for (auto& pChild : m_pChilds)
		{
			pChild->SetScene(pScene);
		}
	};

	CScene* GetScene() { return m_pScene; };

protected:
	// Scene observer pointer (lifetime: Scene owns GameObjects)
	CScene* m_pScene = nullptr;

public:
	// --------------------------------------------
	// 상속 관계
	// --------------------------------------------
	CGameObject* GetParent() { return m_pParent; }
	std::vector<CGameObject*> GetChilds()
	{
		std::vector<CGameObject*> m_pChildPtrs;
		for (auto& child : m_pChilds)
			m_pChildPtrs.push_back(child.get());
		return m_pChildPtrs;
	}
	CGameObject* GetChild(int nIndex) { return m_pChilds[nIndex].get(); }

	void SetParent(CGameObject* pParent) { m_pParent = pParent; };
	void SetChild(std::unique_ptr<CGameObject> pChild) 
	{ 
		pChild->SetParent(this);
		if (m_pScene) pChild->SetScene(m_pScene);

		m_pChilds.push_back(std::move(pChild));
		// 부모의 scene이 있으면 자식에게 전파
	};

protected:
	// Parent
	CGameObject* m_pParent = nullptr;
	// Child
	std::vector<std::unique_ptr<CGameObject>> m_pChilds; // Child Object

public:
	// --------------------------------------------
	// Object Collision
	// --------------------------------------------
	virtual void OnCollision(CGameObject* pOther, CCollider* pColliderA, CCollider* pColliderB); // Collision Event
	BoundingBox GetMergedCollider() const;

	BoundingBox GetMeshBound() {
		if (m_pMesh) return m_pMesh->GetBoundingBox();
		else return BoundingBox();
	}

	virtual BoundingBox GetMergedMeshBound(BoundingBox* pVolume = nullptr);
	void UpdateLocalBoundingBox(const XMFLOAT4X4& pParentTransform = Matrix4x4::Identity());

public:
	// --------------------------------------------
	// Renderer Group
	// --------------------------------------------
	
	// Mesh
	void SetMesh(std::shared_ptr<CMesh> pMesh);
	UINT GetMeshType() { return((m_pMesh) ? m_pMesh->GetType() : 0x00); }

	// Material
	void SetMaterialSize(int nMaterials) { m_ppMaterials.resize(nMaterials); }
	void AddMaterial(std::shared_ptr<CMaterial> pMaterial) { m_ppMaterials.push_back(pMaterial); }
	void SetMaterial(int nIndex, std::shared_ptr<CMaterial> pMaterial) { if(m_ppMaterials.size() <= nIndex) m_ppMaterials.resize(nIndex + 1); m_ppMaterials[nIndex] = pMaterial; }

	// Shader
	void SetShader(std::shared_ptr<CShader> pShader, int nIndex = 0);
protected:
	std::shared_ptr<CMesh> m_pMesh; // Object Mesh

	// Material
	std::vector<std::shared_ptr<CMaterial>> m_ppMaterials; // Object CMaterial

public:
	// --------------------------------------------
	// Shader Variables
	// TODO : 사실상 Object 단위로 Shader Variable을 생성하는 것은 비효율적임
	// --------------------------------------------
	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();

	void CollectShaderVariables();

	virtual void ReleaseUploadBuffers();

public:
	ComPtr<ID3D12Resource> m_pd3dcbGameObject;
	CB_GAMEOBJECT_INFO* m_pcbMappedObject = nullptr;

public:
	// --------------------------------------------
	// Skinning Tag
	// TODO : Bone Tag를 Object가 다루는건 아닌거같음.
	// --------------------------------------------
	std::string m_strTag = "None"; // Object Tag (For Skinning)
	std::string GetTag() const { return m_strTag; }
	
	float GetBoneUpperWeight() const
	{ 
		if (m_strName == "mixamorig:Hips") return 0.55f;
		else if (m_strName == "mixamorig:Spine") return 0.55f;
		else if (m_strName == "mixamorig:Spine1") return 0.75f;
		else if (m_strName == "mixamorig:Spine2") return 0.9f;
		else if (m_strTag == "Upper") return 1.0f;
		else return 0.0f;
	}

public:
	// --------------------------------------------
	// Debug Color
	// TODO : Object Color는 Renderer로 옮겨야 함.
	// --------------------------------------------
	XMFLOAT4 m_xmf4Color = { 1.0f, 1.0f, 1.0f, 1.0f }; // Object Color
	void SetColor(const XMFLOAT4& xmf4Color) { m_xmf4Color = xmf4Color; }
	XMFLOAT4 GetColor() const { return m_xmf4Color; }

public:
	// --------------------------------------------
	// Component
	// --------------------------------------------
	template <typename T>
	T* CreateComponent()
	{
		// 객체 생성
		auto pComponent = std::make_unique<T>(this);
		if (pComponent == nullptr)
		{
			std::string errorMsg = "Failed to create component of type: " + std::string(typeid(T).name()) + "\n";
			OutputDebugStringA(errorMsg.c_str());
			return nullptr;
		}
		auto prawPtr = pComponent.get();
		m_pComponents.push_back(std::move(pComponent));

		// 초기화
		prawPtr->Initialize();

		return prawPtr;
	};

	template <typename T>
	T* GetComponent() const
	{
		for (auto& pComponent : m_pComponents)
		{
			if (auto p = dynamic_cast<T*>(pComponent.get())) return p;
		}
		return nullptr;
	}

	template <>
	CTransform* GetComponent<CTransform>() const
	{
		return m_pTransform.get();
	};

	template <typename T>
	std::vector<T*> GetComponents() {
		std::vector<T*> result;
		for (auto& pComponent : m_pComponents) {
			if (auto casted = dynamic_cast<T*>(pComponent.get())) {
				result.push_back(casted);
			}
		}
		return result;
	}

	template <typename T>
	void GetComponentsInChildren(std::vector<T*>& pVec) const {
		for (auto& pComponent : m_pComponents) {
			auto rawptr = pComponent.get();
			if (auto casted = dynamic_cast<T*>(rawptr)) {
				pVec.push_back(casted);
			}
		}

		for (auto& pChild : m_pChilds) {
			pChild->GetComponentsInChildren<T>(pVec);
		}
	}

	void SetRootMotion(bool bRootMotion) const { if (auto p = GetComponent<CAnimationController>()) p->SetRootMotion(bRootMotion); }


	std::vector<CCollider*>& GetCachedColliders() { return m_pCachesColliders; }
private:
	bool m_bPitchLock = false;
	bool m_bYawLock = false;
	bool m_bRollLock = false;

	// Component
	std::vector<std::unique_ptr<CComponent>> m_pComponents;
	std::vector<CCollider*> m_pCachesColliders; // 모든 Children Collide를 복사할당(For CollisionCheck)

public:
	// --------------------------------------------
	// Model
	// --------------------------------------------
	
	// Load Model
	void LoadMaterialsFromFile(std::ifstream& File, std::shared_ptr<CShader> pShader);
	std::shared_ptr<CTexture> FindReplicatedTexture(const std::wstring pstrTextureName);
	void FindAndSetSkinnedMesh(std::vector<std::shared_ptr<CSkinnedMesh>>& ppSkinnedMeshes, int* pnSkinnedMesh);;
	
	static void LoadAnimationFromFile(std::ifstream& pInFile, std::shared_ptr<CLoadedModelInfo> pLoadedModel);
	static std::unique_ptr<CGameObject> LoadFrameHierarchyFromFile(CGameObject* pParent, std::ifstream& file, std::shared_ptr<CShader> pShader, int* pnSkinnedMeshes, int nDepth = 0);
	static std::shared_ptr<CLoadedModelInfo> LoadGeometryAndAnimationFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, const char* pstrFileName, std::shared_ptr<CShader> pShader);
	
	// Clone
	static bool DeepCopyFromModel(const std::string &strModelName, CGameObject* pGameObject);
	static bool DeepCopyFromModel(const CLoadedModelInfo* pLoadModel, CGameObject* pGameObject);
	bool DeepCopyFromModel(const std::string& strModelName);
	bool DeepCopyFromModel(const CLoadedModelInfo* pLoadModel) { return DeepCopyFromModel(pLoadModel, this); };
	
	CGameObject* FindFrame(std::string strFrameName);

public:
	// Transform
	const DirectX::XMFLOAT3 GetPosition() { return m_pTransform->GetPosition(); }
	const DirectX::XMFLOAT3 GetRightVector() { return m_pTransform->GetRight(); }
	const DirectX::XMFLOAT3 GetUpVector() { return m_pTransform->GetUp(); }
	const DirectX::XMFLOAT3 GetLookVector() { return m_pTransform->GetLook(); }
	const DirectX::XMFLOAT3 GetScale() { return m_pTransform->GetScale(); }

	DirectX::XMFLOAT3 GetRotation() { return m_pTransform->GetRotation(); }
	virtual float GetPitch() { return m_pTransform->GetRotation().x; } // X 축을	기준으로 회전
	virtual float GetYaw() { return m_pTransform->GetRotation().y; } // Y 축을 기준으로 회전
	virtual float GetRoll() { return m_pTransform->GetRotation().z; } // Z 축을 기준으로 회전

	DirectX::XMFLOAT4X4 GetLocalMatrix() { return m_pTransform->GetLocalMatrix(); }
	DirectX::XMFLOAT4X4 GetWorldMatrix() { return m_pTransform->GetWorldMatrix(); }

	DirectX::XMFLOAT3 GetLocalPosition() { return m_pTransform->GetLocalPosition(); };

	//std::unique_ptr<CTransform> GetTransform() { return m_pTransform; }

	void SetPosition(DirectX::XMFLOAT3 xmf3Position) { m_pTransform->SetPosition(xmf3Position); }
	void SetPosition(float fx, float fy, float fz) { m_pTransform->SetPosition(fx, fy, fz); }
	void SetScale(DirectX::XMFLOAT3 xmf3Scale) { m_pTransform->SetPosition(xmf3Scale); };
	void SetScale(float fx, float fy, float fz) { m_pTransform->SetPosition(fx, fy, fz); };

	void Move(DirectX::XMFLOAT3 xmf3Shift) { m_pTransform->Move(xmf3Shift); };
	void Move(float x, float y, float z) { Move(DirectX::XMFLOAT3(x, y, z)); }

	virtual void Move(DWORD dwDirection, float fDistance, float deltaTime);

	void MoveStrafe(float fDistance = 1.0f) { m_pTransform->MoveStrafe(fDistance); };
	 void MoveUp(float fDistance = 1.0f) { m_pTransform->MoveUp(fDistance); };
	 void MoveForward(float fDistance = 1.0f) { m_pTransform->MoveForward(fDistance); };

	void SetRotationAxisLock(bool bPitchLock, bool bYawLock, bool bRollLock);
	virtual void Rotate(float fPitch = 0.0f, float fYaw = 0.0f, float fRoll = 0.0f);
	virtual void Rotate(const XMFLOAT3& pxmf3Axis, float fAngle) { m_pTransform->Rotate(pxmf3Axis, fAngle); }
	virtual void Rotate(const XMFLOAT4& pxmf4Quaternion) { m_pTransform->Rotate(pxmf4Quaternion); }

	void SetLook(const XMFLOAT3& pxmf3Look) { m_pTransform->SetLook(pxmf3Look); }
	void SetLook(float x, float y, float z) { m_pTransform->SetLook(XMFLOAT3(x,y,z)); }

	void SetLocalMatrix(DirectX::XMFLOAT4X4 xmf4x4Local) { m_pTransform->SetLocalMatrix(xmf4x4Local); }
	void SetLocalMatrix(DirectX::XMMATRIX xmf4x4Local) { m_pTransform->SetLocalMatrix(xmf4x4Local); }
	void SetWorldMatrix(DirectX::XMFLOAT4X4 xmf4x4World) { m_pTransform->SetWorldMatrix(xmf4x4World); }
	void SetWorldMatrix(DirectX::XMMATRIX xmf4x4World) { m_pTransform->SetWorldMatrix(xmf4x4World); }

	virtual void UpdateTransform(const DirectX::XMFLOAT4X4* xmf4x4ParentMatrix = nullptr);
	void UpdateTransform(const DirectX::XMFLOAT4X4& xmf4x4ParentMatrix);
	void UpdateTransform(std::shared_ptr<CGameObject>& pGameobject);

public:
	void SetTerrain(void* pTerrain) const { if (auto pRigidBody = GetComponent<CRigidBody>()) pRigidBody->SetTerrainUpdatedContext(pTerrain); }

protected:
	std::unique_ptr<CTransform> m_pTransform;

public:
	// 2D Sprite
	virtual void SetSize(float cx, float cy, float width, float height) {}
};

