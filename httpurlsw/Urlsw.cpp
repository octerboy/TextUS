/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Aptus extension, call this module depending on uri
 Build: created by octerboy, 2006/07/14，Guangzhou
 $Header: /textus/httpurlsw/Urlsw.cpp 24    12-04-04 16:57 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: Urlsw.cpp $"
#define TEXTUS_MODTIME  "$Date: 12-04-04 16:57 $"
#define TEXTUS_BUILDNO  "$Revision: 24 $"
/* $NoKeywords: $ */

#include "Aptus.h"
#include "BTool.h"
#include "Notitia.h"
#include "textus_string.h"
#include <time.h>
#include <stdarg.h>

class Urlsw: public Aptus {
public:
	void ignite_t (TiXmlElement *wood, TiXmlElement *);
	Amor *clone();

	Urlsw();
	~Urlsw();
	bool dextra(Amor::Pius *, unsigned int);
	bool laeve(Amor::Pius *, unsigned int);

	int p_num;	/* 路径数 */
	struct MPath {
		char *val;	
		bool nocase;	/* 不区分大小写, 默认为否, 即"S.xml"与"s.xml"是不同的 */
		const char *head_field;
	};
	MPath *paths;	/* 路径数组, 所指向空间也包括具体的内容 */
	const char *url;	/* URI */
	
	TEXTUS_ORDO concerned;	//关心的ordo
	bool catchLae;
	
	Amor::Pius stop;

	bool isPoineer;
	inline bool canMatch();
	inline void dexHandle(Amor::Pius *, unsigned int from);
	inline void laeHandle(Amor::Pius *, unsigned int from);
#include "httpsrv_obj.h"
#include "tbug.h"
};

#include "casecmp.h"
#include "textus_string.h"

Urlsw::Urlsw()
{
	p_num = 0;
	paths = 0;
	url = (char*) 0;

	stop.ordo = Notitia::DMD_STOP_NEXT;
	stop.indic = 0;
	isPoineer = false;
	concerned = Notitia::TEXTUS_RESERVED;
	catchLae = false;
}

Urlsw::~Urlsw()
{
	if ( isPoineer)
	{
		if ( paths)
		{
			delete[] paths[0].val;
			delete[] paths;
		}
	}
}

void Urlsw::ignite_t (TiXmlElement *cfg, TiXmlElement *url_ele)
{	
	const char *comm_str, *g_field;
	TiXmlElement *p_ele;
	int p_len, m;
	char *p=(char*)0;
	bool g_case;

	WBUG("prius %p, aptus %p, cfg %p", prius, aptus, cfg);
	if ( !url_ele) return;

	comm_str = url_ele->Attribute("ordo");
	BTool::get_textus_ordo(&concerned, comm_str);

	g_field = url_ele->Attribute("field");

	comm_str = url_ele->Attribute("case");
	if ( comm_str && strcasecmp(comm_str, "no") == 0 )
		g_case = true;
	else
		g_case = false;

	comm_str = url_ele->Attribute("laeve");
	if ( comm_str && strcasecmp(comm_str, "yes") == 0 )
		catchLae = true;
	else
		catchLae = false;

	p_ele = url_ele->FirstChildElement("match"); p_num = 0; p_len = 0;
	while(p_ele)
	{
		comm_str = p_ele->GetText();
		if ( comm_str )
		{
			p_len += strlen(comm_str)+1;
			p_num++;
		}
		p_ele = p_ele->NextSiblingElement("match");
	}

	if ( p_num > 0 )
	{
		need_dex = true;
		canAccessed = true;
		paths = new struct MPath [sizeof(struct MPath)*p_num];
		p =  new char[p_len];
		memset(p, 0, p_len);
		need_lae = catchLae;
	}

	WBUG("this p_num %d\n", p_num);
	p_ele = url_ele->FirstChildElement("match"); m = 0;
	while(p_ele)
	{
		comm_str = p_ele->GetText();
		if ( comm_str )
		{
			const char *c_str;
			c_str = p_ele->Attribute("case");
			if ( c_str && strcasecmp(c_str, "no") == 0 )
				paths[m].nocase = true;
			else if ( c_str && strcasecmp(c_str, "yes") == 0 )
				paths[m].nocase = false;
			else	
				paths[m].nocase = g_case;

			if ( !(paths[m].head_field = p_ele->Attribute("field")))
				paths[m].head_field = g_field;	/* 此若不指明HTTP头的域则取全局 */
			paths[m].val = p;
			TEXTUS_STRCPY(p, comm_str);
			p += strlen(comm_str)+1;
			m++;
		}
		p_ele = p_ele->NextSiblingElement("match");
	}
	
	isPoineer = true;       /* 认为自己是开拓者 */
}

