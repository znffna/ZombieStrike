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

#include "ResourceManager.h"

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

	std::string m_strFileName{};

	std::shared_ptr<CGameObject> m_pModelRootObject;
	std::shared_ptr<CGameObject> m_pAnimationRootObject;

	int m_nSkinnedMeshes = 0;
	std::vector <std::shared_ptr<CSkinnedMesh>> m_ppSkinnedMeshes; //[SkinnedMeshes], Skinned Mesh Cache

	std::shared_ptr<CAnimationSets> m_pAnimationSets;

	BoundingBox m_MeshBoundingBox;
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
	};
	
public:
	CGameObject();
	CGameObject(const std::string& strName);
	virtual ~CGameObject();

	void ClearMemberVariables();
	void Init(); 

	// Object Initialization
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) {};
	virtual void DeepCopyFromGameObject(std::shared_ptr<CGameObject> rhs);

	// Active Flag
	bool IsActive() { return m_bActive; }
	void SetActive(bool bActive) { m_bActive = bActive; }

	// Object ID
	UINT GetObjectID() { return m_nObjectID; }
	void SetObjectID(UINT nObjectID) { m_nObjectID = nObjectID; }

	// Server ID
	UINT GetServerID() { return m_nObjectServerID; } // 서버 ID는 Object ID와 동일하게 사용
	void SetServerID(UINT nServerID) { m_nObjectServerID = nServerID; } // 서버 ID는 Object ID와 동일하게 사용

	// Object Name
	std::string GetName() { return m_strName; }
	void SetName(const std::string& strName);
	virtual std::string GetDefaultName() { return "CGameObject"; }

	// Layer
	virtual void SetLayer(GAMEOBJECT_LAYER layer) { m_nLayer = layer; }
	virtual GAMEOBJECT_LAYER GetLayer() { return m_nLayer; }

	void SetState(int state) {
		if (m_pSkinnedAnimationController) {
			if (m_pSkinnedAnimationController->ChangeState(state)) {
				/*if (GetLayer() == LAYER_PLAYER) {
					std::string debugString = std::to_string(GetServerID()) + " Player : Change Animation State to " + std::to_string(static_cast<int>(state)) + "\n";
					OutputDebugStringA(debugString.c_str());
				}*/
			}
		}
	}
	int GetUpperState() {
		if (m_pSkinnedAnimationController) return m_pSkinnedAnimationController->GetUpperState();
		return -1;
	}

	// 상속 관계
	std::shared_ptr<CGameObject> GetParent() { return m_pParent.lock(); }
	std::vector<std::shared_ptr<CGameObject>> GetChilds() { return m_pChilds; }
	std::shared_ptr<CGameObject> GetChild(int nIndex) { return m_pChilds[nIndex]; }

	void SetParent(std::shared_ptr<CGameObject> pParent) { m_pParent = pParent; };
	void SetChild(std::shared_ptr<CGameObject> pChild) { m_pChilds.push_back(pChild); pChild->SetParent(shared_from_this()); };

	// Object Update
	virtual void Update(float fTimeElapsed);

	void UpdateBBCache();

	// Object Render
	virtual void OnPrepareRender();
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = nullptr, bool bDepthWrite = false);

	// Object Collision
	virtual void OnCollision(std::shared_ptr<CGameObject>& pObjectB, std::shared_ptr<CCollider>& pColliderA, std::shared_ptr<CCollider>& pColliderB); // Collision Event
	CAABBCollider GetMergedCollider();

	BoundingBox GetMeshBound() {
		if (m_pMesh) return m_pMesh->GetBoundingBox();
		else return BoundingBox();
	}

	BoundingBox GetMergedMeshBound(BoundingBox* pVolume = nullptr);
	void UpdateLocalBoundingBox(const XMFLOAT4X4& pParentTransform = Matrix4x4::Identity());

public:
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

	virtual void ReleaseUploadBuffers();

protected:
	bool m_bActive; // Active Flag

	GAMEOBJECT_LAYER m_nLayer; // Object Layer

#ifdef _DEBUG
	int nLoadFrames = -1;
