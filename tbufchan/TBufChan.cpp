/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
/**
 Title: HTTP Agent
 Build: created by octerboy, 2006/09/13, Guangzhou
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

class TBufChan: public Amor
{
public:
	int count;
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	TBufChan();
	~TBufChan();
	enum MODE { TBUF, UNIPAC, HYBRID };
	enum DATA_MODE { COPY, MOVE, STILL, CLEAR };

private:
	Amor::Pius clr_timer_pius, alarm_pius;  /* 清超时, 设超时 */
	void *arr[3];
	TBuffer *rcv_buf;
	TBuffer *snd_buf;

	PacketObj *rcv_pac;	/* 来自左节点的PacketObj */
	PacketObj *snd_pac;

	TBuffer house;		/* 暂存 */
	TBuffer right_rcv;	/* 从右节点接收的数据缓冲 */
	TBuffer right_snd;	/* 向右节点发送的数据缓冲 */

	void deliver(Notitia::HERE_ORDO aordo);
	void right_reset();
	Amor::Pius pro_tbuf;
	Amor::Pius pro_unipac;
	Amor::Pius dmd_start;

	bool alive;	/* 通道是否打开 */
	bool demanding;	/* 正在要求通道, 还未有响应 */
	int has_buffered_num;
	
	struct G_CFG {
		Amor::Pius chn_timeout;
		int expired;	/* 超时时间。0: 不设超时 */
		enum MODE mode;
		enum DATA_MODE d_mode;	//是否移走数据, 默认为MOVE, 即将数据从左搬至右边; 有时为COPY, 即为复制, 左边数据还可以作它用
		int pac_fld;	/* PacketObj中那个域进入本缓冲 */
		bool once;
		bool willAsk;
		int max_buffer_times;	//最大缓冲次数
		
		inline G_CFG ( TiXmlElement *cfg ) {
			const char *comm_str;
			chn_timeout.ordo =  Notitia::CHANNEL_TIMEOUT;
			chn_timeout.indic = 0;
			expired = 0;
			mode = TBUF ;	/* 默认情况下是左右TBUF, UNIPAC是左边PacketObj, HYBRID是同时接受左边两种数据 */
			pac_fld = -1;
			once = false;
			willAsk = true;
			max_buffer_times = 16;

			cfg->QueryIntAttribute("max_buffer_times", &(max_buffer_times));
			cfg->QueryIntAttribute("field", &(pac_fld));
			comm_str = cfg->Attribute("once");
			if ( comm_str && strcasecmp(comm_str, "yes" ) ==0 )
				once = true;

			comm_str = cfg->Attribute("demand");
			if ( comm_str && strcasecmp(comm_str, "no" ) ==0 )
				willAsk = false;

			d_mode = MOVE;
			comm_str = cfg->Attribute("data");
			if ( comm_str )
			{ 
				if (strcasecmp(comm_str, "move" ) ==0 ) d_mode = MOVE;
				if (strcasecmp(comm_str, "copy" ) ==0 ) d_mode = COPY;
				if (strcasecmp(comm_str, "still" ) ==0 ) d_mode = STILL;
				if (strcasecmp(comm_str, "clear" ) ==0 ) d_mode = CLEAR;
			}

			cfg->QueryIntAttribute("expired", &(expired));
			comm_str = cfg->Attribute("mode");
			if ( comm_str ) 
			{
				if ( strcasecmp(comm_str, "tbuf" ) ==0 || strcasecmp(comm_str, "tbuff" ) ==0 )
					mode = TBufChan::TBUF;
				else if ( strcasecmp(comm_str, "unipac" ) ==0 )
					mode = TBufChan::UNIPAC;
				else if ( strcasecmp(comm_str, "hybrid" ) ==0 )
					mode = TBufChan::HYBRID;
			}
		};
	};
	struct G_CFG *gCFG;	/* Shared for all objects in this node */
	bool has_config;

#include "wlog.h"
};

void TBufChan::ignite(TiXmlElement *cfg) 
{
	if (!cfg) return;

	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
}

