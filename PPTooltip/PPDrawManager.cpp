#include "stdafx.h"
#include "PPDrawManager.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


/*
	DIBs use RGBQUAD format:
		0xbb 0xgg 0xrr 0x00
*/
#ifndef CLR_TO_RGBQUAD
#define CLR_TO_RGBQUAD(clr)     (RGB(GetBValue(clr), GetGValue(clr), GetRValue(clr)))
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPPDrawManager::CPPDrawManager()
{

}

CPPDrawManager::~CPPDrawManager()
{

}

void CPPDrawManager::GetSizeOfIcon(HICON hIcon, LPSIZE pSize) const
{
	pSize->cx = 0;
	pSize->cy = 0;
	if (hIcon != NULL)
	{
		ICONINFO ii;
		// Gets icon dimension
		::ZeroMemory(&ii, sizeof(ICONINFO));
		if (::GetIconInfo(hIcon, &ii))
		{
			pSize->cx = (DWORD)(ii.xHotspot * 2);
			pSize->cy = (DWORD)(ii.yHotspot * 2);
			//release icon mask bitmaps
			if(ii.hbmMask)
				::DeleteObject(ii.hbmMask);
			if(ii.hbmColor)
				::DeleteObject(ii.hbmColor);
		} //if
	} //if
} //End GetSizeOfIcon

void CPPDrawManager::GetSizeOfBitmap(HBITMAP hBitmap, LPSIZE pSize) const
{
	pSize->cx = 0;
	pSize->cy = 0;
	if (hBitmap != NULL)
	{
		BITMAP	csBitmapSize;
		// Get bitmap size
		int nRetValue = ::GetObject(hBitmap, sizeof(csBitmapSize), &csBitmapSize);
		if (nRetValue)
		{
			pSize->cx = (DWORD)csBitmapSize.bmWidth;
			pSize->cy = (DWORD)csBitmapSize.bmHeight;
		} //if
	} //if
} //End GetSizeOfBitmap

void CPPDrawManager::AlphaBitBlt(HDC hDestDC, int nDestX, int nDestY, DWORD dwWidth, DWORD dwHeight, HDC hSrcDC, int nSrcX, int nSrcY, int percent /* = 100 */)
{
	_ASSERT ((NULL != hDestDC) || (NULL != hSrcDC));

	if (percent >= 100)
	{
		::BitBlt(hDestDC, nDestX, nDestY, dwWidth, dwHeight, hSrcDC, nSrcX, nSrcY, SRCCOPY);
		return;
	} //if

	HDC hTempDC = ::CreateCompatibleDC(hDestDC);
	if (NULL == hTempDC)
		return;
	
	//Creates Source DIB
	LPBITMAPINFO lpbiSrc;
	// Fill in the BITMAPINFOHEADER
	lpbiSrc = (LPBITMAPINFO) new BYTE[sizeof(BITMAPINFOHEADER)];
	lpbiSrc->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lpbiSrc->bmiHeader.biWidth = dwWidth;
	lpbiSrc->bmiHeader.biHeight = dwHeight;
	lpbiSrc->bmiHeader.biPlanes = 1;
	lpbiSrc->bmiHeader.biBitCount = 32;
	lpbiSrc->bmiHeader.biCompression = BI_RGB;
	lpbiSrc->bmiHeader.biSizeImage = dwWidth * dwHeight;
	lpbiSrc->bmiHeader.biXPelsPerMeter = 0;
	lpbiSrc->bmiHeader.biYPelsPerMeter = 0;
	lpbiSrc->bmiHeader.biClrUsed = 0;
	lpbiSrc->bmiHeader.biClrImportant = 0;
	
	COLORREF* pSrcBits = NULL;
	HBITMAP hSrcDib = CreateDIBSection (
		hSrcDC, lpbiSrc, DIB_RGB_COLORS, (void **)&pSrcBits,
		NULL, NULL);
	
	if ((NULL != hSrcDib) && (NULL != pSrcBits))
	{
		HBITMAP hOldTempBmp = (HBITMAP)::SelectObject (hTempDC, hSrcDib);
		::BitBlt (hTempDC, 0, 0, dwWidth, dwHeight, hSrcDC, nSrcX, nSrcY, SRCCOPY);
		::SelectObject (hTempDC, hOldTempBmp);
		
		//Creates Destination DIB
		LPBITMAPINFO lpbiDest;
		// Fill in the BITMAPINFOHEADER
		lpbiDest = (LPBITMAPINFO) new BYTE[sizeof(BITMAPINFOHEADER)];
		lpbiDest->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		lpbiDest->bmiHeader.biWidth = dwWidth;
		lpbiDest->bmiHeader.biHeight = dwHeight;
		lpbiDest->bmiHeader.biPlanes = 1;
		lpbiDest->bmiHeader.biBitCount = 32;
		lpbiDest->bmiHeader.biCompression = BI_RGB;
		lpbiDest->bmiHeader.biSizeImage = dwWidth * dwHeight;
		lpbiDest->bmiHeader.biXPelsPerMeter = 0;
		lpbiDest->bmiHeader.biYPelsPerMeter = 0;
		lpbiDest->bmiHeader.biClrUsed = 0;
		lpbiDest->bmiHeader.biClrImportant = 0;
		
		COLORREF* pDestBits = NULL;
		HBITMAP hDestDib = CreateDIBSection (
			hDestDC, lpbiDest, DIB_RGB_COLORS, (void **)&pDestBits,
			NULL, NULL);
		
		if ((NULL != hDestDib) && (NULL != pDestBits))
		{
			::SelectObject (hTempDC, hDestDib);
			::BitBlt (hTempDC, 0, 0, dwWidth, dwHeight, hDestDC, nDestX, nDestY, SRCCOPY);
			::SelectObject (hTempDC, hOldTempBmp);

			double src_darken = (double)percent / 100.0;
			double dest_darken = 1.0 - src_darken;
			
			for (DWORD pixel = 0; pixel < dwWidth * dwHeight; pixel++, pSrcBits++, pDestBits++)
			{
				*pDestBits = PixelAlpha(*pSrcBits, src_darken, *pDestBits, dest_darken);
			} //for
			
			::SelectObject (hTempDC, hDestDib);
			::BitBlt (hDestDC, nDestX, nDestY, dwWidth, dwHeight, hTempDC, 0, 0, SRCCOPY);
			::SelectObject (hTempDC, hOldTempBmp);

			delete lpbiDest;
			::DeleteObject(hDestDib);
		} //if
		delete lpbiSrc;
		::DeleteObject(hSrcDib);
	} //if

	::DeleteDC(hTempDC);
} //End AlphaBitBlt

