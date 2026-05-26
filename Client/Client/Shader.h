///////////////////////////////////////////////////////////////////////////////
// Date: 2025-02-02
// Shader.h : CShader 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"

class CCamera;
class CScene;
class CTexture;
class CDescriptorHeap;

class CShader
{
public:
	CShader(bool bAllowShadow = true);
	~CShader();

	// For Debugging
	virtual std::wstring GetShaderName() { return L"CShader"; }

	// Create Pipeline State
	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
	void CreateGraphicsPipelineState(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState = 0, bool bDepthWrite = false);
	void CreateComputePipelineState(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dComputeRootSignature, int nPipelineState = 0);

	D3D12_SHADER_BYTECODE CompileShaderFromFile(const WCHAR* pszFileName, LPCSTR pszShaderName, LPCSTR pszShaderProfile, ID3DBlob** ppd3dShaderBlob);
	D3D12_SHADER_BYTECODE ReadCompiledShaderFromFile(const WCHAR* pszFileName, ID3DBlob** ppd3dShaderBlob = NULL);
	void SaveShaderToCSOFile(ID3DBlob* pShaderBlob, LPCSTR pszShaderName);

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreatePixelShaderBranch(int nPipelineState = 0, bool bDepthWrite = false);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreateDepthWritePixelShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreateDomainShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreateHullShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreateGeometryShader(int nPipelineState = 0);
	virtual D3D12_STREAM_OUTPUT_DESC CreateStreamOuputState(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreateComputeShader(int nPipelineState = 0);

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout(int nPipelineState = 0);
	virtual D3D12_RASTERIZER_DESC CreateRasterizerStateBranch(int nPipelineState = 0, bool bDepthWrite = false);
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState(int nPipelineState = 0);
	virtual D3D12_RASTERIZER_DESC CreateDepthWriteRasterizerState(int nPipelineState = 0);
	virtual D3D12_BLEND_DESC CreateBlendState(int nPipelineState = 0);
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilStateBranch(int nPipelineState = 0, bool bDepthWrite = false);
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState(int nPipelineState = 0);
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthWriteDepthStencilState(int nPipelineState = 0);

	virtual D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType(int nPipelineState = 0) { return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; }
	virtual UINT GetRenderTargetCount(int nPipelineState = 0) { return 1; }
	virtual DXGI_FORMAT GetRenderTargetFormat(int nPipelineState = 0, int nRenderTargetIndex = 0, bool bDepthWrite = false) { return (bDepthWrite)? DXGI_FORMAT_R32_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM; }
	virtual DXGI_FORMAT GetDepthStencilFormat(int nPipelineState = 0, bool bDepthWrite = false) { return (bDepthWrite) ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_D24_UNORM_S8_UINT; }
	virtual DXGI_SAMPLE_DESC GetSampleDesc(int nPipelineState = 0);
	virtual D3D12_PIPELINE_STATE_FLAGS GetPipelineStateFlags(int nPipelineState = 0) { return D3D12_PIPELINE_STATE_FLAG_NONE; }

	// Shader Functions
	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) {}
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList) {}
	virtual void ReleaseShaderVariables() {}

	virtual void UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, XMFLOAT4X4* pxmf4x4World) {}

	virtual void OnPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList, int nPipelineState = 0, bool bDepthWrite = false);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);

	virtual void ReleaseUploadBuffers() {}

protected:
	// Initialization
	bool b_Initialized = false;

	// Shader Variables
	int m_nPipelineStates = 1;
	std::vector<ComPtr<ID3D12PipelineState>> m_pd3dPipelineStates; // [m_nPipelineStates]

	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_d3dPipelineStateDesc;

	ComPtr<ID3DBlob> m_pd3dVertexShaderBlob;
	ComPtr<ID3DBlob> m_pd3dPixelShaderBlob;
	ComPtr<ID3DBlob> m_pd3dDomainShaderBlob;
	ComPtr<ID3DBlob> m_pd3dHullShaderBlob;
	ComPtr<ID3DBlob> m_pd3dGeometryShaderBlob;

protected:
	std::unique_ptr<CDescriptorHeap> m_pDescriptorHeap;

	void CreateCbvSrvDescriptorHeaps(ID3D12Device* pd3dDevice, int nConstantBufferViews, int nShaderResourceViews);
	void CreateShaderResourceViews(ID3D12Device* pd3dDevice, CTexture* pTexture, UINT nDescriptorHeapIndex, UINT nRootParameterStartIndex);

protected:
	// 그림자 관련 변수
	/* 해당 flag를 생성자에서 Set하며, 해당 flag를 통해 pipelineState Vector의 마지막에 추가한다.*/
	bool m_bAllowShadow = true; // 그림자 허용 여부
public:
	bool GetAllowShadow() const { return m_bAllowShadow; }
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


//////////////////////////////////////////////////////////////////////////////////////////////
//
class CTexturedShader : public CShader
{
public:
	CTexturedShader();
	virtual ~CTexturedShader();

