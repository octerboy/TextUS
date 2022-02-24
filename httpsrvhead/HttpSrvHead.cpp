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
#include <stdarg.h>
class HttpSrvHead: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	HttpSrvHead();
	~HttpSrvHead();

	Amor::Pius alarm_pius;	/* ���ó�ʱ */
	Amor::Pius clr_timer_pius;	/* �峬ʱ */
	void *tarr[3];
	bool has_config;
	struct G_CFG {
		Amor::Pius tmp_pius;	/* �峬ʱ */
		char server_name[64];
		int head_time_out;	/* ����head�ĳ�ʱ������ */
		char **ext_method;	/* ��չmethod */
		bool isAgent;	/* �Ƿ�Ҫ����http����ͷ��ԭʼ���ݣ�ͨ��Ϊ�� */
		Amor *sch;
		G_CFG( TiXmlElement *cfg) {
			const char *iscopy_str, *svr_str, *time_str, *name_str;
			TiXmlElement *mth_ele;
			int m;
			size_t len;
			TEXTUS_LONG i=0;
			char *p=(char*)0, **q=(char**)0;

			sch = 0;
			iscopy_str = cfg->Attribute("agent");
			if ( iscopy_str && strcasecmp(iscopy_str,"yes") == 0)
				isAgent = true;
		
			memset(server_name, 0, sizeof(server_name));
			if ( (svr_str = cfg->Attribute("server")) )
				TEXTUS_STRNCPY(server_name, svr_str, sizeof(server_name));

			head_time_out = 32000;
			if ( (time_str = cfg->Attribute("time_out")) &&  atoi(time_str) > 0 )
				head_time_out = atoi(time_str);

			tmp_pius.ordo = Notitia::CMD_GET_SCHED;
			tmp_pius.indic = 0;

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
		};
		 ~G_CFG() {
			if ( ext_method)
				delete[] (char*)ext_method;
		};
	};
	struct G_CFG *gCFG;
private:
	Amor::Pius local_pius;	//����������ߴ�������
	
	bool session;
	bool channel_isAlive;
	bool hasTimer;	/* ���趨ʱ��? */
		
	TEXTUS_LONG body_sent_len;	/* http���ѷ����ֽ���,�Դ˾����Ƿ������ǰsession */
	TEXTUS_LONG content_length;	/* ����http����ֽ���, rcv_buf�п���û����ô���ֽ� */
	

	TBuffer req_head_dup;	/* httpͷԭʼ���� */	
	TBuffer *rcv_buf;	/* ��httpͷ��ɺ��⽫��http������� */

	TBuffer *snd_buf;
	TBuffer res_entity;	/* http��Ӧ������� */

	DeHead request;
	DeHead response;

	TBuffer *res_head_buf;	/* http��Ӧͷ, ����ֱ�����ö�����response������ */
	
	bool body_clean;	/* true: rcv_buf is empty. */
	
	void end(bool force=false);
	void req_reset();
	void reset();
	void clean_req();
	bool requestError();
	void deliver(Notitia::HERE_ORDO aordo);
#include "httpsrv_obj.h"
#include "wlog.h"
};

#include <assert.h>

void HttpSrvHead::ignite(TiXmlElement *cfg)
{
	if (!cfg) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
}

bool HttpSrvHead::facio( Amor::Pius *pius)
{
	TBuffer **tb = 0;
	TEXTUS_LONG len = 0;
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
			WBUG("session %d, rest buf " TLONG_FMT " bytes", session, rcv_buf->point - rcv_buf->base);
			deliver(Notitia::PRO_TBUF);
			break;
		}
		
		pre_state = request.state;
			
		WBUG("session %d, ReqStat %d", session, request.state);
		if ( (len = rcv_buf->point - rcv_buf->base) > 0 )
		{	/* feed data into the object of request */
			TEXTUS_LONG fed_len;
			fed_len = request.feed((char*)rcv_buf->base, len);
			if ( gCFG->isAgent ) /* save the http head data */
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
					gCFG->sch->sponte(&clr_timer_pius);
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

				time(&now);
				response.setHeadTime("Date", now);
				response.setHead("Server", gCFG->server_name);

				deliver(Notitia::PRO_HTTP_HEAD); /* the right node can process http head */

			} else if ( pre_state == DeHead::HeadNothing 
				&& request.state == DeHead::Heading )
			{	/* set timer for the whole head */
				hasTimer = true;
				gCFG->sch->sponte(&alarm_pius);
			}
		}
