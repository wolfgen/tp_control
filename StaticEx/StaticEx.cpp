// StaticEx.cpp : implementation file
//

#include "stdafx.h"
#include "StaticEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CStaticEx, CStatic)
    //{{AFX_MSG_MAP(CStaticEx)
    ON_WM_TIMER()
    ON_WM_LBUTTONDOWN()
    ON_WM_SETCURSOR()
    ON_WM_SYSCOLORCHANGE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStaticEx Version 1.2
//
// From now on I'll try to keep a log of fixes and enhancements...
// 
// The new feature were added due to the response of people.
// All I ask is to all you programmers out there, is if you add, fix or
// enhance this code, sent me a copy and I'll send the copy on to www.codeproject.com
//
// Happy Software Engineer :)
// 
// New features include:
//
// A. Support for 3D Fonts
// B. Support for background transparency
// C. More comments provided
// D. If alignment is 'centered' and the window text is seperated by '\r\n'
//      the will be centered accordingly - requested by someone @ nasa ;)
// E. Support for font rotation.
// F. Respond to System Color Change
// G. OnPaint improved performance - using Double Buffering Technique
//
// Thanks to:
// Mark McDowell    - For suggestion on 'Increasing the flexibility of "hypertext" setting...'
// Erich Ruth        - For suggestion on 'Font Rotation'
//

/////////////////////////////////////////////////////////////////////////////
// CStaticEx Version 1.3
//
// A. Added SS_LEFTNOWORDWRAP to include wordwrap
// B. Fix repainting problem 
// C. Fix SetBkColor
// D. Added SS_CENTER

// Thanks to:
// Marius                        - Added styling problem.
// Azing Vondeling & Broker        - Spotting painting Problem.
// Mel Stober                    - Back Color & SS_CENTER
// 
/////////////////////////////////////////////////////////////////////////////
// CStaticEx Version 1.4
//
// A. Fix to transparency mode
// B. Added new SetText3DHiliteColor to change the 3D Font face color - default is white.
// 
// Thanks to:
// michael.groeger                - Spotting Transparency with other controls bug.
//
//
/////////////////////////////////////////////////////////////////////////////
// CStaticEx Version 1.5
//
// A. Sanity handle check
// B. Support Interface Charset
// C. Check compilition with _UNICODE
// D. Fix hyperlink feature
// E. Support default Dialog Font
// F. Inclusion of SS_OWNERDRAW via control creation and subclassing
// G. Modification to Text aligmnent code
// H. New background gradient fill function
// 
// Thanks to:
// Steve Kowald                - Using null handles 
// Alan Chan                - Supporting International Windows
// Dieter Fauth                - Request for default Dialog font
// Herb Illfelder            - Text Alignment code
// 
/////////////////////////////////////////////////////////////////////////////
// CStaticEx Version 1.6
// Jeroen Roosendaal        - SetFont suggestion
// Laurent                    - Spotting SelectObject bugs
// Bernie                    - Fix PreCreateWindow bug
// Jignesh I. Patel            - Added expanded tabs feature
// Jim Farmelant             - Fix SetText crash


//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::CStaticEx
//
// Description:        Default contructor
//
// INPUTS:          
// 
// RETURNS:         
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                26/08/98    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
CStaticEx::CStaticEx()
{
    m_crText = GetSysColor(COLOR_WINDOWTEXT);

    m_hBackBrush      = NULL;

    m_crHiColor       = 0;
    m_crLoColor       = 0;

    m_bTimer          = FALSE;
    m_bState          = FALSE;
    m_bTransparent    = FALSE;
    m_LinkStyle       = LinkNone;
    m_hLinkCursor     = NULL;
    m_Type            = None;
    m_bFont3d         = FALSE;
    m_bToolTips       = FALSE;
    m_bRotation       = FALSE;
    m_fillmode        = Normal;
    m_cr3DHiliteColor = RGB(255,255,255);

    m_hwndBrush = ::CreateSolidBrush(GetSysColor(COLOR_3DFACE));
}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::~CStaticEx
//
// Description:        
//
// INPUTS:          
// 
// RETURNS:         
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                26/08/98    1.0     Origin
// NT ALMOND                15092000    1.5        Handle Check
//////////////////////////////////////////////////////////////////////////
CStaticEx::~CStaticEx()
{
    // Clean up
    m_font.DeleteObject();
    ::DeleteObject(m_hwndBrush);

    // Stop Checking complaining
    if (m_hBackBrush) {
        ::DeleteObject(m_hBackBrush);
    }

}