bool TBufChan::facio( Amor::Pius *pius)
{
	TBuffer **tb;
	PacketObj **tmp;

	struct FieldObj *fld;
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::PRO_UNIPAC:
		WBUG("facio PRO_UNIPAC");
		count = 0;
		if ( gCFG->mode != UNIPAC && gCFG->mode != HYBRID ) 
			return false;

		if ( gCFG->pac_fld < 0 || !rcv_pac ||  rcv_pac->max < gCFG->pac_fld ) 
			break;

		fld = &rcv_pac->fld[gCFG->pac_fld];
		if (fld->no != gCFG->pac_fld )	/* 没有这个域 */
			break;

		if (alive ) 
		{
			right_snd.input(fld->val, fld->range);
			aptus->facio(&pro_tbuf);
		} else {
			if ( has_buffered_num == gCFG->max_buffer_times ) 
			{
				house.reset();
				has_buffered_num = 0;
			}
			has_buffered_num++;
			house.input(fld->val, fld->range); /* 数据进入暂存 */
			if( !demanding && gCFG->willAsk)
			{
				demanding = true;			/* 置标志 */
				if( gCFG->expired > 0 )			/* 如果设了超时 */
					aptus->sponte(&alarm_pius);
				aptus->facio(&dmd_start);
			}
		}
		break;

	case Notitia::SET_UNIPAC:
		WBUG("facio SET_UNIPAC");
		if ( gCFG->mode != UNIPAC) 
			return false;

		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) rcv_pac = *tmp; 
			else
				WLOG(WARNING, "facio SET_UNIPAC rcv_pac null");
			tmp++;
			if ( *tmp) snd_pac = *tmp;
			else
				WLOG(WARNING, "facio SET_UNIPAC snd_pac null");
		} else 
			WLOG(WARNING, "facio SET_UNIPAC null");
		
		break;

	case Notitia::DMD_START_SESSION:	
		WBUG("facio DMD_START_SESSION alive=%d", alive);
		aptus->sponte(&clr_timer_pius); /* 清除定时,初为 0*/
		if (!alive)
			goto Demand;
		break;

	case Notitia::PRO_TBUF:	
		WBUG("facio PRO_TBUF alive=%d", alive);
		if ( gCFG->mode != TBUF && gCFG->mode != HYBRID ) 
			return false;
		if ( !rcv_buf)	
		{
			WLOG(WARNING, "rcv_buf is null!");
			break;
		}
		switch ( gCFG->d_mode)
		{
			case STILL:
				goto PRO_TBUF_END;
			case CLEAR:
				rcv_buf->reset();
				goto PRO_TBUF_END;
			default:
				break;
		}

		if (alive )
		{
			switch ( gCFG->d_mode)
			{
			case MOVE:
				TBuffer::pour(right_snd, *rcv_buf);
				break;
			case COPY:
				right_snd.input(rcv_buf->base, rcv_buf->point - rcv_buf->base);
				break;
			default:
				break;
			}
				
			aptus->facio(&pro_tbuf);
		} else {
			if ( has_buffered_num == gCFG->max_buffer_times ) 
			{
				house.reset();
				has_buffered_num = 0;
			}
			has_buffered_num++;

			if ( gCFG->d_mode)
				TBuffer::pour(house, *rcv_buf);	/* 数据进入暂存 */
			else
				house.input(rcv_buf->base, rcv_buf->point - rcv_buf->base);
	Demand:
			if( !demanding && gCFG->willAsk )
			{
				demanding = true;			/* 置标志 */
				if( gCFG->expired > 0 )			/* 如果设了超时 */
					aptus->sponte(&alarm_pius);
				aptus->facio(&dmd_start);	/* 要求开始 */
			}
		}
	PRO_TBUF_END:
		break;

	case Notitia::SET_TBUF:	/* 取得输入TBuffer地址 */
		WBUG("facio SET_TBUF");
		if ( gCFG->mode != TBUF && gCFG->mode != HYBRID ) 
			return false;

		if ( (tb = (TBuffer **)(pius->indic)))
		{
			if ( *tb) rcv_buf = *tb; 
			else
				WLOG(WARNING, "facio SET_TBUF rcv_buf null");
			tb++;
			if ( *tb) snd_buf = *tb;
			else
				WLOG(WARNING, "facio SET_TBUF snd_buf null");
		} else 
			WLOG(WARNING, "facio SET_TBUF null");
		break;

	case Notitia::DMD_END_SESSION:	/* channel to closed */
		WBUG("facio DMD_END_SESSION");
		aptus->sponte(&clr_timer_pius); /* 清除定时,初为 0*/
		house.reset();	/* 仅仅是把已经收到的数据清空, 右节点不作处理 */
		break;

	case Notitia::TIMER:	/* 连接超时 */
		WBUG("facio TIMER" );
		if ( demanding)
		{
			WLOG(WARNING, "channel time out");
			right_reset();	/* 右边节点的数据与状态全部复位 */
			//aptus->sponte(&clr_timer_pius); /* 清除定时, tpoll先清了 */
			aptus->sponte(&(gCFG->chn_timeout));	/* 向左通知 */
		}
		break;

	case Notitia::TIMER_HANDLE:
		WBUG("facio TIMER_HANDLE %p", pius->indic);
		clr_timer_pius.indic = pius->indic;
		break;

	case Notitia::IGNITE_ALL_READY:	
		WBUG("facio IGNITE_ALL_READY" );			
		goto ALL_READY;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE(IGNITE)_ALL_READY" );			
