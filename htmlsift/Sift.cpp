/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: HTML Content Filter
 Build: created by octerboy, 2006/08/27, Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "casecmp.h"
#include "textus_string.h"
#include <time.h>
#include <stdarg.h>

#define HTTPSRVINLINE inline
class Sift: public Amor
{
public:
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	Sift();
private:
	Amor::Pius local_pius;	//仅用于向左边传回数据
	
	const char *mime;
	const char *host;
	
	TBuffer *rcv_buf;	/* 在http头完成后，这将是http体的内容 */
	TBuffer *snd_buf;

	HTTPSRVINLINE void handle();
#include "httpsrv_obj.h"
#include "wlog.h"
};

#include <assert.h>

bool Sift::facio( Amor::Pius *pius)
{
	TBuffer **tb = 0;
	switch ( pius->ordo )
	{
	case Notitia::PRO_HTTP_RESPONSE:	/* 可以处理HTTP响应数据了 */
		WBUG("facio PRO_HTTP_RESPONSE");
		handle();
		break;

	case Notitia::SET_TBUF:	/* 取得输入TBuffer地址 */
		WBUG("facio SET_TBUF");
		if ( (tb = (TBuffer **)(pius->indic)))
		{	//tb应当不为NULL，*tb是rcv_buf
			if ( *tb) rcv_buf = *tb; 
			else
				WLOG(WARNING, "facio SET_TBUF rcv_buf null");
			tb++;
			if ( *tb) snd_buf = *tb;
			else
				WLOG(WARNING, "facio SET_TBUF snd_buf null");
			aptus->facio(pius);
		} else 
			WLOG(WARNING, "facio SET_TBUF null");
		break;

	default:
		WBUG("facio Notitia::" TLONG_FMTu, pius->ordo);
		return false;
	}

	return true;
}

bool Sift::sponte( Amor::Pius *pius)
{
	switch ( pius->ordo )
	{
	case Notitia::PRO_HTTP_RESPONSE:	/* 可以处理HTTP响应数据了 */
		WBUG("sponte PRO_HTTP_RESPONSE");
		handle();
		break;
		
	default:
		return false;
	}
	return true;
}

Sift::Sift()
{
	rcv_buf = 0;
	snd_buf = 0;
}

Amor* Sift::clone()
{
	Sift *child = new Sift();
	return (Amor*)child;
}

HTTPSRVINLINE void Sift::handle()
{
	register unsigned char *p, *q;
	TEXTUS_LONG len;
	int slen;
	unsigned char src[128];
	unsigned char dst[128];
	//char *host="www.eoooo.net";

	if ( !snd_buf )
	{
		WLOG(WARNING, "snd_buf is null!");
		return;
	}

	mime = getResHead("Content-Type");
	host = getHead("Host");
	WBUG("mime %s", mime);
	WBUG("host %s", host);
	
	if ( mime && host && (strcasecmp( mime, "text/html")== 0 
		|| strncasecmp( mime, "text/html;", 10)== 0 ) )
	{
		int alen;
		slen = static_cast<int>(strlen(host));
		if ( slen > 110 )
			slen = 110;
		memcpy(src, "http://", 7);
		memcpy(&src[7], host, slen);
		//src[slen+7] = '/';
		src[slen+7] = '\0';
		slen +=7;

		memset(dst, ' ', slen+7);
		memcpy(dst, "https://", 8);
		alen = static_cast<int>(strlen("192.168.3.2"));
		memcpy(&dst[8], "192.168.3.2", alen);
		dst[alen+8] = '\0';
		alen += 8;
		printf("dst....... %s\n", dst);
		len = snd_buf->point - snd_buf->base - slen;
		if (len < 0 ) 
			return;
		p = snd_buf->base;
		q = snd_buf->point - alen;
		while( p < q )
		{
			if ( strncasecmp("http://192.168.3.20", (char*)p, alen) == 0 )	/* 找到相要替换的内容 */
			{
/*
				unsigned char *con = p + slen;	
				unsigned char *ptr = con;	
				while ( *ptr != '\'' && *ptr !='\"' && ptr < snd_buf->point ) 
					ptr++;
				if ( ptr < snd_buf->point )
				{
					memmove(p, con, ptr-con + 1);
					p += ptr-con + 1;
					memcpy(p, dst, slen);
					p += slen;
					
				} else {
	*/		
					WBUG("substitue %s", src);
					memcpy(p, dst, alen);
					p += alen;
		//		}
				continue;
			}
			p++;
		}
	}
}

#include "hook.c"
