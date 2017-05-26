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
#include "DeHead.h"
#include "PacData.h"
#include <time.h>
#include <ctype.h>
#include <stdarg.h>

#define HTTPINLINE inline
class Ajpfwd: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	Ajpfwd();

private:
	TBuffer *rcv_buf;	/* 在http头完成后，这将是http体的内容 */
	TBuffer *snd_buf;

	TBuffer *buf_req;	/* unipac处理(可能还有其它的)后, 最终走向tcpcli或tbufchan之类的, */
	TBuffer *buf_ans;

	PacketObj pac_1st;
	PacketObj pac_2nd;	/* 传递至右节点的PacketObj */

	HTTPINLINE void deliver(Notitia::HERE_ORDO aordo);
	HTTPINLINE void head_to_con();	/* con: 即指container, 如tomcat */
	HTTPINLINE void head_from_con();

	HTTPINLINE void body_to_con();
	HTTPINLINE void body_from_con();
	HTTPINLINE void end_from_con();
	HTTPINLINE void get_chunk_from_con();

	Amor::Pius fac_body;
	Amor::Pius spo_body;
	Amor::Pius local_p;

	Amor::Pius spo_prohead;
	Amor::Pius spo_complete;
	
	struct G_CFG {
		bool showHead;
		bool is_ssl;
		bool fast;

		/* 以下ajp请求定义 */
		int prefix_fld;		/* 对于请求与响应都是如此 */
		int chunk_fld;		/* BODY数据, 对于请求与响应都是如此 */
		int headers_fld;	/* 请求或响应头 */

		int method_fld;	
		int protocol_fld;	
		int req_uri_fld;	
		int remote_addr_fld;	
		int remote_host_fld;	
		int server_name_fld;	
		int server_port_fld;	
		int is_ssl_fld;	
		int attributes_fld;	

		/* 以下ajp响应定义 */
		int status_code_fld;	/* 状态 200等 */
		int status_msg_fld;	/* 状态消息 ok */
		int reuse_fld;			/* 是否重用通道 */
		int want_length_fld;		/* 请求长度 */

		inline G_CFG (TiXmlElement *cfg )
		{
			TiXmlElement *fld_ele;
			
			showHead = false;
			if ( cfg->Attribute("show") )
				showHead = true;

			is_ssl = false;
			if ( cfg->Attribute("ssl") )
				is_ssl = true;

			fast = false;
			if ( cfg->Attribute("fast") )
				fast = true;

			fld_ele = cfg->FirstChildElement("prefix"); prefix_fld = 0; if(fld_ele) fld_ele->QueryIntAttribute("field", &prefix_fld);
			fld_ele = cfg->FirstChildElement("method"); method_fld = 0; if(fld_ele) fld_ele->QueryIntAttribute("field", &method_fld);
			fld_ele = cfg->FirstChildElement("protocol");protocol_fld = 0; if(fld_ele) fld_ele->QueryIntAttribute("field", &protocol_fld);
			fld_ele = cfg->FirstChildElement("req_uri"); req_uri_fld = 0; if(fld_ele) fld_ele->QueryIntAttribute("field", &req_uri_fld);
			fld_ele = cfg->FirstChildElement("remote_addr"); remote_addr_fld = 0; if(fld_ele) fld_ele->QueryIntAttribute("field", &remote_addr_fld);
			fld_ele = cfg->FirstChildElement("remote_host"); remote_host_fld = 0; if(fld_ele) fld_ele->QueryIntAttribute("field", &remote_host_fld);
			fld_ele = cfg->FirstChildElement("server_name"); server_name_fld = 0; if(fld_ele) fld_ele->QueryIntAttribute("field", &server_name_fld);
			fld_ele = cfg->FirstChildElement("headers"); headers_fld = 0; if(fld_ele) fld_ele->QueryIntAttribute("field", &headers_fld);
			fld_ele = cfg->FirstChildElement("ssl"); is_ssl_fld = 0; if(fld_ele) fld_ele->QueryIntAttribute("field", &is_ssl_fld);
			fld_ele = cfg->FirstChildElement("attributes"); attributes_fld = 0; if(fld_ele) fld_ele->QueryIntAttribute("field", &attributes_fld);
			fld_ele = cfg->FirstChildElement("chunk"); chunk_fld = 0; if(fld_ele) fld_ele->QueryIntAttribute("field", &chunk_fld);
			fld_ele = cfg->FirstChildElement("status_code"); status_code_fld = 0; if(fld_ele) fld_ele->QueryIntAttribute("field", &status_code_fld);
			fld_ele = cfg->FirstChildElement("status_message"); status_msg_fld = 0; if(fld_ele) fld_ele->QueryIntAttribute("field", &status_msg_fld);
			fld_ele = cfg->FirstChildElement("reuse"); reuse_fld = 0; if(fld_ele) fld_ele->QueryIntAttribute("field", &reuse_fld);
			fld_ele = cfg->FirstChildElement("request_length"); want_length_fld = 0; if(fld_ele) fld_ele->QueryIntAttribute("field", &want_length_fld);
		};
	};

	struct G_CFG *gCFG;	/* Shared for all objects in this node */
	bool has_config;

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

