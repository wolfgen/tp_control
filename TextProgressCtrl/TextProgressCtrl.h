#pragma once

#include "DbMemCD.h"

// TextProgressCtrl.h : header file
//

class CTextProgressCtrl : public CProgressCtrl
{
// Construction
public:
    CTextProgressCtrl();

// Attributes
public:

// Operations
public:
    int         SetPos(int nPos);
    int         StepIt();
    void        SetRange(int nLower, int nUpper);
    int         OffsetPos(int nPos);
    int         SetStep(int nStep);
    void        SetForeColour(COLORREF col);
    void        SetBkColour(COLORREF col);
    void        SetTextForeColour(COLORREF col);
    void        SetTextBkColour(COLORREF col);
    COLORREF    GetForeColour();
    COLORREF    GetBkColour();
    COLORREF    GetTextForeColour();
    COLORREF    GetTextBkColour();
	int			GetPos(){return m_nPos;}
    void        SetShowText(BOOL bShow);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CTextProgressCtrl)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CTextProgressCtrl();

    // Generated message map functions
protected:
    int         m_nPos, 
                m_nStepSize, 
                m_nMax, 
                m_nMin;
    CString     m_strText;
    BOOL        m_bShowText;
    int         m_nBarWidth;
    COLORREF    m_colFore,
                m_colBk,
                m_colTextFore,
                m_colTextBk;

    //{{AFX_MSG(CTextProgressCtrl)
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG
    afx_msg LRESULT OnSetText(WPARAM, LPARAM szText);
    afx_msg LRESULT OnGetText(WPARAM cchTextMax, LPARAM szText);

    DECLARE_MESSAGE_MAP()
};

