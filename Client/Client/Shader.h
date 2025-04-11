///////////////////////////////////////////////////////////////////////////////
// Date: 2025-02-02
// Shader.h : CShader 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"

class CCamera;

class CShader
{
public:
	CShader();
	~CShader();

	// For Debugging
	virtual std::wstring GetShaderName() { return L"CShader"; }

	// Create Pipeline State
	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature);
	void CreateGraphicPipelineState(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState = 0);
	void CreateComputePipelineState(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dComputeRootSignature, int nPipelineState = 0);

	D3D12_SHADER_BYTECODE CompileShaderFromFile(const WCHAR* pszFileName, LPCSTR pszShaderName, LPCSTR pszShaderProfile, ID3DBlob** ppd3dShaderBlob);
	D3D12_SHADER_BYTECODE ReadCompiledShaderFromFile(const WCHAR* pszFileName, ID3DBlob** ppd3dShaderBlob = NULL);

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreateDomainShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreateHullShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreateGeometryShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreateComputeShader(int nPipelineState = 0);

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout(int nPipelineState = 0);
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState(int nPipelineState = 0);
	virtual D3D12_BLEND_DESC CreateBlendState(int nPipelineState = 0);
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState(int nPipelineState = 0);

	virtual D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType(int nPipelineState = 0) { return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; }
	virtual UINT GetRenderTargetCount(int nPipelineState = 0) { return 1; }
	virtual DXGI_FORMAT GetRenderTargetFormat(int nPipelineState = 0, int nRenderTargetIndex = 0) { return DXGI_FORMAT_R8G8B8A8_UNORM; }
	virtual DXGI_FORMAT GetDepthStencilFormat(int nPipelineState = 0) { return DXGI_FORMAT_D24_UNORM_S8_UINT; }
	virtual DXGI_SAMPLE_DESC GetSampleDesc(int nPipelineState = 0);
	virtual D3D12_PIPELINE_STATE_FLAGS GetPipelineStateFlags(int nPipelineState = 0) { return D3D12_PIPELINE_STATE_FLAG_NONE; }

	// Shader Functions
	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) { }
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList) { }
	virtual void ReleaseShaderVariables() { }

	virtual void UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, XMFLOAT4X4* pxmf4x4World) { }

	virtual void OnPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList, int nPipelineState = 0);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);

	virtual void ReleaseUploadBuffers() { }

protected:
	// Shader Variables
	std::vector<ComPtr<ID3D12PipelineState>> m_pd3dPipelineStates; // [m_nPipelineState]

	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_d3dPipelineStateDesc;

	ComPtr<ID3DBlob> m_pd3dVertexShaderBlob;
	ComPtr<ID3DBlob> m_pd3dPixelShaderBlob;
	ComPtr<ID3DBlob> m_pd3dDomainShaderBlob;
	ComPtr<ID3DBlob> m_pd3dHullShaderBlob;
	ComPtr<ID3DBlob> m_pd3dGeometryShaderBlob;
};

////////////////////////////////////////////////////////////////////////////////////////////
//

class CStandardShader : public CShader
{
public:
	CStandardShader();
	virtual ~CStandardShader();

	virtual std::wstring GetShaderName() override { return L"CStandardShader"; }

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState) override;
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState) override;

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout(int nPipelineState) override;
};

////////////////////////////////////////////////////////////////////////////////////////////
//

class CSkinnedAnimationStandardShader : public CStandardShader
{
public:
	CSkinnedAnimationStandardShader();
	virtual ~CSkinnedAnimationStandardShader();

	virtual std::wstring GetShaderName() override { return L"CSkinnedAnimationStandardShader"; }

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout(int nPipelineState) override;
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState) override;


};

////////////////////////////////////////////////////////////////////////////////////////////
//

class CSkyBoxShader : public CShader
{
public:
	CSkyBoxShader();
	virtual ~CSkyBoxShader();

	virtual std::wstring GetShaderName() override { return L"CSkyBoxShader"; }

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState) override;
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState) override;

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout(int nPipelineState) override;
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState(int nPipelineState) override;
};

////////////////////////////////////////////////////////////////////////////////////////////
//

class CTerrainShader : public CShader
{
public:
	CTerrainShader();
	virtual ~CTerrainShader();

	virtual std::wstring GetShaderName() override { return L"CTerrainShader"; }


	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature) override;

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout(int nPipelineState) override;
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState) override;
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////
//

class CColliderShader : public CShader
{
public:
	CColliderShader();
	virtual ~CColliderShader();

	virtual std::wstring GetShaderName() override { return L"CColliderShader"; }

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout(int nPipelineState) override;
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState) override;
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState) override;
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState(int nPipelineState) override;
};
