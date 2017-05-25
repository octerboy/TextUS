/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 Title: primary system infomation
 Build: created by octerboy 2005/04/12
 $Header: /textus/sysadm/SysStat.cpp 3     08-01-01 23:19 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: SysStat.cpp $"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "netrs.h"
#include "BTool.h"
#include "textus_string.h"

#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <time.h>
#include <stdarg.h>

class SysStat :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	SysStat();
	~SysStat();

private:
	Amor::Pius local_pius;  //仅用于向mary传回数据
	Amor::Pius direct_pius;
	TiXmlElement root;

	inline void handle();

	#include "wlog.h"
	#include "httpsrv_obj.h"
};

void SysStat::ignite(TiXmlElement *cfg) { }

bool SysStat::facio( Amor::Pius *pius)
{
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::PRO_SOAP_BODY:	/* 有SOAP请求 */
	case Notitia::PRO_HTTP_REQUEST:	/* 有HTTP请求 */
		WBUG("facio PRO_SOAP_BODY/PRO_HTTP_REQUEST");
		handle();
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

bool SysStat::sponte( Amor::Pius *pius) { return false; }

SysStat::SysStat():root("System")
{
	local_pius.ordo = Notitia::PRO_SOAP_BODY;
	local_pius.indic = &root;
	direct_pius.ordo = Notitia::PRO_HTTP_RESPONSE;
	direct_pius.indic = 0;
}

Amor* SysStat::clone()
{
	return  (Amor*)new SysStat();
}

SysStat::~SysStat() { }

void SysStat::handle()
{
	int i = 0;

	Netrs netrs;
	/* 给出网络接口信息 */
	struct Netrs::NicInfo *nic = netrs.getNicInfo();

	/* 给出路由信息 */
	struct Netrs::RouteInfo  *route = netrs.getRouteInfo();

	root.Clear();
	if ( nic )
	{
		struct Netrs::NicSubInfo *nics = nic->subinfo;

		for (  i = 0; i < nic->nicNum; i++ )
		{
			TiXmlElement eth( "eth" );
			TiXmlElement iface( "iface" );
			TiXmlElement ip( "ip" );
			TiXmlElement mask( "mask" );
			TiXmlElement mac( "mac" );
			TiXmlElement up( "up" );
			TiXmlElement loop( "loop" );
			TiXmlElement p2p( "p2p" );
			TiXmlText text( "" );


			text.SetValue(nics[i].iface);
			iface.InsertEndChild(text);
			eth.InsertEndChild(iface);
			
			text.SetValue(nics[i].ip);
			ip.InsertEndChild(text);
			eth.InsertEndChild(ip);
			
			text.SetValue(nics[i].netmask);
			mask.InsertEndChild(text);
			eth.InsertEndChild(mask);
			
			text.SetValue(nics[i].mac);
			mac.InsertEndChild(text);
			eth.InsertEndChild(mac);

			if ( nics[i].flag & IFF_UP )
			{
				text.SetValue("yes");
			} else
			{
				text.SetValue("no");
			}

			up.InsertEndChild(text);
			eth.InsertEndChild(up);

			if ( nics[i].flag & IFF_LOOPBACK)
			{
				text.SetValue("yes");
			} else
			{
				text.SetValue("no");
			}
			loop.InsertEndChild(text);
			eth.InsertEndChild(loop);
			
			if ( nics[i].flag & IFF_POINTOPOINT)
			{
				text.SetValue("yes");
			} else
			{
				text.SetValue("no");
			}
			p2p.InsertEndChild(text);
			eth.InsertEndChild(p2p);

			root.InsertEndChild(eth);
		}
	}

	if ( route )
	{
		struct Netrs::RouteSubInfo *rns = route->subinfo;	
		for ( i = 0; i < route->routeNum; i++ )
		{
			TiXmlElement route( "route" );
			TiXmlElement destination( "destination" );
			TiXmlElement gateway( "gateway" );
			TiXmlElement iface( "iface" );
			TiXmlElement mask( "mask" );
			TiXmlText text( "" );

			text.SetValue(rns[i].netip);
			destination.InsertEndChild(text);
			route.InsertEndChild(destination);

			text.SetValue(rns[i].gateway);
			gateway.InsertEndChild(text);
			route.InsertEndChild(gateway);

			text.SetValue(rns[i].iface);
			iface.InsertEndChild(text);
			route.InsertEndChild(iface);

			text.SetValue(rns[i].netmask);
			mask.InsertEndChild(text);
			route.InsertEndChild(mask);

			root.InsertEndChild(route);
		}
	}

	/* DNS服务器信息 */
	i = 1;
	while ( true )
	{
		TiXmlElement nameserver( "nameserver" );
		TiXmlText text( "" );

		char *nms = BTool::getaddr ( "/etc/resolv.conf", "nameserver"," \t",i);
		if (!nms ) 
			break;

		text.SetValue(nms);
		nameserver.InsertEndChild(text);
		root.InsertEndChild(nameserver);
		i++;
	}
}
#include "hook.c"

