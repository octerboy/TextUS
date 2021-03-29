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
#include "casecmp.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "DeHead.h"
#include <time.h>
#include <ctype.h>
#include <stdarg.h>

#define HTTPINLINE inline
class HttpAgent: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	HttpAgent();

private:
	TBuffer *rcv_buf;	/* 在http头完成后，这将是http体的内容 */
	TBuffer *snd_buf;

	TBuffer house;		/* 暂存 */
	TBuffer cli_rcv;	/* 从右节点接收的数据缓冲 */
	TBuffer cli_snd;	/* 向右节点发送的数据缓冲 */

	HTTPINLINE void deliver(Notitia::HERE_ORDO aordo);
	HTTPINLINE void reset();
	Amor::Pius fac_tbuf;
	Amor::Pius spo_body;
	bool alive;	/* 向WEB服务端的通道是否打开 */
	TEXTUS_LONG sent_bl;
	TEXTUS_LONG recv_l;
	TEXTUS_LONG res_body_length;

	bool sess;	/* HTTP请求应答开始 */
	bool waitClose;	/* 等服务端结束信号 */
	bool demanding; /* 正在要求通道, 还未有响应 */
	DeHead response;	/* from right node */

	TBuffer res_head;	/* 响应头的原始信息 */
	Amor::Pius spo_prohead;
	Amor::Pius spo_sethead;
	Amor::Pius end_sess;
	
	typedef struct _Chunko {
					/* 很多情况下, 一个chunk一次读完, 所以都处于初始值 */
		bool started ;		/* false: 还没有读完chunk头, true: chunk数据正在读 */
		TEXTUS_LONG body_len;	/* 正在读chunk的长度, len_to_read 不包括CRLF这两个字 */
		TEXTUS_LONG head_len;		/* head的长度， 包括CRLF */
		inline _Chunko ()
		{
			reset();
		};

		inline void reset ()
		{
			started = false;
			body_len = -1;
		};
	} Chunko;
	Chunko chunko;

	struct G_CFG {
		bool showHead;
		bool transparent;
		inline G_CFG (TiXmlElement *cfg )
		{
			const char *str;
			showHead = false;
			transparent = true;
			if ( cfg->Attribute("show") )
				showHead = true;

			if ( (str = cfg->Attribute("opaque")) && strcasecmp(str, "yes") == 0 )
				transparent = false;
		};
	};

	struct G_CFG *gCFG;	/* Shared for all objects in this node */
	bool has_config;

#include "../httpsrvbody/get_chunk_size.c"
#include "httpsrv_obj.h"
#include "wlog.h"
};

#include <assert.h>
#define METHOD_OTHER    0
#define METHOD_GET      1
#define METHOD_POST     2

#define HTTP_NONE 0
#define HTTP_FORM 1
#define HTTP_FILE 2
#define HTTP_UNKOWN  3

void HttpAgent::ignite(TiXmlElement *cfg) 
{ 
	if (!cfg) return;

	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
}