void CStaticEx::UpdateSurface()
{
    CRect (rc);
    GetWindowRect(rc);
    RedrawWindow();

    GetParent()->ScreenToClient(rc);
    GetParent()->InvalidateRect(rc,TRUE);
    GetParent()->UpdateWindow();
}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::ReconstructFont
//
// Description:        Helper function to build font after it was changed
//
// INPUTS:          
// 
// RETURNS:         
//
// NOTES:            PROTECTED
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                26/08/98    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
void CStaticEx::ReconstructFont()
{
    m_font.DeleteObject();
    BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

    ASSERT(bCreated);
}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::OnPaint
//
// Description:        Handles all the drawing code for the label
//
// INPUTS:          
// 
// RETURNS:         
//
// NOTES:            Called by Windows... not by USER
//                    Probably needs tiding up a some point.
//                    Different states will require this code to be reworked.
//
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                22/10/98    1.0     Origin
// NT ALMOND                15092000    1.5        Handle Check
// NT ALMOND                15092000    1.5        Alignment mods
// NT ALMOND                15092000    1.5        Gradient Fill Mode
// NT ALMOND                02072002    1.6        Fill SelectObject bugs
// NT ALMOND                02072002    1.6        Added to expand tabs
//////////////////////////////////////////////////////////////////////////

