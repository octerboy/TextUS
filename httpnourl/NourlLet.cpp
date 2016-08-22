/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Pro for invalid url
 Build:created by octerboy 2005/04/12
 $Header: /textus/httpnourl/NourlLet.cpp 19    13-11-02 15:25 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: NourlLet.cpp $"
#define TEXTUS_MODTIME  "$Date: 13-11-02 15:25 $"
#define TEXTUS_BUILDNO  "$Revision: 19 $"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "textus_string.h"

#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <assert.h>

class NourlLet :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	NourlLet();
	~NourlLet();

private:
#include "httpsrv_obj.h"
	Amor::Pius local_pius;  //仅用于传回数据
	Amor::Pius end_pius;  //仅用于传回数据
	char page[256];
	TiXmlElement ans_rt;
#include "wlog.h"
};

void NourlLet::ignite(TiXmlElement *cfg) { 
	const char *page_str;
	if ( cfg && (page_str = cfg->Attribute("redirect")) )
		TEXTUS_STRNCPY( page, page_str, sizeof(page)-1);
}

bool NourlLet::facio( Amor::Pius *pius)
{
	const char *p;
	TiXmlText *c_val,*s_val;
	TiXmlElement *c_ele, *s_ele;

	const char *path;
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::PRO_HTTP_HEAD:	/* 有HTTP请求 */
	case Notitia::PRO_HTTP_REQUEST:	/* 有HTTP请求 */
		WBUG("facio PRO_HTTP_HEAD/REQUEST");
		path = getHead("Path");
		p = getHead("Query");
		WLOG(INFO, "No process for %s?%s", path, p == 0 ? "": p);
	
		if ( page[0] && strcmp(path, page)!=0 )
		{
			setStatus(303);
			setHead("Location", page);
			output(page);
			local_pius.ordo = Notitia::PRO_HTTP_HEAD;
			aptus->sponte(&local_pius);
			local_pius.ordo = Notitia::PRO_TBUF;
			aptus->sponte(&local_pius);
		} else { 
			sendError(404);
			setContentSize(0);
			local_pius.ordo = Notitia::PRO_HTTP_HEAD;
			aptus->sponte(&local_pius);
		}
		aptus->sponte(&end_pius);
		break;

	case Notitia::PRO_SOAP_BODY:	/* 有SOAP请求 */
		c_val = new TiXmlText("Invalid_Operation");
		s_val = new TiXmlText("no such SoapAction or operation");
		c_ele = new TiXmlElement("faultcode");
		s_ele = new TiXmlElement("faultstring");

		ans_rt.Clear();
		c_ele->LinkEndChild(c_val);	
		s_ele->LinkEndChild(s_val);	
		ans_rt.LinkEndChild(c_ele);	
		ans_rt.LinkEndChild(s_ele);	
		local_pius.ordo = Notitia::ERR_SOAP_FAULT;
		local_pius.indic = &ans_rt;
		aptus->sponte(&local_pius);
		break;

	default:
		return false;
	}
	return true;
}

bool NourlLet::sponte( Amor::Pius *pius) { return false; }
Amor* NourlLet::clone()
{
	NourlLet *child = new NourlLet();
	memcpy(child->page, page,sizeof(page));
	return  (Amor*)child;
}

NourlLet::NourlLet():ans_rt("Response")
{
	local_pius.ordo = Notitia::PRO_HTTP_HEAD;
	local_pius.indic = 0;
	end_pius.ordo = Notitia::END_SESSION;
	end_pius.indic = 0;
	page[0] = '\0';
}

NourlLet::~NourlLet() { } 
#include "hook.c"

