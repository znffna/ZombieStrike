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
