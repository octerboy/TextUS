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

#define SEQ_FLD 3
#define TAG_FLD 5
#define OFFSET_FLD 7
#define MSG_SUM_FLD 9
#define START_SEC_FLD 11
#define START_MILLI_FLD 12
#define INTERVAL_FLD 13
#define BODY_LEN_FLD 15
#define DATA_OPT_FLD 17
#define MD_MAGIC "1AE!#$$$DD112D"  // 每条日志中再加一点内容，再加MD5，以简单实现的防改。
#define MD_SUM_LEN 3

class BufTmr: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	BufTmr();
	~BufTmr();
	enum LocType { 
	HLVAR	= 13,	/* 变长,一个字节的值表示, 0x26表示38字节长度等  */
	HLLVAR	= 14,	/* 变长, 低位在前, 高位在后, 下同 */
	HLLLVAR	= 15,	
	HL4VAR	= 16	
	};
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
		unsigned short time_len;
		unsigned short tag_len;
		unsigned char *tag;
		unsigned short seq_len;
		unsigned char *seq;
		unsigned short opt_len;
		unsigned char *opt;
		LocType len_type;           /* 获取长度值的方式 */
		unsigned short body_len_n;
		
		Amor *sch;
		inline G_CFG ( TiXmlElement *cfg ) {
			const char *str;
			time_out = 0;
			cfg->QueryIntAttribute("time_out", &(time_out));
			if ( time_out <  16777215 ) time_len =3 ;
			if ( time_out <  65535 ) time_len =2 ;
			if ( time_out <  255 ) time_len =1 ;
			sch = 0;
			tag_len = 0;
			tag = 0;
			seq_len = 0;
			seq = 0;
			opt_len =0;
			opt = 0;
			str = cfg->Attribute("tag");
			if ( str ) {
				tag_len = (unsigned short)strlen(str);
				tag = new unsigned char[tag_len];
				tag_len = BTool::unescape(str, tag);
			}
			str = cfg->Attribute("seq");
			if ( str ) {
				seq_len = (unsigned short)strlen(str);
				seq = new unsigned char[seq_len];
				seq_len = BTool::unescape(str, seq);
			}
			str = cfg->Attribute("opt");
			if ( str ) {
				opt_len = (unsigned short)strlen(str);
				opt = new unsigned char[opt_len];
				opt_len = BTool::unescape(str, opt);
			}
			str = cfg->Attribute("body_len");
			len_type = HLLVAR;
			#define LOC(X) if ( str && strcasecmp(str, #X) == 0 )	\
					len_type =X;
			LOC(HLVAR)	
			LOC(HLLVAR)	
			LOC(HLLLVAR)	
			LOC(HL4VAR)	
			switch ( len_type)
			{
			case HLVAR:	
				body_len_n = 1;
				break;

			case HLLVAR:	
				body_len_n = 2;
				break;

			case HLLLVAR:	
				body_len_n = 3;
				break;

			case HL4VAR:	
				body_len_n = 4;
				break;
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
	md_magic_len = static_cast<int>(strlen(md_magic));
}

void BufTmr::stamp()
{
	unsigned TEXTUS_LONG interval, nlen, start_sec, start_milli_l;
	unsigned char offset;
	BTool::MD5_CTX Md5Ctx;
	unsigned char yaBuf[16];
	unsigned short start_milli;
	unsigned int i, yaLen = 0;
#if defined (_WIN32)
	unsigned __int64 etik, stik, intv;
	etik = (unsigned __int64)end_tm.dwLowDateTime + (((unsigned __int64)end_tm.dwHighDateTime) << 32);
	stik = (unsigned __int64)start_tm.dwLowDateTime + (((unsigned __int64)start_tm.dwHighDateTime) << 32);
	intv = (etik-stik)/1000;
	interval = (TEXTUS_LONG) intv;
	start_sec =  (TEXTUS_LONG) ((stik - t2k)/10000000);
	start_milli_l =  (TEXTUS_LONG) ((stik - t2k)/1000 - start_sec*10000);
#else
	interval = 1000* (end_tp.tv_sec - start_tp.tv_sec) + (end_tp.tv_nsec - start_tp.tv_nsec)/1000000;
	start_sec =  start_tp.tv_sec - t2k;
	start_milli_l =  start_tp.tv_nsec/100000;
#endif
	start_milli = (unsigned short)start_milli_l;
	BTool::MD5Init (&Md5Ctx);
	BTool::MD5Update (&Md5Ctx, md_magic, md_magic_len);

	if ( gCFG->seq) {
		tmr_pac.input(SEQ_FLD, gCFG->seq, gCFG->seq_len);
		//MD5Update (&Md5Ctx, (char*)gCFG->seq, gCFG->seq_len);
	} 
	if ( gCFG->tag) {
		tmr_pac.input(TAG_FLD, gCFG->tag, gCFG->tag_len);
		//MD5Update (&Md5Ctx, (char*)gCFG->tag, gCFG->tag_len);
	}
	for ( i = 0 ; i < 4 ; i++)
	{
		yaBuf[3-i] = (unsigned char)(start_sec%256);
		start_sec /=256;
	}
	tmr_pac.input(START_SEC_FLD, yaBuf, 4);
	BTool::MD5Update (&Md5Ctx, (char*)yaBuf, 4);

	yaBuf[0] = (unsigned char)(start_milli%256);
	start_milli /=256;
	yaBuf[1] = (unsigned char)(start_milli%256);
	start_milli /=256;
	tmr_pac.input(START_MILLI_FLD, yaBuf, 2);
	BTool::MD5Update (&Md5Ctx, (char*)yaBuf, 2);

	for ( i = 0 ; i < gCFG->time_len ; i++)
	{
		yaBuf[gCFG->time_len-i-1] = (unsigned char)(interval%256);
		interval /=256;
	}
	tmr_pac.input(INTERVAL_FLD, (char*)yaBuf, gCFG->time_len);
	BTool::MD5Update (&Md5Ctx, (char*)yaBuf, yaLen);

	nlen  = rcv_buf->point - rcv_buf->base;
	if ( gCFG->opt)
	{
		tmr_pac.input(DATA_OPT_FLD, gCFG->opt, gCFG->opt_len);
		BTool::MD5Update (&Md5Ctx, (char*)gCFG->opt, gCFG->opt_len);
		nlen += gCFG->opt_len;
	}

	BTool::MD5Update (&Md5Ctx, (char*)rcv_buf->base, static_cast<int>(nlen));
	BTool::MD5Final ((char *) &yaBuf[0], &Md5Ctx);

	tmr_pac.input(MSG_SUM_FLD, yaBuf, MD_SUM_LEN);
	offset = MD_SUM_LEN;
	tmr_pac.input(OFFSET_FLD, &offset, sizeof(offset));

	for ( i = 0 ; i < gCFG->body_len_n ; i++)
	{
		yaBuf[gCFG->body_len_n-1-i] = (unsigned char)(nlen%256);
		nlen /=256;
	}
	tmr_pac.input(BODY_LEN_FLD, yaBuf, gCFG->body_len_n);

	framing = false;
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
			memcpy(&end_tm, &start_tm, sizeof(start_tm));
#else
			clock_gettime(CLOCK_REALTIME, &start_tp);
			memcpy(&end_tp, &start_tp, sizeof(start_tp));
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
		gCFG->sch->sponte(&clr_timer_pius); /* 清除定时, 刚开始为0, 不起作用 */
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
	clr_timer_pius.indic = 0;	/* 预设改为0,  sched模块也更新, 被更新则用于tpoll模块 */

	alarm_pius.ordo = Notitia::DMD_SET_ALARM;
	alarm_pius.indic = &arr[0];
	tmr_pac.produce(32) ;
	pa[0] = &tmr_pac;
	pa[1] = &tmr_pac;
	pa[2] = 0;
}

BufTmr::~BufTmr() 
{
	if ( framing)
		gCFG->sch->sponte(&clr_timer_pius); /* 清除定时 */
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
