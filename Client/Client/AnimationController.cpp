#include "AnimationController.h"

#include "Mesh.h"
#include "GameObject.h"
#include "AnimationSet.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

float CAnimationTrack::UpdatePosition(float fTrackPosition, float fElapsedTime, float fAnimationLength)
{
	float fTrackElapsedTime = fElapsedTime * m_fSpeed;
	switch (m_nType)
	{
	case ANIMATION_TYPE_LOOP:
	{
		if (m_fPosition < 0.0f) m_fPosition = 0.0f;
		else
		{
			m_fPosition = fTrackPosition + fTrackElapsedTime;
			if (m_fPosition > fAnimationLength)
			{
				m_fPosition = -ANIMATION_CALLBACK_EPSILON;
				return(fAnimationLength);
			}
		}
		//			m_fPosition = fmod(fTrackPosition, m_pfKeyFrameTimes[m_nKeyFrames-1]); // m_fPosition = fTrackPosition - int(fTrackPosition / m_pfKeyFrameTimes[m_nKeyFrames-1]) * m_pfKeyFrameTimes[m_nKeyFrames-1];
		//			m_fPosition = fmod(fTrackPosition, m_fLength); //if (m_fPosition < 0) m_fPosition += m_fLength;
		//			m_fPosition = fTrackPosition - int(fTrackPosition / m_fLength) * m_fLength;
		break;
	}
	case ANIMATION_TYPE_ONCE:
		m_fPosition = fTrackPosition + fTrackElapsedTime;
		if (m_fPosition > fAnimationLength) m_fPosition = fAnimationLength;
		break;
	case ANIMATION_TYPE_PINGPONG:
		break;
	}

	return(m_fPosition);
}

void CAnimationTrack::SetCallbackKeys(int nCallbackKeys)
{
	m_nCallbackKeys = nCallbackKeys;
	m_pCallbackKeys.resize(nCallbackKeys);
}

void CAnimationTrack::SetCallbackKey(int nKeyIndex, float fTime, void* pData)
{
	m_pCallbackKeys[nKeyIndex].m_fTime = fTime;
	m_pCallbackKeys[nKeyIndex].m_pCallbackData = pData;
}

void CAnimationTrack::SetAnimationCallbackHandler(std::shared_ptr<CAnimationCallbackHandler> pCallbackHandler)
{
	m_pAnimationCallbackHandler = pCallbackHandler;
}