void CStaticEx::OnPaint() 
{
    CPaintDC dc(this); // device context for painting

    DWORD dwFlags = 0;

    CRect rc;
    GetClientRect(rc);

    CString strText;
    GetWindowText(strText);

    CBitmap bmp;


    ///////////////////////////////////////////////////////
    //
    // Set up for double buffering...
    //
    CDC* pDCMem;
    CBitmap*    pOldBitmap = NULL;

    if (!m_bTransparent)
    {
        pDCMem = new CDC;
        pDCMem->CreateCompatibleDC(&dc);
        bmp.CreateCompatibleBitmap(&dc,rc.Width(),rc.Height());
        pOldBitmap = pDCMem->SelectObject(&bmp);
    }
    else
    {
        pDCMem = &dc;
    }

    UINT nMode = pDCMem->SetBkMode(TRANSPARENT);


    COLORREF crText = pDCMem->SetTextColor(m_crText);
    CFont *pOldFont = pDCMem->SelectObject(&m_font);


    // Fill in backgound if not transparent
    if (!m_bTransparent)
    {
        if (m_fillmode == Normal)
        {
            CBrush br;

            if (m_hBackBrush != NULL)
                br.Attach(m_hBackBrush);
            else
                br.Attach(m_hwndBrush);

            pDCMem->FillRect(rc,&br);

            br.Detach();
        }
        else // Gradient Fill
        {
            DrawGradientFill(pDCMem, &rc, m_crLoColor, m_crHiColor, 100);
        }

    }


    // If the text is flashing turn the text color on
    // then to the color of the window background.

    LOGBRUSH lb;
    ZeroMemory(&lb,sizeof(lb));

    // Stop Checking complaining
    if (m_hBackBrush)
        ::GetObject(m_hBackBrush,sizeof(lb),&lb);


    // Something to do with flashing
    if (!m_bState && m_Type == Text)
        pDCMem->SetTextColor(lb.lbColor);

    DWORD style = GetStyle();

    switch (style & SS_TYPEMASK)
    {
        case SS_RIGHT: 
            dwFlags = DT_RIGHT | DT_WORDBREAK; 
            break; 

        case SS_CENTER: 
            dwFlags = SS_CENTER | DT_WORDBREAK;
            break;

        case SS_LEFTNOWORDWRAP: 
            dwFlags = DT_LEFT; 
            break;

        default: // treat other types as left
        case SS_LEFT: 
            dwFlags = DT_LEFT | DT_WORDBREAK; 
            break;
    }    


    // Added to expand tabs...
    if(strText.Find(_T('\t')) != -1)
        dwFlags |= DT_EXPANDTABS;

    // If the text centered make an assumtion that
    // the will want to center verticly as well
    if (style & SS_CENTERIMAGE)
    {
        // Apply 
        if (strText.Find(_T("\r\n")) == -1)
        {
            dwFlags |= DT_VCENTER;

            // And because DT_VCENTER only works with single lines
            dwFlags |= DT_SINGLELINE; 
        }

    }

    //
    // 3333   DDDDD
    //     3  D    D
    //   33   D     D    E F X 
    //     3  D    D
    // 3333   DDDDD
    //
    //
    if (m_bRotation)
    {
        int nAlign = pDCMem->SetTextAlign (TA_BASELINE);

        CPoint pt;
        GetViewportOrgEx (pDCMem->m_hDC,&pt) ;
        SetViewportOrgEx (pDCMem->m_hDC,rc.Width() / 2, rc.Height() / 2, NULL) ;
        pDCMem->TextOut (0, 0, strText) ;
        SetViewportOrgEx (pDCMem->m_hDC,pt.x / 2, pt.y / 2, NULL) ;
        pDCMem->SetTextAlign (nAlign);
    }
    else
    {
        if (m_bFont3d) 
        {
            pDCMem->DrawText(strText,rc,dwFlags);

            pDCMem->SetTextColor(m_cr3DHiliteColor);

            if (m_3dType == Raised) {
                rc.OffsetRect(-1,-1);
            }
            else {
                rc.OffsetRect(1,1);
            }

            pDCMem->DrawText(strText,rc,dwFlags);
            m_3dType;
        }
        else {
            if (dwFlags | DT_VCENTER) {
                CRect tCalc;
                tCalc = *rc;
                pDCMem->DrawText(strText, &tCalc, DT_CALCRECT | DT_LEFT | DT_WORDBREAK);

                rc.top = rc.top + (rc.Height() - tCalc.Height()) / 2;

                dwFlags &= (~(DT_VCENTER | DT_SINGLELINE));
                dwFlags |= DT_LEFT;
            }

            pDCMem->DrawText(strText,rc,dwFlags);
        }
    }

    // Restore DC's State
    pDCMem->SetBkMode(nMode);
    pDCMem->SelectObject(pOldFont);
    pDCMem->SetTextColor(crText);

    if (!m_bTransparent)
    {
        dc.BitBlt(0,0,rc.Width(),rc.Height(),pDCMem,0,0,SRCCOPY);
        // continue DC restore 
        pDCMem->SelectObject ( pOldBitmap ) ;
        delete pDCMem;
    }
}


