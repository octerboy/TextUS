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
#include <string.h>
#include <assert.h>


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
	char service_name[256];
	void start();
	void run();

    void logEvent(WORD wType, DWORD dwID,
		const char* pszS1 = NULL,const char* pszS2 = NULL,const char* pszS3 = NULL);
    void setStatus(DWORD dwState);
    bool initialize();
	bool onInit();
 
    void OnStop();
    void OnInterrogate();
    void OnPause();
    void OnContinue();
    void OnShutdown();
	void saveStatus();    

    // data members

    // static data
	static	NTSvc* here_svc; // nasty hack to get object ptr

    HANDLE m_hEventSource;
	int argOffset;
    int m_iMajorVersion;
    int m_iMinorVersion;
	BYTE m_iStartParam, m_iIncParam, m_iState;
    SERVICE_STATUS_HANDLE m_hServiceStatus;
    SERVICE_STATUS m_Status;
    bool m_bIsRunning;
	void log(char *s);
	void my_handle(DWORD dwOpcode);
	void my_main(DWORD dwArgc, LPTSTR* lpszArgv);

#include "wlog.h"
};

// static member functions
static void WINAPI serviceMain(DWORD dwArgc, LPTSTR* lpszArgv);
static void WINAPI handler(DWORD dwOpcode);
NTSvc *NTSvc::here_svc = (NTSvc*)0;
#define NTS_HANDLER_FAILED 0x67L

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
	//void **ps;
	//char *path;
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::MAIN_PARA:	/* 在整个系统中, 这应是最后被通知到的。 */
		WBUG("facio Notitia::MAIN_PARA");
		//ps = (void**)pius->indic;
		//if (!parseStandardArgs( (*(int *)ps[0]) - argOffset, &((char**)ps[1])[argOffset]) )
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

void NTSvc::start()
{
	SERVICE_TABLE_ENTRY st[] = {
		{service_name, serviceMain},
		{NULL, NULL}
	};

    WBUG("Calling StartServiceCtrlDispatcher()");
    BOOL b = ::StartServiceCtrlDispatcher(st);
    WBUG("Returned %d from StartServiceCtrlDispatcher()", b);
}


void NTSvc::my_main(DWORD dwArgc, LPTSTR* lpszArgv)
{   
    WBUG("Entering NTSvc::ServiceMain()");
    // Register the control request handler
    m_Status.dwCurrentState = SERVICE_START_PENDING;
    m_hServiceStatus = RegisterServiceCtrlHandler(service_name,handler);
    if (m_hServiceStatus == NULL) {
        //logEvent(EVENTLOG_ERROR_TYPE, NTS_HANDLER_FAILED);
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
        run();
    }

    // Tell the service manager we are stopped
    setStatus(SERVICE_STOPPED);
    WBUG("Leaving NTSvc::ServiceMain()");
}

void NTSvc::log(char *s)
{
    WBUG(s);
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
        //logEvent(EVENTLOG_ERROR_TYPE, EVMSG_FAILEDINIT);
		WLOG_OSERR("service init failed")
        setStatus(SERVICE_STOPPED);
        return FALSE;    
    }
    
    //logEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_STARTED);
	WLOG(INFO, "service %s started", service_name); 
    setStatus(SERVICE_RUNNING);
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// main function to do the real work of the service

// This function performs the main work of the service. 
// When this function returns the service has stopped.
void NTSvc::run() {
    WBUG("Entering NTSvc::Run()");
    while (m_bIsRunning) {
        WBUG("Sleeping...");
        Sleep(5000);
    }

    // nothing more to do
    WBUG("Leaving NTSvc::Run()");
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
	WBUG("NTSvc::Handler(%lu)", dwOpcode);
    switch (dwOpcode) {
    case SERVICE_CONTROL_STOP: // 1
        setStatus(SERVICE_STOP_PENDING);
        OnStop();
        m_bIsRunning = FALSE;
        //logEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_STOPPED);
		WLOG(INFO, "service %s stopped", service_name);
        break;

    case SERVICE_CONTROL_PAUSE: // 2
        OnPause();
        break;

    case SERVICE_CONTROL_CONTINUE: // 3
        OnContinue();
        break;

    case SERVICE_CONTROL_INTERROGATE: // 4
        OnInterrogate();
        break;

    case SERVICE_CONTROL_SHUTDOWN: // 5
        OnShutdown();
        break;

    default:
		WLOG(ERR, "bad request %d", dwOpcode);
        break;
    }

    // Report current status
	WBUG("Updating status (%lu, %lu)", m_hServiceStatus, m_Status.dwCurrentState);
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
	TEXTUS_SPRINTF(szKey, "SYSTEM\\CurrentControlSet\\Services\\%s\\Parameters", service_name);

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS) {
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
    } else 	return false;

	// Set the initial state
	m_iState = m_iStartParam;

	return true;
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
NTSvc::~NTSvc() { } 
#include "hook.c"

