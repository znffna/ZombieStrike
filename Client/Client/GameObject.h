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

class CGameObject : public std::enable_shared_from_this<CGameObject>
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
	void Initialize();

	// --------------------------------------------
	// Object methods
	// --------------------------------------------
	virtual void Update(float fTimeElapsed);
	virtual void LateUpdate() {};
	void UpdateBBCache();

	virtual void OnPrepareRender();
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr, bool bDepthWrite = false);

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
	// Active Flag
	// --------------------------------------------
	bool IsActive() { return m_bActive; }
	void SetActive(bool bActive) { m_bActive = bActive; }

private:
	bool m_bActive = true; // Active Flag

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
	void SetState(int UpperPose) {
		{
			std::string debugOutput = "SetState called : " + std::to_string(UpperPose) + " on object: " + m_strName + "\n";
			OutputDebugStringA(debugOutput.c_str());
		}
		/*if (m_pSkinnedAnimationController) {
			m_pSkinnedAnimationController->ChangeState(UpperPose);
		}*/
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
		m_pChilds.push_back(std::move(pChild));
		// 부모의 scene이 있으면 자식에게 전파
		if (m_pScene) pChild->SetScene(m_pScene);
	};

protected:
	// Parent
	CGameObject* m_pParent;
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

	BoundingBox GetMergedMeshBound(BoundingBox* pVolume = nullptr);
	void UpdateLocalBoundingBox(const XMFLOAT4X4& pParentTransform = Matrix4x4::Identity());

public:
	// --------------------------------------------
	// Renderer Group
	// --------------------------------------------
	
	// Mesh
	void SetMesh(std::shared_ptr<CMesh> pMesh);
	UINT GetMeshType() { return((m_pMesh) ? m_pMesh->GetType() : 0x00); }

	// Material
	void MaterialResize(int nMaterials) { m_ppMaterials.resize(nMaterials); }
	void AddMaterial(std::shared_ptr<CMaterial> pMaterial) { m_ppMaterials.push_back(pMaterial); }
	void SetMaterial(int nIndex, std::shared_ptr<CMaterial> pMaterial) { if(m_ppMaterials.size() <= nIndex) m_ppMaterials.resize(nIndex + 1); m_ppMaterials[nIndex] = pMaterial; }

	// Shader
	void SetShader(std::shared_ptr<CShader> pShader, int nIndex = 0);
protected:
	std::shared_ptr<CMesh> m_pMesh; // Object Mesh

	// Material
	UINT m_nMaterials = 0;
	std::vector<std::shared_ptr<CMaterial>> m_ppMaterials; // Object CMaterial

public:
	// --------------------------------------------
	// Shader Variables
	// TODO : 사실상 Object 단위로 Shader Variable을 생성하는 것은 비효율적임
	// --------------------------------------------
	void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseShaderVariables();

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
	void LoadMaterialsFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CGameObject* pParent, std::ifstream& File, std::shared_ptr<CShader> pShader);
	std::shared_ptr<CTexture> FindReplicatedTexture(const std::wstring pstrTextureName);
	void FindAndSetSkinnedMesh(std::vector<std::shared_ptr<CSkinnedMesh>>& ppSkinnedMeshes, int* pnSkinnedMesh);;
	
	static void LoadAnimationFromFile(std::ifstream& pInFile, std::shared_ptr<CLoadedModelInfo> pLoadedModel);
	static std::unique_ptr<CGameObject> LoadFrameHierarchyFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CGameObject* pParent, std::ifstream& file, std::shared_ptr<CShader> pShader, int* pnSkinnedMeshes, int nDepth = 0);
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


protected:
	std::unique_ptr<CTransform> m_pTransform;