END:
		WBUG("session %d, rest buf " TLONG_FMT " bytes", session, rcv_buf->point - rcv_buf->base);
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
		WBUG("facio IGNITE_ALL_READY");
		if ( gCFG ) {
			tarr[1] = &gCFG->head_time_out;
		}
		aptus->sponte(&(gCFG->tmp_pius));	//��tpoll, ȡ��sched
		gCFG->sch = (Amor*)(gCFG->tmp_pius.indic);
		if ( !gCFG->sch ) 
		{
			WLOG(ERR, "no sched or tpoll");
		}
		WBUG("get sched = %p", gCFG->sch);
		reset();
		request.ext_method = &gCFG->ext_method;
		break;

        case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		if ( gCFG ) {
			tarr[1] = &gCFG->head_time_out;
		}
		reset();
		request.ext_method = &gCFG->ext_method;
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		gCFG->sch->sponte(&clr_timer_pius);
		channel_isAlive = false;
		reset();
		break;

	case Notitia::START_SESSION:	/* �ײ�ͨ�ų�ʼ */
		WBUG("facio START_SESSION");
		gCFG->sch->sponte(&clr_timer_pius);
		channel_isAlive = true;
		reset();
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
	case Notitia::CMD_HTTP_GET :	/* ȡHTTP�������� */
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
			WBUG("sponte CMD_HTTP_GET GetHead(\"%s\")=" TLONG_FMT , req_cmd->name,req_cmd->valInt);
			break;
				
		case GetRequestCmd::GetHeadULong:
			req_cmd->valULong = request.getHeadULong(req_cmd->name);
			WBUG("sponte CMD_HTTP_GET GetHead(\"%s\")=" TLONG_FMTu , req_cmd->name,req_cmd->valULong);
			break;
				
		case GetRequestCmd::GetLenOfContent:
			req_cmd->len = content_length ;
			WBUG("sponte CMD_HTTP_GET GetLenOfContent=" TLONG_FMT, req_cmd->len);
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
                                
	case Notitia::CMD_HTTP_SET :	/* ��HTTP��Ӧ���� */
		res_cmd = (struct SetResponseCmd *)pius->indic;
		assert(res_cmd);
		switch ( res_cmd->fun)
		{
		case SetResponseCmd::SetHead :
			WBUG("sponte CMD_HTTP_SET SetHead(\"%s\")=\"%s\"", res_cmd->name, res_cmd->valStr);
			response.setHead(res_cmd->name, res_cmd->valStr);
			break;

		case SetResponseCmd::SetHeadInt :
			WBUG("sponte CMD_HTTP_SET SetHeadInt(\"%s\")=" TLONG_FMT, res_cmd->name, res_cmd->valInt);
			response.setHead(res_cmd->name, res_cmd->valInt);
			break;

		case SetResponseCmd::SetHeadULong :
			WBUG("sponte CMD_HTTP_SET SetHeadULong(\"%s\")=" TLONG_FMTu, res_cmd->name, res_cmd->valULong);
			response.setHead(res_cmd->name, res_cmd->valULong);
			break;

		case SetResponseCmd::SetHeadTime :
			WBUG("sponte CMD_HTTP_SET SetHeadTime(\"%s\")=" TLONG_FMTu , res_cmd->name, res_cmd->valTime);
			response.setHeadTime(res_cmd->name, res_cmd->valTime);
			break;

		case SetResponseCmd::AddHead :
			WBUG("sponte CMD_HTTP_SET AddHead(\"%s\")=\"%s\"", res_cmd->name, res_cmd->valStr);
			response.addHead(res_cmd->name, res_cmd->valStr);
			break;

		case SetResponseCmd::AddHeadInt :
			WBUG("sponte CMD_HTTP_SET AddHead(\"%s\")=" TLONG_FMT, res_cmd->name, res_cmd->valInt);
			response.addHead(res_cmd->name, res_cmd->valInt);
			break;

		case SetResponseCmd::SetStatus :
		case SetResponseCmd::SendError :
			WBUG("sponte CMD_HTTP_SET SetStatus(%d)", res_cmd->sc);
			response.setStatus(res_cmd->sc);
			break;

		case SetResponseCmd::SetLenOfContent :
			response.setHead("Content-Length", res_cmd->len);
			WBUG("sponte CMD_HTTP_SET SetLenOfContent(" TLONG_FMT ")", response.content_length);
			break;
			
		case SetResponseCmd::GetHead :
			res_cmd->valStr = response.getHead(res_cmd->name);
			WBUG("sponte CMD_HTTP_SET GetHead(\"%s\")=\"%s\"", res_cmd->name, res_cmd->valStr);
			break;
					
		case SetResponseCmd::GetHeadInt:
			res_cmd->valInt = response.getHeadInt(res_cmd->name);
			WBUG("sponte CMD_HTTP_SET GetHead(\"%s\")=" TLONG_FMT, res_cmd->name, res_cmd->valInt);
				break;

		case SetResponseCmd::GetHeadULong:
			res_cmd->valULong = response.getHeadULong(res_cmd->name);
			WBUG("sponte CMD_HTTP_SET GetHead(\"%s\")=" TLONG_FMTu, res_cmd->name, res_cmd->valULong);
				break;
		default:
			WLOG(NOTICE, "sponte CMD_HTTP_SET %d (it's unknown fun)",res_cmd->fun);
			break;
		}
		break;

	case Notitia::PRO_HTTP_HEAD :	/* ��Ӧ��HTTPͷ�Ѿ�׼���� */
		WBUG("sponte PRO_HTTP_HEAD");
		if(session)	/* ������http�������http��Ӧ */
		{
			if (res_head_buf)
			{	/* ֱ��������Ӧ����ͷ */
				if ( snd_buf->point == snd_buf->base )
				{
					TBuffer::exchange(*res_head_buf, *snd_buf);
				} else
				{
					TEXTUS_LONG len = res_head_buf->point - res_head_buf->base;
					snd_buf->input(res_head_buf->base, len);
					res_head_buf->commit(-len);
				}
			} else /* һ��ķ��� */
			{
				if ( request.method_type == DeHead::HEAD )
				{	/* ��Ӧ������message body */
					response.setHead("Content-Length", (TEXTUS_LONG)0);
				}
				snd_buf->commit(
				    response.getContent((char*)snd_buf->point, snd_buf->limit- snd_buf->point));
			}
			aptus->sponte(&local_pius); //�ȷ���һ��
		}
		if ( response.content_length == 0 )
			end();
		break;

	case Notitia::PRO_TBUF :	/* ��Ӧ��HTTP���Ѿ�׼����,��δ����ȫ���� */
		WBUG("sponte PRO_TBUF session %d", session);
		if(session)	/* ������http������û���жϲ���http��Ӧ */
		{
			body_sent_len += (res_entity.point - res_entity.base);
			TBuffer::pour(*snd_buf, res_entity);
			aptus->sponte(&local_pius); //�ڶ��η���
			if (response.content_length >=0 && body_sent_len >= response.content_length)
			{	/* ���֪��body����, ��ô����֪���Ự�Ƿ���� */
				WBUG("http body has sent out");
				end();	/* �����Ự */
			}
		} else
			res_entity.reset();	/* ����Ӧ���ݶ��� */
			
		break;

	case Notitia::DMD_END_SESSION:	/* ǿ�ƹر� */
		WBUG("sponte DMD_END_SESSION");
		aptus->sponte(&clr_timer_pius);
		end(true);
		break;
		
	case Notitia::CMD_GET_HTTP_HEADBUF:	/* ȡHTTP����ͷ��ԭʼ���� */
		WBUG("sponte CMD_GET_HTTP_HEADBUF");
		pius->indic = &req_head_dup;
		break;
	
	case Notitia::CMD_GET_HTTP_HEADOBJ:	/* ȡHTTP����ͷ��ԭʼ���� */
		WBUG("sponte CMD_GET_HTTP_HEADOBJ");
		pius->indic = &request;
		break;
	
	case Notitia::HTTP_Request_Complete:
		WBUG("sponte HTTP_Request_Complete");
		if ( content_length < 0 && pius->indic )
		content_length = *((TEXTUS_LONG*)(pius->indic));
		break;
	
	case Notitia::HTTP_Request_Cleaned:
		WBUG("sponte HTTP_Request_Cleaned");
		body_clean = true;
		break;

	case Notitia::HTTP_Response_Complete:
		WBUG("sponte HTTP_Response_Complete");
		end();
		break;

	case Notitia::CMD_SET_HTTP_HEAD:	/* ����HTTP��Ӧͷ��ԭʼ����,ÿ�λỰ��Ҫ�������� */
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
	gCFG = 0;
	has_config = false;	/* ��ʼ��Ϊ�Լ����ǿ����� */

	hasTimer = false;
	channel_isAlive = false;
	clr_timer_pius.ordo = Notitia::DMD_CLR_TIMER;
	clr_timer_pius.indic = 0;	/* shed or tpoll give*/
			
	alarm_pius.ordo = Notitia::DMD_SET_ALARM;
	alarm_pius.indic = &tarr[0];
	tarr[0] = this;
	tarr[2] = 0;
	reset();
}

void HttpSrvHead::clean_req()
{
	if ( !body_clean && content_length > 0 )
	{	/* ����ѽ��յ�HTTP������� */
		TEXTUS_LONG out_len = rcv_buf->point - rcv_buf->base;
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
	if ( has_config)
	{
		if(gCFG) delete gCFG;
		gCFG = 0;
	}
}

Amor* HttpSrvHead::clone()
{
	HttpSrvHead *child = new HttpSrvHead();
	child->gCFG = gCFG;
	return (Amor*) child;
}

void HttpSrvHead::end(bool force)
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

/* ��������ύ */
void HttpSrvHead::deliver(Notitia::HERE_ORDO aordo)
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
bool HttpSrvHead::requestError()
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
