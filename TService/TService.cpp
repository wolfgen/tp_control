/******************************************************************************/
/*
/*    NT Service Class
/*
/*    copyright 2003  , Stefan Voitel - Berlin / Germany
/*
/*    stefan.voitel@winways.de
/******************************************************************************/
#include "pch.h"
#include "TService.h"
#include <filesystem>

namespace ModuleTService {
    TService* Service; //
};

// -----------------------------------------------------------------------------

#pragma warning (disable : 100)
#if defined (__BORLANDC__)
#pragma argsused
#endif



void WINAPI TService::ServiceMain (DWORD argc, TCHAR* argv[])
{
	//_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);

    using namespace ModuleTService;

	
	//_CrtSetBreakAlloc(193);

    Service->m_StatusHandle = RegisterServiceCtrlHandlerEx(Service->GetName(), ServiceHandler, NULL);

    if (Service->succeeded (Service->m_StatusHandle != NULL)) {
        Service->SetStatus (SERVICE_START_PENDING);
        Service->m_hDevNotify = Service->RegisterDeviceNotifyHandler(Service->m_StatusHandle);
        if (Service->Init(argc, argv)) {
            Service->SetStatus (SERVICE_RUNNING);
            Service->ServiceProc();
            Service->SetStatus (SERVICE_STOP_PENDING);
            Service->Cleanup();
            Service->SetStatus (SERVICE_STOPPED);
        } else {
            Service->SetStatus (SERVICE_STOPPED);
        }
    } else {
        Service->SetStatus (SERVICE_STOPPED);
    }
}

#pragma warning (default : 100)

// -----------------------------------------------------------------------------

DWORD WINAPI TService::ServiceHandler (DWORD control, DWORD evtype, PVOID evdata, PVOID Context)
{
    DWORD dwError = ERROR_SUCCESS;

    using namespace ModuleTService;

    switch (control) {
        case SERVICE_CONTROL_INTERROGATE:
            Service->SetStatus (Service->m_StatusCode);
            break;

        case SERVICE_CONTROL_PAUSE:
            break;

        case SERVICE_CONTROL_CONTINUE:
            break;

        case SERVICE_CONTROL_DEVICEEVENT:
            dwError = Service->DeviceEventNotify(evtype, evdata);
            break;

        case SERVICE_CONTROL_STOP:
			dwError = Service->StopEvent();
			break;

        case SERVICE_CONTROL_SHUTDOWN:
            dwError = Service->ShutdownEvent();
			break;

/*
            Service->SetStatus (SERVICE_STOP_PENDING);
            if (Service->m_hDevNotify) {
                UnregisterDeviceNotification(Service->m_hDevNotify);
            }            
            Service->m_Terminated = true;
            break;*/

		case SERVICE_CONTROL_SESSIONCHANGE:
            dwError = Service->SessionChangeNotify(evtype, evdata);
			break;
    }

    return dwError;
}

// -----------------------------------------------------------------------------

BOOL WINAPI TService::ConsoleHandler (DWORD dwCtrlType) 
{
    using namespace ModuleTService;

    if (dwCtrlType == CTRL_LOGOFF_EVENT) {
        Service->LogoffEvent();
    } else {
        Service->ShutdownEvent();
    }

    return TRUE;
}

// -----------------------------------------------------------------------------

TService::TService() 
    : m_StatusHandle(NULL)
    , m_StatusCode(SERVICE_STOPPED)
    , m_Terminated(false)
    , m_hDevNotify(NULL)
{

    ModuleTService::Service = this;

    ZeroMemory (m_ErrorString,sizeof(m_ErrorString));
    succeeded(SetConsoleCtrlHandler (ConsoleHandler,TRUE));
}

// -----------------------------------------------------------------------------

TService::~TService() 
{
    if (m_EventLog) {
        DeregisterEventSource (m_EventLog);
    }
}

// -----------------------------------------------------------------------------

bool TService::shopen (tSvcHandle &hService) 
{
    bool success = false;
    tSvcHandle	hSCM;

    if ((hSCM = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS)) != NULL)	{
        hService = OpenService(hSCM,GetName(),SERVICE_ALL_ACCESS);
        success = succeeded (hService != NULL);
        CloseServiceHandle (hSCM);
    }
    else {
        succeeded (false);
    }

    return success;
}

// -----------------------------------------------------------------------------

void TService::shclose (tSvcHandle hService) 
{
    CloseServiceHandle (hService);
}

