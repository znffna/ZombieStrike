///////////////////////////////////////////////////////////////////////////////
// Date: 2025-02-02
// Shader.cpp : CShader 클래스의 소스 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#include "Shader.h"
#include "Mesh.h"
#include "Texture.h"
#include "Scene.h"

CShader::CShader()
{
}

CShader::~CShader()
{
}

void CShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	// Resize Pipeline State Vector
	m_pd3dPipelineStates.resize(1);

	// Create Pipeline State
	CreateGraphicsPipelineState(pd3dDevice, pd3dGraphicsRootSignature, 0);
}

void CShader::CreateGraphicsPipelineState(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState)
{
	::ZeroMemory(&m_d3dPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	m_d3dPipelineStateDesc.pRootSignature = pd3dGraphicsRootSignature;
	m_d3dPipelineStateDesc.VS = CreateVertexShader(nPipelineState);
	m_d3dPipelineStateDesc.PS = CreatePixelShader(nPipelineState);
	m_d3dPipelineStateDesc.DS = CreateDomainShader(nPipelineState);
	m_d3dPipelineStateDesc.HS = CreateHullShader(nPipelineState);
	m_d3dPipelineStateDesc.GS = CreateGeometryShader(nPipelineState);
	m_d3dPipelineStateDesc.StreamOutput = CreateStreamOuputState(nPipelineState);
	m_d3dPipelineStateDesc.RasterizerState = CreateRasterizerState(nPipelineState);
	m_d3dPipelineStateDesc.BlendState = CreateBlendState(nPipelineState);
	m_d3dPipelineStateDesc.DepthStencilState = CreateDepthStencilState(nPipelineState);
	m_d3dPipelineStateDesc.InputLayout = CreateInputLayout(nPipelineState);
	m_d3dPipelineStateDesc.SampleMask = UINT_MAX;
	m_d3dPipelineStateDesc.PrimitiveTopologyType = GetPrimitiveTopologyType(nPipelineState);
	m_d3dPipelineStateDesc.NumRenderTargets = GetRenderTargetCount(nPipelineState);
	for (UINT i = 0; i < m_d3dPipelineStateDesc.NumRenderTargets; ++i)
	{
		m_d3dPipelineStateDesc.RTVFormats[i] = GetRenderTargetFormat(nPipelineState, i);
	}
	m_d3dPipelineStateDesc.DSVFormat = GetDepthStencilFormat(nPipelineState);
	m_d3dPipelineStateDesc.SampleDesc = GetSampleDesc(nPipelineState);
	m_d3dPipelineStateDesc.Flags = GetPipelineStateFlags(nPipelineState);

	HRESULT hResult = pd3dDevice->CreateGraphicsPipelineState(&m_d3dPipelineStateDesc, IID_PPV_ARGS(m_pd3dPipelineStates[nPipelineState].GetAddressOf()));

	// Output Debug Message
	if (hResult == S_OK)
	{
		// Output Debug Message
		std::wstring strDebugString = GetShaderName()+ L" - " + std::to_wstring(nPipelineState) + L" Graphic Pipeline State is created successfully.\n";
		OutputDebugString(strDebugString.data());
		m_pd3dPipelineStates[nPipelineState]->SetName(GetShaderName().data());
	}
	else
	{
		// Output Debug Message
		std::wstring strDebugString = GetShaderName() + L" - " + std::to_wstring(nPipelineState) + L" Graphic Pipeline State is not created successfully.\n";
		OutputDebugString(strDebugString.data());
	}

	if (m_pd3dVertexShaderBlob) m_pd3dVertexShaderBlob.Reset();
	if (m_pd3dPixelShaderBlob) m_pd3dPixelShaderBlob.Reset();
	if (m_pd3dDomainShaderBlob) m_pd3dDomainShaderBlob.Reset();
	if (m_pd3dHullShaderBlob) m_pd3dHullShaderBlob.Reset();
	if (m_pd3dGeometryShaderBlob) m_pd3dGeometryShaderBlob.Reset();

	if (m_d3dPipelineStateDesc.InputLayout.pInputElementDescs) delete[] m_d3dPipelineStateDesc.InputLayout.pInputElementDescs;
}

void CShader::CreateComputePipelineState(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dComputeRootSignature, int nPipelineState)
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC d3dComputePipelineStateDesc;
	::ZeroMemory(&d3dComputePipelineStateDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));

	d3dComputePipelineStateDesc.pRootSignature = pd3dComputeRootSignature;
	d3dComputePipelineStateDesc.CS = CreateComputeShader(nPipelineState);
	d3dComputePipelineStateDesc.NodeMask = 0;
	d3dComputePipelineStateDesc.Flags = GetPipelineStateFlags(nPipelineState);

	HRESULT hResult = pd3dDevice->CreateComputePipelineState(&d3dComputePipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)(m_pd3dPipelineStates[nPipelineState]).GetAddressOf());
}

D3D12_SHADER_BYTECODE CShader::CompileShaderFromFile(const WCHAR* pszFileName, LPCSTR pszShaderName, LPCSTR pszShaderProfile, ID3DBlob** ppd3dShaderBlob)
{
	UINT nCompileFlags = 0;
#if defined(_DEBUG)
	nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pd3dErrorBlob = NULL;
	HRESULT hResult = ::D3DCompileFromFile(pszFileName, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, pszShaderName, pszShaderProfile, nCompileFlags, 0, ppd3dShaderBlob, &pd3dErrorBlob);
	char* pErrorString = NULL;
	if (pd3dErrorBlob) pErrorString = (char*)pd3dErrorBlob->GetBufferPointer();

	D3D12_SHADER_BYTECODE d3dShaderByteCode;
	d3dShaderByteCode.BytecodeLength = (*ppd3dShaderBlob)->GetBufferSize();
	d3dShaderByteCode.pShaderBytecode = (*ppd3dShaderBlob)->GetBufferPointer();

	SaveShaderToCSOFile(*ppd3dShaderBlob, pszShaderName);
	{
		std::string strDebugString = pszShaderName;
		strDebugString = "Shader [" + strDebugString +"] is compiled successfully.\n";
		OutputDebugStringA(strDebugString.data());
	}

	return(d3dShaderByteCode);
}

D3D12_SHADER_BYTECODE CShader::ReadCompiledShaderFromFile(const WCHAR* pszFileName, ID3DBlob** ppd3dShaderBlob)
{
	UINT nReadBytes = 0;

	std::wstring wstrFileName(pszFileName);
	if (wstrFileName.length() < 4 || wstrFileName.substr(wstrFileName.length() - 4) != L".cso")
	{
		wstrFileName = L"Shader/" + wstrFileName + L".cso";
	}

#ifdef _WITH_WFOPEN
	FILE* pFile = NULL;
	::_wfopen_s(&pFile, wstrFileName, L"rb");
	::fseek(pFile, 0, SEEK_END);
	int nFileSize = ::ftell(pFile);
	BYTE* pByteCode = new BYTE[nFileSize];
	::rewind(pFile);
	nReadBytes = (UINT)::fread(pByteCode, sizeof(BYTE), nFileSize, pFile);
	::fclose(pFile);
#else
	std::ifstream ifsFile;
	ifsFile.open(wstrFileName, std::ios::in | std::ios::ate | std::ios::binary);
	nReadBytes = (int)ifsFile.tellg();
	BYTE* pByteCode = new BYTE[nReadBytes];
	ifsFile.seekg(0);
	ifsFile.read((char*)pByteCode, nReadBytes);
	ifsFile.close();
#endif

	D3D12_SHADER_BYTECODE d3dShaderByteCode;
	if (ppd3dShaderBlob)
	{
		*ppd3dShaderBlob = NULL;
		HRESULT hResult = D3DCreateBlob(nReadBytes, ppd3dShaderBlob);
		memcpy((*ppd3dShaderBlob)->GetBufferPointer(), pByteCode, nReadBytes);
		d3dShaderByteCode.BytecodeLength = (*ppd3dShaderBlob)->GetBufferSize();
		d3dShaderByteCode.pShaderBytecode = (*ppd3dShaderBlob)->GetBufferPointer();
	}
	else
	{
		d3dShaderByteCode.BytecodeLength = nReadBytes;
		d3dShaderByteCode.pShaderBytecode = pByteCode;
	}

	return(d3dShaderByteCode);
}

void CShader::SaveShaderToCSOFile(ID3DBlob* pShaderBlob, const LPCSTR pszShaderName)
{
	if (!pShaderBlob || !pszShaderName)
		return;

	std::string shaderFileName(pszShaderName);

	if (shaderFileName.length() < 4 || shaderFileName.substr(shaderFileName.length() - 4) != ".cso")
	{
		shaderFileName = "Shader/" + shaderFileName + ".cso";
	}

#ifdef _WITH_WFOPEN
	FILE* pFile = nullptr;
	if (_wfopen_s(&pFile, shaderFileName, L"wb") == 0 && pFile)
	{
		fwrite(pShaderBlob->GetBufferPointer(), 1, pShaderBlob->GetBufferSize(), pFile);
		fclose(pFile);
	}
#else
	std::ofstream ofsFile(shaderFileName, std::ios::out | std::ios::binary);
	if (ofsFile)
	{
		ofsFile.write(reinterpret_cast<const char*>(pShaderBlob->GetBufferPointer()), pShaderBlob->GetBufferSize());
		ofsFile.close();
	}
#endif
}

