/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Obtain a frame from the flow
 Build: created by octerboy, 2007/10/24
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "textus_string.h"
#include "TBuffer.h"
#include "BTool.h"
#include "casecmp.h"
#include <time.h>
#include <stdarg.h>

class NacFrame: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Amor::Pius*);
	bool sponte( Amor::Pius*);
	Amor *clone();
	
	NacFrame();
	~NacFrame();

private:
	Amor::Pius local_p;
	Amor::Pius clr_timer_pius, alarm_pius;	/* 清超时, 设超时 */
	TBuffer *ask_tb, *res_tb;	//上一级的请求区, 可能是原始或是帧分析区
	TBuffer *ask_pa, *res_pa;		//下一级的数据缓冲区, 可能是帧分析区或是原始数据区
	TBuffer r1st, r2nd;		//下一级的数据缓冲区, 可能是帧分析区或是原始数据区

	bool isFraming;			/* 是否在一帧的分析正在进行 */
	void *arr[3];

	struct G_CFG { 
		bool inverse;	/* 是否反向分析 */
		int max_len;	/* 帧的最大长度 */
		int min_len;	/* 帧的最小长度 */
		int timeout;

		inline G_CFG (TiXmlElement *cfg) {
			const char *inverse_str, *comm_str;
			inverse = false;
			max_len = 8192;
			min_len = 0;
			timeout = 0;

			if( !cfg) return;
			inverse= (inverse_str = cfg->Attribute ("inverse")) && strcasecmp(inverse_str, "yes") == 0;
#define GETINT(x,y) \
	comm_str = cfg->Attribute (y);	\
	if ( comm_str &&  atoi(comm_str) > 0 )	\
			x = atoi(comm_str);
	
			GETINT(timeout, "timeout");
			GETINT(max_len, "maximum");
			GETINT(min_len, "minimum");
			if ( max_len > 9999 )
				max_len = 9999;
		};

		inline ~G_CFG() { };
	};

	struct G_CFG *gCFG;     /* 全局共享参数 */
	bool has_config;

	void deliver(Notitia::HERE_ORDO aordo, bool inver=false);
	bool analyze(TBuffer *raw, TBuffer *plain);
	bool compose(TBuffer *raw, TBuffer *plain);
	TEXTUS_LONG getl(unsigned char *);
	void putl(TEXTUS_LONG , unsigned char *);
	unsigned char checkSum( unsigned char *buf, TEXTUS_LONG);
	void reset()
	{
		ask_pa->reset();
		res_pa->reset();
		isFraming = false;
	}
#include "wlog.h"
};

#include <assert.h>

#define BZERO(X) memset(X, 0 ,sizeof(X))
void NacFrame::ignite(TiXmlElement *cfg)
{
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
}

bool NacFrame::facio( Amor::Pius *pius)
{
	TBuffer **tb = 0;
	bool pro_end= false;

	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF:	/* TBuffer中有数据,进行处理 */
		WBUG("facio PRO_TBUF");
		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//如果tb不为NULL，则*tb应当是rcv_buf，否则在以前的SET_TBUF中就应已设置
			if ( *tb) ask_tb = *tb; //新到请求的TBuffer
			tb++;
			if ( *tb) res_tb = *tb;
		}

		if ( !ask_tb || !res_tb )
		{	//当然输入输出得已经准备好
			WLOG(WARNING, "PRO_TBUF null");
			break;
		}

		if ( !gCFG->inverse)
		{
		ALOOP:
			pro_end = analyze(ask_tb, ask_pa);
			if (pro_end)	//数据包截取完成,向高层提交
			{
				aptus->facio(&local_p);
				if ( ask_tb->point > ask_tb->base)
					goto ALOOP; //再分析一次
			}
		} else {
			pro_end = compose(ask_pa, ask_tb);
			if ( pro_end )
				aptus->facio(&local_p);
		}
		break;

	case Notitia::SET_TBUF:	/* 取得TBuffer地址 */
		WBUG("facio SET_TBUF");
		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//当然tb不能为空
			if ( *tb) ask_tb = *tb; //新到请求的TBuffer
			tb++;
			if ( *tb) res_tb = *tb;
		} else 
			WLOG(WARNING,"SET_TBUF null");
		break;

	case Notitia::START_SESSION:	/* 底层通信初始, 当然这里也初始 */
		WBUG("facio START_SESSION");
		reset();
		break;

	case Notitia::DMD_END_SESSION:	
		WBUG("facio DMD_END_SESSION");
		reset();
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		goto ALL_READY;
		//break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
