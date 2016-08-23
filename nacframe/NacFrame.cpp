/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
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
 $Header: /textus/nacframe/NacFrame.cpp 4     07-12-11 20:09 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: NacFrame.cpp $"
#define TEXTUS_MODTIME  "$Date: 07-12-11 20:09 $"
#define TEXTUS_BUILDNO  "$Revision: 4 $"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "BTool.h"
#include "casecmp.h"
#include <time.h>
#include <stdarg.h>

#define INLINE inline
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
	TBuffer *ask_tb, *res_tb;	//��һ����������, ������ԭʼ����֡������
	TBuffer *ask_pa, *res_pa;		//��һ�������ݻ�����, ������֡����������ԭʼ������
	TBuffer r1st, r2nd;		//��һ�������ݻ�����, ������֡����������ԭʼ������

	bool isFraming;			/* �Ƿ���һ֡�ķ������ڽ��� */
	time_t when_frame_start;		/* ĳһ֡�Ŀ�ʼʱ�� */

	struct G_CFG { 
		bool inverse;	/* �Ƿ������ */
		int max_len;	/* ֡����󳤶� */
		int min_len;	/* ֡����С���� */
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

	struct G_CFG *gCFG;     /* ȫ�ֹ������ */
	bool has_config;

	INLINE void deliver(Notitia::HERE_ORDO aordo, bool inver=false);
	INLINE bool analyze(TBuffer *raw, TBuffer *plain);
	INLINE bool compose(TBuffer *raw, TBuffer *plain);
	INLINE long int getl(unsigned char *);
	INLINE void putl(long int, unsigned char *);
	INLINE unsigned char checkSum( unsigned char *buf, int len );
	INLINE void reset();
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
	case Notitia::PRO_TBUF:	/* TBuffer��������,���д��� */
		WBUG("facio PRO_TBUF");
		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//���tb��ΪNULL����*tbӦ����rcv_buf����������ǰ��SET_TBUF�о�Ӧ������
			if ( *tb) ask_tb = *tb; //�µ������TBuffer
			tb++;
			if ( *tb) res_tb = *tb;
		}

		if ( !ask_tb || !res_tb )
		{	//��Ȼ����������Ѿ�׼����
			WLOG(WARNING, "PRO_TBUF null");
			break;
		}

		if ( !gCFG->inverse)
		{
		ALOOP:
			pro_end = analyze(ask_tb, ask_pa);
			if (pro_end)	//���ݰ���ȡ���,��߲��ύ
			{
				aptus->facio(&local_p);
				if ( ask_tb->point > ask_tb->base)
					goto ALOOP; //�ٷ���һ��
			}
		} else {
			pro_end = compose(ask_pa, ask_tb);
			if ( pro_end )
				aptus->facio(&local_p);
		}
				break;
		break;

	case Notitia::SET_TBUF:	/* ȡ��TBuffer��ַ */
		WBUG("facio SET_TBUF");
		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//��Ȼtb����Ϊ��
			if ( *tb) ask_tb = *tb; //�µ������TBuffer
			tb++;
			if ( *tb) res_tb = *tb;
		} else 
			WLOG(WARNING,"SET_TBUF null");
		break;

	case Notitia::START_SESSION:	/* �ײ�ͨ�ų�ʼ, ��Ȼ����Ҳ��ʼ */
		WBUG("facio START_SESSION");
		reset();
		break;

	case Notitia::DMD_END_SESSION:	
		WBUG("facio DMD_END_SESSION");
		//reset();
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		deliver(Notitia::SET_TBUF);//���һ�����ݱ����TBUFFER�����ַ
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		deliver(Notitia::SET_TBUF);//���һ�����ݱ����TBUFFER�����ַ
		break;

	case Notitia::TIMER:	/* ��ʱ�ź� */
		WBUG("facio TIMER" );
		if ( isFraming)
		if ( time(0) - when_frame_start > gCFG->timeout )
		{
			deliver(Notitia::DMD_CLR_TIMER);
			deliver(Notitia::ERR_FRAME_TIMEOUT, gCFG->inverse);
			WLOG(WARNING, "facio encounter time out");
			reset();
		}
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
	case Notitia::PRO_TBUF :	//����һ֡���ݶ���
		WBUG("sponte PRO_TBUF" );
		if ( !ask_tb || !res_tb )
		{	//��Ȼ����������Ѿ�׼����
			WLOG(WARNING, "PRO_TBUF null");
			break;
		}

		if ( gCFG->inverse)
		{
		ALOOP:
			pro_end = analyze(res_pa, res_tb);

			if (pro_end)	//���ݰ���ȡ���,��߲��ύ
			{
				aptus->sponte(&local_p);
				if ( res_pa->point > res_pa->base )
					goto ALOOP; //�ٷ���һ��
			}
		} else {
			pro_end = compose(res_tb, res_pa);
			if ( pro_end )
				aptus->sponte(&local_p);
		}
		break;

	case Notitia::SET_TBUF:	/* ȡ��TBuffer��ַ */
		WBUG("facio SET_TBUF");
		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//��Ȼtb����Ϊ��
			if ( *tb) ask_pa = *tb; //�µ������TBuffer
			tb++;
			if ( *tb) res_pa = *tb;
		} else 
			WLOG(WARNING,"SET_TBUF null");
		break;

	case Notitia::START_SESSION:	/* �ײ�ͨ�ų�ʼ, ��Ȼ����Ҳ��ʼ */
		WBUG("sponte START_SESSION");
		reset();
		break;

	case Notitia::DMD_END_SESSION:	/* �߼��Ự�ر��� */
		WBUG("sponte END_SESSION" );
		//reset();
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
	isFraming = false;	//��û�п�ʼ����֡

	ask_pa = &r1st;
	res_pa = &r2nd;
}