ALL_READY:
		arr[0] = this;
		arr[1] = &(gCFG->expired);
		arr[2] = 0;
		deliver(Notitia::SET_TBUF);
		break;

	default:
		return false;
	}
	return true;
}

bool TBufChan::sponte( Amor::Pius *pius)
{
	assert(pius);
	
	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF:
		WBUG("sponte PRO_TBUF alive=%d", alive);
		if ( !snd_buf)	
		{
			WLOG(WARNING, "snd_buf is null!");
			break;
		}

		switch ( gCFG->mode )
		{
		case UNIPAC:
			if ( snd_pac )
				snd_pac->input(gCFG->pac_fld, snd_buf->base, snd_buf->point - snd_buf->base );
			aptus->sponte(&pro_unipac);
			break;
		case TBUF :
		case HYBRID :
			TBuffer::pour(*snd_buf, right_rcv);
			aptus->sponte(&pro_tbuf);
			break;
		}
		
		break;

	case Notitia::START_SESSION:	/* channel is alive */
		WBUG("sponte START_SESSION");
		aptus->sponte(&clr_timer_pius); /* 清除定时,初为 0*/

//		if ( demanding )
//		{
//			if ( gCFG->expired > 0 )
//				aptus->sponte(&clr_timer_pius); 
//		} 
		
		right_reset();	//在这里，alive, demanding%都false了
		has_buffered_num = 0;
		if ( house.point > house.base )
		{
			TBuffer::pour(right_snd, house);
			aptus->facio(&pro_tbuf);
		}
		alive = !gCFG->once; /* 通道一次性使用, 发了数据后, alive为false， 通道标志不激活, 等下一次用时关闭再连接 */

		break;

	case Notitia::DMD_END_SESSION:	/* channel closed */
		WBUG("sponte DMD_END_SESSION");
		aptus->sponte(&clr_timer_pius); /* 清除定时,初为 0*/
	
	//	if ( demanding && gCFG->expired > 0 )
	//	{
	//		aptus->sponte(&clr_timer_pius); /* 清除定时 */
			//aptus->sponte(&(gCFG->chn_timeout));	/* 向左通知通道超时?, 不必吧, 因为END_SESSION会通知处理 */
	//	}
	
		right_reset();
		break;

	default:
		return false;
	}
	return true;
}

TBufChan::TBufChan():right_rcv(8192), right_snd(8192)
{
	rcv_pac = 0;
	snd_pac = 0;

	rcv_buf = 0;
	snd_buf = 0;

	house.reset();
	right_reset();

	pro_tbuf.indic = 0;
	pro_tbuf.ordo = Notitia::PRO_TBUF;

	dmd_start.indic = 0;
	dmd_start.ordo = Notitia::DMD_START_SESSION;

	gCFG = 0;
	has_config = false ;
	has_buffered_num = 0;
	clr_timer_pius.ordo = Notitia::DMD_CLR_TIMER;
	clr_timer_pius.indic = 0;

	alarm_pius.ordo = Notitia::DMD_SET_ALARM;
	alarm_pius.indic = &arr[0];
}

TBufChan::~TBufChan() 
{
	aptus->sponte(&clr_timer_pius); /* 清除定时,初为 0*/
	if ( has_config && gCFG)
		delete gCFG;
}

Amor* TBufChan::clone()
{
	TBufChan *child;
	child = new TBufChan();
	child->gCFG = gCFG;

	return (Amor*)child;
}

void TBufChan::right_reset()
{
	right_rcv.reset();
	right_snd.reset();
	alive = false;
	demanding = false;
}

/* 向接力者提交 */
void TBufChan::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	TBuffer *tb[3];
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	
	switch (aordo)
	{
	case Notitia::SET_TBUF:
		WBUG("deliver SET_TBUF");
		tb[0] = &right_snd;
		tb[1] = &right_rcv;
		tb[2] = 0;
		tmp_pius.indic = &tb[0];
		break;

	default:
		WBUG("deliver Notitia::%d",aordo);
		break;
	}
	aptus->facio(&tmp_pius);
	return ;
}

#include "hook.c"
