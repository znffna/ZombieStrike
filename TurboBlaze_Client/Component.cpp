///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-30
// Component.cpp : CComponent 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#include "Component.h"
#include "GameObject.h"


CComponent::CComponent(std::shared_ptr<CGameObject>& pOwnerObject)
{
	m_pOwnerGameObject = pOwnerObject;
	m_bActive = true;
}

CComponent::~CComponent()
{

}

///////////////////////////////////////////////////////////////////////////////
//

CTransform::CTransform(std::shared_ptr<CGameObject>& pOwnerObject)
	: CComponent(pOwnerObject)
{
	m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_xmf4Rotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_xmf3Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
}

CTransform::~CTransform()
{

}

// 사원수로 회전
inline void CTransform::Rotate(const XMFLOAT4& xmf4RotateQuaternion) {
	XMVECTOR currentRotation = XMLoadFloat4(&m_xmf4Rotation); // 현재 회전
	XMVECTOR newRotation = XMLoadFloat4(&xmf4RotateQuaternion); // 새 회전 (사원수)

	// 현재 회전에 새 회전 값을 곱함
	XMVECTOR result = XMQuaternionMultiply(currentRotation, newRotation);

	// 결과를 다시 m_xmf4Rotation에 저장
	XMStoreFloat4(&m_xmf4Rotation, result);
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
}

// 로컬 행렬 반환
inline XMFLOAT4X4 CTransform::GetLocalMatrix() {
	XMFLOAT4X4 xmf4x4Matrix;
	XMMATRIX xmmtxScale = XMMatrixScaling(m_xmf3Scale.x, m_xmf3Scale.y, m_xmf3Scale.z);
	XMMATRIX xmmtxRotation = XMMatrixRotationQuaternion(XMLoadFloat4(&m_xmf4Rotation));
	XMMATRIX xmmtxTranslation = XMMatrixTranslation(m_xmf3Position.x, m_xmf3Position.y, m_xmf3Position.z);

	XMStoreFloat4x4(&xmf4x4Matrix, xmmtxScale * xmmtxRotation * xmmtxTranslation);

	return xmf4x4Matrix;
}

inline void CTransform::UpdateWorldMatrix(const CTransform* transformParent) {
	// 부모 Transform가 없다면, 로컬 행렬을 그대로 사용한다.
	if (nullptr == transformParent) {
		XMStoreFloat4x4(&m_xmf4x4WorldMatrix, XMLoadFloat4x4(&GetLocalMatrix()));
	}
	else
	{
		XMMATRIX xmmtxLocal = XMLoadFloat4x4(&GetLocalMatrix());
		XMMATRIX xmmtxParent = XMLoadFloat4x4(&transformParent->m_xmf4x4WorldMatrix);

		XMStoreFloat4x4(&m_xmf4x4WorldMatrix, xmmtxLocal * xmmtxParent);
	}

	// 자식 Transform에게도 업데이트를 요청한다.
	for (auto& pChildTransform : m_vecpChildTransforms) {
		pChildTransform->UpdateWorldMatrix(this);
	}
}

