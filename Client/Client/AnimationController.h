///////////////////////////////////////////////////////////////////////////////
// Date: 2025-03-31
// AnimationController.h : CAnimationController 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Component.h"
#include "AnimationSet.h"

class CGameObject;
class CLoadedModelInfo;
class CSkinnedMesh;

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

// Animation Set마다 Track을 연결.
// Track에선 실행 속도, 현재 시간, 그리고 가중치를 가지고 있음.
// Track 모음은 AnimationController에서 관리.

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
	float UpdatePosition(float fTrackPosition, float fElapsedTime, float fAnimationLength);;

	void SetCallbackKeys(int nCallbackKeys);;
	void SetCallbackKey(int nKeyIndex, float fTime, void* pData);;
	void SetAnimationCallbackHandler(std::shared_ptr<CAnimationCallbackHandler> pCallbackHandler);;

	void HandleCallback();;
};

enum ANIMATION_POSE // Number == Animation Track Index
{
	/// Player Animation States
	// IDLE(Aiming)
	IDLE = 0,
	// WALK
	WALK_RIGHT,
	WALK_FORWARD_RIGHT,
	WALK_FORWARD,
	WALK_FORWARD_LEFT,
	WALK_LEFT,
	WALK_BACKWARD_LEFT,
	WALK_BACKWARD,
	WALK_BACKWARD_RIGHT,
	// FIRE
	FIRE,
	// Reload
	RELOAD,
	// Hitted
	HITTED,

	/// Zombie Animation States
	ZOMBIE_IDLE = 0,
	ZOMBIE_RUNNING,
	ZOMBIE_ATTACK,
	ZOMBIE_DEATH,
	ZOMBIE_SCREAM,
	ZOMBIE_HIT,

};

// 전역 본 변환 버퍼 관리자 (싱글톤)
class CGlobalBoneTransformManager
{
private:
	CGlobalBoneTransformManager() = default;
	~CGlobalBoneTransformManager() { Shutdown(); }

public:
	static CGlobalBoneTransformManager& Instance()
	{
		static CGlobalBoneTransformManager instance;
		return instance;
	}

	// 복사/이동 방지
	CGlobalBoneTransformManager(const CGlobalBoneTransformManager&) = delete;
	CGlobalBoneTransformManager& operator=(const CGlobalBoneTransformManager&) = delete;

	void Initialize(ID3D12Device* pd3dDevice);
	void Shutdown();

	// Bone 범위 할당 (시작 인덱스 반환)
	UINT AllocateBoneRange(int nBoneCount);

	// Bone Transform 쓰기
	void WriteBoneTransforms(UINT offset, const XMFLOAT4X4* pTransforms, UINT count);

	// GPU 리소스 가져오기
	ID3D12Resource* GetBoneTransformBuffer() const { return m_pd3dDefaultGlobalBoneTransformBuffer.Get(); }

	UINT GetMaxIndex() const { return m_nMaxBoneOffset; }
	UINT GetLastAlloactedIndex() const { return m_nPrevBoneOffset; }

	void PrepareRender(ID3D12GraphicsCommandList* pd3dCommandList);
private:
	static constexpr UINT MAX_TOTAL_BONES = 8192; // 전체 게임에서 사용할 최대 본 개수

	ComPtr<ID3D12Resource> m_pd3dDefaultGlobalBoneTransformBuffer; // StructuredBuffer<float4x4>
	ComPtr<ID3D12Resource> m_pd3dGlobalBoneTransformBuffer;		   // StructuredBuffer<float4x4> 
	XMFLOAT4X4* m_pMappedGlobalBoneTransforms = nullptr;		   // Mapped pointer
	UINT m_nCurrentBoneOffset = 0;								   // 현재 할당된 본 오프셋
	UINT m_nPrevBoneOffset = 0;									   // 이전 프레임에 할당된 본 오프셋
	UINT m_nMaxBoneOffset = 0;									   // 생성된 최대 본 오프셋
	bool m_bInitialized = false;
};

// Animation Set과 Animation Track을 모아놓는 클래스
class CAnimationController : public CComponent
{
public:
	CAnimationController(CGameObject* pOwner) : CComponent(pOwner) {};
	CAnimationController(const CAnimationController& rhs) : CComponent(nullptr)
	{
		// State
		BasePose = rhs.BasePose;
		UpperPose = rhs.UpperPose;
		// Animation 
		m_fTime = rhs.m_fTime;
		m_nAnimationTracks = rhs.m_nAnimationTracks;
		m_pAnimationTracks = rhs.m_pAnimationTracks;
		m_pAnimationSets = rhs.m_pAnimationSets;
		m_nSkinnedMeshes = rhs.m_nSkinnedMeshes;
		m_ppSkinnedMeshes = rhs.m_ppSkinnedMeshes;
	};
	~CAnimationController();