	virtual std::wstring GetShaderName() override { return L"CTexturedShader"; }
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout(int nPipelineState) override;

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState) override;
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState) override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CBillboardShader : public CTexturedShader
{
public:
	CBillboardShader();
	virtual ~CBillboardShader();

	virtual std::wstring GetShaderName() override { return L"CBillboardShader"; }

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState) override;
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState) override;

	virtual D3D12_RASTERIZER_DESC CreateRasterizerState(int nPipelineState) override;
	virtual D3D12_BLEND_DESC CreateBlendState(int nPipelineState) override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CDescriptorHeap;

class CBulletShader : public CBillboardShader
{
public:
	CBulletShader();
	virtual ~CBulletShader();

	virtual std::wstring GetShaderName() override { return L"CBulletShader"; }

	virtual D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType(int nPipelineState) override;
	virtual UINT GetNumRenderTargets(int nPipelineState);
	virtual DXGI_FORMAT GetRTVFormat(int nPipelineState, int nRenderTarget);
	virtual DXGI_FORMAT GetDSVFormat(int nPipelineState);

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState) override;
	virtual D3D12_SHADER_BYTECODE CreateGeometryShader(int nPipelineState) override;
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState) override;
	virtual D3D12_STREAM_OUTPUT_DESC CreateStreamOuputState(int nPipelineState) override;

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout(int nPipelineState) override;
	virtual D3D12_BLEND_DESC CreateBlendState(int nPipelineState) override;
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState(int nPipelineState) override;

	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature) override;

private:
	std::shared_ptr<CDescriptorHeap> m_pd3dCbvSrvDescriptorHeap = NULL;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CIlluminatedShader : public CShader
{
public:
	CIlluminatedShader();
	virtual ~CIlluminatedShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout(int nPipelineState) override;

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState)override;
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState)override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

struct TOLIGHTSPACEINFO
{
	XMFLOAT4X4						m_pxmf4x4ToTextures; //Transposed
	XMFLOAT4						m_pxmf4LightPositions;
};

struct CB_TO_LIGHTSPACES
{
	TOLIGHTSPACEINFO m_ToLightSpaces[MAX_LIGHTS];
};

class CDepthRenderShader : public CSkinnedAnimationStandardShader
{
public:
	CDepthRenderShader();
	virtual ~CDepthRenderShader();

	virtual std::wstring GetShaderName() override { return L"CDepthRenderShader"; }

	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature) override;

	virtual DXGI_FORMAT GetRenderTargetFormat(int nPipelineState, int nRenderTargetIndex, bool bDepthWrite = false) override { return DXGI_FORMAT_R32_FLOAT; }
	virtual DXGI_FORMAT GetDepthStencilFormat(int nPipelineState, bool bDepthWrite) override { return DXGI_FORMAT_D32_FLOAT; }
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState(int nPipelineState) override;
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState(int nPipelineState) override;

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState) override;
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState) override;

	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pContext = NULL);
	virtual void ReleaseObjects();

	void RenderShadowSlice(ID3D12GraphicsCommandList* pd3dCommandList, int shadowIndex, const XMMATRIX& lightView, const XMMATRIX& lightProj, const XMFLOAT3& lightPos, CScene* pScene);

	void PrepareShadowMap(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, CScene* pScene);

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, CScene* pScene);

protected:
	std::shared_ptr<CTexture> m_pDepthFromLightTexture;

	std::array<std::shared_ptr<CCamera>, MAX_DEPTH_TEXTURES> m_ppDepthRenderCameras;

	ComPtr<ID3D12DescriptorHeap> m_pd3dRtvDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE		m_pd3dRtvCPUDescriptorHandles[MAX_DEPTH_TEXTURES];

	ComPtr<ID3D12DescriptorHeap> m_pd3dDsvDescriptorHeap;
	ComPtr<ID3D12Resource> m_pd3dDepthBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE		m_d3dDsvDescriptorCPUHandle;

	XMMATRIX						m_xmProjectionToTexture;

public:
	std::shared_ptr<CTexture> GetDepthTexture();
	ID3D12Resource* GetDepthTextureResource(UINT nIndex);

protected:
	std::vector<TOLIGHTSPACEINFO> m_pToLightSpaces;

	ComPtr<ID3D12Resource> m_pd3dcbToLightSpaces;
	TOLIGHTSPACEINFO* m_pcbMappedToLightSpaces = NULL;

protected:
	int m_nDepthbufferWidth;
	int m_nDepthbufferHeight;
};

class CShadowMapShader : public CSkinnedAnimationStandardShader
{
public:
	CShadowMapShader();
	virtual ~CShadowMapShader();

	virtual std::wstring GetShaderName() override { return L"CShadowMapShader"; }

	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState(int nPipelineState) override;

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState) override;
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState) override;

	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CTexture* pContext = NULL);
	virtual void AnimateObjects(float fTimeElapsed) {}
	virtual void ReleaseObjects();

	virtual void ReleaseUploadBuffers();

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, CScene* pScene);