bool HttpAgent::facio( Amor::Pius *pius)
{
	Amor::Pius tp;
	TBuffer **tb = 0;
	TBuffer *to, *dst;
	TEXTUS_LONG c_sz, dl, rest;

	const char *host;
	switch ( pius->ordo )
	{
	case Notitia::PRO_HTTP_HEAD:	/* HTTP头已经OK */
		WBUG("facio PRO_HTTP_HEAD");
		tp.indic = 0;
		tp.ordo = Notitia::CMD_GET_HTTP_HEADBUF; /* 取HEAD原始数据 */
		aptus->sponte(&tp);

		to = (TBuffer *)tp.indic;
		if ( gCFG->showHead)
		{
			char buf[1024];
			TEXTUS_LONG len;
			memset(buf,0,1024);
			len = to->point - to->base;
			memcpy(buf, to->base, len > 1020 ? 1020:len);
			printf("%s\n",buf);
		}

		if (gCFG->transparent && ( host = getHead("Host"))  && !alive )
		{
			TiXmlElement cfg("host");
			Amor::Pius peer_ps;
			char *p;
			char haddr[128];
			TEXTUS_STRNCPY(haddr, host, 126);
			haddr[126] = '\0';
			p = strpbrk(haddr, ":");	/* p指向端口，如果没有则为80 */
			if ( p ) 
			{
				*p++ = '\0';
				cfg.SetAttribute("port", p);
			} else {
				cfg.SetAttribute("port", "80");
			}
			cfg.SetAttribute("ip", haddr);
			peer_ps.ordo = Notitia::CMD_SET_PEER;
			peer_ps.indic = &cfg;
			aptus->facio(&peer_ps);
		}

		sess = true;
		sent_bl = 0;
		if (alive ) 	/* sess=false: channel is not alive */
		{
			TBuffer::pour(cli_snd, *to);
			aptus->facio(&fac_tbuf);
		} else {
			TBuffer::pour(house, *to);
			demanding = true;
			deliver(Notitia::DMD_START_SESSION);
		}

		if ( rcv_buf->point > rcv_buf->base )	/* 还有数据 */
			goto CLI_PRO;
		break; 

	case Notitia::PRO_TBUF:	/* HTTP body data */
		WBUG("facio PRO_TBUF %d", sess);
		if ( !sess ) break;
CLI_PRO:
		c_sz =  getContentSize();
		assert(rcv_buf);
		dl = rcv_buf->point - rcv_buf->base;

		if (alive ) 	/* sess=false: channel is not alive */
			dst = &cli_snd;
		else 
			dst = &house;

		if ( c_sz < 0 ) 	/* 对于chuncked, 这表明http body未接收完 */
		{
			sent_bl += dl;
			TBuffer::pour(*dst, *rcv_buf);
		} else {
			rest = c_sz - sent_bl;	/* 本HTTP报文还要发多少数据 */
			sent_bl += rest < dl ? rest : dl;
			WBUG ( "rest " TLONG_FMT " , send_bl " TLONG_FMT ", dl " TLONG_FMT " c_sz " TLONG_FMT , rest, sent_bl, dl, c_sz);
			TBuffer::pour(*dst, *rcv_buf, rest);
		}

		tp.indic = 0;
		tp.ordo = Notitia::HTTP_Request_Cleaned; /* 告知HTTP BODY已清空 */
		aptus->sponte(&tp);

		if ( alive)
			aptus->facio(&fac_tbuf);
		break;

	case Notitia::SET_TBUF:	/* 取得输入TBuffer地址 */
		WBUG("facio SET_TBUF");
		if ( (tb = (TBuffer **)(pius->indic)))
		{
			if ( *tb) rcv_buf = *tb; 
			else
				WLOG(WARNING, "facio SET_TBUF rcv_buf null");
			tb++;
			if ( *tb) snd_buf = *tb;
			else
				WLOG(WARNING, "facio SET_TBUF snd_buf null");
			deliver(Notitia::SET_TBUF);
		} else 
			WLOG(WARNING, "facio SET_TBUF null");
		break;

	case Notitia::DMD_END_SESSION:	/* channel is not alive */
		WBUG("facio DMD_END_SESSION");
		house.reset();
		reset();
		break;

	default:
		return false;
	}
	return true;
}

