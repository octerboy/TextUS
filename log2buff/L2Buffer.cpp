/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: logdata to TBuffer
 Build: created by octerboy, 2007/05/28
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "BTool.h"
#include "casecmp.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifndef TINLINE
#define TINLINE inline
#endif

#define BZERO(X) memset(X, 0 ,sizeof(X))

class L2Buffer: public Amor
{
public:
	L2Buffer();
	~L2Buffer();

	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
	
	TBuffer first_buf;

private:
	struct G_CFG {
		unsigned char *tail;
		unsigned int t_len;
		bool levelActive[8];	
		
		int *aliusIDs;		/* 模块的唯一标志 */
		unsigned int numID;
		inline G_CFG() {
			tail = 0;
			t_len = 0;
	
			/* 默认所有日志等级都不记 */
			for (unsigned int j = 0; j < sizeof(levelActive); j++)
				levelActive[j] = false;

			aliusIDs = 0;
			numID = 0;
		};

		inline ~G_CFG() {
			if ( tail )
				delete[] tail;

			if ( aliusIDs )
				delete[] aliusIDs;
		};
	};

	struct G_CFG *gCFG;
	bool has_config;

	TINLINE void deliver(Notitia::HERE_ORDO aordo);

#include "tbug.h"

#ifdef DEBUG
#undef DEBUG
#endif
	enum LogLevel {EMERG=0, ALERT=1, CRIT=2, ERR=3, WARNING=4, NOTICE=5, INFO=6, DEBUG=7};
	
};

void L2Buffer::ignite(TiXmlElement *prop)
{
	const char *comm_str;
	TiXmlElement *dat, *level_ele, *al_ele;

	if (!prop) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
	}

	dat = prop->FirstChildElement("tail");
	comm_str = 0;
	if ( dat ) comm_str = dat->GetText();
	if ( comm_str )
	{
		gCFG->tail = new unsigned char[ strlen(comm_str) +1 ];
		gCFG->t_len = BTool::unescape(comm_str, gCFG->tail);
	}

	level_ele = prop->FirstChildElement("log");
	while(level_ele)
	{
		comm_str = level_ele->Attribute("level");
		if (comm_str) 
		{	/* 扫描要记的日志 */
#define SETLOG(X) if ( strcasecmp ( comm_str, #X ) == 0 ) \
				gCFG->levelActive[X] = true;

			SETLOG(ERR);	SETLOG(DEBUG);	SETLOG(WARNING);
			SETLOG(CRIT);	SETLOG(EMERG);	SETLOG(ALERT);
			SETLOG(INFO);	SETLOG(NOTICE); 
			if ( strcmp(comm_str, "all") == 0 ) 
			{
				gCFG->levelActive[ERR] 		= true;
				gCFG->levelActive[DEBUG] 	= true;
				gCFG->levelActive[WARNING] 	= true;
				gCFG->levelActive[CRIT] 	= true;
				gCFG->levelActive[EMERG] 	= true;
				gCFG->levelActive[ALERT] 	= true;
				gCFG->levelActive[INFO] 	= true;
				gCFG->levelActive[NOTICE]	= true;
			}
			
		}
#undef SETLOG
		level_ele = level_ele->NextSiblingElement("log");
	}

	al_ele = prop->FirstChildElement("alius"); gCFG->numID = 0;
	while(al_ele)
	{
		comm_str = al_ele->Attribute("id");
		if (comm_str) 
		{	/* 扫描要记的模块ID  */
			gCFG->numID++;
		}
		al_ele = al_ele->NextSiblingElement("alius");
	}

	return ;
}

bool L2Buffer::facio( Amor::Pius *pius)
{
	void **ind;
	int id;
	unsigned char *buf;

	switch(pius->ordo )
	{
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");		
		deliver(Notitia::SET_TBUF);
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY %d" , pius->ordo);			
		deliver(Notitia::SET_TBUF);
		break;

	case Notitia::FAC_LOG_EMERG :
#define ISLOG(X)	\
		if ( !gCFG->levelActive[X] )	\
			break;			\
		else				\
			goto NOWLOG;
		ISLOG(EMERG);

	case Notitia::FAC_LOG_ALERT  :
		ISLOG(ALERT);

	case Notitia::FAC_LOG_CRIT   :
		ISLOG(CRIT);

	case Notitia::FAC_LOG_ERR   :
		ISLOG(ERR);

	case Notitia::FAC_LOG_WARNING:
		ISLOG(WARNING);

	case Notitia::FAC_LOG_NOTICE :
		ISLOG(NOTICE);

	case Notitia::FAC_LOG_INFO :
		ISLOG(INFO);

	case Notitia::FAC_LOG_DEBUG:
		ISLOG(DEBUG);

NOWLOG:
		ind = (void **)(pius->indic);
		if (ind) 
		{
			if ( *ind) id = *((int*)(*ind)); 
			ind++;
			if ( *ind) { 
				buf = (unsigned char*)(*ind);
				first_buf.input(buf,strlen((char*)buf));
				if ( gCFG->tail )
					first_buf.input(gCFG->tail, gCFG->t_len);

				deliver(Notitia::PRO_TBUF);
			}
		} else {	
			WBUG("facio FAC_LOG null");
		}
		break;
				
	default:
		return false;
	}
	return true;
}

bool L2Buffer::sponte( Amor::Pius *pius)
{
	return false;
}

L2Buffer::L2Buffer()
{
	gCFG = 0;
	has_config = false;
}

L2Buffer::~L2Buffer()
{
	if (has_config ) 
	{
		if(gCFG) delete gCFG;
	}	
}

Amor* L2Buffer::clone()
{
	L2Buffer *child;
	child = new L2Buffer();
	child->gCFG = gCFG;

	return (Amor*)child;
}

/* 向接力者提交 */
TINLINE void L2Buffer::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	TBuffer *tb[3];
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	static int ss = 0;

	switch (aordo )
	{
	case Notitia::SET_TBUF:
		WBUG("deliver SET_TBUF");
		tb[0] = &first_buf;
		tb[1] = 0;
		tb[2] = 0;
		tmp_pius.indic = &tb[0];
		break;
	default:
		ss++;
		WBUG("deliver Notitia::%d %d", aordo, ss);
		ss--;
		break;
	}
	aptus->facio(&tmp_pius);
	return ;
}

#define AMOR_CLS_TYPE L2Buffer
#include "hook.c"
