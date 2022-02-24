/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
/**
 Title: HTTP InsWay
 Build: created by octerboy, 2020/01/07, Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "BTool.h"
#include "TBuffer.h"
#include "casecmp.h"
#include "textus_string.h"
#include "DeHead.h"
#include <stdlib.h>
#include <time.h>
#if defined(TEXTUS_PLATFORM_64) && !defined(_WIN32)
#include <sys/time.h>
#else
#include <sys/timeb.h>
#endif
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#if defined(__APPLE__)
#define COMMON_DIGEST_FOR_OPENSSL
#include <CommonCrypto/CommonDigest.h>
#else
#include <openssl/sha.h>
#endif
#include "WayData.h"
#include "get_chunk_size.c"

#define FLD_NO_VALUE -9
/* 命令分几种，INS_Normal：标准， */
enum HIns_Type { INS_None = 0, INS_FromRequest=1,  INS_ToResponse=3, INS_ToRequest=5,  INS_FromResponse=6};
enum Head_Type { Head_None = 0, Head_Title=1, Head_Method=2, Head_Path=3, Head_Status=4, Head_Protocol=5, Head_Parameter=6, Head_Name=7, Head_Body=8, Head_Query=9, Head_Content_Length=10, Head_Content_Type=11, Head_Now=12, Head_Time=13};
enum HIns_LOG { HI_LOG_NONE = 0, HI_LOG_STR =0x11, HI_LOG_HEX =0x12, HI_LOG_BOTH = 0x13, HI_LOG_STR_ERR = 0x21, HI_LOG_HEX_ERR = 0x22, HI_LOG_BOTH_ERR = 0x23};

struct HFld {
	Head_Type head;
	const char *name;
	const char *fmt;
};

struct HInsData : ExtInsBase {
	HIns_Type type;
	int subor;	//指示指令报文送给哪一个下级模块

	struct HFld *snd_fld_buf, *rcv_fld_buf;
	enum HIns_LOG log;
	bool wait_left_body, wait_right_body;	/*  针对body接收, 指明是否全部完成才能处理 */
	bool pro_chunk;

	HInsData() 
	{
		type = INS_None;
		subor = 0;
		me = 0;
		snd_fld_buf = 0;
		rcv_fld_buf = 0;
	};

	bool set_def(TiXmlElement *def_ele, struct InsData *insd) 
	{
		const char *p;
		p = def_ele->Attribute("type");	
		if ( !p) return false;
		if ( strcasecmp( p, "FromBrowser") ==0 )
		{
			type =  INS_FromRequest;
		} else if ( strcasecmp( p, "HttpRequest") ==0 )
		{
			type =  INS_ToRequest;
		} else if ( strcasecmp( p, "HttpResponse") ==0 )
		{
			type =  INS_FromResponse;
		} else if ( strcasecmp( p, "ToBrowser") ==0 )
		{
			type =  INS_ToResponse;
		} else 
			return false;

		if ( !insd->err_code ) 
			insd->err_code = def_ele->Attribute("error");
		if ( !insd->err_code ) 
			insd->err_code = def_ele->GetDocument()->RootElement()->Attribute("error"); //取整个文档的根元素定义，如"unknown error"
		if ( (p = def_ele->Attribute("counted")) && ( *p == 'y' || *p == 'Y') )
			insd->counted = true;
		else 
			insd->counted = false;

		if ((p = def_ele->Attribute("function")) && ( *p == 'y' || *p == 'Y') )	//这是函数扩展，出现回调
			insd->isFunction = true;
		else
			insd->isFunction = false;

		pro_chunk = true; /* 默认处理chunk */
		if ((p = def_ele->Attribute("pro_chunk")) && ( *p == 'n' || *p == 'N') )	//不处理chunk
			pro_chunk = false;

		wait_left_body = true; /* 默认等全部body所有的内容 */
		if ((p = def_ele->Attribute("all_left_body")) && ( *p == 'n' || *p == 'N') )	//不等全部的body
			wait_left_body = false;

		wait_right_body = true; /* 默认等全部body所有的内容 */
		if ((p = def_ele->Attribute("all_right_body")) && ( *p == 'n' || *p == 'N') )	//不等全部的body
			wait_right_body = false;

		if ( !insd->log_str ) insd->log_str = def_ele->Attribute("log");
		p = insd->log_str;
		if ( p ) 
		{
			if ( strcasecmp( p, "str") ==0 )
				log = HI_LOG_STR;
			else if ( strcasecmp( p, "estr") ==0 )
				log = HI_LOG_STR_ERR;
			else if ( strcasecmp( p, "hex") ==0 )
				log = HI_LOG_HEX;
			else if ( strcasecmp( p, "ehex") ==0 )
				log = HI_LOG_HEX_ERR;
			else if ( strcasecmp( p, "both") ==0 )
				log = HI_LOG_BOTH;
			else if ( strcasecmp( p, "eboth") ==0 )
				log = HI_LOG_BOTH_ERR;
		}

		subor=Amor::CAN_ALL; 
		def_ele->QueryIntAttribute("subor", &subor);
		if ( subor < Amor::CAN_ALL ) 
			insd->isFunction = true;

		return true;
	};
};

class HttpIns: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	HttpIns();
	~HttpIns();

