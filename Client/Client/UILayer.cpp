#include "UILayer.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

UILayer::UILayer(UINT nFrames, ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Resource** ppd3dRenderTargets, UINT nWidth, UINT nHeight)
{
	m_fWidth = static_cast<float>(nWidth);
	m_fHeight = static_cast<float>(nHeight);
	m_nRenderTargets = nFrames;
	m_ppd3d11WrappedRenderTargets = new ID3D11Resource * [nFrames];
	m_ppd2dRenderTargets = new ID2D1Bitmap1 * [nFrames];

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

uint8_t q8(float x) {
	// [0,1] clamp 후 0..255로 반올림
	x = std::max(0.0f, std::min(1.0f, x));
	return static_cast<uint8_t>(x * 255.0f + 0.5f);
}

// ARGB(8:8:8:8)로 패킹 (원하는 순서로 바꿔도 무방)
static inline uint32_t PackRGBA8(const D2D1_COLOR_F& c) {
	uint32_t r = q8(c.r);
	uint32_t g = q8(c.g);
	uint32_t b = q8(c.b);
	uint32_t a = q8(c.a);
	return (a << 24) | (r << 16) | (g << 8) | (b << 0);
}

ComPtr<ID2D1SolidColorBrush> UILayer::GetBrush(D2D1::ColorF d2dColor)
{
	auto colorKey = PackRGBA8(d2dColor);
	auto it = m_pBrushes.find(colorKey);
	if (it != m_pBrushes.end()) {
		return it->second;
	}
	else {
		return CreateBrush(d2dColor);
	}
}

ComPtr<IDWriteTextFormat> UILayer::GetTextFormat(WCHAR* pszFontName, float fFontSize)
{
	auto it = m_pTextFormats.find({ std::wstring(pszFontName), fFontSize });
	if (it != m_pTextFormats.end()) {
		return it->second;
	}
	else {
		return CreateTextFormat(pszFontName, fFontSize);
	}
}

ComPtr<ID2D1SolidColorBrush> UILayer::CreateBrush(D2D1::ColorF d2dColor)
{
	ComPtr<ID2D1SolidColorBrush> pd2dDefaultTextBrush;
	m_pd2dDeviceContext->CreateSolidColorBrush(d2dColor, pd2dDefaultTextBrush.GetAddressOf());

	auto colorKey = PackRGBA8(d2dColor);
	m_pBrushes[colorKey] = pd2dDefaultTextBrush;

	return(pd2dDefaultTextBrush);
}

ComPtr<IDWriteTextFormat> UILayer::CreateTextFormat(WCHAR* pszFontName, float fFontSize)
{
	ComPtr<IDWriteTextFormat> pdwDefaultTextFormat;
	m_pd2dWriteFactory->CreateTextFormat(L"궁서체", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fFontSize, L"en-us", pdwDefaultTextFormat.GetAddressOf());

	pdwDefaultTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	pdwDefaultTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	//m_pd2dWriteFactory->CreateTextFormat(L"Arial", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fSmallFontSize, L"en-us", &m_pdwDefaultTextFormat);

	//m_pTextFormats[std::make_pair(pszFontName, fFontSize)] = pdwDefaultTextFormat;

	m_pTextFormats[{ std::wstring(pszFontName), fFontSize }] = pdwDefaultTextFormat;

	return(pdwDefaultTextFormat);
}

void UILayer::Render(UINT nFrame, const std::vector<TextBlock*>& vecTextBlocks)
{
	ID3D11Resource* ppResources[] = { m_ppd3d11WrappedRenderTargets[nFrame] };

	m_pd2dDeviceContext->SetTarget(m_ppd2dRenderTargets[nFrame]);
	m_pd3d11On12Device->AcquireWrappedResources(ppResources, _countof(ppResources));

	m_pd2dDeviceContext->BeginDraw();
	// vecTextBlocks 을 전부 Draw
	for (auto& pTextBlock : vecTextBlocks)
	{
		{
			m_pd2dDeviceContext->DrawText(
				pTextBlock->m_pstrText.c_str(),
				(UINT)pTextBlock->m_pstrText.length(),
				GetTextFormat(pTextBlock->GetFont().data(),
					pTextBlock->GetFontSize()).Get(),
				pTextBlock->m_d2dLayoutRect,
				GetBrush(pTextBlock->GetBrush()).Get()
			);
		}
	}
	m_pd2dDeviceContext->EndDraw();

	m_pd2dDeviceContext->SetTarget(nullptr);
	m_pd3d11On12Device->ReleaseWrappedResources(ppResources, _countof(ppResources));
	m_pd3d11DeviceContext->Flush();
}

void UILayer::ReleaseResources()
{
	ClearCache();

	if (m_ppd3d11WrappedRenderTargets == NULL || m_ppd2dRenderTargets == NULL)
	{
		return; // 이미 해제된 경우
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