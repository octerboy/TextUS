/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
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
 $Header: $
*/

#define SCM_MODULE_ID  "$Workfile: NTSvc.cpp $"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include <string.h>
#include <assert.h>
#include "Notitia.h"
#include "Amor.h"
#include <time.h>

class NTSvc :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	NTSvc();
	~NTSvc();

private:
	Amor::Pius local_pius;  //仅用于传回数据
	Amor::Pius end_pius;  //仅用于传回数据
	char service_name[256];
	void start();

    bool parseStandardArgs(int argc, char* argv[]);
    bool isinstalled();
    bool install();
    bool uninstall();
    void logEvent(WORD wType, DWORD dwID,
                  const char* pszS1 = NULL,
                  const char* pszS2 = NULL,
                  const char* pszS3 = NULL);
    void setStatus(DWORD dwState);
    bool initialize();
    virtual void Run();
	bool onInit();
 
    virtual void OnStop();
    virtual void OnInterrogate();
    virtual void OnPause();
    virtual void OnContinue();
    virtual void OnShutdown();
    virtual bool OnUserControl(DWORD dwOpcode);

	void saveStatus()
    
    // static member functions
    static void WINAPI serviceMain(DWORD dwArgc, LPTSTR* lpszArgv);
    static void WINAPI handler(DWORD dwOpcode);

    // data members

    // static data
    static CNTService* m_pThis; // nasty hack to get object ptr

private:
    HANDLE m_hEventSource;
	int argOffset;
	char service_name[64];
    int m_iMajorVersion;
    int m_iMinorVersion;
    SERVICE_STATUS_HANDLE m_hServiceStatus;
    SERVICE_STATUS m_Status;
    bool m_bIsRunning;

#include "wlog.h"
};

void NTSvc::ignite(TiXmlElement *cfg) { 
	const char *comm_str;
	if ( !cfg )
		return;
	if ( (comm_str = cfg->Attribute("service_name")) )
		TEXTUS_STRNCPY( service_name, comm_str, sizeof(service_name)-1);

	if ( (comm_str = cfg->Attribute("offset")) )
		argOffset = atoi(comm_str);
	if ( argOffset < 1 ) 
		argOffset =1;
}

bool NTSvc::facio( Amor::Pius *pius)
{
	void **ps;
	char *path;
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::MAIN_PARA:	/* 在整个系统中, 这应是最后被通知到的。 */
		WBUG("facio Notitia::MAIN_PARA");
		ps = (void**)pius->indic;
		if (!parseStandardArgs( (*(int *)ps[0]) - argOffset, &((char**)ps[1])[argOffset]) )
			start();
		break;
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
	argOffset = 1;
    m_pThis = this;
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

void NTSvc::start()
{
    SERVICE_TABLE_ENTRY st[] = {
        {service_name, serviceMain},
        {NULL, NULL}
    };

    WBUG("Calling StartServiceCtrlDispatcher()");
    bool b = ::StartServiceCtrlDispatcher(st);
    WBUG("Returned from StartServiceCtrlDispatcher()");
    return b;
}

NTSvc* NTSvc::m_pThis = 0;
void NTSvc::serviceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
    // Get a pointer to the C++ object
    NTSvc* pService = m_pThis;
    
    WBUG("Entering NTSvc::ServiceMain()");
    // Register the control request handler
    pService->m_Status.dwCurrentState = SERVICE_START_PENDING;
    pService->m_hServiceStatus = RegisterServiceCtrlHandler(pService->service_name,
                                                           handler);
    if (pService->m_hServiceStatus == NULL) {
        pService->logEvent(EVENTLOG_ERROR_TYPE, EVMSG_CTRLHANDLERNOTINSTALLED);
        return;
    }

    // Start the initialisation
    if (pService->initialize()) {

        // Do the real work. 
        // When the Run function returns, the service has stopped.
        pService->m_bIsRunning = TRUE;
        pService->m_Status.dwWin32ExitCode = 0;
        pService->m_Status.dwCheckPoint = 0;
        pService->m_Status.dwWaitHint = 0;
        pService->run();
    }

    // Tell the service manager we are stopped
    pService->setStatus(SERVICE_STOPPED);

    pService->WBUG("Leaving NTSvc::ServiceMain()");
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
        printf("%s Version %d.%d\n",
               m_szServiceName, m_iMajorVersion, m_iMinorVersion);
        printf("The service is %s installed\n",
               isinstalled() ? "currently" : "not");
        return TRUE; // say we processed the argument

    } else if (_stricmp(argv[1], "-i") == 0) {

        // Request to install.
        if (isinstalled()) {
            printf("%s is already installed\n", m_szServiceName);
        } else {
            // Try and install the copy that's running
            if (install()) {
                printf("%s installed\n", m_szServiceName);
            } else {
                printf("%s failed to install. Error %d\n", m_szServiceName, GetLastError());
            }
        }
        return TRUE; // say we processed the argument

    } else if (_stricmp(argv[1], "-u") == 0) {

        // Request to uninstall.
        if (!isinstalled()) {
            printf("%s is not installed\n", m_szServiceName);
        } else {
            // Try and remove the copy that's installed
            if (uninstall()) {
                // Get the executable file path
                char szFilePath[_MAX_PATH];
                ::GetModuleFileName(NULL, szFilePath, sizeof(szFilePath));
                printf("%s removed. (You must delete the file (%s) yourself.)\n",
                       m_szServiceName, szFilePath);
            } else {
                printf("Could not remove %s. Error %d\n", m_szServiceName, GetLastError());
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
                                           m_szServiceName,
                                           SERVICE_QUERY_CONFIG);
        if (hService) {
            bResult = TRUE;
            ::CloseServiceHandle(hService);
        }

        ::CloseServiceHandle(hSCM);
    }
    
    return bResult;
}