private:
	char my_err_str[1024];
	struct InsWay *cur_insway;
	TBuffer *rcv_buf;	/* 在http头完成后，这将是http体的内容 */
	TBuffer *ans_body_buf;	

	TBuffer house;		/* 暂存 */
	TBuffer cli_rcv;	/* 从右节点接收的数据缓冲 */
	TBuffer cli_snd;	/* 向右节点发送的数据缓冲 */
	TBuffer *hitb[3];
	TBuffer ans_head_buf, request_body_buf;

	Amor::Pius fac_tbuf;
	Amor::Pius spo_body;
	Amor::Pius spo_sethead;
	Amor::Pius pro_hd_ps, other_ps,ans_ins_ps;

	TEXTUS_LONG ans_body_length,req_body_length;
	TEXTUS_LONG chunk_offset;	/* 当前分析位置, 即rcv_buf中相对于base的偏移量 */
	DeHead response, request, *browser_req, browser_ans;
	char response_status[64], browser_status[64];
	bool left_head_ok, left_body_ok, right_body_ok, right_head_ok, req_head_sent, req_sent, ans_head_sent, ans_sent;

	void left_reset() {
		left_head_ok = false;
		left_body_ok = false;
		browser_ans.reset();
		chunko.reset();
		chunk_offset = 0;
	};

	void right_reset() {
		response.reset();
		request.reset();
		request_body_buf.reset();
		chunko.reset();
		cli_rcv.reset();
		cli_snd.reset();
		right_body_ok = false;
		right_head_ok = false;
		chunk_offset = 0;
	};

	struct G_CFG {
		TiXmlDocument doc_h_def;	//pacdef：报文定义
		TiXmlElement *h_def_root;
		TiXmlElement *prop;

		int subor;      //指示指令报文送给哪一个下级模块

		inline G_CFG (TiXmlElement *cfg )
		{
			const char *str;
			subor = Amor::CAN_ALL;
			if ( (str = cfg->Attribute("my_subor")) )
				subor = atoi(str);
			prop = cfg;
		};
	};

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
	bool chunk_all(TBuffer *bo_buf);

	void set_ins(struct InsData *insd);
	void pro_ins( bool pro_start=false);
	void load_def();
	void log_ins ();
	struct G_CFG *gCFG;	/* Shared for all objects in this node */
	bool has_config;
	const char* pro_rply(struct DyVarBase **psnap, struct InsData *insd, DeHead *headp, bool &has_head, TBuffer *&body_buf);
	void get_snd_buf(struct DyVarBase **psnap, struct InsData *insd, DeHead *headp);
	//void ans_ins(bool should_spo);
	void inline ans_ins();
	void log_ht(DeHead *headp, const char *prompt, const char *err, bool has_head, bool force=false);
	void log_ht(TBuffer *tbuf, const char *prompt);

#include "httpsrv_obj.h"
#include "wlog.h"
};
#include <assert.h>

void HttpIns::load_def()
{
	const char *nm =0;
	if ( !( gCFG->h_def_root = gCFG->prop->FirstChildElement("Http")))	
	{
		if ( (nm = gCFG->prop->Attribute("http"))) 
		{
			if (load_xml(nm,  gCFG->doc_h_def,   gCFG->h_def_root, gCFG->prop->Attribute("http_sha"), my_err_str))
			{
				WLOG(WARNING, "%s", my_err_str);	
			}
		} else {
			WLOG(WARNING, "no http definition file!");
		}		
	}
}

void HttpIns::ignite(TiXmlElement *cfg) 
{ 
	if (!cfg) return;

	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
}

bool HttpIns::facio( Amor::Pius *pius)
{
	TBuffer **tb = 0;
	Amor::Pius tmp_pius;

	switch ( pius->ordo )
	{
	case Notitia::PRO_HTTP_REQUEST:	/* HTTP头已经OK */
		WBUG("facio PRO_HTTP_REQUEST");
		left_head_ok = true;
		left_body_ok = true;
		if ( ((struct HInsData *)cur_insway->dat->ext_ins)->me != this->gCFG ) { 
			WBUG("%s is not for me %p %p.",cur_insway->dat->ins_tag,  ((struct HInsData *)cur_insway->dat->ext_ins)->me,  this->gCFG );
			break;	//不是本模块定义的, 不作处理
		}
		pro_ins();
		left_head_ok = false;
		left_body_ok = false;
		break;

	case Notitia::PRO_HTTP_HEAD:	/* HTTP头已经OK */
		WBUG("facio PRO_HTTP_HEAD");
		left_head_ok = true;
		goto CLI_PRO;

	case Notitia::PRO_TBUF:	/* HTTP body data */
		WBUG("facio PRO_TBUF (head_ok = %d)", left_head_ok);
		assert(rcv_buf);
		if ( !left_head_ok ) break;
		if ( ((struct HInsData *)cur_insway->dat->ext_ins)->me != this->gCFG ) { 
			WBUG("%s is not for me %p %p.",cur_insway->dat->ins_tag,  ((struct HInsData *)cur_insway->dat->ext_ins)->me,  this->gCFG );
			break;	//不是本模块定义的, 不作处理
		}
CLI_PRO:
		if ( browser_req->content_length == -1 )
		{	/* Transfer-Encoding */
			struct HInsData *hti;
			hti = (struct HInsData *) cur_insway->dat->ext_ins;
			if ( hti->pro_chunk)
				left_body_ok = chunk_all(rcv_buf);
		} else {
			left_body_ok = ( browser_req->content_length  >= 0  && ( rcv_buf->point - rcv_buf->base >= browser_req->content_length ) );
		}
		pro_ins();
		break;

	case Notitia::SET_TBUF:	/* 取得输入TBuffer地址 */
		WBUG("facio SET_TBUF");
		if ( (tb = (TBuffer **)(pius->indic)))
		{
			if ( *tb) rcv_buf = *tb; 
			else
				WLOG(WARNING, "facio SET_TBUF rcv_buf null");
			tb++;
			if ( *tb) ans_body_buf = *tb;
			else
				WLOG(WARNING, "facio SET_TBUF ans_body_buf null");
		} else 
			WLOG(WARNING, "facio SET_TBUF null");
		other_ps.indic = 0;
		other_ps.ordo = Notitia::CMD_GET_HTTP_HEADOBJ;
		aptus->sponte(&other_ps);
		browser_req = (DeHead*)other_ps.indic;
		break;

	case Notitia::DMD_END_SESSION:	/* channel is not alive */
		WBUG("facio DMD_END_SESSION");
		left_reset();
		break;

	case Notitia::START_SESSION:	/* channel is alive */
		WBUG("facio START_SESSION");
		left_reset();
		break;

	case Notitia::Set_InsWay:    /* 设置 */
		WLOG(NOTICE, "facio Set_InsWay, InsData %p, tag %s", pius->indic, ((struct InsData*)pius->indic)->ins_tag);
		set_ins((struct InsData*)pius->indic);
		other_ps.indic = 0;
		other_ps.ordo = Notitia::CMD_GET_HTTP_HEADOBJ;
		aptus->sponte(&other_ps);
		browser_req = (DeHead*)other_ps.indic;
		break;

	case Notitia::Pro_InsWay:    /* 处理 */
		WBUG("facio Pro_InsWay, tag %s", ((struct InsWay*)pius->indic)->dat->ins_tag);
		if ( !(((struct InsWay*)pius->indic)->dat->ext_ins) ) { 
			WLOG(WARNING, "pro_insway.ext_ins is null !!");
			break;	//不是本模块定义的, 不作处理
		}
		cur_insway = (struct InsWay*)pius->indic;
		if ( ((struct HInsData *)cur_insway->dat->ext_ins)->me != this->gCFG ) { 
			WBUG("%s is not for me %p %p.",cur_insway->dat->ins_tag,  ((struct HInsData *)cur_insway->dat->ext_ins)->me,  this->gCFG );
			break;	//不是本模块定义的, 不作处理
		}
		pro_ins(true);
		break;

	case Notitia::Log_InsWay:    /* 记录报文, 往往是在错误情况 */
		WBUG("facio Log_InsWay");
		if ( ((struct HInsData *)cur_insway->dat)->me != this->gCFG ) { 
			WBUG("%s is not for me.", cur_insway->dat->ins_tag);
			break;	//不是本模块定义的, 不作处理
		}
		log_ins();
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		load_def();
		tmp_pius.ordo = Notitia::SET_TBUF;
		tmp_pius.indic = &hitb[0];
		aptus->facio(&tmp_pius);
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
		tmp_pius.ordo = Notitia::SET_TBUF;
		tmp_pius.indic = &hitb[0];
		aptus->facio(&tmp_pius);
		break;

	default:
		return false;
	}
	return true;
}

