/******************************************************************************/
/*
/*    NT Service Class
/*
/*    copyright 2003  , Stefan Voitel - Berlin / Germany
/*
/*    stefan.voitel@winways.de	
/******************************************************************************/

#ifndef SERVICE_CLASS_INCLUDED
#define SERVICE_CLASS_INCLUDED

#include "tchar.h"

#include <windows.h>
#include <winsvc.h>

#define REG_CONFIG   _T("SYSTEM\\CurrentControlSet\\Services\\%s\\ServiceConfig")
#define REG_EVENTLOG _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\System")

class TService {
protected:
    TService();
    ~TService();

    bool Execute(void);
    bool ConsoleMode(void);

    bool Start(void);
    bool Stop(void);
    bool Install(DWORD dwStartType= SERVICE_DEMAND_START);
    bool Remove(void);
    virtual bool Help(DWORD context = 0);

    bool Terminated(void) 
    {
        return m_Terminated;
    }

    DWORD Terminate(void) 
    {
        if (m_hDevNotify) {
            ::UnregisterDeviceNotification(m_hDevNotify);
        }
        m_Terminated = true;

        return ERROR_SUCCESS;
    }


    const TCHAR *LastError(void) const 
    {
        return m_ErrorString;
    }

    void PrintLastError(const TCHAR *Caption = NULL);

    enum cfValType {
        cfBinary  = REG_BINARY,
        cfDword   = REG_DWORD,
        cfString  = REG_SZ
    };

    bool SetConfigValue (TCHAR* key, BYTE *val, DWORD nval,   cfValType  t = cfString);
    bool GetConfigValue (TCHAR* key, BYTE *buf, DWORD *nbuff, cfValType *t);

    enum evLogType  {
        evError   = EVENTLOG_ERROR_TYPE,
        evWarning = EVENTLOG_WARNING_TYPE,
        evInfo    = EVENTLOG_INFORMATION_TYPE
    };

    void LogEvent       (const TCHAR* e, evLogType t = evInfo, WORD cat = 0);

    virtual const TCHAR*   GetName        (void) = 0;
    virtual void           ServiceProc    (void) = 0;

    virtual const TCHAR* GetDisplayName(void) 
    {
        return GetName();
    }

    DWORD GetInstallPath(std::filesystem::path& pathInstallPath);

    virtual bool Init(DWORD argc, TCHAR* argv[])
    {
        return true;
    }

    virtual void Cleanup(void) 
    {
        return;
    }

    virtual void LogoffEvent(void) 
    {
        return;
    }

    virtual DWORD StopEvent(void)
    {
        return Terminate();
    }

	virtual DWORD ShutdownEvent(void)
	{
		return Terminate();
	}

    virtual HDEVNOTIFY RegisterDeviceNotifyHandler(SERVICE_STATUS_HANDLE hServiceStatus)
    {
        return NULL;
    }

    virtual DWORD DeviceEventNotify(DWORD evtype, PVOID evdata) 
    { 
        return ERROR_SUCCESS;
    }

	virtual DWORD SessionChangeNotify(DWORD evtype, PVOID evdata)
	{
		return ERROR_SUCCESS;
	}

protected:
    
    void SetAcceptStopeFlag()
    {
		SERVICE_STATUS ss;
		ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		ss.dwCurrentState = m_StatusCode;
		ss.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_SESSIONCHANGE;
		ss.dwWin32ExitCode = NOERROR;
		ss.dwServiceSpecificExitCode = NOERROR;
		ss.dwCheckPoint = 0;
		ss.dwWaitHint = 3000;

		if (!succeeded(SetServiceStatus(m_StatusHandle, &ss))) {
			LogEvent(m_ErrorString, evWarning);
		}
    }

	void ClearAcceptStopeFlag()
	{
		SERVICE_STATUS ss;
		ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		ss.dwCurrentState = m_StatusCode;
		ss.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_SESSIONCHANGE;
		ss.dwWin32ExitCode = NOERROR;
		ss.dwServiceSpecificExitCode = NOERROR;
		ss.dwCheckPoint = 0;
		ss.dwWaitHint = 3000;

		if (!succeeded(SetServiceStatus(m_StatusHandle, &ss))) {
			LogEvent(m_ErrorString, evWarning);
		}
	}

private:

    typedef SC_HANDLE tSvcHandle;

    SERVICE_STATUS_HANDLE  m_StatusHandle;
    HDEVNOTIFY             m_hDevNotify;
    DWORD                  m_StatusCode;
    HANDLE                 m_EventLog;
    bool                   m_Terminated;
    TCHAR                  m_ErrorString[512];

    bool shopen (tSvcHandle &hService);
    void shclose(tSvcHandle hService);
    bool succeeded(BOOL	ReturnValue);

    void SetStatus(DWORD status);
    static void  WINAPI    ServiceMain    (DWORD argc, TCHAR* argv[]);
    static DWORD WINAPI    ServiceHandler (DWORD control, DWORD evtype, PVOID evdata, PVOID Context);
    static BOOL  WINAPI    ConsoleHandler (DWORD dwCtrlType);
};

#endif // SERVICE_CLASS_INCLUDED