bool NTSvc::install()
{
    // Open the Service Control Manager
    SC_HANDLE hSCM = ::OpenSCManager(NULL, // local machine
                                     NULL, // ServicesActive database
                                     SC_MANAGER_ALL_ACCESS); // full access
    if (!hSCM) return FALSE;

    // Get the executable file path
    char szFilePath[_MAX_PATH];
    ::GetModuleFileName(NULL, szFilePath, sizeof(szFilePath));

    // Create the service
    SC_HANDLE hService = ::CreateService(hSCM,
                                         m_szServiceName,
                                         m_szServiceName,
                                         SERVICE_ALL_ACCESS,
                                         SERVICE_WIN32_OWN_PROCESS,
                                         SERVICE_DEMAND_START,        // start condition
                                         SERVICE_ERROR_NORMAL,
                                         szFilePath,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL);
    if (!hService) {
        ::CloseServiceHandle(hSCM);
        return FALSE;
    }

    // make registry entries to support logging messages
    // Add the source name as a subkey under the Application
    // key in the EventLog service portion of the registry.
    char szKey[256];
    HKEY hKey = NULL;
    strcpy(szKey, "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");
    strcat(szKey, m_szServiceName);
    if (::RegCreateKey(HKEY_LOCAL_MACHINE, szKey, &hKey) != ERROR_SUCCESS) {
        ::CloseServiceHandle(hService);
        ::CloseServiceHandle(hSCM);
        return FALSE;
    }

    // Add the Event ID message-file name to the 'EventMessageFile' subkey.
    ::RegSetValueEx(hKey,
                    "EventMessageFile",
                    0,
                    REG_EXPAND_SZ, 
                    (CONST BYTE*)szFilePath,
                    strlen(szFilePath) + 1);     

    // Set the supported types flags.
    DWORD dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
    ::RegSetValueEx(hKey,
                    "TypesSupported",
                    0,
                    REG_DWORD,
                    (CONST BYTE*)&dwData,
                     sizeof(DWORD));
    ::RegCloseKey(hKey);

    logEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_INSTALLED, m_szServiceName);

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
                                       m_szServiceName,
                                       DELETE);
    if (hService) {
        if (::DeleteService(hService)) {
            logEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_REMOVED, m_szServiceName);
            bResult = TRUE;
        } else {
            logEvent(EVENTLOG_ERROR_TYPE, EVMSG_NOTREMOVED, m_szServiceName);
        }
        ::CloseServiceHandle(hService);
    }
    
    ::CloseServiceHandle(hSCM);
    return bResult;
}

///////////////////////////////////////////////////////////////////////////////////////
// Logging functions

// This function makes an entry into the application event log
void NTSvc::logEvent(WORD wType, DWORD dwID,
                          const char* pszS1,
                          const char* pszS2,
                          const char* pszS3)
{
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
                                               m_szServiceName); // source name
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
        setStatus(SERVICE_STOPPED);
        return FALSE;    
    }
    
    logEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_STARTED);
    setStatus(SERVICE_RUNNING);

    WBUG("Leaving NTSvc::initialize()");
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// main function to do the real work of the service