D3D12_SHADER_BYTECODE CShader::CreateVertexShader(int nPipelineState)
{
	D3D12_SHADER_BYTECODE d3dShaderByteCode;
	d3dShaderByteCode.BytecodeLength = 0;
	d3dShaderByteCode.pShaderBytecode = NULL;

	return (d3dShaderByteCode);
}

D3D12_SHADER_BYTECODE CShader::CreatePixelShader(int nPipelineState)
{
	D3D12_SHADER_BYTECODE d3dShaderByteCode;
	d3dShaderByteCode.BytecodeLength = 0;
	d3dShaderByteCode.pShaderBytecode = NULL;

	return (d3dShaderByteCode);
}

D3D12_SHADER_BYTECODE CShader::CreateDomainShader(int nPipelineState)
{
	D3D12_SHADER_BYTECODE d3dShaderByteCode;
	d3dShaderByteCode.BytecodeLength = 0;
	d3dShaderByteCode.pShaderBytecode = NULL;

	return (d3dShaderByteCode);
}

D3D12_SHADER_BYTECODE CShader::CreateHullShader(int nPipelineState)
{
	D3D12_SHADER_BYTECODE d3dShaderByteCode;
	d3dShaderByteCode.BytecodeLength = 0;
	d3dShaderByteCode.pShaderBytecode = NULL;

	return (d3dShaderByteCode);
}

D3D12_SHADER_BYTECODE CShader::CreateGeometryShader(int nPipelineState)
{
	D3D12_SHADER_BYTECODE d3dShaderByteCode;
	d3dShaderByteCode.BytecodeLength = 0;
	d3dShaderByteCode.pShaderBytecode = NULL;

	return (d3dShaderByteCode);
}

D3D12_STREAM_OUTPUT_DESC CShader::CreateStreamOuputState(int nPipelineState)
{
	D3D12_STREAM_OUTPUT_DESC d3dStreamOutputDesc;
	::ZeroMemory(&d3dStreamOutputDesc, sizeof(D3D12_STREAM_OUTPUT_DESC));

	d3dStreamOutputDesc.NumEntries = 0;
	d3dStreamOutputDesc.NumStrides = 0;
	d3dStreamOutputDesc.pBufferStrides = NULL;
	d3dStreamOutputDesc.pSODeclaration = NULL;
	d3dStreamOutputDesc.RasterizedStream = 0;

	return(d3dStreamOutputDesc);
}

D3D12_SHADER_BYTECODE CShader::CreateComputeShader(int nPipelineState)
{
	D3D12_SHADER_BYTECODE d3dShaderByteCode;
	d3dShaderByteCode.BytecodeLength = 0;
	d3dShaderByteCode.pShaderBytecode = NULL;

	return (d3dShaderByteCode);
}

D3D12_INPUT_LAYOUT_DESC CShader::CreateInputLayout(int nPipelineState)
{
	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = NULL;
	d3dInputLayoutDesc.NumElements = 0;

	return(d3dInputLayoutDesc);
}

D3D12_RASTERIZER_DESC CShader::CreateRasterizerState(int nPipelineState)
{
	D3D12_RASTERIZER_DESC d3dRasterizerDesc;
	::ZeroMemory(&d3dRasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
	//	d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
	d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	d3dRasterizerDesc.FrontCounterClockwise = FALSE;
	d3dRasterizerDesc.DepthBias = 0;
	d3dRasterizerDesc.DepthBiasClamp = 0.0f;
	d3dRasterizerDesc.SlopeScaledDepthBias = 0.0f;
	d3dRasterizerDesc.DepthClipEnable = TRUE;
	d3dRasterizerDesc.MultisampleEnable = FALSE;
	d3dRasterizerDesc.AntialiasedLineEnable = FALSE;
	d3dRasterizerDesc.ForcedSampleCount = 0;
	d3dRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return(d3dRasterizerDesc);
}

D3D12_BLEND_DESC CShader::CreateBlendState(int nPipelineState)
{
	D3D12_BLEND_DESC d3dBlendDesc;
	::ZeroMemory(&d3dBlendDesc, sizeof(D3D12_BLEND_DESC));
	d3dBlendDesc.AlphaToCoverageEnable = FALSE;
	d3dBlendDesc.IndependentBlendEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].BlendEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].LogicOpEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	d3dBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	d3dBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	d3dBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	d3dBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	d3dBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return(d3dBlendDesc);
}

D3D12_DEPTH_STENCIL_DESC CShader::CreateDepthStencilState(int nPipelineState)
{
	D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
	::ZeroMemory(&d3dDepthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	d3dDepthStencilDesc.DepthEnable = TRUE;
	d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	d3dDepthStencilDesc.StencilEnable = FALSE;
	d3dDepthStencilDesc.StencilReadMask = 0x00;
	d3dDepthStencilDesc.StencilWriteMask = 0x00;
	d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return (d3dDepthStencilDesc);	
}

DXGI_SAMPLE_DESC CShader::GetSampleDesc(int nPipelineState)
{
	DXGI_SAMPLE_DESC dxgiSampleDesc;
	dxgiSampleDesc.Count = 1;
	dxgiSampleDesc.Quality = 0;

	return (dxgiSampleDesc);
}

void CShader::OnPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList, int nPipelineState)
{
	if (nPipelineState < m_pd3dPipelineStates.size())
	{
		if (m_pd3dPipelineStates[nPipelineState]) pd3dCommandList->SetPipelineState(m_pd3dPipelineStates[nPipelineState].Get());
	}
}

void CShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
}

void CShader::CreateCbvSrvDescriptorHeaps(ID3D12Device* pd3dDevice, int nConstantBufferViews, int nShaderResourceViews)
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	d3dDescriptorHeapDesc.NumDescriptors = nConstantBufferViews + nShaderResourceViews; //CBVs + SRVs 
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap);

	m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle = m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle = m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	m_pDescriptorHeap->m_d3dSrvCPUDescriptorStartHandle.ptr = m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * nConstantBufferViews);
	m_pDescriptorHeap->m_d3dSrvGPUDescriptorStartHandle.ptr = m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * nConstantBufferViews);

	m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle;
	m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle;
	m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dSrvCPUDescriptorStartHandle;
	m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dSrvGPUDescriptorStartHandle;
}

void CShader::CreateShaderResourceViews(ID3D12Device* pd3dDevice, CTexture* pTexture, UINT nDescriptorHeapIndex, UINT nRootParameterStartIndex)
{
	m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += (::gnCbvSrvDescriptorIncrementSize * nDescriptorHeapIndex);
	m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += (::gnCbvSrvDescriptorIncrementSize * nDescriptorHeapIndex);

	int nTextures = pTexture->GetTextures();
	UINT nTextureType = pTexture->GetTextureType();
	for (int i = 0; i < nTextures; i++)
	{
		ID3D12Resource* pShaderResource = pTexture->GetResource(i);
		D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = pTexture->GetShaderResourceViewDesc(i);
		pd3dDevice->CreateShaderResourceView(pShaderResource, &d3dShaderResourceViewDesc, m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

		pTexture->SetGpuDescriptorHandle(i, m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

	}
	int nRootParameters = pTexture->GetRootParameters();
	for (int i = 0; i < nRootParameters; i++) pTexture->SetRootParameterIndex(i, nRootParameterStartIndex + i);
}

///////////////////////////////////////////////////////////////////////////////


CStandardShader::CStandardShader()
{
}

CStandardShader::~CStandardShader()
{
}

D3D12_SHADER_BYTECODE CStandardShader::CreateVertexShader(int nPipelineState)
{
#ifdef _COMPILE_SHADER
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "VSStandard", "vs_5_1", &m_pd3dVertexShaderBlob));
#else
	return(CShader::ReadCompiledShaderFromFile(L"VSStandard", &m_pd3dVertexShaderBlob));
#endif
}

D3D12_SHADER_BYTECODE CStandardShader::CreatePixelShader(int nPipelineState)
{
#ifdef _COMPILE_SHADER
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PSStandard", "ps_5_1", &m_pd3dPixelShaderBlob));
#else
	return(CShader::ReadCompiledShaderFromFile(L"PSStandard", &m_pd3dPixelShaderBlob));
#endif
}

