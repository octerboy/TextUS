/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Tcp连接控制扩展
 Build:created by octerboy, 2006/07/21
 $Header: /textus/tcpctrl/TCtrl.cpp 12    14-04-14 7:25 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: TCtrl.cpp $"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include  <sys/types.h>
#include "textus_string.h"
#if !defined(_WIN32)
#include  <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif
#include <errno.h>
#include <stdarg.h>
#include "PacData.h"

#define CLIENT_MAX 20
#define BZERO(X) memset(X, 0 ,sizeof(X))

class TCtrl: public Amor
{
public:
	void ignite(TiXmlElement *);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
	TCtrl();
	~TCtrl();

private:
	
	bool has_config;
	struct G_CFG {
		int srvip_fld;	
		int srvport_fld;
		int srvmac_fld;

		int cliip_fld;	
		int cliport_fld;
		int climac_fld;

		int max_fld;		/* 最大域号 */

		bool limited;
		char ip[CLIENT_MAX][64]; /* 所允许的客户端的ip号, 内容共享 */
		inline G_CFG() {
			srvip_fld = -1;	
			srvport_fld = -1;
			srvmac_fld = -1;

			cliip_fld = -1;	
			cliport_fld = -1;
			climac_fld = -1;
			max_fld = 10;
			limited = false;
			BZERO(ip);
		};

	};
	struct G_CFG *gCFG;

	int* fd;
	Amor::Pius local_pius;	//仅用于向mary传回数据
	TiXmlElement *peer;
	struct sockaddr_in name1;
	struct sockaddr_in name2;
	bool isPioneer;

	PacketObj f_pac;	/* facio 处理 */
	PacketObj s_pac;	/* sponte 处理 */
	PacketObj *pa[3];	/* 保存上两个指针 */

	PacketObj *first_pac;	/* facio 处理 */
	PacketObj *second_pac;	/* sponte 处理 */
	Amor::Pius pro_unipac;

#include "wlog.h"
};

void TCtrl::ignite(TiXmlElement *cfg)
{
	TiXmlElement *cli_ele;	
	const char *str;
	int i = 0;

	if (!cfg) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
	}

	cfg->QueryIntAttribute("server_ip", &(gCFG->srvip_fld));
	cfg->QueryIntAttribute("server_port", &(gCFG->srvport_fld));
	cfg->QueryIntAttribute("server_mac", &(gCFG->srvmac_fld));
	cfg->QueryIntAttribute("client_ip", &(gCFG->cliip_fld));
	cfg->QueryIntAttribute("client_port", &(gCFG->cliport_fld));
	cfg->QueryIntAttribute("client_mac", &(gCFG->climac_fld));
	cfg->QueryIntAttribute("maxium", &(gCFG->max_fld));

#define MAXFLD(x) \
	if( gCFG->max_fld <= gCFG->x )	\
		gCFG->max_fld = gCFG->x;

	if( gCFG->max_fld <= 0 )
	{
		MAXFLD(srvip_fld);
		MAXFLD(srvport_fld);
		MAXFLD(srvmac_fld);
		MAXFLD(cliip_fld);
		MAXFLD(cliport_fld);
		MAXFLD(climac_fld);
	}

	f_pac.produce(gCFG->max_fld);
	s_pac.produce(gCFG->max_fld);

	str = cfg->Attribute("limited");
	if ( str && strcmp(str, "yes") == 0 )
		gCFG->limited = true;

	cli_ele = cfg->FirstChildElement("client");
	while ( cli_ele )
	{
		str = cli_ele->Attribute("ip");
		if( str )
		{
			TEXTUS_STRNCPY(gCFG->ip[i], str, 64);
		}
		i++;
		if ( i == CLIENT_MAX) break;
		cli_ele = cli_ele->NextSiblingElement("client");
	}
	isPioneer = true;
}

