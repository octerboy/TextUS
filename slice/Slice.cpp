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
 Build: created by octerboy, 2005/06/10
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

#define INLINE inline
class Slice: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Amor::Pius*);
	bool sponte( Amor::Pius*);
	Amor *clone();
	
	Slice();
	~Slice();

	enum HEAD_TYPE {  
		RIGID = 0, 	/* �̶����� */
		SJL06 = 1,	/* ����ֽڱ�ʾ����, �����ֽ�˳��, ��power pcƽ̨���ֽ�˳��,
				����������SJL06��ͨѶ���˽����ַ�ʽ.
				��ʽΪ:
				char[0] << 8 + char[1], 
				char[0]��char[1]�ֱ�Ϊ�����еĵ�1�͵�2�ֽ�
				*/
		CUPS = 2,	/* 10�����ַ���ʾ, �� "0023" ��ʾ23���ֽ� */
		TERM = 3,	/* ��ĳ���򼸸��ַ�Ϊһ֡����, ��󴫵�ʱȥ���⼸�������� */
		MYSQL = 4,	/* ����ֽڱ�ʾ����, x86ƽ̨�������ֽ�˳��,
				����������mysql��ͨѶ���˽����ַ�ʽ.
				��ʽΪ:
				char[0] + char[1] << 8, 
				char[0]��char[1]�ֱ�Ϊ�����еĵ�1�͵�2�ֽ�
				*/
		UNDEFINED = -1	/* δ���� */
	};

