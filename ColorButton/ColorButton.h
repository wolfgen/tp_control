#pragma once

#ifndef __COLORBUTTON_H__INCLUDED
#define __COLORBUTTON_H__INCLUDED



class CColorButton : public CButton
{
public:
	enum POINTDIRECTION {POINT_UP, POINT_DOWN, POINT_LEFT, POINT_RIGHT};
	
	// Construction
public:
	CColorButton();
	virtual ~CColorButton();
	void SetColorInfo(COLORREF clrBk, COLORREF clrBtn) {m_clrBk = clrBk; m_clrBtn = clrBtn;}
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTriangleButton)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	enum enumShape {Rectangle, Triangle};

	COLORREF	m_clrBk;
	COLORREF	m_clrBtn;
	enumShape	m_nShape;
	// Generated message map functions
protected:
	//{{AFX_MSG(CTriangleButton)
	//}}AFX_MSG
	

	DECLARE_MESSAGE_MAP()
};


#endif // __TRIANGLEBUTTON_H__INCLUDED
