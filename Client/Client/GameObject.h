///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// GameObject.h : CGameObject 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once

// Library
#include "stdafx.h"

// Component
#include "Component.h"
#include "Transform.h"
#include "Rigidbody.h"
#include "Collider.h"

#include "AnimationController.h"

// Resource
#include "Mesh.h"
#include "Texture.h"
#include "Shader.h"
#include "Material.h"

#define COMPONENT_KEY(T) typeid(T).name()

class CGameObject;
class CTexture;
class CShader;
class CCamera;

////////////////////////////////////////////////////////////////////////////////////////
//

class CLoadedModelInfo
{
public:
	CLoadedModelInfo() { };
	~CLoadedModelInfo()	{ };

	std::shared_ptr<CGameObject> m_pModelRootObject;

	int m_nSkinnedMeshes = 0;
	std::vector <std::shared_ptr<CSkinnedMesh>> m_ppSkinnedMeshes; //[SkinnedMeshes], Skinned Mesh Cache

	std::shared_ptr<CAnimationSets> m_pAnimationSets;

	BoundingBox m_ModelBoundingBox;
public:
	void PrepareSkinning();;
};

struct CB_GAMEOBJECT_INFO
{
	XMFLOAT4X4						m_xmf4x4World;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CGameObject : public std::enable_shared_from_this<CGameObject>
{
public:
	CGameObject();
	CGameObject(const std::string& strName);
	virtual ~CGameObject();

	void ClearMemberVariables();
	void Init();
	// Object Initialization
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12CommandList* pd3dCommandList) {
		// Transform Owner Setting
	};

	virtual void GetResourcesAndComponents(std::shared_ptr<CGameObject> rhs)
	{
		// 복사 할당 연산자 호출
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
		// if (rhs->m_pTransform) m_pTransform = std::make_shared<CTransform>(*rhs->m_pTransform);

		// Copy Components
		for (auto& pComponent : rhs->m_pComponents)
		{
			auto pclone = pComponent.second->Clone();
			m_pComponents[pComponent.first] = pclone;
			pclone->Init(this);
		}

		// Copy Childs
		std::shared_ptr<CGameObject> pnewChild;
		for (auto& pChild : rhs->m_pChilds)
		{
			pnewChild = std::make_shared<CGameObject>();
			pnewChild->GetResourcesAndComponents(pChild);
			m_pChilds.push_back(pnewChild);
		}
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

	//std::unique_ptr<CTransform> GetTransform() { return m_pTransform; }

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
	void SetLocalMatrix(DirectX::XMMATRIX xmf4x4Local) { m_pTransform->SetLocalMatrix(xmf4x4Local); }
	void SetWorldMatrix(DirectX::XMFLOAT4X4 xmf4x4World) { m_pTransform->SetWorldMatrix(xmf4x4World); }
	void SetWorldMatrix(DirectX::XMMATRIX xmf4x4World) { m_pTransform->SetWorldMatrix(xmf4x4World); }

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
		std::shared_ptr<T> pComponent = std::make_shared<T>(pOwner.get());
		m_pComponents[COMPONENT_KEY(T)] = pComponent;
		pComponent->Init(pOwner.get());
		return pComponent;
	};

	template <typename T>
	std::shared_ptr<T> GetComponent()
	{
		if constexpr (std::is_same_v<T, CTransform>) return m_pTransform;
		
		auto iter = m_pComponents.find(COMPONENT_KEY(T));
		if (iter != m_pComponents.end()) return std::dynamic_pointer_cast<T>(iter->second);
		return nullptr;
	}

	template <>
	std::shared_ptr<CTransform> GetComponent<CTransform>()
	{
		return m_pTransform;
	};

	template <>
	std::shared_ptr<CCollider> GetComponent<CCollider>()
	{
		if (GetComponent<CAABBCollider>()) return GetComponent<CAABBCollider>();
		else if (GetComponent<COBBCollider>()) return GetComponent<COBBCollider>();
		else if (GetComponent<CSphereCollider>()) return GetComponent<CSphereCollider>();
		return nullptr;
	}