bool HttpIns::sponte( Amor::Pius *pius)
{
	TEXTUS_LONG len;
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF:	/* 置HTTP响应数据 */
		WBUG("sponte PRO_TBUF");
		WBUG("Client request %s  ResStat %s", req_sent? "sent": "not sent", response.state == 0 ? "HeadNothing" : ( response.state == 1 ? "Heading" :"HeadOK" ));
		if ( !req_sent ) { 
			cli_rcv.reset();
			break;	//未发出请求，则不接响应
		}
J_AGAIN:
		if ( response.state == DeHead::HeadOK ) 
		{
			right_head_ok = true;
			if ( response.content_length == -1 )
			{	/* Transfer-Encoding */
				struct HInsData *hti;
				hti = (struct HInsData *) cur_insway->dat->ext_ins;
				if ( hti->pro_chunk) {
					right_body_ok = chunk_all(&cli_rcv);
				}
			} else  {
				right_body_ok = ( response.content_length  >= 0  && (cli_rcv.point - cli_rcv.base) >= response.content_length );
			}
			pro_ins();
		} else if ( (len = cli_rcv.point - cli_rcv.base) > 0 )
		{	/* feed data into the object of response */
			TEXTUS_LONG fed_len;
			fed_len = response.feed((char*)cli_rcv.base, len);
			cli_rcv.commit(-fed_len); /* clear the head data  */
			goto J_AGAIN;
		}
		break;

	case Notitia::START_SESSION:	/* channel is alive */
		WBUG("sponte START_SESSION");
		right_reset();
		break;

	case Notitia::DMD_END_SESSION:	/* channel is not alive */
		WBUG("sponte DMD_END_SESSION");
		right_reset();
		break;
	default:
		return false;
	}
	return true;
}

