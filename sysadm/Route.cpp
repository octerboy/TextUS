/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 Title: Set/Get the Route Table
 Build: created by octerboy 2005/04/12
 $Header: /textus/sysadm/Route.cpp 3     08-01-01 23:19 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: Route.cpp $"
#define TEXTUS_MODTIME  "$Date: 08-01-01 23:19 $"
#define TEXTUS_BUILDNO  "$Revision: 3 $"
/* $NoKeywords: $ */

#include "BTool.h"
#include "Amor.h"
#include "Notitia.h"
#include "textus_string.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdarg.h>

class Route :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	Route();

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

void Route::ignite(TiXmlElement *cfg) { }

bool Route::facio( Amor::Pius *pius)
{
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::PRO_SOAP_BODY:	/* 有SOAP请求 */
	case Notitia::PRO_HTTP_REQUEST:	/* 有HTTP请求 */
		WBUG("facio PRO_SOAP_BODY/PRO_HTTP_REQUEST");
		if ( pius->indic )
		{
			if ( !handle2((TiXmlElement *)pius->indic) )
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

bool Route::sponte( Amor::Pius *pius) { return false; }

Route::Route():root("Route")
{
	local_pius.ordo = Notitia::PRO_SOAP_BODY;
	local_pius.indic = &root;

	fault_ps.ordo = Notitia::ERR_SOAP_FAULT;
	fault_ps.indic = &root;

	direct_pius.ordo = Notitia::PRO_HTTP_RESPONSE;
	direct_pius.indic = 0;
}

Amor* Route::clone()
{
	return  (Amor*)new Route();
}

void Route::handle1()
{
	FILE *fp;

	root.Clear();
	fp = fopen ( "/etc/sysconfig/static-routes","r");
	if ( fp )
	{
		char fline[256];
		while ( fgets(fline,255,fp))
		{
			char *val;
			TiXmlElement route( "route" );
			TiXmlElement destination( "destination" );
			TiXmlElement gateway( "gateway" );
			TiXmlElement iface( "iface" );
			TiXmlElement mask( "mask" );
			TiXmlText text( "" );

			val = strstr(fline," gw ");	
			*val = '\0';
			val += strlen(" gw ");
			text.SetValue(val);
			gateway.InsertEndChild(text);

			val = strstr(fline," netmask ");	
			*val = '\0';
			val += strlen(" netmask ");
			text.SetValue(val);
			mask.InsertEndChild(text);

			val = strstr(fline," net ");	
			*val = '\0';
			val += strlen(" net ");
			text.SetValue(val);
			destination.InsertEndChild(text);

			val = fline;
			text.SetValue(val);
			iface.InsertEndChild(text);

			route.InsertEndChild(destination);
			route.InsertEndChild(gateway);
			route.InsertEndChild(iface);
			route.InsertEndChild(mask);

			root.InsertEndChild(route);
		}
		fclose(fp);
	}
}

bool Route::handle2(TiXmlElement *reqele)
{
	TiXmlText *c_val = new TiXmlText("");
	TiXmlText *s_val = new TiXmlText("");
	TiXmlElement *c_ele = new TiXmlElement("faultcode");
	TiXmlElement *s_ele = new TiXmlElement("faultstring");

	TiXmlElement *route;
	FILE *fp;
	bool ret=true;

	root.Clear();
	fp = fopen ("/etc/sysconfig/static-routes","w");
	if( !fp )
	{
		WLOG(WARNING, "Cannot open the file of /etc/sysconfig/static-routes");
		s_val->SetValue("cannot open the file /etc/sysconfig/static-routes.");
		c_val->SetValue("ROUTE_SET_FAIL");
		root.Clear();
		c_ele->LinkEndChild(c_val);	
		s_ele->LinkEndChild(s_val);	
		root.LinkEndChild(c_ele);	
		root.LinkEndChild(s_ele);	
		ret = false;
		goto SystemSetRouteEnd;
	}

	route = reqele->FirstChildElement("route");
	while (route)
	{
		TiXmlElement *destination = route->FirstChildElement("destination");
		TiXmlElement *gateway = route->FirstChildElement("gateway");
		TiXmlElement *iface = route->FirstChildElement("iface");
		TiXmlElement *mask = route->FirstChildElement("mask");
		
		if (  destination->GetText() &&  iface->GetText() && mask->GetText() )
		{
			fprintf(fp, "%s net %s netmask %s gw %s\n", 
				iface->GetText(),
				destination->GetText(),
				mask->GetText(),
				gateway->GetText());
		}

		route = route->NextSiblingElement ("route");
	} 
	fclose(fp);

SystemSetRouteEnd:
	return ret;
}
#include "hook.c"