// -----------------------------------------------------------------------------
void 	TService::SetStatus (DWORD status)
{
    m_StatusCode  = status;

    SERVICE_STATUS ss;
    ss.dwServiceType              = SERVICE_WIN32_OWN_PROCESS;
    ss.dwCurrentState             = m_StatusCode;
    ss.dwControlsAccepted	      = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_SESSIONCHANGE;
    ss.dwWin32ExitCode            = NOERROR;
    ss.dwServiceSpecificExitCode  = NOERROR;
    ss.dwCheckPoint               = 0;
    ss.dwWaitHint                 = 3000;

    if (! succeeded (SetServiceStatus (m_StatusHandle,&ss))) {
        LogEvent (m_ErrorString,evWarning);
    }
}

// -----------------------------------------------------------------------------

bool TService::succeeded (BOOL success) 
{
    DWORD SysError = success ? ERROR_SUCCESS : GetLastError();

    FormatMessage ( FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    SysError,
                    MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                    m_ErrorString,
                    sizeof(m_ErrorString),
                    NULL );

    return SysError == ERROR_SUCCESS;
}

// -----------------------------------------------------------------------------

bool TService::Execute () 
{
    m_EventLog = RegisterEventSource (NULL,GetName());

    SERVICE_TABLE_ENTRY entries[2];
    entries[0].lpServiceName = (TCHAR*) GetName();
    entries[0].lpServiceProc = ServiceMain;
    entries[1].lpServiceName = NULL;
    entries[1].lpServiceProc = NULL;

    bool  success = succeeded (StartServiceCtrlDispatcher(entries));
    if (! success) {
        LogEvent (m_ErrorString,evError);
    }

    return success;
}

// -----------------------------------------------------------------------------

bool TService::ConsoleMode (void)
{
    m_EventLog = RegisterEventSource(NULL,GetName());

    bool success = false;
    if (Init(0, nullptr))	{
        ServiceProc();
        Cleanup ();
        success = true;
    }

    return success;
}

// -----------------------------------------------------------------------------

bool TService::Start()
{
    bool         success = false;
    tSvcHandle   hService;


    if (shopen (hService)) {
        success = succeeded (StartService (hService,0,NULL));
        shclose (hService);
    }

    return success;
}

// -----------------------------------------------------------------------------

bool TService::Stop() 
{
    bool success = false;
    tSvcHandle hService;

    if (shopen (hService)) {
        SERVICE_STATUS   state;
        success = succeeded (ControlService (hService,SERVICE_CONTROL_STOP,&state));
        shclose (hService);
    }

    return success;
}

// -----------------------------------------------------------------------------

bool TService::Remove()
{
    Stop();

    bool success = false;
    tSvcHandle hService;

    if (shopen (hService)) {
        success = succeeded (DeleteService (hService));
        shclose (hService);

        HKEY hKey;
        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,REG_EVENTLOG,0,KEY_ALL_ACCESS,&hKey) == ERROR_SUCCESS) {
            RegDeleteKey (hKey,GetName());
            RegCloseKey (hKey);
        }
    }

    return success;
}

// -----------------------------------------------------------------------------

bool TService::Install(DWORD dwStartType)
{
    bool      success = false;
    SC_HANDLE hService,hSCManager;
    TCHAR     imagePath[MAX_PATH];

    ::GetModuleFileName(NULL, imagePath, MAX_PATH);
    hSCManager = ::OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    if (succeeded (hSCManager != NULL)) {
		hService = ::CreateService(hSCManager,
								   GetName(),
								   GetDisplayName(),
								   SERVICE_ALL_ACCESS,
								   SERVICE_WIN32_OWN_PROCESS,
								   dwStartType,
								   SERVICE_ERROR_NORMAL,
								   imagePath,
								   NULL,
								   NULL,
								   NULL,
								   NULL,
								   NULL);

        if (succeeded (hService != NULL)) {
            ::CloseServiceHandle (hService);
            success = true;
        }
        
        ::CloseServiceHandle (hSCManager);
    }

    if (success) {
        UINT f = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;

        TCHAR szKey [MAX_PATH];
        wsprintf (szKey, _T("%s\\%s"), REG_EVENTLOG, GetName());

        HKEY hKey;
        if (RegCreateKey (HKEY_LOCAL_MACHINE, szKey, &hKey) == ERROR_SUCCESS) {
            TCHAR  mod[MAX_PATH];
            DWORD  len = ::GetModuleFileName (NULL, mod, MAX_PATH);

            RegSetValueEx(hKey, _T("TypesSupported"),   0, REG_DWORD, (BYTE*)&f,  sizeof(DWORD));
            RegSetValueEx(hKey, _T("EventMessageFile"), 0, REG_SZ,    (BYTE*)mod, len+1);
            RegCloseKey (hKey);
        }
    }

    return success;
}

// -----------------------------------------------------------------------------