//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::OnTimer
//
// Description:        Used in conjunction with 'FLASH' functions
//
// INPUTS:          Windows API
// 
// RETURNS:         Windows API
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                26/08/98    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
void CStaticEx::OnTimer(UINT nIDEvent) 
{

    m_bState = !m_bState;

    UpdateSurface();

    CStatic::OnTimer(nIDEvent);
}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::OnSetCursor
//
// Description:        Used in conjunction with 'LINK' function
//
// INPUTS:          Windows API
// 
// RETURNS:         Windows API
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                26/08/98    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
BOOL CStaticEx::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{

    if ((m_hLinkCursor) && (m_LinkStyle != LinkNone)) {
        ::SetCursor(m_hLinkCursor);
        return TRUE;
    }

    return CStatic::OnSetCursor(pWnd, nHitTest, message);
}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::OnLButtonDown
//
// Description:        Called when a link is click on
//
// INPUTS:          Windows API
// 
// RETURNS:         Windows API
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                26/08/98    1.0     Origin
// NT ALMOND                02072002    1.6     Added Mail support
//////////////////////////////////////////////////////////////////////////
void CStaticEx::OnLButtonDown(UINT nFlags, CPoint point) 
{

    if (!m_hWndNotify) {
        CString strLink;

        GetWindowText(strLink);
        if (m_LinkStyle == HyperLink)
        {
            ShellExecute(NULL,_T("open"),m_sLink.IsEmpty() ? strLink : m_sLink,NULL,NULL,SW_SHOWNORMAL);
        }
        if (m_LinkStyle == MailLink)
        {
            strLink = _T("mailto:") + strLink;
            ShellExecute( NULL, NULL,  strLink,  NULL, NULL, SW_SHOWNORMAL );
        }
    }
    else {
        ::SendMessage(m_hWndNotify, m_nMsgNotify, GetDlgCtrlID(), 0);
    }

    CStatic::OnLButtonDown(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// THE FUNCTIONS START HERE :----
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::SetText
//
// Description:        Short cut to set window text - caption - label
//
// INPUTS:          Text to use
// 
// RETURNS:         Reference to this
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                26081998    1.0     Origin
// NT ALMOND                02072002    1.6     Crash Fix
//////////////////////////////////////////////////////////////////////////
CStaticEx& CStaticEx::SetText(const CString& strText)
{
    if(IsWindow(this->GetSafeHwnd())) 
    {
        SetWindowText(strText);
        UpdateSurface();
    }

    return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::SetTextColor
//
// Description:        Sets the text color 
//
// INPUTS:          True or false
// 
// RETURNS:         Reference to 'this' object
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                22/10/98    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
CStaticEx& CStaticEx::SetTextColor(COLORREF crText)
{

    m_crText = crText;

    UpdateSurface();
    return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::SetFontBold
//
// Description:        Sets the font ot bold 
//
// INPUTS:          True or false
// 
// RETURNS:         Reference to 'this' object
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                22/10/98    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
CStaticEx& CStaticEx::SetFontBold(BOOL bBold)
{    

    m_lf.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
    ReconstructFont();
    UpdateSurface();
    return *this;
}



//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::SetFontUnderline
//
// Description:        Sets font underline attribue
//
// INPUTS:          True of false
// 
// RETURNS:         Reference to 'this' object
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                26/08/98    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
CStaticEx& CStaticEx::SetFontUnderline(BOOL bSet)
{    
    m_lf.lfUnderline = bSet;
    ReconstructFont();
    UpdateSurface();

    return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::SetFontItalic
//
// Description:        Sets font italic attribute
//
// INPUTS:          True of false
// 
// RETURNS:         Reference to 'this' object
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                26/08/98    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
CStaticEx& CStaticEx::SetFontItalic(BOOL bSet)
{

    m_lf.lfItalic = bSet;
    ReconstructFont();
    UpdateSurface();

    return *this;    
}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::SetSunken
//
// Description:        Sets sunken effect on border
//
// INPUTS:          True of false
// 
// RETURNS:         Reference to 'this' object
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                26/08/98    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
CStaticEx& CStaticEx::SetSunken(BOOL bSet)
{

    if (!bSet)
        ModifyStyleEx(WS_EX_STATICEDGE,0,SWP_DRAWFRAME);
    else
        ModifyStyleEx(0,WS_EX_STATICEDGE,SWP_DRAWFRAME);

    return *this;    
}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::SetBorder
//
// Description:        Toggles the border on/off
//
// INPUTS:          True of false
// 
// RETURNS:         Reference to 'this' object
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                26/08/98    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
CStaticEx& CStaticEx::SetBorder(BOOL bSet)
{

    if (!bSet)
        ModifyStyle(WS_BORDER,0,SWP_DRAWFRAME);
    else
        ModifyStyle(0,WS_BORDER,SWP_DRAWFRAME);

    return *this;    
}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::SetFontSize
//
// Description:        Sets the font size
//
// INPUTS:          True of false
// 
// RETURNS:         Reference to 'this' object
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                26/08/98    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
CStaticEx& CStaticEx::SetFontSize(int nSize)
{

    CFont cf;
    LOGFONT lf;

    cf.CreatePointFont(nSize * 10, m_lf.lfFaceName);
    cf.GetLogFont(&lf);

    m_lf.lfHeight = lf.lfHeight;
    m_lf.lfWidth  = lf.lfWidth;

    ReconstructFont();
    UpdateSurface();

    return *this;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::SetBkColor
//
// Description:        Sets background color
//
// INPUTS:          Colorref of background color
// 
// RETURNS:         Reference to 'this' object
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                26/08/98    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
CStaticEx& CStaticEx::SetBkColor(COLORREF crBkgnd, COLORREF crBkgndHigh , BackFillMode mode)
{

    m_crLoColor = crBkgnd;
    m_crHiColor = crBkgndHigh;

    m_fillmode = mode;

    if (m_hBackBrush)
        ::DeleteObject(m_hBackBrush);


    if (m_fillmode == Normal)
        m_hBackBrush = ::CreateSolidBrush(crBkgnd);

    UpdateSurface();

    return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::SetFontName
//
// Description:        Sets the fonts face name
//
// INPUTS:          String containing font name
// 
// RETURNS:         Reference to 'this' object
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                26/08/98    1.0     Origin
// NT ALMOND                15092000    1.5        Support internation windows
//////////////////////////////////////////////////////////////////////////
CStaticEx& CStaticEx::SetFontName(const CString& strFont, BYTE byCharSet /* Default = ANSI_CHARSET */)
{    

    m_lf.lfCharSet = byCharSet;

    _tcscpy_s(m_lf.lfFaceName, LF_FACESIZE, strFont);
    ReconstructFont();
    UpdateSurface();

    return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::FlashText
//
// Description:        As the function states
//
// INPUTS:          True or false
// 
// RETURNS:         Reference to 'this' object
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                26/08/98    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
CStaticEx& CStaticEx::FlashText(BOOL bActivate)
{

    if (m_bTimer)
        KillTimer(1);

    if (bActivate)
    {
        m_bState = FALSE;

        m_bTimer = TRUE;

        SetTimer(1,500,NULL);

        m_Type = Text;
    }
    else
        m_Type = None; // Fix

    return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::FlashBackground
//
// Description:        As the function states
//
// INPUTS:          True or false
// 
// RETURNS:         Reference to 'this' object
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                26/08/98    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
CStaticEx& CStaticEx::FlashBackground(BOOL bActivate)
{

    if (m_bTimer)
        KillTimer(1);

    if (bActivate)
    {
        m_bState = FALSE;

        m_bTimer = TRUE;
        SetTimer(1,500,NULL);

        m_Type = Background;
    }

    return *this;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::SetLink
//
// Description:        Indicates the string is a link
//
// INPUTS:          True or false
// 
// RETURNS:         Reference to 'this' object
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                26/08/98    1.0     Origin
// NT ALMOND                26/08/99    1.2        Added flexbility of
//                                                Sending Click meessage to parent
//
//////////////////////////////////////////////////////////////////////////

CStaticEx& CStaticEx::SetLink( BOOL bLink, LinkStyle style, HWND hWndNotify, UINT nMsgNotify )
{
    if (bLink) {
        m_LinkStyle = style;
    }
    else {
        m_LinkStyle = LinkNone;
    }

    if (hWndNotify && nMsgNotify) {
        m_hWndNotify = hWndNotify;
        m_nMsgNotify = nMsgNotify;
    }
    else {
        m_hWndNotify = 0;
        m_nMsgNotify = 0;
    }

    if (m_LinkStyle != LinkNone) {
        ModifyStyle(0, SS_NOTIFY);
    } 
    else {
        ModifyStyle(SS_NOTIFY, 0);
    }

    return *this;
}

void CStaticEx::LoadLinkCursor( )
{
    if (m_hLinkCursor != NULL) { // No cursor handle - try to load one    
        return;
    }

    // First try to load the Win98 / Windows 2000 hand cursor
    m_hLinkCursor = ::LoadCursor(NULL, IDC_HAND);
    if (m_hLinkCursor != NULL) {
        TRACE(_T("CStaticEx@LoadLinkCursor: loading from IDC_HAND\r\n"));
        return;
    }

    // Still no cursor handle - load the WinHelp hand cursor

    // The following appeared in Paul DiLascia's Jan 1998 MSJ articles.
    // It loads a "hand" cursor from the winhlp32.exe module.

    TRACE(_T("CStaticEx@LoadLinkCursor: loading from winhlp32\n"));

    // Get the windows directory
    CString strWndDir;
    GetWindowsDirectory(strWndDir.GetBuffer(MAX_PATH), MAX_PATH);
    strWndDir.ReleaseBuffer();

    strWndDir += _T("\\winhlp32.exe");

    // This retrieves cursor #106 from winhlp32.exe, which is a hand pointer
    HMODULE hModule = LoadLibrary(strWndDir);
    if (hModule) {
        HCURSOR hHandCursor = ::LoadCursor(hModule, MAKEINTRESOURCE(106));
        if (hHandCursor) {
            m_hLinkCursor = CopyCursor(hHandCursor);
        }
        FreeLibrary(hModule);
    }
}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::SetTransparent
//
// Description:        Sets the StaticEx window to be transpaent
//
// INPUTS:          True or false
// 
// RETURNS:         Reference to 'this' object
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                22/10/98    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
CStaticEx& CStaticEx::SetTransparent(BOOL bSet)
{

    m_bTransparent = bSet;
    ModifyStyleEx(0,WS_EX_TRANSPARENT); // Fix for transparency
    UpdateSurface();

    return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::SetFont3D
//
// Description:        Sets the 3D attribute of the font.
//
// INPUTS:          True or false, Raised or Sunken
// 
// RETURNS:         Reference to 'this' object
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                22/10/98    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
CStaticEx& CStaticEx::SetFont3D(BOOL bSet,Type3D type)
{

    m_bFont3d = bSet;
    m_3dType = type;
    UpdateSurface();


    return *this;
}

void CStaticEx::OnSysColorChange() 
{

    if (m_hwndBrush)
        ::DeleteObject(m_hwndBrush);

    m_hwndBrush = ::CreateSolidBrush(GetSysColor(COLOR_3DFACE));

    UpdateSurface();


}



//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::SetRotationAngle
//
// Description:        Sets the rotation angle for the current font.
//
// INPUTS:          Angle in Degress
// 
// RETURNS:         Reference to 'this' object
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                22/10/98    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
CStaticEx& CStaticEx::SetRotationAngle(UINT nAngle,BOOL bRotation)
{
    // Arrrrh...
    // Your looking in here why the font is rotating, aren't you?
    // Well try setting the font name to 'Arial' or 'Times New Roman'
    // Make the Angle 180 and set bRotation to true.
    //
    // Font rotation _ONLY_ works with TrueType fonts...
    //
    // 
    m_lf.lfEscapement = m_lf.lfOrientation = (nAngle * 10);
    m_bRotation = bRotation;

    ReconstructFont();

    UpdateSurface();


    return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::SetText3DHiliteColor
//
// Description:        Sets the 3D font hilite color
//
// INPUTS:          Color 
// 
// RETURNS:         Reference to 'this' object
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                17/07/00    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
CStaticEx& CStaticEx::SetText3DHiliteColor(COLORREF cr3DHiliteColor)
{
    m_cr3DHiliteColor = cr3DHiliteColor;
    UpdateSurface();


    return *this;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::PreSubclassWindow
//
// Description:        Assigns default dialog font
//
// INPUTS:          
// 
// RETURNS:         
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                15092000    1.5     Origin
// NT ALMOND                02072002    1.6     Fix crash when GetFont returns NULL
//////////////////////////////////////////////////////////////////////////
void CStaticEx::PreSubclassWindow() 
{

    CStatic::PreSubclassWindow();

    CFont* cf = GetFont();
    if(cf !=NULL)
    {
        cf->GetObject(sizeof(m_lf),&m_lf);
    }
    else
    {
        GetObject(GetStockObject(SYSTEM_FONT),sizeof(m_lf),&m_lf);
    }

    LoadLinkCursor();

    ReconstructFont();

}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::PreCreateWindow
//
// Description:        
//
// INPUTS:          
// 
// RETURNS:         
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                15092000    1.5     Origin
//////////////////////////////////////////////////////////////////////////
BOOL CStaticEx::PreCreateWindow(CREATESTRUCT& cs) 
{    
    return CStatic::PreCreateWindow(cs);
}

//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::DrawGradientFill
//
// Description:        Internal help function to gradient fill background
//
// INPUTS:          
// 
// RETURNS:         
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                15092000    1.5     Origin
//////////////////////////////////////////////////////////////////////////
void CStaticEx::DrawGradientFill(CDC* pDC, CRect* pRect, COLORREF crStart, COLORREF crEnd, int nSegments)
{
    // Get the starting RGB values and calculate the incremental
    // changes to be applied.

    COLORREF cr;
    int nR = GetRValue(crStart);
    int nG = GetGValue(crStart);
    int nB = GetBValue(crStart);

    int neB = GetBValue(crEnd);
    int neG = GetGValue(crEnd);
    int neR = GetRValue(crEnd);

    if(nSegments > pRect->Width())
        nSegments = pRect->Width();

    int nDiffR = (neR - nR);
    int nDiffG = (neG - nG);
    int nDiffB = (neB - nB);

    int ndR = 256 * (nDiffR) / (max(nSegments,1));
    int ndG = 256 * (nDiffG) / (max(nSegments,1));
    int ndB = 256 * (nDiffB) / (max(nSegments,1));

    nR *= 256;
    nG *= 256;
    nB *= 256;

    neR *= 256;
    neG *= 256;
    neB *= 256;

    int nCX = pRect->Width() / max(nSegments,1), nLeft = pRect->left, nRight;
    pDC->SelectStockObject(NULL_PEN);

    for (int i = 0; i < nSegments; i++, nR += ndR, nG += ndG, nB += ndB)
    {
        // Use special code for the last segment to avoid any problems
        // with integer division.

        if (i == (nSegments - 1))
            nRight = pRect->right;
        else
            nRight = nLeft + nCX;

        cr = RGB(nR / 256, nG / 256, nB / 256);

        {
            CBrush br(cr);
            CBrush* pbrOld = pDC->SelectObject(&br);
            pDC->Rectangle(nLeft, pRect->top, nRight + 1, pRect->bottom);
            pDC->SelectObject(pbrOld);
        }

        // Reset the left side of the drawing rectangle.

        nLeft = nRight;
    }
}


//////////////////////////////////////////////////////////////////////////
//
// Function:        CStaticEx::SetFont
//
// Description:        Sets font with LOGFONT structure
//
// INPUTS:          
// 
// RETURNS:         
//
// NOTES:            
// 
// MODIFICATIONS:
//
// Name                     Date        Version Comments
// NT ALMOND                02072002    1.6     Origin
//////////////////////////////////////////////////////////////////////////
CStaticEx& CStaticEx::SetFont(LOGFONT lf)
{
    CopyMemory(&m_lf, &lf, sizeof(m_lf));
    ReconstructFont();
    UpdateSurface();
    return *this;

}

BOOL CStaticEx::OnEraseBkgnd(CDC* pDC) 
{
    // TODO: Add your message handler code here and/or call default

    return TRUE;
}
