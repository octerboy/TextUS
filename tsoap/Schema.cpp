/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
Title: schema parse
Build: created by octerboy, 2006/12/01
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
#include "version_1.c"

#include "Wsdl.h"
Schema::Schema (const char *nm)
{
	int len;
	clear();
	if ( !nm )
		return;
	len = strlen(nm);
	uri = new char[len+1];
	memcpy(uri, nm, len );
	uri[len] = 0;
}


void Schema::parse(TiXmlElement *sma)
{
}

