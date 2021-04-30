/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: NT Serivce Pro 
 Build:created by octerboy 2007/01/11
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "textus_string.h"
#include <time.h>
#include <psapi.h>
#include <string.h>
#include <assert.h>
#pragma comment(lib,"advapi32.lib")

class NTSvc :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	NTSvc();
	~NTSvc();

	Amor::Pius local_pius;  //仅用于传回数据
	Amor::Pius end_pius;  //仅用于传回数据
	char service_name[2048];
	Amor *sch;
	Amor::Pius tmp_ps;
	void logEvent(WORD wType, DWORD dwID, const char* pszS1 = NULL,const char* pszS2 = NULL,const char* pszS3 = NULL);
	void setStatus(DWORD dwState);
	bool initialize();
	bool onInit();
 
	void OnStop();
	void OnInterrogate();
	void OnPause();
	void OnContinue();
	void OnShutdown();
	void saveStatus();    

	static	NTSvc* here_svc; // nasty hack to get object ptr

	HANDLE m_hEventSource;
	int m_iMajorVersion;
	int m_iMinorVersion;
	BYTE m_iStartParam, m_iIncParam, m_iState;
	SERVICE_STATUS_HANDLE m_hServiceStatus;
	SERVICE_STATUS m_Status;
	int argOffset;
	char appPath[_MAX_PATH];
	char event_file[_MAX_PATH];

	bool m_bIsRunning;
	void my_handle(DWORD dwOpcode);
	void my_main(DWORD dwArgc, LPTSTR* lpszArgv);

	bool parseStandardArgs(int argc, char* argv[]);
	bool uninstall();
	bool install(const char *cfg);
	bool isinstalled();
#include "wlog.h"
};
#define EVMSG_INSTALLED                  0x00000064L
#define EVMSG_REMOVED                    0x00000065L
#define EVMSG_NOTREMOVED                 0x00000066L
#define EVMSG_CTRLHANDLERNOTINSTALLED    0x00000067L
#define EVMSG_FAILEDINIT                 0x00000068L
#define EVMSG_STARTED                    0x00000069L
#define EVMSG_BADREQUEST                 0x0000006AL
#define EVMSG_DEBUG                      0x0000006BL
#define EVMSG_STOPPED                    0x0000006CL
#define NTS_HANDLER_FAILED		0x0000006DL

// static member functions
static void WINAPI serviceMain(DWORD dwArgc, LPTSTR* lpszArgv);
static void WINAPI handler(DWORD dwOpcode);
NTSvc *NTSvc::here_svc = (NTSvc*)0;

void NTSvc::ignite(TiXmlElement *cfg) { 
	const char *comm_str;
	char *p;
	char errmsg[64];

	if ( !cfg )
		return;
	if ( (comm_str = cfg->Attribute("service_name")) )
		TEXTUS_STRNCPY( service_name, comm_str, sizeof(service_name)-1);

	if ( (comm_str = cfg->Attribute("event_file")) )
		TEXTUS_STRNCPY(event_file , comm_str, sizeof(event_file)-1);
	else
		TEXTUS_STRNCPY(event_file , "tusEvent.dll", sizeof(event_file)-1);

	if ( (comm_str = cfg->Attribute("offset")) )
		argOffset = atoi(comm_str);
	if ( argOffset < 1 ) 
		argOffset =1;

	TEXTUS_STRCPY(appPath, cfg->GetDocument()->Value());

	p = &appPath[strlen(appPath)-1];
	while ( *p != '\\' && *p != '/'  && p != &appPath[0] ) p--;
	if (  *p == '\\' || *p == '/'  ) *p = 0;
    	if (strlen(appPath) > 0 ) {
		if( !SetCurrentDirectory(appPath))
		{
			TEXTUS_SPRINTF(errmsg, "SetCurrentDirectory failed (%d)", GetLastError());
			logEvent(EVENTLOG_ERROR_TYPE, EVMSG_FAILEDINIT, errmsg);
		}
	} else {
    		::GetModuleFileName(NULL, appPath, sizeof(appPath));
		p = &appPath[strlen(appPath)-1];
		while ( *p != '\\' && *p != '/'  && p != &appPath[0] ) p--;
		if (  *p == '\\' || *p == '/'  ) *p = 0;
	}
}