ALL_READY:
		arr[0] = this;
		arr[1] = &(gCFG->timeout);
		arr[2] = 0;
		deliver(Notitia::SET_TBUF);//向后一级传递本类的TBUFFER对象地址
		break;

	case Notitia::TIMER:	/* 超时信号, 不用清了 */
		WBUG("facio TIMER" );
		if ( isFraming)
		{
			deliver(Notitia::ERR_FRAME_TIMEOUT, gCFG->inverse);
			WLOG(WARNING, "facio encounter time out");
			reset();
		}
		break;

	case Notitia::TIMER_HANDLE:
		WBUG("facio TIMER_HANDLE");
		clr_timer_pius.indic = pius->indic;
		break;

	default:
		return false;
	}
	return true;
}

bool NacFrame::sponte( Amor::Pius *pius)
{
	TBuffer **tb = 0;
	assert(pius);
	bool pro_end = false;
	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF :	//处理一帧数据而已
		WBUG("sponte PRO_TBUF" );
		if ( !ask_tb || !res_tb )
		{	//当然输入输出得已经准备好
			WLOG(WARNING, "PRO_TBUF null");
			break;
		}

		if ( gCFG->inverse)
		{
		ALOOP:
			pro_end = analyze(res_pa, res_tb);

			if (pro_end)	//数据包截取完成,向高层提交
			{
				aptus->sponte(&local_p);
				if ( res_pa->point > res_pa->base )
					goto ALOOP; //再分析一次
			}
		} else {
			pro_end = compose(res_tb, res_pa);
			if ( pro_end )
				aptus->sponte(&local_p);
		}
		break;

	case Notitia::SET_TBUF:	/* 取得TBuffer地址 */
		WBUG("facio SET_TBUF");
		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//当然tb不能为空
			if ( *tb) ask_pa = *tb; //新到请求的TBuffer
			tb++;
			if ( *tb) res_pa = *tb;
		} else 
			WLOG(WARNING,"SET_TBUF null");
		break;

	case Notitia::START_SESSION:	/* 底层通信初始, 当然这里也初始 */
		WBUG("sponte START_SESSION");
		reset();
		break;

	case Notitia::DMD_END_SESSION:	/* 高级会话关闭了 */
		WBUG("sponte END_SESSION" );
		reset();
		break;

	default:
		return false;
	}
	return true;
}

NacFrame::NacFrame()
{
	local_p.ordo = Notitia::PRO_TBUF;
	local_p.indic = 0;

	ask_tb = 0;
	res_tb = 0;
	
	gCFG = 0;
	has_config = false;
	isFraming = false;	//还没有开始接收帧

	ask_pa = &r1st;
	res_pa = &r2nd;
	clr_timer_pius.ordo = Notitia::DMD_CLR_TIMER;
	clr_timer_pius.indic = 0;

	alarm_pius.ordo = Notitia::DMD_SET_ALARM;
	alarm_pius.indic = arr;
}

NacFrame::~NacFrame()
{
	aptus->sponte(&clr_timer_pius);
	if ( has_config && gCFG )
		delete gCFG;
}

/* 向接力者提交 */
void NacFrame::deliver(Notitia::HERE_ORDO aordo, bool _inver)
{
	Amor::Pius tmp_pius;
	TBuffer *pn[3];

	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	switch ( aordo)
	{
	case Notitia::SET_TBUF:
		WBUG("deliver SET_TBUF");
		pn[0] = &r1st;
		pn[1] = &r2nd;
		//pn[0] = &ask_pa;
		//pn[1] = &res_pa;
		pn[2] = 0;
		tmp_pius.indic = &pn[0];
		break;

	default:
		WBUG("deliver Notitia::%d", aordo);
		break;
	}
	if ( _inver )
		aptus->sponte(&tmp_pius);
	else
		aptus->facio(&tmp_pius);
	return ;
}

Amor* NacFrame::clone()
{
	NacFrame *child;
	child = new NacFrame();

	child->gCFG =gCFG; 
	return (Amor*)child;
}

