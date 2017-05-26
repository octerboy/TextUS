/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Induco attachment
 Desc: Extending of Aptus. it makes another object cloned when one amor object clones
 Build: :created by octerboy, 2006/08/04, Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Animus.h"

class Induco: public Aptus {
public:
	void ignite_t (TiXmlElement *wood, TiXmlElement *);
	Amor *clone();
	bool facio(Pius *);

	Induco();
	~Induco();
	
	void hup(Animus *); 

protected:
	static Induco *g_effect;	/* the first effect node, only global one */
	Animus *end;			/* the effect node */

	bool isCause;			/* true: this class is for cause node */
	bool save_end;			/* true: keep end which will be deleted */
#include "tbug.h"
};

#include "Notitia.h"
#include "textus_string.h"
#include <stdio.h>
#include <string.h>

Induco* Induco::g_effect= (Induco*) 0;

void Induco::ignite_t (TiXmlElement *cfg, TiXmlElement *sz_ele)
{
	const char *comm_str;

	WBUG("this %p , prius %p, aptus %p, cfg %p, owner %p", this, prius, aptus, cfg, owner);

	if ( !sz_ele) return;
	
	canAccessed = true;

	isCause = true;
	if ( (comm_str =sz_ele->Attribute("causation")) && strcmp(comm_str, "effect") == 0 )
	{	/* this is effect node */
		g_effect = this;
		isCause = false;
	}

	save_end = true;
	if ( (comm_str =sz_ele->Attribute("save")) && strcmp(comm_str, "no") == 0  )
		save_end = false;
}

bool Induco::facio( Amor::Pius *pius)
{      
	switch ( pius->ordo)
	{
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		if ( g_effect )
			hup( (Animus *) (g_effect->aptus) );
		break;
	default:
		return false;
	}
	return false;
}

Amor *Induco::clone() 
{
	Induco *child = 0;
	child = new Induco();
	Aptus::inherit( (Aptus*)child );
	if ( !isCause ) 
	{
		child->canAccessed = false;
		goto End;
	}
#define Inherit(x) child->x = x;

	Inherit(isCause);
	Inherit(save_end);

	child->hup(end);
End:
	return  (Amor*)child;
}

void Induco::hup(Animus *fa)
{
	if ( !isCause || !fa ) return ;	/* for the effect node, it's NULL when clone() */
	
	end = (Animus *) fa->clone();
	if (!save_end)
		end = 0;
}

Induco::Induco()
{
	isCause = true;
	save_end = true;
	end = (Animus *) 0;
}

Induco::~Induco() 
{
	WBUG("this %p deleted!\n", this);
	if (end) 
	{
		end->refer_count--;
		if ( end->refer_count == 0 )
			delete end;
		end = 0;
	}
}

#define TEXTUS_APTUS_TAG {'I', 'n', 'd', 'u', 'c', 'e', 0};
#include "hook.c"
