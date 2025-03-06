///////////////////////////////////////////////////////////////////////////////
// Date: 2025-02-05
// Camera.cpp : CCamera 클래스의 소스 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#include "Camera.h"

CCamera::CCamera()
{

}

CCamera::~CCamera()
{
}

void CCamera::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(CB_CAMERA_INFO) + 255) & ~255); //256의 배수
	m_pd3dcbCamera = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbCamera->Map(0, NULL, (void**)&m_pcbMappedCamera);
}

void CCamera::ReleaseShaderVariables()
{
	if (m_pd3dcbCamera) m_pd3dcbCamera->Unmap(0, NULL);
	m_pd3dcbCamera.Reset();
}

void CCamera::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	// Update the Constant Buffer
	XMFLOAT4X4 xmf4x4View;
	XMStoreFloat4x4(&xmf4x4View, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4View)));

	// gmtxView
	::memcpy(&m_pcbMappedCamera->m_xmf4x4View, &xmf4x4View, sizeof(XMFLOAT4X4));
	// gmtxInvView
	XMStoreFloat4x4(&m_pcbMappedCamera->m_xmf4x4InverseView, XMMatrixTranspose(XMMatrixInverse(NULL, XMLoadFloat4x4(&m_xmf4x4View))));

	XMFLOAT4X4 xmf4x4Projection;
	XMStoreFloat4x4(&xmf4x4Projection, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4Projection)));
	// gmtxProjection
	::memcpy(&m_pcbMappedCamera->m_xmf4x4Projection, &xmf4x4Projection, sizeof(XMFLOAT4X4));
	// gmtxInvProjection
	XMStoreFloat4x4(&m_pcbMappedCamera->m_xmf4x4InvProjection, XMMatrixTranspose(XMMatrixInverse(NULL, XMLoadFloat4x4(&m_xmf4x4Projection))));

	// gvCameraPosition
	//::memcpy(&m_pcbMappedCamera->m_xmf3Position, &m_xmf3Position, sizeof(XMFLOAT3));

	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = m_pd3dcbCamera->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(ROOT_PARAMETER_CAMERA, d3dGpuVirtualAddress);
}

void CCamera::SetViewport(int xTopLeft, int yTopLeft, int nWidth, int nHeight, float fMinZ, float fMaxZ)
{
	m_d3dViewport.TopLeftX = (float)xTopLeft;
	m_d3dViewport.TopLeftY = (float)yTopLeft;
	m_d3dViewport.Width = (float)nWidth;
	m_d3dViewport.Height = (float)nHeight;
	m_d3dViewport.MinDepth = fMinZ;
	m_d3dViewport.MaxDepth = fMaxZ;
}

void CCamera::SetScissorRect(LONG xLeft, LONG yTop, LONG xRight, LONG yBottom)
{
	m_d3dScissorRect.left = xLeft;
	m_d3dScissorRect.top = yTop;
	m_d3dScissorRect.right = xRight;
	m_d3dScissorRect.bottom = yBottom;
}

void CCamera::SetViewportsAndScissorRects(ID3D12GraphicsCommandList* pd3dCommandList)
{
	pd3dCommandList->RSSetViewports(1, &m_d3dViewport);
	pd3dCommandList->RSSetScissorRects(1, &m_d3dScissorRect);
}

void CCamera::GenerateViewMatrix()
{
	m_xmf4x4View = Matrix4x4::LookAtLH(m_xmf3Position, m_xmf3Look, m_xmf3Up);
}

void CCamera::GenerateViewMatrix(XMFLOAT3 xmf3Position, XMFLOAT3 xmf3LookAt, XMFLOAT3 xmf3Up)
{
	m_xmf3Position = xmf3Position;
	m_xmf3Look = xmf3LookAt;
	m_xmf3Up = xmf3Up;

	GenerateViewMatrix();
}

void CCamera::RegenerateViewMatrix()
{
	m_xmf3Look = Vector3::Normalize(m_xmf3Look);
	m_xmf3Right = Vector3::CrossProduct(m_xmf3Up, m_xmf3Look, true);
	m_xmf3Up = Vector3::CrossProduct(m_xmf3Look, m_xmf3Right, true);

	m_xmf4x4View._11 = m_xmf3Right.x; m_xmf4x4View._12 = m_xmf3Up.x; m_xmf4x4View._13 = m_xmf3Look.x;
	m_xmf4x4View._21 = m_xmf3Right.y; m_xmf4x4View._22 = m_xmf3Up.y; m_xmf4x4View._23 = m_xmf3Look.y;
	m_xmf4x4View._31 = m_xmf3Right.z; m_xmf4x4View._32 = m_xmf3Up.z; m_xmf4x4View._33 = m_xmf3Look.z;
	m_xmf4x4View._41 = -Vector3::DotProduct(m_xmf3Position, m_xmf3Right);
	m_xmf4x4View._42 = -Vector3::DotProduct(m_xmf3Position, m_xmf3Up);
	m_xmf4x4View._43 = -Vector3::DotProduct(m_xmf3Position, m_xmf3Look);
}