void HttpIns::set_ins (struct InsData *insd)
{
	TiXmlElement *p_ele, *def_ele = 0;
	const char *p=0, *q=0; 

	int i = 0,a_num;
	size_t lnn; 

	struct HInsData *hti;
	struct HFld *h_fld;

	if ( insd->ext_ins ) return;	//已经定义过了,  不理
	if ( !gCFG->h_def_root ) 
	{
		WLOG(WARNING, "no http definition root!");
		return;
	}
	def_ele = gCFG->h_def_root->FirstChildElement(insd->ins_tag);	
	if ( !def_ele ) return; //在基础报文中没有定义, 不理

	insd->up_subor = gCFG->subor;
	hti = (struct HInsData *) new struct HInsData;
	if ( !hti->set_def(def_ele,insd) ) 
	{
		delete hti;
		return ;
	}
	insd->ext_ins = (struct ExtInsBase *)hti;

	/* 先预置发送的每个域 */
	a_num = 0;
	if ( hti->type != INS_ToRequest && hti->type != INS_ToResponse ) {
		goto RCV_PRO;	
	}

	for (p_ele= def_ele->FirstChildElement(); p_ele; p_ele = p_ele->NextSiblingElement())
	{
		p = p_ele->Value();
		if ( !p ) continue;
		if ( strcasecmp(p, "title") == 0 ) 
			a_num++;
		else if ( strcasecmp(p, "method") == 0 ) 
			a_num++;
		else if ( strcasecmp(p, "path") == 0 ) 
			a_num++;
		else if ( strcasecmp(p, "status") == 0 ) 
			a_num++;
		else if ( strcasecmp(p, "protocol") == 0 ) 
			a_num++;
		else if ( strcasecmp(p, "query") == 0 ) 
			a_num++;
		//else if ( strcasecmp(p, "parameter") == 0 ) 
		//	a_num++;
		else if ( strcasecmp(p, "Content-Type") == 0 ) 
			a_num++;
		else if ( strcasecmp(p, "Content-Length") == 0 ) 
			a_num++;
		else if ( strcasecmp(p, "body") == 0 ) 
			a_num++;
		else  { 
			if ( p_ele->Attribute("head") ||  p_ele->Attribute("headNow") ||  p_ele->Attribute("headTime") )
				a_num++;
		}
		
	}

	if ( a_num ==0 ) goto RCV_PRO;	
	insd->snd_lst  = new struct CmdSnd[a_num];
	hti->snd_fld_buf  = new struct HFld[a_num];

	for (p_ele= def_ele->FirstChildElement(),i = 0; p_ele; p_ele = p_ele->NextSiblingElement())
	{
		p = p_ele->Value();
		if ( !p ) continue;
		insd->snd_lst[i].tag = p_ele->Value();
		h_fld = &hti->snd_fld_buf[i];
		insd->snd_lst[i].ext_fld = h_fld;
		
		h_fld->head = Head_None;
		if ( strcasecmp(p, "title") == 0 ) 
			h_fld->head = Head_Title;
		else if ( strcasecmp(p, "method") == 0 ) 
			h_fld->head = Head_Method;
		else if ( strcasecmp(p, "path") == 0 ) 
			h_fld->head = Head_Path;
		else if ( strcasecmp(p, "status") == 0 ) 
			h_fld->head = Head_Status;
		else if ( strcasecmp(p, "Content-Length") == 0 ) 
			h_fld->head = Head_Content_Length;
		else if ( strcasecmp(p, "Content-Type") == 0 ) 
			h_fld->head = Head_Content_Type;
		else if ( strcasecmp(p, "query") == 0 ) 
			h_fld->head = Head_Query;
		else if ( strcasecmp(p, "protocol") == 0 ) 
			h_fld->head = Head_Protocol;
		//else if ( strcasecmp(p, "parameter") == 0 ) 
		//{
		//	h_fld->head = Head_Parameter;
		//	h_fld->name = p_ele->Attribute("name");
		//}
		else if ( strcasecmp(p, "body") == 0 ) 
			h_fld->head = Head_Body;
		else { 
			if ( (q = p_ele->Attribute("head")) )
			{
				h_fld->head = Head_Name;
			} else if ( (q = p_ele->Attribute("headNow")) )
			{
				h_fld->head = Head_Now;
			} else if ( ( q = p_ele->Attribute("headTime") ) )
			{
				h_fld->head = Head_Time;
				h_fld->fmt = p_ele->Attribute("fmt");
			}

			if ( q && strlen(q) > 0 ) 
			{
				h_fld->name = q;
			} else {
				h_fld->name = p;
			}
		}

		p = p_ele->GetText();
		if ( p ) {
			lnn = strlen(p);
			insd->snd_lst[i].cmd_buf = new unsigned char[lnn+1];
			insd->snd_lst[i].cmd_len = BTool::unescape(p, insd->snd_lst[i].cmd_buf) ;
		}
		i++;
	}
	insd->snd_num = i;	//最后再更新一次发送域的数目

	/* 预置接收的每个域 */
RCV_PRO:
	insd->rcv_num = 0;
	if ( hti->type != INS_FromRequest && hti->type != INS_FromResponse ) {
		goto LAST_CON;	
	}

	a_num = 0;
	for (p_ele= def_ele->FirstChildElement(); p_ele; p_ele = p_ele->NextSiblingElement())
	{
		p = p_ele->Value();
		if ( !p ) continue;
		if ( strcasecmp(p, "title") == 0 ) 
			a_num++;
		else if ( strcasecmp(p, "method") == 0 ) 
			a_num++;
		else if ( strcasecmp(p, "path") == 0 ) 
			a_num++;
		else if ( strcasecmp(p, "status") == 0 ) 
			a_num++;
		else if ( strcasecmp(p, "protocol") == 0 ) 
			a_num++;
		else if ( strcasecmp(p, "parameter") == 0 ) 
			a_num++;
		else if ( strcasecmp(p, "Content-Length") == 0 ) 
			a_num++;
		else if ( strcasecmp(p, "body") == 0 ) 
			a_num++;
		else  { 
			if ( p_ele->Attribute("head") ||  p_ele->Attribute("headNow") ||  p_ele->Attribute("headTime") )
				a_num++;
		}
	}

	if ( a_num ==0 ) goto LAST_CON;	
	insd->rcv_lst = new struct CmdRcv[a_num];
	hti->rcv_fld_buf  = new struct HFld[a_num];

	for (p_ele= def_ele->FirstChildElement(),i = 0; p_ele; p_ele = p_ele->NextSiblingElement())
	{
		p = p_ele->Value();
		if ( !p ) continue;
		insd->rcv_lst[i].tag = p_ele->Value();
		h_fld = &hti->rcv_fld_buf[i];
		insd->rcv_lst[i].ext_fld = h_fld;	//扩展域

		h_fld->head = Head_None;
		if ( strcasecmp(p, "title") == 0 ) 
			h_fld->head = Head_Title;
		else if ( strcasecmp(p, "method") == 0 ) 
			h_fld->head = Head_Method;
		else if ( strcasecmp(p, "path") == 0 ) 
			h_fld->head = Head_Path;
		else if ( strcasecmp(p, "status") == 0 ) 
			h_fld->head = Head_Status;
		else if ( strcasecmp(p, "protocol") == 0 ) 
			h_fld->head = Head_Protocol;
		else if ( strcasecmp(p, "parameter") == 0 ) 
		{
			h_fld->head = Head_Parameter;
			h_fld->name = p_ele->Attribute("name");
		} else if ( strcasecmp(p, "Content-Length") == 0 ) 
			h_fld->head = Head_Content_Length;
		else if ( strcasecmp(p, "body") == 0 ) 
			h_fld->head = Head_Body;
		else { 
			if ( (q = p_ele->Attribute("head")) )
			{
				h_fld->head = Head_Name;
			} else if ( (q = p_ele->Attribute("headNow")) )
			{
				h_fld->head = Head_Now;
			} else if ( ( q = p_ele->Attribute("headTime") ) )
			{
				h_fld->head = Head_Time;
				h_fld->fmt = p_ele->Attribute("fmt");
			}

			if ( q && strlen(q) > 0 ) 
			{
				h_fld->name = q;
			} else {
				h_fld->name = p;
			}
		}

		p = p_ele->GetText();
		if ( p ) {
			lnn = strlen(p);
			insd->rcv_lst[i].must_con = new unsigned char[lnn+1];
			insd->rcv_lst[i].must_len = BTool::unescape(p, insd->rcv_lst[i].must_con) ;
			insd->rcv_lst[i].err_code = p_ele->Attribute("error");	//接收域若有不符合，设此错误码
		}
		p = p_ele->Attribute("disp");
		if (!p) p = def_ele->Attribute("disp");
		if (!p) p = def_ele->GetDocument()->RootElement()->Attribute("disp");
		if ( p && strcasecmp(p, "hex") == 0 )
			insd->rcv_lst[i].err_disp_hex = true;
		i++;
	}

	insd->rcv_num = i;	//最后再更新一次接收域的数目
LAST_CON:
	hti->me = this->gCFG;
	return ;
}

