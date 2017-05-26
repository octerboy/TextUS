/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: TCP client to Textus
 Build: created by octerboy, 2005/06/10
 $Id$
*/
#define SCM_MODULE_ID	"$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Notitia.h"
#include "textus_string.h"
#include <time.h>
#include "Notitia.h"
#include "Amor.h"
#include <stdarg.h>

#ifndef TINLINE
#define TINLINE inline
#endif 

class CliPol: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	CliPol();
	~CliPol();

private:
	Amor::Pius local_pius;
	Amor::Pius info_pius;

	char errMsg[2048];

	bool has_config;
	struct G_CFG {
		int cli_num;
		TiXmlElement **cli_addr;	
		inline G_CFG(TiXmlElement *prop) {
			TiXmlElement *var_ele;
			const char *vn = "peer";
			int many,i;
	
			cli_num = 0;
			for (var_ele = root->FirstChildElement(vn); var_ele; var_ele = var_ele->NextSiblingElement(vn) ) 
			{
				many = 1;
				var_ele->QueryIntAttribute("channels", &(many));
				cli_num += many;
			}

			cli_addr = new TiXmlElement *[cli_num];
			i = 0; cli_num = 0;
			for (var_ele = root->FirstChildElement(vn); var_ele; var_ele = var_ele->NextSiblingElement(vn)) 
			{
				many = 1;
				var_ele->QueryIntAttribute("channels", &(many));
				for ( i = 0; i < many; i++)
				{
					cli_addr[cli_num] = var_ele;
					cli_num++;
				}
			}

		};
	};
	struct G_CFG *gCFG;

#include "wlog.h"
};

#include <assert.h>
#include <stdio.h>
#include "textus_string.h"
#include "casecmp.h"

void CliPol::ignite(TiXmlElement *cfg)
{
	if ( !gCFG) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
}

bool CliPol::facio( Amor::Pius *pius)
{
	const char *ip_str;
	TiXmlElement *cfg;
	TBuffer **tb;
	assert(pius);
	switch (pius->ordo)
	{
	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		end(true);
		break;

	case Notitia::WINMAIN_PARA:
	case Notitia::MAIN_PARA:
		establish();		//开始建立连接
		break;

	case Notitia::TIMER:
		WBUG("facio TIMER, last %ld", last_failed_time);
		if ( gCFG->try_interval > 0 && tcpcli->connfd == -1 &&  last_failed_time &&
			*((time_t *)(pius->indic)) - last_failed_time >= gCFG->try_interval )
		{	//最近发生一次连接, 而且连接失败, 间隔时间到达设定值
			establish();		//开始建立连接
		}
		break;

	case Notitia::IGNITE_ALL_READY:
	case Notitia::CLONE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY/CLONE_ALL_READY");
		set_peer();

		break;

	default:
		return false;
	}	
	return true;
}

bool CliPol::sponte( Amor::Pius *pius) 
{ 
	switch (pius->ordo)
	{
	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		end(true);
		break;

	default:
		return false;
	}	
	return true;
}

CliPol::CliPol()
{
	gCFG = 0;
	has_config = false;
}

CliPol::~CliPol()
{	
	if (has_config )
		delete gCFG;
}

TINLINE void CliPol::establish()
{
	WLOG(INFO, "estabish to %s:%d  .....", tcpcli->server_ip, tcpcli->server_port);
	if (!tcpcli->annecto(gCFG->block_mode))
	{
		errpro();
		return ;
	}

	if ( tcpcli->isConnecting) 
	{	//正连接中,则向schedule登记
		mytor.scanfd = tcpcli->connfd;
		deliver(Notitia::FD_SETWR);
		deliver(Notitia::FD_SETRD);

#if defined(_WIN32)
		deliver(Notitia::FD_SETEX);
#endif
	} else /* 连接完成 */
		establish_done();
}

Amor* CliPol::clone()
{
	CliPol *child;

	child = new CliPol();
	child->gCFG = gCFG;
	return (Amor*)child;
}

/* 向接力者提交 */
TINLINE void CliPol::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;

	switch (aordo )
	{
	case Notitia::PRO_TBUF:
		WBUG("deliver PRO_TBUF");
		break;

	case Notitia::END_SESSION:
		WBUG("deliver END_SESSION");
		aptus->facio(&tmp_pius);
		break;

	case Notitia::START_SESSION:
		WBUG("deliver START_SESSION");
		aptus->facio(&tmp_pius);
		break;

	case Notitia::DMD_SET_TIMER:
	case Notitia::DMD_CLR_TIMER:
		WBUG("deliver DMD_SET(CLR)_TIMER(%d)", aordo);
		tmp_pius.indic = this;
		break;

	case Notitia::FD_CLRRD:
	case Notitia::FD_CLRWR:
	case Notitia::FD_CLREX:
	case Notitia::FD_SETRD:
	case Notitia::FD_SETWR:
	case Notitia::FD_SETEX:
		local_pius.ordo =aordo;
		aptus->sponte(&local_pius);	//向Sched
		return ;

	default:
		break;
	}
	aptus->sponte(&tmp_pius);
}

#include "hook.c"
