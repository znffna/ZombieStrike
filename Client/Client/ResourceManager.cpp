#include "ResourceManager.h"


inline CResource& CResource::Use() {
	isUsed = true;
	return *this;
}

// 리소스 해제
void CResource::Release() {
	if (resource) {
		resource->Unmap(0, NULL);
		isUsed = false;
		CResourceManager::GetInstance().ReleaseSkinningBoneTransform(*this);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

void CResourceManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ID3D12RootSignature* rootsignature) {
	m_d3dDevice = device;

	m_d3dGraphicsCommandList = commandList;
	m_d3dGraphicRootSignature = rootsignature;

	CreateSkinnedTransformBuffer();
	LoadModelList();
}



// 모든 리소스 해제
void CResourceManager::ReleaseResources() {
	ModelInfos.clear();
	TextureInfos.clear();
	MeshInfos.clear();
	for (auto& resource : m_ppd3dcbSkinningBoneTransforms) {
		resource.Release();
		resource.resource.Reset();
	}
}

////////////////////////////////////////////
// 텍스쳐 정보를 저장
void CResourceManager::SetTexture(const std::string& name, std::shared_ptr<CTexture> texture) {
	if (texture == nullptr) return;
	TextureInfos[name] = texture;
}

std::shared_ptr<CTexture> CResourceManager::GetTexture(const std::string& name) {
	if (TextureInfos.find(name) != TextureInfos.end()) {
		// 이미 로드된 모델이 있는 경우
		return TextureInfos[name];
	}
	return nullptr;
}

void CResourceManager::LoadModelList(std::string filepath) {
	std::ifstream file(filepath);
	std::string modelname;
	while (file >> modelname) {
		GetModelInfo(modelname);
	}
}

void CResourceManager::SetSkinInfo(const std::string& name, std::shared_ptr<CLoadedModelInfo> modelInfo) {
	ModelInfos[name] = modelInfo;
}

inline std::shared_ptr<CLoadedModelInfo> CResourceManager::GetModelInfo(const std::string& name) {
	if (ModelInfos.find(name) != ModelInfos.end()) {
		// 이미 로드된 모델이 있는 경우
		if (ModelInfos[name]) return ModelInfos[name];
		else return nullptr;
	}

	ModelInfos[name] = nullptr;

	// 없는경우 바로 불러와서 저장하고 return 한다.
	std::string filepath = "Model/" + name + ".bin";
	/*OutputDebugStringA(filepath.c_str());
	OutputDebugStringA("\n");*/

	auto pModelInfo = CGameObject::LoadGeometryAndAnimationFromFile(m_d3dDevice, m_d3dGraphicsCommandList, m_d3dGraphicRootSignature, filepath.c_str(), nullptr);
	if (pModelInfo) {
		SetSkinInfo(name, pModelInfo);
		return pModelInfo;
	}
	else {
		// 로드 실패
		/*	std::string strDebug = "Failed to load model: " + name;
		OutputDebugStringA(strDebug.c_str());
		*/
	}
	return nullptr;
}

// 메쉬 정보를 저장
void CResourceManager::SetMesh(const std::string& name, std::shared_ptr<CMesh> pMesh) {
	MeshInfos[name] = pMesh;
}

std::shared_ptr<CMesh> CResourceManager::GetMesh(const std::string& name) {
	if (MeshInfos.find(name) != MeshInfos.end()) {
		return MeshInfos[name];
	}
	return nullptr;
}

void CResourceManager::CreateSkinnedTransformBuffer() {
	m_ppd3dcbSkinningBoneTransforms.resize(SKINNED_TRANSFORM_GPU_BUFFER);
	UINT ncbElementBytes = (((sizeof(XMFLOAT4X4) * SKINNED_ANIMATION_BONES) + 255) & ~255); //256의 배수

	for (int i = 0; i < SKINNED_TRANSFORM_GPU_BUFFER; i++)
	{
		m_ppd3dcbSkinningBoneTransforms[i].index = i;
		m_ppd3dcbSkinningBoneTransforms[i].isUsed = false;
		m_ppd3dcbSkinningBoneTransforms[i].resource = ::CreateBufferResource(m_d3dDevice, m_d3dGraphicsCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
	}
}

CResource& CResourceManager::GetSkinningBoneTransforms() {
	for (int i = 0; i < m_ppd3dcbSkinningBoneTransforms.size(); ++i) {
		if (false == m_ppd3dcbSkinningBoneTransforms[i].isUsed) {
			
			m_ppd3dcbSkinningBoneTransforms[i].isUsed = true;
			++m_nSkinningBoneTransformsCount;
			/*{
				std::string debugName = "GetSkinningBoneTransforms() - Skinning Bone Transforms [" + std::to_string(i) + "] is return, now SKinningBoneTransforms Uses : " + std::to_string(m_nSkinningBoneTransformsCount) + "\n";
				OutputDebugStringA(debugName.c_str());
			}*/
			return m_ppd3dcbSkinningBoneTransforms[i];
		}
	}

	// 이 아래 코드가 실행되면, 리소스가 부족한 경우이다.
	// 리소스가 부족한경우 아래코드는 Resource Container를 조작하기에 기존 &로 수행되는 코드가 터지게 된다.
	// 따라서, 아래 코드가 진행될 경우, SKINNED_TRANSFORM_GPU_BUFFER 을 높여서 다시 실행하자.
	{
		CResource resource;
		resource.isUsed = true;
		resource.index = m_ppd3dcbSkinningBoneTransforms.size();
		UINT ncbElementBytes = (((sizeof(XMFLOAT4X4) * SKINNED_ANIMATION_BONES) + 255) & ~255); //256의 배수
		resource.resource = ::CreateBufferResource(m_d3dDevice, m_d3dGraphicsCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
		m_ppd3dcbSkinningBoneTransforms.push_back(resource);
		return m_ppd3dcbSkinningBoneTransforms.back();
	}
}

void CResourceManager::ReleaseSkinningBoneTransform(const CResource& cResource) {
	// count를 위해 만든 Method.
	--m_nSkinningBoneTransformsCount;
	/*{
		std::string debugName = "ReleaseSkinningBoneTransform() - Skinning Bone Transforms [" + std::to_string(cResource.index) + "] is return, now SKinningBoneTransforms Uses : " + std::to_string(m_nSkinningBoneTransformsCount) + "\n";
		OutputDebugStringA(debugName.c_str());
	}*/
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

UILayer::UILayer(UINT nFrames, UINT nTextBlocks, ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Resource** ppd3dRenderTargets, UINT nWidth, UINT nHeight)
{
	m_fWidth = static_cast<float>(nWidth);
	m_fHeight = static_cast<float>(nHeight);
	m_nRenderTargets = nFrames;
	m_ppd3d11WrappedRenderTargets = new ID3D11Resource * [nFrames];
	m_ppd2dRenderTargets = new ID2D1Bitmap1 * [nFrames];

	m_pTextBlocks.reserve(nTextBlocks);
	m_pTextPools.resize(nTextBlocks);

	InitializeDevice(pd3dDevice, pd3dCommandQueue, ppd3dRenderTargets);
}

void UILayer::InitializeDevice(ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Resource** ppd3dRenderTargets)
{
	UINT d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	D2D1_FACTORY_OPTIONS d2dFactoryOptions = { };

#if defined(_DEBUG) || defined(DBG)
	d2dFactoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
	d3d11DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	ID3D11Device* pd3d11Device = NULL;
	ID3D12CommandQueue* ppd3dCommandQueues[] = { pd3dCommandQueue };
	::D3D11On12CreateDevice(pd3dDevice, d3d11DeviceFlags, nullptr, 0, reinterpret_cast<IUnknown**>(ppd3dCommandQueues), _countof(ppd3dCommandQueues), 0, (ID3D11Device**)&pd3d11Device, (ID3D11DeviceContext**)&m_pd3d11DeviceContext, nullptr);

	pd3d11Device->QueryInterface(__uuidof(ID3D11On12Device), (void**)&m_pd3d11On12Device);
	pd3d11Device->Release();

#if defined(_DEBUG) || defined(DBG)
	ID3D12InfoQueue* pd3dInfoQueue;
	if (SUCCEEDED(pd3dDevice->QueryInterface(IID_PPV_ARGS(&pd3dInfoQueue))))
	{
		D3D12_MESSAGE_SEVERITY pd3dSeverities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_MESSAGE_ID pd3dDenyIds[] = { D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE };

		D3D12_INFO_QUEUE_FILTER d3dInforQueueFilter = { };
		d3dInforQueueFilter.DenyList.NumSeverities = _countof(pd3dSeverities);
		d3dInforQueueFilter.DenyList.pSeverityList = pd3dSeverities;
		d3dInforQueueFilter.DenyList.NumIDs = _countof(pd3dDenyIds);
		d3dInforQueueFilter.DenyList.pIDList = pd3dDenyIds;

		pd3dInfoQueue->PushStorageFilter(&d3dInforQueueFilter);
	}
	pd3dInfoQueue->Release();
#endif

	IDXGIDevice* pdxgiDevice = NULL;
	m_pd3d11On12Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&pdxgiDevice);

	::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &d2dFactoryOptions, (void**)&m_pd2dFactory);
	HRESULT hResult = m_pd2dFactory->CreateDevice(pdxgiDevice, (ID2D1Device2**)m_pd2dDevice.GetAddressOf());
	m_pd2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, (ID2D1DeviceContext2**)&m_pd2dDeviceContext);

	m_pd2dDeviceContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

	::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&m_pd2dWriteFactory);
	pdxgiDevice->Release();

	D2D1_BITMAP_PROPERTIES1 d2dBitmapProperties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));

	for (UINT i = 0; i < m_nRenderTargets; i++)
	{
		D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
		m_pd3d11On12Device->CreateWrappedResource(ppd3dRenderTargets[i], &d3d11Flags, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT, IID_PPV_ARGS(&m_ppd3d11WrappedRenderTargets[i]));
		IDXGISurface* pdxgiSurface = NULL;
		m_ppd3d11WrappedRenderTargets[i]->QueryInterface(__uuidof(IDXGISurface), (void**)&pdxgiSurface);
		m_pd2dDeviceContext->CreateBitmapFromDxgiSurface(pdxgiSurface, &d2dBitmapProperties, &m_ppd2dRenderTargets[i]);
		pdxgiSurface->Release();
	}
}

