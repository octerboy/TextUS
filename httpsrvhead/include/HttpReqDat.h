/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
/**
 Titile: Http Request Head Data
 Build: created by octerboy 2013/06/27
 $Header: /textus/httpsrvhead/DeHead.h 21    13-05-21 8:59 Octerboy $
 $Date: 13-05-21 8:59 $
 $Revision: 21 $
*/

/* $NoKeywords: $ */

#ifndef HTTPREQDAT_H
#define HTTPREQDAT_H
#define HEAD_MAX_SIZE	4096
#include <string.h>
#include <time.h>
#include "textus_string.h"

struct HttpReqDat
{
	enum { 	HeadOK = 2,
		Heading = 1,
		HeadNothing = 0
	 	/*  0：尚未开始，没有一字节进入本对象 */                    
		/* 1：正在进行，有字节进入本对象，但HTTP头未完全接收*/   
		/* 2： HTTP头已经OK */                                       
	} state;

	char 	buff[HEAD_MAX_SIZE];	/* 内部缓冲区 */
	long	buff_size;	/* 内部缓冲区空间 */

	char*	point; 	/* 分析起始位置 */
	char*	valid;	/* 有效数据末位置 */
	char*	limit;  /* 空间最后位置 */
				
	/* HTTP头内容 */
	char* query;
	char* path;
	const char* method;
	char* protocol;

	int status;
	char *title;

	long content_length;
	char* content_type;

	enum METHOD_TYPE { CONNECT =0, OPTIONS =1, GET = 2, HEAD = 3, POST = 4, PUT=5, DELETTE=6, TRACE =7, PROPFIND=8, PROPPATCH=9, 
			MKCOL=10, COPY=11, MOVE=12, LOCK=13, UNLOCK=14, ACL=15, REPORT=16, VERSION_CONTROL=17, CHECKIN=18, CHECKOUT=19, 
			UNCHECKOUT=20, SEARCH=21, MKWORKSPACE=22, UPDATE=23, LABEL=24, MERGE=25, BASELINE_CONTROL=26, MKACTIVITY=27, 
			EXTENSION = 99, NONE= -1 };
	enum METHOD_TYPE method_type;
	
	char ***ext_method;	/* 扩展方法 */

	typedef struct Field_Value {
		char *name;
		char type;
		char *str;
		long val;
		time_t when;
	} FIELD_VALUE;
	FIELD_VALUE *field_values;
	int field_num;
	
};
#endif
