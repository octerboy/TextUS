/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
/**
 Titile: DeHead Definition
 Build: created by octerboy 2006/02/16
 $Id$
 $Date$
 $Revision$
*/

/* $NoKeywords: $ */

#include "textus_os.h"
#ifndef DEHTTPHEAD_H
#define DEHTTPHEAD_H
#define HEAD_MAX_SIZE	81920
#include <string.h>
#include <time.h>
#include "textus_string.h"

class DeHead
{
public:
	enum TYPE {REQUEST = 0, RESPONSE = 2}  type;
	DeHead( enum TYPE t = REQUEST);
	~DeHead();
	
	enum { 	HeadOK = 2,
		Heading = 1,
		HeadNothing = 0
	 	/*  0：尚未开始，没有一字节进入本对象 */                    
		/* 1：正在进行，有字节进入本对象，但HTTP头未完全接收*/   
		/* 2： HTTP头已经OK */                                       
	} state;


	/* HTTP报文数据分析相关 */
	TEXTUS_LONG feed(char *data, TEXTUS_LONG len);	/*返回 true:未失败; false:解析失败 */

	/* 取得HTTP头内容 */
	const char* 	getHead(const char* name); 
	const char** 	getHeadArray(const char* name);
	TEXTUS_LONG 	getHeadInt(const char* name); 
	unsigned TEXTUS_LONG 	getHeadULong(const char* name); 
	/* 设置HTTP头内容 */
	void 	setHead (const char* name, const char* value);
	void 	setHead (const char* name, TEXTUS_LONG value);
	void 	setHeadTime (const char* name, time_t value);

	void 	setField (const char* name, const char* v1, TEXTUS_LONG v2, time_t v3, char which );

	void 	addField (const char* name, const char* v1, TEXTUS_LONG v2, time_t v3, char which );

	void	addHead (const char* name, const char* value);
	void	addHead (const char* name, TEXTUS_LONG value);
	void	addHeadTime (const char* name, time_t value);

	void 	setStatus (int );

	char 	buff[HEAD_MAX_SIZE];	/* 内部缓冲区 */
	TEXTUS_LONG	buff_size;	/* 内部缓冲区空间 */

	char*	point; 	/* 分析起始位置 */
	char*	valid;	/* 有效数据末位置 */
	char*	limit;  /* 空间最后位置 */
				
	
	/* HTTP头内容 */
	char* query;
	char _query[2048];
	char* path;
	char  _path[2048];
	const char* method;
	char* protocol;
	char  _protocol[16];

	int status;
	char *title;
	char _title[256]; 	/* 响应时有这内容 */

	TEXTUS_LONG content_length;
	char* content_type;

	enum METHOD_TYPE { ACL=0, BASELINE_CONTROL=1, BIND=2, CHECKIN=3, CHECKOUT=4, CONNECT=5, COPY=6, MDELETE=7,/* msvc hate DELETE */ GET=8, HEAD=9, LABEL=10, LINK=11, LOCK=12, 
			MERGE=13, MKACTIVITY=14, MKCALENDAR=15, MKCOL=16, MKREDIRECTREF=17, MKWORKSPACE=18, MOVE=19, OPTIONS=20, ORDERPATCH=21, PATCH=22, 
			POST=23, PRI=24, PROPFIND=25, PROPPATCH=26, PUT=27, REBIND=28, REPORT=29, SEARCH=30, TRACE=31, UNBIND=32, UNCHECKOUT=33, UNLINK=34, 
			UNLOCK=35, UPDATE=36, UPDATEREDIRECTREF=37, VERSION_CONTROL=38, EXTENSION = 99, NONE= -1 };
	enum METHOD_TYPE method_type;
	
	char ***ext_method;	/* 扩展方法 */

	typedef struct Field_Value {
		char *name;
		char _name[128];
		char type;
		char *str;
		const char *str_array[64];	//某些Head包含多个内容, 以逗号相隔. 这里假定最多63个。
		char _str[1024];
		TEXTUS_LONG val;
		unsigned TEXTUS_LONG uval;
		time_t when;
		inline Field_Value()
		{
			reset();
		};

		inline void reset()
		{
			name = 0;
			_name[63] = '\0';
			str = (char *)0;
			_str[255] ='\0';			
			val =0;
			when =0;
			str_array[0] = 0;
		};

		inline void setn(const char *n)
		{
			TEXTUS_STRNCPY(_name, n, 127);
			name = _name;
		};

		inline void setv(const char *v)
		{
			TEXTUS_STRNCPY(_str, v, 1023);
			str = _str;
			type = 0;	/* 字符串类型 */
		};

		inline void setv(TEXTUS_LONG v)
		{
			val = v;
			type = 1;	/* 整型 */
		};

		inline void setTime(time_t v)
		{
			when = v;
			type = 2;	/* 这是时间值 */
		};

		inline void setv(unsigned TEXTUS_LONG v)
		{
			uval = v;
			type = 3;	/* 无符号整型 */
		};

	} FIELD_VALUE;
	FIELD_VALUE *field_values;
	int field_num;
	
	char* get_line();
	bool parse();
	int getContent(char *out_buf, TEXTUS_LONG out_buf_size);
	void reset();	/* 复位，恢复到初始状态 */
	void expand();
	void get_method(const char *);
};
#endif