#endif

	// Object ID
	static UINT m_nObjectIDCounter; // Object ID Counter

	UINT m_nObjectID; // Object ID
	UINT m_nObjectServerID; // Object Server ID
	std::string m_strName;  // Object Name

public:
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


	std::shared_ptr<CMesh> m_pMesh; // Object Mesh

	// CMaterial
	UINT m_nMaterials = 0;
	std::vector<std::shared_ptr<CMaterial>> m_ppMaterials; // Object CMaterial

public:
	XMFLOAT4 m_xmf4Color = { 1.0f, 1.0f, 1.0f, 1.0f }; // Object Color
	void SetColor(const XMFLOAT4& xmf4Color) { m_xmf4Color = xmf4Color; }
	XMFLOAT4 GetColor() const { return m_xmf4Color; }

public:
	// Transform
	bool m_bPitchLock = false;
	bool m_bYawLock = false;
	bool m_bRollLock = false;
	std::shared_ptr<CTransform> m_pTransform = std::make_shared<CTransform>(this);

	// Component
	std::vector<std::shared_ptr<CComponent>> m_pComponents;

	std::vector<std::shared_ptr<CCollider>> m_pCachesColliders; // 모든 Children Collide를 복사할당(For CollisionCheck)

public:
	// Shader Variables
	ComPtr<ID3D12Resource> m_pd3dcbGameObject;
	CB_GAMEOBJECT_INFO* m_pcbMappedObject = nullptr;

protected:
	// Parent
	std::weak_ptr<CGameObject> m_pParent;

	// Child
	std::vector<std::shared_ptr<CGameObject>> m_pChilds; // Child Object
public:
	// Animation	
	std::shared_ptr<CAnimationController> m_pSkinnedAnimationController;

	// Load Model
	void LoadMaterialsFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::shared_ptr<CGameObject> pParent, std::ifstream& File, std::shared_ptr<CShader> pShader);
	std::shared_ptr<CTexture> FindReplicatedTexture(const _TCHAR* pstrTextureName);
	void FindAndSetSkinnedMesh(std::vector<std::shared_ptr<CSkinnedMesh>>& ppSkinnedMeshes, int* pnSkinnedMesh);;
	
	static void LoadAnimationFromFile(std::ifstream& pInFile, std::shared_ptr<CLoadedModelInfo> pLoadedModel);
	static std::shared_ptr<CGameObject> LoadFrameHierarchyFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std::shared_ptr<CGameObject> pParent, std::ifstream& file, std::shared_ptr<CShader> pShader, int* pnSkinnedMeshes, int nDepth = 0);
	static std::shared_ptr<CLoadedModelInfo> LoadGeometryAndAnimationFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, const char* pstrFileName, std::shared_ptr<CShader> pShader);
	
	// Clone
	static bool DeepCopyFromModel(const std::string &strModelName, std::shared_ptr<CGameObject>& pGameObject);
	static bool DeepCopyFromModel(const std::shared_ptr<CLoadedModelInfo>& pLoadModel, std::shared_ptr<CGameObject>& pGameObject);
	bool DeepCopyFromModel(const std::string& strModelName);;
	bool DeepCopyFromModel(const std::shared_ptr<CLoadedModelInfo>& pLoadModel) { auto pThis = shared_from_this(); return DeepCopyFromModel(pLoadModel, pThis); };
	
	std::shared_ptr<CGameObject> FindFrame(std::string strFrameName);
public:
	// Component
	template <typename T>
	std::shared_ptr<T> CreateComponent(std::shared_ptr<CGameObject> pOwner)
	{
		std::shared_ptr<T> pComponent = std::make_shared<T>(pOwner.get());
		m_pComponents.push_back(pComponent);
		pComponent->Init(pOwner.get());
		return pComponent;
	};

	template <typename T>
	std::shared_ptr<T> GetComponent() const
	{
		for (auto& pComponent : m_pComponents)
		{
			if (auto p = std::dynamic_pointer_cast<T>(pComponent)) return p;
		}
		return nullptr;
	}

	template <>
	std::shared_ptr<CTransform> GetComponent<CTransform>() const
	{
		return m_pTransform;
	};

	template <typename T>
	std::vector<std::shared_ptr<T>> GetComponents() {
		std::vector<std::shared_ptr<T>> result;
		for (auto& pComponent : m_pComponents) {
			if (auto casted = std::dynamic_pointer_cast<T>(pComponent)) {
				result.push_back(casted);
			}
		}
		return result;
	}

	template <typename T>
	void GetComponentsInChildren(std::vector<std::shared_ptr<T>>& pVec) const {
		for (auto& pComponent : m_pComponents) {
			if (auto casted = std::dynamic_pointer_cast<T>(pComponent)) {
				pVec.push_back(casted);
			}
		}

		for (auto& pChild : m_pChilds) {
			pChild->GetComponentsInChildren<T>(pVec);
		}
	}

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