void CPPDrawManager::AlphaChannelBitBlt(HDC hDestDC, int nDestX, int nDestY, DWORD dwWidth, DWORD dwHeight, HDC hSrcDC, int nSrcX, int nSrcY)
{
	_ASSERT ((NULL != hDestDC) || (NULL != hSrcDC));

	HDC hTempDC = ::CreateCompatibleDC(hDestDC);
	if (NULL == hTempDC)
		return;
	
	//Creates Source DIB
	LPBITMAPINFO lpbiSrc;
	// Fill in the BITMAPINFOHEADER
	lpbiSrc = (LPBITMAPINFO) new BYTE[sizeof(BITMAPINFOHEADER)];
	lpbiSrc->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lpbiSrc->bmiHeader.biWidth = dwWidth;
	lpbiSrc->bmiHeader.biHeight = dwHeight;
	lpbiSrc->bmiHeader.biPlanes = 1;
	lpbiSrc->bmiHeader.biBitCount = 32;
	lpbiSrc->bmiHeader.biCompression = BI_RGB;
	lpbiSrc->bmiHeader.biSizeImage = dwWidth * dwHeight;
	lpbiSrc->bmiHeader.biXPelsPerMeter = 0;
	lpbiSrc->bmiHeader.biYPelsPerMeter = 0;
	lpbiSrc->bmiHeader.biClrUsed = 0;
	lpbiSrc->bmiHeader.biClrImportant = 0;
	
	COLORREF* pSrcBits = NULL;
	HBITMAP hSrcDib = CreateDIBSection (
		hSrcDC, lpbiSrc, DIB_RGB_COLORS, (void **)&pSrcBits,
		NULL, NULL);
	
	if ((NULL != hSrcDib) && (NULL != pSrcBits))
	{
		HBITMAP hOldTempBmp = (HBITMAP)::SelectObject (hTempDC, hSrcDib);
		::BitBlt (hTempDC, 0, 0, dwWidth, dwHeight, hSrcDC, nSrcX, nSrcY, SRCCOPY);
		::SelectObject (hTempDC, hOldTempBmp);
		
		//Creates Destination DIB
		LPBITMAPINFO lpbiDest;
		// Fill in the BITMAPINFOHEADER
		lpbiDest = (LPBITMAPINFO) new BYTE[sizeof(BITMAPINFOHEADER)];
		lpbiDest->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		lpbiDest->bmiHeader.biWidth = dwWidth;
		lpbiDest->bmiHeader.biHeight = dwHeight;
		lpbiDest->bmiHeader.biPlanes = 1;
		lpbiDest->bmiHeader.biBitCount = 32;
		lpbiDest->bmiHeader.biCompression = BI_RGB;
		lpbiDest->bmiHeader.biSizeImage = dwWidth * dwHeight;
		lpbiDest->bmiHeader.biXPelsPerMeter = 0;
		lpbiDest->bmiHeader.biYPelsPerMeter = 0;
		lpbiDest->bmiHeader.biClrUsed = 0;
		lpbiDest->bmiHeader.biClrImportant = 0;
		
		COLORREF* pDestBits = NULL;
		HBITMAP hDestDib = CreateDIBSection (
			hDestDC, lpbiDest, DIB_RGB_COLORS, (void **)&pDestBits,
			NULL, NULL);
		
		if ((NULL != hDestDib) && (NULL != pDestBits))
		{
			::SelectObject (hTempDC, hDestDib);
			::BitBlt (hTempDC, 0, 0, dwWidth, dwHeight, hDestDC, nDestX, nDestY, SRCCOPY);
			::SelectObject (hTempDC, hOldTempBmp);

			double src_darken;
			BYTE nAlpha;
			
			for (DWORD pixel = 0; pixel < dwWidth * dwHeight; pixel++, pSrcBits++, pDestBits++)
			{
				nAlpha = LOBYTE(*pSrcBits >> 24);
				src_darken = (double)nAlpha / 255.0;
				*pDestBits = PixelAlpha(*pSrcBits, src_darken, *pDestBits, 1.0 - src_darken);
			} //for
			
			::SelectObject (hTempDC, hDestDib);
			::BitBlt (hDestDC, nDestX, nDestY, dwWidth, dwHeight, hTempDC, 0, 0, SRCCOPY);
			::SelectObject (hTempDC, hOldTempBmp);

			delete lpbiDest;
			::DeleteObject(hDestDib);
		} //if
		delete lpbiSrc;
		::DeleteObject(hSrcDib);
	} //if

	::DeleteDC(hTempDC);
} //End of AlphaChannelBitBlt

HBITMAP CPPDrawManager::CreateImageEffect(HBITMAP hBitmap, DWORD dwWidth, DWORD dwHeight, DWORD dwEffect, BOOL bUseMask /* = TRUE */, COLORREF clrMask /* = RGB(255, 0, 255) */, COLORREF clrMono /* = RGB(255, 255, 255) */)
{
	HBITMAP hOldSrcBmp = NULL;
	HBITMAP hOldResBmp = NULL;  
	HDC hMainDC = NULL;
	HDC hSrcDC = NULL;
	HDC hResDC = NULL;
	
	hMainDC = ::GetDC(NULL);
	hSrcDC = ::CreateCompatibleDC(hMainDC);
	hResDC = ::CreateCompatibleDC(hMainDC);

	hOldSrcBmp = (HBITMAP)::SelectObject(hSrcDC, hBitmap);

	LPBITMAPINFO lpbi;

	// Fill in the BITMAPINFOHEADER
	lpbi = (LPBITMAPINFO) new BYTE[sizeof(BITMAPINFOHEADER)];
	lpbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lpbi->bmiHeader.biWidth = dwWidth;
	lpbi->bmiHeader.biHeight = dwHeight;
	lpbi->bmiHeader.biPlanes = 1;
	lpbi->bmiHeader.biBitCount = 32;
	lpbi->bmiHeader.biCompression = BI_RGB;
	lpbi->bmiHeader.biSizeImage = dwWidth * dwHeight;
	lpbi->bmiHeader.biXPelsPerMeter = 0;
	lpbi->bmiHeader.biYPelsPerMeter = 0;
	lpbi->bmiHeader.biClrUsed = 0;
	lpbi->bmiHeader.biClrImportant = 0;

	COLORREF* pBits = NULL;
	HBITMAP hDibBmp = CreateDIBSection (
		hSrcDC, lpbi, DIB_RGB_COLORS, (void **)&pBits,
		NULL, NULL);

	if (hDibBmp == NULL || pBits == NULL)
	{
		delete lpbi;
		_ASSERT (FALSE);
		return NULL;
	} //if

	hOldResBmp = (HBITMAP)::SelectObject (hResDC, hDibBmp);
	::BitBlt (hResDC, 0, 0, dwWidth, dwHeight, hSrcDC, 0, 0, SRCCOPY);

	clrMask = CLR_TO_RGBQUAD(clrMask);
	clrMono = CLR_TO_RGBQUAD(clrMono);

	DWORD dwAlpha;
	for (DWORD pixel = 0; pixel < dwWidth * dwHeight; pixel++, *pBits++)
	{
		COLORREF color = (COLORREF)*pBits;
		//ENG: Extract an original alpha value
		dwAlpha = color & 0xFF000000;
		if (dwAlpha != 0) 
			m_bIsAlpha = TRUE;
		if (bUseMask && (color == clrMask))
		{
			//This is transparent area
			color = RGB(0, 0, 0);
		}
		else 
		{
			//ENG: Color conversion
			if (dwEffect & IMAGE_EFFECT_GRAYEN) color = GrayMirrorColor(color);
			if (dwEffect & IMAGE_EFFECT_DARKEN) color = DarkenColor(color, 0.75);
			if (dwEffect & IMAGE_EFFECT_LIGHTEN) color = LightenColor(color, 0.25);
			if (dwEffect & IMAGE_EFFECT_MONOCHROME) color = clrMono;
		} //if
		if (dwEffect & IMAGE_EFFECT_INVERT) color = InvertColor(color);
		//ENG: Merges a color with an original alpha value
		*pBits = (color | dwAlpha);
	} //for

	::SelectObject(hSrcDC, hOldSrcBmp);
	::SelectObject(hResDC, hOldResBmp);
	::DeleteDC(hSrcDC);
	::DeleteDC(hResDC);
	::ReleaseDC(NULL, hMainDC);
	
	delete lpbi;

	return hDibBmp;
} //End CreateImageEffect

//----------------------------------------------------------
// CPPDrawManager::GrayMirrorColor()
//	Graying color in RGBQUAD format
//----------------------------------------------------------
// Parameter:
//	clrColor	- RGBQUAD value from DIB
// Return value:
//	A grayed color in the RGBQUAD format
//----------------------------------------------------------
COLORREF CPPDrawManager::GrayMirrorColor(COLORREF clrColor)
{
	BYTE nGrayColor = (BYTE)((GetBValue(clrColor) * 0.299) + (GetGValue(clrColor) * 0.587) + (GetRValue(clrColor) * 0.114));
	
	return RGB(nGrayColor, nGrayColor, nGrayColor);
} //End of GrayMirrorColor

COLORREF CPPDrawManager::GrayColor(COLORREF clrColor)
{
	BYTE nGrayColor = (BYTE)((GetRValue(clrColor) * 0.299) + (GetGValue(clrColor) * 0.587) + (GetBValue(clrColor) * 0.114));
	
	return RGB(nGrayColor, nGrayColor, nGrayColor);
} //End GrayColor

COLORREF CPPDrawManager::InvertColor(COLORREF clrColor)
{
	return RGB(255 - GetRValue(clrColor), 255 - GetGValue(clrColor), 255 - GetBValue(clrColor));
} //End InvertColor

COLORREF CPPDrawManager::DarkenColor(COLORREF clrColor, double darken)
{
	if (darken >= 0.0 && darken < 1.0)
	{
		BYTE color_r, color_g, color_b;
		color_r = (BYTE)(GetRValue(clrColor) * darken);
		color_g = (BYTE)(GetGValue(clrColor) * darken);
		color_b = (BYTE)(GetBValue(clrColor) * darken);
		clrColor = RGB(color_r, color_g, color_b);
	} //if
	
	return clrColor;
} //End DarkenColor