NacFrame::~NacFrame()
{
	deliver(Notitia::DMD_CLR_TIMER);
	if ( has_config && gCFG )
		delete gCFG;
}

/* ��������ύ */
INLINE void NacFrame::deliver(Notitia::HERE_ORDO aordo, bool _inver)
{
	Amor::Pius tmp_pius;
	TBuffer *pn[3];

	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	switch ( aordo)
	{
	case Notitia::DMD_SET_TIMER:
	case Notitia::DMD_CLR_TIMER:
		WBUG("deliver(sponte) DMD_SET_TIMER/DMD_CLR_TIMER (%d)",aordo);
		tmp_pius.indic = (void*) this;
		aptus->sponte(&tmp_pius);
		return ;
		break;

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
INLINE bool NacFrame::analyze(TBuffer *raw, TBuffer *plain)
{
	/* ���ֽڱ�ʾ�������ݵĳ��� */
	bool shouldTiming = false;
	long int frameLen = 0;
	long int len = raw->point - raw->base;	/* �������ݳ���, ����Ƕ�ν���, �����յ��ܳ��� */

	WBUG("analyze raw length is %lu ", len);
	if ( !isFraming && len >=1 && raw->base[0] == STX )
	{
		isFraming = true;
		shouldTiming = true;	/* �п���û��һ������, ��Ҫ�趨��ʱ */
	} 

	if ( !isFraming)
	{
		WBUG("analyze encouter invalid data to be discarded.");
		raw->reset();
		return false;
	}

	if ( len >=3 )
	{
		int ilen;
		ilen = getl(&raw->base[1]);	/* ilenָ��Ч���ݳ��� */

		//֡�Ĵ�С���
		if ( ilen < gCFG->min_len || frameLen > gCFG->max_len )
		{	//�쳣����, ��������
			deliver(Notitia::ERR_FRAME_LENGTH, gCFG->inverse);
			WLOG(WARNING, "analyze encounter too large/small frame, the length is %lu", frameLen);
			reset();
			return false;
		}
		frameLen = ilen + 5;	/* frameLen����STX ETX��5�ֽ� */
	}

	if ( len < 3 || len < frameLen )
	{ 		
		if ( shouldTiming )
		{	//���һ�ο�ʼ, ��һ�¿�ʼʱ��
			time(&when_frame_start);
			isFraming = true;
			deliver(Notitia::DMD_SET_TIMER);
		}
		return false;		//����ͷ����
	}

	if  ( len >= frameLen ) 
	{	/* �յ�һ֡���� */
		if ( raw->base[frameLen-1] != checkSum(&raw->base[1], frameLen-2) )	/* �������֡ */
		{
			deliver(Notitia::ERR_FRAME_LENGTH, gCFG->inverse);
			WLOG(WARNING, "analyze encounter checksum error frame");
			reset();
			return false;
		}
		TBuffer::pour(*plain, *raw, frameLen-2); /* ���� */
		if ( isFraming ) 
		{
			isFraming = false;	//��ʹһ֡��ʼ��, ���ﶼ������
			deliver(Notitia::DMD_CLR_TIMER);
		}
		plain->commit(-3); /* ����STX������ͷ */
		raw->commit(-2);   /* ����ETX��LRC */
		return true;
	}
	return false;
}

INLINE bool NacFrame::compose(TBuffer *raw, TBuffer *plain)
{
	unsigned char *where;
	unsigned char tmp[2];
	long int len = plain->point - plain->base;	/* Ӧ�����ݳ��� */
	long int hasLen = raw->point - raw->base;	/* ԭ�����ݳ��� */

	WBUG("compose all length is %lu ", len);
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
	TBuffer::pour(*raw, *plain);	/* ����Ӧ������ */

	tmp[0] = ETX;
	raw->input(tmp,2);
	where = &raw->base[hasLen+1];	/* �ó�����ֵ���ڵľ����ƫ��λ�� */
	putl(len, where);
	raw->base[hasLen+len+4] = checkSum(&raw->base[1], len+3);
	
	return true;
}

INLINE  long int NacFrame::getl(unsigned char *base)
{
	long int len = 0;
	
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

	WBUG("getl length is %lu from frame", len);
	return len;
}

INLINE unsigned char  NacFrame::checkSum( unsigned char *buf, int len )
{
	int i;
	unsigned char lrc = 0x00;
	for ( i=0; i<len; i++)
		lrc ^= buf[i];
	return lrc;
}
 
INLINE void NacFrame::putl(long int nlen, unsigned char *yaBuf)
{
	WBUG("putl length is %lu to frame", nlen);

	yaBuf[1] = (unsigned char) ((nlen%10) | ((nlen%100)/10) << 4);
	nlen /= 100;
	yaBuf[0] = (unsigned char) ((nlen%10) | (nlen/10) << 4);	
}

INLINE void NacFrame::reset()
{
	ask_pa->reset();
	res_pa->reset();
	isFraming = false;
}

#include "hook.c"