ComPtr<ID2D1SolidColorBrush> UILayer::CreateBrush(D2D1::ColorF d2dColor)
{
	ComPtr<ID2D1SolidColorBrush> pd2dDefaultTextBrush;
	m_pd2dDeviceContext->CreateSolidColorBrush(d2dColor, pd2dDefaultTextBrush.GetAddressOf());

	return(pd2dDefaultTextBrush);
}

ComPtr<IDWriteTextFormat> UILayer::CreateTextFormat(WCHAR* pszFontName, float fFontSize)
{
	ComPtr<IDWriteTextFormat> pdwDefaultTextFormat;
	m_pd2dWriteFactory->CreateTextFormat(L"궁서체", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fFontSize, L"en-us", pdwDefaultTextFormat.GetAddressOf());

	pdwDefaultTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	pdwDefaultTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	//m_pd2dWriteFactory->CreateTextFormat(L"Arial", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fSmallFontSize, L"en-us", &m_pdwDefaultTextFormat);

	return(pdwDefaultTextFormat);
}

void UILayer::StorePoolTextBlock(UINT nIndex, std::wstring* pstrUIText, D2D1_RECT_F* pd2dLayoutRect, ComPtr<IDWriteTextFormat> pdwFormat, ComPtr<ID2D1SolidColorBrush> pd2dTextBrush)
{
	m_pTextPools[nIndex] = std::make_shared<TextBlock>();
	if (pstrUIText) m_pTextPools[nIndex]->m_pstrText = *pstrUIText;
	if (pd2dLayoutRect) m_pTextPools[nIndex]->m_d2dLayoutRect = *pd2dLayoutRect;
	if (pdwFormat) m_pTextPools[nIndex]->m_pdwFormat = pdwFormat;
	if (pd2dTextBrush) m_pTextPools[nIndex]->m_pd2dTextBrush = pd2dTextBrush;
}