Amor *Urlsw::clone()
{	
	Urlsw *child = 0;
	child = new Urlsw();
	Aptus::inherit( (Aptus*)child );
	child->p_num =  p_num;
	child->paths =  paths;
	child->concerned = concerned;
	child->catchLae = catchLae;
	
	return  (Amor*)child;
}

bool Urlsw::laeve(Amor::Pius *pius, unsigned int from)
{
	if ( concerned == pius->ordo)
		laeHandle(pius, from);
	else
		return false;
	return true;
}

bool Urlsw::dextra(Amor::Pius *pius, unsigned int from)
{
	switch (pius->ordo)
	{
	case Notitia::PRO_HTTP_HEAD:	/* HTTP请求HEAD */
		WBUG("Urlsw dextra PRO_HTTP_HEAD owner is %p", owner);
#ifndef NDEBUG 
		goto HANDLEPRO;
#endif
	case Notitia::PRO_HTTP_REQUEST:	/* HTTP请求 */
		WBUG("Urlsw dextra PRO_HTTP_REQUEST owner is %p", owner);
#ifndef NDEBUG 
		goto HANDLEPRO;
#endif
	case Notitia::PRO_TINY_XML:	/* XML请求 */
		WBUG("Urlsw dextra PRO_TINY_XML owner is %p", owner);
#ifndef NDEBUG 
		goto HANDLEPRO;
#endif
	case Notitia::PRO_SOAP_HEAD:	/* SOAP请求HEAD */
		WBUG("Urlsw dextra PRO_SOAP_HEAD owner is %p", owner);
#ifndef NDEBUG 
		goto HANDLEPRO;
#endif
	case Notitia::PRO_SOAP_BODY:	/* SOAP请求BODY */
		WBUG("Urlsw dextra PRO_SOAP_BODY owner is %p", owner);
#ifndef NDEBUG 
	HANDLEPRO:
#endif
		dexHandle(pius, from);
		break;
			
	default:
		if ( concerned == pius->ordo)
			dexHandle(pius, from);
		else
			return false;
	}
	return true;
}

void Urlsw::dexHandle(Amor::Pius *pius, unsigned int from)
{
	if ( canMatch())
	{
		WBUG("Urlsw dextra matched for owner %p", owner );
		((Aptus *) aptus)->dextra(pius, from+1);/* 继续完成对本owner的访问,不影响其它Assistant */
		if( prius)  prius->laeve(&stop, 0) ;	/* 终止对下一个节点的访问 */
	}
}

void Urlsw::laeHandle(Amor::Pius *pius, unsigned int from)
{
	if ( canMatch())
	{
		WBUG("Urlsw laeve matched for owner %p", owner );
		((Aptus *) aptus)->laeve(pius, from+1);/* 继续完成对本owner的访问,不影响其它Assistant */
	}
}

bool Urlsw::canMatch()
{
	bool match;
	int i;
	const char *head_val;

	match = false;
	for (i = 0; i <p_num && !match ; i++)
	{
		WBUG("dextra head_field %s for owner %p, match %s", paths[i].head_field=0?"Path":paths[i].head_field, owner, paths[i].val );
		if ( paths[i].head_field)
		{
			head_val = getHead( (char*) paths[i].head_field);
		} else {
			if (!url ) 
				url = getHead("Path");
			head_val=url;
		}
		
		if ( !head_val ) 
			continue;

		WBUG("dextra head value %s", head_val );
		if ( paths[i].nocase )
		{
			match = ( strcasecmp(head_val, paths[i].val ) == 0 );
		}  else {
			match = ( strcmp(head_val, paths[i].val ) == 0 );
		} 
	}

	return match;
}

#define TEXTUS_APTUS_TAG { 'U', 'R', 'L', 0};
#include "hook.c"