COLORREF CPPDrawManager::LightenColor(COLORREF clrColor, double lighten)
{
	if (lighten > 0.0 && lighten <= 1.0)
	{
		BYTE color_r, color_g, color_b;
		
		lighten += 1.0;
		color_r = (BYTE)min((DWORD)GetRValue(clrColor) * lighten, 255.0);
		color_g = (BYTE)min((DWORD)GetGValue(clrColor) * lighten, 255.0);
		color_b = (BYTE)min((DWORD)GetBValue(clrColor) * lighten, 255.0);
		clrColor = RGB(color_r, color_g, color_b);
	} //if
	
	return clrColor;
} //End LightenColor

COLORREF CPPDrawManager::PixelAlpha(COLORREF clrSrc, double src_darken, COLORREF clrDest, double dest_darken)
{
	return RGB (GetRValue (clrSrc) * src_darken + GetRValue (clrDest) * dest_darken, 
				GetGValue (clrSrc) * src_darken + GetGValue (clrDest) * dest_darken, 
				GetBValue (clrSrc) * src_darken + GetBValue (clrDest) * dest_darken);
	
} //End PixelAlpha

HICON CPPDrawManager::StretchIcon(HICON hIcon, DWORD dwWidth, DWORD dwHeight)
{
	HICON hStretchedIcon = NULL;
	HDC   hMainDC = NULL;
	HDC   hSrcDC = NULL;
	HDC   hDestDC = NULL;
	BITMAP bmp;
	HBITMAP hOldSrcBitmap = NULL;
	HBITMAP hOldDestBitmap = NULL;
	ICONINFO csOriginal, csStretched;
	
	if (!::GetIconInfo(hIcon, &csOriginal))
		return FALSE;
	
	hMainDC = ::GetDC(NULL);
	hSrcDC = ::CreateCompatibleDC(hMainDC);
	hDestDC = ::CreateCompatibleDC(hMainDC);
	
	if ((NULL == hMainDC) || (NULL == hSrcDC) || (NULL == hDestDC))
		return NULL;
	
	if (::GetObject(csOriginal.hbmColor, sizeof(BITMAP), &bmp))
	{
		DWORD	dwWidthOrg = csOriginal.xHotspot * 2;
		DWORD	dwHeightOrg = csOriginal.yHotspot * 2;
		
		csStretched.hbmColor = ::CreateBitmap(dwWidth, dwHeight, bmp.bmPlanes, bmp.bmBitsPixel, NULL);
		if (NULL != csStretched.hbmColor)
		{
			hOldSrcBitmap = (HBITMAP)::SelectObject(hSrcDC, csOriginal.hbmColor);
			hOldDestBitmap = (HBITMAP)::SelectObject(hDestDC, csStretched.hbmColor);
			::StretchBlt(hDestDC, 0, 0, dwWidth, dwHeight, hSrcDC, 0, 0, dwWidthOrg, dwHeightOrg, SRCCOPY);
			if (::GetObject(csOriginal.hbmMask, sizeof(BITMAP), &bmp))
			{
				csStretched.hbmMask = ::CreateBitmap(dwWidth, dwHeight, bmp.bmPlanes, bmp.bmBitsPixel, NULL);
				if (NULL != csStretched.hbmMask)
				{
					::SelectObject(hSrcDC, csOriginal.hbmMask);
					::SelectObject(hDestDC, csStretched.hbmMask);
					::StretchBlt(hDestDC, 0, 0, dwWidth, dwHeight, hSrcDC, 0, 0, dwWidthOrg, dwHeightOrg, SRCCOPY);
				} //if
			} //if
			::SelectObject(hSrcDC, hOldSrcBitmap);
			::SelectObject(hDestDC, hOldDestBitmap);
			csStretched.fIcon = TRUE;
			hStretchedIcon = ::CreateIconIndirect(&csStretched);
		} //if
		::DeleteObject(csStretched.hbmColor);
		::DeleteObject(csStretched.hbmMask);
	} //if
	
	::DeleteObject(csOriginal.hbmColor);
	::DeleteObject(csOriginal.hbmMask);
	::DeleteDC(hSrcDC);
	::DeleteDC(hDestDC);
	::ReleaseDC(NULL, hMainDC);
	
	return hStretchedIcon;
} //End StretchIcon

void CPPDrawManager::FillGradient (HDC hDC, LPCRECT lpRect, 
								COLORREF colorStart, COLORREF colorFinish, 
								BOOL bHorz/* = TRUE*/)
{
    // this will make 2^6 = 64 fountain steps
    int nShift = 6;
    int nSteps = 1 << nShift;

	RECT r2;
	r2.top = lpRect->top;
	r2.left = lpRect->left;
	r2.right = lpRect->right;
	r2.bottom = lpRect->bottom;

	int nHeight = lpRect->bottom - lpRect->top;
	int nWidth = lpRect->right - lpRect->left;

	for (int i = 0; i < nSteps; i++)
    {
        // do a little alpha blending
        BYTE bR = (BYTE) ((GetRValue(colorStart) * (nSteps - i) +
                   GetRValue(colorFinish) * i) >> nShift);
        BYTE bG = (BYTE) ((GetGValue(colorStart) * (nSteps - i) +
                   GetGValue(colorFinish) * i) >> nShift);
        BYTE bB = (BYTE) ((GetBValue(colorStart) * (nSteps - i) +
                   GetBValue(colorFinish) * i) >> nShift);

		HBRUSH hBrush = ::CreateSolidBrush(RGB(bR, bG, bB));
		
        // then paint with the resulting color

        if (!bHorz)
        {
            r2.top = lpRect->top + ((i * nHeight) >> nShift);
            r2.bottom = lpRect->top + (((i + 1) * nHeight) >> nShift);
            if ((r2.bottom - r2.top) > 0)
                ::FillRect(hDC, &r2, hBrush);
        }
        else
        {
            r2.left = lpRect->left + ((i * nWidth) >> nShift);
            r2.right = lpRect->left + (((i + 1) * nWidth) >> nShift);
            if ((r2.right - r2.left) > 0)
                ::FillRect(hDC, &r2, hBrush);
        } //if
		
		if (NULL != hBrush)
		{
			::DeleteObject(hBrush);
			hBrush = NULL;
		} //if
    } //for
} //End FillGradient

