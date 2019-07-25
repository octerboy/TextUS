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
		RIGID = 0, 	/* 固定类型 */
		SJL06 = 1,	/* 多个字节表示长度, 网络字节顺序, 即power pc平台的字节顺序,
				编程者最早从SJL06的通讯中了解这种方式.
				方式为:
				char[0] << 8 + char[1], 
				char[0]和char[1]分别为传输中的第1和第2字节
				*/
		CUPS = 2,	/* 10进制字符表示, 如 "0023" 表示23个字节 */
		TERM = 3,	/* 以某个或几个字符为一帧结束, 向后传递时去掉这几个结束符 */
		MYSQL = 4,	/* 多个字节表示长度, x86平台的主机字节顺序,
				编程者最早从mysql的通讯中了解这种方式.
				方式为:
				char[0] + char[1] << 8, 
				char[0]和char[1]分别为传输中的第1和第2字节
				*/
		UNDEFINED = -1	/* 未定义 */
	};

private:
	Amor::Pius local_p;
	Amor::Pius clr_timer_pius, alarm_pius;	/* 清超时, 设超时 */
	TBuffer *ask_tb, *res_tb;	//上一级的请求区, 可能是原始或是帧分析区
	TBuffer *ask_pa, *res_pa;		//下一级的数据缓冲区, 可能是帧分析区或是原始数据区
	TBuffer r1st, r2nd;		//下一级的数据缓冲区, 可能是帧分析区或是原始数据区

	bool isFraming;			/* 是否在一帧的分析正在进行 */
	void *arr[3];

	struct G_CFG { 
		bool onlyOnce;	/* 有多个数据包只分析第一个 */
		bool inverse;	/* 是否反向分析 */
		unsigned char *term_str;	/* 结束字符串 */
		unsigned int term_len;

		unsigned char *pre_ana_str;	/* 接收起始字符串 */
		unsigned int pre_ana_len;

		unsigned char *pre_comp_str;	/* 响应开头字符串 */
		unsigned int pre_comp_len;
		int adjust;	/* 长度值调整, 对于长度值包括了整个报文长度的, 此值为 0
						有时，长度值只是报文的部分, 比如2字节长度头表示后面的长度,
						调整值为2 
					 */
		unsigned char len_size;		/* 表示报文长度的字节数 */
		unsigned int max_len;	/* 帧的最大长度 */
		unsigned int min_len;	/* 帧的最小长度 */
		unsigned int offset;		/* 偏移量, 从所接数据的偏移offset个字节数计算长度 */
		int timeout;		/* 一帧的最大接收时间, 秒数 */

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

			type_str = cfg->Attribute("length");	/* 取得长度方式 */
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

			if ( len_size > 15 ) 	/* 表示长度的字节数最大为15 */
				len_size = 15;

			cfg->QueryIntAttribute("adjust", &(adjust));	/* 长度调整量 */

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

	struct G_CFG *gCFG;     /* 全局共享参数 */
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
				if ( !gCFG->onlyOnce && ask_tb->point > ask_tb->base)
					goto ALOOP; //再分析一次
			}
		} else {
			pro_end = compose(ask_pa, ask_tb);
			if ( pro_end )
				aptus->facio(&local_p);
		}
				break;
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
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
ALL_READY:
		arr[0] = this;
		arr[1] = &(gCFG->timeout);
		arr[2] = 0;
		deliver(Notitia::SET_TBUF);//向后一级传递本类的TBUFFER对象地址
		break;

	case Notitia::TIMER:	/* 超时信号 */
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
				if ( !gCFG->onlyOnce && res_pa->point > res_pa->base )
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
	isFraming = false;	//还没有开始接收帧

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