void CAnimationTrack::HandleCallback()
{
	if (m_pAnimationCallbackHandler)
	{
		for (int i = 0; i < m_nCallbackKeys; i++)
		{
			if (::IsEqual(m_pCallbackKeys[i].m_fTime, m_fPosition, ANIMATION_CALLBACK_EPSILON))
			{
				if (m_pCallbackKeys[i].m_pCallbackData) m_pAnimationCallbackHandler->HandleCallback(m_pCallbackKeys[i].m_pCallbackData, m_fPosition);
				break;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

CAnimationController::CAnimationController(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, int nAnimationTracks, std::shared_ptr<CLoadedModelInfo> pModel)
{
	m_nAnimationTracks = nAnimationTracks;
	m_pAnimationTracks.resize(nAnimationTracks);

	m_pModelRootObject = pModel->m_pModelRootObject;
	m_nSkinnedMeshes = pModel->m_nSkinnedMeshes;
	m_ppSkinnedMeshes = pModel->m_ppSkinnedMeshes;

	m_pAnimationSets = pModel->m_pAnimationSets;

	m_ppd3dcbSkinningBoneTransforms.resize(m_nSkinnedMeshes);
	m_ppcbxmf4x4MappedSkinningBoneTransforms.resize(m_nSkinnedMeshes);

	UINT ncbElementBytes = (((sizeof(XMFLOAT4X4) * SKINNED_ANIMATION_BONES) + 255) & ~255); //256ÀÇ ¹è¼ö
	for (int i = 0; i < m_nSkinnedMeshes; i++)
	{
		m_ppd3dcbSkinningBoneTransforms[i] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
		m_ppd3dcbSkinningBoneTransforms[i]->Map(0, NULL, (void**)&m_ppcbxmf4x4MappedSkinningBoneTransforms[i]);

		std::wstring name = L"Skinning Bone Transforms [" + std::to_wstring(i) + L"]";
		m_ppd3dcbSkinningBoneTransforms[i]->SetName(name.c_str());
	}

	for (int i = 0; i < m_nAnimationTracks; i++)
	{
		m_pAnimationTracks[i].SetAnimationSet(0);
		m_pAnimationTracks[i].SetCallbackKeys(0);
		m_pAnimationTracks[i].SetAnimationCallbackHandler(NULL);
	}
}

CAnimationController::~CAnimationController()
{
	for (int i = 0; i < m_nSkinnedMeshes; i++)
	{
		if (m_ppd3dcbSkinningBoneTransforms[i]) {
			m_ppd3dcbSkinningBoneTransforms[i]->Unmap(0, NULL);
			m_ppd3dcbSkinningBoneTransforms[i].Reset();
		}
		if (m_ppcbxmf4x4MappedSkinningBoneTransforms[i]) m_ppcbxmf4x4MappedSkinningBoneTransforms[i] = NULL;
	}
}

void CAnimationController::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	for (int i = 0; i < m_nSkinnedMeshes; i++)
	{
		m_ppSkinnedMeshes[i]->m_pd3dcbSkinningBoneTransforms = m_ppd3dcbSkinningBoneTransforms[i];
		m_ppSkinnedMeshes[i]->m_pcbxmf4x4MappedSkinningBoneTransforms = m_ppcbxmf4x4MappedSkinningBoneTransforms[i];
	}
}

void CAnimationController::AdvanceTime(float fElapsedTime, CGameObject* pRootGameObject)
{
	m_fTime += fElapsedTime;
	if (false == m_pAnimationTracks.empty())
	{
#ifdef _WITH_OBJECT_TRANSFORM
		for (int j = 0; j < m_pAnimationSets->m_nBoneFrames; j++) m_pAnimationSets->m_ppBoneFrameCaches[j]->m_xmf4x4Local = Matrix4x4::Zero();
#else
		for (int j = 0; j < m_pAnimationSets->m_nBoneFrames; j++) m_pAnimationSets->m_ppBoneFrameCaches[j]->SetLocalMatrix(Matrix4x4::Zero());
#endif

		for (int k = 0; k < m_nAnimationTracks; k++)
		{
			if (m_pAnimationTracks[k].m_bEnable)
			{
				std::shared_ptr<CAnimationSet> pAnimationSet = m_pAnimationSets->m_pAnimationSets[m_pAnimationTracks[k].m_nAnimationSet];
				float fPosition = m_pAnimationTracks[k].UpdatePosition(m_pAnimationTracks[k].m_fPosition, fElapsedTime, pAnimationSet->m_fLength);
				for (int j = 0; j < m_pAnimationSets->m_nBoneFrames; j++)
				{
#ifdef _WITH_OBJECT_TRANSFORM
					XMFLOAT4X4 xmf4x4Transform = m_pAnimationSets->m_ppBoneFrameCaches[j]->m_xmf4x4Local;
#else
					XMFLOAT4X4 xmf4x4Transform = m_pAnimationSets->m_ppBoneFrameCaches[j]->GetLocalMatrix();
#endif
					XMFLOAT4X4 xmf4x4TrackTransform = pAnimationSet->GetSRT(j, fPosition);
					xmf4x4Transform = Matrix4x4::Add(xmf4x4Transform, Matrix4x4::Scale(xmf4x4TrackTransform, m_pAnimationTracks[k].m_fWeight));
#ifdef _WITH_OBJECT_TRANSFORM
					m_pAnimationSets->m_ppBoneFrameCaches[j]->m_xmf4x4Local = xmf4x4Transform;
#else
					m_pAnimationSets->m_ppBoneFrameCaches[j]->SetLocalMatrix(xmf4x4Transform);
#endif
				}
				m_pAnimationTracks[k].HandleCallback();
			}
		}
#ifdef _WITH_DEBUG_ANIMATION_UPDATE
#ifdef _WITH_OBJECT_TRANSFORM
		for (int j = 0; j < m_pAnimationSets->m_nBoneFrames; j++)
		{
			XMFLOAT4X4 xmf4x4Transform = m_pAnimationSets->m_ppBoneFrameCaches[j]->m_xmf4x4Local;

			// Print Matrix
			TCHAR pstrDebug[256] = { 0 };
			_stprintf_s(pstrDebug, 256, _T("Bone Frame %d\n"), j);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("%f %f %f %f\n"), xmf4x4Transform._11, xmf4x4Transform._12, xmf4x4Transform._13, xmf4x4Transform._14);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("%f %f %f %f\n"), xmf4x4Transform._21, xmf4x4Transform._22, xmf4x4Transform._23, xmf4x4Transform._24);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("%f %f %f %f\n"), xmf4x4Transform._31, xmf4x4Transform._32, xmf4x4Transform._33, xmf4x4Transform._34);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("%f %f %f %f\n"), xmf4x4Transform._41, xmf4x4Transform._42, xmf4x4Transform._43, xmf4x4Transform._44);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("----------------\n"));
		}
#else
		for (int j = 0; j < m_pAnimationSets->m_nBoneFrames; j++)
		{
			XMFLOAT4X4 xmf4x4Transform = m_pAnimationSets->m_ppBoneFrameCaches[j]->GetLocalMatrix();

			// Print Matrix
			TCHAR pstrDebug[256] = { 0 };
			_stprintf_s(pstrDebug, 256, _T("Bone Frame %d\n"), j);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("%f %f %f %f\n"), xmf4x4Transform._11, xmf4x4Transform._12, xmf4x4Transform._13, xmf4x4Transform._14);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("%f %f %f %f\n"), xmf4x4Transform._21, xmf4x4Transform._22, xmf4x4Transform._23, xmf4x4Transform._24);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("%f %f %f %f\n"), xmf4x4Transform._31, xmf4x4Transform._32, xmf4x4Transform._33, xmf4x4Transform._34);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("%f %f %f %f\n"), xmf4x4Transform._41, xmf4x4Transform._42, xmf4x4Transform._43, xmf4x4Transform._44);
			OutputDebugString(pstrDebug);
			_stprintf_s(pstrDebug, 256, _T("----------------\n"));
		}
#endif // _WITH_OBJECT_TRANSFORM/

#endif // _WITH_DEBUG_ANIMATION_UPDATE/

		pRootGameObject->UpdateTransform(NULL);

		OnRootMotion(pRootGameObject);
		OnAnimationIK(pRootGameObject);
	}
}
