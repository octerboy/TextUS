/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 Title: HTTP Serivce Head proccess
 Build: reated by octerboy, 2005/06/10
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "casecmp.h"
#include "TBuffer.h"
#include "DeHead.h"
//#include "textus_string.h"
#include <sys/timeb.h>
#include <stdarg.h>
#ifndef TINLINE
#define TINLINE inline
#endif
class HttpSrvHead: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	HttpSrvHead();
	~HttpSrvHead();

	char server_name[64];
private:
	Amor::Pius local_pius;	//仅用于向左边传回数据
	
	bool isPoineer;
	bool session;
	bool channel_isAlive;
	int head_time_out;	/* 接收head的超时秒数 */
	void *tarr[3];
	Amor::Pius alarm_pius;	/* 设置超时 */
	bool hasTimer;	/* 已设定时否? */
	Amor::Pius clr_timer_pius;	/* 清超时 */
		
	long body_sent_len;	/* http体已发送字节数,以此决定是否结束当前session */
	long content_length;	/* 请求http体的字节数, rcv_buf中可能没有这么多字节 */
	
	char **ext_method;	/* 扩展method */

	TBuffer req_head_dup;	/* http头原始副本 */	
	TBuffer *rcv_buf;	/* 在http头完成后，这将是http体的内容 */

	TBuffer *snd_buf;
	TBuffer res_entity;	/* http响应体的内容 */

	DeHead request;
	DeHead response;

	TBuffer *res_head_buf;	/* http响应头, 用于直接设置而不用response的内容 */
	
	bool isAgent;	/* 是否要保存http请求头的原始数据，通常为否 */
	bool body_clean;	/* true: rcv_buf is empty. */
	
	TINLINE void end(bool force=false);
	TINLINE void req_reset();
	TINLINE void reset();
	TINLINE void clean_req();
	TINLINE bool requestError();
	TINLINE void deliver(Notitia::HERE_ORDO aordo);
#include "httpsrv_obj.h"
#include "wlog.h"
};

#include <assert.h>

void HttpSrvHead::ignite(TiXmlElement *cfg)
{
	const char *iscopy_str, *svr_str, *time_str, *name_str;
	TiXmlElement *mth_ele;
	int m, len, i=0;
	char *p=(char*)0, **q=(char**)0;
	
	iscopy_str = cfg->Attribute("agent");
	if ( iscopy_str && strcasecmp(iscopy_str,"yes") == 0)
		isAgent = true;
		
	if ( (svr_str = cfg->Attribute("server")) )
		TEXTUS_STRNCPY(server_name, svr_str, sizeof(server_name));

	if ( (time_str = cfg->Attribute("time_out")) &&  atoi(time_str) > 0 )
		head_time_out = atoi(time_str)*1000;

	if (ext_method)
		delete[] (char*) ext_method;
	ext_method = 0;

	mth_ele = cfg->FirstChildElement("method"); m = 0; len=0;
	while(mth_ele)
	{
		name_str = mth_ele->Attribute("name") ;
		if ( name_str )
		{
			len += strlen(name_str)+1;
			m++;
		}
		mth_ele = mth_ele->NextSiblingElement("method");
	}

	if ( m  > 0 )
	{
		i = sizeof(char*)*(m+1);
		p =  new char[i+len];
		q =  ext_method = (char**) p;
		q[m] = (char*) 0;
	}

	mth_ele = cfg->FirstChildElement("method"); m = 0;
	while(mth_ele)
	{
		name_str = mth_ele->Attribute("name") ;
		if ( name_str )
		{
			q[m] = &p[i];
			TEXTUS_STRCPY(q[m], name_str);
			i += strlen(name_str)+1;
			m++;
		}
		mth_ele = mth_ele->NextSiblingElement("method");
	}
	isPoineer = true;
}