bool NTSvc::facio( Amor::Pius *pius)
{
	void **ps;
	BOOL ret;
	SERVICE_TABLE_ENTRY st[] = {
		{service_name, serviceMain},
		{NULL, NULL}
	};
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::MAIN_PARA:	/* 在整个系统中, 这应是最后被通知到的。 */
		WBUG("facio Notitia::MAIN_PARA");
		ps = (void**)pius->indic;
		if (parseStandardArgs( (*(int *)ps[0]) - argOffset, &((char**)ps[1])[argOffset]) ) break;
		goto SRV_START;

	case Notitia::WINMAIN_PARA:	/* 在整个系统中, 这应是最后被通知到的。 */
		WBUG("facio Notitia::WinMain_PARA");
	SRV_START:
    		WBUG("Calling StartServiceCtrlDispatcher(service_name(%s), serviceMain(%p))", st[0].lpServiceName, st[0].lpServiceProc);
		ret = StartServiceCtrlDispatcher(st);
		if ( !ret ) 
		{
			WLOG_OSERR("StartServiceCtrlDispatcher");
		}
		WBUG("Returned %d from StartServiceCtrlDispatcher()", ret);
		break;
	
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		tmp_ps.ordo = Notitia::CMD_GET_SCHED;
		tmp_ps.indic = 0;
		aptus->sponte(&tmp_ps);	//向tpoll, 取得sched
		sch = (Amor*)tmp_ps.indic;
		if ( !sch ) 
		{
			WLOG(ERR, "no sched or tpoll");
			break;
		}

		
	default:
		return false;
	}
	return true;
}

bool NTSvc::sponte( Amor::Pius *pius) { return false; }
Amor* NTSvc::clone()
{
	return  (Amor*)this;
}

NTSvc::NTSvc()
{
	memset(service_name, 0, sizeof(service_name));
    here_svc = this;
    m_iMajorVersion = 1;
    m_iMinorVersion = 0;
    m_hEventSource = NULL;

    // set up the initial service status 
    m_hServiceStatus = NULL;
    m_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_Status.dwCurrentState = SERVICE_STOPPED;
    m_Status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    m_Status.dwWin32ExitCode = 0;
    m_Status.dwServiceSpecificExitCode = 0;
    m_Status.dwCheckPoint = 0;
    m_Status.dwWaitHint = 0;
    m_bIsRunning = FALSE;
}

void NTSvc::my_main(DWORD dwArgc, LPTSTR* lpszArgv)
{   
	void *ps[3];
	Amor::Pius para;
	para.ordo = Notitia::MAIN_PARA;

	WBUG("%s has parameters %d ", service_name, dwArgc);
	for ( unsigned int i = 0 ; i < dwArgc ; i++)
	{
		WBUG("para[%d] %s ", i, lpszArgv[i]);
	}
	ps[0] = &dwArgc;
	ps[1] = lpszArgv;
	ps[2] = 0;
	para.indic = ps;
    WBUG("Entering NTSvc::ServiceMain()");
    // Register the control request handler
    m_Status.dwCurrentState = SERVICE_START_PENDING;
    m_hServiceStatus = RegisterServiceCtrlHandler(service_name,handler);
	if (m_hServiceStatus == NULL) {
		logEvent(EVENTLOG_ERROR_TYPE, NTS_HANDLER_FAILED);
		WLOG_OSERR("RegisterServiceCtrlHandler failed");
		return;
	}

    // Start the initialisation
    if (initialize()) {

        // Do the real work. 
        // When the Run function returns, the service has stopped.
        m_bIsRunning = TRUE;
        m_Status.dwWin32ExitCode = 0;
        m_Status.dwCheckPoint = 0;
        m_Status.dwWaitHint = 0;
	aptus->facio(&para);	
    }

    // Tell the service manager we are stopped
    setStatus(SERVICE_STOPPED);
//    WBUG("Leaving NTSvc::ServiceMain()");
}

///////////////////////////////////////////////////////////////////////////////////////
// Logging functions

