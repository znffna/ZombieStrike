///////////////////////////////////////////////////////////////////////////////
// Date: 2025-03-28
// Transform.cpp : CTransform 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#include "Transform.h"
#include "GameObject.h"
#include "Mesh.h"

///////////////////////////////////////////////////////////////////////////////
//

void CTransform::SetPosition(float fx, float fy, float fz)
{
	m_xmf3Position = DirectX::XMFLOAT3(fx, fy, fz);

	m_xmf4x4Local._41 = fx;
	m_xmf4x4Local._42 = fy;
	m_xmf4x4Local._43 = fz;

	UpdateTransform(nullptr);
}

void CTransform::SetScale(float fx, float fy, float fz)
{
	m_xmf3Scale = DirectX::XMFLOAT3(fx, fy, fz);

	m_xmf4x4Local._11 = fx;
	m_xmf4x4Local._22 = fy;
	m_xmf4x4Local._33 = fz;

	UpdateTransform(nullptr);
}

void CTransform::Move(DirectX::XMFLOAT3 xmf3Shift)
{
	m_xmf3Position = Vector3::Add(m_xmf3Position, xmf3Shift);

	m_xmf4x4Local._41 = m_xmf3Position.x;
	m_xmf4x4Local._42 = m_xmf3Position.y;
	m_xmf4x4Local._43 = m_xmf3Position.z;

	UpdateTransform(nullptr);
}

void CTransform::MoveStrafe(float fDistance)
{
	DirectX::XMFLOAT3 xmf3Right = GetRight();
	DirectX::XMFLOAT3 xmf3Shift = Vector3::ScalarProduct(xmf3Right, fDistance);
	Move(xmf3Shift);
}

void CTransform::MoveUp(float fDistance)
{
	DirectX::XMFLOAT3 xmf3Up = GetUp();
	DirectX::XMFLOAT3 xmf3Shift = Vector3::ScalarProduct(xmf3Up, fDistance);
	Move(xmf3Shift);
}

void CTransform::MoveForward(float fDistance)
{
	DirectX::XMFLOAT3 xmf3Look = GetLook();
	DirectX::XMFLOAT3 xmf3Shift = Vector3::ScalarProduct(xmf3Look, fDistance);
	Move(xmf3Shift);
}

void CTransform::Rotate(float fPitch, float fYaw, float fRoll)
{
	m_xmf3Rotation.x += fPitch;
	m_xmf3Rotation.y += fYaw;
	m_xmf3Rotation.z += fRoll;

	XMMATRIX xmmtxRotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(fPitch), XMConvertToRadians(fYaw), XMConvertToRadians(fRoll));
	m_xmf4x4Local = Matrix4x4::Multiply(xmmtxRotate, m_xmf4x4Local);

	UpdateTransform(nullptr);
}

void CTransform::Rotate(const XMFLOAT3& pxmf3Axis, float fAngle)
{
	XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&pxmf3Axis), XMConvertToRadians(fAngle));
	m_xmf4x4Local = Matrix4x4::Multiply(xmmtxRotate, m_xmf4x4Local);

	m_xmf3Rotation = GetEulerAngles(m_xmf4x4Local, m_xmf3Scale);

	UpdateTransform(nullptr);
}

void CTransform::Rotate(const XMFLOAT4& pxmf4Quaternion)
{
	XMMATRIX xmmtxRotate = XMMatrixRotationQuaternion(XMLoadFloat4(&pxmf4Quaternion));
	m_xmf4x4Local = Matrix4x4::Multiply(xmmtxRotate, m_xmf4x4Local);

	m_xmf3Rotation = GetEulerAngles(m_xmf4x4Local, m_xmf3Scale);

	UpdateTransform(nullptr);
}

XMFLOAT3 CTransform::GetEulerAngles(const XMFLOAT4X4& worldMatrix, const XMFLOAT3& scale) const
{
	// 스케일을 제거한 회전 행렬을 얻기 위해 각 축을 정규화
	XMFLOAT3X3 rotationMatrix;
	rotationMatrix._11 = worldMatrix._11 / scale.x;
	rotationMatrix._12 = worldMatrix._12 / scale.x;
	rotationMatrix._13 = worldMatrix._13 / scale.x;

	rotationMatrix._21 = worldMatrix._21 / scale.y;
	rotationMatrix._22 = worldMatrix._22 / scale.y;
	rotationMatrix._23 = worldMatrix._23 / scale.y;

	rotationMatrix._31 = worldMatrix._31 / scale.z;
	rotationMatrix._32 = worldMatrix._32 / scale.z;
	rotationMatrix._33 = worldMatrix._33 / scale.z;

	// 회전 행렬을 XMVECTOR로 변환
	XMMATRIX rotMat = XMLoadFloat3x3(&rotationMatrix);

	// Euler 각을 추출
	float pitch, yaw, roll;
	yaw = atan2f(rotMat.r[0].m128_f32[2], rotMat.r[2].m128_f32[2]) * RAD_TO_DEG;
	pitch = asinf(-rotMat.r[1].m128_f32[2]) * RAD_TO_DEG;
	roll = atan2f(rotMat.r[1].m128_f32[0], rotMat.r[1].m128_f32[1]) * RAD_TO_DEG;

	return XMFLOAT3(pitch, yaw, roll); // 각도(x,y,z) 값 반환
}

void CTransform::UpdateTransform(const DirectX::XMFLOAT4X4* xmf4x4ParentMatrix)
{
	m_xmf4x4World = (xmf4x4ParentMatrix) ? Matrix4x4::Multiply(m_xmf4x4Local, *xmf4x4ParentMatrix) : m_xmf4x4Local;

#ifdef _WITH_TRANSFORM_HIERARCHY
	for (auto& pChild : m_vecChildTransforms)
	{
		pChild->UpdateTransform(&m_xmf4x4World);
	}
#endif
}

void CTransform::UpdateTransform(const std::shared_ptr<CGameObject> pParentObject)
{
	SetWorldMatrix((pParentObject) ? Matrix4x4::Multiply(m_xmf4x4Local, (pParentObject->GetWorldMatrix())) : m_xmf4x4Local);
#ifdef	_WITH_TRANSFORM_HIERARCHY
	for (auto& pChild : m_vecChildTransforms)
	{
		pChild->UpdateTransform(&m_xmf4x4World);
	}
#endif
}

#ifdef	_WITH_TRANSFORM_HIERARCHY

void CTransformComponent::SetParent(std::shared_ptr<CTransform> pParentTransform)
{
	m_pParentTransform = pParentTransform;
	pParentTransform->SetChild(shared_from_this());
}

void CTransformComponent::SetChild(std::shared_ptr<CTransform> pChildTransform)
{
	m_vecChildTransforms.push_back(pChildTransform);
	pChildTransform->SetParent(shared_from_this());
}

void CTransformComponent::RemoveChild(std::shared_ptr<CTransform> pChildTransform)
{
	auto iter = std::find(m_vecChildTransforms.begin(), m_vecChildTransforms.end(), pChildTransform);
	if (iter != m_vecChildTransforms.end())
	{
		m_vecChildTransforms.erase(iter);
	}
}

#endif