bool HttpSrvHead::facio( Amor::Pius *pius)
{
	TBuffer **tb = 0;
	long len = 0;
	int pre_state; 
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF:
		WBUG("facio PRO_TBUF session %d, ReqStat %d", session, request.state);
		if ((tb = (TBuffer **)(pius->indic))) 
		{
			if ( *tb) rcv_buf = *tb;
			tb++;
			if ( *tb) snd_buf = *tb;
		}

		if ( !rcv_buf || !snd_buf )
		{
			WLOG(ERR, "facio PRO_TBUF null");
			break;
		}
HEADPRO:
		if ( request.state == DeHead::HeadOK ) /* Http head is ready, rest data still in rcv_buf */
		{
			deliver(Notitia::PRO_TBUF);
			goto END;
			break;
		}
		
		pre_state = request.state;
			
		WBUG("session %d, ReqStat %d", session, request.state);
		if ( (len = rcv_buf->point - rcv_buf->base) > 0 )
		{	/* feed data into the object of request */
			long fed_len;
			fed_len = request.feed((char*)rcv_buf->base, len);
			if ( isAgent ) /* save the http head data */
				req_head_dup.input(rcv_buf->base, fed_len);
			
			rcv_buf->commit(-fed_len); /* clear the head data  */
			
			if ( request.state == DeHead::HeadOK ) 
			{	/* session begins */
				const char *con_str;
    				time_t now;
				WBUG("Http Head is OK!");
				if ( hasTimer )
				{	/* clear timer */
					hasTimer = false;
					aptus->sponte(&clr_timer_pius);
				}

				session = true;	
				if ( requestError() ) break;

				content_length = request.getHeadInt("Content-Length");

				con_str= request.getHead("Connection");
				if ( !con_str )
					response.setHead("Connection", "close");
				else if ( strcasecmp(con_str, "Keep-alive") == 0 )
					response.setHead("Connection", "Keep-alive");
				else if ( strcasecmp(con_str, "close") == 0 )
					response.setHead("Connection", "close");
				else if ( strcasecmp(request.protocol, "HTTP/1.1") == 0 )
					response.setHead("Connection", "Keep-alive");
				else
					response.setHead("Connection", "close");

				//ftime(&now);
				time(&now);
				response.setHeadTime("Date", now);
				response.setHead("Server", server_name);

				deliver(Notitia::PRO_HTTP_HEAD); /* the right node can process http head */

			} else if ( pre_state == DeHead::HeadNothing 
				&& request.state == DeHead::Heading )
			{	/* set timer for the whole head */
				hasTimer = true;
				aptus->sponte(&alarm_pius);
			}
		}
END:
		WBUG("session %d, rest buf %ld bytes", session, rcv_buf->point - rcv_buf->base);
		if ( !session && rcv_buf->point > rcv_buf->base)	/* support pipe */
			goto HEADPRO;

		break;

	case Notitia::SET_TBUF:
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

	case Notitia::IGNITE_ALL_READY:	
		WLOG(INFO,"%s","tdate_parse - parse string dates into internal form, stripped-down version, Copyright (c)1995 by Jef Poskanzer <jef@acme.com>.  All rights reserved.");
		break;
		
	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		aptus->sponte(&clr_timer_pius);
		channel_isAlive = false;
		reset();
		break;

	case Notitia::START_SESSION:	/* 底层通信初始 */
		WBUG("facio START_SESSION");
		aptus->sponte(&clr_timer_pius);
		channel_isAlive = true;
		reset();
		WBUG("facio START_SESSION");
		break;

	case Notitia::TIMER:
		WLOG(WARNING, "http head time out");
		end(true);
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

bool HttpSrvHead::sponte( Amor::Pius *pius)
{
	assert(pius);
	struct GetRequestCmd *req_cmd = 0;
	struct SetResponseCmd *res_cmd = 0;
	
	switch ( pius->ordo )
	{
	case Notitia::CMD_HTTP_GET :	/* 取HTTP请求数据 */
		req_cmd = (struct GetRequestCmd *)pius->indic;
		assert(req_cmd);
		switch ( req_cmd->fun)
		{
		case GetRequestCmd::GetHead :
			req_cmd->valStr = request.getHead(req_cmd->name);
			WBUG("sponte CMD_HTTP_GET GetHead(\"%s\")=\"%s\"", req_cmd->name, req_cmd->valStr == 0? "": req_cmd->valStr);
			break;
						
		case GetRequestCmd::GetHeadArr :
			req_cmd->valStrArr = request.getHeadArray(req_cmd->name);
			WBUG("sponte CMD_HTTP_GET GetHeadArr(\"%s\")=\"%s\"", req_cmd->name, req_cmd->valStrArr == 0? "": req_cmd->valStrArr[0]);
			break;
						
		case GetRequestCmd::GetHeadInt:
			req_cmd->valInt = request.getHeadInt(req_cmd->name);
			WBUG("sponte CMD_HTTP_GET GetHead(\"%s\")=%ld", req_cmd->name,req_cmd->valInt);
			break;
				
		case GetRequestCmd::GetLenOfContent:
			req_cmd->len = content_length ;
			WBUG("sponte CMD_HTTP_GET GetLenOfContent=%ld",req_cmd->len);
			break;

		case GetRequestCmd::GetQuery:
			req_cmd->valStr = request.query;
			WBUG("sponte CMD_HTTP_GET GetQuery(\"%s\")", request.query);
			break;

		default:
			WLOG(NOTICE, "sponte CMD_HTTP_GET %d (it's unknown fun)",req_cmd->fun);
			req_cmd->valStr = 0;	
			req_cmd->valInt = 0;	
			req_cmd->len = 0;	
			req_cmd->filename =0;	
			req_cmd->type =0;	
			req_cmd->content =0;	
			break;
		}           
		break;          	
                                
	case Notitia::CMD_HTTP_SET :	/* 置HTTP响应数据 */
		res_cmd = (struct SetResponseCmd *)pius->indic;
		assert(res_cmd);
		switch ( res_cmd->fun)
		{
		case SetResponseCmd::SetHead :
			WBUG("sponte CMD_HTTP_SET SetHead(\"%s\")=\"%s\"", res_cmd->name, res_cmd->valStr);
			response.setHead(res_cmd->name, res_cmd->valStr);
			break;

		case SetResponseCmd::SetHeadInt :
			WBUG("sponte CMD_HTTP_SET SetHeadInt(\"%s\")=%ld", res_cmd->name, res_cmd->valInt);
			response.setHead(res_cmd->name, res_cmd->valInt);
			break;

		case SetResponseCmd::SetHeadTime :
			WBUG("sponte CMD_HTTP_SET SetHeadTime(\"%s\")=%lu", res_cmd->name, res_cmd->valTime);
			response.setHeadTime(res_cmd->name, res_cmd->valTime);
			break;

		case SetResponseCmd::AddHead :
			WBUG("sponte CMD_HTTP_SET AddHead(\"%s\")=\"%s\"", res_cmd->name, res_cmd->valStr);
			response.addHead(res_cmd->name, res_cmd->valStr);
			break;

		case SetResponseCmd::AddHeadInt :
			WBUG("sponte CMD_HTTP_SET AddHead(\"%s\")=%ld", res_cmd->name, res_cmd->valInt);
			response.addHead(res_cmd->name, res_cmd->valInt);
			break;

		case SetResponseCmd::SetStatus :
		case SetResponseCmd::SendError :
			WBUG("sponte CMD_HTTP_SET SetStatus(%d)", res_cmd->sc);
			response.setStatus(res_cmd->sc);
			break;

		case SetResponseCmd::SetLenOfContent :
			response.setHead("Content-Length", res_cmd->len);
			WBUG("sponte CMD_HTTP_SET SetLenOfContent(%ld)", response.content_length);
			break;
			
		case SetResponseCmd::GetHead :
			res_cmd->valStr = response.getHead(res_cmd->name);
			WBUG("sponte CMD_HTTP_SET GetHead(\"%s\")=\"%s\"", res_cmd->name, res_cmd->valStr);
			break;
					
		case SetResponseCmd::GetHeadInt:
			res_cmd->valInt = response.getHeadInt(res_cmd->name);
			WBUG("sponte CMD_HTTP_SET GetHead(\"%s\")=%ld", res_cmd->name, res_cmd->valInt);
				break;
		default:
			WLOG(NOTICE, "sponte CMD_HTTP_SET %d (it's unknown fun)",res_cmd->fun);
			break;
		}
		break;

	case Notitia::PRO_HTTP_HEAD :	/* 响应的HTTP头已经准备好 */
		WBUG("sponte PRO_HTTP_HEAD");
		if(session)	/* 必须有http请求才有http响应 */
		{
			if (res_head_buf)
			{	/* 直接设置响应报文头 */
				if ( snd_buf->point == snd_buf->base )
				{
					TBuffer::exchange(*res_head_buf, *snd_buf);
				} else
				{
					long len = res_head_buf->point - res_head_buf->base;
					snd_buf->input(res_head_buf->base, len);
					res_head_buf->commit(-len);
				}
			} else /* 一般的方法 */
			{
				if ( request.method_type == DeHead::HEAD )
				{	/* 响应不包括message body */
					response.setHead("Content-Length", (long)0);
				}
				snd_buf->commit(
				    response.getContent((char*)snd_buf->point, snd_buf->limit- snd_buf->point));
			}
			aptus->sponte(&local_pius); //先发回一次
		}
		if ( response.content_length == 0 )
			end();
		break;

	case Notitia::PRO_TBUF :	/* 响应的HTTP体已经准备好,但未必是全部的 */
		WBUG("sponte PRO_TBUF session %d", session);
		if(session)	/* 必须有http请求并且没有中断才有http响应 */
		{
			body_sent_len += (res_entity.point - res_entity.base);
			TBuffer::pour(*snd_buf, res_entity);
			aptus->sponte(&local_pius); //第二次发回
			if (response.content_length >=0 && body_sent_len >= response.content_length)
			{	/* 如果知道body长度, 那么就能知道会话是否结束 */
				WBUG("http body has sent out");
				end();	/* 结束会话 */
			}
		} else
			res_entity.reset();	/* 将响应数据丢弃 */
			
		break;

	case Notitia::DMD_END_SESSION:	/* 强制关闭 */
		WBUG("sponte DMD_END_SESSION");
		aptus->sponte(&clr_timer_pius);
		end(true);
		break;
		
	case Notitia::CMD_GET_HTTP_HEADBUF:	/* 取HTTP请求头的原始数据 */
		WBUG("sponte CMD_GET_HTTP_HEADBUF");
		pius->indic = &req_head_dup;
		break;
	
	case Notitia::CMD_GET_HTTP_HEADOBJ:	/* 取HTTP请求头的原始数据 */
		WBUG("sponte CMD_GET_HTTP_HEADOBJ");
		pius->indic = &request;
		break;
	
	case Notitia::HTTP_Request_Complete:
		WBUG("sponte HTTP_Request_Complete");
		if ( content_length < 0 && pius->indic )
		content_length = *((long*)(pius->indic));
		break;
	
	case Notitia::HTTP_Request_Cleaned:
		WBUG("sponte HTTP_Request_Cleaned");
		body_clean = true;
		break;

	case Notitia::HTTP_Response_Complete:
		WBUG("sponte HTTP_Response_Complete");
		end();
		break;

	case Notitia::CMD_SET_HTTP_HEAD:	/* 设置HTTP响应头的原始数据,每次会话都要重新设置 */
		WBUG("sponte CMD_SET_HTTP_HEAD");
		res_head_buf = (TBuffer *)pius->indic;
		break;

	default:
		return false;
	}
	return true;
}

HttpSrvHead::HttpSrvHead():req_head_dup(512), res_entity(8192), 
	request(DeHead::REQUEST),response(DeHead::RESPONSE)
{
	local_pius.ordo = Notitia::PRO_TBUF;
	local_pius.indic = 0;

	rcv_buf = 0;
	snd_buf = 0;
	isPoineer = false;	/* 开始认为自己不是开拓者 */
	isAgent = false;	/* 通常不用保存请求头的原始数据 */
	memset(server_name, 0, sizeof(server_name));

	clr_timer_pius.ordo = Notitia::DMD_CLR_TIMER;
	clr_timer_pius.indic = 0;	/* shed or tpoll give*/

	alarm_pius.ordo = Notitia::DMD_SET_ALARM;
	alarm_pius.indic = &tarr[0];
	tarr[0] = this;
	tarr[1] = &head_time_out;
	tarr[2] = 0;
	head_time_out = 32000;
	hasTimer = false;

	ext_method = 0;
	request.ext_method = &ext_method;

	channel_isAlive = false;
	reset();
}

void HttpSrvHead::clean_req()
{
	if ( !body_clean && content_length > 0 )
	{	/* 清除已接收的HTTP体的数据 */
		long out_len = rcv_buf->point - rcv_buf->base;
		rcv_buf->commit( content_length < out_len ? -content_length: -out_len);
		body_clean = true;
	}
}

void HttpSrvHead::req_reset()
{
	request.reset();
	req_head_dup.reset();
	content_length = -1;
}

void HttpSrvHead::reset()
{
	req_reset();
	response.reset();
	res_entity.reset();
	res_head_buf = (TBuffer*) 0;
	body_sent_len = 0;
	session = false;
}

HttpSrvHead::~HttpSrvHead() {
	if ( isPoineer)
	{
		if ( ext_method)
			delete[] (char*)ext_method;
	}
}

Amor* HttpSrvHead::clone()
{
	HttpSrvHead *child = new HttpSrvHead();
	child->isAgent = isAgent;
	child->head_time_out = head_time_out;
	child->ext_method = ext_method;
	memcpy(child->server_name, server_name, sizeof(server_name));
	return (Amor*) child;
}

TINLINE void HttpSrvHead::end(bool force)
{
	if ( channel_isAlive)
	if ( force || (session && strcasecmp(response.getHead("Connection"), "keep-alive") != 0) )
	{
		Amor::Pius tmp_pius;
		WBUG("will terminate session");
		tmp_pius.ordo = Notitia::END_SESSION;
		tmp_pius.indic = 0;
		aptus->sponte(&tmp_pius);
	}
	clean_req();
	body_clean = false;
	reset();
}

/* 向接力者提交 */
TINLINE void HttpSrvHead::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	TBuffer *tb[3];
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	
	switch (aordo)
	{
		case Notitia::SET_TBUF:
			WBUG("deliver SET_TBUF");
			tb[0] = rcv_buf;
			tb[1] = &res_entity;
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

/* Set the response, send it and stop communication if HTTP head has error */
TINLINE bool HttpSrvHead::requestError()
{
	if ( request.status == 200 ) return false;
		
	response.setStatus(request.status);
	WLOG(NOTICE, "request error %s", response.title);
	WBUG("Http request encounter %d, %s", request.status, response.title);

	res_entity.grant(2048);
	res_entity.point +=TEXTUS_SNPRINTF((char*)res_entity.point, 1024, "<HTML><HEAD><TITLE>%d %s</TITLE></HEAD>\n<BODY BGCOLOR=\"#cc9999\"><H4> %d %s</H4>\n", response.status, response.title, response.status, response.title);

	res_entity.point +=TEXTUS_SNPRINTF((char*)res_entity.point, 1024, "<HR>\n<ADDRESS><A HREF=\"%s\">HttpSrvHead version %s</A></ ADDRESS>\n</BODY></HTML>\n", "httpsrvheaderr.html", SCM_MODULE_ID );

	response.setHead("Connection", "close");
	response.setHead("Content-Length",res_entity.point - res_entity.base);

	snd_buf->commit(
		response.getContent((char*)snd_buf->point, snd_buf->limit - snd_buf->point));
	aptus->sponte(&local_pius); /* the first send to IE etc. */

	if ( !session ) 
		return true;	/* Oh, Do not send */
	TBuffer::exchange(res_entity, *snd_buf);
	aptus->sponte(&local_pius); /* the second to IE etc. */

	end();
	return true;	
}

#include "hook.c"
