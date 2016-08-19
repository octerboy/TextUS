/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 Title: set system time
 Build: created by octerboy 2005/04/12
 $Header: /textus/sysadm/Systime.cpp 3     08-01-01 23:19 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: Systime.cpp $"
#define TEXTUS_MODTIME  "$Date: 08-01-01 23:19 $"
#define TEXTUS_BUILDNO  "$Revision: 3 $"
/* $NoKeywords: $ */


#include "Notitia.h"
#include "Amor.h"
#include "textus_string.h"
#include <time.h>
#include <stdarg.h>
#if !defined (_WIN32)
#include <sys/time.h>
#endif
#include <errno.h>
#include "textus_string.h"

class Systime :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	Systime();

private:
	Amor::Pius local_pius;  //仅用于向mary传回数据
	Amor::Pius direct_pius;
	Amor::Pius fault_ps;    /* 向左传递 ERR_SOAP_FAULT */
	void handle1();
	bool handle2(TiXmlElement *);
	TiXmlElement root;

	#include "wlog.h"
	#include "httpsrv_obj.h"
};

void Systime::ignite(TiXmlElement *cfg) { }

bool Systime::facio( Amor::Pius *pius)
{
	TiXmlElement *tmp;
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::PRO_SOAP_BODY:	/* 有SOAP请求 */
	case Notitia::PRO_HTTP_REQUEST:	/* 有HTTP请求 */
		WBUG("facio PRO_SOAP_BODY/PRO_HTTP_REQUEST");
		tmp = 0x0;
		if ( pius->indic )
			tmp = (TiXmlElement *)pius->indic ;

		if ( tmp && tmp->FirstChildElement()) 
		{
			if ( !handle2(tmp) )
			{
				aptus->sponte(&fault_ps);
				break;
			}
		} else {
			handle1();
		}

		if ( pius->ordo == Notitia::PRO_SOAP_BODY )
			aptus->sponte(&local_pius);
		else {
			TiXmlPrinter printer;	
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

bool Systime::sponte( Amor::Pius *pius) { return false; }

Systime::Systime():root("SysTime")
{
	local_pius.ordo = Notitia::PRO_SOAP_BODY;
	local_pius.indic = &root;

	fault_ps.ordo = Notitia::ERR_SOAP_FAULT;
	fault_ps.indic = &root;

	direct_pius.ordo = Notitia::PRO_HTTP_RESPONSE;
	direct_pius.indic = 0;
}

Amor* Systime::clone()
{
	return  (Amor*)new Systime();
}

void Systime::handle1()
{
	time_t now;
	struct tm* mytm;
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
	struct tm tmytm;
#endif
	char expr[10];
	now = time(&now);
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
       mytm = &tmytm;
       localtime_s(mytm, &now);
#else
	mytm = localtime(&now);
#endif

	TiXmlElement year( "year" );
	TiXmlElement month( "month" );
	TiXmlElement day( "day" );
	TiXmlElement hour( "hour" );
	TiXmlElement minute( "minute" );
	TiXmlElement second( "second" );
	TiXmlText text( "" );

	root.Clear();

	TEXTUS_SPRINTF(expr,"%d",mytm->tm_year+1900);
	text.SetValue(expr);
	year.InsertEndChild(text);
	
	TEXTUS_SPRINTF(expr,"%d",mytm->tm_mon+1);
	text.SetValue(expr);
	month.InsertEndChild(text);
	
	TEXTUS_SPRINTF(expr,"%d",mytm->tm_mday);
	text.SetValue(expr);
	day.InsertEndChild(text);
	
	TEXTUS_SPRINTF(expr,"%d",mytm->tm_hour);
	text.SetValue(expr);
	hour.InsertEndChild(text);
	
	TEXTUS_SPRINTF(expr,"%d",mytm->tm_min);
	text.SetValue(expr);
	minute.InsertEndChild(text);
	
	TEXTUS_SPRINTF(expr,"%d",mytm->tm_sec);
	text.SetValue(expr);
	second.InsertEndChild(text);

	root.InsertEndChild(year);
	root.InsertEndChild(month);
	root.InsertEndChild(day);
	root.InsertEndChild(hour);
	root.InsertEndChild(minute);
	root.InsertEndChild(second);
}

bool Systime::handle2(TiXmlElement *reqele)
{
	char tmpstr[300];
	TiXmlText *c_val = new TiXmlText("");
	TiXmlText *s_val = new TiXmlText("");
	TiXmlElement *c_ele = new TiXmlElement("faultcode");
	TiXmlElement *s_ele = new TiXmlElement("faultstring");

	bool ret=true;

	TiXmlElement *year;
	TiXmlElement *month;
	TiXmlElement *day;
	TiXmlElement *hour;
	TiXmlElement *minute;
	TiXmlElement *second;

	root.Clear();
	year = reqele->FirstChildElement("year");
	month = reqele->FirstChildElement("month");
	day = reqele->FirstChildElement("day");
	hour = reqele->FirstChildElement("hour");
	minute = reqele->FirstChildElement("minute");
	second = reqele->FirstChildElement("second");

	if ( year && month && day && hour && minute && second 
		&& year->GetText() &&  month->GetText()  &&  day->GetText() 
		&& hour->GetText() &&  minute->GetText() && second->GetText() )
	{
#if defined (_WIN32)
		SYSTEMTIME mytm;	
		mytm.wSecond 	= atoi (second->GetText());
		mytm.wMinute 	= atoi (minute->GetText());
		mytm.wHour 	= atoi (hour->GetText());
		mytm.wDay 	= atoi (day->GetText());
		mytm.wMonth 	= atoi (month->GetText());
		mytm.wYear 	= atoi (year->GetText());
		mytm.wMilliseconds  = 0;
		mytm.wDayOfWeek   = -1;
		if ( !SetLocalTime(&mytm) )
#else
		struct tm mytm;	
		struct timeval tv;
		struct timezone tz;
		mytm.tm_sec 	=atol (second->GetText());
		mytm.tm_min 	=atol (minute->GetText());
		mytm.tm_hour 	=atol (hour->GetText());
		mytm.tm_mday 	=atol (day->GetText());
		mytm.tm_mon 	=atol (month->GetText())-1;
		mytm.tm_year 	=atol (year->GetText())-1900;
		gettimeofday(&tv, &tz);
		mytm.tm_isdst = daylight;
		tv.tv_sec = (long) mktime(&mytm);
		tv.tv_usec = 0;
		if ( settimeofday(&tv, &tz) !=0  )
#endif
		{
#if defined (_WIN32)
			char errstr[256];
			char *s;
			DWORD dw = GetLastError();
			FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw, 
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)errstr, 256, NULL );
			s= strstr(errstr, "\r\n") ;
			if (s )  *s = '\0';
			WLOG(ALERT, "set time error %d: %s", dw, errstr);
			TEXTUS_SPRINTF(tmpstr, "set time error: %s", errstr);
#else
			WLOG(ALERT, "set time error: %s", strerror(errno));
			TEXTUS_SPRINTF(tmpstr, "set time error: %s", strerror(errno));
#endif
			s_val->SetValue(tmpstr);
			c_val->SetValue("TIME_SET_FAIL");
			goto Err;
		} else 
		{
			time_t now = time(&now);
#if defined (_WIN32)
			char str[128];
			TEXTUS_SPRINTF(tmpstr,"%s", ctime_s(str,128, &now));
#else
			TEXTUS_SPRINTF(tmpstr,"%s", ctime(&now));
#endif
			s_val->SetValue(tmpstr);
			root.LinkEndChild(s_val);	
			ret = true;
			goto SystemSetTimeEnd;
		}
	} else
	{
		s_val->SetValue("no enough time data.");
		c_val->SetValue("TIME_SET_FAIL");
		goto Err;
	}
Err:
	c_ele->LinkEndChild(c_val);	
	s_ele->LinkEndChild(s_val);	
	root.LinkEndChild(c_ele);	
	root.LinkEndChild(s_ele);	
	ret = false;
	
SystemSetTimeEnd:
	return ret;
}

#include "hook.c"