void UILayer::UpdateTextOutputs(UINT nIndex, std::wstring* pstrUIText, D2D1_RECT_F* pd2dLayoutRect, ComPtr<IDWriteTextFormat> pdwFormat, ComPtr<ID2D1SolidColorBrush> pd2dTextBrush)
{
	if (pstrUIText) m_pTextPools[nIndex]->m_pstrText = *pstrUIText;
	if (pd2dLayoutRect) m_pTextBlocks[nIndex]->m_d2dLayoutRect = *pd2dLayoutRect;
	if (pdwFormat) m_pTextBlocks[nIndex]->m_pdwFormat = pdwFormat;
	if (pd2dTextBrush) m_pTextBlocks[nIndex]->m_pd2dTextBrush = pd2dTextBrush;
}

void UILayer::Render(UINT nFrame)
{
	if(m_pTextBlocks.size() > 0)
	{
		ID3D11Resource* ppResources[] = { m_ppd3d11WrappedRenderTargets[nFrame] };

		m_pd2dDeviceContext->SetTarget(m_ppd2dRenderTargets[nFrame]);
		m_pd3d11On12Device->AcquireWrappedResources(ppResources, _countof(ppResources));

		m_pd2dDeviceContext->BeginDraw();
		for (UINT i = 0; i < m_pTextBlocks.size(); i++)
		{
			if(m_pTextBlocks[i]->m_bActive) m_pd2dDeviceContext->DrawText(m_pTextBlocks[i]->m_pstrText.c_str(), (UINT)m_pTextBlocks[i]->m_pstrText.length(), m_pTextBlocks[i]->m_pdwFormat.Get(), m_pTextBlocks[i]->m_d2dLayoutRect, m_pTextBlocks[i]->m_pd2dTextBrush.Get());
		}
		m_pd2dDeviceContext->EndDraw();

		m_pd3d11On12Device->ReleaseWrappedResources(ppResources, _countof(ppResources));
		m_pd3d11DeviceContext->Flush();
	}
}