#ifdef USE_SHADE
void CPPDrawManager::SetShade(LPCRECT lpRect, UINT shadeID /* = 0 */, BYTE granularity /* = 8 */, 
						  BYTE coloring /* = 0 */, COLORREF hicr /* = 0 */, COLORREF midcr /* = 0 */, COLORREF locr /* = 0 */)
{
	long	sXSize,sYSize,bytes,j,i,k,h;
	BYTE	*iDst ,*posDst;
	
	sYSize = lpRect->bottom - lpRect->top; 
	sXSize = lpRect->right - lpRect->left; 

	m_dNormal.Create(sXSize,sYSize,8);					//create the default bitmap

	long r,g,b;											//build the shaded palette
	for(i = 0; i < 129; i++)
	{
		r=((128-i)*GetRValue(locr)+i*GetRValue(midcr))/128;
		g=((128-i)*GetGValue(locr)+i*GetGValue(midcr))/128;
		b=((128-i)*GetBValue(locr)+i*GetBValue(midcr))/128;
		m_dNormal.SetPaletteIndex((BYTE)i,(BYTE)r,(BYTE)g,(BYTE)b);
	} //for
	for(i=1;i<129;i++){
		r=((128-i)*GetRValue(midcr)+i*GetRValue(hicr))/128;
		g=((128-i)*GetGValue(midcr)+i*GetGValue(hicr))/128;
		b=((128-i)*GetBValue(midcr)+i*GetBValue(hicr))/128;
		m_dNormal.SetPaletteIndex((BYTE)(i+127),(BYTE)r,(BYTE)g,(BYTE)b);
	} //for

	m_dNormal.BlendPalette(hicr,coloring);	//color the palette

	bytes = m_dNormal.GetLineWidth();
	iDst = m_dNormal.GetBits();
	posDst =iDst;
	long a,x,y,d,xs,idxmax,idxmin;

	int grainx2 = RAND_MAX/(max(1,2*granularity));
	idxmax=255-granularity;
	idxmin=granularity;

	switch (shadeID)
	{
//----------------------------------------------------
	case EFFECT_METAL:
		m_dNormal.Clear();
		// create the strokes
		k=40;	//stroke granularity
		for(a=0;a<200;a++){
			x=rand()/(RAND_MAX/sXSize); //stroke postion
			y=rand()/(RAND_MAX/sYSize);	//stroke position
			xs=rand()/(RAND_MAX/min(sXSize,sYSize))/2; //stroke lenght
			d=rand()/(RAND_MAX/k);	//stroke color
			for(i=0;i<xs;i++){
				if (((x-i)>0)&&((y+i)<sYSize))
					m_dNormal.SetPixelIndex(x-i,y+i,(BYTE)d);
				if (((x+i)<sXSize)&&((y-i)>0))
					m_dNormal.SetPixelIndex(sXSize-x+i,y-i,(BYTE)d);
			} //for
		} //for
		//blend strokes with SHS_DIAGONAL
		posDst =iDst;
		a=(idxmax-idxmin-k)/2;
		for(i = 0; i < sYSize; i++) {
			for(j = 0; j < sXSize; j++) {
				d=posDst[j]+((a*i)/sYSize+(a*(sXSize-j))/sXSize);
				posDst[j]=(BYTE)d;
				posDst[j]+=rand()/grainx2;
			} //for
			posDst+=bytes;
		} //for

		break;
//----------------------------------------------------
	case EFFECT_HARDBUMP:	// 
		//set horizontal bump
		for(i = 0; i < sYSize; i++) {
			k=(255*i/sYSize)-127;
			k=(k*(k*k)/128)/128;
			k=(k*(128-granularity*2))/128+128;
			for(j = 0; j < sXSize; j++) {
				posDst[j]=(BYTE)k;
				posDst[j]+=rand()/grainx2-granularity;
			} //for
			posDst+=bytes;
		} //for
		//set vertical bump
		d=min(16,sXSize/6);	//max edge=16
		a=sYSize*sYSize/4;
		posDst =iDst;
		for(i = 0; i < sYSize; i++) {
			y=i-sYSize/2;
			for(j = 0; j < sXSize; j++) {
				x=j-sXSize/2;
				xs=sXSize/2-d+(y*y*d)/a;
				if (x>xs) posDst[j]=(BYTE)idxmin+(BYTE)(((sXSize-j)*128)/d);
				if ((x+xs)<0) posDst[j]=(BYTE)idxmax-(BYTE)((j*128)/d);
				posDst[j]+=rand()/grainx2-granularity;
			} //for
			posDst+=bytes;
		} //for
		break;
//----------------------------------------------------
	case EFFECT_SOFTBUMP: //
		for(i = 0; i < sYSize; i++) {
			h=(255*i/sYSize)-127;
			for(j = 0; j < sXSize; j++) {
				k=(255*(sXSize-j)/sXSize)-127;
				k=(h*(h*h)/128)/128+(k*(k*k)/128)/128;
				k=k*(128-granularity)/128+128;
				if (k<idxmin) k=idxmin;
				if (k>idxmax) k=idxmax;
				posDst[j]=(BYTE)k;
				posDst[j]+=rand()/grainx2-granularity;
			} //for
			posDst+=bytes;
		} //for
		break;
//----------------------------------------------------
	case EFFECT_VBUMP: // 
		for(j = 0; j < sXSize; j++) {
			k=(255*(sXSize-j)/sXSize)-127;
			k=(k*(k*k)/128)/128;
			k=(k*(128-granularity))/128+128;
			for(i = 0; i < sYSize; i++) {
				posDst[j+i*bytes]=(BYTE)k;
				posDst[j+i*bytes]+=rand()/grainx2-granularity;
			} //for
		} //for
		break;
//----------------------------------------------------
	case EFFECT_HBUMP: //
		for(i = 0; i < sYSize; i++) {
			k=(255*i/sYSize)-127;
			k=(k*(k*k)/128)/128;
			k=(k*(128-granularity))/128+128;
			for(j = 0; j < sXSize; j++) {
				posDst[j]=(BYTE)k;
				posDst[j]+=rand()/grainx2-granularity;
			} //for
			posDst+=bytes;
		} //for
		break;
//----------------------------------------------------
	case EFFECT_DIAGSHADE:	//
		a=(idxmax-idxmin)/2;
		for(i = 0; i < sYSize; i++) {
			for(j = 0; j < sXSize; j++) {
				posDst[j]=(BYTE)(idxmin+a*i/sYSize+a*(sXSize-j)/sXSize);
				posDst[j]+=rand()/grainx2-granularity;
			} //for
			posDst+=bytes;
		} //for
		break;
//----------------------------------------------------
	case EFFECT_HSHADE:	//
		a=idxmax-idxmin;
		for(i = 0; i < sYSize; i++) {
			k=a*i/sYSize+idxmin;
			for(j = 0; j < sXSize; j++) {
				posDst[j]=(BYTE)k;
				posDst[j]+=rand()/grainx2-granularity;
			} //for
			posDst+=bytes;
		} //for
		break;
//----------------------------------------------------
	case EFFECT_VSHADE:	//:
		a=idxmax-idxmin;
		for(j = 0; j < sXSize; j++) {
			k=a*(sXSize-j)/sXSize+idxmin;
			for(i = 0; i < sYSize; i++) {
				posDst[j+i*bytes]=(BYTE)k;
				posDst[j+i*bytes]+=rand()/grainx2-granularity;
			} //for
		} //for
		break;
//----------------------------------------------------
	case EFFECT_NOISE:
		for(i = 0; i < sYSize; i++) {
			for(j = 0; j < sXSize; j++) {
				posDst[j]=128+rand()/grainx2-granularity;
			} //for
			posDst+=bytes;
		} //for
	} //switch
//----------------------------------------------------
} //End SetShade
#endif

void CPPDrawManager::FillEffect(HDC hDC, DWORD dwEffect, LPCRECT lpRect, COLORREF clrBegin, COLORREF clrMid /* = 0 */, COLORREF clrEnd /* = 0 */,  BYTE granularity /* = 0 */, BYTE coloring /* = 0 */)
{
	HBRUSH hBrush = NULL;

	RECT rect;
	rect.left = lpRect->left;
	rect.top = lpRect->top;
	rect.right = lpRect->right;
	rect.bottom = lpRect->bottom;

	int nHeight = rect.bottom - rect.top;
	int nWidth = rect.right - rect.left;
	
	switch (dwEffect)
	{
	default:
		hBrush = ::CreateSolidBrush(clrBegin);
		::FillRect(hDC, lpRect, hBrush);
		break;
	case EFFECT_HGRADIENT:
		FillGradient(hDC, lpRect, clrBegin, clrEnd, TRUE);
		break;
	case EFFECT_VGRADIENT:
		FillGradient(hDC, lpRect, clrBegin, clrEnd, FALSE);
		break;
	case EFFECT_HCGRADIENT:
		rect.right = rect.left + nWidth / 2;
		FillGradient(hDC, &rect, clrBegin, clrEnd, TRUE);
		rect.left = rect.right;
		rect.right = lpRect->right;
		FillGradient(hDC, &rect, clrEnd, clrBegin, TRUE);
		break;
	case EFFECT_3HGRADIENT:
		rect.right = rect.left + nWidth / 2;
		FillGradient(hDC, &rect, clrBegin, clrMid, TRUE);
		rect.left = rect.right;
		rect.right = lpRect->right;
		FillGradient(hDC, &rect, clrMid, clrEnd, TRUE);
		break;
	case EFFECT_VCGRADIENT:
		rect.bottom = rect.top + nHeight / 2;
		FillGradient(hDC, &rect, clrBegin, clrEnd, FALSE);
		rect.top = rect.bottom;
		rect.bottom = lpRect->bottom;
		FillGradient(hDC, &rect, clrEnd, clrBegin, FALSE);
		break;
	case EFFECT_3VGRADIENT:
		rect.bottom = rect.top + nHeight / 2;
		FillGradient(hDC, &rect, clrBegin, clrMid, FALSE);
		rect.top = rect.bottom;
		rect.bottom = lpRect->bottom;
		FillGradient(hDC, &rect, clrMid, clrEnd, FALSE);
		break;
#ifdef USE_SHADE
	case EFFECT_NOISE:
	case EFFECT_DIAGSHADE:
	case EFFECT_HSHADE:
	case EFFECT_VSHADE:
	case EFFECT_HBUMP:
	case EFFECT_VBUMP:
	case EFFECT_SOFTBUMP:
	case EFFECT_HARDBUMP:
	case EFFECT_METAL:
		rect.left = 0;
		rect.top = 0;
		rect.right = nWidth;
		rect.bottom = nHeight;
		SetShade(&rect, dwEffect, granularity, coloring, clrBegin, clrMid, clrEnd);
		m_dNormal.Draw(hDC, lpRect->left, lpRect->top);
		break; 
#endif
	} //switch

	if (NULL != hBrush)
	{
		::DeleteObject(hBrush);
		hBrush = NULL;
	} //if
} //End FillEffect