/* 向接力者提交 */
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
	/* 两字节表示后续数据的长度 */
	unsigned long frameLen = 0;
	unsigned long len = raw->point - raw->base;	//数据长度

	if (len == 0 ) return false;	/* 没有数据, 也许被其它程序吃掉了 */
	if ( gCFG->head_types == TERM )
		return analyze_term(raw, plain);

	WBUG("analyze raw length is %lu ", len);
	if ( len < gCFG->offset + gCFG->len_size )
	{ 		
		if (!isFraming)
		{	//这第一次开始
			isFraming = true;
			aptus->sponte(&alarm_pius);
		}
		return false;		//不够头长度
	}
	
	if ( gCFG->offset > 0 ) {
		if ( memcmp(raw->base, gCFG->pre_ana_str, gCFG->pre_ana_len ) != 0 ) 
		{
			WLOG(WARNING, "analyze encounter invalid string");
			aptus->sponte(&clr_timer_pius);	//可能已经定时
			reset();
			return false;
		}
	}
	frameLen = getl(&raw->base[gCFG->offset]);	/* frameLen 包括所有数据长度, 计算值加adjust */
	//帧的大小检查
	if ( frameLen < gCFG->min_len || frameLen > gCFG->max_len )
	{	//异常数据, 丢弃而已
		WLOG(WARNING, "analyze encounter too large/small frame, the length is %lu", frameLen);
		aptus->sponte(&clr_timer_pius);	//可能已经定时
		deliver(Notitia::ERR_FRAME_LENGTH, gCFG->inverse);
		reset();
		return false;
	}

	if  ( len >= frameLen ) 
	{	/* 收到一帧数据 */
		TBuffer::pour(*plain, *raw, frameLen); /* 倒入 */
		if ( isFraming ) 
		{
			isFraming = false;	//即使一帧开始了, 这里都结束了
			aptus->sponte(&clr_timer_pius);
		}
		return true;
	} else
	{	//数据头有了, 但还未完成
		if (!isFraming)
		{	//这第一次开始, 设一下定时
			isFraming = true;
			aptus->sponte(&alarm_pius);
		}
		return false;
	}
}

INLINE bool Slice::compose(TBuffer *raw, TBuffer *plain)
{
	unsigned char *where;
	long len = plain->point - plain->base;	/* 应用数据长度, 应用数据应留出包括长度头的空间 */
	long hasLen = raw->point - raw->base;	/* 原有数据长度 */

	WBUG("compose all length is %lu ", len);
	if ( len<=0 ) 
		WLOG(NOTICE, "compose zero length");
	if ( gCFG->head_types == TERM )
		return compose_term(raw, plain);

	//printf("plaing space %d,  free %d\n", plain->limit - plain->base, plain->point - plain->base);
	//printf("raw space %d,  free %d\n", raw->limit - raw->base, raw->point - raw->base);

	TBuffer::pour(*raw, *plain);		/* 加上应用数据, 这可能是直接交换内部地址,raw->base会变 */
	where = &raw->base[hasLen+gCFG->offset];	/* 得出长度值所在的具体的偏移位置 */
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
	frameLen += gCFG->adjust;	/* 调整为所有数据的长度 */
	return frameLen;
}

INLINE void Slice::putl(unsigned long frameLen, unsigned char *pac)
{
	int i;
	frameLen -= gCFG->adjust;	/* 原值为所有数据长度, 减去调整值 */
	
	WBUG("putl length is %lu to frame", frameLen);
	switch ( gCFG->head_types)
	{
	case RIGID:
		if ( frameLen != gCFG->fixed_len )	/* 对于固定长度, 不等则日志, 但仍允许数据发出 */
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
			frameLen += gCFG->term_len;	/* 所有数据内度长度 */
			break;
		}
	}

	//printf("frameLen %d\n", frameLen);
	if ( frameLen == -1 )
	{	/* 一帧正在进行中 */
		if (!isFraming)
		{	//这第一次开始, 定时
			isFraming = true;
			aptus->sponte(&alarm_pius);
		}
		ret = false;
	} else {
		/* 已经有完整一帧了 */
		if (isFraming)
		{
			isFraming = false;	//即使一帧开始了, 这里都结束了
			aptus->sponte(&clr_timer_pius);
		}
		if ( frameLen > 0 ) 	/* 空数据的帧不往后传 */
		{
			TBuffer::pour(*plain, *raw, frameLen); /* 倒入 */
			
			ret = true;
		}
	}

	return ret;
}

INLINE bool Slice::compose_term(TBuffer *raw, TBuffer *plain)
{
	long len = plain->point - plain->base;	/* 数据长度, 留出结尾字符空间 */
	long hasLen = raw->point - raw->base;	/* 原有数据长度 */

	if ( len ==0 ) 
	{
		WLOG(NOTICE, "compose zero length");
		return true;
	}

	TBuffer::pour(*raw, *plain);		/* 加上具体的应用数据,  */
	memcpy(raw->base + len+hasLen - gCFG->term_len, gCFG->term_str, gCFG->term_len); /* 加上结束符 */
	return true;
}

INLINE void Slice::reset()
{
	ask_pa->reset();
	res_pa->reset();
	isFraming = false;
	aptus->sponte(&clr_timer_pius);	/* 初始为0 */
}

#include "hook.c"
