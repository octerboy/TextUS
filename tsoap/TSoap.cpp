/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Tiny Soap
 Build: created by octerboy 2006/10/13, Guangzhou
 $Header: /textus/tsoap/TSoap.cpp 18    08-01-10 1:17 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: TSoap.cpp $"
#define TEXTUS_MODTIME  "$Date: 08-01-10 1:17 $"
#define TEXTUS_BUILDNO  "$Revision: 18 $"
/* $NoKeywords: $ */

#include "Notitia.h"
#include "Amor.h"
#include "textus_string.h"

#include <stdarg.h>
#include <time.h>
#if !defined (_WIN32)
#include <sys/time.h>
#endif
#include <errno.h>
#include "textus_string.h"
#include "Wsdl.h"
#define TSPINLINE inline
#define BZERO(X) memset(X, 0, sizeof(X));

class TSoap :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	TSoap();
	~TSoap();

private:
	Amor::Pius local_pius;  //仅用于向mary传回数据
	Amor::Pius head_p;
	Amor::Pius body_p;
	TiXmlDocument *req_doc, *res_doc;	
	TiXmlElement *head, *body;
	Message *last;
	time_t now;
	bool has_config;

	struct G_CFG {
		char envVal[512], headVal[512], bodyVal[512], faultVal[512];
		Wsdl *wsdl;
		TiXmlElement *res_root;
		inline G_CFG() {
			BZERO(envVal)
			BZERO(headVal)
			BZERO(bodyVal)
			BZERO(faultVal)
			wsdl = 0;
			res_root = 0;
		};
		inline ~G_CFG() {
			if(res_root) delete res_root;
			if(wsdl) delete wsdl;
			if(wsdl) delete wsdl;
		};
	};
	struct G_CFG *gCFG;	/* 全局共享参数 */

	void TSPINLINE handle();
	void TSPINLINE setSoap( char [], Amor::Pius *, bool err=false);
	#include "httpsrv_obj.h"
	#include "wlog.h"
};

void TSoap::ignite(TiXmlElement *cfg) 
{ 
	NString *n;
	char *ql;
	int len;

	if (!cfg) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
	}

	if ( gCFG->wsdl ) 
		delete gCFG->wsdl;
	gCFG->wsdl = new Wsdl();

	gCFG->wsdl->parse(cfg->FirstChildElement());
	ql = gCFG->wsdl->qlt["SOAP-ENV"].value;
	len = strlen(ql);
	if ( len > 500 ) len = 500;

	BZERO(gCFG->envVal);
	BZERO(gCFG->headVal);
	BZERO(gCFG->bodyVal);
	BZERO(gCFG->faultVal);
	if ( len > 0 )
	{
#define SETCOLON(X) \
		memcpy(gCFG->X, ql, len);	\
		gCFG->X[len]=':';

		SETCOLON(envVal)
		SETCOLON(headVal)
		SETCOLON(bodyVal)
		SETCOLON(faultVal)
		len++;
	}

	memcpy(&gCFG->envVal[len], "Envelope", 8);
	memcpy(&gCFG->headVal[len], "Head", 4);
	memcpy(&gCFG->bodyVal[len], "Body", 4);
	memcpy(&gCFG->faultVal[len], "Fault", 5);

	if ( gCFG->res_root)
		delete gCFG->res_root;
	gCFG->res_root = new TiXmlElement(gCFG->envVal);

	while ( (n=gCFG->wsdl->env_attr.get()) )
	{
		gCFG->res_root->SetAttribute((const char*)n->nm, (const char*)n->value);
	}
}

bool TSoap::facio( Amor::Pius *pius)
{
	TiXmlDocument **doc=0;
	const char *query = 0;
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::PRO_HTTP_REQUEST:	/* 有请求, 但不是soap请求 */
		WBUG("facio PRO_HTTP_REQUEST");
		query = getHead("Query");
		if (query && strcasecmp(query, "wsdl") == 0 && gCFG->wsdl->asmx)
		{
			if ( now > 0  && now == getHeadInt("If-Modified-Since")) 
			{	
				setStatus(304);
				setContentSize(0);
			} else {
				/* now == 0 : I have never sent 
				    now !=  : expired 
				*/
				time(&now);
				setHeadTime("Last-Modified", now);
				setContentSize(gCFG->wsdl->asm_len);
				output(gCFG->wsdl->asmx, gCFG->wsdl->asm_len);
			}
			setHead("Content-Type", "text/xml");
			local_pius.ordo = Notitia::PRO_HTTP_RESPONSE;
			local_pius.indic = 0;
			aptus->sponte(&local_pius);
		} else
			aptus->facio(pius);
		break;

	case Notitia::PRO_TINY_XML:	/* 有XML数据请求 */
		WBUG("facio PRO_TINY_XML");
		if( req_doc )
			handle();
		break;

	case Notitia::SET_TINY_XML:	/* 取得输入xmlDoc地址 */
		WBUG("facio SET_TINY_XML");
		if ( (doc = (TiXmlDocument **)(pius->indic)))
		{	//tb应当不为NULL，*tb是rcv_buf
			if ( *doc) req_doc = *doc; 
			else
				WLOG(WARNING, "facio SET_TINY_XML req_doc null");
			doc++;
			if ( *doc) res_doc = *doc;
			else
				WLOG(WARNING, "facio SET_TINY_XML res_doc null");
		} else 
			WLOG(WARNING, "facio SET_TINY_XML null");
		break;

	case Notitia::IGNITE_ALL_READY:	
	case Notitia::CLONE_ALL_READY:
		break;
		
	default:
		return false;
	}
	return true;
}