void CPPDrawManager::MultipleCopy(HDC hDestDC, int nDestX, int nDestY, DWORD dwDestWidth, DWORD dwDestHeight, 
										HDC hSrcDC, int nSrcX, int nSrcY, DWORD dwSrcWidth, DWORD dwSrcHeight)
{
	// Horizontal copying
	int right, bottom;
	int nDestRight = (int)(nDestX + dwDestWidth);
	int nDestBottom = (int)(nDestY + dwDestHeight);
	for (int x = nDestX; x < nDestRight; x+= dwSrcWidth)
	{
		right = min (x + (int)dwSrcWidth, nDestRight);
		// Vertical copying
		for (int y = nDestY; y < nDestBottom; y+= dwSrcHeight)
		{
			bottom = min (y + (int)dwSrcHeight, nDestBottom);
			::BitBlt(hDestDC, x, y, right - x, bottom - y, hSrcDC, nSrcX, nSrcY, SRCCOPY);
		} //for
	} //for
} //End MultipleCopy

void CPPDrawManager::DrawBitmap(HDC hDC, int x, int y, DWORD dwWidth, DWORD dwHeight, HBITMAP hSrcBitmap,
					BOOL bUseMask, COLORREF crMask, 
					DWORD dwEffect /* = IMAGE_EFFECT_NONE */, 
					BOOL bShadow /* = FALSE */, 
					DWORD dwCxShadow /* = PPDRAWMANAGER_SHADOW_XOFFSET */, 
					DWORD dwCyShadow /* = PPDRAWMANAGER_SHADOW_YOFFSET */,
					DWORD dwCxDepth /* = PPDRAWMANAGER_SHADOW_XDEPTH */, 
					DWORD dwCyDepth /* = PPDRAWMANAGER_SHADOW_YDEPTH */,
					COLORREF clrShadow /* = PPDRAWMANAGER_SHADOW_COLOR */)
{
	m_bIsAlpha = FALSE;
	if (NULL == hSrcBitmap)
		return;

	SIZE sz;
	GetSizeOfBitmap(hSrcBitmap, &sz);

	HDC hSrcDC = ::CreateCompatibleDC(hDC);
	HDC hDestDC = ::CreateCompatibleDC(hDC);
	
	HBITMAP hBitmapTemp = ::CreateCompatibleBitmap(hDC, dwWidth, dwHeight);

	HBITMAP hOldSrcBitmap = (HBITMAP)::SelectObject(hSrcDC, hSrcBitmap);
	HBITMAP hOldDestBitmap = (HBITMAP)::SelectObject(hDestDC, hBitmapTemp);

	//Scales a bitmap if need
	if (((DWORD)sz.cx != dwWidth) || ((DWORD)sz.cy != dwHeight))
		::StretchBlt(hDestDC, 0, 0, dwWidth, dwHeight, hSrcDC, 0, 0, sz.cx, sz.cy, SRCCOPY);
	else
		::BitBlt(hDestDC, 0, 0, dwWidth, dwHeight, hSrcDC, 0, 0, SRCCOPY);

	::SelectObject(hDestDC, hOldDestBitmap);
	
	HBITMAP hMaskBmp = CreateImageEffect(hBitmapTemp, dwWidth, dwHeight, IMAGE_EFFECT_MASK, bUseMask, crMask);
	HBITMAP hBitmap = CreateImageEffect(hBitmapTemp, dwWidth, dwHeight, dwEffect, bUseMask, crMask, clrShadow);
	
	if (bShadow)
	{
		if (dwEffect & IMAGE_EFFECT_SHADOW)
		{
			POINT ptShadow;
			ptShadow.x = x + dwCxShadow;
			ptShadow.y = y + dwCyShadow;
			HBITMAP hShadowBmp =  CreateImageEffect(hBitmapTemp, dwWidth, dwHeight, IMAGE_EFFECT_MASK, bUseMask, crMask, InvertColor(clrShadow));
			DrawShadow(hDC, ptShadow.x, ptShadow.y, dwWidth, dwHeight, hShadowBmp, dwEffect & IMAGE_EFFECT_GRADIENT_SHADOW, dwCxDepth, dwCyDepth);
			::DeleteObject(hShadowBmp);
		}
		else
		{
			x += dwCxShadow;
			y += dwCyShadow;
		} //if
	} //if
	
	if (m_bIsAlpha)
	{
		::SelectObject(hSrcDC, hBitmap);
		AlphaChannelBitBlt(hDC, x, y, dwWidth, dwHeight, hSrcDC, 0, 0);
	}
	else
	{
		//Merge the image mask with background
		::SelectObject(hSrcDC, hMaskBmp);
		::BitBlt(hDC, x, y, dwWidth, dwHeight, hSrcDC, 0, 0, SRCAND);
		
		//Draw the image
		::SelectObject(hSrcDC, hBitmap);
		::BitBlt(hDC, x, y, dwWidth, dwHeight, hSrcDC, 0, 0, SRCPAINT);
	}
	
	::SelectObject(hSrcDC, hOldSrcBitmap);
	
	::DeleteDC(hDestDC);
	::DeleteDC(hSrcDC);

	::DeleteObject(hBitmap);
	::DeleteObject(hMaskBmp);
	::DeleteObject(hBitmapTemp);
} //End DrawBitmap

void CPPDrawManager::DrawIcon(HDC hDC, int x, int y, DWORD dwWidth, DWORD dwHeight, HICON hSrcIcon,
  							DWORD dwEffect /* = IMAGE_EFFECT_NONE */, 
							BOOL bShadow /* = FALSE */, 
							DWORD dwCxShadow /* = PPDRAWMANAGER_SHADOW_XOFFSET */, 
							DWORD dwCyShadow /* = PPDRAWMANAGER_SHADOW_YOFFSET */,
							DWORD dwCxDepth /* = PPDRAWMANAGER_SHADOW_XDEPTH */, 
							DWORD dwCyDepth /* = PPDRAWMANAGER_SHADOW_YDEPTH */,
							COLORREF clrShadow /* = PPDRAWMANAGER_SHADOW_COLOR */)
{
	m_bIsAlpha = FALSE;
	if (NULL == hSrcIcon)
		return;

	SIZE sz;
	GetSizeOfIcon(hSrcIcon, &sz);

	HICON hIcon = NULL;

	if (((DWORD)sz.cx == dwWidth) && ((DWORD)sz.cy == dwHeight))
		hIcon = ::CopyIcon(hSrcIcon);
	else hIcon = StretchIcon(hSrcIcon, dwWidth, dwHeight);
	
	ICONINFO csOriginal;

	if (!::GetIconInfo(hIcon, &csOriginal))
		return;

	HDC hSrcDC = ::CreateCompatibleDC(hDC);
	
	HBITMAP hBitmap;
	if (dwEffect & IMAGE_EFFECT_MONOCHROME)
		hBitmap = CreateImageEffect(csOriginal.hbmMask, dwWidth, dwHeight, dwEffect, TRUE, RGB(255, 255, 255), clrShadow);
	else
		hBitmap = CreateImageEffect(csOriginal.hbmColor, dwWidth, dwHeight, dwEffect, TRUE, RGB(0, 0, 0), clrShadow);
	HBITMAP hOldSrcBitmap = (HBITMAP)::SelectObject(hSrcDC, hBitmap);

	if (bShadow)
	{
		if (dwEffect & IMAGE_EFFECT_SHADOW)
		{
			POINT ptShadow;
			ptShadow.x = x + dwCxShadow;
			ptShadow.y = y + dwCyShadow;
			HBITMAP hShadowBmp =  CreateImageEffect(csOriginal.hbmMask, dwWidth, dwHeight, IMAGE_EFFECT_MASK, TRUE, RGB(255, 255, 255), InvertColor(clrShadow));
			DrawShadow(hDC, ptShadow.x, ptShadow.y, dwWidth, dwHeight, hShadowBmp, dwEffect & IMAGE_EFFECT_GRADIENT_SHADOW, dwCxDepth, dwCyDepth);
			::DeleteObject(hShadowBmp);
		}
		else
		{
			x += dwCxShadow;
			y += dwCyShadow;
		} //if
	} //if
	
	if (m_bIsAlpha)
	{
//		::SelectObject(hSrcDC, hBitmap);
		AlphaChannelBitBlt(hDC, x, y, dwWidth, dwHeight, hSrcDC, 0, 0);
	}
	else
	{
		//-------------------------------------------------------------------
		// !!! ATTENTION !!!
		// I don't know why a icon uses text's color
		// Therefore I change a text's color to BLACK and after draw I restore
		// original color
		//-------------------------------------------------------------------
		COLORREF crOldColor = ::SetTextColor(hDC, RGB(0, 0, 0));
		//Merge the image mask with background
		::SelectObject(hSrcDC, csOriginal.hbmMask);
		::BitBlt(hDC, x, y, dwWidth, dwHeight, hSrcDC, 0, 0, SRCAND);
		//Draw the image
		::SelectObject(hSrcDC, hBitmap);
		::BitBlt(hDC, x, y, dwWidth, dwHeight, hSrcDC, 0, 0, SRCPAINT);
		::SetTextColor(hDC, crOldColor);
	} //if

	::SelectObject(hSrcDC, hOldSrcBitmap);
	::DeleteDC(hSrcDC);
	::DeleteObject(hBitmap);
	::DestroyIcon(hIcon);

	::DeleteObject(csOriginal.hbmColor);
	::DeleteObject(csOriginal.hbmMask);
} //End DrawIcon