void Ajpfwd::ignite(TiXmlElement *cfg) 
{ 
	if (!cfg) return;

	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
}

bool Ajpfwd::facio( Amor::Pius *pius)
{
	TBuffer **tb = 0;
	switch ( pius->ordo )
	{
	case Notitia::PRO_HTTP_REQUEST: /* 整个HTTP请求已经ok */
		WBUG("facio PRO_HTTP_REQUEST");
		body_to_con();
		break;

	case Notitia::HTTP_ASKING: /* HTTP头已经OK, 但请求体正进行中 */
		WBUG("facio HTTP_ASKING");
		body_to_con();
		break;

	case Notitia::PRO_HTTP_HEAD:	
		head_to_con();
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
			deliver(Notitia::SET_UNIPAC);
		} else 
			WLOG(WARNING, "facio SET_TBUF null");
		break;

	default:
		return false;
	}
	return true;
}

bool Ajpfwd::sponte( Amor::Pius *pius)
{
	TBuffer **tb = 0;
	unsigned char *res_ch;
	assert(pius);
	
	switch ( pius->ordo )
	{
	case Notitia::SET_TBUF:	/* 取得右边的输入TBuffer地址, 为快速模式 */
		WBUG("sponte SET_TBUF");
		if ( (tb = (TBuffer **)(pius->indic)))
		{
			if ( *tb) buf_req = *tb; 
			else
				WLOG(WARNING, "sponte SET_TBUF rcv_buf null");
			tb++;
			if ( *tb) buf_ans = *tb;
			else
				WLOG(WARNING, "sponte SET_TBUF snd_buf null");
		} else 
			WLOG(WARNING, "sponte SET_TBUF null");
		break;

	case Notitia::PRO_UNIPAC:	/* 来自右边的AJP响应 */
		WBUG("sponte PRO_UNIPAC");
		res_ch = pac_2nd.getfld(gCFG->prefix_fld);
		switch ( *res_ch)
		{
		case 0x3:
			body_from_con();
			break;
		case 0x4:
			head_from_con();
			break;
		case 0x5:
			end_from_con();
			break;
		case 0x6:
			get_chunk_from_con();
			break;
		}
		break;

	case Notitia::PRO_TBUF:	/* 置HTTP响应数据, 仅应于快速模式 */
		WBUG("sponte PRO_TBUF");
		if ( !snd_buf)	
		{
			WLOG(WARNING, "snd_buf is null!");
			break;
		}
		break;

	default:
		return false;
	}
	return true;
}

Ajpfwd::Ajpfwd()
{
	gCFG = 0 ;
	local_p.ordo = Notitia::PRO_UNIPAC;
	local_p.indic = 0;

	fac_body.indic = 0;
	fac_body.ordo = Notitia::PRO_TBUF;

	spo_body.indic = 0;
	spo_body.ordo = Notitia::PRO_TBUF;

	spo_prohead.ordo = Notitia::PRO_HTTP_HEAD;
	spo_prohead.indic =  0;

	spo_complete.ordo = Notitia::HTTP_Response_Complete;
	spo_complete.indic =  0;

	rcv_buf = 0;
	snd_buf = 0;
	buf_req = 0;
	buf_ans = 0;
}

Amor* Ajpfwd::clone()
{
	Ajpfwd *child;
	child =  new Ajpfwd();

	child->gCFG = gCFG;
	return (Amor*) child;
}

