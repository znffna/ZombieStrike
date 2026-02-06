#pragma once
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

#include "stdafx.h"

#include "TextComponent.h"

class UILayer
{
public:
	UILayer(UINT nFrames, ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Resource** ppd3dRenderTargets, UINT nWidth, UINT nHeight);
	~UILayer() { ReleaseResources(); }

	void Render(UINT nFrame, const std::vector<TextBlock*>& vecTextObjects);

public:
	void InitializeDevice(ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Resource** ppd3dRenderTargets);
	void ReleaseResources();

	float                           m_fWidth = 0.0f;
	float                           m_fHeight = 0.0f;

	ComPtr<ID3D11DeviceContext> m_pd3d11DeviceContext;
	ComPtr<ID3D11On12Device> m_pd3d11On12Device;
	ComPtr<IDWriteFactory> m_pd2dWriteFactory;
	ComPtr<ID2D1Factory3> m_pd2dFactory;
	ComPtr<ID2D1Device2> m_pd2dDevice;
	ComPtr<ID2D1DeviceContext2> m_pd2dDeviceContext;

	UINT             m_nRenderTargets = 0;
	ID3D11Resource** m_ppd3d11WrappedRenderTargets = NULL;
	ID2D1Bitmap1** m_ppd2dRenderTargets = NULL;

private:

	// Caching Brush & TextFormat 
	std::unordered_map<uint32_t, ComPtr<ID2D1SolidColorBrush>> m_pBrushes;
	std::map<std::pair<std::wstring, float>, ComPtr<IDWriteTextFormat>> m_pTextFormats;

	void ClearCache() {
		m_pBrushes.clear();
		m_pTextFormats.clear();
	}

	// Create & Get Brush & TextFormat
	ComPtr<ID2D1SolidColorBrush> CreateBrush(D2D1::ColorF d2dColor);
	ComPtr<IDWriteTextFormat> CreateTextFormat(WCHAR* pszFontName, float fFontSize);

	ComPtr<ID2D1SolidColorBrush> GetBrush(D2D1::ColorF d2dColor);
	ComPtr<IDWriteTextFormat> GetTextFormat(WCHAR* pszFontName, float fFontSize);
};