void CPPDrawManager::DrawShadow(HDC hDestDC, int nDestX, int nDestY, DWORD dwWidth, DWORD dwHeight, HBITMAP hMask, BOOL bGradient /* = FALSE */, DWORD dwDepthX /* = PPDRAWMANAGER_SHADOW_YOFFSET */, DWORD dwDepthY /* = PPDRAWMANAGER_SHADOW_XOFFSET */)
{
	HDC hSrcDC = ::CreateCompatibleDC(hDestDC);
	if (NULL == hSrcDC)
		return;

	HDC hTempDC = ::CreateCompatibleDC(hDestDC);
	if (NULL == hTempDC)
	{
		::DeleteDC(hSrcDC);
		return;
	} // if
	
	//Creates Source DIB
	LPBITMAPINFO lpbiSrc;
	// Fill in the BITMAPINFOHEADER
	lpbiSrc = (LPBITMAPINFO) new BYTE[sizeof(BITMAPINFOHEADER)];
	lpbiSrc->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lpbiSrc->bmiHeader.biWidth = dwWidth;
	lpbiSrc->bmiHeader.biHeight = dwHeight;
	lpbiSrc->bmiHeader.biPlanes = 1;
	lpbiSrc->bmiHeader.biBitCount = 32;
	lpbiSrc->bmiHeader.biCompression = BI_RGB;
	lpbiSrc->bmiHeader.biSizeImage = dwWidth * dwHeight;
	lpbiSrc->bmiHeader.biXPelsPerMeter = 0;
	lpbiSrc->bmiHeader.biYPelsPerMeter = 0;
	lpbiSrc->bmiHeader.biClrUsed = 0;
	lpbiSrc->bmiHeader.biClrImportant = 0;
	
	COLORREF* pSrcBits = NULL;
	HBITMAP hSrcDib = CreateDIBSection (
		hSrcDC, lpbiSrc, DIB_RGB_COLORS, (void **)&pSrcBits,
		NULL, NULL);
	
	if ((NULL != hSrcDib) && (NULL != pSrcBits))
	{
		HBITMAP hOldSrcBmp = (HBITMAP)::SelectObject (hSrcDC, hSrcDib);
		HBITMAP hOldTempBmp = (HBITMAP)::SelectObject (hTempDC, hMask);
		if (bGradient)
		{
			if (!(dwDepthX & 0x1)) dwDepthX++;
			if (!(dwDepthY & 0x1)) dwDepthY++;
			::BitBlt(hSrcDC, 0, 0, dwWidth, dwHeight, hTempDC, 0, 0, WHITENESS);
			::StretchBlt (hSrcDC, dwDepthX / 2, dwDepthY / 2, dwWidth - dwDepthX, dwHeight - dwDepthY, hTempDC, 0, 0, dwWidth, dwHeight, SRCCOPY);
		}
		else
		{
			::BitBlt(hSrcDC, 0, 0, dwWidth, dwHeight, hTempDC, 0, 0, SRCCOPY);
		} //if
		::SelectObject (hTempDC, hOldTempBmp);
		::SelectObject (hSrcDC, hOldSrcBmp);
		
		//Creates Destination DIB
		LPBITMAPINFO lpbiDest;
		// Fill in the BITMAPINFOHEADER
		lpbiDest = (LPBITMAPINFO) new BYTE[sizeof(BITMAPINFOHEADER)];
		lpbiDest->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		lpbiDest->bmiHeader.biWidth = dwWidth;
		lpbiDest->bmiHeader.biHeight = dwHeight;
		lpbiDest->bmiHeader.biPlanes = 1;
		lpbiDest->bmiHeader.biBitCount = 32;
		lpbiDest->bmiHeader.biCompression = BI_RGB;
		lpbiDest->bmiHeader.biSizeImage = dwWidth * dwHeight;
		lpbiDest->bmiHeader.biXPelsPerMeter = 0;
		lpbiDest->bmiHeader.biYPelsPerMeter = 0;
		lpbiDest->bmiHeader.biClrUsed = 0;
		lpbiDest->bmiHeader.biClrImportant = 0;
		
		COLORREF* pDestBits = NULL;
		HBITMAP hDestDib = CreateDIBSection (
			hDestDC, lpbiDest, DIB_RGB_COLORS, (void **)&pDestBits,
			NULL, NULL);
		
		if ((NULL != hDestDib) && (NULL != pDestBits))
		{
			::SelectObject (hTempDC, hDestDib);
			::BitBlt (hTempDC, 0, 0, dwWidth, dwHeight, hDestDC, nDestX, nDestY, SRCCOPY);
			::SelectObject (hTempDC, hOldTempBmp);
			
			if (bGradient)
			{
				double * depth = new double [dwWidth * dwHeight];
				SmoothMaskImage(dwWidth, dwHeight, pSrcBits, dwDepthX, dwDepthY, depth);
				for(DWORD pixel = 0; pixel < dwWidth * dwHeight; pixel++, pDestBits++)
					*pDestBits = DarkenColor(*pDestBits, *(depth + pixel));
				delete [] depth;
			}
			else
			{
				for(DWORD pixel = 0; pixel < dwWidth * dwHeight; pixel++, pSrcBits++, pDestBits++)
					*pDestBits = DarkenColor(*pDestBits, (double)GetRValue(*pSrcBits) / 255.0);
			} //if
				
			
			::SelectObject (hTempDC, hDestDib);
			::BitBlt (hDestDC, nDestX, nDestY, dwWidth, dwHeight, hTempDC, 0, 0, SRCCOPY);
			::SelectObject (hTempDC, hOldTempBmp);

			delete lpbiDest;
			::DeleteObject(hDestDib);
		} //if
		delete lpbiSrc;
		::DeleteObject(hSrcDib);
	} //if
	::DeleteDC(hTempDC);
	::DeleteDC(hSrcDC);
} //End DrawIcon