void UILayer::ReleaseResources()
{
	for (UINT i = 0; i < m_pTextBlocks.size(); i++)
	{
		m_pTextBlocks[i]->m_pdwFormat.Reset();
		m_pTextBlocks[i]->m_pd2dTextBrush.Reset();
	}

	for (UINT i = 0; i < m_nRenderTargets; i++)
	{
		ID3D11Resource* ppResources[] = { m_ppd3d11WrappedRenderTargets[i] };
		m_pd3d11On12Device->ReleaseWrappedResources(ppResources, _countof(ppResources));
	}

	m_pd2dDeviceContext->SetTarget(nullptr);
	m_pd3d11DeviceContext->Flush();

	for (UINT i = 0; i < m_nRenderTargets; i++)
	{
		m_ppd2dRenderTargets[i]->Release();
		m_ppd3d11WrappedRenderTargets[i]->Release();
	}

	m_pd2dDeviceContext.Reset();
	m_pd2dWriteFactory.Reset();
	m_pd2dDevice.Reset();
	m_pd2dFactory.Reset();
	m_pd3d11DeviceContext.Reset();
	m_pd3d11On12Device.Reset();
}

std::shared_ptr<TextBlock>  UILayer::GetNewTextBlock(int nPoolIndex) {
	if (m_pTextPools.size() > nPoolIndex) {
		if (m_pTextPools[nPoolIndex]->m_pdwFormat && m_pTextPools[nPoolIndex]->m_pd2dTextBrush) {
			auto pBlock = std::make_shared<TextBlock>();
			pBlock->m_pstrText = m_pTextPools[nPoolIndex]->m_pstrText;
			pBlock->m_d2dLayoutRect = m_pTextPools[nPoolIndex]->m_d2dLayoutRect;
			pBlock->m_pdwFormat = m_pTextPools[nPoolIndex]->m_pdwFormat;
			pBlock->m_pd2dTextBrush = m_pTextPools[nPoolIndex]->m_pd2dTextBrush;
			m_pTextBlocks.push_back(pBlock);
			return pBlock;
		}
	}
	else {
		auto pBlock = std::make_shared<TextBlock>();
		pBlock->m_pstrText = m_pTextPools[0]->m_pstrText;
		pBlock->m_d2dLayoutRect = m_pTextPools[0]->m_d2dLayoutRect;
		pBlock->m_pdwFormat = m_pTextPools[0]->m_pdwFormat;
		pBlock->m_pd2dTextBrush = m_pTextPools[0]->m_pd2dTextBrush;
		m_pTextBlocks.push_back(pBlock);
		return pBlock;
	}
	return nullptr;
}