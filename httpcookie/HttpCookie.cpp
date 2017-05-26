/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title:HttpCookieing Pro
 Build: created by octerboy, 2006/11/02, Hebi(Henan)
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "SetCookie.h"
#include "Amor.h"
#include "Notitia.h"
#include "casecmp.h"
#include "textus_string.h"
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#if !defined (_WIN32)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#define  BUF_MAX 512
class HttpCookie: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	HttpCookie();
	~HttpCookie();
#include "httpsrv_obj.h"

private:
	bool isPoineer ;
	int buf_max;
	char *mybuf, *valbuf;
	struct Kie {
		char *name;
		char *value;
		inline Kie() {
			name = value = 0;
		};
	};
	Kie *kies;	/* end with NULL pointer */
	int kNum;	/* How many */

	char domain[256];
#include "wlog.h"
};

#include <assert.h>

void HttpCookie::ignite(TiXmlElement *cfg) 
{
	const char *comm_str;
	comm_str = cfg->Attribute("buf_size");
	if (  comm_str && atoi(comm_str) > 0 )
		buf_max = atoi(comm_str);
	else
		buf_max = BUF_MAX;
	mybuf = new char [ buf_max];
	valbuf = new char [buf_max];
}

HttpCookie::HttpCookie()
{
	mybuf = 0;
	valbuf = 0;
	
	kNum = 1;
	kies = new Kie[kNum];
}

HttpCookie::~HttpCookie() 
{
	if ( mybuf )
		delete[] mybuf;
	if ( valbuf )
		delete[] valbuf;
	delete []kies;
}

Amor* HttpCookie::clone()
{
	HttpCookie *child = new HttpCookie();

#define INH(X) child->X = X;
	INH(buf_max);
	child->mybuf = new char[child->buf_max];
	child->valbuf = new char[child->buf_max];
	
	return (Amor*) child;
}

bool HttpCookie::facio( Amor::Pius *pius)
{
	assert(pius);
	const char *kie_str;
	char *ptr, *ptr2 ,*end;
	int count;
	int i, rlen, len;
	struct Kie *p_k;

	switch ( pius->ordo )
	{
	case Notitia::PRO_HTTP_HEAD:
	case Notitia::PRO_HTTP_REQUEST:
		WBUG("facio PRO_HTTP_REQUEST/PRO_HTTP_HEAD");
		memset(kies, 0, sizeof(struct Kie)*kNum);
		kie_str = getHead("Cookie");
		if ( kie_str )
		{
			len = strlen(kie_str);
			rlen = len < buf_max-1 ? len : buf_max-1;
			memcpy(mybuf, kie_str, rlen ); 
			mybuf[rlen] = 0;
		} else
			goto LAST;

		/* caculate the cookie number */
		for( count = 0,ptr = mybuf,i=0; i < rlen; ptr++, i++)
			if ( *ptr == '=' ) count ++;
	
		WBUG("cookie count = %d",count);

		if( count > kNum-1)
		{
			delete []kies;
			kNum = count+1;
			kies = new Kie[kNum];
		}
		
		// Begin Parse the cookie 
		end = &mybuf[rlen];
		for(ptr = mybuf, ptr2 = mybuf, p_k = kies; 
			ptr < end; ptr++)
		{
			if( *ptr== '=' ) // name end
			{
				char *q;
				p_k->name = ptr2;
				q = ptr;
				while ( q >= p_k->name 
					&& (*q == ' ' || *q == '\t' || *q == '=' ) 
				) q--;
				*++q = '\0';

				q = ptr;
				q++;
				p_k->value = q;
				while ( *q != ';' && q < end ) q++;
				*q++ = '\0';
				ptr2 = ptr = q;
				while ( ptr2 < end && (*ptr2 == ' ' || *ptr2 == '\t') ) 
					ptr2++;	/* 指向下一个name*/
				p_k++;
			}
		}
	LAST:
		aptus->facio(pius);		
		break;

	default:
		return false;
	}
	return true;
}

bool HttpCookie::sponte( Amor::Pius *pius)
{
	char *name = 0;
	char **dic;
	const char *host, *dom;
	struct SetCookie *coo;
	struct Kie *p_k;
	int bufLen;
	
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::GET_DOMAIN:
		WBUG("sponte GET_DOMAIN");
		host = getHead("Host");
#define TDOMAIN "Domain"
		dom = getHead(TDOMAIN);
		if( dom)
		{
			pius->indic = (void*)dom;
		} else if ( host )
		{	/* 给出域 */
			struct in_addr in;
			char *p, *q;

			memset(domain,0,sizeof(domain));
			memcpy(domain, host, strlen(host));
			q = domain;
			p = strpbrk(q, ":");
			if ( p ) *p = '\0';
			p  = &q[strlen(q)];
#if defined (_WIN32)
			if ( (in.s_addr = inet_addr(q)) != INADDR_NONE )
				goto END;
#else
       			if ( inet_aton((const char *)q, &in) != 0 ) /* 这是一个IP地址 */
				goto END;
#endif
			p = q;
			while ( *p != '\0' )
			{
				if ( *p == '.' ) break;
				p++;
			}
		END:
			pius->indic = (void*)p;
		}
		break;

	case Notitia::GET_COOKIE:
		WBUG("sponte GET_COOKIE");
		dic = (char **)(pius->indic);
		if ( dic)
		{
			if ( *dic) 
				name = *dic;	/* 要取的cookie名称 */
			else
				WLOG(WARNING, "facio GET_COOKIE name is null");
			dic++;
			p_k = kies ;
			while(p_k->name )
			{
				if (strcmp(p_k->name , name) == 0 )
				{
					*dic = p_k->value;
					break;
				}
				p_k++;
			}
		} else 
			WLOG(WARNING, "facio GET_COOKIE null");
		break;

	case Notitia::SET_COOKIE:
		WBUG("sponte SET_COOKIE");
		bufLen = 0;
		coo = (struct SetCookie *)(pius->indic);
		if ( coo )
		{
			memset(valbuf, 0, buf_max);
			if ( coo->name && coo->value)
				bufLen +=TEXTUS_SNPRINTF(&valbuf[bufLen], buf_max-bufLen, "%s=%s;", coo->name, coo->value);
			if ( coo->expires)
				bufLen +=TEXTUS_SNPRINTF(&valbuf[bufLen], buf_max-bufLen, "expires=%s;", coo->expires);
			if ( coo->path)
				bufLen +=TEXTUS_SNPRINTF(&valbuf[bufLen], buf_max-bufLen, "path=%s;", coo->path);
			if ( coo->domain)
				bufLen +=TEXTUS_SNPRINTF(&valbuf[bufLen], buf_max-bufLen, "domain=%s;", coo->domain);
			valbuf[bufLen-1] = '\0';

			addHead("Set-Cookie", valbuf);
		} else 
			WLOG(WARNING, "facio SET_COOKIE null");

		break;

	default:
		return false;
	}
	return true;
}

#include "hook.c"
