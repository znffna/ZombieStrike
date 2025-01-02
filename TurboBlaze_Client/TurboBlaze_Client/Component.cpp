///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-30
// Component.cpp : CComponent 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#include "Component.h"
#include "GameObject.h"


CComponent::CComponent(const std::shared_ptr<CGameObject>& pOwnerObject)
{
	m_pOwnerGameObject = pOwnerObject;
	m_bActive = true;
}

CComponent::~CComponent()
{

}

///////////////////////////////////////////////////////////////////////////////
//

CTransform::CTransform(const std::shared_ptr<CGameObject>& pOwnerObject)
	: CComponent(pOwnerObject)
{
	m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_xmf4Rotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_xmf3Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);

	XMStoreFloat4x4(&m_xmf4x4LocalMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_xmf4x4WorldMatrix, XMMatrixIdentity());
}

CTransform::~CTransform()
{

}

void CTransform::SetWorldDirty(bool bPropagation)
{
	if (!m_bWorldDirty)
	{
		m_bWorldDirty = true;

		if (bPropagation)
		{
			for (auto& pChildTransform : m_vecpChildTransforms)
			{
				pChildTransform->SetWorldDirty(bPropagation);
			}
		}
	}
}

// 사원수로 회전
inline void CTransform::Rotate(const XMFLOAT4& xmf4RotateQuaternion) {
	XMVECTOR currentRotation = XMLoadFloat4(&m_xmf4Rotation); // 현재 회전
	XMVECTOR newRotation = XMLoadFloat4(&xmf4RotateQuaternion); // 새 회전 (사원수)

	// 현재 회전에 새 회전 값을 곱함
	XMVECTOR result = XMQuaternionMultiply(currentRotation, newRotation);

	// 결과를 다시 m_xmf4Rotation에 저장
	XMStoreFloat4(&m_xmf4Rotation, result);

	// 로컬 행렬을 다시 계산해야 한다.
	SetLocalDirty();
}

// 오일러 각도로 회전
inline void CTransform::Rotate(const XMFLOAT3& eulerAngles) {
	// 오일러 각도(Euler Angles)로 회전 (Roll, Pitch, Yaw)
	XMVECTOR currentRotation = XMLoadFloat4(&m_xmf4Rotation); // 현재 회전
	XMVECTOR newRotation = XMQuaternionRotationRollPitchYaw(eulerAngles.x, eulerAngles.y, eulerAngles.z);

	// 현재 회전에 새 회전 값을 곱함
	XMVECTOR result = XMQuaternionMultiply(currentRotation, newRotation);

	// 결과를 다시 m_xmf4Rotation에 저장
	XMStoreFloat4(&m_xmf4Rotation, result);

	// 로컬 행렬을 다시 계산해야 한다.
	SetLocalDirty();
}

// 로컬 행렬 반환
inline XMFLOAT4X4 CTransform::GetLocalMatrix() {
	if (m_bLocalDirty)
	{
		XMMATRIX xmmtxScale = XMMatrixScaling(m_xmf3Scale.x, m_xmf3Scale.y, m_xmf3Scale.z);
		XMMATRIX xmmtxRotation = XMMatrixRotationQuaternion(XMLoadFloat4(&m_xmf4Rotation));
		XMMATRIX xmmtxTranslation = XMMatrixTranslation(m_xmf3Position.x, m_xmf3Position.y, m_xmf3Position.z);
		XMStoreFloat4x4(&m_xmf4x4LocalMatrix, xmmtxScale * xmmtxRotation * xmmtxTranslation);
		m_bLocalDirty = false;
	}

	return m_xmf4x4LocalMatrix;
}

XMFLOAT4X4 CTransform::GetWorldMatrix()
{
	if (m_bWorldDirty) {
		UpdateWorldMatrix(false);
	}
	return m_xmf4x4WorldMatrix;
}

inline void CTransform::UpdateWorldMatrix(bool bUpdateChild) {
	// 부모 Transform가 없다면, 로컬 행렬을 그대로 사용한다.
	if(m_bWorldDirty)
	{
		XMFLOAT4X4 xmf4x4Local = GetLocalMatrix();

		if (nullptr == m_pParentTransform) {
			// 로컬 행렬을 월드 행렬로 설정한다.
			XMStoreFloat4x4(&m_xmf4x4WorldMatrix, XMLoadFloat4x4(&xmf4x4Local));
		}
		else
		{
			// 부모 Transform이 있다면, 부모 Transform의 월드 행렬과 로컬 행렬을 곱하여 월드 행렬을 계산한다.
			XMFLOAT4X4 xmf4x4ParentWorld = m_pParentTransform->GetWorldMatrix();
			XMMATRIX xmmtxParent = XMLoadFloat4x4(&xmf4x4ParentWorld);
			XMMATRIX xmmtxLocal = XMLoadFloat4x4(&xmf4x4Local);

			XMStoreFloat4x4(&m_xmf4x4WorldMatrix, xmmtxLocal * xmmtxParent);
		}
		m_bWorldDirty = false;
	}

	if (bUpdateChild)
	{
		for (auto& pChildTransform : m_vecpChildTransforms)
		{
			pChildTransform->UpdateWorldMatrixTopDown();
		}
	}
}

void CTransform::SetLocalDirty()
{
	if (!m_bLocalDirty)
	{
		m_bLocalDirty = true;
		if(m_bWorldDirty){
			SetWorldDirty(true);
		}
	}
}
	