// This function performs the main work of the service. 
// When this function returns the service has stopped.
void NTSvc::Run()
{
    WBUG("Entering NTSvc::Run()");
    while (m_bIsRunning) {
        WBUG("Sleeping...");
        Sleep(5000);
    }

    // nothing more to do
    WBUG("Leaving NTSvc::Run()");
}

//////////////////////////////////////////////////////////////////////////////////////
// Control request handlers

// static member function (callback) to handle commands from the
// service control manager
void NTSvc::handler(DWORD dwOpcode)
{
    // Get a pointer to the object
    NTSvc* pService = m_pThis;
    
    pService->WBUG("NTSvc::Handler(%lu)", dwOpcode);
    switch (dwOpcode) {
    case SERVICE_CONTROL_STOP: // 1
        pService->setStatus(SERVICE_STOP_PENDING);
        pService->OnStop();
        pService->m_bIsRunning = FALSE;
        pService->logEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_STOPPED);
        break;

    case SERVICE_CONTROL_PAUSE: // 2
        pService->OnPause();
        break;

    case SERVICE_CONTROL_CONTINUE: // 3
        pService->OnContinue();
        break;

    case SERVICE_CONTROL_INTERROGATE: // 4
        pService->OnInterrogate();
        break;

    case SERVICE_CONTROL_SHUTDOWN: // 5
        pService->OnShutdown();
        break;

    default:
        if (dwOpcode >= SERVICE_CONTROL_USER) {
            if (!pService->OnUserControl(dwOpcode)) {
                pService->logEvent(EVENTLOG_ERROR_TYPE, EVMSG_BADREQUEST);
            }
        } else {
            pService->logEvent(EVENTLOG_ERROR_TYPE, EVMSG_BADREQUEST);
        }
        break;
    }

    // Report current status
    pService->WBUG("Updating status (%lu, %lu)",
                       pService->m_hServiceStatus,
                       pService->m_Status.dwCurrentState);
    ::SetServiceStatus(pService->m_hServiceStatus, &pService->m_Status);
}
        
// Called when the service is first initialized
bool NTSvc::onInit()
{
	// Read the registry parameters
    // Try opening the registry key:
    // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\<AppName>\Parameters
    HKEY hkey;
	char szKey[1024];
	strcpy(szKey, "SYSTEM\\CurrentControlSet\\Services\\");
	strcat(szKey, service_name);
	strcat(szKey, "\\Parameters");
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     szKey,
                     0,
                     KEY_QUERY_VALUE,
                     &hkey) == ERROR_SUCCESS) {
        // Yes we are installed
        DWORD dwType = 0;
        DWORD dwSize = sizeof(m_iStartParam);
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
    }

	// Set the initial state
	m_iState = m_iStartParam;

	return TRUE;
}

// Called when the service control manager wants to stop the service
void NTSvc::OnStop()
{
    WBUG("NTSvc::OnStop()");
}

// called when the service is interrogated
void NTSvc::OnInterrogate()
{
    WBUG("NTSvc::OnInterrogate()");
}

// called when the service is paused
void NTSvc::OnPause()
{
    WBUG("NTSvc::OnPause()");
}

// called when the service is continued
void NTSvc::OnContinue()
{
    WBUG("NTSvc::OnContinue()");
}

// called when the service is shut down
void NTSvc::OnShutdown()
{
    WBUG("NTSvc::OnShutdown()");
}

// called when the service gets a user control message
bool NTSvc::OnUserControl(DWORD dwOpcode)
{
    switch (dwOpcode) {
    case SERVICE_CONTROL_USER + 0:

        // Save the current status in the registry
        saveStatus();
        return TRUE;

    default:
        break;
    }
    return FALSE; // say not handled
}

// Save the current status in the registry
void NTSvc::saveStatus()
{
    WBUG("Saving current status");
    // Try opening the registry key:
    // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\<AppName>\...
    HKEY hkey = NULL;
	char szKey[1024];
	strcpy(szKey, "SYSTEM\\CurrentControlSet\\Services\\");
	strcat(szKey, m_szServiceName);
	strcat(szKey, "\\Status");
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
NTSvc::~NTSvc() { } 
#include "hook.c"

