/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
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
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "textus_string.h"
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

