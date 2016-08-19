/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Dummy
 Build:created by octerboy 2005/04/12
 $Header: /textus/dummy/Dummy.cpp 2     12-04-04 16:54 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: Dummy.cpp $"
#define TEXTUS_MODTIME  "$Date: 12-04-04 16:54 $"
#define TEXTUS_BUILDNO  "$Revision: 2 $"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include <assert.h>
#include <stdarg.h>

class Dummy :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	Dummy();
	~Dummy();

private:
#include "wlog.h"
};

void Dummy::ignite(TiXmlElement *cfg) { }

bool Dummy::facio( Amor::Pius *pius)
{
	assert(pius);
	WBUG("facio Notitia::%lu", pius->ordo);
	return false;
}

bool Dummy::sponte( Amor::Pius *pius) { 
	assert(pius);
	WBUG("sponte Notitia::%lu", pius->ordo);
	return false; 
}

Amor* Dummy::clone()
{
	Dummy *child = new Dummy();
	return  (Amor*)child;
}

Dummy::Dummy() { }

Dummy::~Dummy() { } 
#include "hook.c"