// This function makes an entry into the application event log
void NTSvc::logEvent(WORD wType, DWORD dwID,
                          const char* pszS1,
                          const char* pszS2,
                          const char* pszS3)
{
#ifdef USE_LOG_EVENT
    const char* ps[3];
    ps[0] = pszS1;
    ps[1] = pszS2;
    ps[2] = pszS3;

    int iStr = 0;
    for (int i = 0; i < 3; i++) {
        if (ps[i] != NULL) iStr++;
    }
        
    // Check the event source has been registered and if
    // not then register it now
    if (!m_hEventSource) {
        m_hEventSource = ::RegisterEventSource(NULL,  // local machine
                                               service_name); // source name
    }

    if (m_hEventSource) {
        ::ReportEvent(m_hEventSource,
                      wType,
                      0,
                      dwID,
                      NULL, // sid
                      iStr,
                      0,
                      ps,
                      NULL);
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////
// Service initialization

bool NTSvc::initialize()
{
    WBUG("Entering NTSvc::initialize()");

    // Start the initialization
    setStatus(SERVICE_START_PENDING);
    
    // Perform the actual initialization
    bool bResult = onInit(); 
    
    // Set final state
    m_Status.dwWin32ExitCode = GetLastError();
    m_Status.dwCheckPoint = 0;
    m_Status.dwWaitHint = 0;
    if (!bResult) {
		logEvent(EVENTLOG_ERROR_TYPE, EVMSG_FAILEDINIT);
		WLOG_OSERR("service init failed")
		setStatus(SERVICE_STOPPED);
        return FALSE;    
    }
    
	logEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_STARTED, service_name, " started.");
	WLOG(INFO, "service %s started", service_name); 
    setStatus(SERVICE_RUNNING);
    return TRUE;
}

// callback service control manager
void WINAPI handler(DWORD dwOpcode)
{
	NTSvc::here_svc->my_handle(dwOpcode);
}

void WINAPI serviceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
	NTSvc::here_svc->my_main(dwArgc, lpszArgv);
}
   
void NTSvc::my_handle(DWORD dwOpcode)
{
    switch (dwOpcode) {
    case SERVICE_CONTROL_STOP: // 1
        setStatus(SERVICE_STOP_PENDING);
        OnStop();
        m_bIsRunning = FALSE;
        logEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_STOPPED, service_name, " stopped.");
	//WLOG(INFO, "service %s stopped", service_name);
        break;

    case SERVICE_CONTROL_PAUSE: // 2
        OnPause();
	WLOG(INFO, "service %s pause", service_name);
        break;

    case SERVICE_CONTROL_CONTINUE: // 3
        OnContinue();
	WLOG(INFO, "service %s continue", service_name);
        break;

    case SERVICE_CONTROL_INTERROGATE: // 4
        OnInterrogate();
	WLOG(INFO, "service %s INTERROGATE", service_name);
        break;

    case SERVICE_CONTROL_SHUTDOWN: // 5
	WLOG(INFO, "service %s shutdown", service_name);
        OnShutdown();
        break;

    default:
		WLOG(ERR, "bad request %lu", dwOpcode);
        break;
    }

    // Report current status
	WBUG("Updating status (%p, %d)", m_hServiceStatus, m_Status.dwCurrentState);
	::SetServiceStatus(m_hServiceStatus, &m_Status);
}
        
// Called when the service is first initialized
bool NTSvc::onInit()
{
	// Read the registry parameters
    // Try opening the registry key:
    // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\<AppName>\Parameters
	HKEY hkey;
	char szKey[1024];
        DWORD dwType = 0;
        DWORD dwSize = sizeof(m_iStartParam);
	LSTATUS lst;
	TEXTUS_SPRINTF(szKey, "SYSTEM\\CurrentControlSet\\services\\%s", service_name);

	WBUG("szkey %s", szKey);
	lst = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hkey);
	if ( lst == ERROR_SUCCESS) {
        // Yes we are installed
        RegQueryValueEx(hkey,
                        "Start",
                        NULL,
                        &dwType,
                        (BYTE*)&m_iStartParam,
                        &dwSize);
        dwSize = sizeof(m_iIncParam);
        RegQueryValueEx(hkey,
                        "Inc",
                        NULL,
                        &dwType,
                        (BYTE*)&m_iIncParam,
                        &dwSize);
        RegCloseKey(hkey);
	} else  {
		char *s;
		char error_string[1024];
		FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, lst, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) error_string, 1024, NULL );
		s= strstr(error_string, "\r\n") ;
		if (s )  *s = '\0';
		WLOG(ERR, " RegOpenKeyEx error %d %s", lst, error_string);
		return false;
	}

	// Set the initial state
	m_iState = m_iStartParam;
	return true;
}