void HttpIns::get_snd_buf(struct DyVarBase **psnap, struct InsData *insd, DeHead *headp)
{
	int i,j;
	unsigned TEXTUS_LONG t_len;
	struct HInsData *hti;
	struct HFld *h_fld;
#if defined(_WIN32) && (_MSC_VER < 1400 )
	struct _timeb now;
#else
#if defined(TEXTUS_PLATFORM_64) && !defined(_WIN32)
	struct timeval now;
#else
	struct timeb now;
#endif

#endif

	hti = (struct HInsData *)insd->ext_ins;
	t_len = 0;

	//printf("get snd insd->snd_num %d\n", insd->snd_num);
	house.reset();
	for ( i = 0 ; i < insd->snd_num; i++ ) 
	{
		if ( insd->snd_lst[i].dy_num ==0 ) 
			t_len += insd->snd_lst[i].cmd_len;	//这是在pacdef中send元素定义的
		 else {
			for ( j = 0; j < insd->snd_lst[i].dy_num; j++ )
			{
				if (  insd->snd_lst[i].dy_list[j].dy_pos < 0 ) 
					t_len += insd->snd_lst[i].dy_list[j].len;
				 else 
					t_len += psnap[insd->snd_lst[i].dy_list[j].dy_pos]->c_len;
			}
		}
	}
	house.grant(t_len);
	for ( i = 0 ; i < insd->snd_num; i++ ) 
	{
		t_len = 0;
		house.reset();
		//printf("insd->snd_lst[%d].dy_num %d\n", i, insd->snd_lst[i].dy_num);
		if ( insd->snd_lst[i].dy_num ==0 )
		{	/* 这是在pacdef中send元素定义的一个域的内容 */
			memcpy(house.point, insd->snd_lst[i].cmd_buf, insd->snd_lst[i].cmd_len);
			t_len = insd->snd_lst[i].cmd_len;
		} else {
			for ( j = 0; j < insd->snd_lst[i].dy_num; j++ )
			{
				/* 由一个列表来构成一个域的内容 */
				if (  insd->snd_lst[i].dy_list[j].dy_pos < 0 ) 
				{
					/* 静态内容*/
					memcpy(&house.point[t_len], insd->snd_lst[i].dy_list[j].con, insd->snd_lst[i].dy_list[j].len);
					t_len += insd->snd_lst[i].dy_list[j].len;
				} else {
					/* 动态内容*/
					memcpy(&house.point[t_len], psnap[insd->snd_lst[i].dy_list[j].dy_pos]->val_p, psnap[insd->snd_lst[i].dy_list[j].dy_pos]->c_len);
					t_len += psnap[insd->snd_lst[i].dy_list[j].dy_pos]->c_len;
				}
			}
		}
		//printf("t_len %d\n", t_len);
		//if ( t_len > 0 ) req_pac->commit(insd->snd_lst[i].fld_no, t_len);	//域的确认
		house.commit(t_len);	//内容确认
		house.point[0] = 0;
		h_fld = ( struct HFld *)(insd->snd_lst[i].ext_fld);
		switch ( h_fld->head )
		{
		case Head_Title:
			TEXTUS_STRNCPY(headp->_title, (const char*)house.base, sizeof(headp->_title)-1 );
			headp->title = headp->_title;
			break;
		case Head_Method:
			headp->get_method((const char*)house.base);
			break;
		case Head_Path:
			TEXTUS_STRNCPY(headp->_path, (const char*)house.base, sizeof(headp->_path)-1);
			headp->path = headp->_path;
			break;
		case Head_Status:	/* None */
			headp->setStatus(atoi((const char*)house.base));
			break;
		case Head_Content_Length:
			if ( house.base[0] == 0 ) 
				headp->setHead("Content-Length", FLD_NO_VALUE); //有域无值
			else 
				headp->setHead("Content-Length", atoi((const char*)house.base));
			break;
		case Head_Content_Type:
			headp->setField("Content-Type", (const char*)house.base, 0, 0, 0);
			break;
		case Head_Protocol:
			TEXTUS_STRNCPY(headp->_protocol, (char*)house.base, 255);
			headp->protocol = headp->_protocol;
			break;
		//case Head_Parameter:
			//request.setParameter(h_fld->name, house.base); 待实现
		//	break;
		case Head_Name:
			//printf("name ------- %s\n", h_fld->name);
			headp->setField(h_fld->name, (const char*)house.base, 0, 0, 0);
			break;
		case Head_Now:
			//printf("Now name ------- %s\n", h_fld->name);
			headp->setField(h_fld->name, 0, 0, 0, 2);
	#if defined(_WIN32) && (_MSC_VER < 1400 )
			_ftime(&now);
	#else
	#if defined(TEXTUS_PLATFORM_64) && !defined(_WIN32)
			gettimeofday(&now,0);
			#define NOW_TIME now.tv_sec
	#else
			ftime(&now);
			#define NOW_TIME now.time
	#endif
	#endif
			headp->setHeadTime(h_fld->name, NOW_TIME);
			break;
		case Head_Time:
			headp->setHeadTime (h_fld->name, atol((const char*)house.base));
			break;
		case Head_Body:
			switch ( hti->type )
			{
			case INS_ToRequest:
				request_body_buf.reset();
				TBuffer::pour(request_body_buf, house);
				break;
			case INS_ToResponse:
				TBuffer::pour(*ans_body_buf, house);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
	return ;
}

void HttpIns::pro_ins (bool pro_start)
{
	struct HInsData *hti;
	struct InsReply *rep;
	const char *err;
	bool has_head;
	TBuffer *body_buf =0;
	hti = (struct HInsData *) cur_insway->dat->ext_ins;
	//if ( strcmp(cur_insway->dat->ins_tag, "InitPac") == 0 )
	//	{int *a =0 ; *a = 0; };

	rep = (struct InsReply *)cur_insway->reply;
	pro_hd_ps.subor = hti->subor;

	switch ( hti->type) {
	case INS_FromRequest:
		if ( hti->wait_left_body && !left_body_ok ) return ;
		err = pro_rply(cur_insway->psnap, cur_insway->dat, browser_req, has_head, body_buf);
		log_ht(browser_req, "Browser request head", err, has_head);
		if ( body_buf )
			log_ht(body_buf, "Browser request body");
		else if (pro_start)	//对于接收head，应一个指令做完, 而接收body可以多个. 
			ans_head_sent = false;
		//ans_ins(hti->subor >= Amor::CAN_ALL);
		ans_ins();
		break;

	case INS_ToResponse:
		if(!ans_head_sent)
		{
			browser_ans.reset();
			get_snd_buf(cur_insway->psnap, cur_insway->dat, &browser_ans);
			ans_body_buf->reset();
			if (  browser_ans.content_length == FLD_NO_VALUE ) //有设置域,但无值
			{
				browser_ans.setHead("Content-Length", (ans_body_buf->point - ans_body_buf->base));
			}
			spo_sethead.indic = &ans_head_buf;
			aptus->sponte(&spo_sethead);
			ans_head_buf.reset();
			ans_head_buf.commit(browser_ans.getContent((char*)ans_head_buf.point, ans_head_buf.limit - ans_head_buf.point));
			log_ht(&ans_head_buf, "Browser answer head");
			ans_body_length=0;
			ans_sent=false;		//整个响应并未完成
			ans_head_sent=true;
			aptus->sponte(&pro_hd_ps);	//发送head
			spo_sethead.indic = 0;
			aptus->sponte(&spo_sethead);	//缓冲复原
		}

		//根据 browser_ans.content_length  判断是否结束
		if( ans_head_sent && ans_body_buf->point > ans_body_buf->base ) 
		{
			ans_body_length += (ans_body_buf->point - ans_body_buf->base);
			if ( ans_body_length >= browser_ans.content_length )
			{
				ans_sent= true;	//等待一个新的开始
			}
			log_ht(ans_body_buf, "Browser answer body");
			aptus->sponte(&spo_body);
		} else
			ans_sent= true;	//等待一个新的开始
		if ( ans_sent)
			ans_ins();
		break;

	case INS_FromResponse:
		//printf("from -- response hti->wait_right_body && !right_body_ok %d %d\n", hti->wait_right_body, right_body_ok );
		if ( hti->wait_right_body && !right_body_ok ) return ;
		err = pro_rply(cur_insway->psnap, cur_insway->dat, &response, has_head, body_buf);
		log_ht(&response, "Client reply head", err, has_head);
		if ( body_buf )
			log_ht(body_buf, "Client reply body");
		ans_ins();
		break;

	case INS_ToRequest:
		if (pro_start) req_head_sent = false; //head应由一个指令做完
		if( !req_head_sent )
		{
			right_reset();
			get_snd_buf(cur_insway->psnap, cur_insway->dat, &request);
			if ( request.content_length == FLD_NO_VALUE ) //有设置域,但无值
			{
				request.setHead("Content-Length", (request_body_buf.point - request_body_buf.base));
			}
			cli_snd.commit(request.getContent((char*)cli_snd.point, cli_snd.limit- cli_snd.point));
			log_ht(&cli_snd, "Client request head");
			req_sent= false;
			req_body_length = 0;
			req_head_sent = true;
			aptus->facio(&fac_tbuf);
		} 

		//根据 request.content_length 判断是否结束
		if ( req_head_sent && request_body_buf.point > request_body_buf.base ) 
		{
			req_body_length += (request_body_buf.point - request_body_buf.base);
			log_ht(&cli_snd, "Client request body");
			if ( req_body_length >= request.content_length )
			{
				req_sent= true;	//等待一个新的开始
			}
			TBuffer::pour(cli_snd, request_body_buf);
			aptus->facio(&fac_tbuf);
		} else
			req_sent= true;	//等待一个新的开始
		if ( req_sent)
			ans_ins();
		break;

	default :
		break;
	}
}

const char* HttpIns::pro_rply(struct DyVarBase **psnap, struct InsData *insd, DeHead *headp, bool &has_head, TBuffer *&body_buf)
{
	int ii;
	char *fc;
	size_t rlen, mlen;
	TEXTUS_LONG llen;
	struct CmdRcv *rply=0;
	char con[512];
    	char timebuf[100];
	struct HInsData *hti;
	struct HFld *h_fld;
				
	hti = (struct HInsData *)insd->ext_ins;
	has_head = false;
	for (ii = 0; ii < insd->rcv_num; ii++)
	{
		rply = &insd->rcv_lst[ii];
		h_fld = (struct HFld *)rply->ext_fld;
		fc = 0;
		rlen = 0 ;
		switch ( h_fld->head )
		{
		case Head_Title:
			fc = headp->title;
			has_head = true;
			break;
		case Head_Method:
			fc = (char*)headp->method;
			has_head = true;
			break;
		case Head_Path:
			fc = &headp->_path[0];
			has_head = true;
			break;
		case Head_Status:
			fc = &response_status[0];
			has_head = true;
			TEXTUS_SNPRINTF(fc, sizeof(response_status), "%d", response.status);
			break;
		case Head_Protocol:
			fc = headp->protocol;
			has_head = true;
			break;
		case Head_Content_Length:
			rlen = headp->content_length ;
			has_head = true;
			break;
		case Head_Content_Type:
			fc = headp->content_type;
			has_head = true;
			break;
		case Head_Query:
			fc = headp->query;
			has_head = true;
			break;
		case Head_Parameter:
			fc = (char*)getPara(h_fld->name, &llen);
			rlen = llen;
			has_head = true;
			break;
		case Head_Name:
			fc = (char*)headp->getHead(h_fld->name);
			has_head = true;
			break;
		case Head_Time:
			{
			time_t lval;
		    struct tm *tdatePtr;
		 	struct tm tdate;
			const char* rfc1123_fmt = "%a, %d %b %Y %H:%M:%S GMT";

			tdatePtr = &tdate;
			lval = headp->getHeadInt(h_fld->name);
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
			gmtime_s(&tdate, &lval );
#else
			tdatePtr = gmtime( &lval );
#endif
			(void) strftime( timebuf, sizeof(timebuf), h_fld->fmt ? h_fld->fmt: rfc1123_fmt, tdatePtr );
			fc = &timebuf[0];
			}
			has_head = true;
			break;
		case Head_Body:
			switch ( hti->type )
			{
			case INS_FromRequest:
				fc = (char*)rcv_buf->base;
				rlen = rcv_buf->point - rcv_buf->base;
				body_buf = rcv_buf;
				break;
			case INS_FromResponse:
				fc = (char*)cli_rcv.base;
				rlen = cli_rcv.point - cli_rcv.base;
				body_buf = &cli_rcv;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}

		if (rlen==0  && fc  ) rlen = strlen(fc);
		if (rply->must_con ) 
		{
			if ( !fc ) 
			{
				TEXTUS_SPRINTF(my_err_str, "recv http error: field %d does not exist", rply->fld_no);
				goto ErrRet;
			}
			if ( !(rply->must_len == rlen && memcmp(rply->must_con, fc, rlen) == 0 ) ) 
			{
				mlen = rlen > sizeof(con) ? sizeof(con):rlen;
				
				if ( rply->err_disp_hex )
				{
					byte2hex((unsigned char*)fc, mlen, con);
					con[mlen*2] = 0;
				} else {
					memcpy(con, fc, mlen);
					con[mlen] = 0;
				}
				TEXTUS_SPRINTF(my_err_str, "recv http error: field %d should not content %s", rply->fld_no, con);
				goto ErrRet;
			}
		}
		//{int *a =0 ; *a = 0; };
		if ( rply->dyna_pos > 0 && fc) 
		{
			if ( rlen >= (rply->start ) )	
			{
				rlen -= (rply->start-1); //start是从1开始
				if ( rply->length > 0 && rply->length < rlen)
					rlen = rply->length;
				psnap[rply->dyna_pos]->input((unsigned char*)&fc[rply->start-1], rlen);
			}
		}
	}
	return (const char*)0;
ErrRet:
	if ( rply->err_code )
		return rply->err_code;
	else
		return insd->err_code;	//可能对于域不符合的情况，未定义错误码，就取基础报文或map中的定义
}

void HttpIns::log_ht(TBuffer *content, const char *prompt)
{
	TBuffer tbuf;
	struct HInsData *hti;
	unsigned char *p;
	
	hti = (struct HInsData *)cur_insway->dat->ext_ins;

	size_t plen, clen;
	size_t j;
	char *rstr;
	size_t o;
	bool has_str=false, has_hex=false;

	if (hti->log &0x1) has_str = true;
	if (hti->log &0x2) has_hex = true;
	plen = strlen(prompt);
	clen = content->point - content->base;
	tbuf.grant(plen + 2 + clen*((has_str ? 1:0) + (has_hex ? 2:0)));
	tbuf.input((unsigned char*)prompt, plen);
	tbuf.input((unsigned char*)" [", 2);
	rstr = (char*) tbuf.point;
	for ( p = content->base,j=0 ; p < content->point; p++,j++) {
		unsigned char c = *p;
		if ( has_hex) {	//需要16进制显示
			o = j*2;
			rstr[o++] = Obtainx((c & 0xF0 ) >> 4 );
			rstr[o] = Obtainx(c & 0x0F );
		} 
		if ( has_str ) {//需要字符显示
			o = (clen+1)*(has_hex ?2:0) + j;
			if ( c >= 0x20 && c <= 0x7e )
				rstr[o] =  c;
			else
				rstr[o] =  '.';
		}
	}
	if ( has_hex) { //需要16进制显示
		o = j*2;
		rstr[o++] = ']';
		rstr[o] = '[';
	}
	o = (clen+1)*(has_hex ?2:0) + j;
	rstr[o++] = ']';
	tbuf.commit(o);
	tbuf.point[0] = 0;
	//printf("--- %s\n", tbuf.base);
	WLOG(INFO, (char*)tbuf.base);
}

void HttpIns::log_ins ()
{
	struct HInsData *hti;
	hti = (struct HInsData *)cur_insway->dat->ext_ins;
	if ( hti->me != this->gCFG ) return;	//不是本模块定义的, 不作处理
	switch ( hti->type) {
	case INS_FromRequest:
		log_ht(browser_req, "Browser Request Head", 0, true,true);
		break;
	case INS_FromResponse:
		log_ht(&response, "Client Reply Head", 0, true,true);
		break;
	default:
		break;
	}
}

void HttpIns::log_ht(DeHead *htp, const char *prompt, const char *err_code, bool has_head, bool force)
{
	TBuffer tbuf;
	struct HInsData *hti;
	unsigned char *p;

	hti = (struct HInsData *)cur_insway->dat->ext_ins;
	if ( force ) {
		if ( has_head ) {
			tbuf.input((unsigned char*)prompt, strlen(prompt));
			goto WRITE;
		}
	}
	if ( hti->log & 0x10) 	//正常日志
	{
		if ( has_head ) {
			tbuf.input((unsigned char*)prompt, strlen(prompt));
			goto WRITE;
		}
	} else  if ( cur_insway->dat->rcv_num > 0 && err_code)	//发生错误才记日志
	{
		if ( has_head ) {
			tbuf.input((unsigned char*)my_err_str, strlen(my_err_str));
			goto WRITE;
		}
	}
	return;
WRITE:
	tbuf.input((unsigned char*)" ", 1);
	tbuf.input((unsigned char*)htp->buff, htp->valid - (&htp->buff[0]));
	for ( p = tbuf.base; p < tbuf.point; p++)
	{
		if ( *p == '\0' ) { *p = '\r'; p++; *p='\n';}
	}
	tbuf.point[0] = 0;
	WLOG(INFO, (char*)tbuf.base);
}

void HttpIns::ans_ins ()
{
	ans_ins_ps.indic = cur_insway->reply;
	aptus->sponte(&ans_ins_ps);     //向左发出指令, 
}

HttpIns::HttpIns():cli_rcv(8192), cli_snd(8192), response(DeHead::RESPONSE), request(DeHead::REQUEST), browser_ans(DeHead::RESPONSE)
{
	gCFG = 0 ;
	has_config = false;

	fac_tbuf.indic = 0;
	fac_tbuf.ordo = Notitia::PRO_TBUF;
	spo_body.indic = 0;
	spo_body.ordo = Notitia::PRO_TBUF;

	spo_sethead.ordo = Notitia::CMD_SET_HTTP_HEAD;

	hitb[0] = &cli_snd;
	hitb[1] = &cli_rcv;
	hitb[2] = 0;
	rcv_buf = 0;
	ans_body_buf = 0;
	pro_hd_ps.ordo = Notitia::PRO_HTTP_HEAD;
	pro_hd_ps.indic = 0;
	pro_hd_ps.subor = Amor::CAN_ALL;

	ans_ins_ps.ordo =  Notitia::Ans_InsWay;
	ans_ins_ps.indic = 0;
	ans_ins_ps.subor = Amor::CAN_ALL;

	req_sent = false;
}

HttpIns::~HttpIns() {
	if ( has_config  ) {	
		if(gCFG) delete gCFG;
		gCFG = 0;
	}
}

Amor* HttpIns::clone()
{
	HttpIns *child;
	child =  new HttpIns();
	child->gCFG = gCFG;
	return (Amor*) child;
}

bool HttpIns::chunk_all(TBuffer *bo_buf)
{
	unsigned char *ptr, *c_base;	/* c_base指示在一个完整chunk之后, 或就是base */
	WBUG("Transfer-Encoding...");
	bool ret = false;

	c_base = (unsigned char*) 0;	/* 0: 表示刚从PRO_TBUF来,  */
HERE:
	if ( c_base == (unsigned char*) 0 )
		c_base = bo_buf->base + chunk_offset ; 

	if ( !chunko.started )
	{	/* 这一行应该是指示chunk长度 */
		for (ptr = c_base ; ptr < bo_buf->point - 1; ptr++ )
		{	/* 判断一行的CRLF */
			if ( *ptr == '\r' && *(ptr+1) == '\n' )
			{
				chunko.body_len = get_chunk_size(c_base);
				WBUG("A Chunk size " TLONG_FMT, chunko.body_len);
				chunko.head_len = (ptr - c_base) + 2;	/* 包括CRLF */
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
			for (ptr = &c_base[chunko.head_len-2] ; ptr < bo_buf->point - 3; ptr++ )
			{	/* 判断CRLFCRLF */
				if ( *ptr == '\r' && *(ptr+1) == '\n' &&
					*(ptr+2) == '\r' && *(ptr+3) == '\n' )
				{
					/* chunk传完了*/
					WBUG("Chunk completed!");
					ptr +=4 ;		/* ptr指向后面的数据, 另一个HTTP报文 */
					memmove(c_base, ptr, bo_buf->point - ptr);	/* 数据前移,footers数据被盖 */
					bo_buf->point -= (ptr - c_base);	
					chunko.reset();
					ret = true;
				}
			}
				
		} else if ( chunko.body_len > 0) {
				/* 具体数据 */
			TEXTUS_LONG bz = chunko.body_len + chunko.head_len +2;	/* 整个Chunk长度, 2是因为包括CRLF */
			if ( bo_buf->point - c_base >= bz ) 
			{
				/* 一个Chunk完整了 */
				WBUG("A Chunk cut out " TLONG_FMT " bytes", bz);
				/* 只留下data */
				ptr = &(c_base[chunko.head_len]); /* 指向数据区 */
				memmove(c_base, ptr, bo_buf->point - ptr);	/* body数据前移, head数据被盖 */
				bo_buf->point -= chunko.head_len;	
				c_base += chunko.body_len;	/* c_base指向了body结尾的CRLF */
				ptr = c_base +2;	/* ptr指向了本chunk后面的数据 */
				memmove(c_base, ptr, bo_buf->point - ptr);	/* 后数据前移, CRLF被挤掉 */
				bo_buf->point -= 2;	

				chunk_offset += chunko.body_len;
				chunko.reset();

				if ( c_base < bo_buf->point) /* 下面还有,再找下一个 */
				goto HERE;
			}
		}
	}
	return ret;
}

#include "hook.c"
