/********************************************************************
	created:	2008/07/22
	created:	22:7:2008   10:25
	filename: 	SelectDialog.cpp
	file base:	SelectDialog
	file ext:	cpp
	author:		Hojjat Bohlooli - software@tarhafarinin.ir
	
	purpose:	
*********************************************************************/
#include "stdafx.h"
#include "SelectDialog.h"

#define WM_QUERY_SELECTED_PATH		WM_USER + 1



#pragma warning( push )
#pragma warning( disable : 4311 4312 )
// CSelectDialog
CString CSelectDialog::m_strCurrentDirectory;
CStringArray CSelectDialog::m_SelectedItemList;
WNDPROC CSelectDialog::m_wndProc = NULL;

IMPLEMENT_DYNAMIC(CSelectDialog, CFileDialog)

CSelectDialog::CSelectDialog(BOOL bOpenFileDialog,
							 LPCTSTR lpszDefExt,
							 LPCTSTR lpszFileName,
							 DWORD dwFlags,
							 LPCTSTR lpszFilter,
							 CWnd* pParentWnd)
							 :
				CFileDialog(
							 bOpenFileDialog,
							 lpszDefExt,
							 lpszFileName,
							 dwFlags | OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_HIDEREADONLY & (~OFN_SHOWHELP),
							 lpszFilter,
							 pParentWnd,
							 0,
							 0)
{
	dwFlags |= (OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_HIDEREADONLY & (~OFN_SHOWHELP));
};

CSelectDialog::~CSelectDialog()
{
};

BEGIN_MESSAGE_MAP(CSelectDialog, CFileDialog)
END_MESSAGE_MAP()