private:
	Amor::Pius local_p;
	Amor::Pius clr_timer_pius, alarm_pius;	/* �峬ʱ, �賬ʱ */
	TBuffer *ask_tb, *res_tb;	//��һ����������, ������ԭʼ����֡������
	TBuffer *ask_pa, *res_pa;		//��һ�������ݻ�����, ������֡����������ԭʼ������
	TBuffer r1st, r2nd;		//��һ�������ݻ�����, ������֡����������ԭʼ������

	bool isFraming;			/* �Ƿ���һ֡�ķ������ڽ��� */
	void *arr[3];

	struct G_CFG { 
		bool onlyOnce;	/* �ж�����ݰ�ֻ������һ�� */
		bool inverse;	/* �Ƿ������ */
		unsigned char *term_str;	/* �����ַ��� */
		unsigned int term_len;

		unsigned char *pre_ana_str;	/* ������ʼ�ַ��� */
		unsigned int pre_ana_len;

		unsigned char *pre_comp_str;	/* ��Ӧ��ͷ�ַ��� */
		unsigned int pre_comp_len;
		int adjust;	/* ����ֵ����, ���ڳ���ֵ�������������ĳ��ȵ�, ��ֵΪ 0
						��ʱ������ֵֻ�Ǳ��ĵĲ���, ����2�ֽڳ���ͷ��ʾ����ĳ���,
						����ֵΪ2 
					 */
		unsigned char len_size;		/* ��ʾ���ĳ��ȵ��ֽ��� */
		unsigned int max_len;	/* ֡����󳤶� */
		unsigned int min_len;	/* ֡����С���� */
		unsigned int offset;		/* ƫ����, ���������ݵ�ƫ��offset���ֽ������㳤�� */
		int timeout;		/* һ֡��������ʱ��, ���� */

		HEAD_TYPE head_types;
		unsigned long fixed_len ;

		inline G_CFG (TiXmlElement *cfg) 
		{
			const char *inverse_str, *comm_str, *type_str;
			adjust = 0;

			max_len = 8192;
			min_len = 0;
			timeout = 0;
			offset	= 0;
			len_size = 0;

			term_str = (unsigned char*)0;
			term_len = 0;
			pre_ana_str = (unsigned char*)0;
			pre_ana_len = 0;
			pre_comp_str = (unsigned char*)0;
			pre_comp_len = 0;
			head_types = UNDEFINED;
			onlyOnce = false;

			inverse_str = cfg->Attribute ("inverse");
			inverse = (inverse_str != (const char*) 0  &&  strcasecmp(inverse_str, "yes") == 0 );

			comm_str = cfg->Attribute ("once");
			if ( comm_str && (comm_str[0] == 'y' || comm_str[0] == 'Y') )
				onlyOnce = true;

			comm_str = cfg->Attribute ("rcv_guide");
			if ( comm_str ) {
				pre_ana_len = strlen(comm_str);
				pre_ana_str= new unsigned char[pre_ana_len];
				pre_ana_len = BTool::unescape(comm_str, pre_ana_str);
			}

			comm_str = cfg->Attribute ("snd_guide");
			if ( comm_str ) {
				pre_comp_len = strlen(comm_str);
				pre_comp_str= new unsigned char[pre_comp_len];
				pre_comp_len = BTool::unescape(comm_str, pre_comp_str);
			}

			type_str = cfg->Attribute("length");	/* ȡ�ó��ȷ�ʽ */
			if ( type_str )
			{
				if ( atoi(type_str) > 0 )
				{
					head_types = RIGID;
					fixed_len = atol(type_str);
					len_size = 0;

				} else if ( strncasecmp(type_str, "octet:", 6) == 0 )
				{
					len_size = atoi(&type_str[6]);
					head_types = SJL06;

				} else if ( strncasecmp(type_str, "noctet:", 7) == 0 )
				{
					len_size = atoi(&type_str[7]);
					head_types = SJL06;

				} else if ( strncasecmp(type_str, "hoctet:", 7) == 0 )
				{
					len_size = atoi(&type_str[7]);
					head_types = MYSQL;

				} else if ( strncasecmp(type_str, "string:", 7) == 0 )
				{
					len_size = atoi(&type_str[7]);
					head_types = CUPS;

				} else if ( strncasecmp(type_str, "term:", 5) == 0 )
				{
					term_len = strlen(&type_str[5]);
					term_str= new unsigned char[term_len];
					term_len = BTool::unescape(&type_str[5], term_str);
					head_types = TERM;
				}
			}

			if ( head_types < 0 ) return ;

			if ( len_size > 15 ) 	/* ��ʾ���ȵ��ֽ������Ϊ15 */
				len_size = 15;

			cfg->QueryIntAttribute("adjust", &(adjust));	/* ���ȵ����� */

		#define GETINT(x,y) \
			comm_str = cfg->Attribute (y);	\
			if ( comm_str &&  atoi(comm_str) > 0 )	\
			x = atoi(comm_str);
	
			GETINT(offset, "offset");

			GETINT(timeout, "timeout");
			GETINT(max_len, "maximum");
			GETINT(min_len, "minimum");
		};

		inline ~G_CFG() {
			if ( term_str ) 
				delete[] term_str;
			if ( pre_ana_str ) 
				delete[] pre_ana_str;
			if ( pre_comp_str ) 
				delete[] pre_comp_str;
		};
	};

	struct G_CFG *gCFG;     /* ȫ�ֹ������ */
	bool has_config;

	INLINE void deliver(Notitia::HERE_ORDO aordo, bool inver=false);
	INLINE bool analyze(TBuffer *raw, TBuffer *plain);
	INLINE bool analyze_term(TBuffer *raw, TBuffer *plain);
	INLINE bool compose(TBuffer *raw, TBuffer *plain);
	INLINE bool compose_term(TBuffer *raw, TBuffer *plain);
	INLINE unsigned long int getl(unsigned char *);
	INLINE void putl(unsigned long int, unsigned char *);
	INLINE void reset();
#include "wlog.h"
};

#include <assert.h>

#define BZERO(X) memset(X, 0 ,sizeof(X))
void Slice::ignite(TiXmlElement *cfg)
{
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
}

bool Slice::facio( Amor::Pius *pius)
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
				if ( !gCFG->onlyOnce && ask_tb->point > ask_tb->base)
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
		reset();
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		goto ALL_READY;
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
ALL_READY:
		arr[0] = this;
		arr[1] = &(gCFG->timeout);
		arr[2] = 0;
		deliver(Notitia::SET_TBUF);//���һ�����ݱ����TBUFFER�����ַ
		break;

	case Notitia::TIMER:	/* ��ʱ�ź� */
		WBUG("facio TIMER" );
		if ( isFraming)
		{
			WLOG(WARNING, "facio encounter time out");
			deliver(Notitia::ERR_FRAME_TIMEOUT, gCFG->inverse);
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

bool Slice::sponte( Amor::Pius *pius)
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
				if ( !gCFG->onlyOnce && res_pa->point > res_pa->base )
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
		WBUG("sponte DMD_END_SESSION" );
		reset();
		break;

	default:
		return false;
	}
	return true;
}

