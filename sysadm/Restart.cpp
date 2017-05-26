/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Application restart
 Build: created by octerboy 2006/09/26, Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "textus_string.h"
#include "casecmp.h"
#include <time.h>
#include <stdarg.h>

class Restart :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	Restart();

private:
	Amor::Pius local_pius;  //仅用于传回数据
	Amor::Pius end_pius;  //仅用于传回数据
	char page[256];
#include "httpsrv_obj.h"
#include "wlog.h"
};

void Restart::ignite(TiXmlElement *cfg) {
	const char *page_str;
	if ( cfg && (page_str = cfg->Attribute("redirect")) )
		TEXTUS_STRNCPY( page, page_str, sizeof(page)-1);
}

bool Restart::facio( Amor::Pius *pius)
{
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::PRO_HTTP_REQUEST:	/* 有HTTP请求 */
	case Notitia::PRO_HTTP_HEAD:	/* 有HTTP请求 */
		WBUG("facio PRO_HTTP_REQUEST/HEAD");
		local_pius.ordo = Notitia::CMD_MAIN_EXIT;
		aptus->sponte(&local_pius);
		if ( strlen(page) > 0 )
		{
			setStatus(303);
			setHead("Location", page);
			output(page);
			local_pius.ordo = Notitia::PRO_HTTP_HEAD;
			aptus->sponte(&local_pius);
			local_pius.ordo = Notitia::PRO_TBUF;
			aptus->sponte(&local_pius);
		} else { 
			sendError(503);
			setContentSize(0);
			local_pius.ordo = Notitia::PRO_HTTP_HEAD;
			aptus->sponte(&local_pius);
		}
		break;

	   default:
		return false;
	}
	return true;
}

bool Restart::sponte( Amor::Pius *pius) { return false; }

Restart::Restart()
{
	local_pius.indic = 0;
	memset(page, 0, sizeof(page));
}

Amor* Restart::clone()
{
	Restart *child = new Restart();
	memcpy(child->page, page, sizeof(page));
	return  (Amor*)child;
}

#include "hook.c"