D3D12_INPUT_LAYOUT_DESC CStandardShader::CreateInputLayout(int nPipelineState)
{
	UINT nInputElementDescs = 5;
	D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[2] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[3] = { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[4] = { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 4, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

///////////////////////////////////////////////////////////////////////////////
//

CSkyBoxShader::CSkyBoxShader()
{
}

CSkyBoxShader::~CSkyBoxShader()
{
}

D3D12_SHADER_BYTECODE CSkyBoxShader::CreateVertexShader(int nPipelineState)
{
#ifdef _COMPILE_SHADER
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "VSSkyBox", "vs_5_1", &m_pd3dVertexShaderBlob));
#else
	return(CShader::ReadCompiledShaderFromFile(L"VSSkyBox", &m_pd3dVertexShaderBlob));
#endif
}

D3D12_SHADER_BYTECODE CSkyBoxShader::CreatePixelShader(int nPipelineState)
{
#ifdef _COMPILE_SHADER
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PSSkyBox", "ps_5_1", &m_pd3dPixelShaderBlob));
#else
	return(CShader::ReadCompiledShaderFromFile(L"PSSkyBox", &m_pd3dPixelShaderBlob));
#endif
}

D3D12_INPUT_LAYOUT_DESC CSkyBoxShader::CreateInputLayout(int nPipelineState)
{
	UINT nInputElementDescs = 1;
	D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

D3D12_DEPTH_STENCIL_DESC CSkyBoxShader::CreateDepthStencilState(int nPipelineState)
{
	D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
	d3dDepthStencilDesc.DepthEnable = TRUE;
	d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	d3dDepthStencilDesc.StencilEnable = FALSE;
	d3dDepthStencilDesc.StencilReadMask = 0xff;
	d3dDepthStencilDesc.StencilWriteMask = 0xff;
	d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_INCR;
	d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_DECR;
	d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	return(d3dDepthStencilDesc);
}

///////////////////////////////////////////////////////////////////////////////
//

CTerrainShader::CTerrainShader()
{
}

CTerrainShader::~CTerrainShader()
{
}

void CTerrainShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	// Resize Pipeline State Vector
	m_pd3dPipelineStates.resize(1);

	// Create Pipeline State
	CreateGraphicsPipelineState(pd3dDevice, pd3dGraphicsRootSignature, 0);
}

D3D12_INPUT_LAYOUT_DESC CTerrainShader::CreateInputLayout(int nPipelineState)
{
	UINT nInputElementDescs = 5;
	D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];
		
	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[1] = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[2] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[3] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[4] = { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;
	
	return(d3dInputLayoutDesc);
}

D3D12_SHADER_BYTECODE CTerrainShader::CreateVertexShader(int nPipelineState)
{
#ifdef _COMPILE_SHADER
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "VSTerrain", "vs_5_1", &m_pd3dVertexShaderBlob));
#else
	return(CShader::ReadCompiledShaderFromFile(L"VSTerrain", &m_pd3dVertexShaderBlob));
#endif
}

D3D12_SHADER_BYTECODE CTerrainShader::CreatePixelShader(int nPipelineState)
{
#ifdef _COMPILE_SHADER
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PSTerrain", "ps_5_1", &m_pd3dPixelShaderBlob));
#else
	return(CShader::ReadCompiledShaderFromFile(L"PSTerrain", &m_pd3dPixelShaderBlob));
#endif
}

///////////////////////////////////////////////////////////////////////////////
//

CSkinnedAnimationStandardShader::CSkinnedAnimationStandardShader()
{
}

CSkinnedAnimationStandardShader::~CSkinnedAnimationStandardShader()
{
}