Slice::Slice()
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
	clr_timer_pius.ordo = Notitia::DMD_CLR_TIMER;
	clr_timer_pius.indic = 0;

	alarm_pius.ordo = Notitia::DMD_SET_TIMER;
	alarm_pius.indic = this;
}

Slice::~Slice()
{
	aptus->sponte(&clr_timer_pius);
	if ( has_config && gCFG )
		delete gCFG;
}

/* ��������ύ */
INLINE void Slice::deliver(Notitia::HERE_ORDO aordo, bool _inver)
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

Amor* Slice::clone()
{
	Slice *child;
	child = new Slice();

	child->gCFG =gCFG; 
	return (Amor*)child;
}

INLINE bool Slice::analyze(TBuffer *raw, TBuffer *plain)
{
	/* ���ֽڱ�ʾ�������ݵĳ��� */
	unsigned long frameLen = 0;
	unsigned long len = raw->point - raw->base;	//���ݳ���

	if (len == 0 ) return false;	/* û������, Ҳ����������Ե��� */
	if ( gCFG->head_types == TERM )
		return analyze_term(raw, plain);

	WBUG("analyze raw length is %lu ", len);
	if ( len < gCFG->offset + gCFG->len_size )
	{ 		
		if (!isFraming)
		{	//���һ�ο�ʼ
			isFraming = true;
			aptus->sponte(&alarm_pius);
		}
		return false;		//����ͷ����
	}
	
	if ( gCFG->offset > 0 ) {
		if ( memcmp(raw->base, gCFG->pre_ana_str, gCFG->pre_ana_len ) != 0 ) 
		{
			WLOG(WARNING, "analyze encounter invalid string");
			aptus->sponte(&clr_timer_pius);	//�����Ѿ���ʱ
			reset();
			return false;
		}
	}
	frameLen = getl(&raw->base[gCFG->offset]);	/* frameLen �����������ݳ���, ����ֵ��adjust */
	//֡�Ĵ�С���
	if ( frameLen < gCFG->min_len || frameLen > gCFG->max_len )
	{	//�쳣����, ��������
		WLOG(WARNING, "analyze encounter too large/small frame, the length is %lu", frameLen);
		aptus->sponte(&clr_timer_pius);	//�����Ѿ���ʱ
		deliver(Notitia::ERR_FRAME_LENGTH, gCFG->inverse);
		reset();
		return false;
	}

	if  ( len >= frameLen ) 
	{	/* �յ�һ֡���� */
		TBuffer::pour(*plain, *raw, frameLen); /* ���� */
		if ( isFraming ) 
		{
			isFraming = false;	//��ʹһ֡��ʼ��, ���ﶼ������
			aptus->sponte(&clr_timer_pius);
		}
		return true;
	} else
	{	//����ͷ����, ����δ���
		if (!isFraming)
		{	//���һ�ο�ʼ, ��һ�¶�ʱ
			isFraming = true;
			aptus->sponte(&alarm_pius);
		}
		return false;
	}
}

INLINE bool Slice::compose(TBuffer *raw, TBuffer *plain)
{
	unsigned char *where;
	long len = plain->point - plain->base;	/* Ӧ�����ݳ���, Ӧ������Ӧ������������ͷ�Ŀռ� */
	long hasLen = raw->point - raw->base;	/* ԭ�����ݳ��� */

	WBUG("compose all length is %lu ", len);
	if ( len<=0 ) 
		WLOG(NOTICE, "compose zero length");
	if ( gCFG->head_types == TERM )
		return compose_term(raw, plain);

	//printf("plaing space %d,  free %d\n", plain->limit - plain->base, plain->point - plain->base);
	//printf("raw space %d,  free %d\n", raw->limit - raw->base, raw->point - raw->base);

	TBuffer::pour(*raw, *plain);		/* ����Ӧ������, �������ֱ�ӽ����ڲ���ַ,raw->base��� */
	where = &raw->base[hasLen+gCFG->offset];	/* �ó�����ֵ���ڵľ����ƫ��λ�� */
	if ( gCFG->offset > 0 ) {
		memcpy(raw->base, gCFG->pre_comp_str, gCFG->pre_comp_len);
	}
	putl(len, where);
	
	return true;
}