// Called when the service control manager wants to stop the service
void NTSvc::OnStop()
{
	Amor::Pius tmp_pius;
	tmp_pius.indic = 0;
	tmp_pius.ordo = Notitia::CMD_MAIN_EXIT;
	sch->sponte(&tmp_pius);
}

// called when the service is interrogated
void NTSvc::OnInterrogate()
{
}

// called when the service is paused
void NTSvc::OnPause()
{
}

// called when the service is continued
void NTSvc::OnContinue()
{
}

// called when the service is shut down
void NTSvc::OnShutdown()
{
}


// Save the current status in the registry
void NTSvc::saveStatus()
{
    WBUG("Saving current status");
    // Try opening the registry key:
    // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\<AppName>\...
    HKEY hkey = NULL;
	char szKey[1024];
	TEXTUS_SPRINTF(szKey, "SYSTEM\\CurrentControlSet\\Services\\%s\\Status", service_name);
    DWORD dwDisp;
	DWORD dwErr;
    WBUG("Creating key: %s", szKey);
    dwErr = RegCreateKeyEx(	HKEY_LOCAL_MACHINE,
                           	szKey,
                   			0,
                   			"",
                   			REG_OPTION_NON_VOLATILE,
                   			KEY_WRITE,
                   			NULL,
                   			&hkey,
                   			&dwDisp);
	if (dwErr != ERROR_SUCCESS) {
		WBUG("Failed to create Status key (%lu)", dwErr);
		return;
	}	

    // Set the registry values
	WBUG("Saving 'Current' as %ld", m_iState); 
    RegSetValueEx(hkey,
                  "Current",
                  0,
                  REG_DWORD,
                  (BYTE*)&m_iState,
                  sizeof(m_iState));


    // Finished with key
    RegCloseKey(hkey);

}

void NTSvc::setStatus(DWORD dwState)
{
    m_Status.dwCurrentState = dwState;
    ::SetServiceStatus(m_hServiceStatus, &m_Status);
}

////////////////////////////////////////////////////////////////////////////////////////
// Default command line argument parsing