bool TSoap::sponte( Amor::Pius *pius) 
{
	switch ( pius->ordo )
	{
	case Notitia::PRO_SOAP_HEAD:
		WBUG("sponte PRO_SOAP_HEAD");
		setSoap(gCFG->headVal, pius);
		break;

	case Notitia::ERR_SOAP_FAULT:
		WBUG("sponte ERR_SOAP_FAULT");
		setSoap(gCFG->bodyVal, pius, true);
		goto XML_BACK;

	case Notitia::PRO_SOAP_BODY:
		WBUG("sponte PRO_SOAP_BODY");
		setSoap(gCFG->bodyVal, pius);
XML_BACK:
		local_pius.ordo = Notitia::PRO_TINY_XML;
		local_pius.indic = 0;
		aptus->sponte(&local_pius);
		break;
	default:
		return false;
	}
	return true;
}

TSoap::TSoap()
{
	head_p.ordo = Notitia::PRO_SOAP_HEAD;
	head_p.indic = 0;
	body_p.ordo = Notitia::PRO_SOAP_BODY;
	body_p.indic = 0;

	res_doc = req_doc = 0;
	head = body = 0;
	last = 0;
	now = 0;
	gCFG = 0;
	has_config = false;
}

TSoap::~TSoap()
{
	if ( has_config  )
	{	
		if(gCFG) delete gCFG;
	}
}

Amor* TSoap::clone()
{
	TSoap *child = new TSoap();
	child->gCFG = gCFG;
	return  (Amor*) child;
}

void TSoap::handle()
{
	char tag[256];
	const char *soapAction;
	int len = 0, len2= 0;
	TiXmlElement *reqele = req_doc->RootElement();
	char *nsq = Wsdl::getQualifier((char*)reqele->Value());
		
	len2 = len = strlen(nsq);

	if ( len2 > 0 )  
	{
		memcpy(tag, nsq, len2);
		tag[len2] = ':'; len2++;
	}
	memcpy(&tag[len2], "Header", 4); len2 +=4;
	tag[len2] = '\0';
	head = reqele->FirstChildElement(tag);
	head_p.indic = head;

	if ( len > 0 )
	{
		memcpy(tag, nsq, len);
		tag[len2] = ':'; len++;
	}
	memcpy(&tag[len], "Body", 4); len +=4;
	tag[len] = '\0';
	body = reqele->FirstChildElement(tag);
	if ( body )
	{
		soapAction = getHead("SOAPAction");
		gCFG->wsdl->msgs.rewind();
		if ( soapAction )
		while ( (last = gCFG->wsdl->msgs.get()) )	/* 找到相应message定义 */
		{
			if ( last->soapAction && 
				( strcmp(last->soapAction, soapAction) == 0 
				  || strcmp(last->soapAction2, soapAction) == 0) )
				break;
		}

		if ( last && (last->fWrapped || last->fRpc ) )
			body_p.indic = body->FirstChildElement(); /* 这是一个方法子节点 */
		else
			body_p.indic = body;
	}

	if ( head_p.indic )
		aptus->facio(&head_p);	/* 给出head */
	aptus->facio(&body_p);	/* 给出body */
}

void TSoap::setSoap( char val[], Amor::Pius *pius, bool err)
{
	TiXmlElement *ele, *root;
	TiXmlElement abody(val);
	
	if (!res_doc ) return;

	root = res_doc->RootElement();
	if ( !root )
	{
		res_doc->InsertEndChild(*(gCFG->res_root));
		root = res_doc->RootElement();
	}
	
	if ( pius->indic )
	{
		ele = (TiXmlElement *)pius->indic;
		if ( err )
		{
			ele->SetValue(gCFG->faultVal);
			abody.InsertEndChild(*ele);
		} else if (last && (last->fWrapped || last->fRpc ) )
		{
			/* 客户端用了个MSOSOAPLib30, 发现有下面一行,
				" Operation.Load reader, False "
				这个操作总是不行, 去掉就可以了. 不知是内容不对, 还是本不该有
			if ( gCFG->wsdl->targetns)
				ele->SetAttribute("xmlns",gCFG->wsdl->targetns);
			*/

			if ( last->response && last->response->args[0].elem )
				ele->SetValue(last->response->args[0].elem);
				
			abody.InsertEndChild(*ele);
		} else {
			TiXmlElement *each;
			each = ele->FirstChildElement();
			while ( each)
			{
				abody.InsertEndChild(*each);
				each = each->NextSiblingElement();
			}
		}
	}
	last = 0;
	root->InsertEndChild(abody);
}
#include "hook.c"