	virtual void Initialize() override { Clear(); }
	virtual void OnDestroy() override { Clear(); }

	virtual std::unique_ptr<CComponent> Clone(CGameObject* newOwner) const { auto ret = std::make_unique<CAnimationController>(*this); ret->SetOwnerInternal(newOwner); return (ret); };


	void Clear();
	void SetModel(CLoadedModelInfo* pModel);

public:
	// State
	ANIMATION_POSE BasePose = IDLE;  // Lower Body Animation 
	ANIMATION_POSE UpperPose = IDLE; // Upper Body Animation 

	// Animation 
	float 							m_fTime = 0.0f;

	int 						m_nAnimationTracks = 0;
	std::vector<CAnimationTrack> m_pAnimationTracks;

	std::shared_ptr<CAnimationSets> m_pAnimationSets;

	int m_nSkinnedMeshes = 0;
	std::vector<std::shared_ptr<CSkinnedMesh>> m_ppSkinnedMeshes; //[SkinnedMeshes], Skinned Mesh Cache

	//std::vector<ComPtr<ID3D12Resource>> m_ppd3dcbSkinningBoneTransforms; //[SkinnedMeshes]
	//std::vector<XMFLOAT4X4*> m_ppcbxmf4x4MappedSkinningBoneTransforms; //[SkinnedMeshes]

	// 각 SkinnedMesh의 전역 버퍼 내 시작 인덱스
	std::vector<UINT> m_vSkinnedMeshBoneOffsets;

	// CPU 캐시: [SkinnedMeshes][Bones]
	std::vector<std::vector<XMFLOAT4X4>> m_xmf4x4SkinningBoneTransforms; //[SkinnedMeshes], AdvanceTime이후 여기에 저장, 나중에 m_ppcbxmf4x4MappedSkinningBoneTransforms에 복사

public:
	void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);;

	void SetTrackAnimationSet(int nAnimationTrack, int nAnimationSet) { if (!m_pAnimationTracks.empty()) m_pAnimationTracks[nAnimationTrack].m_nAnimationSet = nAnimationSet; };

private:
	void SetTrackEnable(int nAnimationTrack, bool bEnable) { if (!m_pAnimationTracks.empty()) m_pAnimationTracks[nAnimationTrack].SetEnable(bEnable); };
	
public:
	void SetTrackPosition(int nAnimationTrack, float fPosition) { if (!m_pAnimationTracks.empty()) m_pAnimationTracks[nAnimationTrack].SetPosition(fPosition); };
	void SetTrackSpeed(int nAnimationTrack, float fSpeed) { if (!m_pAnimationTracks.empty()) m_pAnimationTracks[nAnimationTrack].SetSpeed(fSpeed); };
	void SetTrackWeight(int nAnimationTrack, float fWeight) { if (!m_pAnimationTracks.empty()) m_pAnimationTracks[nAnimationTrack].SetWeight(fWeight); };

	void SetCallbackKeys(int nAnimationTrack, int nCallbackKeys) { if (!m_pAnimationTracks.empty()) m_pAnimationTracks[nAnimationTrack].SetCallbackKeys(nCallbackKeys); };
	void SetCallbackKey(int nAnimationTrack, int nKeyIndex, float fTime, void* pData) { if (!m_pAnimationTracks.empty()) m_pAnimationTracks[nAnimationTrack].SetCallbackKey(nKeyIndex, fTime, pData); };
	void SetAnimationCallbackHandler(int nAnimationTrack, std::shared_ptr<CAnimationCallbackHandler> pCallbackHandler) { if (!m_pAnimationTracks.empty()) m_pAnimationTracks[nAnimationTrack].SetAnimationCallbackHandler(pCallbackHandler); };

	void AdvanceTime(float fElapsedTime, CGameObject* pRootGameObject);
	void ApplyPitchToSpine(CGameObject* pRootGameObject);

public:
	// Pose Control

public:
	bool m_bRootMotion = false;
	std::shared_ptr<CGameObject> m_pModelRootObject = nullptr;

	CGameObject* m_pRootMotionObject = nullptr;
	XMFLOAT3 m_xmf3FirstRootMotionPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);

	void SetRootMotion(bool bRootMotion) { m_bRootMotion = bRootMotion; }

	virtual void OnRootMotion(CGameObject* pRootGameObject) { }
	virtual void OnAnimationIK(CGameObject* pRootGameObject) { }
};
