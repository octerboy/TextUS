/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Aptus extension, call this module depending on uri
 Build: created by octerboy, 2006/07/14��Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "BTool.h"
#include "Notitia.h"
#include "textus_string.h"
#include "casecmp.h"
#include <time.h>
#include <stdarg.h>

class Urlsw: public Amor{
public:

	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);

	Amor *clone();
	Urlsw();
	~Urlsw();

	int p_num;	/* ·����, Ҳ�����ٸ�ƥ���� */
	struct MPath {
		const char *val;	//��xml�ļ��еõ���Ҫƥ���ֵ
		bool nocase;	/* �����ִ�Сд, Ĭ��Ϊ��, ��"S.xml"��"s.xml"�ǲ�ͬ�� */
		const char *head_field;	//HTTPͷ����, ���Ϊ��, ��ȡpath
		int subor;		//ƥ���, ���������ordo
	};
	MPath *paths;	/* ·������, ��ָ��ռ�Ҳ������������� */
	const char *url;	/* URI */
	
	TEXTUS_ORDO concerned;	//���ĵ�ordo
	int cur_subor;	//��ǰ������ordo

	bool isPoineer;
	inline bool canMatch();
#include "httpsrv_obj.h"
#include "wlog.h"
};

Urlsw::Urlsw()
{
	p_num = 0;
	paths = 0;
	url = (const char*) 0;

	isPoineer = false;
	concerned = Notitia::TEXTUS_RESERVED;
}

Urlsw::~Urlsw()
{
	if ( isPoineer)
	{
		if ( paths)
		{
			delete[] paths;
		}
	}
}

void Urlsw::ignite(TiXmlElement *cfg)
{	
	const char *comm_str, *g_field;
	TiXmlElement *p_ele;
	int m;
	bool g_case;

	concerned = Notitia::get_ordo(cfg->Attribute("ordo"));

	g_field = cfg->Attribute("field");

	comm_str = cfg->Attribute("case");
	if ( comm_str && strcasecmp(comm_str, "no") == 0 )
		g_case = true;
	else
		g_case = false;


	p_ele = cfg->FirstChildElement("match"); p_num = 0;
	while(p_ele)
	{
		comm_str = p_ele->GetText();
		if ( comm_str )
		{
			p_num++;
		}
		p_ele = p_ele->NextSiblingElement("match");
	}

	if ( p_num > 0 )
	{
		paths = new struct MPath [sizeof(struct MPath)*p_num];
	}

	WBUG("this p_num %d\n", p_num);
	p_ele = cfg->FirstChildElement("match"); m = 0;
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
				paths[m].head_field = g_field;	/* ������ָ��HTTPͷ������ȡcfg�趨�� */

			paths[m].subor = 0;
			p_ele->QueryIntAttribute("subor", &(paths[m].subor));
			paths[m].val = comm_str;
			m++;
		}
		p_ele = p_ele->NextSiblingElement("match");
	}
	
	isPoineer = true;       /* ��Ϊ�Լ��ǿ����� */
}

Amor *Urlsw::clone()
{	
	Urlsw *child = 0;
	child = new Urlsw();
	child->p_num =  p_num;
	child->paths =  paths;
	child->concerned = concerned;
	
	return  (Amor*)child;
}

bool Urlsw::sponte(Amor::Pius *pius)
{
	if ( concerned == pius->ordo)
		pius->subor = cur_subor;
	return false;
}

bool Urlsw::facio(Amor::Pius *pius)
{
	switch (pius->ordo)
	{
	case Notitia::PRO_HTTP_HEAD:	/* HTTP����HEAD */
		WBUG("Urlsw facio PRO_HTTP_HEAD");
		goto HANDLEPRO;

	case Notitia::PRO_HTTP_REQUEST:	/* HTTP���� */
		WBUG("Urlsw facio PRO_HTTP_REQUEST");
		goto HANDLEPRO;

	case Notitia::PRO_TINY_XML:	/* XML���� */
		WBUG("Urlsw dextra PRO_TINY_XML");
		goto HANDLEPRO;

	case Notitia::PRO_SOAP_HEAD:	/* SOAP����HEAD */
		WBUG("Urlsw facio PRO_SOAP_HEAD");
		goto HANDLEPRO;

	case Notitia::WebSock_Start:	/* WebSockЭ�鿪ʼ */
		WBUG("Urlsw facio WebSock_Start");
		goto HANDLEPRO;

	case Notitia::PRO_SOAP_BODY:	/* SOAP����BODY */
		WBUG("Urlsw facio PRO_SOAP_BODY");

HANDLEPRO:
	if ( canMatch() ) 
		pius->subor = cur_subor;
		break;
			
	default:
		WBUG("Urlsw facio Notitia:%d", concerned);
		if ( concerned == pius->ordo && canMatch() ) 
			pius->subor = cur_subor;
		break;
	}
	return false;
}

bool Urlsw::canMatch()
{
	bool match;
	int i;
	const char *head_val;

	match = false;
	for (i = 0; i <p_num && !match ; i++)
	{
		WBUG("match head_field %s, match %s", paths[i].head_field=0?"Path":paths[i].head_field, paths[i].val );
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

		WBUG("match head value %s", head_val );
		if ( paths[i].nocase )
		{
			match = ( strcasecmp(head_val, paths[i].val ) == 0 );
		}  else {
			match = ( strcmp(head_val, paths[i].val ) == 0 );
		} 
		if (match ) 
		{
			cur_subor =  paths[i].subor;
			break;
		}
	}

	return match;
}
/* ���ٲ���Attachment�ķ�ʽ��, ƥ���ֵsubor */
#include "hook.c"