bool TCtrl::facio( Amor::Pius *pius)
{
	Amor::Pius tmp_pius;
#if defined(__linux__) || defined(_AIX) || defined(__APPLE__)
	socklen_t clilen; 
#else
	int clilen;
#endif
	clilen= sizeof(name1);

	assert(pius);
	
	switch ( pius->ordo )
	{
	case Notitia::DMD_END_SESSION:	
		WBUG("facio DMD_END_SESSION");
		fd = 0;
		break;

	case Notitia::IGNITE_ALL_READY:	
		WBUG("facio IGNITE_ALL_READY");
		goto MYPRO;
		break;

	case Notitia::CLONE_ALL_READY:	
		WBUG("facio IGNITE_ALL_READY/CLONE_ALL_READY");	/* 获得其fd的地址 */
	MYPRO:
		local_pius.ordo = Notitia::CMD_GET_FD;
		local_pius.indic = 0;
		aptus->sponte(&local_pius);
		fd = (int *)local_pius.indic;
		if ( !fd )
			WLOG(INFO, "CMD_GET_FD get 0");

		tmp_pius.ordo = Notitia::SET_UNIPAC;
		tmp_pius.indic = pa;
		aptus->facio(&tmp_pius);
		break;

	case Notitia::START_SERVICE:	
		WBUG("facio START_SERVICE");
		isPioneer = true;
		break;

	case Notitia::END_SERVICE:	
		WBUG("facio END_SERVICE");
		isPioneer = false;
		break;

	case Notitia::START_SESSION:	
		WBUG("facio START_SESSION");
		if ( isPioneer ) break;
		if (!fd || *fd < 0) break;
		if ( getsockname(*fd, (struct sockaddr *)&name1, &clilen) < 0 || 
			getpeername(*fd, (struct sockaddr *)&name2, &clilen) < 0 )
		{
#if defined(_WIN32)
			WLOG(ERR, "getsockname/getpeername err#:%d", GetLastError());
#else
			WLOG(ERR, "getsockname/getpeername %d: %s", errno, strerror(errno));
#endif
			break;
		} else {
			char *str;
			unsigned short prt;
		
			first_pac->reset();	
			str = inet_ntoa(name1.sin_addr);
			peer->SetAttribute("srvip", str);	
			if ( gCFG->srvip_fld > 0 ) 
				first_pac->input(gCFG->srvip_fld, (unsigned char*)str, strlen(str));

			prt = ntohs(name1.sin_port);
			peer->SetAttribute("srvport", prt);	
			if ( gCFG->srvport_fld > 0 ) 
				first_pac->input(gCFG->srvport_fld, (unsigned char*)&prt, sizeof(prt));

			str = inet_ntoa(name2.sin_addr);
			peer->SetAttribute("cliip", str);	
			if ( gCFG->cliip_fld > 0 ) 
				first_pac->input(gCFG->cliip_fld, (unsigned char*)str, strlen(str));

			prt = ntohs(name2.sin_port);
			peer->SetAttribute("cliport", prt);	
			if ( gCFG->cliport_fld > 0 ) 
				first_pac->input(gCFG->cliport_fld, (unsigned char*)&prt, sizeof(prt));

			WLOG(NOTICE, "start_session(fd=%d) %s:%s-%s:%s", *fd, peer->Attribute("srvip"), peer->Attribute("srvport"),\
				peer->Attribute("cliip"), peer->Attribute("cliport"));
			if ( gCFG->limited ) 
			{
				const char *cli_ip = peer->Attribute("cliip");
				int i;
				for ( i = 0 ; i < CLIENT_MAX; i++ )
				{
					if (strcmp ( gCFG->ip[i], cli_ip) == 0 )
						break;
				}
				if ( i >= CLIENT_MAX ) 
					goto FAIL;
			}
			aptus->facio(&pro_unipac);
		}
		goto End;
FAIL:
		WLOG(WARNING, "%s is not allowed", peer->Attribute("cliip"));
		local_pius.ordo = Notitia::DMD_END_SESSION;
		aptus->sponte(&local_pius);
End:
		break;

	default:
		return false;
	}
	return true;
}

bool TCtrl::sponte( Amor::Pius *pius) 
{ 
	PacketObj **tmp;
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::CMD_GET_PEER:	
		WBUG("sponte CMD_GET_PEER");
		if ( fd ) 
			pius->indic = peer;
		break;

	case Notitia::SET_UNIPAC:
		WBUG("sponte SET_UNIPAC");
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) first_pac = *tmp; 
			else
				WLOG(WARNING, "sponte SET_UNIPAC rcv_pac null");
			tmp++;
			if ( *tmp) second_pac = *tmp;
			else
				WLOG(WARNING, "sponte SET_UNIPAC snd_pac null");
		} else 
			WLOG(WARNING, "sponte SET_UNIPAC null");

		break;
	default:
		return false;
	}

	return true;
}

Amor* TCtrl::clone()
{
	TCtrl *child = new TCtrl();
	child->gCFG = gCFG;
	child->f_pac.produce(gCFG->max_fld);
	child->s_pac.produce(gCFG->max_fld);

	return (Amor*)child;
}

TCtrl::TCtrl()
{
	local_pius.indic = 0;
	peer = new TiXmlElement("peer");
	fd = 0;
	isPioneer = false;

	pa[0] = &f_pac;
	pa[1] = &s_pac;
	pa[2] = 0;

	first_pac = &f_pac;
	second_pac = &s_pac;
	gCFG = 0;
	has_config = false;

	pro_unipac.ordo = Notitia::PRO_UNIPAC;
	pro_unipac.indic = 0;
}

TCtrl::~TCtrl()
{
	delete peer;
}
#include "hook.c"
