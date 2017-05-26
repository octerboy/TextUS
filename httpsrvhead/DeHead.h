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

#ifndef DEHTTPHEAD_H
#define DEHTTPHEAD_H
#define HEAD_MAX_SIZE	8192
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
	 	/*  0����δ��ʼ��û��һ�ֽڽ��뱾���� */                    
		/* 1�����ڽ��У����ֽڽ��뱾���󣬵�HTTPͷδ��ȫ����*/   
		/* 2�� HTTPͷ�Ѿ�OK */                                       
	} state;


	/* HTTP�������ݷ������ */
	long feed(char *data, long len);	/*���� true:δʧ��; false:����ʧ�� */

	/* ȡ��HTTPͷ���� */
	const char* 	getHead(const char* name); 
	const char** 	getHeadArray(const char* name);
	long 	getHeadInt(const char* name); 
	/* ����HTTPͷ���� */
	void 	setHead (const char* name, const char* value);
	void 	setHead (const char* name, long value);
	void 	setHeadTime (const char* name, time_t value);

	void 	setField (const char* name, const char* v1, long v2, time_t v3, char which );

	void 	addField (const char* name, const char* v1, long v2, time_t v3, char which );

	void	addHead (const char* name, const char* value);
	void	addHead (const char* name, long value);
	void	addHeadTime (const char* name, time_t value);

	void 	setStatus (int );

	char 	buff[HEAD_MAX_SIZE];	/* �ڲ������� */
	long	buff_size;	/* �ڲ��������ռ� */

	char*	point; 	/* ������ʼλ�� */
	char*	valid;	/* ��Ч����ĩλ�� */
	char*	limit;  /* �ռ����λ�� */
				
	
	/* HTTPͷ���� */
	char* query;
	char _query[2048];
	char* path;
	char  _path[2048];
	const char* method;
	char* protocol;
	char  _protocol[16];

	int status;
	char *title;
	char _title[256]; 	/* ��Ӧʱ�������� */

	long content_length;
	char* content_type;

	enum METHOD_TYPE { CONNECT = 0, OPTIONS = 1, GET = 2, HEAD = 3, POST=4, PUT=5, MDELETE=6, TRACE=7, PROPFIND=8, PROPPATCH=9, MKCOL=10, COPY=11, MOVE=12, LOCK=13, UNLOCK=14, ACL=15, REPORT=16, VERSION_CONTROL=17, CHECKIN = 18, CHECKOUT = 19, UNCHECKOUT = 20, SEARCH = 21, MKWORKSPACE =22, UPDATE =23, LABEL =24, MERGE =25, BASELINE_CONTROL= 26, MKACTIVITY=27, EXTENSION = 99, NONE= -1 };
	enum METHOD_TYPE method_type;
	
	char ***ext_method;	/* ��չ���� */

	typedef struct Field_Value {
		char *name;
		char _name[128];
		char type;
		char *str;
		const char *str_array[64];	//ĳЩHead�����������, �Զ������. ����ٶ����63����
		char _str[1024];
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
			type = 0;	/* �ַ������� */
		};

		inline void setv(int v)
		{
			val = v;
			type = 1;	/* ���� */
		};

		inline void setTime(time_t v)
		{
			when = v;
			type = 2;	/* ����ʱ��ֵ */
		};
	} FIELD_VALUE;
	FIELD_VALUE *field_values;
	int field_num;
	
	char* get_line();
	bool parse();
	int getContent(char *out_buf, long out_buf_size);
	void reset();	/* ��λ���ָ�����ʼ״̬ */
	void expand();
	void get_method(const char *);
};
#endif
