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
#include "BTool.h"
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
#include "md5.h"

#define SEQ_FLD 3
#define TAG_FLD 5
#define OFFSET_FLD 7
#define MSG_SUM_FLD 9
#define START_SEC_FLD 11
#define START_MILLI_FLD 12
#define INTERVAL_FLD 13
#define BODY_LEN_FLD 15
#define MD_MAGIC "1AE!#$$$DD112D"  // 每条日志中再加一点内容，再加MD5，以简单实现的防改。
#define MD_SUM_LEN 3

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
	Amor::Pius clr_timer_pius, alarm_pius, pro_unipac, pro_tbuf;  /* 清超时, 设超时 */
	PacketObj tmr_pac;     /* 起止时间参数 */
	PacketObj *pa[3];
	void *arr[3];
	TBuffer *rcv_buf;
#if defined (_WIN32)
	FILETIME start_tm, end_tm;
	static unsigned __int64 t2k;
#else
	struct timespec start_tp, end_tp;
	static time_t t2k;
#endif
	static char md_magic[64];	/*默认是MD_MAGIC */
	static int md_magic_len;

	bool framing;	/* 通道是否打开 */
	void stamp();

	struct G_CFG {
		int time_out;	/* 超时时间。0: 不设超时 */
		unsigned short tag_len;
		unsigned char *tag;
		unsigned short seq_len;
		unsigned char *seq;
		
		Amor *sch;
		inline G_CFG ( TiXmlElement *cfg ) {
			const char *str;
			time_out = 0;
			cfg->QueryIntAttribute("time_out", &(time_out));
			sch = 0;
			tag_len = 0;
			tag = 0;
			seq_len = 0;
			seq = 0;
			str = cfg->Attribute("tag");
			if ( str ) {
				tag_len = strlen(str);
				tag = new unsigned char[tag_len];
				tag_len = BTool::unescape(str, tag);
			}
			str = cfg->Attribute("seq");
			if ( str ) {
				seq_len = strlen(str);
				seq = new unsigned char[seq_len];
				seq_len = BTool::unescape(str, seq);
			}
		};
	};
	struct G_CFG *gCFG;	/* Shared for all objects in this node */
	bool has_config;

#include "wlog.h"
};

char BufTmr::md_magic[] = {0};
int BufTmr::md_magic_len = 0;
#if defined (_WIN32)
unsigned __int64 BufTmr::t2k = 0;
#else
time_t BufTmr::t2k = 0;
#endif

void BufTmr::ignite(TiXmlElement *cfg) 
{
#if defined (_WIN32)
	SYSTEMTIME y2k;
	FILETIME ct2k;
	y2k.wYear = 2000;
	y2k.wMonth = 1;
	y2k.wDay = 1;
	y2k.wHour = 0;

	y2k.wMinute = 0;
	y2k.wSecond = 0;
	y2k.wMilliseconds = 0;
	SystemTimeToFileTime(&y2k, &ct2k);
	t2k = (unsigned __int64)ct2k.dwLowDateTime + (((unsigned __int64)ct2k.dwHighDateTime) << 32);
#else
	struct tm time_str;
	time_str.tm_year    = 100;
	time_str.tm_mon = 0;
	time_str.tm_mday = 1;
	time_str.tm_hour = 0;
	time_str.tm_min = 0;
	time_str.tm_sec = 0;
	time_str.tm_isdst = 0;
	t2k = mktime(&time_str);
#endif
	if (!cfg) return;

	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
	TEXTUS_STRCPY(md_magic, MD_MAGIC);
	md_magic_len = strlen(md_magic);
}

