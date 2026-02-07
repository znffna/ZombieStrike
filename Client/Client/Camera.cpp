///////////////////////////////////////////////////////////////////////////////
// Date: 2025-02-05
// Camera.cpp : CCamera 클래스의 소스 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#include "Camera.h"
#include "GameObject.h"

#include "Transform.h"

#include "Scene.h"

void CCamera::Initialize()
{
	CComponent::Initialize();

	// 카메라의 등록
	if(auto pScene = gameObject->GetScene()) pScene->RegisterCamera(this);
}

void CCamera::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(CB_CAMERA_INFO) + 255) & ~255); //256의 배수
	m_pd3dcbCamera = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbCamera->Map(0, NULL, (void**)&m_pcbMappedCamera);

	CComponent::CreateShaderVariables(pd3dDevice, pd3dCommandList);
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
	::memcpy(&m_pcbMappedCamera->m_xmf3Position, &m_xmf3Position, sizeof(XMFLOAT3));

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

void CCamera::GenerateViewMatrix(const XMFLOAT3& xmf3Position, const XMFLOAT3& xmf3LookAt, const XMFLOAT3& xmf3Up)
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
	// 회전량 기록 및 제어
	Clamp(m_fPitch, x, -89.0f, 89.0f); // -90 ~ 90으로 제한
	Normalize(m_fYaw, y, -180.0f, 180.0f); // -180 ~ 180으로 조정
	Normalize(m_fRoll, z, -180.0f, 180.0f); // -180 ~ 180으로 조정

	// 회전 적용
	/*XMVECTOR qShift = XMQuaternionRotationRollPitchYaw(
		XMConvertToRadians(x),
		XMConvertToRadians(y),
		XMConvertToRadians(z)
	);
	qShift = XMQuaternionNormalize(qShift);

	XMVECTOR qCurrent = XMLoadFloat4(&m_xmf4Rotation);
	qCurrent = XMQuaternionMultiply(qShift, qCurrent);
	XMStoreFloat4(&m_xmf4Rotation, qCurrent);

	XMVECTOR vRight = XMLoadFloat3(&m_xmf3Right);
	XMVECTOR vUp = XMLoadFloat3(&m_xmf3Up);
	XMVECTOR vLook = XMLoadFloat3(&m_xmf3Look);

	vRight = XMVector3Rotate(vRight, qShift);
	vUp = XMVector3Rotate(vUp, qShift);
	vLook = XMVector3Rotate(vLook, qShift);

	XMStoreFloat3(&m_xmf3Right, vRight);
	XMStoreFloat3(&m_xmf3Up, vUp);
	XMStoreFloat3(&m_xmf3Look, vLook);*/

	// 고정축 회전 적용
	{
		XMFLOAT3 xmf3Right = m_xmf3Right;
		XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&xmf3Right), XMConvertToRadians(x));
		m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
	}

	{
		XMFLOAT3 xmf3Up = XMFLOAT3(0, 1, 0);
		XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&xmf3Up), XMConvertToRadians(y));
		m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
	}

	{
		XMFLOAT3 xmf3Look = m_xmf3Look;
		XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&xmf3Look), XMConvertToRadians(z));
		m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
	}
}

BoundingFrustum CCamera::GetCameraWorldFrustum() const
{
	BoundingFrustum frustum;
	// Projection 매트릭스로부터 frustum 생성
	XMMATRIX xmmtxProjection = XMLoadFloat4x4(&m_xmf4x4Projection);
	BoundingFrustum::CreateFromMatrix(frustum, xmmtxProjection);

	// View 매트릭스로부터 Frustum을 World 좌표계로 변환 (View의 역행렬을 적용)
	XMMATRIX xmmtxView = XMLoadFloat4x4(&m_xmf4x4View);
	XMMATRIX xmmtxInvView = XMMatrixInverse(nullptr, xmmtxView);

	BoundingFrustum frustumWorld;
	frustum.Transform(frustumWorld, xmmtxInvView);

	return frustumWorld;
}

BoundingFrustum CCamera::GetCameraFrustum(float sliceNearZ, float sliceFarZ) const
{
	// 1) cascade slice 기준 Projection 생성
	XMMATRIX proj = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(m_fFovAngle),
		m_fAspectRatio,
		sliceNearZ,
		sliceFarZ
	);

	// 2) Projection → View-space BoundingFrustum
	BoundingFrustum frustumView;
	BoundingFrustum::CreateFromMatrix(frustumView, proj);

	// 3) View-space → World-space 변환
	XMMATRIX view = XMLoadFloat4x4(&m_xmf4x4View);
	XMMATRIX invView = XMMatrixInverse(nullptr, view);

	BoundingFrustum frustumWorld;
	frustumView.Transform(frustumWorld, invView);

	return frustumWorld;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//