bool HttpAgent::sponte( Amor::Pius *pius)
{
	Amor::Pius tp;
	TEXTUS_LONG len;
	bool sent_out = false; 
	assert(pius);
	
	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF:	/* 置HTTP响应数据 */
		WBUG("sponte PRO_TBUF %d", sess);
		if ( !snd_buf)	
		{
			WLOG(WARNING, "snd_buf is null!");
			break;
		}
		if ( !sess ) break;

		WBUG("sess %d ResStat %d", sess, response.state);
		if ( response.state == DeHead::HeadOK ) /* Head is ready, rest data still in cli_rcv */
			goto END;
		
		if ( (len = cli_rcv.point - cli_rcv.base) > 0 )
		{	/* feed data into the object of response */
			recv_l += response.feed((char*)cli_rcv.base, len);
			
			waitClose = false;
			if ( response.state == DeHead::HeadOK ) 
			{
				res_body_length = -1;
				WBUG("Response Head is OK! status=%d", response.status);
				switch ( response.status )
				{
				case  -1:
					cli_rcv.reset();
					break;
				case 100:	/* Continue头, 但这没有给Browser, 有问题吗? */
					response.reset();
					TBuffer::pour(*snd_buf, cli_rcv);
					aptus->sponte(&spo_body);
					recv_l = 0;
					return true;
				default:
					res_body_length = response.getHeadInt("Content-Length");
					setContentSize(res_body_length);
					setHead("Content-Type", response.getHead("Content-Type"));
					/* 内部的http响应头不会给Browser, 以上的赋值是为了其它程序处理参考,
					   如httpsrvhead等.  最好全部复制一份? 还是搞个可配置?
					*/
					if ( res_body_length == 0 )
					{
						const char *connect;
						connect = response.getHead("Connection");
						if ( connect && strcasecmp(connect, "close") == 0 )
						{	/* 继续发数据,等到服务端通道关闭 */
							waitClose = true;
							setContentSize(-2);/*这使得httpsrvhead不结束session */
						}
					}

					res_head.reset();	
					TBuffer::pour(res_head, cli_rcv, recv_l);
					if ( gCFG->showHead)
					{	/* 显示一下响应头 */
						char buf[1024];
						memset(buf,0,1024);
						memcpy(buf, res_head.base, recv_l > 1020 ? 1020 : recv_l);
						printf("%s\n",buf);
					}

					aptus->sponte(&spo_sethead);	/* 将收到的http头原始复制 但只是传递指针而已 */
					aptus->sponte(&spo_prohead);	/* 把http头数据发出去 */ 
					break;
				}
				WBUG("recv_l(Head len) " TLONG_FMT ", content_len " TLONG_FMT , recv_l, res_body_length);
				recv_l = res_body_length;
			}
		}

	END:	
		if ( waitClose )
		{
			if ( cli_rcv.point > cli_rcv.base)
			{
				TBuffer::pour(*snd_buf, cli_rcv);
				aptus->sponte(&spo_body);
			}
			break;
		}

		if ( res_body_length >= 0 )
		{
			WBUG("recv_l should rceive  " TLONG_FMT " bytes", recv_l);
			if ( recv_l == 0 ) 
			{
				reset();
				break;
			}

			recv_l -= ( cli_rcv.point - cli_rcv.base ); 
			WBUG("recv_l still " TLONG_FMT , recv_l);
			sent_out = ( recv_l <= 0 ) ;

			TBuffer::pour(*snd_buf, cli_rcv);
			aptus->sponte(&spo_body);

		} else if ( res_body_length == -1 ) 
		{
			/* Transfer-Encoding */	

			unsigned char *ptr, *c_base;	/* c_base指示在一个完整chunk之后, 或就是cli_rcv.base */
			WBUG("Transfer-Encoding...");

			c_base = (unsigned char*) 0;	/* 0: 表示刚从PRO_TBUF来,  */
			sent_out = false;		/* 指示chunk们是否都传了 */
		HERE:
			if ( c_base == (unsigned char*) 0 )
				c_base = cli_rcv.base ; 

			if ( !chunko.started )
			{	/* 这一行应该是指示chunk长度 */
				for (ptr = c_base ; ptr < cli_rcv.point - 1; ptr++ )
				{	/* 判断一行的CRLF */
					if ( *ptr == '\r' && *(ptr+1) == '\n' )
					{
						chunko.body_len = get_chunk_size(c_base);
						WBUG("A Chunk size " TLONG_FMT , chunko.body_len);
						chunko.head_len = (ptr - c_base) + 2;
						break;
					}
				}
				chunko.started =  ( chunko.body_len >= 0 );
			}

			if ( chunko.started )
			{	/* 这里是数据了, 从ptr开始 */
				if ( chunko.body_len == 0 )
				{	/* 下面应该是footers, 扫描到CRLFCRLF为止 */
					/* 一种情况是 0CRLFCRLF
					   另一种是: 0.....CRLFCRLF 或 0....CRLF....CRLFCRLF
					   所以, ptr倒回两个字节, 把chunk头的CRLF也算进去
					*/
					for (ptr = &c_base[chunko.head_len-2] ; ptr < cli_rcv.point - 3; ptr++ )
					{	/* 判断CRLFCRLF */
						if ( *ptr == '\r' && *(ptr+1) == '\n' &&
							*(ptr+2) == '\r' && *(ptr+3) == '\n' )
						{
							/* chunk传完了*/
							WBUG("Chunk completed!");
							c_base = ptr+4;
							chunko.reset();
							sent_out = true;
						}
					}
				
				} else if ( chunko.body_len > 0) {
					/* 具体数据 */
					TEXTUS_LONG bz = chunko.body_len + chunko.head_len +2;	/* 整个Chunk长度, 2是因为包括CRLF */
					if ( cli_rcv.point - c_base >= bz ) 
					{
						/* 一个Chunk完整了 */
						WBUG("A Chunk cut out");
						c_base  += bz;
						chunko.reset();
						if ( c_base < cli_rcv.point) /* 下面还有,再找下一个 */
							goto HERE;
					}
				}
			}

			if ( c_base > cli_rcv.base)
			{	/* 如果有Chunk分析出来了 */
				TBuffer::pour(*snd_buf, cli_rcv, c_base - cli_rcv.base);
				aptus->sponte(&spo_body);
			}

			/* end of Transfer-Encoding */
		}

		if ( sent_out ) 
		{ 
			tp.indic = 0;
			tp.ordo = Notitia::HTTP_Response_Complete;
			aptus->sponte(&tp);
			reset();
		}
		break;

	case Notitia::START_SESSION:	/* channel is alive */
		WBUG("sponte START_SESSION");
		alive = true;
		demanding = false;
		if ( house.point > house.base )
		{
			TBuffer::pour(cli_snd, house);
			aptus->facio(&fac_tbuf);
		}
		break;

	case Notitia::DMD_END_SESSION:	/* channel is not alive */
		WBUG("sponte DMD_END_SESSION");
		alive = false;
		if ( waitClose )
		{
			waitClose = false;
			aptus->sponte(&end_sess);
		}
		if ( demanding )
		{
			demanding = false;
			aptus->sponte(&end_sess);
		}
			
		break;
	default:
		return false;
	}
	return true;
}

