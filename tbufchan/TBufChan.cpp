/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
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
 $Header: /textus/tbufchan/TBufChan.cpp 11    14-04-12 13:54 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: TBufChan.cpp $"
#define TEXTUS_MODTIME  "$Date: 14-04-12 13:54 $"
#define TEXTUS_BUILDNO  "$Revision: 11 $"
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

#ifndef TINLINE
#define TINLINE inline
#endif

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

private:
	TBuffer *rcv_buf;
	TBuffer *snd_buf;

	PacketObj *rcv_pac;	/* ������ڵ��PacketObj */
	PacketObj *snd_pac;

	TBuffer house;		/* �ݴ� */
	TBuffer right_rcv;	/* ���ҽڵ���յ����ݻ��� */
	TBuffer right_snd;	/* ���ҽڵ㷢�͵����ݻ��� */

	TINLINE void deliver(Notitia::HERE_ORDO aordo);
	TINLINE void right_reset();
	Amor::Pius pro_tbuf;
	Amor::Pius pro_unipac;
	Amor::Pius dmd_start;

	bool alive;	/* ͨ���Ƿ�� */
	bool demanding;	/* ����Ҫ��ͨ��, ��δ����Ӧ */
	
	struct G_CFG {
		Amor::Pius chn_timeout;
		int expired;	/* ��ʱʱ�䡣0: ���賬ʱ */
		enum MODE mode;
		int pac_fld;	/* PacketObj���Ǹ�����뱾���� */
		bool once;
		bool willAsk;
		
		inline G_CFG ( TiXmlElement *cfg ) {
			const char *comm_str;
			chn_timeout.ordo =  Notitia::CHANNEL_TIMEOUT;
			chn_timeout.indic = 0;
			expired = 0;
			mode = TBUF ;	/* Ĭ�������������TBUF, UNIPAC�����PacketObj, HYBRID��ͬʱ��������������� */
			pac_fld = -1;
			once = false;
			willAsk = true;

			cfg->QueryIntAttribute("field", &(pac_fld));
			comm_str = cfg->Attribute("once");
			if ( comm_str && strcasecmp(comm_str, "yes" ) ==0 )
				once = true;

			comm_str = cfg->Attribute("demand");
			if ( comm_str && strcasecmp(comm_str, "no" ) ==0 )
				willAsk = false;

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
		if (fld->no != gCFG->pac_fld )	/* û������� */
			break;

		if (alive ) 
		{
			right_snd.input(fld->val, fld->range);
			aptus->facio(&pro_tbuf);
		} else {
			house.input(fld->val, fld->range); /* ���ݽ����ݴ� */
			if( !demanding && gCFG->willAsk)
			{
				demanding = true;			/* �ñ�־ */
				if( gCFG->expired > 0 )			/* ������˳�ʱ */
					deliver(Notitia::DMD_SET_ALARM);
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

		if (alive )
		{
			TBuffer::pour(right_snd, *rcv_buf);
			aptus->facio(&pro_tbuf);
		} else {
			TBuffer::pour(house, *rcv_buf);	/* ���ݽ����ݴ� */
	Demand:
			if( !demanding && gCFG->willAsk )
			{
				demanding = true;			/* �ñ�־ */
				if( gCFG->expired > 0 )			/* ������˳�ʱ */
					deliver(Notitia::DMD_SET_ALARM);
				aptus->facio(&dmd_start);	/* Ҫ��ʼ */
			}
		}
		break;

	case Notitia::SET_TBUF:	/* ȡ������TBuffer��ַ */
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
		house.reset();	/* �����ǰ��Ѿ��յ����������, �ҽڵ㲻������ */
		break;

	case Notitia::TIMER:	/* ���ӳ�ʱ */
		WBUG("facio TIMER" );
		if ( demanding)
		{
			WLOG(WARNING, "channel time out");
			right_reset();	/* �ұ߽ڵ��������״̬ȫ����λ */
			deliver(Notitia::DMD_CLR_TIMER);	/* �����ʱ */
			aptus->sponte(&(gCFG->chn_timeout));	/* ����֪ͨ */
		}
		break;

	case Notitia::IGNITE_ALL_READY:	
		WBUG("facio IGNITE_ALL_READY" );			
		goto ALL_READY;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE(IGNITE)_ALL_READY" );			
ALL_READY:
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

		if ( demanding )
		{
			if ( gCFG->expired > 0 )
				deliver(Notitia::DMD_CLR_TIMER);
		} 
		
		right_reset();	//�����alive, demanding%��false��
		if ( house.point > house.base )
		{
			TBuffer::pour(right_snd, house);
			aptus->facio(&pro_tbuf);
		}
		alive = !gCFG->once; /* ͨ��һ����ʹ��, �������ݺ�, aliveΪfalse�� ͨ����־������, ����һ����ʱ�ر������� */

		break;

	case Notitia::DMD_END_SESSION:	/* channel closed */
		WBUG("sponte DMD_END_SESSION");
		if ( demanding && gCFG->expired > 0 )
		{
			deliver(Notitia::DMD_CLR_TIMER);
			//aptus->sponte(&(gCFG->chn_timeout));	/* ����֪ͨͨ����ʱ?, ���ذ�, ��ΪEND_SESSION��֪ͨ���� */
		}
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
}

TBufChan::~TBufChan() 
{
	if ( demanding && gCFG->expired > 0 )
		deliver(Notitia::DMD_CLR_TIMER);
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

TINLINE void TBufChan::right_reset()
{
	right_rcv.reset();
	right_snd.reset();
	alive = false;
	demanding = false;
}

/* ��������ύ */
TINLINE void TBufChan::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	void *arr[3];
	TBuffer *tb[3];
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	
	switch (aordo)
	{
	case Notitia::DMD_CLR_TIMER:
		WBUG("deliver(sponte) DMD_CLR_TIMER");
		tmp_pius.indic = (void*) this;
		aptus->sponte(&tmp_pius);
		goto END ;

	case Notitia::DMD_SET_ALARM:
		WBUG("deliver(sponte) DMD_SET_ALARM");
		tmp_pius.indic = &arr[0];
		arr[0] = this;
		arr[1] = &(gCFG->expired);
		arr[2] = 0;
		aptus->sponte(&tmp_pius);
		goto END;

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
END:
	return ;
}

#include "hook.c"
