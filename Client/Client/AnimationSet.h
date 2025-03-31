///////////////////////////////////////////////////////////////////////////////
// Date: 2025-03-28
// AnimationSet.h : CAnimationSet 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once
#include "stdafx.h"

class CGameObject;

//#define _WITH_ANIMATION_SRT
class CAnimationSet
{
public:
	CAnimationSet(float fLength, int nFramesPerSecond, int nKeyFrameTransforms, int nSkinningBones, char* pstrName);;

	~CAnimationSet();;

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
	XMFLOAT4X4 GetSRT(int nBone, float fPosition);;
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