void CPPDrawManager::DrawImageList(HDC hDC, int x, int y, DWORD dwWidth, DWORD dwHeight, HBITMAP hSrcBitmap,
					int nIndex, int cx, int cy,
					BOOL bUseMask, COLORREF crMask, 
					DWORD dwEffect /*= IMAGE_EFFECT_NONE*/, 
					BOOL bShadow /*= FALSE*/, 
					DWORD dwCxShadow /*= PPDRAWMANAGER_SHADOW_XOFFSET*/, 
					DWORD dwCyShadow /*= PPDRAWMANAGER_SHADOW_YOFFSET*/,
					DWORD dwCxDepth /*= PPDRAWMANAGER_SHADOW_XDEPTH*/, 
					DWORD dwCyDepth /*= PPDRAWMANAGER_SHADOW_YDEPTH*/,
					COLORREF clrShadow /*= PPDRAWMANAGER_SHADOW_COLOR*/)
{
	if ((NULL == hSrcBitmap) || !cx || !cy)
		return;

	SIZE sz;
	GetSizeOfBitmap(hSrcBitmap, &sz);

	//ENG: Gets a max columns and rows of the images on the bitmap
	int nMaxCol = sz.cx / cx;
	int nMaxRow = sz.cy / cy;
	int nMaxImages = nMaxCol * nMaxRow;

	if ((nIndex < nMaxImages) && nMaxCol && nMaxRow)
	{
		//ENG: Gets an specified image from the bitmap
		HDC hSrcDC = ::CreateCompatibleDC(hDC);
		HDC hDestDC = ::CreateCompatibleDC(hDC);
		HBITMAP hIconBmp = ::CreateCompatibleBitmap(hDC, cx, cy);
		HBITMAP hOldSrcBmp = (HBITMAP)::SelectObject(hSrcDC, hSrcBitmap);
		HBITMAP hOldDestBmp = (HBITMAP)::SelectObject(hDestDC, hIconBmp);
		::BitBlt(hDestDC, 0, 0, cx, cy, hSrcDC, (nIndex % nMaxCol) * cx, (nIndex / nMaxCol) * cy, SRCCOPY);
		::SelectObject(hSrcDC, hOldSrcBmp);
		::SelectObject(hDestDC, hOldDestBmp);
		::DeleteDC(hSrcDC);
		::DeleteDC(hDestDC);
		DrawBitmap( hDC, x, y, dwWidth, dwHeight, hIconBmp, 
					bUseMask, crMask, dwEffect, 
					bShadow, dwCxShadow, dwCyShadow, 
					dwCxDepth, dwCyDepth, clrShadow);
		::DeleteObject(hIconBmp);
	} //if
} //End of DrawImageList

void CPPDrawManager::MaskToDepth(HDC hDC, DWORD dwWidth, DWORD dwHeight, HBITMAP hMask, double * pDepth, BOOL bGradient /* = FALSE */, DWORD dwDepthX /* = PPDRAWMANAGER_CXSHADOW */, DWORD dwDepthY /* = PPDRAWMANAGER_CYSHADOW */)
{
	HDC hSrcDC = ::CreateCompatibleDC(hDC);
	if (NULL == hSrcDC)
	{
		::DeleteDC(hSrcDC);
		hSrcDC = NULL;
		return;
	} //if

	HDC hTempDC = ::CreateCompatibleDC(hDC);
	if (NULL == hTempDC)
	{
		::DeleteDC(hTempDC);
		hTempDC = NULL;
		return;
	} //if
	
	//Creates Source DIB
	LPBITMAPINFO lpbiSrc;
	// Fill in the BITMAPINFOHEADER
	lpbiSrc = (LPBITMAPINFO) new BYTE[sizeof(BITMAPINFOHEADER)];
	lpbiSrc->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lpbiSrc->bmiHeader.biWidth = dwWidth;
	lpbiSrc->bmiHeader.biHeight = dwHeight;
	lpbiSrc->bmiHeader.biPlanes = 1;
	lpbiSrc->bmiHeader.biBitCount = 32;
	lpbiSrc->bmiHeader.biCompression = BI_RGB;
	lpbiSrc->bmiHeader.biSizeImage = dwWidth * dwHeight;
	lpbiSrc->bmiHeader.biXPelsPerMeter = 0;
	lpbiSrc->bmiHeader.biYPelsPerMeter = 0;
	lpbiSrc->bmiHeader.biClrUsed = 0;
	lpbiSrc->bmiHeader.biClrImportant = 0;
	
	COLORREF* pSrcBits = NULL;
	HBITMAP hSrcDib = CreateDIBSection (
		hSrcDC, lpbiSrc, DIB_RGB_COLORS, (void **)&pSrcBits,
		NULL, NULL);
	
	if ((NULL != hSrcDib) && (NULL != pSrcBits))
	{
		HBITMAP hOldSrcBmp = (HBITMAP)::SelectObject (hSrcDC, hSrcDib);
		HBITMAP hOldTempBmp = (HBITMAP)::SelectObject (hTempDC, hMask);
		if (bGradient)
		{
			if (!(dwDepthX & 0x1)) dwDepthX++;
			if (!(dwDepthY & 0x1)) dwDepthY++;
			::BitBlt(hSrcDC, 0, 0, dwWidth, dwHeight, hTempDC, 0, 0, WHITENESS);
			::StretchBlt (hSrcDC, dwDepthX / 2, dwDepthY / 2, dwWidth - dwDepthX, dwHeight - dwDepthY, hTempDC, 0, 0, dwWidth, dwHeight, SRCCOPY);
		}
		else
		{
			::BitBlt(hSrcDC, 0, 0, dwWidth, dwHeight, hTempDC, 0, 0, SRCCOPY);
		} //if
		::SelectObject (hTempDC, hOldTempBmp);
		::SelectObject (hSrcDC, hOldSrcBmp);
		
		if (bGradient)
		{
			SmoothMaskImage(dwWidth, dwHeight, pSrcBits, dwDepthX, dwDepthY, pDepth);
		}
		else
		{
			for (DWORD pixel = 0; pixel < (dwHeight * dwWidth); pixel++, pSrcBits++, pDepth++)
			{
				*pDepth = GetRValue(*pSrcBits) / 255;
			} //for
		} //if
		delete lpbiSrc;
		::DeleteObject(hSrcDib);
	} //if
	::DeleteDC(hTempDC);
	::DeleteDC(hSrcDC);
} //End MaskToDepth

void CPPDrawManager::DarkenByDepth(HDC hDC, int x, int y, DWORD dwWidth, DWORD dwHeight, double * pDepth)
{
	HDC hSrcDC = ::CreateCompatibleDC(hDC);
	if (NULL == hSrcDC)
	{
		::DeleteDC(hSrcDC);
		hSrcDC = NULL;
		return;
	} //if
	
	//Creates Source DIB
	LPBITMAPINFO lpbiSrc;
	// Fill in the BITMAPINFOHEADER
	lpbiSrc = (LPBITMAPINFO) new BYTE[sizeof(BITMAPINFOHEADER)];
	lpbiSrc->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lpbiSrc->bmiHeader.biWidth = dwWidth;
	lpbiSrc->bmiHeader.biHeight = dwHeight;
	lpbiSrc->bmiHeader.biPlanes = 1;
	lpbiSrc->bmiHeader.biBitCount = 32;
	lpbiSrc->bmiHeader.biCompression = BI_RGB;
	lpbiSrc->bmiHeader.biSizeImage = dwWidth * dwHeight;
	lpbiSrc->bmiHeader.biXPelsPerMeter = 0;
	lpbiSrc->bmiHeader.biYPelsPerMeter = 0;
	lpbiSrc->bmiHeader.biClrUsed = 0;
	lpbiSrc->bmiHeader.biClrImportant = 0;
	
	COLORREF* pSrcBits = NULL;
	HBITMAP hSrcDib = CreateDIBSection (
		hSrcDC, lpbiSrc, DIB_RGB_COLORS, (void **)&pSrcBits,
		NULL, NULL);
	
	if ((NULL != hSrcDib) && (NULL != pSrcBits))
	{
		HBITMAP hOldSrcBmp = (HBITMAP)::SelectObject (hSrcDC, hSrcDib);
		::BitBlt(hSrcDC, 0, 0, dwWidth, dwHeight, hDC, x, y, SRCCOPY);
		::SelectObject (hSrcDC, hOldSrcBmp);
		
		for (DWORD pixel = 0; pixel < (dwHeight * dwWidth); pixel++, pSrcBits++, pDepth++)
		{
			*pSrcBits = DarkenColor(*pSrcBits, *pDepth);
		} //for

		hOldSrcBmp = (HBITMAP)::SelectObject (hSrcDC, hSrcDib);
		::BitBlt(hDC, x, y, dwWidth, dwHeight, hSrcDC, 0, 0, SRCCOPY);
		::SelectObject (hSrcDC, hOldSrcBmp);

		delete lpbiSrc;
		::DeleteObject(hSrcDib);
	} //if
	::DeleteDC(hSrcDC);
} //DarkenByDepth

