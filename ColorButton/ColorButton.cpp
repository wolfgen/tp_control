// ColorButton.cpp : implementation file
//

#include "stdafx.h"
#include "math.h"
#include "ColorButton.h"
#include <Winuser.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTriangleButton

CColorButton::CColorButton()
{
	m_clrBk = ::GetSysColor(COLOR_BTNFACE);
	m_clrBtn = ::GetSysColor(COLOR_BTNFACE);
}

CColorButton::~CColorButton()
{
}

BEGIN_MESSAGE_MAP(CColorButton, CButton)
	//{{AFX_MSG_MAP(CRoundButton)
	ON_WM_NCHITTEST()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CColorButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	ASSERT(lpDrawItemStruct != NULL);

	CRect rect = lpDrawItemStruct->rcItem;
	CDC* pDC   = CDC::FromHandle(lpDrawItemStruct->hDC);
	UINT state = lpDrawItemStruct->itemState;
	UINT nStyle = GetStyle();

	int nSavedDC = pDC->SaveDC();

	//make the rect a square
	rect.bottom = rect.right = min(rect.bottom, rect.right);
	pDC->FillSolidRect(rect, m_clrBk);

	rect.right -= 1; rect.bottom -= 1;	//avoid drawing outside area

	//make some pens
	CPen HighlightPen(PS_SOLID, 1, ::GetSysColor(COLOR_3DHIGHLIGHT));
	CPen DarkShadowPen(PS_SOLID, 1, ::GetSysColor(COLOR_3DDKSHADOW));
	CPen ShadowPen(PS_SOLID, 1, ::GetSysColor(COLOR_3DSHADOW));
	CPen BlackPen(PS_SOLID, 1, RGB(0,0,0));
	CBrush brushBtn(m_clrBtn);
	CRgn rgnBtn;
	CPoint ptVertex[4];

	switch (m_nShape)
	{
	case CColorButton::Triangle:

		ptVertex[0] = CPoint(0, rect.bottom);
		ptVertex[1] = CPoint(0,  0);
		ptVertex[2] = CPoint(rect.right, rect.bottom / 2);

		rgnBtn.CreatePolygonRgn(ptVertex , 3, ALTERNATE);
		pDC->FillRgn(&rgnBtn, &brushBtn);

		if ((state & ODS_SELECTED))	
		{	
			//Button is down	
			pDC->SelectObject(ShadowPen);
			pDC->MoveTo(0, rect.bottom);
			pDC->LineTo(0, 0);
			pDC->LineTo(rect.right, rect.bottom / 2);

			pDC->SelectObject(DarkShadowPen);
			pDC->MoveTo(1, rect.bottom - 1);
			pDC->LineTo(1, 1);
			pDC->LineTo(rect.right - 1, rect.bottom / 2 - 1);

			pDC->SelectObject(HighlightPen);
			pDC->MoveTo(0, rect.bottom);
			pDC->LineTo(rect.right, rect.bottom / 2);		
		} 
		else 
		{											
			//Button is not down
			pDC->SelectObject(ShadowPen);
			pDC->MoveTo(0, rect.bottom);
			pDC->LineTo(rect.right - 1, rect.bottom / 2 - 1);

			pDC->SelectObject(DarkShadowPen);
			pDC->MoveTo(0, rect.bottom);
			pDC->LineTo(rect.right, rect.bottom / 2);

			pDC->SelectObject(HighlightPen);
			pDC->MoveTo(0, rect.bottom);
			pDC->LineTo(0, 0);
			pDC->LineTo(rect.right, rect.bottom / 2);
		}

		break;

	case CColorButton::Rectangle:

		ptVertex[0] = CPoint(0, 0);
		ptVertex[1] = CPoint(rect.right,0);
		ptVertex[2] = CPoint(rect.right, rect.bottom);
		ptVertex[3] = CPoint(0, rect.bottom);

		rgnBtn.CreatePolygonRgn(ptVertex , 4, ALTERNATE);
		pDC->FillRgn(&rgnBtn, &brushBtn);

		if ((state & ODS_SELECTED))	
		{	
			//Button is down	
			pDC->SelectObject(DarkShadowPen);
			pDC->MoveTo(rect.left, rect.bottom);
			pDC->LineTo(rect.left, rect.top);
			pDC->LineTo(rect.right, rect.top);

			pDC->SelectObject(ShadowPen);
			pDC->MoveTo(rect.left + 1, rect.bottom - 1);
			pDC->LineTo(rect.left + 1, rect.top + 1);
			pDC->LineTo(rect.right - 1, rect.top + 1);
	
		} 
		else 
		{											
			//Button is not down
			pDC->SelectObject(DarkShadowPen);
			pDC->MoveTo(rect.left, rect.bottom);
			pDC->LineTo(rect.right, rect.bottom);
			pDC->LineTo(rect.right, rect.top);

			pDC->SelectObject(ShadowPen);
			pDC->MoveTo(rect.left + 1, rect.bottom - 1);
			pDC->LineTo(rect.right - 1, rect.bottom - 1);
			pDC->LineTo(rect.right - 1, rect.top + 1);

			pDC->SelectObject(HighlightPen);
			pDC->MoveTo(rect.left, rect.bottom);
			pDC->LineTo(rect.left, rect.top);
			pDC->LineTo(rect.right, rect.top);
		}
		break;
	}
	pDC->RestoreDC(nSavedDC);									
}