#define STX 0x02
#define ETX 0x03
bool NacFrame::analyze(TBuffer *raw, TBuffer *plain)
{
	/* 两字节表示后续数据的长度 */
	bool shouldTiming = false;
	TEXTUS_LONG frameLen = 0;
	TEXTUS_LONG len = raw->point - raw->base;	/* 已收数据长度, 如果是多次接收, 则是收的总长度 */

	WBUG("analyze raw length is " TLONG_FMTu, len);
	if ( !isFraming && len >=1 && raw->base[0] == STX )
	{
		isFraming = true;
		shouldTiming = true;
	} 

	if ( !isFraming)
	{
		WBUG("analyze encouter invalid data to be discarded.");
		raw->reset();
		return false;
	}

	if ( len >=3 )
	{
		TEXTUS_LONG ilen;
		ilen = getl(&raw->base[1]);	/* ilen指有效数据长度 */

		//帧的大小检查
		if ( ilen < gCFG->min_len || ilen > gCFG->max_len )
		{	//异常数据, 丢弃而已
			aptus->sponte(&clr_timer_pius);	//可能已经定时
			WLOG(WARNING, "analyze encounter too large/small frame, the length is %lu", ilen);
			deliver(Notitia::ERR_FRAME_LENGTH, gCFG->inverse);
			reset();
			return false;
		}
		frameLen = ilen + 5;	/* frameLen包括STX ETX等5字节 */
		if ( len < frameLen )
			goto ING_PRO;
	} else {
		goto ING_PRO;
	}

	/* 至此, 已经收到一帧数据 len >= frameLen */
	aptus->sponte(&clr_timer_pius);
	if ( raw->base[frameLen-1] != checkSum(&raw->base[1], frameLen-2) )	/* 检查数据帧 */
	{
		WLOG(WARNING, "analyze encounter checksum error frame");
		deliver(Notitia::ERR_FRAME_LENGTH, gCFG->inverse);
		reset();
		return false;
	}
	TBuffer::pour(*plain, *raw, frameLen-2); /* 倒入 */
	if ( isFraming ) 
	{
		isFraming = false;	//即使一帧开始了, 这里都结束了
	}
	plain->commit(-3); /* 移走STX及长度头 */
	raw->commit(-2);   /* 移走ETX及LRC */
	return true;

ING_PRO:
	if ( shouldTiming)	//帧刚刚开始
		aptus->sponte(&alarm_pius);
	return false;
}

bool NacFrame::compose(TBuffer *raw, TBuffer *plain)
{
	unsigned char *where;
	unsigned char tmp[2];
	TEXTUS_LONG len = plain->point - plain->base;	/* 应用数据长度 */
	TEXTUS_LONG hasLen = raw->point - raw->base;	/* 原有数据长度 */

	WBUG("compose all length is " TLONG_FMTu, len);
	if ( len<=0 ) 
		WLOG(NOTICE, "compose zero length");

	if ( len > gCFG->max_len ) 
	{
		deliver(Notitia::ERR_FRAME_LENGTH, gCFG->inverse);
		WLOG(WARNING, "compose encounter too larg frame, the length is %lu", len);
		reset();
		return false;
	}
	//printf("plaing space %d,  free %d\n", plain->limit - plain->base, plain->point - plain->base);
	//printf("raw space %d,  free %d\n", raw->limit - raw->base, raw->point - raw->base);

	tmp[0] = STX;
	raw->input(tmp,1);
	tmp[0]= tmp[1] = 0x00;
	raw->input(tmp,2);
	TBuffer::pour(*raw, *plain);	/* 加上应用数据 */

	tmp[0] = ETX;
	raw->input(tmp,2);
	where = &raw->base[hasLen+1];	/* 得出长度值所在的具体的偏移位置 */
	putl(len, where);
	raw->base[hasLen+len+4] = checkSum(&raw->base[1], len+3);
	
	return true;
}

TEXTUS_LONG NacFrame::getl(unsigned char *base)
{
	unsigned TEXTUS_LONG len = 0;
	
	len = base[1] & 0x0F;	
	if ( len > 9 ) 
		return -1;

	len += ((base[1] & 0xF0 ) >> 4 ) *10;
	if ( len > 99 ) 
		return -1;

	len += (base[0] & 0x0F ) *100;
	if ( len > 999 )
		return -1;

	len += ((base[0] & 0xF0 ) >> 4 ) *1000;
	if ( len > 9999 )
		return -1;

	WBUG("getl length is " TLONG_FMTu" from frame", len);
	return len;
}

unsigned char  NacFrame::checkSum( unsigned char *buf, TEXTUS_LONG len)
{
	TEXTUS_LONG i;
	unsigned char lrc = 0x00;
	for ( i=0; i<len; i++)
		lrc ^= buf[i];
	return lrc;
}
 
void NacFrame::putl(TEXTUS_LONG nlen, unsigned char *yaBuf)
{
	WBUG("putl length is " TLONG_FMT " to frame", nlen);

	yaBuf[1] = (unsigned char) ((nlen%10) | ((nlen%100)/10) << 4);
	nlen /= 100;
	yaBuf[0] = (unsigned char) ((nlen%10) | (nlen/10) << 4);	
}

#include "hook.c"
