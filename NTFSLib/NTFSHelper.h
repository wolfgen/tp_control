#pragma once

#include "D:\SVN\json\single_include\nlohmann\json.hpp"

namespace NTFSHelper
{
	DWORD SelectHostFile(CString strDrive, CString& strFileName);

	DWORD ReadMFTRecord(CString strFileName, BYTE* pRecord);
	DWORD WriteMFTRecord(CString strFileName, BYTE* pRecord);


	DWORD AppendFileConent(CString strSrcFile, CString strDstFile, HWND hProgress = NULL);
	DWORD ExtractFileConent(CString strSrcFile, CString strDstFile);
	DWORD RemoveFileContent(CString strSrcFile);

	DWORD TurncateFile(CString strSrcFile, ULONGLONG ullOffset);



	DWORD SetExtendInfo(CString strSrcFile, CString strHiddenFile, CString strMFT);
	DWORD GetExtendInfo(CString strSrcFile, nlohmann::json& j);

	DWORD GetFileAddress(CString strFileName, ULONGLONG& ullRef);
}