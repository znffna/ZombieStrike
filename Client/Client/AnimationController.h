///////////////////////////////////////////////////////////////////////////////
// Date: 2025-03-31
// AnimationController.h : CAnimationController 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"
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

// Animation Set과 Animation Track을 모아놓는 클래스
class CAnimationController
{
public:
	CAnimationController(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, int nAnimationTracks, std::shared_ptr<CLoadedModelInfo> pModel);;
	~CAnimationController();;

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
	void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);;

	void SetTrackAnimationSet(int nAnimationTrack, int nAnimationSet) { if (!m_pAnimationTracks.empty()) m_pAnimationTracks[nAnimationTrack].m_nAnimationSet = nAnimationSet; };

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
