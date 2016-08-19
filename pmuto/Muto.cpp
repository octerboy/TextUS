/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Aptus extension, exchange array element(0,1) of pius->indic
 Build: created by octerboy, 2006/10/08£¬Guangzhou
 $Header: /textus/pmuto/Muto.cpp 9     08-01-10 1:15 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: Muto.cpp $"
#define TEXTUS_MODTIME  "$Date: 08-01-10 1:15 $"
#define TEXTUS_BUILDNO  "$Revision: 9 $"
/* $NoKeywords: $ */

#include "Aptus.h"
#include "Notitia.h"

class Muto: public Aptus {
public:
	void ignite_t (TiXmlElement *wood, TiXmlElement *);
	Amor *clone();

	Muto();
	bool dextra(Amor::Pius *, unsigned int);
	Notitia::HERE_ORDO mordo;
#include "tbug.h"
};

#include "casecmp.h"
#include "textus_string.h"

Muto::Muto()
{
	mordo = Notitia::TEXTUS_RESERVED;
}

void Muto::ignite_t (TiXmlElement *cfg, TiXmlElement *mt_ele)
{	
	const char *comm_str;	
	if ( (comm_str = mt_ele->Attribute("ordo")) ) 
	{
#define GON(X) if ( strcasecmp(comm_str, #X) == 0 ) mordo = Notitia::X;
		GON(SET_TBUF);
		GON(SET_UNIPAC);
		GON(SET_TINY_XML);
#undef GON
		if (mordo == Notitia::TEXTUS_RESERVED )
			mordo = (Notitia::HERE_ORDO)atoi(comm_str);
	}
	canAccessed = true;
	need_dex = true;
}

Amor *Muto::clone()
{	
	Muto *child = 0;
	child = new Muto();
	Aptus::inherit( (Aptus*)child );
	child->mordo = mordo;
	
	return  (Amor*)child;
}

bool Muto::dextra(Amor::Pius *pius, unsigned int from)
{
	void *tb[3];
	void **pt;
	Amor::Pius another;
	if (pius->ordo == mordo)
	{
		WBUG("Muto dextra Notitia::%d owner is %p", pius->ordo, owner);
		another.ordo = mordo;
		another.indic = tb;
		tb[2] = 0;
		if ( (pt = (void **)pius->indic) )
		{
			if ( *pt) 
				tb[1] = *pt; 
			pt++;
			if ( *pt) tb[0] = *pt;

		}
		((Aptus *) aptus)->dextra(&another, from+1);	
		return true;
	}

	return false;
}
#define TEXTUS_APTUS_TAG { 'M', 'u', 't', 'o', 0};
#include "hook.c"