void CPPDrawManager::SmoothMaskImage(const int ImageWidth, 
									 const int ImageHeight,
									 const COLORREF* const pInitImg,
									 const int KerWidth,
									 const int KerHeight,
									 double* const pResImg_R /*= NULL*/)
{
	double* const pfBuff1 = new double[(ImageWidth  + KerWidth  - 1) * 
		(ImageHeight + KerHeight - 1)];
	double* const pfBuff2 = new double[(ImageWidth  + KerWidth  - 1) * 
		(ImageHeight + KerHeight - 1)];
	
	// expanding initial image with a padding procdure
	double* p = pfBuff1;
	const COLORREF * pInitImg_It = pInitImg;
	if(NULL != pResImg_R)
	{
		for(int _i = - KerHeight/2; _i < ImageHeight +  KerHeight/2; _i++)
		{
			for(int _j = -KerWidth/2; _j < ImageWidth + KerWidth/2;  _j++, p++)
			{
				if ((_i >= 0) && (_i < ImageHeight) && (_j >= 0) && (_j < ImageWidth))
				{
					*p = GetRValue(*pInitImg_It++);
					//					pInitImg_It++;
				}
				else
					*p = 255;
			} //for
		} //for
		
		//---
		GetPartialSums(pfBuff1,
			(ImageHeight + KerHeight - 1),
			(ImageWidth  + KerWidth  - 1),
			KerHeight,
			KerWidth,
			pfBuff2,
			pResImg_R);
		
		for(int i = 0; i < ImageHeight*ImageWidth; i++)
			*(pResImg_R + i) /= KerHeight*KerWidth*255;
	} //if
	
	
	delete []pfBuff1;
	delete []pfBuff2;
} //End SmoothMaskImage

void CPPDrawManager::GetPartialSums(const double* const pM,
									unsigned int nMRows,
									unsigned int nMCols,
									unsigned int nPartRows,
									unsigned int nPartCols,
									double* const pBuff,
									double* const pRes)
{
	const double* it1;
	const double* it2;
	const double* it3;
	
	double* pRowsPartSums;
	const unsigned int nRowsPartSumsMRows = nMRows;
	const unsigned int nRowsPartSumsMCols = nMCols - nPartCols + 1;	
	
	const unsigned int nResMRows = nMRows - nPartRows + 1;
	const unsigned int nResMCols = nMCols - nPartCols + 1;
	
	
	unsigned int i,j;
	double s;
	it1          = pM;
	pRowsPartSums = pBuff;
	
	for(i = 0; i < nMRows; i++)
	{
		//-------------
		it2 = it1;
		s = 0;
		for(j = 0;j < nPartCols;j++)s+=*it2++;
		//-------------
		it3 = it1;
		*pRowsPartSums++ = s;
		for(/*j = nPartCols*/; j < nMCols; j++)
		{
			s+=*it2 - *it3;
			*pRowsPartSums++ = s;
			it2++;
			it3++;
		} //for
		//--
		it1 += nMCols;
	} //for
	const double* it4;
	const double* it5;
	const double* it6;
	
	double* pResIt;
	
	it4    = pBuff;
	pResIt = pRes;
	
	for(j = 0; j < nRowsPartSumsMCols; j++)
	{
		pResIt = pRes + j;
		//-------------
		it5 = it4;
		s = 0;
		for(i = 0; i < nPartRows; i++)
		{
			s += *it5;
			it5 += nRowsPartSumsMCols; 
		} //for
		//-------------
		it6 = it4;
		*pResIt = s;
		pResIt += nRowsPartSumsMCols;
		
		for(; i < nRowsPartSumsMRows; i++)
		{
			s += *it5 - *it6;//cout<<s<<endl;
			*pResIt = s;
			pResIt += nResMCols;
			it5 += nRowsPartSumsMCols;
			it6 += nRowsPartSumsMCols;
		} //for
		//--
		it4 ++;
	} //for
} //End GetPartialSums

void CPPDrawManager::DrawRectangle(HDC hDC, LPRECT lpRect, COLORREF crLight, COLORREF crDark, int nStyle /* = PEN_SOLID */, int nSize /* = 1 */)
{
	DrawRectangle(hDC, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom, crLight, crDark, nStyle, nSize);
}

void CPPDrawManager::DrawRectangle(HDC hDC, int left, int top, int right, int bottom, 
								   COLORREF crLight, COLORREF crDark, int nStyle /* = PEN_SOLID */, 
								   int nSize /* = 1 */)
{
	if ((PEN_NULL == nStyle) || (nSize < 1))
		return;

	int nSysPenStyle = PS_SOLID;
	int nDoubleLineOffset = nSize * 2;
	switch (nStyle)
	{
	case PEN_DASH: nSysPenStyle = PS_DASH; break;
	case PEN_DOT: nSysPenStyle = PS_DOT; break;
	case PEN_DASHDOT: nSysPenStyle = PS_DASHDOT; break;
	case PEN_DASHDOTDOT: nSysPenStyle = PS_DASHDOTDOT; break;
	case PEN_DOUBLE:
	case PEN_SOLID:
	default:
		nSysPenStyle = PS_SOLID; 
		break;
	} //switch

	//Insideframe
	left += nSize / 2;
	top += nSize / 2;
	right -= nSize / 2;
	bottom -= nSize / 2;

	//Creates a light pen
	HPEN hPen = ::CreatePen(nSysPenStyle, nSize, crLight);
	HPEN hOldPen = (HPEN)::SelectObject(hDC, hPen);

	//Draw light border
	::MoveToEx(hDC, left, bottom, NULL);
	::LineTo(hDC, left, top);
	::LineTo(hDC, right, top);
	if (PEN_DOUBLE == nStyle)
	{
		::MoveToEx(hDC, left + nDoubleLineOffset, bottom - nDoubleLineOffset, NULL);
		::LineTo(hDC, left + nDoubleLineOffset, top + nDoubleLineOffset);
		::LineTo(hDC, right - nDoubleLineOffset, top + nDoubleLineOffset);
	} //if

	//Creates a dark pen if needed
	if (crLight != crDark)
	{
		SelectObject(hDC, hOldPen);
		::DeleteObject(hPen);
		hPen = ::CreatePen(nSysPenStyle, nSize, crDark);
		hOldPen = (HPEN)::SelectObject(hDC, hPen);
	} //if

	//Draw dark border
	::MoveToEx(hDC, right, top, NULL);
	::LineTo(hDC, right, bottom);
	::LineTo(hDC, left, bottom);
	if (PEN_DOUBLE == nStyle)
	{
		::MoveToEx(hDC, right - nDoubleLineOffset, top + nDoubleLineOffset, NULL);
		::LineTo(hDC, right - nDoubleLineOffset, bottom - nDoubleLineOffset);
		::LineTo(hDC, left + nDoubleLineOffset, bottom - nDoubleLineOffset);
	} //if

	SelectObject(hDC, hOldPen);
	::DeleteObject(hPen);
} //End DrawRectangle

void CPPDrawManager::DrawLine(HDC hDC, 
							  int xStart, int yStart, int xEnd, int yEnd, 
							  COLORREF color, int nStyle /* = PEN_SOLID */, 
							  int nSize /* = 1 */) const
{
	if ((PEN_NULL == nStyle) || (nSize < 1))
		return;
	
	int nSysPenStyle;
	int nDoubleLineOffset = nSize * 2;
	switch (nStyle)
	{
	case PEN_DASH: nSysPenStyle = PS_DASH; break;
	case PEN_DOT: nSysPenStyle = PS_DOT; break;
	case PEN_DASHDOT: nSysPenStyle = PS_DASHDOT; break;
	case PEN_DASHDOTDOT: nSysPenStyle = PS_DASHDOTDOT; break;
	case PEN_DOUBLE:
	case PEN_SOLID:
	default:
		nSysPenStyle = PS_SOLID; 
		break;
	} //switch

	HPEN hPen = ::CreatePen(nSysPenStyle, nSize, color);
	HPEN hOldPen = (HPEN)::SelectObject(hDC, hPen);

	::MoveToEx(hDC, xStart, yStart, NULL);
	::LineTo(hDC, xEnd, yEnd);
	
	if (PEN_DOUBLE == nStyle)
	{
		if (xStart != xEnd)
		{
			yStart += nDoubleLineOffset;
			yEnd += nDoubleLineOffset;
		} //if
		
		if (yStart != yEnd)
		{
			xStart += nDoubleLineOffset;
			xEnd += nDoubleLineOffset;
		} //if
		::MoveToEx(hDC, xStart, yStart, NULL);
		::LineTo(hDC, xEnd, yEnd);
	} //if
	SelectObject(hDC, hOldPen);
	::DeleteObject(hPen);
} //End DrawLine

