#if !defined(AFX_STATICEX_H__A4EABEC5_2E8C_11D1_B79F_00805F9ECE10__INCLUDED_)
#define AFX_STATICEX_H__A4EABEC5_2E8C_11D1_B79F_00805F9ECE10__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// StaticEx.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStaticEx window

class CStaticEx : public CStatic
{
// Construction
public:
	typedef enum  { 
        LinkNone,
        HyperLink,
        MailLink 
    } LinkStyle;

	typedef enum  {
        None,
        Text,
        Background 
    } FlashType;

	typedef enum  { 
        Raised,
        Sunken
    } Type3D;
	
    typedef enum {
        Normal,
        Gradient
    } BackFillMode;

	CStaticEx();
	virtual CStaticEx& SetBkColor(COLORREF crBkgnd, COLORREF crBkgndHigh = 0, BackFillMode mode = Normal);
	virtual CStaticEx& SetTextColor(COLORREF crText);
	virtual CStaticEx& SetText(const CString& strText);
	virtual CStaticEx& SetFontBold(BOOL bBold);
	virtual CStaticEx& SetFontName(const CString& strFont, BYTE byCharSet = ANSI_CHARSET);
	virtual CStaticEx& SetFontUnderline(BOOL bSet);
	virtual CStaticEx& SetFontItalic(BOOL bSet);
	virtual CStaticEx& SetFontSize(int nSize);
	virtual CStaticEx& SetSunken(BOOL bSet);
	virtual CStaticEx& SetBorder(BOOL bSet);
	virtual CStaticEx& SetTransparent(BOOL bSet);
	virtual CStaticEx& FlashText(BOOL bActivate);
	virtual CStaticEx& FlashBackground(BOOL bActivate);
    virtual CStaticEx& SetLink(BOOL bLink, LinkStyle style, HWND hWndNotify, UINT nMsgNotify);
	virtual CStaticEx& SetFont3D(BOOL bSet,Type3D type=Raised);
	virtual CStaticEx& SetRotationAngle(UINT nAngle,BOOL bRotation);
	virtual CStaticEx& SetText3DHiliteColor(COLORREF cr3DHiliteColor);
	virtual CStaticEx& SetFont(LOGFONT lf);

// Attributes
public:
protected:
	void UpdateSurface();
	void ReconstructFont();
	void DrawGradientFill(CDC* pDC, CRect* pRect, COLORREF crStart, COLORREF crEnd, int nSegments);
    void LoadLinkCursor();

	COLORREF		m_crText;
	COLORREF		m_cr3DHiliteColor;
	HBRUSH			m_hwndBrush;
	HBRUSH			m_hBackBrush;
	LOGFONT			m_lf;
	CFont			m_font;
	BOOL			m_bState;
	BOOL			m_bTimer;
	LinkStyle		m_LinkStyle;
	BOOL			m_bTransparent;
	BOOL			m_bFont3d;
	BOOL			m_bToolTips;
	BOOL			m_bRotation;
	FlashType		m_Type;
	HCURSOR			m_hLinkCursor;
	Type3D			m_3dType;
	BackFillMode	m_fillmode;
	COLORREF		m_crHiColor;
	COLORREF		m_crLoColor;
	CString			m_sLink;
    UINT            m_nMsgNotify;
    HWND            m_hWndNotify;

	// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStaticEx)
	protected:
	virtual void PreSubclassWindow();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CStaticEx();

	// Generated message map functions
protected:
	//{{AFX_MSG(CStaticEx)
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnSysColorChange();
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STATICEX_H__A4EABEC5_2E8C_11D1_B79F_00805F9ECE10__INCLUDED_)