bool TService::Help (DWORD context) 
{
    TCHAR file[MAX_PATH + 6], *p, *EndPtr;

    EndPtr = &file[MAX_PATH + 5];

    ::GetModuleFileName (NULL, file, MAX_PATH);

    if ((p = _tcsrchr (file, _T('.'))) != NULL) {
        _tcscpy_s(p, EndPtr - p, _T(".hlp"));
    }

    return WinHelp (NULL, file, context ? HELP_CONTEXT : HELP_CONTENTS, context) != 0;
}

// -----------------------------------------------------------------------------

void TService::PrintLastError (const TCHAR *cap)
{
    DWORD n;

    if (cap != NULL && *cap) {
        WriteConsole(GetStdHandle(STD_ERROR_HANDLE), _T("\r\n"), 2, &n, NULL);
        WriteConsole(GetStdHandle(STD_ERROR_HANDLE), cap, (DWORD)_tcslen(cap), &n, NULL);
    }

    char oem[sizeof(m_ErrorString)];
    ZeroMemory (oem,sizeof(oem));

    CharToOemBuff(m_ErrorString, oem, (DWORD)_tcslen(m_ErrorString));
    WriteConsole(GetStdHandle(STD_ERROR_HANDLE), _T("\r\n"), 2, &n, NULL);
    WriteConsole(GetStdHandle(STD_ERROR_HANDLE), oem, (DWORD)strlen(oem), &n, NULL);
}

// -----------------------------------------------------------------------------

void TService::LogEvent (const TCHAR* str, evLogType type, WORD Category) 
{
    if (m_EventLog != NULL) {
        const TCHAR* msgs[1];
        msgs[0] = str;
        ReportEvent (m_EventLog,(WORD) type, Category, 0, NULL, 1, 0, msgs, NULL);
    }
}

DWORD TService::GetInstallPath(std::filesystem::path& pathInstallPath)
{
    DWORD dwErrCode = ERROR_SUCCESS;

    LPQUERY_SERVICE_CONFIG lpServiceConfig = NULL;

	tSvcHandle hService = NULL;

	if (shopen(hService)) 
    {
		DWORD dwBytesNeeded = 0, dwBufSize = 0;

		if (!QueryServiceConfig(hService, NULL, 0, &dwBytesNeeded))
		{
			dwErrCode = GetLastError();

			if (ERROR_INSUFFICIENT_BUFFER == dwErrCode)
			{
                dwErrCode = ERROR_SUCCESS;

				dwBufSize = dwBytesNeeded;

                lpServiceConfig = (LPQUERY_SERVICE_CONFIG)HeapAlloc(GetProcessHeap(), 0, dwBufSize);
			}
			else
			{
                goto err_handler;
			}
		}

		if (QueryServiceConfig(hService, lpServiceConfig, dwBufSize, &dwBytesNeeded))
		{         
            pathInstallPath = (std::filesystem::path(lpServiceConfig->lpBinaryPathName)).parent_path();
		}
	}
    else
    {
        dwErrCode = GetLastError();
    }

err_handler:

    if (lpServiceConfig)
    HeapFree(GetProcessHeap(), 0, lpServiceConfig);


    if (hService)
        shclose(hService);

    return dwErrCode;
}

// -----------------------------------------------------------------------------

bool TService::SetConfigValue (TCHAR* key, BYTE *b, DWORD n, cfValType t) 
{
    TCHAR   RegPath[255];
    HKEY    hk;
    DWORD   disp;
    bool    success = false;

    wsprintf (RegPath, REG_CONFIG,GetName());

    LSTATUS lStatus = RegCreateKeyEx( HKEY_LOCAL_MACHINE, 
                                      RegPath,
                                      0, 
                                      NULL,
                                      REG_OPTION_NON_VOLATILE,
                                      KEY_ALL_ACCESS, 
                                      NULL, 
                                      &hk, 
                                      &disp );

    if (succeeded (lStatus == ERROR_SUCCESS)) {
        ZeroMemory (b,n);
        lStatus = RegSetValueEx (hk, key, 0,(DWORD)t, b, n);
        success = succeeded (lStatus == ERROR_SUCCESS);
        RegCloseKey (hk);
    }
    return success;
}

// -----------------------------------------------------------------------------

bool TService::GetConfigValue (TCHAR* key, BYTE *b, DWORD *n, cfValType *t) 
{
    TCHAR   RegPath[255];
    HKEY    hk;
    DWORD   disp;
    bool    success = false;
    LSTATUS lStatus = ERROR_SUCCESS;

    wsprintf (RegPath, REG_CONFIG, GetName());

    lStatus = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                              RegPath,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hk,
                              &disp );

    if (succeeded (lStatus == ERROR_SUCCESS)) {
        ZeroMemory (b,*n);
        lStatus = RegQueryValueEx (hk, key, 0, (DWORD*)t, b, n);
        success = succeeded(lStatus == ERROR_SUCCESS);
        RegCloseKey (hk);
    }

    return success;
}

// -----------------------------------------------------------------------------
// EOF