// Returns TRUE if it found an arg it recognised, FALSE if not
// Note: processing some arguments causes output to stdout to be generated.
bool NTSvc::parseStandardArgs(int argc, char* argv[])
{
    // See if we have any command line args we recognise
    if (argc <= 1) return FALSE;

    if (_stricmp(argv[1], "-v") == 0) {

        // Spit out version info
        printf("%s %s Version %d.%d\n", argv[0],
               service_name, m_iMajorVersion, m_iMinorVersion);
        printf("The service is %s installed\n",
               isinstalled() ? "currently" : "not");
        return TRUE; // say we processed the argument

    } else if (_stricmp(argv[1], "-i") == 0) {

        // Request to install.
        if (isinstalled()) {
            printf("%s is already installed\n", service_name);
        } else {
            // Try and install the copy that's running
            if (install(argv[0])) {
                printf("%s installed successfully!\n", service_name);
            } else {
                printf("%s failed to install. Error %d\n", service_name, GetLastError());
            }
        }
        return TRUE; // say we processed the argument

    } else if (_stricmp(argv[1], "-u") == 0) {

        // Request to uninstall.
        if (!isinstalled()) {
            printf("%s is not installed\n", service_name);
        } else {
            // Try and remove the copy that's installed
            if (uninstall()) {
                // Get the executable file path
                char filePath[_MAX_PATH];
                ::GetModuleFileName(NULL, filePath, sizeof(filePath));
                printf("%s removed. (You must delete the file (%s) yourself.)\n",
                       service_name, filePath);
            } else {
                printf("Could not remove %s. Error %d\n", service_name, GetLastError());
            }
        }
        return TRUE; // say we processed the argument
    
    }
         
    // Don't recognise the args
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////
// install/uninstall routines

// Test if the service is currently installed
bool NTSvc::isinstalled()
{
    bool bResult = FALSE;

    // Open the Service Control Manager
    SC_HANDLE hSCM = ::OpenSCManager(NULL, // local machine
                                     NULL, // ServicesActive database
                                     SC_MANAGER_ALL_ACCESS); // full access
    if (hSCM) {

        // Try to open the service
        SC_HANDLE hService = ::OpenService(hSCM,
                                           service_name,
                                           SERVICE_QUERY_CONFIG);
        if (hService) {
            bResult = TRUE;
            ::CloseServiceHandle(hService);
        }

        ::CloseServiceHandle(hSCM);
    }
    
    return bResult;
}

bool NTSvc::install(const char *cfg)
{
    // Open the Service Control Manager
	char filePath[_MAX_PATH];
    SC_HANDLE hSCM = ::OpenSCManager(NULL, // local machine
                                     NULL, // ServicesActive database
                                     SC_MANAGER_ALL_ACCESS); // full access
    if (!hSCM) return FALSE;

    // Get the executable file path
    // Create the service
    ::GetModuleFileName(NULL, filePath, sizeof(filePath));
    // Create the service
	TEXTUS_STRCAT(filePath, " ");
	TEXTUS_STRCAT(filePath, cfg);
    SC_HANDLE hService = ::CreateService(hSCM,
                                         service_name,
                                         service_name,
                                         SERVICE_ALL_ACCESS,
                                         SERVICE_WIN32_OWN_PROCESS,
                                         SERVICE_AUTO_START,        // start condition
                                         SERVICE_ERROR_NORMAL,
                                         filePath,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL);
    if (!hService) {
        ::CloseServiceHandle(hSCM);
        return FALSE;
    }

#ifdef USE_LOG_EVENT
    // make registry entries to support logging messages
    // Add the source name as a subkey under the Application
    // key in the EventLog service portion of the registry.
    char szKey[256];
    HKEY hKey = NULL;
    TEXTUS_STRCPY(szKey, "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");
    TEXTUS_STRCAT(szKey, service_name);
    if (::RegCreateKey(HKEY_LOCAL_MACHINE, szKey, &hKey) != ERROR_SUCCESS) {
        ::CloseServiceHandle(hService);
        ::CloseServiceHandle(hSCM);
        return FALSE;
    }

    // Add the Event ID message-file name to the 'EventMessageFile' subkey.
	TEXTUS_STRCPY(filePath, appPath);
	TEXTUS_STRCAT(filePath, " ");
	TEXTUS_STRCAT(filePath, event_file);
    ::RegSetValueEx(hKey,
                    "EventMessageFile",
                    0,
                    REG_EXPAND_SZ, 
                    (CONST BYTE*)filePath,
                    (DWORD)strlen(filePath) + 1);     

    // Set the supported types flags.
    DWORD dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
    ::RegSetValueEx(hKey,
                    "TypesSupported",
                    0,
                    REG_DWORD,
                    (CONST BYTE*)&dwData,
                     sizeof(DWORD));
    ::RegCloseKey(hKey);

    logEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_INSTALLED, service_name);
#endif
    // tidy up
    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);
    return TRUE;
}

bool NTSvc::uninstall()
{
    // Open the Service Control Manager
    SC_HANDLE hSCM = ::OpenSCManager(NULL, // local machine
                                     NULL, // ServicesActive database
                                     SC_MANAGER_ALL_ACCESS); // full access
    if (!hSCM) return FALSE;

    bool bResult = FALSE;
    SC_HANDLE hService = ::OpenService(hSCM,
                                       service_name,
                                       DELETE);
    if (hService) {
        if (::DeleteService(hService)) {
            logEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_REMOVED, service_name);
            bResult = TRUE;
        } else {
            logEvent(EVENTLOG_ERROR_TYPE, EVMSG_NOTREMOVED, service_name);
        }
        ::CloseServiceHandle(hService);
    }
    
    ::CloseServiceHandle(hSCM);
    return bResult;
}

NTSvc::~NTSvc() { } 
#include "hook.c"