HttpAgent::HttpAgent():cli_rcv(8192), cli_snd(8192), response(DeHead::RESPONSE)
{
	gCFG = 0 ;
	has_config = false;

	fac_tbuf.indic = 0;
	fac_tbuf.ordo = Notitia::PRO_TBUF;
	spo_body.indic = 0;
	spo_body.ordo = Notitia::PRO_TBUF;

	spo_sethead.ordo = Notitia::CMD_SET_HTTP_HEAD;
	spo_sethead.indic =  &res_head;
	spo_prohead.ordo = Notitia::PRO_HTTP_HEAD;
	spo_prohead.indic =  0;

	end_sess.ordo = Notitia::END_SESSION;
	end_sess.indic =  0;

	rcv_buf = 0;
	snd_buf = 0;
	alive = false;
	reset();
}

Amor* HttpAgent::clone()
{
	HttpAgent *child;
	child =  new HttpAgent();
	child->gCFG = gCFG;
	return (Amor*) child;
}

HTTPINLINE void HttpAgent::reset()
{
	sess = false;
	waitClose = false;
	demanding = false;
	sent_bl = 0;
	recv_l = 0;
	response.reset();
}

/* 向接力者提交 */
HTTPINLINE void HttpAgent::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	TBuffer *tb[3];
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	
	switch (aordo)
	{
		case Notitia::SET_TBUF:
			WBUG("deliver SET_TBUF");
			tb[0] = &cli_snd;
			tb[1] = &cli_rcv;
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
