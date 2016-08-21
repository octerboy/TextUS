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
 $Date: 12-04-04 16:50 $
 $Revision: 19 $
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
	virtual void ignite(TiXmlElement *wood) {};

	/* Amor objects communicate by Pius object */
	struct Pius {
		 TEXTUS_ORDO ordo;	/* the type of inidc */
		 int sub;		/* the sub type of inidc
						比如同为PRO_UNIPAC, 子节点Module有几个dbport, 到不同数据库等, 或其它unipac接口的. 
						此量区分不同的Module. 父节点的Module可以设置不同值, 以便不同的PacketObj传到不同的子Module。 */
		 void *indic;		/* data pointer for any type */
		 inline Pius()
		 {
			sub = 0;
			indic = 0;
			ordo = -1;
		 }
	};

	virtual bool facio( Pius *) = 0 ;
	virtual bool sponte( Pius *) = 0;
	virtual Amor *clone() = 0;
	
	virtual ~Amor() {};
};
#endif