// 2D Sprite
	virtual void SetSize(float cx, float cy, float width, float height) {}
};

////////////////////////////////////////////////////////////////////////////////////////
//

class CRotatingObject : public CGameObject
{
public:
	CRotatingObject();
	virtual ~CRotatingObject();

	// Object Initialization
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandlist);
	virtual std::string GetDefaultName() override { return "CRotatingObject"; } 

	static std::shared_ptr<CRotatingObject> Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	// Object Update
	virtual void Update(float fTimeElapsed) override;

	// Set Rotation Speed
	void SetRotationSpeed(float fRotationSpeed) { m_fRotationSpeed = fRotationSpeed; }

	// Set Rotation Axis
	void SetRotationAxis(DirectX::XMFLOAT3 xmf3RotationAxis) { m_xmf3RotationAxis = xmf3RotationAxis; }

private:
	float m_fRotationSpeed = 90.0f; // 초당 회전 속도
	XMFLOAT3 m_xmf3RotationAxis = XMFLOAT3(0.0f, 1.0f, 0.0f); // 회전 축

};  // CRotatingObject

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
};  // CCubeObject

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

	void AddBullets(const std::vector<CBulletVertex>& pBulletVertices)
	{
		std::dynamic_pointer_cast<CBulletMesh>(m_pMesh)->AddBullets(pBulletVertices);
	}

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
		std::dynamic_pointer_cast<CBulletMesh>(m_pMesh)->AddBullets(pBulletVertices);
	}

	void ClearFireInfos() {
		m_pFireInfos.clear();
	}

}; // CBulletParticleObject

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class TextBlock
{
public:
	bool						    m_bActive = true;
	std::wstring                    m_pstrText;
	D2D1_RECT_F                     m_d2dLayoutRect;
	ComPtr<IDWriteTextFormat> m_pdwFormat;
	ComPtr<ID2D1SolidColorBrush> m_pd2dTextBrush;

	void SetText(std::wstring pstrUIText) {
		m_pstrText = pstrUIText;
	}

	void SetActive(bool bActive) {
		m_bActive = bActive;
	}
};

class TextBlock;

class UILayer
{
public:
	UILayer(UINT nFrames, UINT nTextBlocks, ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Resource** ppd3dRenderTargets, UINT nWidth, UINT nHeight);

	void StorePoolTextBlock(UINT nIndex, std::wstring* pstrUIText, D2D1_RECT_F* pd2dLayoutRect, ComPtr<IDWriteTextFormat> pdwFormat, ComPtr<ID2D1SolidColorBrush> pd2dTextBrush);
	void UpdateTextOutputs(UINT nIndex, std::wstring* pstrUIText, D2D1_RECT_F* pd2dLayoutRect, ComPtr<IDWriteTextFormat> pdwFormat, ComPtr<ID2D1SolidColorBrush> pd2dTextBrush);
	void Render(UINT nFrame);
	void ReleaseResources();

	std::shared_ptr<TextBlock> GetNewTextBlock(int nPoolIndex = 0);

	ComPtr<ID2D1SolidColorBrush> CreateBrush(D2D1::ColorF d2dColor);
	ComPtr<IDWriteTextFormat> CreateTextFormat(WCHAR* pszFontName, float fFontSize);

public:
	void InitializeDevice(ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Resource** ppd3dRenderTargets);

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

	std::vector<std::shared_ptr<TextBlock>> m_pTextBlocks;
	std::vector<std::shared_ptr<TextBlock>> m_pTextPools;
};