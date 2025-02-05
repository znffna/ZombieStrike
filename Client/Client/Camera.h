///////////////////////////////////////////////////////////////////////////////
// Date: 2025-02-05
// Camera.h : CCamera 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"

struct CB_CAMERA_INFO
{
	XMFLOAT4X4				m_xmf4x4View;
	XMFLOAT4X4				m_xmf4x4InverseView;
	XMFLOAT4X4				m_xmf4x4Projection;
	XMFLOAT3				m_xmf3Position;
	float					m_fPadding;
};

class CCamera
{
public:
	CCamera();
	virtual ~CCamera();

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
	void GenerateViewMatrix(XMFLOAT3 xmf3Position, XMFLOAT3 xmf3LookAt, XMFLOAT3 xmf3Up);
	void RegenerateViewMatrix();

	void GenerateProjectionMatrix(float aspectRatio, float fov, float nearZ, float farZ);

private:
	// ViewPort and ScissorRect
	D3D12_VIEWPORT m_d3dViewport;
	D3D12_RECT m_d3dScissorRect;

	// Camera Outter Variables
	float m_fAspectRatio = 16.0f / 9.0f;
	float m_fFovAngle = 90.0f;
	float m_fNearZ = 1.0f;
	float m_fFarZ = 500.0f;

	// Camera Inner Variables
	XMFLOAT3 m_xmf3Position;
	XMFLOAT3 m_xmf3Right;
	XMFLOAT3 m_xmf3Up;
	XMFLOAT3 m_xmf3Look; 

	//XMFLOAT3 m_xmf3LookAtWorld; // World
	//XMFLOAT3 m_xmf3Offset;
	//float m_fTimeLag;

	// Camera Shader Matrix
	XMFLOAT4X4 m_xmf4x4View;
	XMFLOAT4X4 m_xmf4x4Projection;

	// Camera Shader Variables
	ComPtr<ID3D12Resource> m_pd3dcbCamera;
	CB_CAMERA_INFO* m_pcbMappedCamera = NULL;
};