	// Object Collision
	virtual bool IsCollided(std::shared_ptr<CGameObject>& pGameObject, UINT nDepth = 0);// Collision Check
	virtual void OnCollision(std::shared_ptr<CGameObject>& pGameObject); // Collision Event

	BoundingBox GetMergedBoundingBox(BoundingBox* pVolume = nullptr);

	// Mesh
	void SetMesh(std::shared_ptr<CMesh> pMesh);
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

#ifdef _DEBUG
	int nLoadFrames = -1;
#endif

	// Object ID
	static UINT m_nObjectIDCounter; // Object ID Counter

	UINT m_nObjectID; // Object ID
	std::string m_strName;  // Object Name

	std::shared_ptr<CMesh> m_pMesh; // Object Mesh

	// CMaterial
	UINT m_nMaterials = 0;
	std::vector<std::shared_ptr<CMaterial>> m_ppMaterials; // Object CMaterial
public:
	// Transform
	std::shared_ptr<CTransform> m_pTransform = std::make_shared<CTransform>(this);

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
	static bool CloneByModel(std::string& strModelName, std::shared_ptr<CGameObject>& pGameObject);
	static std::shared_ptr<CGameObject> LoadFrameHierarchyFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CGameObject> pParent, std::ifstream& file, std::shared_ptr<CShader> pShader, int* pnSkinnedMeshes, int nDepth = 0);
	static std::shared_ptr<CLoadedModelInfo> LoadGeometryAndAnimationFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, const char* pstrFileName, std::shared_ptr<CShader> pShader);

	std::shared_ptr<CGameObject> FindFrame(std::string strFrameName);
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
	static std::shared_ptr<CHeightMapTerrain> InitializeByBinary(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature,
		LPCTSTR pBinFileName, LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength,
		XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color);
	virtual std::string GetDefaultName() override { return "CHeightMapTerrain"; }

	static std::shared_ptr<CHeightMapTerrain> Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature,
		LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength,
		XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color);

	// Object Render
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera) override;

	//지형의 높이를 계산하는 함수이다(월드 좌표계). 높이 맵의 높이에 스케일의 y를 곱한 값이다. 
	float GetHeight(float x, float z) {
		if (isBinary) {
			//높이 맵의 좌표의 정수 부분과 소수 부분을 계산한다. 
			int nx = (int)x;
			int nz = (int)z;
			float fxPercent = x - nx;
			float fzPercent = z - nz;

			const auto& fBottomLeft = m_pVertices.at(nx + (nz * m_nWidth)).m_xmf3Position.y;
			const auto& fBottomRight = m_pVertices.at((nx + 1) + (nz * m_nWidth)).m_xmf3Position.y;
			const auto& fTopLeft = m_pVertices.at(nx + ((nz + 1) * m_nWidth)).m_xmf3Position.y;
			const auto& fTopRight = m_pVertices.at((nx + 1) + ((nz + 1) * m_nWidth)).m_xmf3Position.y;

			//사각형의 네 점을 보간하여 높이(픽셀 값)를 계산한다. 
			float fTopHeight = fTopLeft * (1 - fxPercent) + fTopRight * fxPercent;
			float fBottomHeight = fBottomLeft * (1 - fxPercent) + fBottomRight * fxPercent;
			float fHeight = fBottomHeight * (1 - fzPercent) + fTopHeight * fzPercent;

			return(fHeight);
		}
		else return(m_pHeightMapImage->GetHeight(x / m_xmf3Scale.x, z / m_xmf3Scale.z) * m_xmf3Scale.y);
	}
	
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

	// Binary 로 생성시
	bool isBinary = false;

	std::vector<CTerrainVertex> m_pVertices;
	std::vector<UINT> m_pIndices;
};