// CSelectDialog message handlers
BOOL CSelectDialog::OnFileNameOK()
{
	if (CFileDialog* pDlg = (CFileDialog*)CWnd::FromHandle(GetParent()->m_hWnd))
	{
		CWnd* pWnd = pDlg->GetDlgItem(lst2);	//getting list
		if (pWnd == NULL)
			return FALSE;

		m_SelectedItemList.RemoveAll();			// emptying list
		
		CListCtrl* wndLst1 = (CListCtrl*)(pWnd->GetDlgItem(1));

		int nSelected = wndLst1->GetSelectedCount();
		if (!nSelected)		// nothing selected -- don't retrieve list
			return FALSE;
		CString strItemText, strDirectory = GetPathName();

		if ((GetFileAttributes(strDirectory) & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
		{
			PathRemoveFileSpec(strDirectory.GetBuffer(MAX_PATH));
			strDirectory.ReleaseBuffer();
		}
		
		PathAddBackslash(strDirectory.GetBuffer(MAX_PATH));
		strDirectory.ReleaseBuffer();

		CString fileslist = _T("");
		pDlg->SendMessage(CDM_GETSPEC, (WPARAM)MAX_PATH,
			(LPARAM)fileslist.GetBuffer(MAX_PATH));
		fileslist.ReleaseBuffer();

		strItemText = strDirectory + fileslist;
		if(nSelected == 1 && fileslist != _T(""))
		{
			m_SelectedItemList.Add(strItemText);
			return CFileDialog::OnFileNameOK();
		}
	}
	::MessageBeep( MB_ICONQUESTION );
	return 1; //don't let the dialog to close
};

void CSelectDialog::OnFolderChange()
{
	m_strCurrentDirectory = GetFolderPath();
	CFileDialog::OnFolderChange();

};

void CSelectDialog::OnInitDone()
{

	m_strCurrentDirectory = GetFolderPath();
	CWnd* pFD = GetParent();


	HideControl(edt1);
	HideControl(cmb1);
	HideControl(stc2);

	HideControl(cmb13);
	HideControl(stc3);

	CRect rectCancel; pFD->GetDlgItem(IDCANCEL)->GetWindowRect(&rectCancel);
	pFD->ScreenToClient(&rectCancel);

	CRect rectOK; pFD->GetDlgItem(IDOK)->GetWindowRect(&rectOK);
	pFD->ScreenToClient(&rectOK);
	pFD->GetDlgItem(IDOK)->SetWindowPos(0,rectCancel.left - rectOK.Width() - 5, rectCancel.top, 0,0, SWP_NOZORDER | SWP_NOSIZE);

	CRect rectList2; pFD->GetDlgItem(lst1)->GetWindowRect(&rectList2);
	pFD->ScreenToClient(&rectList2);
	pFD->GetDlgItem(lst1)->SetWindowPos(0,0,0,rectList2.Width(), abs(rectList2.top - (rectCancel.top - 5)), SWP_NOMOVE | SWP_NOZORDER);

	CRect rectStatic;pFD->GetDlgItem(stc3)->GetWindowRect(&rectStatic);
	pFD->ScreenToClient(&rectStatic);
	pFD->GetDlgItem(stc3)->SetWindowPos(0,rectCancel.left - 375,rectCancel.top + 5, rectStatic.Width(), rectStatic.Height(), SWP_NOZORDER);

	CRect rectEdit1;pFD->GetDlgItem(cmb13)->GetWindowRect(&rectEdit1);
	pFD->ScreenToClient(&rectEdit1);
	pFD->GetDlgItem(cmb13)->SetWindowPos(0,rectCancel.left - 320,rectCancel.top, rectEdit1.Width() - 15, rectEdit1.Height(), SWP_NOZORDER);

	SetControlText(stc3, _T("Item name:"));
	SetControlText(IDOK, _T("ѡ��"));
	
	m_wndProc = (WNDPROC)::SetWindowLong(pFD->m_hWnd, GWL_WNDPROC, (long)WindowProcNew);
	pFD->CenterWindow();

	CToolBar* pToolBar = (CToolBar*)(pFD->GetDlgItem(ctl1));
	pToolBar->ShowWindow(SW_HIDE);
	pToolBar-> GetToolBarCtrl().DeleteButton(0);


};

LRESULT CALLBACK CSelectDialog::WindowProcNew(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	if (message ==  WM_COMMAND)
	{
		if (HIWORD(wParam) == BN_CLICKED)
		{
			if (LOWORD(wParam) == IDOK)
			{
				if (CFileDialog* pDlg = (CFileDialog*)CWnd::FromHandle(hwnd))
				{
					m_SelectedItemList.RemoveAll();			// emptying list

					CWnd* pWnd = pDlg->GetDlgItem(lst2);	//getting list

					if (pWnd == NULL)
						return FALSE;

					CListCtrl* wndLst1 = (CListCtrl*)(pWnd->GetDlgItem(1));

					int nSelected = wndLst1->GetSelectedCount();

					if (!nSelected)		// nothing selected
					{
						nSelected = 1;
						//return FALSE;
					}

					CString strItemText;

					int nItem = wndLst1->GetNextItem(-1,LVNI_SELECTED);

					CString fileslist = _T("");

					pDlg->SendMessage(CDM_GETSPEC, (WPARAM)(MAX_PATH * MAX_PATH), (LPARAM)fileslist.GetBuffer(MAX_PATH * MAX_PATH));

					fileslist.ReleaseBuffer();

					//////////////////   Add directory names to list
					while((nSelected--) > 0)
					{
						strItemText = wndLst1->GetItemText(nItem,0);

						strItemText = m_strCurrentDirectory + strItemText;

						DWORD attr = GetFileAttributes(strItemText);

						if((attr != 0xFFFFFFFF) && (attr & FILE_ATTRIBUTE_DIRECTORY))
							m_SelectedItemList.Add(strItemText);

						nItem = wndLst1->GetNextItem(nItem, LVNI_SELECTED);
					}

					//////////////////   Add FILE names to list
					strItemText = _T("");

					nSelected = wndLst1->GetSelectedCount();

					if(nSelected > m_SelectedItemList.GetCount())
					{
						int MoreThanOnFile = fileslist.Find(_T("\""));
						if(MoreThanOnFile != -1)
						{
							for(int i=0; i<fileslist.GetLength(); i++)
								if(fileslist[i] != '\"')
								{
									strItemText.AppendFormat(_T("%c"),fileslist[i]);
									if(fileslist[i-1] == '\"' && fileslist[i] == ' ')
										strItemText.Delete(strItemText.GetLength()-1);
								}
								else if(!strItemText.IsEmpty())
								{
									m_SelectedItemList.Add((m_strCurrentDirectory + strItemText));
									strItemText.Empty();
								}
						}
						else
							m_SelectedItemList.Add(m_strCurrentDirectory + fileslist);
					}
					pDlg->EndDialog(IDOK);

					return NULL;
				} // if IDOK
			}
		} // if BN_CLICKED
	}// if WM_COMMAND
	return CallWindowProc(m_wndProc, hwnd, message, wParam, lParam);
}

#pragma warning( pop )

void CSelectDialog::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class

	CFileDialog::OnOK();
}


LRESULT CSelectDialog::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: Add your specialized code here and/or call the base class

	switch (message)
	{
	case WM_QUERY_SELECTED_PATH:

		*((CString*)wParam) = GetPathName();

		break;

	default:
		break;
	}
	return CFileDialog::DefWindowProc(message, wParam, lParam);
}


BOOL CSelectDialog::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// TODO: Add your specialized code here and/or call the base class

	OFNOTIFY* pOFN = (OFNOTIFY*)lParam;

	if (pOFN->hdr.code == CDN_SELCHANGE)
	{
		// Selection changed
		// Get the filename
		CString strCurrentDirectory;

		GetParent()->SendMessage(CDM_GETFILEPATH, MAX_PATH, (LPARAM)(strCurrentDirectory.GetBuffer(MAX_PATH)));
		PathRemoveFileSpec(strCurrentDirectory.GetBuffer(MAX_PATH));
		PathAddBackslash(strCurrentDirectory.GetBuffer(MAX_PATH));
		strCurrentDirectory.ReleaseBuffer();

		if (strCurrentDirectory != _T(""))
			m_strCurrentDirectory = strCurrentDirectory;

	}

	return CFileDialog::OnNotify(wParam, lParam, pResult);
}