/* 向接力者提交 */
HTTPINLINE void Ajpfwd::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	PacketObj *tb[3];
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	
	switch (aordo)
	{
		case Notitia::SET_UNIPAC:
			WBUG("deliver SET_UNIPAC");
			tb[0] = &pac_1st;
			tb[1] = &pac_2nd;
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

HTTPINLINE void Ajpfwd::head_from_con()
{
	struct AjpHeadAttrType *head;
	struct ComplexType *ctp;
	unsigned char *p;
	int st_code;

	p = pac_2nd.getfld(gCFG->status_code_fld);
	st_code = (p[0] << 8) + p[1];
	setStatus(st_code);	/* message就不转发了, 仅根据此, 由httpsrvhead设置message内容 */

	ctp = (struct ComplexType *)pac_2nd.getfld(gCFG->headers_fld);
	if ( !ctp )
		return;
	if ( ctp->type != PacketObj::AJP_HEAD_ATTR )
		return;

	head = (struct AjpHeadAttrType *)(ctp->value);
	while ( head )
	{
		head->string[head->str_len-1] = 0;	/* 把末尾给0, 为了后面的操作 */
		switch ( head->sc_name )
		{
		case 0xA001: 
			setHead("Content-Type", head->string);
			break;
		case 0xA002: 
			setHead("Content-Language", head->string);
			break;
		case 0xA003: 
			setHead("Content-Length", head->string);
			break;
		case 0xA004: 
			setHead("Date", head->string);
			break;
		case 0xA005: 
			setHead("Last-Modified", head->string);
			break;
		case 0xA006: 
			setHead("Location", head->string);
			break;
		case 0xA007: 
			addHead("Set-Cookie", head->string);
			break;
		case 0xA008: 
			addHead("Set-Cookie2", head->string);
			break;
		case 0xA009: 
			setHead("Servlet-Engine", head->string);
			break;
		case 0xA00A: 
			setHead("Status", head->string);
			break;
		case 0xA00B: 
			setHead("WWW-Authenticate", head->string);
			break;

		case PacketObj::INVALID_SC_NAME: 
			head->name[head->nm_len-1] = 0;
			addHead(head->name, head->string);
			break;
		default:
			break;
		}		
		head = head->next;
	}
	aptus->sponte(&spo_prohead);
}

HTTPINLINE void Ajpfwd::body_from_con()
{
	unsigned char *p;
	int len;
	p = pac_2nd.getfld(gCFG->chunk_fld, &len);
	snd_buf->input(p, len);
	aptus->sponte(&spo_body);
}

HTTPINLINE void Ajpfwd::end_from_con()
{
	aptus->sponte(&spo_complete);
}

HTTPINLINE void Ajpfwd::get_chunk_from_con()
{
/* 怎么弄? 就等发送?*/
}

HTTPINLINE void Ajpfwd::head_to_con()
{
	Amor::Pius tp;
	DeHead *req;
	DeHead::FIELD_VALUE *head_fld;
	TiXmlElement *peer;
	int port, i;
	char buf[32], meth;

	tp.indic = 0;
	tp.ordo = Notitia::CMD_GET_HTTP_HEADOBJ; /* 取HEAD头 */
	aptus->sponte(&tp);
	req = (DeHead *)tp.indic;

	tp.indic = 0;
	tp.ordo = Notitia::CMD_GET_PEER; /* 取套接字的双方地址及端口 */
	peer = (TiXmlElement *)tp.indic;

	pac_1st.input(gCFG->prefix_fld, 0x02);	/* JK_AJP13_FORWARD_REQUEST */

	meth= req->method_type;
	pac_1st.input(gCFG->method_fld, meth);

	if ( req->protocol)
		pac_1st.input(gCFG->protocol_fld, req->protocol, strlen(req->protocol));

	if ( req->path)
		pac_1st.input(gCFG->req_uri_fld, req->path, strlen(req->path));	

	if ( req->query)
		pac_1st.inputAJP(gCFG->attributes_fld, 0x05, (unsigned short)(strlen(req->query) & 0xffff), req->query);

	if ( peer )
	{
		const char *rm_prt="AJP_REMOTE_PORT";
		const char *lc_addr="AJP_LOCAL_ADDR";
		pac_1st.input(gCFG->remote_addr_fld, peer->Attribute("cliip"), strlen(peer->Attribute("cliip")));
		pac_1st.input(gCFG->remote_host_fld, &meth, 0);	/* &meth无用, 只是 */
		pac_1st.input(gCFG->server_name_fld, peer->Attribute("srvip"), strlen(peer->Attribute("srvip")));
		peer->QueryIntAttribute("srvport", &port);
		buf[0] = (port >> 8) & 0xff;
		buf[1] = port & 0xff;
		pac_1st.input(gCFG->server_port_fld, (const char*)&buf, 2);

		pac_1st.inputAJP(gCFG->attributes_fld, (unsigned short)(strlen(rm_prt) & 0xffff), rm_prt, (unsigned short)(strlen(peer->Attribute("cliport")) & 0xffff), peer->Attribute("cliport"));
		pac_1st.inputAJP(gCFG->attributes_fld, (unsigned short)(strlen(lc_addr) & 0xffff), lc_addr, (unsigned short)(strlen(peer->Attribute("srvip")) & 0xffff), peer->Attribute("srvip"));
	}

	if ( gCFG->is_ssl)
		pac_1st.input(gCFG->is_ssl_fld, 0x1);	
	else
		pac_1st.input(gCFG->is_ssl_fld, 0x0);	

	for ( i = 0 ; i < req->field_num; i++)
	{
    		char *p;
		unsigned short sc_name;
		int nm_len;

		head_fld = &req->field_values[i];

		sc_name = 0x0;
		p = head_fld->str;
		if ( gCFG->showHead)
		{
			WLOG(INFO, "HTTP_HEAD %s %s", head_fld->name, head_fld->str);
		}
    		switch (head_fld->name[0]) 
		{
       		case 'A':
       		case 'a':
			nm_len = strlen(head_fld->name);
			if ( nm_len < 8 )
			{ 
       				if (strcasecmp(p, "Accept") == 0) sc_name = 0xa001;
			} else switch (head_fld->name[7]) 
			{
				case 'C':
				case 'c':
               				if (strcasecmp(p, "Accept-charset") == 0) sc_name = 0xa002;
					break;
				case 'E':
				case 'e':
               				if (strcasecmp(p, "Accept-encoding") == 0) sc_name = 0xa003;
					break;
				case 'L':
				case 'l':
               				if (strcasecmp(p, "Accept-language") == 0) sc_name = 0xa004;
					break;
				case 'Z':
				case 'z':
                			if (strcasecmp(p, "Authorization") == 0) sc_name = 0xa005;
					break;
				default:
					break;
			}
        		break;
        	case 'C':
        	case 'c':
			nm_len = strlen(head_fld->name);
			if ( nm_len > 8 )
			{ 
				switch (head_fld->name[8])
				{
				case 'O':
				case 'o':
            				if (strcasecmp(p, "Connection") == 0) sc_name = 0xa006;
					break;
				case 'T':
				case 't':
	       				if (strcasecmp(p, "Content-type") == 0) sc_name = 0xa007;
					break;
				case 'L':
				case 'l':
            				if (strcasecmp(p, "Content-length") == 0) sc_name = 0xa008;
					break;
				default:
					break;
				}
			} else {
				switch (nm_len)
				{
				case 6:
            				if (strcasecmp(p, "Cookie") == 0) sc_name = 0xa009;
					break;
				case 7:
	       				if (strcasecmp(p, "Cookie2") == 0) sc_name = 0xa00a;
					break;
				default:
					break;
				}
			}
        		break;
        	case 'H':
        	case 'h':
            		if (strcasecmp(p, "Host") == 0) sc_name = 0xa00b;
        		break;
        	case 'P':
        	case 'p':
            		if (strcasecmp(p, "Pragma") == 0) sc_name = 0xa00c;
        		break;
        	case 'R':
        	case 'r':
            		if (strcasecmp(p, "Referer") == 0) sc_name = 0xa00d;
        		break;
        	case 'U':
        	case 'u':
            		if (strcasecmp(p, "User-agent") == 0) sc_name = 0xa00e;
        		break;
        	default:
			break;
		}

		if ( sc_name == 0x0 )
		{	/* 头名称不是典型的 */
			pac_1st.inputAJP(gCFG->headers_fld, (unsigned short)(strlen(head_fld->name) & 0xffff), head_fld->name, (unsigned short)(strlen(head_fld->str) & 0xffff), head_fld->str);
		} else {
			pac_1st.inputAJP(gCFG->headers_fld, sc_name, (unsigned short)(strlen(head_fld->str) & 0xffff), head_fld->str);
		}
	}
	aptus->facio(&local_p);	 /* 发送AJP FORWARD*/
}

HTTPINLINE void Ajpfwd::body_to_con()
{
#define AJP_FRAME_MAX 8180
	long len, len2;
	unsigned char *p;

	len2 = len = rcv_buf->point - rcv_buf->base;
	p = rcv_buf->base;
	while ( len > AJP_FRAME_MAX)
	{
		pac_1st.input(gCFG->prefix_fld, 0x0B);	/* match only, not pack it */
		pac_1st.input(gCFG->chunk_fld, p, AJP_FRAME_MAX );
		aptus->facio(&local_p);	 /* 发送AJP body*/
		p += AJP_FRAME_MAX;
		len -= AJP_FRAME_MAX;
	}
	pac_1st.input(gCFG->prefix_fld, 0x0B);	/* match only, not pack it */
	pac_1st.input(gCFG->chunk_fld, p, len);
	rcv_buf->commit(-len2);
	aptus->facio(&local_p);	 /* 发送AJP body*/
}

#include "hook.c"
