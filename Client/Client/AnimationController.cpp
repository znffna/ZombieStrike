#include "AnimationController.h"

#include "Mesh.h"
#include "GameObject.h"
#include "AnimationSet.h"

#include "Camera.h"
#include "Player.h"

#include "ResourceManager.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

inline bool CAnimationTrack::CheckTag(const std::string& strTag) const {
	if (strTag == "None") return true;
	else if (strTag == "Upper") return (m_strTag != "Lower");
	else if (strTag == "Lower") return (m_strTag != "Upper");
	return true;
}

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

CAnimationController::CAnimationController()
{
}

void CAnimationController::Clear()
{
	m_fTime = 0.0f;
	for (int i = 0; i < m_nSkinnedMeshes; i++)
	{
		if (m_ppd3dcbSkinningBoneTransforms[i].resource) {
			m_ppd3dcbSkinningBoneTransforms[i].Release();
		}
		if (m_ppcbxmf4x4MappedSkinningBoneTransforms[i]) m_ppcbxmf4x4MappedSkinningBoneTransforms[i] = NULL;
	}

	m_pModelRootObject = NULL;
	m_nSkinnedMeshes = 0;
	m_ppSkinnedMeshes.clear();
	m_pAnimationSets = NULL;
	m_nAnimationTracks = 0;
	m_pAnimationTracks.clear();
}

void CAnimationController::SettingByModel(std::shared_ptr<CLoadedModelInfo>& pModel, int nAnimationTracks)
{
	Clear();

	state = IDLE;

	m_pModelRootObject = pModel->m_pModelRootObject;
	m_nSkinnedMeshes = pModel->m_nSkinnedMeshes;
	m_ppSkinnedMeshes = pModel->m_ppSkinnedMeshes;

	m_pAnimationSets = pModel->m_pAnimationSets;

	m_ppd3dcbSkinningBoneTransforms.resize(m_nSkinnedMeshes);
	m_ppcbxmf4x4MappedSkinningBoneTransforms.resize(m_nSkinnedMeshes);

	if (nAnimationTracks == -1) 
		m_nAnimationTracks = m_pAnimationSets->m_nAnimationSets;
	else m_nAnimationTracks = nAnimationTracks;

	m_pRootMotionObject = pModel->m_pAnimationRootObject;

	UINT ncbElementBytes = (((sizeof(XMFLOAT4X4) * SKINNED_ANIMATION_BONES) + 255) & ~255); //256의 배수
	for (int i = 0; i < m_nSkinnedMeshes; i++)
	{
		m_ppd3dcbSkinningBoneTransforms[i] = CResourceManager::GetInstance().GetSkinningBoneTransforms();
		m_ppd3dcbSkinningBoneTransforms[i].resource->Map(0, NULL, (void**)&m_ppcbxmf4x4MappedSkinningBoneTransforms[i]);

		std::wstring name = L"Skinning Bone Transforms [" + std::to_wstring(i) + L"]";
		m_ppd3dcbSkinningBoneTransforms[i].resource->SetName(name.c_str());
	}

	m_pAnimationTracks.resize(m_nAnimationTracks);
	for (int i = 0; i < m_nAnimationTracks; i++)
	{
		m_pAnimationTracks[i].SetAnimationSet(0);
		m_pAnimationTracks[i].SetCallbackKeys(0);
		m_pAnimationTracks[i].SetAnimationCallbackHandler(NULL);
	}

}

CAnimationController::~CAnimationController()
{
	Clear();
}

void CAnimationController::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	for (int i = 0; i < m_nSkinnedMeshes; i++)
	{
		m_ppSkinnedMeshes[i]->m_pd3dcbSkinningBoneTransforms = m_ppd3dcbSkinningBoneTransforms[i].resource;
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
				
				if(pRootGameObject->GetLayer() == CGameObject::LAYER_PLAYER){
					std::string debugString = pRootGameObject->GetName() + ": Animation Track: " + std::to_string(k) + " Position: " + std::to_string(fPosition) + "\n";
					OutputDebugStringA(debugString.c_str());
				}

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

		ApplyPitchToSpine(pRootGameObject);

		if (m_pRootMotionObject) {
			auto pTransform = m_pRootMotionObject->GetLocalMatrix();
			// Position의 이동을 사용하지 않음
			pTransform._41 = 0.0f;
			//pTransform._42 = 0.0f; y축은 기본 pivot이 발바닥 가운데이기에 y축 이동을 통해 캐릭터를 땅위로 끌어올림.
			pTransform._43 = 0.0f;
			m_pRootMotionObject->SetLocalMatrix(pTransform);
		}

		pRootGameObject->UpdateTransform(NULL);

		OnRootMotion(pRootGameObject);
		OnAnimationIK(pRootGameObject);
	}
}

void CAnimationController::ApplyPitchToSpine(CGameObject* pRootGameObject)
{
	if (auto pCamera = pRootGameObject->GetComponent<CCamera>())
	{
		auto pitch = pRootGameObject->GetPitch();
		auto rotateMatrix = DirectX::XMMatrixRotationX(XMConvertToRadians(pitch / 3.0f));
		if (auto pSpine = pRootGameObject->FindFrame("mixamorig:Spine")) {
			auto pSpineTransform = pSpine->GetLocalMatrix();
			pSpineTransform = Matrix4x4::Multiply(rotateMatrix, pSpineTransform);
			pSpine->SetLocalMatrix(pSpineTransform);
		}
		if (auto pSpine1 = pRootGameObject->FindFrame("mixamorig:Spine1")) {
			auto pSpineTransform = pSpine1->GetLocalMatrix();
			pSpineTransform = Matrix4x4::Multiply(rotateMatrix, pSpineTransform);
			pSpine1->SetLocalMatrix(pSpineTransform);
		}
		if (auto pSpine2 = pRootGameObject->FindFrame("mixamorig:Spine2")) {
			auto pSpineTransform = pSpine2->GetLocalMatrix();
			pSpineTransform = Matrix4x4::Multiply(rotateMatrix, pSpineTransform);
			pSpine2->SetLocalMatrix(pSpineTransform);
		}
	}
}

bool CAnimationController::ChangeState(ANIMATION_STATE state)
{
	return ChangeState(state, 0.0f);
}

bool CAnimationController::ChangeState(ANIMATION_STATE state, float fPosition)
{
	if (state == this->state) return false; // 현재 상태와 같으면 변경하지 않음
	ANIMATION_STATE beforeState = this->state;
	this->state = state;

	/*{
		std::string debugString = "Change Animation State: " + std::to_string(static_cast<int>(beforeState)) + " to " + std::to_string(static_cast<int>(state)) + "\n";
		OutputDebugStringA(debugString.c_str());
	}*/

	SetTrackEnable(beforeState, false);
	SetTrackEnable(state, true);
	SetTrackPosition(state, fPosition);
	return true;
}