D3D12_INPUT_LAYOUT_DESC CSkinnedAnimationStandardShader::CreateInputLayout(int nPipelineState)
{
	UINT nInputElementDescs = 7;
	D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[2] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[3] = { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[4] = { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 4, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[5] = { "BONEINDEX", 0, DXGI_FORMAT_R32G32B32A32_SINT, 5, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[6] = { "BONEWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 6, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

D3D12_SHADER_BYTECODE CSkinnedAnimationStandardShader::CreateVertexShader(int nPipelineState)
{
#ifdef _COMPILE_SHADER
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "VSSkinnedAnimationStandard", "vs_5_1", &m_pd3dVertexShaderBlob));
#else
	return(CShader::ReadCompiledShaderFromFile(L"VSSkinnedAnimationStandard", &m_pd3dVertexShaderBlob));
#endif
}

///////////////////////////////////////////////////////////////////////////////////
//

CColliderShader::CColliderShader()
{
}

CColliderShader::~CColliderShader()
{
}

D3D12_INPUT_LAYOUT_DESC CColliderShader::CreateInputLayout(int nPipelineState)
{
	UINT nInputElementDescs = 1;
	D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

D3D12_SHADER_BYTECODE CColliderShader::CreateVertexShader(int nPipelineState)
{
#ifdef _COMPILE_SHADER
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "VSCollider", "vs_5_1", &m_pd3dVertexShaderBlob));
#else
	return(CShader::ReadCompiledShaderFromFile(L"VSCollider", &m_pd3dVertexShaderBlob));
#endif
}

D3D12_SHADER_BYTECODE CColliderShader::CreatePixelShader(int nPipelineState)
{
#ifdef _COMPILE_SHADER
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PSCollider", "ps_5_1", &m_pd3dPixelShaderBlob));
#else
	return(CShader::ReadCompiledShaderFromFile(L"PSCollider", &m_pd3dPixelShaderBlob));
#endif
}

D3D12_RASTERIZER_DESC CColliderShader::CreateRasterizerState(int nPipelineState)
{
	D3D12_RASTERIZER_DESC d3dRasterizerDesc;
	::ZeroMemory(&d3dRasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
	d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
	//d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	d3dRasterizerDesc.FrontCounterClockwise = FALSE;
	d3dRasterizerDesc.DepthBias = 0;
	d3dRasterizerDesc.DepthBiasClamp = 0.0f;
	d3dRasterizerDesc.SlopeScaledDepthBias = 0.0f;
	d3dRasterizerDesc.DepthClipEnable = TRUE;
	d3dRasterizerDesc.MultisampleEnable = FALSE;
	d3dRasterizerDesc.AntialiasedLineEnable = FALSE;
	d3dRasterizerDesc.ForcedSampleCount = 0;
	d3dRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return(d3dRasterizerDesc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CTexturedShader::CTexturedShader()
{
}

CTexturedShader::~CTexturedShader()
{
}

D3D12_INPUT_LAYOUT_DESC CTexturedShader::CreateInputLayout(int nPipelineState)
{
	UINT nInputElementDescs = 2;
	D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

D3D12_SHADER_BYTECODE CTexturedShader::CreateVertexShader(ID3DBlob** ppd3dShaderBlob, int nPipelineState)
{
#ifdef _COMPILE_SHADER
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "VSTextured", "vs_5_1", ppd3dShaderBlob));
#else
	return(CShader::ReadCompiledShaderFromFile(L"VSTextured", ppd3dShaderBlob));
#endif
}

D3D12_SHADER_BYTECODE CTexturedShader::CreatePixelShader(ID3DBlob** ppd3dShaderBlob, int nPipelineState)
{
#ifdef _COMPILE_SHADER
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PSTextured", "ps_5_1", ppd3dShaderBlob));
#else
	return(CShader::ReadCompiledShaderFromFile(L"PSTextured", ppd3dShaderBlob));
#endif
}

void CTexturedShader::CreateGraphicsPipelineState(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState)
{
	m_nPipelineStates = 1;
	m_pd3dPipelineStates.resize(m_nPipelineStates);

	CShader::CreateGraphicsPipelineState(pd3dDevice, pd3dGraphicsRootSignature, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CBillboardShader::CBillboardShader()
{
}

CBillboardShader::~CBillboardShader()
{
}

D3D12_SHADER_BYTECODE CBillboardShader::CreateVertexShader(ID3DBlob** ppd3dShaderBlob, int nPipelineState)
{
#ifdef _COMPILE_SHADER
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "VSBillboard", "vs_5_1", ppd3dShaderBlob));
#else
	return(CShader::ReadCompiledShaderFromFile(L"VSBillboard", ppd3dShaderBlob));
#endif
}

D3D12_SHADER_BYTECODE CBillboardShader::CreatePixelShader(ID3DBlob** ppd3dShaderBlob, int nPipelineState)
{
#ifdef _COMPILE_SHADER
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PSBillboard", "ps_5_1", ppd3dShaderBlob));
#else
	return(CShader::ReadCompiledShaderFromFile(L"PSBillboard", ppd3dShaderBlob));
#endif
}

D3D12_RASTERIZER_DESC CBillboardShader::CreateRasterizerState(int nPipelineState)
{
	D3D12_RASTERIZER_DESC d3dRasterizerDesc;
	::ZeroMemory(&d3dRasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
	d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	d3dRasterizerDesc.FrontCounterClockwise = FALSE;
	d3dRasterizerDesc.DepthBias = 0;
	d3dRasterizerDesc.DepthBiasClamp = 0.0f;
	d3dRasterizerDesc.SlopeScaledDepthBias = 0.0f;
	d3dRasterizerDesc.DepthClipEnable = TRUE;
	d3dRasterizerDesc.MultisampleEnable = FALSE;
	d3dRasterizerDesc.AntialiasedLineEnable = FALSE;
	d3dRasterizerDesc.ForcedSampleCount = 0;
	d3dRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return(d3dRasterizerDesc);
}

D3D12_BLEND_DESC CBillboardShader::CreateBlendState(int nPipelineState)
{
	D3D12_BLEND_DESC d3dBlendDesc;
	::ZeroMemory(&d3dBlendDesc, sizeof(D3D12_BLEND_DESC));
	d3dBlendDesc.AlphaToCoverageEnable = TRUE;
	d3dBlendDesc.IndependentBlendEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	d3dBlendDesc.RenderTarget[0].LogicOpEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	d3dBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	d3dBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	d3dBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	d3dBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	d3dBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return(d3dBlendDesc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

#include "Scene.h";

CBulletShader::CBulletShader()
{
}

CBulletShader::~CBulletShader()
{
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE CBulletShader::GetPrimitiveTopologyType(int nPipelineState)
{
	return(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
}

UINT CBulletShader::GetNumRenderTargets(int nPipelineState)
{
	return((nPipelineState == 0) ? 0 : 1);
}

DXGI_FORMAT CBulletShader::GetRTVFormat(int nPipelineState, int nRenderTarget)
{
	return((nPipelineState == 0) ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_R8G8B8A8_UNORM);
}

DXGI_FORMAT CBulletShader::GetDSVFormat(int nPipelineState)
{
	return(DXGI_FORMAT_D24_UNORM_S8_UINT);
}

D3D12_SHADER_BYTECODE CBulletShader::CreateVertexShader(int nPipelineState)
{
#ifdef _COMPILE_SHADER
	if (nPipelineState == 0)
		return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "VSBulletStreamOutput", "vs_5_1", &m_pd3dVertexShaderBlob));
	else
		return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "VSBulletDraw", "vs_5_1", &m_pd3dVertexShaderBlob));
#else
	if (nPipelineState == 0)
		return(CShader::ReadCompiledShaderFromFile(L"VSBulletStreamOutput", &m_pd3dVertexShaderBlob));
	else
		return(CShader::ReadCompiledShaderFromFile(L"VSBulletDraw", &m_pd3dVertexShaderBlob));
#endif

}

D3D12_SHADER_BYTECODE CBulletShader::CreateGeometryShader(int nPipelineState)
{
#ifdef _COMPILE_SHADER
	if (nPipelineState == 0)
		return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "GSBulletStreamOutput", "gs_5_1", &m_pd3dGeometryShaderBlob));
	else
		return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "GSBulletDraw", "gs_5_1", &m_pd3dGeometryShaderBlob));
#else
	if (nPipelineState == 0)
		return(CShader::ReadCompiledShaderFromFile(L"GSBulletStreamOutput", &m_pd3dGeometryShaderBlob));
	else
		return(CShader::ReadCompiledShaderFromFile(L"GSBulletDraw", &m_pd3dGeometryShaderBlob));
#endif
}

D3D12_SHADER_BYTECODE CBulletShader::CreatePixelShader(int nPipelineState)
{
#ifdef _COMPILE_SHADER
	if (nPipelineState == 0)
		return(CShader::CreatePixelShader(0));
	else
		return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PSBulletDraw", "ps_5_1", &m_pd3dPixelShaderBlob));
#else
	if (nPipelineState == 0)
		return(CShader::CreatePixelShader(0));
	else
		return(CShader::ReadCompiledShaderFromFile(L"PSBulletDraw", &m_pd3dPixelShaderBlob));
#endif
}

D3D12_BLEND_DESC CBulletShader::CreateBlendState(int nPipelineState)
{
	D3D12_BLEND_DESC d3dBlendDesc;
	::ZeroMemory(&d3dBlendDesc, sizeof(D3D12_BLEND_DESC));
	d3dBlendDesc.AlphaToCoverageEnable = FALSE;
	d3dBlendDesc.IndependentBlendEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	d3dBlendDesc.RenderTarget[0].LogicOpEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	d3dBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	d3dBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
	d3dBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	d3dBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	d3dBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return(d3dBlendDesc);
}

D3D12_DEPTH_STENCIL_DESC CBulletShader::CreateDepthStencilState(int nPipelineState)
{
	D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
	::ZeroMemory(&d3dDepthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	d3dDepthStencilDesc.DepthEnable = TRUE;
	d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;	
	d3dDepthStencilDesc.StencilEnable = FALSE;
	d3dDepthStencilDesc.StencilReadMask = 0x00;
	d3dDepthStencilDesc.StencilWriteMask = 0x00;
	d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return(d3dDepthStencilDesc);
}

D3D12_INPUT_LAYOUT_DESC CBulletShader::CreateInputLayout(int nPipelineState)
{
	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	UINT nInputElementDescs = 5;
	D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[1] = { "LASTPOSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[2] = { "VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[3] = { "LIFETIME", 0, DXGI_FORMAT_R32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[4] = { "TYPE", 0, DXGI_FORMAT_R32_SINT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

D3D12_STREAM_OUTPUT_DESC CBulletShader::CreateStreamOuputState(int nPipelineState)
{
	D3D12_STREAM_OUTPUT_DESC d3dStreamOutputDesc;
	::ZeroMemory(&d3dStreamOutputDesc, sizeof(D3D12_STREAM_OUTPUT_DESC));

	if (nPipelineState == 0)
	{
		UINT nStreamOutputDecls = 5;
		D3D12_SO_DECLARATION_ENTRY* pd3dStreamOutputDecls = new D3D12_SO_DECLARATION_ENTRY[nStreamOutputDecls];
		pd3dStreamOutputDecls[0] = { 0, "POSITION", 0, 0, 3, 0 };
		pd3dStreamOutputDecls[1] = { 0, "LASTPOSITION", 0, 0, 3, 0 };
		pd3dStreamOutputDecls[2] = { 0, "VELOCITY", 0, 0, 3, 0 };
		pd3dStreamOutputDecls[3] = { 0, "LIFETIME", 0, 0, 1, 0 };
		pd3dStreamOutputDecls[4] = { 0, "TYPE", 0, 0, 1, 0 };

		UINT* pBufferStrides = new UINT[1];
		pBufferStrides[0] = sizeof(CBulletVertex);

		d3dStreamOutputDesc.NumEntries = nStreamOutputDecls;
		d3dStreamOutputDesc.pSODeclaration = pd3dStreamOutputDecls;
		d3dStreamOutputDesc.NumStrides = 1;
		d3dStreamOutputDesc.pBufferStrides = pBufferStrides;
		d3dStreamOutputDesc.RasterizedStream = D3D12_SO_NO_RASTERIZED_STREAM;
	}

	return(d3dStreamOutputDesc);
}

void CBulletShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	m_nPipelineStates = 2;
	m_pd3dPipelineStates.resize(m_nPipelineStates);

	CShader::CreateGraphicsPipelineState(pd3dDevice, pd3dGraphicsRootSignature, 0); //Stream Output Pipeline State
	CShader::CreateGraphicsPipelineState(pd3dDevice, pd3dGraphicsRootSignature, 1); //Draw Pipeline State
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

CIlluminatedShader::CIlluminatedShader()
{
}

CIlluminatedShader::~CIlluminatedShader()
{
}

D3D12_INPUT_LAYOUT_DESC CIlluminatedShader::CreateInputLayout(int nPipelineState)
{
	UINT nInputElementDescs = 2;
	D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

D3D12_SHADER_BYTECODE CIlluminatedShader::CreateVertexShader(int nPipelineState)
{
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "VSLighting", "vs_5_1", m_pd3dVertexShaderBlob.GetAddressOf()));
}

D3D12_SHADER_BYTECODE CIlluminatedShader::CreatePixelShader(int nPipelineState)
{
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PSLighting", "ps_5_1", m_pd3dPixelShaderBlob.GetAddressOf()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

CDepthRenderShader::CDepthRenderShader(CScene* pScene)
	: CIlluminatedShader(), m_pScene(pScene), m_pd3dcbToLightSpaces(nullptr), m_pcbMappedToLightSpaces(nullptr)
{
	m_pToLightSpaces.resize(1);

	XMFLOAT4X4 xmf4x4ToTexture = { 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.0f, 1.0f };
	m_xmProjectionToTexture = XMLoadFloat4x4(&xmf4x4ToTexture);
}

CDepthRenderShader::~CDepthRenderShader()
{
}

D3D12_DEPTH_STENCIL_DESC CDepthRenderShader::CreateDepthStencilState(int nPipelineState)
{
	D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
	::ZeroMemory(&d3dDepthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	d3dDepthStencilDesc.DepthEnable = TRUE;
	d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS; //D3D12_COMPARISON_FUNC_LESS_EQUAL
	d3dDepthStencilDesc.StencilEnable = FALSE;
	d3dDepthStencilDesc.StencilReadMask = 0x00;
	d3dDepthStencilDesc.StencilWriteMask = 0x00;
	d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return(d3dDepthStencilDesc);
}

D3D12_RASTERIZER_DESC CDepthRenderShader::CreateRasterizerState(int nPipelineState)
{
	D3D12_RASTERIZER_DESC d3dRasterizerDesc;
	::ZeroMemory(&d3dRasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
	d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	d3dRasterizerDesc.FrontCounterClockwise = FALSE;
#ifdef _WITH_RASTERIZER_DEPTH_BIAS
	d3dRasterizerDesc.DepthBias = 250000;
#endif
	d3dRasterizerDesc.DepthBiasClamp = 0.0f;
	d3dRasterizerDesc.SlopeScaledDepthBias = 1.0f;
	d3dRasterizerDesc.DepthClipEnable = TRUE;
	d3dRasterizerDesc.MultisampleEnable = FALSE;
	d3dRasterizerDesc.AntialiasedLineEnable = FALSE;
	d3dRasterizerDesc.ForcedSampleCount = 0;
	d3dRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return(d3dRasterizerDesc);
}

D3D12_SHADER_BYTECODE CDepthRenderShader::CreatePixelShader(int nPipelineState)
{
#ifdef _COMPILE_SHADER
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PSDepthWriteShader", "ps_5_1", m_pd3dPixelShaderBlob.GetAddressOf()));
#else
	return(CShader::ReadCompiledShaderFromFile(L"PSDepthWriteShader", m_pd3dPixelShaderBlob.GetAddressOf()));
#endif
}

void CDepthRenderShader::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	UINT ncbDepthElementBytes;

	ncbDepthElementBytes = ((sizeof(TOLIGHTSPACEINFO) + 255) & ~255); //256의 배수
	m_pd3dcbToLightSpaces = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbDepthElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbToLightSpaces->Map(0, NULL, (void**)&m_pcbMappedToLightSpaces);
}

void CDepthRenderShader::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	::memcpy(m_pcbMappedToLightSpaces, m_pToLightSpaces.data(), sizeof(TOLIGHTSPACEINFO));

	D3D12_GPU_VIRTUAL_ADDRESS d3dcbToLightGpuVirtualAddress = m_pd3dcbToLightSpaces->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(ROOT_PARAMETER_TO_LIGHT, d3dcbToLightGpuVirtualAddress);
}

void CDepthRenderShader::ReleaseShaderVariables()
{
	if (m_pd3dcbToLightSpaces)
	{
		m_pd3dcbToLightSpaces->Unmap(0, NULL);
		m_pd3dcbToLightSpaces.Reset();
	}
}

void CDepthRenderShader::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pContext)
{
	m_nDepthbufferWidth = WINDOW_WIDTH * 4;
	m_nDepthbufferHeight = WINDOW_HEIGHT * 4;

	m_pDescriptorHeap = std::make_unique<CDescirptorHeap>();

	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	d3dDescriptorHeapDesc.NumDescriptors = MAX_DEPTH_TEXTURES;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	HRESULT hResult = pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dRtvDescriptorHeap);

	m_pDepthFromLightTexture = std::make_shared<CTexture>(MAX_DEPTH_TEXTURES, RESOURCE_TEXTURE2D_ARRAY, 1);

	D3D12_CLEAR_VALUE d3dClearValue = { DXGI_FORMAT_R32_FLOAT, { 1.0f, 1.0f, 1.0f, 1.0f } };
	for (UINT i = 0; i < MAX_DEPTH_TEXTURES; i++) m_pDepthFromLightTexture->CreateTexture(pd3dDevice, pd3dCommandList, i, RESOURCE_TEXTURE2D, m_nDepthbufferWidth, m_nDepthbufferHeight, 1, 0, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON, &d3dClearValue);

	D3D12_RENDER_TARGET_VIEW_DESC d3dRenderTargetViewDesc;
	d3dRenderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	d3dRenderTargetViewDesc.Texture2D.MipSlice = 0;
	d3dRenderTargetViewDesc.Texture2D.PlaneSlice = 0;
	d3dRenderTargetViewDesc.Format = DXGI_FORMAT_R32_FLOAT;

	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < MAX_DEPTH_TEXTURES; i++)
	{
		ID3D12Resource* pd3dTextureResource = m_pDepthFromLightTexture->GetResource(i);
		pd3dDevice->CreateRenderTargetView(pd3dTextureResource, &d3dRenderTargetViewDesc, d3dRtvCPUDescriptorHandle);
		m_pd3dRtvCPUDescriptorHandles[i] = d3dRtvCPUDescriptorHandle;
		d3dRtvCPUDescriptorHandle.ptr += ::gnRtvDescriptorIncrementSize;
	}

	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hResult = pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dDsvDescriptorHeap);

	D3D12_RESOURCE_DESC d3dResourceDesc;
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = m_nDepthbufferWidth;
	d3dResourceDesc.Height = m_nDepthbufferHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
	d3dResourceDesc.SampleDesc.Count = 1;
	d3dResourceDesc.SampleDesc.Quality = 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	::ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapProperties.CreationNodeMask = 1;
	d3dHeapProperties.VisibleNodeMask = 1;

	d3dClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;

	pd3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dClearValue, __uuidof(ID3D12Resource), (void**)&m_pd3dDepthBuffer);

	D3D12_DEPTH_STENCIL_VIEW_DESC d3dDepthStencilViewDesc;
	::ZeroMemory(&d3dDepthStencilViewDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
	d3dDepthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	d3dDepthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	d3dDepthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

	m_d3dDsvDescriptorCPUHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	pd3dDevice->CreateDepthStencilView(m_pd3dDepthBuffer.Get(), &d3dDepthStencilViewDesc, m_d3dDsvDescriptorCPUHandle);

	for (int i = 0; i < MAX_DEPTH_TEXTURES; i++)
	{
		m_ppDepthRenderCameras[i] = std::make_shared<CCamera>();
		m_ppDepthRenderCameras[i]->SetViewport(0, 0, m_nDepthbufferWidth, m_nDepthbufferHeight, 0.0f, 1.0f);
		m_ppDepthRenderCameras[i]->SetScissorRect(0, 0, m_nDepthbufferWidth, m_nDepthbufferHeight);
		m_ppDepthRenderCameras[i]->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	}

	CreateCbvSrvDescriptorHeaps(pd3dDevice, 0, m_pDepthFromLightTexture->GetTextures());
	CreateShaderResourceViews(pd3dDevice, m_pDepthFromLightTexture.get(), 0, ROOT_PARAMETER_DEPTH_WRITE);

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CDepthRenderShader::ReleaseObjects()
{
	for (int i = 0; i < MAX_DEPTH_TEXTURES; i++)
	{
		if (m_ppDepthRenderCameras[i])
		{
			m_ppDepthRenderCameras[i]->ReleaseShaderVariables();
			m_ppDepthRenderCameras[i].reset();
		}
	}

	if (m_pDepthFromLightTexture) m_pDepthFromLightTexture.reset();
	if (m_pd3dDepthBuffer) m_pd3dDepthBuffer.Reset();

	if (m_pd3dRtvDescriptorHeap) m_pd3dRtvDescriptorHeap.Reset();
	if (m_pd3dDsvDescriptorHeap) m_pd3dDsvDescriptorHeap.Reset();
}

void CreateFrustumPoints(XMMATRIX& xmmtxProjection, XMVECTOR* pxmvCornerPoints)
{
	BoundingFrustum xmFrustrum(xmmtxProjection);

	static const XMVECTORU32 vGrabY = { 0x00000000,0xFFFFFFFF,0x00000000,0x00000000 };
	static const XMVECTORU32 vGrabX = { 0xFFFFFFFF,0x00000000,0x00000000,0x00000000 };

	XMVECTORF32 vRightTop = { xmFrustrum.RightSlope, xmFrustrum.TopSlope, 1.0f, 1.0f };
	XMVECTORF32 vLeftBottom = { xmFrustrum.LeftSlope, xmFrustrum.BottomSlope, 1.0f, 1.0f };
	XMVECTORF32 vNear = { xmFrustrum.Near, xmFrustrum.Near, xmFrustrum.Near, 1.0f };
	XMVECTORF32 vFar = { xmFrustrum.Far, xmFrustrum.Far, xmFrustrum.Far, 1.0f };
	XMVECTOR vRightTopNear = XMVectorMultiply(vRightTop, vNear);
	XMVECTOR vRightTopFar = XMVectorMultiply(vRightTop, vFar);
	XMVECTOR vLeftBottomNear = XMVectorMultiply(vLeftBottom, vNear);
	XMVECTOR vLeftBottomFar = XMVectorMultiply(vLeftBottom, vFar);

	pxmvCornerPoints[0] = vRightTopNear;
	pxmvCornerPoints[1] = XMVectorSelect(vRightTopNear, vLeftBottomNear, vGrabX);
	pxmvCornerPoints[2] = vLeftBottomNear;
	pxmvCornerPoints[3] = XMVectorSelect(vRightTopNear, vLeftBottomNear, vGrabY);

	pxmvCornerPoints[4] = vRightTopFar;
	pxmvCornerPoints[5] = XMVectorSelect(vRightTopFar, vLeftBottomFar, vGrabX);
	pxmvCornerPoints[6] = vLeftBottomFar;
	pxmvCornerPoints[7] = XMVectorSelect(vRightTopFar, vLeftBottomFar, vGrabY);
}

struct TRIANGLECULLING
{
	XMVECTOR pt[3];
	BOOL culled;
};

void ComputeOrthographicProjectionNearAndFar(float& fNear, float& fFar, FXMVECTOR xmvLightOrthographicMin, FXMVECTOR xmvLightOrthographicMax, XMVECTOR* pxmvSceneAABBLightSpacePoints)
{
	fNear = FLT_MAX;
	fFar = -FLT_MAX;

	TRIANGLECULLING pxmvSceneAABBTriangles[16];
	int nTriangles = 1;

	static const int pnTriangleIndices[36] = { 0,1,2, 1,2,3, 4,5,6, 5,6,7, 0,2,4, 2,4,6, 1,3,5, 3,5,7, 0,1,4, 1,4,5, 2,3,6, 3,6,7 };
	int nPointPassesCollision[3];

	float fxOrthographicMin = XMVectorGetX(xmvLightOrthographicMin);
	float fxOrthographicMax = XMVectorGetX(xmvLightOrthographicMax);
	float fyOrthographicMin = XMVectorGetY(xmvLightOrthographicMin);
	float fyOrthographicMax = XMVectorGetY(xmvLightOrthographicMax);

	for (int i = 0; i < 12; i++)
	{
		pxmvSceneAABBTriangles[0].pt[0] = pxmvSceneAABBLightSpacePoints[pnTriangleIndices[i * 3 + 0]];
		pxmvSceneAABBTriangles[0].pt[1] = pxmvSceneAABBLightSpacePoints[pnTriangleIndices[i * 3 + 1]];
		pxmvSceneAABBTriangles[0].pt[2] = pxmvSceneAABBLightSpacePoints[pnTriangleIndices[i * 3 + 2]];
		pxmvSceneAABBTriangles[0].culled = false;

		nTriangles = 1;

		for (int j = 0; j < 4; j++)
		{
			float fEdge;
			int nComponent;

			if (j == 0)
			{
				fEdge = fxOrthographicMin;
				nComponent = 0;
			}
			else if (j == 1)
			{
				fEdge = fxOrthographicMax;
				nComponent = 0;
			}
			else if (j == 2)
			{
				fEdge = fyOrthographicMin;
				nComponent = 1;
			}
			else
			{
				fEdge = fyOrthographicMax;
				nComponent = 1;
			}

			for (int k = 0; k < nTriangles; k++)
			{
				if (!pxmvSceneAABBTriangles[k].culled)
				{
					int nInsideVertices = 0;
					XMVECTOR tempOrder;

					if (j == 0)
					{
						for (int m = 0; m < 3; m++)
						{
							nPointPassesCollision[m] = (XMVectorGetX(pxmvSceneAABBTriangles[k].pt[m]) > XMVectorGetX(xmvLightOrthographicMin)) ? 1 : 0;
							nInsideVertices += nPointPassesCollision[m];
						}
					}
					else if (j == 1)
					{
						for (int m = 0; m < 3; m++)
						{
							nPointPassesCollision[m] = (XMVectorGetX(pxmvSceneAABBTriangles[k].pt[m]) < XMVectorGetX(xmvLightOrthographicMax)) ? 1 : 0;
							nInsideVertices += nPointPassesCollision[m];
						}
					}
					else if (j == 2)
					{
						for (int m = 0; m < 3; m++)
						{
							nPointPassesCollision[m] = (XMVectorGetY(pxmvSceneAABBTriangles[k].pt[m]) > XMVectorGetY(xmvLightOrthographicMin)) ? 1 : 0;
							nInsideVertices += nPointPassesCollision[m];
						}
					}
					else
					{
						for (int m = 0; m < 3; m++)
						{
							nPointPassesCollision[m] = (XMVectorGetY(pxmvSceneAABBTriangles[k].pt[m]) < XMVectorGetY(xmvLightOrthographicMax)) ? 1 : 0;
							nInsideVertices += nPointPassesCollision[m];
						}
					}

					if (nPointPassesCollision[1] && !nPointPassesCollision[0])
					{
						tempOrder = pxmvSceneAABBTriangles[k].pt[0];
						pxmvSceneAABBTriangles[k].pt[0] = pxmvSceneAABBTriangles[k].pt[1];
						pxmvSceneAABBTriangles[k].pt[1] = tempOrder;
						nPointPassesCollision[0] = TRUE;
						nPointPassesCollision[1] = FALSE;
					}
					if (nPointPassesCollision[2] && !nPointPassesCollision[1])
					{
						tempOrder = pxmvSceneAABBTriangles[k].pt[1];
						pxmvSceneAABBTriangles[k].pt[1] = pxmvSceneAABBTriangles[k].pt[2];
						pxmvSceneAABBTriangles[k].pt[2] = tempOrder;
						nPointPassesCollision[1] = TRUE;
						nPointPassesCollision[2] = FALSE;
					}
					if (nPointPassesCollision[1] && !nPointPassesCollision[0])
					{
						tempOrder = pxmvSceneAABBTriangles[k].pt[0];
						pxmvSceneAABBTriangles[k].pt[0] = pxmvSceneAABBTriangles[k].pt[1];
						pxmvSceneAABBTriangles[k].pt[1] = tempOrder;
						nPointPassesCollision[0] = TRUE;
						nPointPassesCollision[1] = FALSE;
					}

					if (nInsideVertices == 0)
					{
						pxmvSceneAABBTriangles[k].culled = true;
					}
					else if (nInsideVertices == 1)
					{
						pxmvSceneAABBTriangles[k].culled = false;

						XMVECTOR vVert0ToVert1 = pxmvSceneAABBTriangles[k].pt[1] - pxmvSceneAABBTriangles[k].pt[0];
						XMVECTOR vVert0ToVert2 = pxmvSceneAABBTriangles[k].pt[2] - pxmvSceneAABBTriangles[k].pt[0];

						float fHitPointTimeRatio = fEdge - XMVectorGetByIndex(pxmvSceneAABBTriangles[k].pt[0], nComponent);
						float fDistanceAlongVector01 = fHitPointTimeRatio / XMVectorGetByIndex(vVert0ToVert1, nComponent);
						float fDistanceAlongVector02 = fHitPointTimeRatio / XMVectorGetByIndex(vVert0ToVert2, nComponent);
						vVert0ToVert1 *= fDistanceAlongVector01;
						vVert0ToVert1 += pxmvSceneAABBTriangles[k].pt[0];
						vVert0ToVert2 *= fDistanceAlongVector02;
						vVert0ToVert2 += pxmvSceneAABBTriangles[k].pt[0];

						pxmvSceneAABBTriangles[k].pt[1] = vVert0ToVert2;
						pxmvSceneAABBTriangles[k].pt[2] = vVert0ToVert1;
					}
					else if (nInsideVertices == 2)
					{
						pxmvSceneAABBTriangles[nTriangles] = pxmvSceneAABBTriangles[k + 1];

						pxmvSceneAABBTriangles[k].culled = false;
						pxmvSceneAABBTriangles[k + 1].culled = false;

						XMVECTOR vVert2ToVert0 = pxmvSceneAABBTriangles[k].pt[0] - pxmvSceneAABBTriangles[k].pt[2];
						XMVECTOR vVert2ToVert1 = pxmvSceneAABBTriangles[k].pt[1] - pxmvSceneAABBTriangles[k].pt[2];

						float fHitPointTime_2_0 = fEdge - XMVectorGetByIndex(pxmvSceneAABBTriangles[k].pt[2], nComponent);
						float fDistanceAlongVector_2_0 = fHitPointTime_2_0 / XMVectorGetByIndex(vVert2ToVert0, nComponent);
						vVert2ToVert0 *= fDistanceAlongVector_2_0;
						vVert2ToVert0 += pxmvSceneAABBTriangles[k].pt[2];

						pxmvSceneAABBTriangles[k + 1].pt[0] = pxmvSceneAABBTriangles[k].pt[0];
						pxmvSceneAABBTriangles[k + 1].pt[1] = pxmvSceneAABBTriangles[k].pt[1];
						pxmvSceneAABBTriangles[k + 1].pt[2] = vVert2ToVert0;

						float fHitPointTime_2_1 = fEdge - XMVectorGetByIndex(pxmvSceneAABBTriangles[k].pt[2], nComponent);
						float fDistanceAlongVector_2_1 = fHitPointTime_2_1 / XMVectorGetByIndex(vVert2ToVert1, nComponent);
						vVert2ToVert1 *= fDistanceAlongVector_2_1;
						vVert2ToVert1 += pxmvSceneAABBTriangles[k].pt[2];
						pxmvSceneAABBTriangles[k].pt[0] = pxmvSceneAABBTriangles[k + 1].pt[1];
						pxmvSceneAABBTriangles[k].pt[1] = pxmvSceneAABBTriangles[k + 1].pt[2];
						pxmvSceneAABBTriangles[k].pt[2] = vVert2ToVert1;

						++nTriangles;
						++k;
					}
					else
					{
						pxmvSceneAABBTriangles[k].culled = false;
					}
				}
			}
		}
		for (int n = 0; n < nTriangles; n++)
		{
			if (!pxmvSceneAABBTriangles[n].culled)
			{
				for (int x = 0; x < 3; x++)
				{
					float z = XMVectorGetZ(pxmvSceneAABBTriangles[n].pt[x]);
					if (fNear > z) fNear = z;
					if (fFar < z) fFar = z;
				}
			}
		}
	}
}

#define _WITH_ORTHOGRAPHIC_PROJECT_SCENE
//#define _WITH_ORTHOGRAPHIC_PROJECT_CAMERA_FRUSTUM
//#define _WITH_ORTHOGRAPHIC_PROJECT_CAMERA_FRUSTUM_SCENE

XMMATRIX CreateOrthographicProjectionMatrix(XMMATRIX& xmmtxLightView, CCamera* pSceneCamera, BoundingBox* pxmSceneBoundingBox)
{
	XMMATRIX xmmtxProjection;

#ifdef _WITH_ORTHOGRAPHIC_PROJECT_SCENE
	XMFLOAT3 pxmf3SceneAABBPoints[8];
	pxmSceneBoundingBox->GetCorners(pxmf3SceneAABBPoints);

	XMVECTOR xmvLightSpaceSceneAABBMin = g_XMFltMax;
	XMVECTOR xmvLightSpaceSceneAABBMax = g_XMFltMin;

	XMVECTOR pxmvLightSpaceSceneAABBPoints[8];
	for (int i = 0; i < 8; i++)
	{
		XMFLOAT4 xmf4SceneAABBPoint = XMFLOAT4(pxmf3SceneAABBPoints[i].x, pxmf3SceneAABBPoints[i].y, pxmf3SceneAABBPoints[i].z, 1.0f);
		pxmvLightSpaceSceneAABBPoints[i] = XMVector4Transform(XMLoadFloat4(&xmf4SceneAABBPoint), xmmtxLightView);
		xmvLightSpaceSceneAABBMin = XMVectorMin(pxmvLightSpaceSceneAABBPoints[i], xmvLightSpaceSceneAABBMin);
		xmvLightSpaceSceneAABBMax = XMVectorMax(pxmvLightSpaceSceneAABBPoints[i], xmvLightSpaceSceneAABBMax);
	}

	float fNearPlaneDistance = XMVectorGetZ(xmvLightSpaceSceneAABBMin);
	float fFarPlaneDistance = XMVectorGetZ(xmvLightSpaceSceneAABBMax);
	xmmtxProjection = XMMatrixOrthographicOffCenterLH(XMVectorGetX(xmvLightSpaceSceneAABBMin), XMVectorGetX(xmvLightSpaceSceneAABBMax), XMVectorGetY(xmvLightSpaceSceneAABBMin), XMVectorGetY(xmvLightSpaceSceneAABBMax), fNearPlaneDistance, fFarPlaneDistance);
#endif

#ifdef _WITH_ORTHOGRAPHIC_PROJECT_CAMERA_FRUSTUM
	XMFLOAT3 pxmf3CameraFrustumPoints[8];
	BoundingFrustum xmFrustrum(XMLoadFloat4x4(&pSceneCamera->m_xmf4x4Projection));
	xmFrustrum.GetCorners(pxmf3CameraFrustumPoints);

	XMMATRIX xmmtxInverseCameraView = XMMatrixInverse(NULL, XMLoadFloat4x4(&pSceneCamera->m_xmf4x4View));

	XMVECTOR xmvLightSpaceCameraFrustumAABBMin = g_XMFltMax;
	XMVECTOR xmvLightSpaceCameraFrustumAABBMax = g_XMFltMin;
	XMVECTOR xmvLightSpaceCameraFrustumPoint;
	XMVECTOR pxmvCameraFrustumPoints[8];
	for (int i = 0; i < 8; i++)
	{
		XMFLOAT4 xmf4CameraFrustumPoint = XMFLOAT4(pxmf3CameraFrustumPoints[i].x, pxmf3CameraFrustumPoints[i].y, pxmf3CameraFrustumPoints[i].z, 1.0f);
		pxmvCameraFrustumPoints[i] = XMLoadFloat4(&xmf4CameraFrustumPoint); //Camera Space
		xmvLightSpaceCameraFrustumPoint = XMVector4Transform(XMVector4Transform(pxmvCameraFrustumPoints[i], xmmtxInverseCameraView), xmmtxLightView); //Camera Space to World space, World space to Light Space
		xmvLightSpaceCameraFrustumAABBMin = XMVectorMin(xmvLightSpaceCameraFrustumPoint, xmvLightSpaceCameraFrustumAABBMin);
		xmvLightSpaceCameraFrustumAABBMax = XMVectorMax(xmvLightSpaceCameraFrustumPoint, xmvLightSpaceCameraFrustumAABBMax);
	}

	float fNearPlaneDistance = XMVectorGetZ(xmvLightSpaceCameraFrustumAABBMin);
	float fFarPlaneDistance = XMVectorGetZ(xmvLightSpaceCameraFrustumAABBMax);
	xmmtxProjection = XMMatrixOrthographicOffCenterLH(XMVectorGetX(xmvLightSpaceCameraFrustumAABBMin), XMVectorGetX(xmvLightSpaceCameraFrustumAABBMax), XMVectorGetY(xmvLightSpaceCameraFrustumAABBMin), XMVectorGetY(xmvLightSpaceCameraFrustumAABBMax), fNearPlaneDistance, fFarPlaneDistance);
#endif

#ifdef _WITH_ORTHOGRAPHIC_PROJECT_CAMERA_FRUSTUM_SCENE
	/*Fit to Scene*/
	XMFLOAT3 pxmf3CameraFrustumPoints[8];
	BoundingFrustum xmFrustrum(XMLoadFloat4x4(&pSceneCamera->m_xmf4x4Projection));
	xmFrustrum.GetCorners(pxmf3CameraFrustumPoints);

	XMMATRIX xmmtxInverseCameraView = XMMatrixInverse(NULL, XMLoadFloat4x4(&pSceneCamera->m_xmf4x4View));

	XMVECTOR xmvLightSpaceCameraFrustumAABBMin = g_XMFltMax;
	XMVECTOR xmvLightSpaceCameraFrustumAABBMax = g_XMFltMin;
	XMVECTOR xmvLightSpaceCameraFrustumPoint;
	XMVECTOR pxmvCameraFrustumPoints[8];
	for (int i = 0; i < 8; i++)
	{
		XMFLOAT4 xmf4CameraFrustumPoint = XMFLOAT4(pxmf3CameraFrustumPoints[i].x, pxmf3CameraFrustumPoints[i].y, pxmf3CameraFrustumPoints[i].z, 1.0f);
		pxmvCameraFrustumPoints[i] = XMLoadFloat4(&xmf4CameraFrustumPoint); //Camera Space
		xmvLightSpaceCameraFrustumPoint = XMVector4Transform(XMVector4Transform(pxmvCameraFrustumPoints[i], xmmtxInverseCameraView), xmmtxLightView); //Camera Space to World space, World space to Light Space
		xmvLightSpaceCameraFrustumAABBMin = XMVectorMin(xmvLightSpaceCameraFrustumPoint, xmvLightSpaceCameraFrustumAABBMin);
		xmvLightSpaceCameraFrustumAABBMax = XMVectorMax(xmvLightSpaceCameraFrustumPoint, xmvLightSpaceCameraFrustumAABBMax);
	}

	XMVECTOR xmvLightOrthographicMin = xmvLightSpaceCameraFrustumAABBMin;
	XMVECTOR xmvLightOrthographicMax = xmvLightSpaceCameraFrustumAABBMax;

	XMVECTOR xmvDiagonal = XMVector3Length(pxmvCameraFrustumPoints[0] - pxmvCameraFrustumPoints[6]);
	float fBound = XMVectorGetX(xmvDiagonal);
	XMVECTOR xmvBoarderOffset = (xmvDiagonal - (xmvLightSpaceCameraFrustumAABBMax - xmvLightSpaceCameraFrustumAABBMin)) * g_XMOneHalf;
	XMVectorSetZ(xmvBoarderOffset, 0.0f);
	XMVectorSetW(xmvBoarderOffset, 0.0f);
	xmvLightOrthographicMax += xmvBoarderOffset;
	xmvLightOrthographicMin -= xmvBoarderOffset;

	float fLightCameraOrthographicMinZ = XMVectorGetZ(xmvLightOrthographicMin);

	///*Move Light Texel Size
	XMVECTOR xmvUnitsPerTexel = g_XMZero;
	float fUnitsPerTexel = fBound / (float)_DEPTH_BUFFER_HEIGHT;
	xmvUnitsPerTexel = XMVectorSet(fUnitsPerTexel, fUnitsPerTexel, 0.0f, 0.0f);

	xmvLightOrthographicMin /= xmvUnitsPerTexel;
	xmvLightOrthographicMin = XMVectorFloor(xmvLightOrthographicMin);
	xmvLightOrthographicMin *= xmvUnitsPerTexel;

	xmvLightOrthographicMax /= xmvUnitsPerTexel;
	xmvLightOrthographicMax = XMVectorFloor(xmvLightOrthographicMax);
	xmvLightOrthographicMax *= xmvUnitsPerTexel;
	//*/

	XMFLOAT3 pxmf3SceneAABBPoints[8];
	pxmSceneBoundingBox->GetCorners(pxmf3SceneAABBPoints);

	XMVECTOR pxmvLightSpaceSceneAABBPoints[8];
	for (int i = 0; i < 8; i++)
	{
		XMFLOAT4 xmf4SceneAABBPoint = XMFLOAT4(pxmf3SceneAABBPoints[i].x, pxmf3SceneAABBPoints[i].y, pxmf3SceneAABBPoints[i].z, 1.0f);
		pxmvLightSpaceSceneAABBPoints[i] = XMVector4Transform(XMLoadFloat4(&xmf4SceneAABBPoint), xmmtxLightView);
	}

	float fNearPlaneDistance = 0.0f;
	float fFarPlaneDistance = 10000.0f;

	ComputeOrthographicProjectionNearAndFar(fNearPlaneDistance, fFarPlaneDistance, xmvLightOrthographicMin, xmvLightOrthographicMax, pxmvLightSpaceSceneAABBPoints);
	if (fLightCameraOrthographicMinZ > fNearPlaneDistance) fNearPlaneDistance = fLightCameraOrthographicMinZ; //Pan Caking

	xmmtxProjection = XMMatrixOrthographicOffCenterLH(XMVectorGetX(xmvLightOrthographicMin), XMVectorGetX(xmvLightOrthographicMax), XMVectorGetY(xmvLightOrthographicMin), XMVectorGetY(xmvLightOrthographicMax), fNearPlaneDistance, fFarPlaneDistance);
#endif

	return(xmmtxProjection);
}

void CDepthRenderShader::PrepareShadowMap(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (!m_pScene) return;

	BoundingBox xmBoundingBox = m_pScene->CalculateBoundingBox();
	auto pLights = m_pScene->GetLights();

	for (int j = 0; j < MAX_LIGHTS; j++)
	{
		if (pLights[j].m_bEnable)
		{
			XMFLOAT3 xmf3Position = pLights[j].m_xmf3Position;
			XMFLOAT3 xmf3Look = pLights[j].m_xmf3Direction;
			XMFLOAT3 xmf3Up = XMFLOAT3(0.0f, +1.0f, 0.0f);

			XMMATRIX xmmtxLightView = XMMatrixLookToLH(XMLoadFloat3(&xmf3Position), XMLoadFloat3(&xmf3Look), XMLoadFloat3(&xmf3Up));

			XMMATRIX xmmtxProjection = XMMatrixIdentity();
			if (pLights[j].m_nType == DIRECTIONAL_LIGHT)
			{
				xmmtxProjection = CreateOrthographicProjectionMatrix(xmmtxLightView, pCamera, &xmBoundingBox);
			}
			else if (pLights[j].m_nType == SPOT_LIGHT)
			{
				float fFovAngle = 60.0f; // pLights->pLights[j].m_fPhi = cos(60.0f);
				float fAspectRatio = float(m_nDepthbufferWidth) / float(m_nDepthbufferHeight);
				float fNearPlaneDistance = 10.0f, fFarPlaneDistance = pLights[j].m_fRange;

				xmmtxProjection = XMMatrixPerspectiveFovLH(XMConvertToRadians(fFovAngle), fAspectRatio, fNearPlaneDistance, fFarPlaneDistance);
			}
			else if (pLights[j].m_nType == POINT_LIGHT)
			{
				//ShadowMap[6]
			}

			m_ppDepthRenderCameras[j]->SetPosition(xmf3Position);
			XMStoreFloat4x4(&m_ppDepthRenderCameras[j]->m_xmf4x4View, xmmtxLightView);
			XMStoreFloat4x4(&m_ppDepthRenderCameras[j]->m_xmf4x4Projection, xmmtxProjection);

			XMMATRIX xmmtxToTexture = XMMatrixTranspose(xmmtxLightView * xmmtxProjection * m_xmProjectionToTexture);
			XMStoreFloat4x4(&m_pToLightSpaces[0].m_pxmf4x4ToTextures[j], xmmtxToTexture);
			m_pToLightSpaces[0].m_pxmf4LightPositions[j] = XMFLOAT4(xmf3Position.x, xmf3Position.y, xmf3Position.z, 1.0f);

			::SynchronizeResourceTransition(pd3dCommandList, m_pDepthFromLightTexture->GetResource(j), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

			float pfClearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			pd3dCommandList->ClearRenderTargetView(m_pd3dRtvCPUDescriptorHandles[j], pfClearColor, 0, NULL);
			pd3dCommandList->ClearDepthStencilView(m_d3dDsvDescriptorCPUHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
			pd3dCommandList->OMSetRenderTargets(1, &m_pd3dRtvCPUDescriptorHandles[j], TRUE, &m_d3dDsvDescriptorCPUHandle);

			Render(pd3dCommandList, m_ppDepthRenderCameras[j].get());

			::SynchronizeResourceTransition(pd3dCommandList, m_pDepthFromLightTexture->GetResource(j), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
		}
		else
		{
			m_pToLightSpaces[0].m_pxmf4LightPositions[j].w = 0.0f;
		}
	}
}

void CDepthRenderShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	CShader::OnPrepareRender(pd3dCommandList);

	pCamera->SetViewportsAndScissorRects(pd3dCommandList);
	pCamera->UpdateShaderVariables(pd3dCommandList);

	if (!m_pScene) return;

	m_pScene->RenderWorldObject(pd3dCommandList, pCamera);
}

std::shared_ptr<CTexture> CDepthRenderShader::GetDepthTexture()
{ 
	return m_pDepthFromLightTexture;
}

ID3D12Resource* CDepthRenderShader::GetDepthTextureResource(UINT nIndex)
{
	return(m_pDepthFromLightTexture->GetResource(nIndex)); 
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

CShadowMapShader::CShadowMapShader(CScene* pScene)
	: CIlluminatedShader(), m_pScene(pScene)
{
}

CShadowMapShader::~CShadowMapShader()
{
}

D3D12_DEPTH_STENCIL_DESC CShadowMapShader::CreateDepthStencilState(int nPipelineState)
{
	D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
	::ZeroMemory(&d3dDepthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	d3dDepthStencilDesc.DepthEnable = TRUE;
	d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	d3dDepthStencilDesc.StencilEnable = FALSE;
	d3dDepthStencilDesc.StencilReadMask = 0x00;
	d3dDepthStencilDesc.StencilWriteMask = 0x00;
	d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return(d3dDepthStencilDesc);
}

D3D12_SHADER_BYTECODE CShadowMapShader::CreateVertexShader(int nPipelineState)
{
#ifdef _COMPILE_SHADER
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "VSShadowMapShadow", "vs_5_1", m_pd3dVertexShaderBlob.GetAddressOf()));
#else
	return(CShader::ReadCompiledShaderFromFile(L"VSShadowMapShadow", m_pd3dVertexShaderBlob.GetAddressOf()));
#endif
}

D3D12_SHADER_BYTECODE CShadowMapShader::CreatePixelShader(int nPipelineState)
{
#ifdef _COMPILE_SHADER
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PSShadowMapShadow", "ps_5_1", m_pd3dPixelShaderBlob.GetAddressOf()));
#else
	return(CShader::ReadCompiledShaderFromFile(L"PSShadowMapShadow", m_pd3dPixelShaderBlob.GetAddressOf()));
#endif
}

void CShadowMapShader::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
}

void CShadowMapShader::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_pDepthFromLightTexture) m_pDepthFromLightTexture->UpdateShaderVariables(pd3dCommandList);
}

void CShadowMapShader::ReleaseShaderVariables()
{
}

void CShadowMapShader::ReleaseUploadBuffers()
{
}

void CShadowMapShader::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::shared_ptr<CTexture> pContext)
{
	m_pDepthFromLightTexture = (std::shared_ptr<CTexture>) pContext;

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CShadowMapShader::ReleaseObjects()
{
	if (m_pDepthFromLightTexture) m_pDepthFromLightTexture.reset();
}

void CShadowMapShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (!m_pScene) return;
	CShader::Render(pd3dCommandList, pCamera);

	UpdateShaderVariables(pd3dCommandList);

	m_pScene->RenderWorldObject(pd3dCommandList, pCamera);
	//m_pObjectsShader->m_pDirectionalLight->UpdateShaderVariables(pd3dCommandList);
	//m_pObjectsShader->m_pDirectionalLight->Render(pd3dCommandList, pCamera);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

CTextureToViewportShader::CTextureToViewportShader()
{
}

CTextureToViewportShader::~CTextureToViewportShader()
{
}

D3D12_DEPTH_STENCIL_DESC CTextureToViewportShader::CreateDepthStencilState(int nPipelineState)
{
	D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
	::ZeroMemory(&d3dDepthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	d3dDepthStencilDesc.DepthEnable = FALSE;
	d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	d3dDepthStencilDesc.StencilEnable = FALSE;
	d3dDepthStencilDesc.StencilReadMask = 0x00;
	d3dDepthStencilDesc.StencilWriteMask = 0x00;
	d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return(d3dDepthStencilDesc);
}

D3D12_SHADER_BYTECODE CTextureToViewportShader::CreateVertexShader(int nPipelineState)
{
#ifdef _COMPILE_SHADER
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "VSTextureToViewport", "vs_5_1", m_pd3dVertexShaderBlob.GetAddressOf()));
#else
	return(CShader::ReadCompiledShaderFromFile(L"VSTextureToViewport", m_pd3dVertexShaderBlob.GetAddressOf()));
#endif
}

D3D12_SHADER_BYTECODE CTextureToViewportShader::CreatePixelShader(int nPipelineState)
{
#ifdef _COMPILE_SHADER
	return(CShader::CompileShaderFromFile(L"Shaders.hlsl", "PSTextureToViewport", "ps_5_1", m_pd3dPixelShaderBlob.GetAddressOf()));
#else
	return(CShader::ReadCompiledShaderFromFile(L"PSTextureToViewport", m_pd3dPixelShaderBlob.GetAddressOf()));
#endif
}

void CTextureToViewportShader::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_pDepthFromLightTexture) m_pDepthFromLightTexture->UpdateShaderVariables(pd3dCommandList);
}

void CTextureToViewportShader::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::shared_ptr<CTexture> pContext)
{
	m_pDepthFromLightTexture = (std::shared_ptr<CTexture>) pContext;

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CTextureToViewportShader::ReleaseObjects()
{
	m_pDepthFromLightTexture.reset();
}

void CTextureToViewportShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	D3D12_VIEWPORT d3dViewport = { 0.0f, 0.0f, WINDOW_WIDTH * 0.25f, WINDOW_HEIGHT * 0.25f, 0.0f, 1.0f };
	D3D12_RECT d3dScissorRect = { 0, 0, WINDOW_WIDTH / 4, WINDOW_HEIGHT / 4 };
	pd3dCommandList->RSSetViewports(1, &d3dViewport);
	pd3dCommandList->RSSetScissorRects(1, &d3dScissorRect);

	CShader::Render(pd3dCommandList, pCamera);

	UpdateShaderVariables(pd3dCommandList);

	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pd3dCommandList->DrawInstanced(6, 1, 0, 0);
}


