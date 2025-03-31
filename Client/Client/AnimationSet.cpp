#include "AnimationSet.h"
#include "GameObject.h"

CAnimationSet::CAnimationSet(float fLength, int nFramesPerSecond, int nKeyFrameTransforms, int nSkinningBones, char* pstrName)
{
	m_fLength = fLength;
	m_nFramesPerSecond = nFramesPerSecond;
	m_pstrAnimationSetName = pstrName;

	m_nKeyFrames = nKeyFrameTransforms;
	m_pfKeyFrameTimes.resize(nKeyFrameTransforms);
	m_ppxmf4x4KeyFrameTransforms.resize(nKeyFrameTransforms);
	for (int i = 0; i < nKeyFrameTransforms; i++) m_ppxmf4x4KeyFrameTransforms[i].resize(nSkinningBones);
}

CAnimationSet::~CAnimationSet()
{
	m_pfKeyFrameTimes.clear();
	for (int i = 0; i < m_nKeyFrames; i++) m_ppxmf4x4KeyFrameTransforms[i].clear();
	m_ppxmf4x4KeyFrameTransforms.clear();
}

XMFLOAT4X4 CAnimationSet::GetSRT(int nBone, float fPosition)
{
	XMFLOAT4X4 xmf4x4Transform = Matrix4x4::Identity();

	for (int i = 0; i < (m_nKeyFrames - 1); i++)
	{
		if ((m_pfKeyFrameTimes[i] <= fPosition) && (fPosition < m_pfKeyFrameTimes[i + 1]))
		{
			float t = (fPosition - m_pfKeyFrameTimes[i]) / (m_pfKeyFrameTimes[i + 1] - m_pfKeyFrameTimes[i]);
			xmf4x4Transform = Matrix4x4::Interpolate(m_ppxmf4x4KeyFrameTransforms[i][nBone], m_ppxmf4x4KeyFrameTransforms[i + 1][nBone], t);
			break;
		}
	}
	if (fPosition >= m_pfKeyFrameTimes[m_nKeyFrames - 1]) xmf4x4Transform = m_ppxmf4x4KeyFrameTransforms[m_nKeyFrames - 1][nBone];

	return(xmf4x4Transform);
}

/////////////////////////////////////////////////////////////////////////////
//

CAnimationSets::CAnimationSets(int nAnimationSets)
{
	m_nAnimationSets = nAnimationSets;
	m_pAnimationSets.resize(nAnimationSets);
}
