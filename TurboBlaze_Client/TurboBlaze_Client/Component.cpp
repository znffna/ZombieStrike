///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-30
// Component.cpp : CComponent Ŭ������ ���� ����
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

// ������� ȸ��
inline void CTransform::Rotate(const XMFLOAT4& xmf4RotateQuaternion) {
	XMVECTOR currentRotation = XMLoadFloat4(&m_xmf4Rotation); // ���� ȸ��
	XMVECTOR newRotation = XMLoadFloat4(&xmf4RotateQuaternion); // �� ȸ�� (�����)

	// ���� ȸ���� �� ȸ�� ���� ����
	XMVECTOR result = XMQuaternionMultiply(currentRotation, newRotation);

	// ����� �ٽ� m_xmf4Rotation�� ����
	XMStoreFloat4(&m_xmf4Rotation, result);

	// ���� ����� �ٽ� ����ؾ� �Ѵ�.
	SetLocalDirty();
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

	// ���� ����� �ٽ� ����ؾ� �Ѵ�.
	SetLocalDirty();
}

// ���� ��� ��ȯ
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
	// �θ� Transform�� ���ٸ�, ���� ����� �״�� ����Ѵ�.
	if(m_bWorldDirty)
	{
		XMFLOAT4X4 xmf4x4Local = GetLocalMatrix();

		if (nullptr == m_pParentTransform) {
			// ���� ����� ���� ��ķ� �����Ѵ�.
			XMStoreFloat4x4(&m_xmf4x4WorldMatrix, XMLoadFloat4x4(&xmf4x4Local));
		}
		else
		{
			// �θ� Transform�� �ִٸ�, �θ� Transform�� ���� ��İ� ���� ����� ���Ͽ� ���� ����� ����Ѵ�.
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
	

