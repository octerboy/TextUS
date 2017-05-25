/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Aptus extesioin, keep the indic ( or array) for specific ordo
 Build: created by octerboy, 2006/06/24, Panyu
 $Header: /textus/keep/Keep.cpp 12    12-04-04 16:58 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: Keep.cpp $"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Aptus.h"
#include "Notitia.h"

class Keep: public Aptus {
public:
	void ignite_t (TiXmlElement *wood, TiXmlElement *);
	Amor *clone();

	Keep();
	bool facio_n (Amor::Pius *, unsigned int);
	bool laeve( Amor::Pius *pius, unsigned int);

private:
	TEXTUS_ORDO set_ordo;	/* 要记录的ordo */
	int style;			/* 0:仅记录indic, -1:记录indic数组中的每一个指针,以空指针结尾;
					   >0: 指定数组个数 */
	Amor::Pius rec;
#include "tbug.h"
};

#include "textus_string.h"
#include "casecmp.h"
Keep::Keep()
{
	set_ordo = Notitia::SET_TBUF;
	style = 2;	/* 为了SET_TBUF而默认 */
	rec.indic = 0;
}

void Keep::ignite_t (TiXmlElement *cfg, TiXmlElement *ke_ele)
{
	const char *set_str, *style_str;
	WBUG("this %p, prius %p, aptus %p, cfg %p", this, prius, aptus, cfg);
	
        if ( !ke_ele) return;

	canAccessed = true;
	if ( (set_str = ke_ele->Attribute("set")) )
	{
		if (strcasecmp(set_str , "SET_TBUF") == 0 )
			set_ordo = Notitia::SET_TBUF;
		else
			set_ordo = atoi(set_str);
		need_fac = true;
	}		

	if ( (style_str = ke_ele->Attribute("style")) )
		style = atoi(style_str);

	if ( style > 0 )
	{
		if ( rec.indic ) delete[] (void **)(rec.indic);	
		rec.indic = new void* [style];
	}
}

Amor *Keep::clone()
{	
	Keep *child = 0;
	child = new Keep();
	Aptus::inherit( (Aptus*) child);
	child->set_ordo = set_ordo;
	child->style = style;
	
	return  (Amor*)child;
}

bool Keep::laeve( Amor::Pius *pius, unsigned int from)
{
	/* 取出indic */
	if (pius->ordo ==  Notitia::CMD_GET_PIUS )
	{	
		pius->indic = &rec;
		return true;
	} else 
		return false;
}

bool Keep::facio_n (Amor::Pius *pius, unsigned int from)
{	/* 在owner->facio()之时 */
	if ( pius->ordo == set_ordo)
	{
		rec.ordo = set_ordo;	
		if ( style > 0 )
		{	/* 指定数组元素个数 */
			for ( int i = 0 ; i < style; i++)
			{
				((void **)(rec.indic))[i] = ((void **)(pius->indic))[i]  ;
			}
		} else if ( style == 0 )
		{	/* 直接赋值 */
			rec.indic = pius->indic;
		} else 
		{	/* 以空指针结尾的情况 */
			int i,j;
			void **p ;
			if ( rec.indic ) delete[] (void **) (rec.indic);	
			rec.indic = 0;
			p = (void**) pius->indic; i = 0;
			while (	!(*p) )
			{
				i++;
				p++;
			}

			if ( i > 0 )
				rec.indic = new void* [i+1];
			
			for ( j = 0; j <= i ; j++)
			{
				((void **)(rec.indic))[j] = ((void **)(pius->indic))[j]  ;
			}
		}
	}
	return false;	/* pass */
}
#define TEXTUS_APTUS_TAG { 'K','e','e','p', 0};
#include "hook.c"
