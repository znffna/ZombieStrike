///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-30
// Component.cpp : CComponent Ŭ������ ���� ����
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

// ������� ȸ��
inline void CTransform::Rotate(const XMFLOAT4& xmf4RotateQuaternion) {
	XMVECTOR currentRotation = XMLoadFloat4(&m_xmf4Rotation); // ���� ȸ��
	XMVECTOR newRotation = XMLoadFloat4(&xmf4RotateQuaternion); // �� ȸ�� (�����)

	// ���� ȸ���� �� ȸ�� ���� ����
	XMVECTOR result = XMQuaternionMultiply(currentRotation, newRotation);

	// ����� �ٽ� m_xmf4Rotation�� ����
	XMStoreFloat4(&m_xmf4Rotation, result);
}

// ���Ϸ� ������ ȸ��
inline void CTransform::Rotate(const XMFLOAT3& eulerAngles) {
	// ���Ϸ� ����(Euler Angles)�� ȸ�� (Roll, Pitch, Yaw)
	XMVECTOR currentRotation = XMLoadFloat4(&m_xmf4Rotation); // ���� ȸ��
	XMVECTOR newRotation = XMQuaternionRotationRollPitchYaw(eulerAngles.x, eulerAngles.y, eulerAngles.z);

	// ���� ȸ���� �� ȸ�� ���� ����
	XMVECTOR result = XMQuaternionMultiply(currentRotation, newRotation);

	// ����� �ٽ� m_xmf4Rotation�� ����
	XMStoreFloat4(&m_xmf4Rotation, result);
}

// ���� ��� ��ȯ
inline XMFLOAT4X4 CTransform::GetLocalMatrix() {
	XMFLOAT4X4 xmf4x4Matrix;
	XMMATRIX xmmtxScale = XMMatrixScaling(m_xmf3Scale.x, m_xmf3Scale.y, m_xmf3Scale.z);
	XMMATRIX xmmtxRotation = XMMatrixRotationQuaternion(XMLoadFloat4(&m_xmf4Rotation));
	XMMATRIX xmmtxTranslation = XMMatrixTranslation(m_xmf3Position.x, m_xmf3Position.y, m_xmf3Position.z);

	XMStoreFloat4x4(&xmf4x4Matrix, xmmtxScale * xmmtxRotation * xmmtxTranslation);

	return xmf4x4Matrix;
}

inline void CTransform::UpdateWorldMatrix(const CTransform* transformParent) {
	// �θ� Transform�� ���ٸ�, ���� ����� �״�� ����Ѵ�.
	if (nullptr == transformParent) {
		XMStoreFloat4x4(&m_xmf4x4WorldMatrix, XMLoadFloat4x4(&GetLocalMatrix()));
	}
	else
	{
		XMMATRIX xmmtxLocal = XMLoadFloat4x4(&GetLocalMatrix());
		XMMATRIX xmmtxParent = XMLoadFloat4x4(&transformParent->m_xmf4x4WorldMatrix);

		XMStoreFloat4x4(&m_xmf4x4WorldMatrix, xmmtxLocal * xmmtxParent);
	}

	// �ڽ� Transform���Ե� ������Ʈ�� ��û�Ѵ�.
	for (auto& pChildTransform : m_vecpChildTransforms) {
		pChildTransform->UpdateWorldMatrix(this);
	}
}

