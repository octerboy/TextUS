/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@gmail.com)
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
 $Header: /textus/httpsrvhead/DeHead.h 21    13-05-21 8:59 Octerboy $
 $Date$
 $Revision$
*/

/* $NoKeywords: $ */

#ifndef HTTPANSDAT_H
#define HTTPANSDAT_H
#define HEAD_MAX_SIZE	4096
#include <string.h>
#include <time.h>
#include "textus_string.h"

class HttpAnsDat
{
public:
	/* HTTP头内容 */
	char query[256];
	char path[256];
	char  protocol[16];

	int status;
	char title[256]; 	/* 响应时有这内容 */

	long content_length;
	char* content_type;

	typedef struct Field_Value {
		char *name;
		char _name[64];
		char type;
		char *str;
		char _str[256];
		long val;
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
		};

		inline void setn(const char *n)
		{
			TEXTUS_STRNCPY(_name, n, 63);
			name = _name;
		};

		inline void setv(const char *v)
		{
			TEXTUS_STRNCPY(_str, v, 255);
			str = _str;
			type = 0;	/* 字符串类型 */
		};

		inline void setv(int v)
		{
			val = v;
			type = 1;	/* 整型 */
		};

		inline void setTime(time_t v)
		{
			when = v;
			type = 2;	/* 这是时间值 */
		};
	} FIELD_VALUE;
	FIELD_VALUE *field_values;
	int field_num;
	
	char* get_line();
	bool parse();
	int getContent(char *out_buf, long out_buf_size);
	void reset();	/* 复位，恢复到初始状态 */
	void expand();
	void get_method(const char *);
};
#endif
