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
	TBuffer *rcv_buf;	/* ��httpͷ��ɺ��⽫��http������� */
	TBuffer *snd_buf;

	TBuffer house;		/* �ݴ� */
	TBuffer cli_rcv;	/* ���ҽڵ���յ����ݻ��� */
	TBuffer cli_snd;	/* ���ҽڵ㷢�͵����ݻ��� */

	HTTPINLINE void deliver(Notitia::HERE_ORDO aordo);
	HTTPINLINE void reset();
	Amor::Pius fac_tbuf;
	Amor::Pius spo_body;
	bool alive;	/* ��WEB����˵�ͨ���Ƿ�� */
	TEXTUS_LONG sent_bl;
	TEXTUS_LONG recv_l;
	TEXTUS_LONG res_body_length;

	bool sess;	/* HTTP����Ӧ��ʼ */
	bool waitClose;	/* �ȷ���˽����ź� */
	bool demanding; /* ����Ҫ��ͨ��, ��δ����Ӧ */
	DeHead response;	/* from right node */

	TBuffer res_head;	/* ��Ӧͷ��ԭʼ��Ϣ */
	Amor::Pius spo_prohead;
	Amor::Pius spo_sethead;
	Amor::Pius end_sess;
	
	typedef struct _Chunko {
					/* �ܶ������, һ��chunkһ�ζ���, ���Զ����ڳ�ʼֵ */
		bool started ;		/* false: ��û�ж���chunkͷ, true: chunk�������ڶ� */
		TEXTUS_LONG body_len;	/* ���ڶ�chunk�ĳ���, len_to_read ������CRLF�������� */
		TEXTUS_LONG head_len;		/* head�ĳ��ȣ� ����CRLF */
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
	case Notitia::PRO_HTTP_HEAD:	/* HTTPͷ�Ѿ�OK */
		WBUG("facio PRO_HTTP_HEAD");
		tp.indic = 0;
		tp.ordo = Notitia::CMD_GET_HTTP_HEADBUF; /* ȡHEADԭʼ���� */
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
			p = strpbrk(haddr, ":");	/* pָ��˿ڣ����û����Ϊ80 */
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

		if ( rcv_buf->point > rcv_buf->base )	/* �������� */
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

		if ( c_sz < 0 ) 	/* ����chuncked, �����http bodyδ������ */
		{
			sent_bl += dl;
			TBuffer::pour(*dst, *rcv_buf);
		} else {
			rest = c_sz - sent_bl;	/* ��HTTP���Ļ�Ҫ���������� */
			sent_bl += rest < dl ? rest : dl;
			WBUG ( "rest " TLONG_FMT " , send_bl " TLONG_FMT ", dl " TLONG_FMT " c_sz " TLONG_FMT , rest, sent_bl, dl, c_sz);
			TBuffer::pour(*dst, *rcv_buf, rest);
		}

		tp.indic = 0;
		tp.ordo = Notitia::HTTP_Request_Cleaned; /* ��֪HTTP BODY����� */
		aptus->sponte(&tp);

		if ( alive)
			aptus->facio(&fac_tbuf);
		break;

	case Notitia::SET_TBUF:	/* ȡ������TBuffer��ַ */
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
	case Notitia::PRO_TBUF:	/* ��HTTP��Ӧ���� */
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
				case 100:	/* Continueͷ, ����û�и�Browser, ��������? */
					response.reset();
					TBuffer::pour(*snd_buf, cli_rcv);
					aptus->sponte(&spo_body);
					recv_l = 0;
					return true;
				default:
					res_body_length = response.getHeadInt("Content-Length");
					setContentSize(res_body_length);
					setHead("Content-Type", response.getHead("Content-Type"));
					/* �ڲ���http��Ӧͷ�����Browser, ���ϵĸ�ֵ��Ϊ������������ο�,
					   ��httpsrvhead��.  ���ȫ������һ��? ���Ǹ��������?
					*/
					if ( res_body_length == 0 )
					{
						const char *connect;
						connect = response.getHead("Connection");
						if ( connect && strcasecmp(connect, "close") == 0 )
						{	/* ����������,�ȵ������ͨ���ر� */
							waitClose = true;
							setContentSize(-2);/*��ʹ��httpsrvhead������session */
						}
					}

					res_head.reset();	
					TBuffer::pour(res_head, cli_rcv, recv_l);
					if ( gCFG->showHead)
					{	/* ��ʾһ����Ӧͷ */
						char buf[1024];
						memset(buf,0,1024);
						memcpy(buf, res_head.base, recv_l > 1020 ? 1020 : recv_l);
						printf("%s\n",buf);
					}

					aptus->sponte(&spo_sethead);	/* ���յ���httpͷԭʼ���� ��ֻ�Ǵ���ָ����� */
					aptus->sponte(&spo_prohead);	/* ��httpͷ���ݷ���ȥ */ 
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

			unsigned char *ptr, *c_base;	/* c_baseָʾ��һ������chunk֮��, �����cli_rcv.base */
			WBUG("Transfer-Encoding...");

			c_base = (unsigned char*) 0;	/* 0: ��ʾ�մ�PRO_TBUF��,  */
			sent_out = false;		/* ָʾchunk���Ƿ񶼴��� */
		HERE:
			if ( c_base == (unsigned char*) 0 )
				c_base = cli_rcv.base ; 

			if ( !chunko.started )
			{	/* ��һ��Ӧ����ָʾchunk���� */
				for (ptr = c_base ; ptr < cli_rcv.point - 1; ptr++ )
				{	/* �ж�һ�е�CRLF */
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
			{	/* ������������, ��ptr��ʼ */
				if ( chunko.body_len == 0 )
				{	/* ����Ӧ����footers, ɨ�赽CRLFCRLFΪֹ */
					/* һ������� 0CRLFCRLF
					   ��һ����: 0.....CRLFCRLF �� 0....CRLF....CRLFCRLF
					   ����, ptr���������ֽ�, ��chunkͷ��CRLFҲ���ȥ
					*/
					for (ptr = &c_base[chunko.head_len-2] ; ptr < cli_rcv.point - 3; ptr++ )
					{	/* �ж�CRLFCRLF */
						if ( *ptr == '\r' && *(ptr+1) == '\n' &&
							*(ptr+2) == '\r' && *(ptr+3) == '\n' )
						{
							/* chunk������*/
							WBUG("Chunk completed!");
							c_base = ptr+4;
							chunko.reset();
							sent_out = true;
						}
					}
				
				} else if ( chunko.body_len > 0) {
					/* �������� */
					TEXTUS_LONG bz = chunko.body_len + chunko.head_len +2;	/* ����Chunk����, 2����Ϊ����CRLF */
					if ( cli_rcv.point - c_base >= bz ) 
					{
						/* һ��Chunk������ */
						WBUG("A Chunk cut out");
						c_base  += bz;
						chunko.reset();
						if ( c_base < cli_rcv.point) /* ���滹��,������һ�� */
							goto HERE;
					}
				}
			}

			if ( c_base > cli_rcv.base)
			{	/* �����Chunk���������� */
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

/* ��������ύ */
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