void BufTmr::stamp()
{
	unsigned long interval, length, start_sec, start_milli;
	unsigned char offset;
	MD5_CTX Md5Ctx;
	unsigned char md[16];
	//if ( !framing ) return;
#if defined (_WIN32)
	unsigned __int64 etik, stik, intv;
	etik = (unsigned __int64)end_tm.dwLowDateTime + (((unsigned __int64)end_tm.dwHighDateTime) << 32);
	stik = (unsigned __int64)start_tm.dwLowDateTime + (((unsigned __int64)start_tm.dwHighDateTime) << 32);
	intv = (etik-stik)/1000;
	interval = (long) intv;
	start_sec =  (long) ((stik - t2k)/10000000);
	start_milli =  (long) ((stik - t2k)/1000 - start_sec*10000);
#else
	interval = 1000* (end_tp.tv_sec - start_tp.tv_sec) + (end_tp.tv_nsec - start_tp.tv_nsec)/1000000;
	start_sec =  start_tp.tv_sec - t2k;
	start_milli =  start_tp.tv_nsec/100000;
#endif
	length  = rcv_buf->point - rcv_buf->base;
	tmr_pac.input(BODY_LEN_FLD, (unsigned char*)&length, sizeof(length));
	tmr_pac.input(SEQ_FLD, gCFG->seq, gCFG->seq_len);
	tmr_pac.input(TAG_FLD, gCFG->tag, gCFG->tag_len);
	tmr_pac.input(START_SEC_FLD, (unsigned char*)&start_sec, sizeof(start_sec));
	tmr_pac.input(START_MILLI_FLD, (unsigned char*)&start_milli, sizeof(start_milli));
	tmr_pac.input(INTERVAL_FLD, (unsigned char*)&interval, sizeof(interval));

	MD5Init (&Md5Ctx);
	MD5Update (&Md5Ctx, md_magic, md_magic_len);
	MD5Update (&Md5Ctx, (char*)&start_sec, sizeof(start_sec));
	MD5Update (&Md5Ctx, (char*)&start_milli, sizeof(start_milli));
	MD5Update (&Md5Ctx, (char*)&interval, sizeof(interval));
	MD5Update (&Md5Ctx, (char*)rcv_buf->base, length);
	MD5Final ((char *) &md[0], &Md5Ctx);
	tmr_pac.input(MSG_SUM_FLD, md, MD_SUM_LEN);
	offset = MD_SUM_LEN;
	tmr_pac.input(OFFSET_FLD, &offset, sizeof(offset));

	framing = false;
	//gCFG->sch->sponte(&clr_timer_pius); /* 一次定时，不用清除 */
	aptus->facio(&pro_unipac);
	aptus->facio(&pro_tbuf);
}

bool BufTmr::facio( Amor::Pius *pius)
{
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF:	
		WBUG("facio PRO_TBUF");
		if ( !framing ) {
#if defined (_WIN32)
			GetSystemTimeAsFileTime(&start_tm);
#else
			clock_gettime(CLOCK_REALTIME, &start_tp);
#endif
			framing = true;
			if ( gCFG->time_out == 0 ) 
			{
#if defined (_WIN32)
				end_tm = start_tm;
#else
				end_tp = start_tp;
#endif
				stamp();
			} else {
				gCFG->sch->sponte(&alarm_pius); /* 定时开始 */
			}
		} else {	//framing ， 每来一次, 记一下时间
#if defined (_WIN32)
			GetSystemTimeAsFileTime(&end_tm);
#else
			clock_gettime(CLOCK_REALTIME, &end_tp);
#endif
		}
		break;

	case Notitia::DMD_END_SESSION:	/* channel to closed */
	case Notitia::START_SESSION:	/* channel is alive */
		WBUG("facio DMD_END_SESSION/START_SESSION");
		framing = false;
		break;

	case Notitia::TIMER:	/* 连接超时 */
		WBUG("facio TIMER" );
		stamp();
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
		pro_unipac.indic = &pa[0];
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

	case Notitia::SET_TBUF:	/* 取得输入TBuffer地址 */
		WBUG("facio SET_TBUF");
		pro_tbuf.indic =pius->indic;
		{ TBuffer **tb;
		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//当然tb不能为空
			if ( *tb) 
			{	//新到请求的TBuffer
				rcv_buf = *tb;
			}
			tb++;
			//if ( *tb) rcv_buf = *tb;
		} else 
			WLOG(NOTICE,"facio PRO_TBUF null.");
		}
		return false; /* 可以续传 */
		break;

	default:
		return false;
	}
	return true;
}

bool BufTmr::sponte( Amor::Pius *pius) { return false; }

BufTmr::BufTmr()
{
	pro_unipac.indic = 0;
	pro_unipac.ordo = Notitia::PRO_UNIPAC;
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
