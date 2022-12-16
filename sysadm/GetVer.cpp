/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 Title: Get the version of all modules
 Build: created by octerboy 2006/12/23
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Eros.h"
#include "Notitia.h"
#include "casecmp.h"
#include "textus_string.h"
#include "Logger.h"

#include <time.h>
#if !defined (_WIN32)
#include <sys/utsname.h>
#endif

class GetVer :public Eros, public TusLogger
{
public:
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
	TiXmlElement root;

	GetVer();

private:
	Amor::Pius local_pius;
	Amor::Pius direct_pius;
	#include "httpsrv_obj.h"
};

bool GetVer::facio( Amor::Pius *pius)
{
	Amor::Pius neo;
	char tmpstr[512];
	TiXmlText text("");
	TiXmlElement ver( "OS" );
#if defined (_WIN32)
	OSVERSIONINFOEX osver;
#else
	struct utsname osver;
#endif
	tmpstr[0] = 0;
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::PRO_SOAP_BODY:	/* 有SOAP请求 */
	case Notitia::PRO_HTTP_REQUEST:	/* 有HTTP请求 */
		WBUG("facio PRO_SOAP_BODY/PRO_HTTP_REQUEST");
		neo.ordo = Notitia::CMD_GET_VERSION;
		neo.indic = 0;
		root.Clear();

#if defined (_WIN32)
		osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);	
#pragma warning(disable: 4996)
		GetVersionEx((LPOSVERSIONINFO)&osver);	
		TEXTUS_SNPRINTF(tmpstr, sizeof(tmpstr)-1, "Microsoft Windows(%d, %08x) %d.%d.%d %s", osver.dwPlatformId, osver.wSuiteMask, osver.dwMajorVersion, osver.dwMinorVersion, osver.dwPlatformId == 2 ? osver.dwBuildNumber : osver.dwBuildNumber & 0xff, osver.szCSDVersion);
#else
		uname(&osver);
		TEXTUS_SNPRINTF(tmpstr, sizeof(tmpstr)-1, "<system>%s</system> <node>%s</node> <release>%s</release> <version>%s</version> <machine>%s</machine>", osver.sysname, osver.nodename, osver.release, osver.version, osver.machine);

#endif
		text.SetValue(tmpstr);
		ver.InsertEndChild(text);
		root.InsertEndChild(ver);

		aptus->facio(&neo);
		WBUG("CMD_GET_VERSION %s", tmpstr);
		if ( neo.indic)
			root.InsertEndChild(*((TiXmlElement *)neo.indic))->SetValue("Application");

		if ( pius->ordo == Notitia::PRO_SOAP_BODY )
			aptus->sponte(&local_pius);
		else {
			TiXmlPrinter printer;	
			root.SetValue("Version");
			root.Accept(&printer);
			setContentSize(printer.Size());
			output((char*)printer.CStr(), printer.Size());
			setHead("Content-Type", "text/xml");
			aptus->sponte(&direct_pius);
		}
		break;

	case Notitia::IGNITE_ALL_READY:	
	case Notitia::CLONE_ALL_READY:
		break;
		
	   default:
		return false;
	}
	return true;
}

bool GetVer::sponte( Amor::Pius *pius) { return false; }

GetVer::GetVer():root("Version")
{
	local_pius.ordo = Notitia::PRO_SOAP_BODY;
	local_pius.indic = &root;
	direct_pius.ordo = Notitia::PRO_HTTP_RESPONSE;
	direct_pius.indic = 0;
}

Amor* GetVer::clone()
{
	return  (Amor*)new GetVer();
}

#include "hook.c"