CThirdPersonCamera::CThirdPersonCamera(CGameObject* pObject)
	: CCamera(pObject)
{

}

CThirdPersonCamera::~CThirdPersonCamera()
{
}

void CThirdPersonCamera::Rotate(float x, float y, float z)
{
	Clamp(m_fPitch, x, -89.0f, 89.0f); // -90 ~ 90으로 제한
	Normalize(m_fYaw, y, -180.0f, 180.0f); // -180 ~ 180으로 조정
	Normalize(m_fRoll, z, -180.0f, 180.0f); // -180 ~ 180으로 조정

	// 고정축 회전 적용
	{
		XMFLOAT3 xmf3Right = m_xmf3Right;
		XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&xmf3Right), XMConvertToRadians(x));
		m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
	}

	{
		XMFLOAT3 xmf3Up = XMFLOAT3(0, 1, 0);
		XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&xmf3Up), XMConvertToRadians(y));
		m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
	}

	{
		XMFLOAT3 xmf3Look = m_xmf3Look;
		XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&xmf3Look), XMConvertToRadians(z));
		m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
	}
}

void CThirdPersonCamera::Update(float fTimeElapsed)
{
	if (gameObject)
	{
		auto pChaseTransform = gameObject->GetComponent<CTransform>();

		// 카메라의 회전 행렬 계산
		XMFLOAT4X4 xmf4x4Rotate = Matrix4x4::Identity();
		XMStoreFloat4x4(&xmf4x4Rotate, XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_fPitch), XMConvertToRadians(m_fYaw), XMConvertToRadians(m_fRoll)));
		/*XMFLOAT3 xmf3Right = pChaseTransform->GetRight();
		XMFLOAT3 xmf3Up = pChaseTransform->GetUp();
		XMFLOAT3 xmf3Look = pChaseTransform->GetLook();
		xmf4x4Rotate._11 = xmf3Right.x; xmf4x4Rotate._21 = xmf3Up.x; xmf4x4Rotate._31 = xmf3Look.x;
		xmf4x4Rotate._12 = xmf3Right.y; xmf4x4Rotate._22 = xmf3Up.y; xmf4x4Rotate._32 = xmf3Look.y;
		xmf4x4Rotate._13 = xmf3Right.z; xmf4x4Rotate._23 = xmf3Up.z; xmf4x4Rotate._33 = xmf3Look.z;

		if (m_fPitch != 0.0f)
		{
			XMMATRIX xmmtxRotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_fPitch), XMConvertToRadians(0.0f), XMConvertToRadians(0.0f));
			xmf4x4Rotate = Matrix4x4::Multiply(xmmtxRotate, xmf4x4Rotate);
		}*/

		// 오브젝트 대비 상대적 위치 설정
		XMFLOAT3 xmf3Offset = Vector3::TransformCoord(m_xmf3Offset, xmf4x4Rotate); // 상대적 위치에 회전 행렬 적용
		XMFLOAT3 xmf3Position = Vector3::Add(Vector3::Add(pChaseTransform->GetPosition(), XMFLOAT3(0, 1, 0)), xmf3Offset); // 상대적 위치	+ 오브젝트 위치 = 카메라 목표 위치
		XMFLOAT3 xmf3Direction = Vector3::Subtract(xmf3Position, m_xmf3Position); // 목표 위치 - 현재 위치 = 가야할 방향

		float fLength = Vector3::Length(xmf3Direction); // 가야할 거리 계산
		xmf3Direction = Vector3::Normalize(xmf3Direction); // 가야하는 방향 추출
		float fTimeLagScale = (m_fTimeLag) ? fTimeElapsed * (1.0f / m_fTimeLag) : 1.0f; // 시간 지연 계산

		float fDistance = fLength * fTimeLagScale; // 이동 거리 계산
		if (fDistance > fLength)fDistance = fLength; // 이동 거리가 목표 위치보다 멀면 목표 위치로 이동
		if (fLength < 0.01f)fDistance = fLength; // 거리가 0.01f 이하면 이동하지 않음

		if (fDistance > 0) // 이동해야할 경우
		{
			m_xmf3Position = Vector3::Add(m_xmf3Position, xmf3Direction, fDistance);
			OnTerrainUpdateCallback(fTimeElapsed);

			//SetLookAt(xmf3LookAt);
			SetLookAt(Vector3::Add(m_xmf3Position, Vector3::TransformCoord(XMFLOAT3(0, 0, 1), xmf4x4Rotate)));

			RegenerateViewMatrix();
		}
	}
}

