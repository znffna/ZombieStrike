#pragma once

#include "Component.h"

class TextBlock
{
public:
	bool						    m_bActive = true;
	std::wstring                    m_pstrText = L"Text";
	D2D1_RECT_F                     m_d2dLayoutRect = {0,0, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT };
	//ComPtr<IDWriteTextFormat> m_pdwFormat;
	//ComPtr<ID2D1SolidColorBrush> m_pd2dTextBrush;
	std::wstring					m_strFontKey = L"Consolas";
	float							m_fFontSize = 12.0f;
	D2D1::ColorF					m_cBrushKey{ 0,0,0 };

	// Setters
	void SetActive(bool bActive) {
		m_bActive = bActive;
	}

	void SetText(std::wstring pstrUIText) {
		m_pstrText = pstrUIText;
	}

	void SetSize(float x, float y, float width, float height, bool isCenter = true) {
		if (isCenter) {
			x = x - width / 2;
			y = y - height / 2;
		}
		m_d2dLayoutRect.left = x;
		m_d2dLayoutRect.top = y;
		m_d2dLayoutRect.right = x + width;
		m_d2dLayoutRect.bottom = y + height;
	}


	void SetFont(const std::wstring& strFontKey) {
		m_strFontKey = strFontKey;
	}

	void SetFontSize(float fFontSize) {
		m_fFontSize = fFontSize;
	}

	void SetBrush(const D2D1::ColorF& cBrushKey) {
		m_cBrushKey = cBrushKey;
	}

	void SetColor(const D2D1::ColorF& cBrushKey) {
		SetBrush(cBrushKey);
	}

	// Getters
	bool IsActive() const {
		return m_bActive;
	}
	std::wstring GetText() const {
		return m_pstrText;
	}
	D2D1_RECT_F GetSize() const {
		return m_d2dLayoutRect;
	}
	std::wstring GetFont() const {
		return m_strFontKey;
	}
	float GetFontSize() const {
		return m_fFontSize;
	}

	D2D1::ColorF GetBrush() const {
		return m_cBrushKey;
	}
};

class CTextComponent : public CComponent
{
public:
	CTextComponent(CGameObject* pOwner) : CComponent(pOwner) { Initialize(); }
	CTextComponent(const CTextComponent& rhs) : CComponent(rhs), m_TextBlock(rhs.m_TextBlock) {}
	virtual ~CTextComponent() { OnDestroy(); }

	void Initialize();
	virtual void OnDestroy();;


	virtual std::unique_ptr<CComponent> Clone(CGameObject* newOwner) const { auto ret = std::make_unique<CTextComponent>(*this); ret->SetOwnerInternal(newOwner); return (ret); };

public:
	TextBlock* GetTextBlock() { return &m_TextBlock; };

	// TextBlock Setters
	void SetActive(bool bActive) { m_TextBlock.SetActive(bActive); }
	void SetText(std::wstring pstrUIText) { m_TextBlock.SetText(pstrUIText); }
	void SetSize(float x, float y, float width, float height, bool isCenter = true) { m_TextBlock.SetSize(x, y, width, height, isCenter); }
	void SetFont(const std::wstring& strFontKey) { m_TextBlock.SetFont(strFontKey); }
	void SetFontSize(float fFontSize) { m_TextBlock.SetFontSize(fFontSize); }
	void SetBrush(const D2D1::ColorF& cBrushKey) { m_TextBlock.SetBrush(cBrushKey); }
	void SetColor(const D2D1::ColorF& cBrushKey) { m_TextBlock.SetColor(cBrushKey); }

	// TextBlock Getters
	bool IsActive() const { return m_TextBlock.IsActive(); }
	std::wstring GetText() const { return m_TextBlock.GetText(); }
	D2D1_RECT_F GetSize() const { return m_TextBlock.GetSize(); }
	std::wstring GetFont() const { return m_TextBlock.GetFont(); }
	float GetFontSize() const { return m_TextBlock.GetFontSize(); }
	D2D1::ColorF GetBrush() const { return m_TextBlock.GetBrush(); }

private:
	TextBlock m_TextBlock;
};


