#include "AnimationController.h"

#include "Mesh.h"
#include "GameObject.h"
#include "AnimationSet.h"

#include "Camera.h"
#include "Player.h"

#include "ResourceManager.h"

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

#define MAX_INSTANCE_SKINNED_OBJECT		512


void CGlobalBoneTransformManager::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pCommandList)
{
	//Create Shader Buffers for Skinned Meshes [Global]
	// Upload Buffer for Global Bone Transform
	UINT ncbElementBytes = (((sizeof(XMFLOAT4X4) * SKINNED_ANIMATION_BONES * MAX_INSTANCE_SKINNED_OBJECT) + 255) & ~255); //256의 배수
	m_pd3dGlobalBoneTransformBuffer = ::CreateBufferResource(pd3dDevice, nullptr, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
	std::wstring name = L"Global Skinning Bone Transforms Upload Buffer";
	m_pd3dGlobalBoneTransformBuffer->SetName(name.c_str());

	// Map the buffer
	m_pd3dGlobalBoneTransformBuffer->Map(0, NULL, (void**)&m_pMappedGlobalBoneTransforms);

	// Default Buffer for Global Bone Transform
	m_pd3dDefaultGlobalBoneTransformBuffer = ::CreateBufferResource(pd3dDevice, nullptr, NULL, ncbElementBytes, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
	name = L"Global Skinning Bone Transforms Default Buffer";
	m_pd3dGlobalBoneTransformBuffer->SetName(name.c_str());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = SKINNED_ANIMATION_BONES * MAX_INSTANCE_SKINNED_OBJECT;
	srvDesc.Buffer.StructureByteStride = sizeof(XMFLOAT4X4);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	gpuhandle = CResourceManager::Instance().CreateShaderResourceView(pd3dDevice, m_pd3dDefaultGlobalBoneTransformBuffer.Get(), srvDesc);
	m_nMaxBoneOffset = ncbElementBytes;

}

void CGlobalBoneTransformManager::Shutdown()
{
	if (m_pd3dGlobalBoneTransformBuffer)
	{
		m_pd3dGlobalBoneTransformBuffer->Unmap(0, NULL);
		m_pd3dGlobalBoneTransformBuffer.Reset();
	}
	if (m_pd3dDefaultGlobalBoneTransformBuffer)
	{
		m_pd3dDefaultGlobalBoneTransformBuffer.Reset();
	}
}

UINT CGlobalBoneTransformManager::AllocateBoneRange(int nBoneCount)
{
	UINT nOffset = m_nCurrentBoneOffset;
	m_nCurrentBoneOffset += nBoneCount;
	return nOffset;
}

void CGlobalBoneTransformManager::WriteBoneTransforms(UINT offset, const XMFLOAT4X4* pTransforms, UINT count)
{
	memcpy(&m_pMappedGlobalBoneTransforms[offset], pTransforms, sizeof(XMFLOAT4X4) * count);

#ifdef _DEBUG
	{
		std::string debugOutput = "WriteBoneTransforms called : offset=" + std::to_string(offset) + ", count=" + std::to_string(count) + "\n";
		//OutputDebugStringA(debugOutput.c_str());
	}
#endif
}

void CGlobalBoneTransformManager::PrepareRender(ID3D12GraphicsCommandList* pd3dCommandList)
{
	// Set Resource Barrier for Copy
	D3D12_RESOURCE_BARRIER d3dResourceBarrier;
	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource = m_pd3dDefaultGlobalBoneTransformBuffer.Get();
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	// Copy from Upload Buffer to Default Buffer
	pd3dCommandList->CopyResource(m_pd3dDefaultGlobalBoneTransformBuffer.Get(), m_pd3dGlobalBoneTransformBuffer.Get());

	// Set Resource Barrier for Vertex and Constant Buffer
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	// 다음 프레임을위해 Index 초기화
	m_nPrevBoneOffset = m_nCurrentBoneOffset;
	m_nCurrentBoneOffset = 0;

	pd3dCommandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_SKINNED_BONE_TRANSFORM, gpuhandle); // Set SRV for Global Bone Transform Buffer
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

void CAnimationController::Clear()
{
	m_fTime = 0.0f;
	for (int i = 0; i < m_nSkinnedMeshes; i++)
	{
		//if (m_ppd3dcbSkinningBoneTransforms[i]) m_ppd3dcbSkinningBoneTransforms[i]->Release();
		//if (m_ppcbxmf4x4MappedSkinningBoneTransforms[i]) m_ppcbxmf4x4MappedSkinningBoneTransforms[i] = NULL;
	}

	m_pModelRootObject = NULL;
	m_nSkinnedMeshes = 0;
	m_ppSkinnedMeshes.clear();
	m_pAnimationSets = NULL;
	m_nAnimationTracks = 0;
	m_pAnimationTracks.clear();

	UpperPose = IDLE;
	BasePose = IDLE;
}

void CAnimationController::SetModel(CLoadedModelInfo* pModel)
{
	Clear();

	m_pModelRootObject = pModel->m_pModelRootObject;
	m_nSkinnedMeshes = pModel->m_nSkinnedMeshes;
	m_ppSkinnedMeshes = pModel->m_ppSkinnedMeshes;

	m_pAnimationSets = pModel->m_pAnimationSets;

	m_vSkinnedMeshBoneOffsets.resize(m_nSkinnedMeshes);

	m_xmf4x4SkinningBoneTransforms.resize(m_nSkinnedMeshes);
	for(int i = 0; i < m_nSkinnedMeshes; i++)
	{
		m_xmf4x4SkinningBoneTransforms[i].resize(m_ppSkinnedMeshes[i]->m_nSkinningBones);
	}

	m_nAnimationTracks = m_pAnimationSets->m_nAnimationSets;

	m_pAnimationTracks.resize(m_nAnimationTracks);
	for (int i = 0; i < m_nAnimationTracks; i++)
	{
		m_pAnimationTracks[i].SetAnimationSet(i);
		m_pAnimationTracks[i].SetCallbackKeys(0);
		m_pAnimationTracks[i].SetAnimationCallbackHandler(NULL);
		m_pAnimationTracks[i].SetEnable(false);
	}
	m_pAnimationTracks[0].SetEnable(true);
}

CAnimationController::~CAnimationController()
{
	OnDestroy();
}

void CAnimationController::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	//for (int i = 0; i < m_nSkinnedMeshes; i++)
	//{
	//	// 실제 저장해놓은 BoneMatrix를 GPU에 복사
	//	// 저장해놓는것은 AdvanceTime에서 수행
	//	memcpy(m_ppcbxmf4x4MappedSkinningBoneTransforms[i],	m_xmf4x4SkinningBoneTransforms[i].data(), sizeof(XMFLOAT4X4) * m_xmf4x4SkinningBoneTransforms[i].size());

	//	// SKinnedMesh가 참조할 수 있도록 설정
	//	m_ppSkinnedMeshes[i]->m_pd3dcbSkinningBoneTransforms = m_ppd3dcbSkinningBoneTransforms[i].Get();
	//	m_ppSkinnedMeshes[i]->m_pcbxmf4x4MappedSkinningBoneTransforms = m_ppcbxmf4x4MappedSkinningBoneTransforms[i];
	//}
	for (int i = 0; i < m_nSkinnedMeshes; i++)
	{
		m_ppSkinnedMeshes[i]->m_nSkinningBoneTransformsOffset = m_vSkinnedMeshBoneOffsets[i];
		m_ppSkinnedMeshes[i]->m_pd3dcbSkinningBoneTransforms = CGlobalBoneTransformManager::Instance().GetBoneTransformBuffer();
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

		int nLowerState = static_cast<int>(BasePose);
		int nUpperState = static_cast<int>(UpperPose);

		if(nLowerState != nUpperState){
			// 하체 우선 적용
			{
				std::shared_ptr<CAnimationSet> pAnimationSet = m_pAnimationSets->m_pAnimationSets[m_pAnimationTracks[nLowerState].m_nAnimationSet];
				float fPosition = m_pAnimationTracks[nLowerState].UpdatePosition(m_pAnimationTracks[nLowerState].m_fPosition, fElapsedTime, pAnimationSet->m_fLength);
				for (int j = 0; j < m_pAnimationSets->m_nBoneFrames; j++)
				{
#ifdef _WITH_OBJECT_TRANSFORM
					XMFLOAT4X4 xmf4x4Transform = m_pAnimationSets->m_ppBoneFrameCaches[j]->m_xmf4x4Local;
#else
					XMFLOAT4X4 xmf4x4Transform = m_pAnimationSets->m_ppBoneFrameCaches[j]->GetLocalMatrix();
#endif
					XMFLOAT4X4 xmf4x4TrackTransform = pAnimationSet->GetSRT(j, fPosition);
					xmf4x4Transform = Matrix4x4::Add(xmf4x4Transform, Matrix4x4::Scale(xmf4x4TrackTransform, m_pAnimationTracks[nLowerState].m_fWeight));
#ifdef _WITH_OBJECT_TRANSFORM
					m_pAnimationSets->m_ppBoneFrameCaches[j]->m_xmf4x4Local = xmf4x4Transform;
#else
					m_pAnimationSets->m_ppBoneFrameCaches[j]->SetLocalMatrix(xmf4x4Transform);
#endif
				}
				m_pAnimationTracks[nLowerState].HandleCallback();
			}

			{ // 상체 Lerp 적용
				std::shared_ptr<CAnimationSet> pAnimationSet = m_pAnimationSets->m_pAnimationSets[m_pAnimationTracks[nUpperState].m_nAnimationSet];
				float fPosition = m_pAnimationTracks[nUpperState].UpdatePosition(m_pAnimationTracks[nUpperState].m_fPosition, fElapsedTime, pAnimationSet->m_fLength);
				for (int j = 0; j < m_pAnimationSets->m_nBoneFrames; j++)
				{
#ifdef _WITH_OBJECT_TRANSFORM
					XMFLOAT4X4 xmf4x4Transform = m_pAnimationSets->m_ppBoneFrameCaches[j]->m_xmf4x4Local;
#else
					XMFLOAT4X4 xmf4x4Transform = m_pAnimationSets->m_ppBoneFrameCaches[j]->GetLocalMatrix();
#endif
					XMFLOAT4X4 xmf4x4TrackTransform = pAnimationSet->GetSRT(j, fPosition);
					xmf4x4Transform = Matrix4x4::Interpolate(xmf4x4Transform, xmf4x4TrackTransform, m_pAnimationSets->m_ppBoneFrameCaches[j]->GetBoneUpperWeight());
#ifdef _WITH_OBJECT_TRANSFORM
					m_pAnimationSets->m_ppBoneFrameCaches[j]->m_xmf4x4Local = xmf4x4Transform;
#else
					m_pAnimationSets->m_ppBoneFrameCaches[j]->SetLocalMatrix(xmf4x4Transform);
#endif
				}
				m_pAnimationTracks[nUpperState].HandleCallback();
			}
		}
		else
		{
			// 임시	저장 공간 확보
			std::vector<std::vector<XMFLOAT4X4>> localtransform; // AnimationTrack 개수만큼 std::vector 생성
			localtransform.resize(m_nAnimationTracks);
			for(auto& vec : localtransform)
				vec.resize(m_pAnimationSets->m_nBoneFrames, Matrix4x4::Zero()); // 각 std::vector에 BoneFrame 개수만큼 Matrix4x4::Zero()로 초기화된 요소 추가

			// 실제	애니메이션 Pose를 저장
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
						localtransform[k][j] = xmf4x4Transform;
					}
					m_pAnimationTracks[k].HandleCallback();
				}
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

		// pRootGameObject->UpdateTransform(NULL);
		m_pModelRootObject->UpdateTransform(pRootGameObject->GetWorldMatrix());

		// 여기에서 직접 자신이 사용하는 Skinned Mesh들에 대해 UpdateSkinningBoneTransforms를 호출해야 함
		auto& globalbone = CGlobalBoneTransformManager::Instance();
		for (int i = 0; i < m_nSkinnedMeshes; i++)
		{
			auto index = globalbone.AllocateBoneRange(m_ppSkinnedMeshes[i]->m_nSkinningBones);
			m_ppSkinnedMeshes[i]->UpdateSkinningBoneTransforms(m_xmf4x4SkinningBoneTransforms[i]);
			globalbone.WriteBoneTransforms(index, m_xmf4x4SkinningBoneTransforms[i].data(), m_ppSkinnedMeshes[i]->m_nSkinningBones);
			//m_ppSkinnedMeshes[i]->UpdateSkinningBoneTransforms(m_ppcbxmf4x4MappedSkinningBoneTransforms[i]);

			m_vSkinnedMeshBoneOffsets[i] = index;
		}

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