public:
	CTexture* m_pDepthFromLightTexture = nullptr;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CTextureToViewportShader : public CShader
{
public:
	CTextureToViewportShader();
	virtual ~CTextureToViewportShader();

	virtual std::wstring GetShaderName() override { return L"CTextureToViewportShader"; }

	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState(int nPipelineState) override;

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState) override;
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState) override;

	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CTexture* pContext = NULL);
	virtual void ReleaseObjects();

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);

protected:
	CTexture* m_pDepthFromLightTexture = nullptr;
};

class CShadowToViewportShader : public CTextureToViewportShader
{
public:
	CShadowToViewportShader() : CTextureToViewportShader() {};
	virtual ~CShadowToViewportShader() {};

	virtual std::wstring GetShaderName() override { return L"CShadowToViewportShader"; }

	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState) override
	{
#ifdef _COMPILE_SHADER
		return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PSShadowToViewport", "ps_5_1", m_pd3dPixelShaderBlob.GetAddressOf()));
#else
		return(CShader::ReadCompiledShaderFromFile(L"PSShadowToViewport", m_pd3dPixelShaderBlob.GetAddressOf()));
#endif
	};
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CMinimapShader : public CShader
{
public:
	CMinimapShader();
	virtual ~CMinimapShader();
	virtual std::wstring GetShaderName() override { return L"CMinimapShader"; }

	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature) override;

	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) {};
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList) {};
	virtual void ReleaseShaderVariables() {};

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseObjects();

	virtual void PrepareMinimap(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, CScene* pScene);

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, CScene* pScene);

protected:
	std::shared_ptr<CTexture> m_pMinimapTexture;

	std::vector<std::shared_ptr<CCamera>> m_ppMinimapCameras;

	ComPtr<ID3D12DescriptorHeap> m_pd3dRtvDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE		m_pd3dRtvCPUDescriptorHandles[MAX_DEPTH_TEXTURES];

	ComPtr<ID3D12DescriptorHeap> m_pd3dDsvDescriptorHeap;
	ComPtr<ID3D12Resource> m_pd3dDepthBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE		m_d3dDsvDescriptorCPUHandle;

	XMMATRIX						m_xmProjectionToTexture;

public:
	std::shared_ptr<CTexture> GetMinimapTexture();
	ID3D12Resource* GetMinimapTextureResource(UINT nIndex);

private:
	int m_nMinimapCount = 1;
	int m_nMinimapBufferWidth = 512;
	int m_nMinimapBufferHeight = 512;
};

class CMinimapToViewportShader : public CTextureToViewportShader
{
public:
	CMinimapToViewportShader() : CTextureToViewportShader() {};
	virtual ~CMinimapToViewportShader() {};

	virtual std::wstring GetShaderName() override { return L"CMinimapToViewportShader"; }

	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState) override
	{
#ifdef _COMPILE_SHADER
		return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PSMinimapToViewport", "ps_5_1", m_pd3dPixelShaderBlob.GetAddressOf()));
#else
		return(CShader::ReadCompiledShaderFromFile(L"PSMinimapToViewport", m_pd3dPixelShaderBlob.GetAddressOf()));
#endif
	};

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
	{
		/*D3D12_VIEWPORT d3dViewport = { 0.0f, 0.0f, WINDOW_WIDTH * 0.25f, WINDOW_HEIGHT * 0.25f, 0.0f, 1.0f };
		D3D12_RECT d3dScissorRect = { 0, 0, WINDOW_WIDTH / 4, WINDOW_HEIGHT / 4 };*/
		// 우측 상단에 미니맵 렌더링 (화면 크기의 1/4)
		float minimapWidth = WINDOW_WIDTH * 0.25f;
		float minimapHeight = WINDOW_HEIGHT * 0.25f;
		float minimapX = WINDOW_WIDTH - minimapWidth;  // 우측 끝에서 미니맵 너비만큼 왼쪽
		float minimapY = 0.0f;  // 화면 상단

		D3D12_VIEWPORT d3dViewport = {
			minimapX,           // TopLeftX
			minimapY,           // TopLeftY  
			minimapWidth,       // Width
			minimapHeight,      // Height
			0.0f,               // MinDepth
			1.0f                // MaxDepth
		};

		D3D12_RECT d3dScissorRect = {
			(LONG)minimapX,                          // left
			(LONG)minimapY,                          // top
			(LONG)(minimapX + minimapWidth),         // right
			(LONG)(minimapY + minimapHeight)         // bottom
		};

		pd3dCommandList->RSSetViewports(1, &d3dViewport);
		pd3dCommandList->RSSetScissorRects(1, &d3dScissorRect);

		XMFLOAT4X4 xmf4x4Identity = Matrix4x4::Identity();
		pd3dCommandList->SetGraphicsRoot32BitConstants(ROOT_PARAMETER_OBJECT, 16, &xmf4x4Identity, 0);

		CShader::OnPrepareRender(pd3dCommandList);
		UpdateShaderVariables(pd3dCommandList);

		pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pd3dCommandList->DrawInstanced(6, 1, 0, 0);
	}
};