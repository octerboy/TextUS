/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
/**
 Title:The public interface of textus module
 Build: Created by octerboy 2005/6/9
 $Date$
 $Revision$
*/

/* $NoKeywords: $ */

#ifndef AMOR_H
#define AMOR_H
#if defined(_WIN32) 
#include <windows.h>
#endif 

#if !defined(TEXTUS_AMOR_STORAGE)
#if defined(_WIN32) 
#define TEXTUS_AMOR_STORAGE __declspec(dllimport) 
#else
#define TEXTUS_AMOR_STORAGE
#endif
#endif

#include "tinyxml.h"

typedef unsigned long TEXTUS_ORDO;

class TEXTUS_AMOR_STORAGE Amor {
public:
	Amor *aptus;
	enum SUB_ORDO { CAN_ALL = -1 };
	virtual void ignite(TiXmlElement *wood) {};

	/* Amor objects communicate by Pius object */
	struct Pius {
		 TEXTUS_ORDO ordo;	/* the type of inidc */
		 int subor;		/* the sub type of inidc, 以此区分要传递的下一级不同的Module. */
						
		 void *indic;		/* data pointer for any type */
		Pius() { subor = CAN_ALL; indic=0;};	
	};
	virtual bool facio( Pius *) = 0 ;
	virtual bool sponte( Pius *) = 0;
	virtual Amor *clone() = 0;
	virtual ~Amor() {};
};
#endif