INLINE  unsigned long Slice::getl(unsigned char *buf)
{
	int i;
	unsigned long frameLen = 0;
	char l_str[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	
	switch ( gCFG->head_types)
	{
	case RIGID:
		frameLen = gCFG->fixed_len;
		break;

	case SJL06:
		for ( i = 0 ; i < gCFG->len_size; i++)
		{
			frameLen <<= 8;	
			frameLen += buf[i];
		}
		break;

	case MYSQL:
		for ( i = gCFG->len_size-1; i >=0;  i--)
		{
			frameLen <<= 8;	
			frameLen += buf[i];
		}
		break;

	case CUPS:
		memcpy(l_str, buf, gCFG->len_size);
		frameLen = atol(l_str);
		break;
	default:
		WLOG(WARNING, "unkown head type %d",gCFG->head_types);
		break;
	}

	WBUG("getl length is %lu from frame", frameLen);
	frameLen += gCFG->adjust;	/* ����Ϊ�������ݵĳ��� */
	return frameLen;
}

INLINE void Slice::putl(unsigned long frameLen, unsigned char *pac)
{
	int i;
	frameLen -= gCFG->adjust;	/* ԭֵΪ�������ݳ���, ��ȥ����ֵ */
	
	WBUG("putl length is %lu to frame", frameLen);
	switch ( gCFG->head_types)
	{
	case RIGID:
		if ( frameLen != gCFG->fixed_len )	/* ���ڹ̶�����, ��������־, �����������ݷ��� */
			WLOG(NOTICE, "putl length %ld is not equal to %ld", frameLen, gCFG->fixed_len );
		break;

	case SJL06:
		for ( i = gCFG->len_size - 1; i >=0 ; i-- )
		{
			pac[i] = (unsigned char ) (frameLen & 0xff);
			frameLen >>= 8;
		}
		break;

	case MYSQL:
		for ( i =0 ; i < gCFG->len_size;  i++ )
		{
			pac[i] = (unsigned char ) (frameLen & 0xff);
			frameLen >>= 8;
		}
		break;

	case CUPS:
		for ( i = gCFG->len_size - 1; i >=0 ; i-- )
		{
			pac[i] = (unsigned char)('0' + frameLen%10);
			frameLen /= 10;
		}
		break;

	default:
		WLOG(WARNING, "unkown head type %d",gCFG->head_types);
		break;
	}
}

INLINE bool Slice::analyze_term(TBuffer *raw, TBuffer *plain)
{
	long frameLen = -1;
	unsigned char* ptr = raw->base;
	bool ret = false;
	
	//printf("term %s", raw->base);
	for ( ; ptr <= raw->point - gCFG->term_len; ptr++)
	{
		if ( memcmp( ptr, gCFG->term_str, gCFG->term_len) == 0 )
		{
			frameLen = ptr - raw->base;
			frameLen += gCFG->term_len;	/* ���������ڶȳ��� */
			break;
		}
	}

	//printf("frameLen %d\n", frameLen);
	if ( frameLen == -1 )
	{	/* һ֡���ڽ����� */
		if (!isFraming)
		{	//���һ�ο�ʼ, ��ʱ
			isFraming = true;
			aptus->sponte(&alarm_pius);
		}
		ret = false;
	} else {
		/* �Ѿ�������һ֡�� */
		if (isFraming)
		{
			isFraming = false;	//��ʹһ֡��ʼ��, ���ﶼ������
			aptus->sponte(&clr_timer_pius);
		}
		if ( frameLen > 0 ) 	/* �����ݵ�֡������ */
		{
			TBuffer::pour(*plain, *raw, frameLen); /* ���� */
			
			ret = true;
		}
	}

	return ret;
}

INLINE bool Slice::compose_term(TBuffer *raw, TBuffer *plain)
{
	long len = plain->point - plain->base;	/* ���ݳ���, ������β�ַ��ռ� */
	long hasLen = raw->point - raw->base;	/* ԭ�����ݳ��� */

	if ( len ==0 ) 
	{
		WLOG(NOTICE, "compose zero length");
		return true;
	}

	TBuffer::pour(*raw, *plain);		/* ���Ͼ����Ӧ������,  */
	memcpy(raw->base + len+hasLen - gCFG->term_len, gCFG->term_str, gCFG->term_len); /* ���Ͻ����� */
	return true;
}

INLINE void Slice::reset()
{
	ask_pa->reset();
	res_pa->reset();
	isFraming = false;
	aptus->sponte(&clr_timer_pius);	/* ��ʼΪ0 */
}

#include "hook.c"