void CThirdPersonCamera::Update(const XMFLOAT3& xmf3LookAt, float fTimeElapsed)
{
	if (gameObject)
	{
		auto pChaseTransform = gameObject->GetComponent<CTransform>();

		// 카메라의 회전 행렬 계산
		XMFLOAT4X4 xmf4x4Rotate = Matrix4x4::Identity();
		XMStoreFloat4x4(&xmf4x4Rotate, XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_fPitch), XMConvertToRadians(m_fYaw), XMConvertToRadians(m_fRoll))) ;
		/*XMFLOAT3 xmf3Right = pChaseTransform->GetRight();
		XMFLOAT3 xmf3Up = pChaseTransform->GetUp();
		XMFLOAT3 xmf3Look = pChaseTransform->GetLook();
		xmf4x4Rotate._11 = xmf3Right.x; xmf4x4Rotate._21 = xmf3Up.x; xmf4x4Rotate._31 = xmf3Look.x;
		xmf4x4Rotate._12 = xmf3Right.y; xmf4x4Rotate._22 = xmf3Up.y; xmf4x4Rotate._32 = xmf3Look.y;
		xmf4x4Rotate._13 = xmf3Right.z; xmf4x4Rotate._23 = xmf3Up.z; xmf4x4Rotate._33 = xmf3Look.z;

		if (m_fPitch != 0.0f)
		{
			XMMATRIX xmmtxRotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_fPitch), XMConvertToRadians(0.0f), XMConvertToRadians(0.0f));
			xmf4x4Rotate = Matrix4x4::Multiply(xmmtxRotate, xmf4x4Rotate);
		}*/

		// 오브젝트 대비 상대적 위치 설정
		XMFLOAT3 xmf3Offset = Vector3::TransformCoord(m_xmf3Offset, xmf4x4Rotate); // 상대적 위치에 회전 행렬 적용
		XMFLOAT3 xmf3Position = Vector3::Add(Vector3::Add(pChaseTransform->GetPosition(), XMFLOAT3(0,1,0)), xmf3Offset); // 상대적 위치	+ 오브젝트 위치 = 카메라 목표 위치
		XMFLOAT3 xmf3Direction = Vector3::Subtract(xmf3Position, m_xmf3Position); // 목표 위치 - 현재 위치 = 가야할 방향
				
		float fLength = Vector3::Length(xmf3Direction); // 가야할 거리 계산
		xmf3Direction = Vector3::Normalize(xmf3Direction); // 가야하는 방향 추출
		float fTimeLagScale = (m_fTimeLag) ? fTimeElapsed * (1.0f / m_fTimeLag) : 1.0f; // 시간 지연 계산

		float fDistance = fLength * fTimeLagScale; // 이동 거리 계산
		if (fDistance > fLength)fDistance = fLength; // 이동 거리가 목표 위치보다 멀면 목표 위치로 이동
		if (fLength < 0.01f)fDistance = fLength; // 거리가 0.01f 이하면 이동하지 않음

		if (fDistance > 0) // 이동해야할 경우
		{
			m_xmf3Position = Vector3::Add(m_xmf3Position, xmf3Direction, fDistance);
			OnTerrainUpdateCallback(fTimeElapsed);

			//SetLookAt(xmf3LookAt);
			SetLookAt(Vector3::Add(m_xmf3Position, Vector3::TransformCoord(XMFLOAT3(0,0,1), xmf4x4Rotate)));

			RegenerateViewMatrix();
		}
	}
}

void CThirdPersonCamera::SetLookAt(const XMFLOAT3& vLookAt)
{
	XMFLOAT4X4 mtxLookAt = Matrix4x4::LookAtLH(m_xmf3Position, vLookAt, gameObject? gameObject->GetUpVector() : XMFLOAT3{0,1,0});
	m_xmf3Right = XMFLOAT3(mtxLookAt._11, mtxLookAt._21, mtxLookAt._31);
	m_xmf3Up = XMFLOAT3(mtxLookAt._12, mtxLookAt._22, mtxLookAt._32);
	m_xmf3Look = XMFLOAT3(mtxLookAt._13, mtxLookAt._23, mtxLookAt._33);
}

void CThirdPersonCamera::OnTerrainUpdateCallback(float fTimeElapsed)
{
	if(m_pTerrainUpdatedContext)
	{
		CHeightMapTerrain* pTerrain = (CHeightMapTerrain*)m_pTerrainUpdatedContext;
		XMFLOAT3 xmf3Scale = pTerrain->GetScale();
		XMFLOAT3 xmf3CameraPosition = GetPosition();
		float fHeight = pTerrain->GetHeight(xmf3CameraPosition.x, xmf3CameraPosition.z);
		fHeight += m_fNearZ * cos(XMConvertToRadians(m_fFovAngle / 2)); // 카메라 높이 보정
		if (xmf3CameraPosition.y <= fHeight)
		{
			float dy = fHeight - xmf3CameraPosition.y;
			xmf3CameraPosition.y = fHeight;
			SetPosition(xmf3CameraPosition);
		}
	}
}