void CCamera::GenerateProjectionMatrix(float aspectRatio, float fov, float nearZ, float farZ)
{
	m_fAspectRatio = aspectRatio;
	m_fFovAngle = fov;
	m_fNearZ = nearZ;
	m_fFarZ = farZ;

	m_xmf4x4Projection = Matrix4x4::PerspectiveFovLH(XMConvertToRadians(m_fFovAngle), m_fAspectRatio, m_fNearZ, m_fFarZ);
}

void CCamera::Rotate(float x, float y, float z)
{
	/* 
	{
		//// 다음 코드는 
		XMMATRIX xmmtxRotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(x), XMConvertToRadians(y), XMConvertToRadians(z));
		m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
	}
	*/

	/*
	{  // 이는 1개의 축에 대한 변경시엔 문제가 없지만, 3개의 축에 대한 변경시 문제가 발생한다.
		if (x != 0.0f) {
			XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Right), XMConvertToRadians(x));
			m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
			m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
			m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);

			fPitch += x;
			if (fPitch > 90.0f) fPitch -= 90.0f;
			if (fPitch < -90.0f) fPitch += 90.0f;
		}
		if (y != 0.0f) {
			XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Up), XMConvertToRadians(y));
			m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
			m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
			m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);

			fYaw += y;
			if (fYaw > 90.0f) fYaw -= 90.0f;
			if (fYaw < -90.0f) fYaw += 90.0f;
		}
		if (z != 0.0f) {
			XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Look), XMConvertToRadians(z));
			m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
			m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
			m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);

			fRoll += z;
			if (fRoll > 90.0f) fRoll -= 90.0f;
			if (fRoll < -90.0f) fRoll += 90.0f;
		}
	}
	*/

	{
		fPitch += x;
		if (fPitch > 180.0f) fPitch -= 360.0f;
		if (fPitch <= -180.0f) fPitch += 360.0f;

		/*
		// 위아래 회전을 제한(Up vector를 월드 Up으로 고정시 사용)
		if (fPitch >= 90.0f) {
			x = x - (fPitch - 89.9f); fPitch = 89.9f;
		}
		if (fPitch <= -90.0f) {
			x = x - (fPitch + 89.9f); fPitch = -89.9f;
		}
		*/

		fYaw += y;
		if (fYaw > 180.0f) fYaw -= 360.0f;
		if (fYaw <= -180.0f) fYaw += 360.0f;

		fRoll += z;
		if (fRoll > 180.0f) fRoll -= 360.0f;
		if (fRoll <= -180.0f) fRoll += 360.0f;

		XMVECTOR qCurrent = XMLoadFloat4(&m_xmf4Rotation);

		XMVECTOR qX = XMQuaternionRotationAxis(XMLoadFloat3(&m_xmf3Right), XMConvertToRadians(x));
		XMVECTOR qY = XMQuaternionRotationAxis(XMLoadFloat3(&m_xmf3Up), XMConvertToRadians(y));
		XMVECTOR qZ = XMQuaternionRotationAxis(XMLoadFloat3(&m_xmf3Look), XMConvertToRadians(z));

		qCurrent = XMQuaternionMultiply(qZ, qCurrent);
		qCurrent = XMQuaternionMultiply(qY, qCurrent);
		qCurrent = XMQuaternionMultiply(qX, qCurrent);
		qCurrent = XMQuaternionNormalize(qCurrent); // 정규화

		XMStoreFloat4(&m_xmf4Rotation, qCurrent);

		XMVECTOR vRight = XMLoadFloat3(&m_xmf3Right);
		XMVECTOR vUp = XMLoadFloat3(&m_xmf3Up);
		XMVECTOR vLook = XMLoadFloat3(&m_xmf3Look);

		vRight = XMVector3Rotate(vRight, qCurrent);
		vUp = XMVector3Rotate(vUp, qCurrent);
		vLook = XMVector3Rotate(vLook, qCurrent);

		XMStoreFloat3(&m_xmf3Right, vRight);
		XMStoreFloat3(&m_xmf3Up, vUp);
		XMStoreFloat3(&m_xmf3Look, vLook);

	}
}
