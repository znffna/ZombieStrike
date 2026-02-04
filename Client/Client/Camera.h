///////////////////////////////////////////////////////////////////////////////
// Date: 2025-02-05
// Camera.h : CCamera 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"
#include "Component.h"

class CTransform;
class CGameObject;

struct CB_CAMERA_INFO
{
	XMFLOAT4X4				m_xmf4x4View;
	XMFLOAT4X4				m_xmf4x4InverseView;
	XMFLOAT4X4				m_xmf4x4Projection;
	XMFLOAT4X4				m_xmf4x4InvProjection;
	XMFLOAT3				m_xmf3Position;
	float					m_fPadding;
};

class CCamera : public CComponent
{
public:
	CCamera(CGameObject* pObject) : CComponent(pObject) {};
	CCamera(const CCamera& rhs) : CComponent(nullptr)
	{
		m_d3dViewport = rhs.m_d3dViewport;
		m_d3dScissorRect = rhs.m_d3dScissorRect;
		m_fAspectRatio = rhs.m_fAspectRatio;
		m_fFovAngle = rhs.m_fFovAngle;
		m_fNearZ = rhs.m_fNearZ;
		m_fFarZ = rhs.m_fFarZ;
		m_xmf3Position = rhs.m_xmf3Position;
		m_xmf3Right = rhs.m_xmf3Right;
		m_xmf3Up = rhs.m_xmf3Up;
		m_xmf3Look = rhs.m_xmf3Look;
		m_fPitch = rhs.m_fPitch;
		m_fYaw = rhs.m_fYaw;
		m_fRoll = rhs.m_fRoll;
		m_xmf4Rotation = rhs.m_xmf4Rotation;
		m_xmf3Offset = rhs.m_xmf3Offset;
		m_fTimeLag = rhs.m_fTimeLag;
		m_xmf4x4View = rhs.m_xmf4x4View;
		m_xmf4x4Projection = rhs.m_xmf4x4Projection;
	};
		
	virtual ~CCamera() {};

	virtual void Initialize();

	virtual std::unique_ptr<CComponent> Clone(CGameObject* newOwner) const { auto ret = std::make_unique<CCamera>(*this); ret->SetOwnerInternal(newOwner); return (ret); };

	// Camera Shader Variables
	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();

	// ViewPort and ScissorRect
	void SetViewport(int xTopLeft, int yTopLeft, int nWidth, int nHeight, float fMinZ = 0.0f, float fMaxZ = 1.0f);
	void SetScissorRect(LONG xLeft, LONG yTop, LONG xRight, LONG yBottom);

	virtual void SetViewportsAndScissorRects(ID3D12GraphicsCommandList* pd3dCommandList);

	// Matrix Functions
	void GenerateViewMatrix();
	void GenerateViewMatrix(const XMFLOAT3& xmf3Position, const XMFLOAT3& xmf3LookAt, const XMFLOAT3& xmf3Up);
	void RegenerateViewMatrix();

	void GenerateProjectionMatrix(float aspectRatio, float fov, float nearZ, float farZ);

	// Camera Position
	void SetPosition(float x, float y, float z) { m_xmf3Position = XMFLOAT3(x, y, z); };
	void SetPosition(const XMFLOAT3& xmf3Position) { m_xmf3Position = xmf3Position; };

	void SetOffset(const XMFLOAT3& xmf3Offset) { m_xmf3Offset = xmf3Offset; };

	XMFLOAT3 GetPosition() { return m_xmf3Position; };
	void Move(XMFLOAT3 xmf3Shift) { m_xmf3Position = Vector3::Add(m_xmf3Position, xmf3Shift); };
	void Move(float x, float y, float z) { Move(XMFLOAT3(x, y, z)); };

	// Camera Roatation
	XMFLOAT3 GetLook() { return m_xmf3Look; }
	XMFLOAT3 GetUp() { return m_xmf3Up; }
	XMFLOAT3 GetRight() { return m_xmf3Right; }

	float GetPitch() { return m_fPitch; }
	float GetYaw() { return m_fYaw; }
	float GetRoll() { return m_fRoll; }

	void SetPitch(float fPitch) { m_fPitch = fPitch; }

	virtual void Rotate(float x, float y, float z);
	void Rotate(const XMFLOAT3& xmf3Shift) { Rotate(xmf3Shift.x, xmf3Shift.y, xmf3Shift.z); }

	virtual void Update(const XMFLOAT3& xmf3LookAt, float fTimeElapsed) { }

protected:	
	// ViewPort and ScissorRect
	D3D12_VIEWPORT m_d3dViewport;
	D3D12_RECT m_d3dScissorRect;

	// Camera Outter Variables
	float m_fAspectRatio = 16.0f / 9.0f;
	float m_fFovAngle = 90.0f;
	float m_fNearZ = 1.0f;
	float m_fFarZ = 500.0f;

	// Camera Inner Variables
	XMFLOAT3 m_xmf3Position = XMFLOAT3(0.0f, 0.0f, -10.0f); // 현 카메라 위치[World]
	XMFLOAT3 m_xmf3Right = XMFLOAT3(1.0f, 0.0f, 0.0f);
	XMFLOAT3 m_xmf3Up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	XMFLOAT3 m_xmf3Look = XMFLOAT3(0.0f, 0.0f, 1.0f);

	float m_fPitch = 0.0f;
	float m_fYaw = 0.0f;
	float m_fRoll = 0.0f;
	XMFLOAT4 m_xmf4Rotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	//XMFLOAT3 m_xmf3LookAtWorld; // World
	XMFLOAT3 m_xmf3Offset = XMFLOAT3{0.0f, 0.0f, 0.0f}; // 오브젝트(Owner)와의 상대적 포지션[Local]
	float m_fTimeLag = 0.0f;
public:
	// Camera Shader Matrix
	XMFLOAT4X4 m_xmf4x4View;
	XMFLOAT4X4 m_xmf4x4Projection;

	// Camera Shader Variables
	ComPtr<ID3D12Resource> m_pd3dcbCamera;
	CB_CAMERA_INFO* m_pcbMappedCamera = NULL;

public:
	virtual void OnTerrainUpdateCallback(float fTimeElapsed) {};
	void SetTerrainUpdatedContext(LPVOID pContext) { m_pTerrainUpdatedContext = pContext; }

	LPVOID						m_pTerrainUpdatedContext = NULL;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CThirdPersonCamera : public CCamera
{
public:
	CThirdPersonCamera(CGameObject* pObject = nullptr);
	virtual ~CThirdPersonCamera();

	virtual void Rotate(float x, float y, float z) override;

	virtual void Update(float fTimeElapsed) override;
	virtual void Update(const XMFLOAT3 & xmf3LookAt, float fTimeElapsed) override;
	virtual void SetLookAt(const XMFLOAT3 & vLookAt);

	virtual void OnTerrainUpdateCallback(float fTimeElapsed) override;

};