public:
	// 2D Sprite
	virtual void SetSize(float cx, float cy, float width, float height) {}
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CSkyBox : public CGameObject
{
public:
	CSkyBox();
	virtual ~CSkyBox();

	virtual GAMEOBJECT_LAYER GetLayer() override { return GAMEOBJECT_LAYER::LAYER_SKYBOX; }

	// Object Initialization
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
	virtual std::string GetDefaultName() override { return "CSkyBox"; }

	static std::shared_ptr<CSkyBox> Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);

	// Object Render
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite = false) override;
}; // CSkyBox

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CHeightMapTerrain : public CGameObject
{
public:
	CHeightMapTerrain();
	virtual ~CHeightMapTerrain();

	virtual GAMEOBJECT_LAYER GetLayer() override { return GAMEOBJECT_LAYER::LAYER_TERRAIN; }

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
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite = false) override;

	//지형의 높이를 계산하는 함수이다(월드 좌표계). 높이 맵의 높이에 스케일의 y를 곱한 값이다. 
	float GetHeight(float x, float z) {
		if (isBinary) {
			if ((x < 0) || (z < 0) || (x >= m_nWidth) || (z >= m_nLength)) return(0.0f);
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
}; // CHeightMapTerrain

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct FIRE_INFO {
	XMFLOAT3 xmf3Position;
	XMFLOAT3 xmf3Look;
	XMFLOAT3 xmf3MuzzlePosition; // 총구 위치(렌더링 파티클 점 생성시 사용)
	int nBulletType = 0; // 총알 타입(0: 일반, 1: 산탄총 등)
	float fRange = 0.0f;
	float fspeed = 0.0f; // 총알 속도
};

class CBulletParticleObject : public CGameObject
{
	// TODO : Bullet을 전부 관리하는 Object로 변경할 예정
	// 현황 : GPU상에서 모든 Bullet을 파티클처럼 관리 하는 중(즉, 생성만 직접하고 소멸은 GPU에서 SO를 통해 출력시 discard하는 방식)
	// 목표 : 사격 즉시 피격위치 확정 및 GPU에 파티클 출력.
	//      : 이떄 총알은 GPU상에서 전진되며, GPU에 파티클 생성시에 주어진 거리 비례 LifeTime을 소유.
	//      : 즉, 총알이 날아가는 듯한 느낌만 주기 위함이며, 실제 피격효과로 인한 출력은 HitResult에 의해
	//      : 별도 파티클 생성으로 이루어 진다.(즉, Trail과 혈흔 표현을 별도로 구현 예정)
public:
	CBulletParticleObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, XMFLOAT3 xmf3Position, XMFLOAT3 xmf3Look, float fLifetime, XMFLOAT3 xmf3Acceleration, XMFLOAT3 xmf3Color, XMFLOAT2 xmf2Size, UINT nMaxParticles);
	virtual ~CBulletParticleObject();

	virtual GAMEOBJECT_LAYER GetLayer() override { return GAMEOBJECT_LAYER::LAYER_BULLET; }

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite = false);
	virtual void OnPostRender();

	// method
	void AddBullet(const XMFLOAT3& pOrigin, const XMFLOAT3& xmf3Look, float fRange);
	void AddBullet(const CBulletVertex& pBulletVertex);

private:
	std::vector<FIRE_INFO> m_pFireInfos;

	std::shared_ptr<CTexture> m_pRandowmValueTexture;
	std::shared_ptr<CTexture> m_pRandowmValueOnSphereTexture;

public:
	void AddFireInfo(const FIRE_INFO& fireInfo) {
		m_pFireInfos.push_back(fireInfo);
	}

	std::vector<FIRE_INFO> GetFireInfos() const {
		return m_pFireInfos;
	}

	void UpdateBulletVertices(const std::vector<CBulletVertex>& pBulletVertices) {
		//std::dynamic_pointer_cast<CBulletMesh>(m_pMesh)->AddBullets(pBulletVertices);
	}

	void ClearFireInfos() {
		m_pFireInfos.clear();
	}

}; // CBulletParticleObject

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class UILayer
{
public:
	UILayer(UINT nFrames, ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Resource** ppd3dRenderTargets, UINT nWidth, UINT nHeight);
	~UILayer() { ReleaseResources(); }

	void Render(UINT nFrame, const std::vector<TextBlock*>& vecTextObjects);

public:
	void InitializeDevice(ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Resource** ppd3dRenderTargets);
	void ReleaseResources();

	float                           m_fWidth = 0.0f;
	float                           m_fHeight = 0.0f;

	ComPtr<ID3D11DeviceContext> m_pd3d11DeviceContext;
	ComPtr<ID3D11On12Device> m_pd3d11On12Device;
	ComPtr<IDWriteFactory> m_pd2dWriteFactory;
	ComPtr<ID2D1Factory3> m_pd2dFactory;
	ComPtr<ID2D1Device2> m_pd2dDevice;
	ComPtr<ID2D1DeviceContext2> m_pd2dDeviceContext ;

	UINT             m_nRenderTargets = 0;
	ID3D11Resource** m_ppd3d11WrappedRenderTargets = NULL;
	ID2D1Bitmap1** m_ppd2dRenderTargets = NULL;

private:

	// Caching Brush & TextFormat 
	std::unordered_map<uint32_t, ComPtr<ID2D1SolidColorBrush>> m_pBrushes;
	std::map<std::pair<std::wstring, float>, ComPtr<IDWriteTextFormat>> m_pTextFormats;

	void ClearCache() {
		m_pBrushes.clear();
		m_pTextFormats.clear();
	}

	// Create & Get Brush & TextFormat
	ComPtr<ID2D1SolidColorBrush> CreateBrush(D2D1::ColorF d2dColor);
	ComPtr<IDWriteTextFormat> CreateTextFormat(WCHAR* pszFontName, float fFontSize);

	ComPtr<ID2D1SolidColorBrush> GetBrush(D2D1::ColorF d2dColor);
	ComPtr<IDWriteTextFormat> GetTextFormat(WCHAR* pszFontName, float fFontSize);
};