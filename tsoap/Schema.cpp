/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
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
 $Header: /textus/tsoap/Schema.cpp 5     08-01-01 23:59 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: Schema.cpp $"
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

