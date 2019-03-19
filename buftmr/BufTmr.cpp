/* Copyright (c) 2019-2022 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
/**
 Title: Buffer for Timer
 Build: created by octerboy, 2019/03/19, Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "casecmp.h"
#include "PacData.h"
#include "textus_string.h"
#include <time.h>
#include <ctype.h>
#include <stdarg.h>
#include <assert.h>
#if !defined (_WIN32)
#include <sys/time.h>
#endif

class BufTmr: public Amor
{
public:
	int count;
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	BufTmr();
	~BufTmr();

private:
	Amor::Pius clr_timer_pius, alarm_pius, pro_tbuf;  /* 清超时, 设超时 */
	PacketObj tmr_pac;     /* 起止时间参数 */
	PacketObj *pa[3];
	void *arr[3];
	TBuffer *rcv_buf, *snd_buf;
#if !defined (_WIN32)
	struct timeval start_tv, end_tv;
	struct timezone start_tz, end_tz;
#else
	 FILETIME start_tm, end_tm;
#endif

	bool framing;	/* 通道是否打开 */

	struct G_CFG {
		int time_out;	/* 超时时间。0: 不设超时 */
		Amor *sch;
		inline G_CFG ( TiXmlElement *cfg ) {
			time_out = 0;
			cfg->QueryIntAttribute("time_out", &(time_out));
			sch = 0;
		};
	};
	struct G_CFG *gCFG;	/* Shared for all objects in this node */
	bool has_config;

#include "wlog.h"
};

void BufTmr::ignite(TiXmlElement *cfg) 
{
	if (!cfg) return;

	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
}

bool BufTmr::facio( Amor::Pius *pius)
{
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF:	
		WBUG("facio PRO_TBUF");
		if ( !framing ) {
#if !defined (_WIN32)
			gettimeofday(&start_tv, &start_tz);
#else
			GetSystemTimeAsFileTime(&start_tm);
#endif
			framing = true;
			gCFG->sch->sponte(&alarm_pius); /* 定时开始 */
		}
		break;

	case Notitia::DMD_END_SESSION:	/* channel to closed */
	case Notitia::START_SESSION:	/* channel is alive */
		WBUG("facio DMD_END_SESSION/START_SESSION");
		framing = false;
		break;

	case Notitia::TIMER:	/* 连接超时 */
		WBUG("facio TIMER" );
		if ( framing )
		{
			framing = false;
			gCFG->sch->sponte(&clr_timer_pius); /* 清除定时 */
#if !defined (_WIN32)
			gettimeofday(&end_tv, &end_tz);
#else
			GetSystemTimeAsFileTime(&end_tm);
#endif
			aptus->facio(&pro_tbuf);
		}
		break;

	case Notitia::TIMER_HANDLE:
		WBUG("facio TIMER_HANDLE %p", pius->indic);
		clr_timer_pius.indic = pius->indic;
		break;

	case Notitia::IGNITE_ALL_READY:	
		WBUG("facio IGNITE_ALL_READY" );			
		{
			Amor::Pius tmp_p;
			tmp_p.ordo = Notitia::CMD_GET_SCHED;
			tmp_p.indic = 0;
			aptus->sponte(&tmp_p);	//向tpoll, 取得sched
			gCFG->sch = (Amor*)tmp_p.indic;
			if ( !gCFG->sch ) 
			{
				WLOG(ERR, "no sched or tpoll");
				break;
			}
		}
		goto ALL_READY;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE(IGNITE)_ALL_READY" );			
ALL_READY:
		arr[0] = this;
		arr[1] = &(gCFG->time_out);
		arr[2] = 0;
		{
			Amor::Pius tmp_p;
			tmp_p.ordo = Notitia::SET_UNIPAC;
			tmp_p.indic = &pa[0];
			aptus->facio(&tmp_p);
		}
		break;

	default:
		return false;
	}
	return true;
}

bool BufTmr::sponte( Amor::Pius *pius) { return false; }

BufTmr::BufTmr()
{
	pro_tbuf.indic = 0;
	pro_tbuf.ordo = Notitia::PRO_TBUF;

	gCFG = 0;
	has_config = false ;
	clr_timer_pius.ordo = Notitia::DMD_CLR_TIMER;
	clr_timer_pius.indic = this;	/* 预设为this,  用于sched模块, 被更新则用于tpoll模块 */

	alarm_pius.ordo = Notitia::DMD_SET_ALARM;
	alarm_pius.indic = &arr[0];
	tmr_pac.produce(16) ;
	pa[0] = &tmr_pac;
	pa[1] = &tmr_pac;
	pa[2] = 0;
}

BufTmr::~BufTmr() 
{
	if ( framing)
		gCFG->sch->sponte(&clr_timer_pius); /* 清除定时, 不再重连服务端 */
	if ( has_config && gCFG)
		delete gCFG;
}

Amor* BufTmr::clone()
{
	BufTmr *child;
	child = new BufTmr();
	child->gCFG = gCFG;
	return (Amor*)child;
}

#include "hook.c"
