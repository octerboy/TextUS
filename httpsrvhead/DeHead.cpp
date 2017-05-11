/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
/**
 Title: Analyze  Http Head 
 Build: created by octerboy 2006/03/06, Shenyang
 $Header: /textus/httpsrvhead/DeHead.cpp 34    13-05-21 8:51 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: DeHead.cpp $"
#define TEXTUS_MODTIME  "$Date: 13-05-21 8:51 $"
#define TEXTUS_BUILDNO  "$Revision: 34 $"
#include "version_1.c"
/* $NoKeywords: $ */

#include "casecmp.h"
#include "DeHead.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <sys/timeb.h>

#define METHOD_NUM 28
static const char*method_string[] = { "CONNECT", "OPTIONS", "GET", "HEAD", "POST", "PUT", "DELETE", "TRACE", "PROPFIND", "PROPPATCH", "MKCOL", 
"COPY", "MOVE", "LOCK", "UNLOCK", "ACL", "REPORT", "VERSION-CONTROL", "CHECKIN", "CHECKOUT", "UNCHECKOUT", "SEARCH", "MKWORKSPACE", 
"UPDATE", "LABEL", "MERGE", "BASELINE_CONTROL", "MKACTIVITY" };

#include "tdate_parse.c"

#define Obtainc(s)   (s >= 'A' && s <='F' ? s-'A'+10 :(s >= 'a' && s <='f' ? s-'a'+10 : s-'0' ) )

static void convert( char* out, char* in, int len)
{
	for (int i =0 ; *in && i < len; in++, out++, i++)
	{
		if (*in == '%' &&  isxdigit(in[1]) && isxdigit(in[2]) )
		{
			*out = ((Obtainc( in[1] ) << 4) & 0xf0) | Obtainc( in[2] );
			in += 2;
			i +=2;
		} else 
			*out = (*in == '+' ? ' ' : *in);
       }
       *out = '\0';
}

typedef struct
{
	int len;
	int type; 	/* 数据类型, 0:字符串, 1:整型, 2:时间 */
	char name[32];	/* 域名称*/
}HeadField;

#define FIELD_NUM  8
static HeadField head_names[FIELD_NUM];
static HeadField head_fields[FIELD_NUM]={
			{15,1,"Content-Length:"},
			{ 5,2,"Date:"},
			{ 8,2,"Expires:"},
			{18,2,"If-Modified-Since:"},
			{20,2,"If-Unmodified-Since:"},
			{14,2,"Last-Modified:"},
			{18,0,"Transfer-Encoding:"},
			{13,0,"Content-Type:"}
	};

DeHead::DeHead(enum TYPE _type)
{
	type = _type;
	field_num = 16;
	field_values = new FIELD_VALUE[field_num];	
	ext_method = (char ***)0;
	for ( int i = 0; i < FIELD_NUM; i++ )
	{
		head_names[i].len = head_fields[i].len - 1;
		head_names[i].type = head_fields[i].type;
		TEXTUS_STRCPY(head_names[i].name, head_fields[i].name);
		head_names[i].name[head_names[i].len] = '\0';
	}		
	buff_size = HEAD_MAX_SIZE;
	limit = &buff[buff_size-1];		/* 最大位置 */

	reset();
}

void DeHead::reset()
{
	state = HeadNothing;		/* HTTP头尚未开始 */
	memset(buff, 0, buff_size);
	valid = point = &buff[0];		/* 内部缓冲区分析起始位置、有效位置 */

    	method = (char*)0;
	method_type = NONE;

    	path =(char *)0;
	_path[2047] = '\0';

	query = (char *)0;
	_query[0] = 0;
	_query[2047] = 0;

    	protocol = (char *)0;
	_protocol[0] = 0;
	_protocol[15] = 0;

	status = 0;

	title = (char *)0;
	_title[0] = 0;
	_title[255] = 0;

	content_type = (char*) 0;
	content_length = -99;

	for ( int i = 0; i < field_num; i++ )
		field_values[i].reset();
}

DeHead::~DeHead()
{
	reset();
	delete []field_values;
}

/* 
data: 输入数据指针
len:  输入 数据长度
返回：实际接收的数据长度, < 0表示发生错误
*/
long DeHead::feed(char *data, long len)
{
	char *start_pos, *end_tail4 = 0, *end_tail2 =0;
	
	if ( state == HeadOK ||  len <= 0) 
	{	/* 已经分析完毕，不再接受数据 */
		return 0;
	}
	
	/* HTTP头未接收完成, 将数据放到buff缓冲之中 */
	if ( valid + len >= limit ) 
		len = limit - valid -1;	/* 超出部分不会被接收 */
		
	start_pos = valid;		/* buff中这一次的开始位置 */
	memcpy(valid, data, len);
	valid +=len;
	
	if ( (end_tail4 = strstr( buff, "\r\n\r\n" ) ) == (char*) 0  
		&& (end_tail2 = strstr( buff, "\n\n") ) == (char*) 0 )
	{	/* 还未接收到完整的HTTP头 */
		if (  valid - &buff[0] >=  buff_size -2 )
			reset();	/* 缓冲已满，还未接收完整，放弃*/
		else 
			state = Heading; /* 部分数据接收成功 */
		return len;
	}

	/* HTTP头已经完整, end_tail4(2) +4(2) - data为实际接收长度len,其余不接收 */
	state = HeadOK;
	if ( end_tail4 ) 
		len = end_tail4 +4 - start_pos;
		
	if ( end_tail2 ) 
		len = end_tail2 +2 - start_pos;

	if (!parse()) /* 分析出错，报文头非法 */
		return len;

	valid = point;	/* 有效数据点取最后分析点 */
		
	return len;
}

char* DeHead::get_line()
{
    	char c, *q = point;
    	while(point <= valid)
	{
		c = *point;
		if ( c == '\n' || c == '\r' )
	    	{	
			*point++ = '\0';
	    		if ( c == '\r' && point <= valid && *point == '\n' )
				*point++ = '\0';
	    		return q;
		}
		point++;
	}
    	return (char *)0;
}

bool DeHead::parse()
{
	bool chuncked;
	int field_pos;
	char* method_str=(char*)0, *status_str, *line, *cp;

    	status = 200;	/* 先假定分析中未发现错误 */
	chuncked = false;	/* 假定由Content-Length确定消息体的长度 */
	if ( type == REQUEST ) 
	{
		method_str = get_line();
    		path = strpbrk( method_str, " \t" );
    		if ( path == (char*) 0 )
		{	
			status=400;
			return false;
		}	
    		*path++ = '\0';
    		path += strspn( path, " \t" );
    		protocol = strpbrk( path, " \t" );
    		if ( protocol == (char*) 0 )
		{	
			status=400;
			return false;
		}	
    		*protocol++ = '\0';
    		query = strchr( path, '?' );
    		if ( query ) 
    			*query++ = '\0';

	} else { /* RESPONSE */
		protocol = get_line();
    		status_str = strpbrk( protocol, " \t" );
    		if ( status_str == (char*) 0 )
		{	
			status= -1;
			return false;
		}	
    		*status_str++ = '\0';
    		status_str+= strspn( status_str, " \t" );
		title = strpbrk(status_str, " \t");
    		if ( title == (char*) 0 )
		{	
			status= -1;
			return false;
		}	
		*title++ = '\0';
		status = atoi(status_str);
	}
	
	field_pos = 0;
	/* Parse the rest of the buff headers. */
	while ( ( line = get_line() ) != (char*) 0 )
	{
		int i;
		if ( line[0] == '\0' ) break;

		if ( field_pos == field_num ) /* 空间不够 */
			expand();

		for ( i = 0; i < FIELD_NUM; i++ )
	    	if ( strncasecmp( line, head_fields[i].name, head_fields[i].len ) == 0 )
		{
	    		cp = &line[head_fields[i].len];
	    		cp += strspn( cp, " \t" );
			field_values[field_pos].name = head_names[i].name;
			field_values[field_pos].type = head_fields[i].type;
	    		switch (head_fields[i].type)
	    		{
	    		case 0:
	    			field_values[field_pos].str = cp;
	    			break;
	    		case 1:
	    			field_values[field_pos].val = atol( cp );
	    			break;
	    		case 2:
				field_values[field_pos].when = tdate_parse( cp );
	    			break;
	    		default:
	    			break;
	    		}

			switch( i ) 
			{
			case 0:
				content_length = field_values[field_pos].val ;
				break;
			case 6: /* Transfer-Encoding(6是其数组下标)头存在, 消息体长度不能在这里确定 */
				chuncked = true;
				break;
			case 7:
				content_type = field_values[field_pos].str ;
				break;
			default:
				break;
			}
	    		break;
		}
		if ( i == FIELD_NUM ) /* 不在预定义范围 */
		{
			cp = strpbrk(line, ":");
			if ( !cp )  continue;
    			*cp++ = '\0';
    			cp+= strspn( cp, " \t" );
	    		field_values[field_pos].type = 0; 	/* 字符串型 */
	    		field_values[field_pos].str = cp;
	    		field_values[field_pos].name = line;
		}
		field_pos++;
	}
	/* 所有的head域都分析完了 */
	if ( chuncked )	/* content-length域置-1, 以表示本域不存在, 消息体长度无法在此确定 */
	    	content_length = -1;
		
	if ( type ==RESPONSE )
		return true;

	get_method(method_str);
	if ( method_type == NONE )	/* 不支持的method */
	{	status=501;
		return false;
	}	

	convert( _path, path, 255 );
	return true;
}

long DeHead::getHeadInt(const char* name)
{
	int i;
	if (strcasecmp(name, "Method") ==0 )
		return method_type;
	if (strcasecmp(name, "Content-Length") ==0 )
		return content_length;
	else {
		for ( i = 0; i < field_num; i++ )
		if ( !field_values[i].name )
			break;
		else if ( strcasecmp( name, field_values[i].name) == 0 )
		{
			if ( field_values[i].type == 1 )
				return field_values[i].val;
			else if ( field_values[i].type == 2 )
				return (int)field_values[i].when;
			else
				return atol(field_values[i].str);
			break;
		}		
	}
	return 0; 			  	
}

const char* DeHead::getHead(const char* name)
{
	int i;
	if (strcasecmp(name, "Content-Type") ==0 )
		return content_type;
	else if (strcasecmp(name, "Method") ==0 )
		return method;
	else if (strcasecmp(name, "Protocol") ==0 )
		return protocol;		
	else if (strcasecmp(name, "Query") ==0 )
		return query;		
	else if (strcasecmp(name,"Path")==0) 
		return &_path[0];
	else if (strcasecmp(name, "Title") ==0 )
		return title;
	else 
	{ 
		for ( i = 0; i < field_num; i++ )
		if ( !field_values[i].name )
			break;
		else if ( strcasecmp( name, field_values[i].name) == 0 )
		{
			if ( field_values[i].type == 0 )
				return field_values[i].str;
	    		break;
	    	}
	}
	return (char*)0; 			  	
}

const char** DeHead::getHeadArray(const char* name)
{
	int i,j;
	char *p, *q;
	for ( i = 0; i < field_num; i++ )
	{
		if ( !field_values[i].name )	//已经到了最后了
		{
			break;
		} else if ( strcasecmp( name, field_values[i].name) == 0 )
		{
			if ( field_values[i].type == 0 )
			{
				if ( field_values[i].str_array[0] != 0 )
					return field_values[i].str_array;
				else {	//进行解析
					p = field_values[i].str; //p开头不会有空格之类的空白字符, 开始解析时已经处理了.
					j = 0;
					while(*p)	
					{
						field_values[i].str_array[j] = p;
						q= strpbrk(p, ",");
						if ( q )
						{ 
    							*q++ = '\0';
	    						q+= strspn( q, " \t" );
						}
						p = q;
						j++;
						if ( !p || j == sizeof(field_values[i].str_array)/sizeof(char*)-1 ) break;
					}
					field_values[i].str_array[j] = 0;
					return field_values[i].str_array;
				}
			}
			break;
		}
	}
	return (const char**)0; 			  	
}

void DeHead::setHead(const char* name, const char* value)
{
	if( !value ) return;

	if (strcasecmp(name,"Title")==0)
	{
		TEXTUS_STRNCPY(_title, value, sizeof(_title) );
		title = _title;

	} else if (strcasecmp(name,"Query")==0)
	{
		TEXTUS_STRNCPY(_query, value, 2047);
		query = _query;

	} else if (strcasecmp(name,"Protocol")==0)
	{
		TEXTUS_STRNCPY(_protocol, value, 255);
		protocol = _protocol;

	} else if (strcasecmp(name,"Path")==0)
	{
		TEXTUS_STRNCPY(_path, value, 2047);
		path = _path;

	} else if (strcasecmp(name,"Method")==0)
	{
		get_method(value);
	} else 
		setField(name, value, 0, 0, 0);
}

void DeHead::setField(const char* name, const char* strv, long lv, time_t tv, char which )
{
	Field_Value *fld = (Field_Value *)0;
	int i = 0;

	for ( i = 0, fld=&field_values[0]; i < field_num ; i++, fld++) 
	if ( fld->name == (char*) 0 || strcasecmp( name, fld->name) == 0 ) 
		break; 		

	if ( i == field_num )
	{	/* 这属于空间不够 */
		expand();
		fld = &field_values[i];	
		fld->setn(name);	
	} else if ( i < field_num && fld->name == (char*) 0 )
	{	/* 这属于新域 */
		fld->setn(name);
	}						

	/* 到这里, fld就确定了 */
	switch (which)
	{
	case 0:
		fld->setv(strv);	
		break;
	case 1:
		fld->setv(lv);	
		break;
	case 2:
		fld->setTime(tv);	
		break;
	default:
		break;
	}

	if (strcasecmp(name, "Content-Type") ==0 )
		content_type = fld->str;	/* 将指针指向数组内部 */

	return ; 			  			
}

void DeHead::setHead(const char* name, long value)
{
	if (strcasecmp(name,"Status")==0)
		setStatus(value);
	else
		setField(name, 0, value, 0, 1);

	if (strcasecmp(name,"Content-Length")==0) 
		content_length = value;

	return ; 			  			
}

void DeHead::setHeadTime(const char* name, time_t value)
{
	setField(name, 0, 0, value, 2);
	return ; 			  			
}

void DeHead::addField(const char* name, const char* strv, long lv, time_t tv, char which)
{
	Field_Value *fld;
	int i;
	for ( i = 0, fld=&field_values[0];  i < field_num && fld->name; i++, fld++); 

	if ( i == field_num )
	{	/* 这属于空间不够 */
		expand();
		fld = &field_values[i];
	}

	fld->setn(name);
	switch (which)
	{
	case 0:
		fld->setv(strv);	
		break;
	case 1:
		fld->setv(lv);	
		break;
	case 2:
		fld->setTime(tv);	
		break;
	default:
		break;
	}
}

void DeHead::addHead(const char* name, const char* value)
{
	addField(name, value, 0, 0, 0);
}

void DeHead::addHead(const char* name, long value)
{
	addField(name, 0, value, 0, 1);
}

void DeHead::addHeadTime(const char* name, time_t value)
{
	addField(name, 0, 0, value, 2);
}

void DeHead::setStatus (int sc)
{
	status=sc;
	title = _title;
    	switch (status)
    	{
    	case 200:   
		TEXTUS_STRCPY(title,"OK");
		break;
    	case 201:   
		TEXTUS_STRCPY(title,"Created");
		break;
    	case 202:   
		TEXTUS_STRCPY(title,"Accepted");
		break;
    	case 203:   
		TEXTUS_STRCPY(title,"Non-Authoritative Information");
		break;
    	case 204:   
		TEXTUS_STRCPY(title,"No Content");
		break;
    	case 205:   
		TEXTUS_STRCPY(title,"Reset Content");
		break;
    	case 206:   
		TEXTUS_STRCPY(title,"Partial Content");
		break;
    	case 300: 
		TEXTUS_STRCPY(title,"Multiple Choices");
		break;
    	case 301: 
		TEXTUS_STRCPY(title,"Moved Permanently");
		break;
    	case 302: 
		TEXTUS_STRCPY(title,"Found");
		break;
    	case 303: 
		TEXTUS_STRCPY(title,"See Other");
		break;
    	case 304:   
		TEXTUS_STRCPY(title,"Not Modified");
        	break;
    	case 305: 
		TEXTUS_STRCPY(title,"Use Proxy");
		break;
    	case 306: 
		TEXTUS_STRCPY(title,"Unused");
		break;
    	case 307:
		TEXTUS_STRCPY(title,"Temporary Redirect");
		break;
    	case 400:   
		TEXTUS_STRCPY(title,"Bad Request");
        	break;
    	case 401:
		TEXTUS_STRCPY(title,"Unauthorized");
		break;
    	case 402:     
		TEXTUS_STRCPY(title,"Payment Required");
   	 	break;
    	case 403:      
		TEXTUS_STRCPY(title,"Forbidden");
    	  	break;	
    	case 404: 
		TEXTUS_STRCPY(title,"Not Found");
    		break;
    	case 405:
		TEXTUS_STRCPY(title,"Method Not Allowed");
    		break;
    	case 406:
		TEXTUS_STRCPY(title,"Not Acceptable");
    		break;
    	case 407:
		TEXTUS_STRCPY(title,"Proxy Error");
    		break;
    	case 408:			 	 
		TEXTUS_STRCPY(title,"time-out");
        	break;
    	case 409:
		TEXTUS_STRCPY(title,"Confilct");
    		break;
    	case 410:
		TEXTUS_STRCPY(title,"Gone");
    		break;
    	case 411:
		TEXTUS_STRCPY(title,"Length Required");
    		break;
    	case 412:
		TEXTUS_STRCPY(title,"Precondition Failed");
    		break;
    	case 413:
		TEXTUS_STRCPY(title,"Request Entity Too Large");
    		break;
    	case 414:
		TEXTUS_STRCPY(title,"Request-URI Too Large");
    		break;
    	case 415:
		TEXTUS_STRCPY(title,"Unsupported Media type");
    		break;
    	case 500:
		TEXTUS_STRCPY(title,"Internal Server Error");
    		break;
    	case 501:
		TEXTUS_STRCPY(title,"Not Implemented");
    		break;
    	case 502:
		TEXTUS_STRCPY(title,"Bad Gateway");
    		break;
    	case 503:
		TEXTUS_STRCPY(title,"Service Unavailable");
    		break;
    	case 504:
		TEXTUS_STRCPY(title,"Gateway Time-out");
    		break;
    	case 505:
		TEXTUS_STRCPY(title,"HTTP Version not supported");
    		break;
    	default:
		TEXTUS_STRCPY(title,"No found ErrID");
	}
}

void DeHead::get_method(const char *method_str)
{
	int i;
	method_type = NONE;	
	method = (char *)0;
	if ( strcasecmp( method_str, method_string[GET] ) == 0 )
	{
		method_type = GET;
		method = method_string[GET];
	} else if ( strcasecmp( method_str, method_string[POST] ) == 0 )
	{
		method_type = POST;
		method = method_string[POST];
	} else {
		for ( i = 0 ; i < METHOD_NUM; i++)
		{
    			if ( strcasecmp( method_str, method_string[i] ) == 0 )
			{
				method_type = (METHOD_TYPE)i;
				method = method_string[i];
				break;
			}
		}
	
		if ( i == METHOD_NUM &&  ext_method && *ext_method )
		{
			char **p = *ext_method;
			while (*p)
			{
				if ( strcasecmp( method_str, *p) == 0 )
				{
					method = *p;
					method_type = EXTENSION;
					break;
				}
				p++;
			}
		}
	}
}
	
void DeHead::expand()
{
	int old = field_num;
	field_num *=2 ;
	FIELD_VALUE *p = field_values;
	field_values = new FIELD_VALUE [field_num];
	memset( field_values, 0, sizeof(FIELD_VALUE)*field_num);
	memcpy( field_values, p, sizeof(FIELD_VALUE)*old);
	delete[] p;
}

int DeHead::getContent (char *out_buf, long _size)
{
	Field_Value *fld;
    	char timebuf[100];
    	struct tm *tdatePtr;
    	struct tm tdate;
    	const char* rfc1123_fmt = "%a, %d %b %Y %H:%M:%S GMT";

	unsigned int res_head_len = 0;
	int out_buf_size =  _size > 65536 ?  65536 : _size;
	
	tdatePtr = &tdate;
#define ADD_TO_HEAD(X) \
	res_head_len+= TEXTUS_SNPRINTF(&out_buf[res_head_len], out_buf_size - res_head_len, X)
#define ADD_TO_HEAD3(X,Y,Z) \
	res_head_len+= TEXTUS_SNPRINTF(&out_buf[res_head_len], out_buf_size - res_head_len, X,Y,Z)
#define ADD_TO_HEAD4(W,X,Y,Z) \
	res_head_len+= TEXTUS_SNPRINTF(&out_buf[res_head_len], out_buf_size - res_head_len, W,X,Y,Z)
#define ADD_TO_HEAD6(U,V,W,X,Y,Z) \
	res_head_len+= TEXTUS_SNPRINTF(&out_buf[res_head_len], out_buf_size - res_head_len, U,V,W,X,Y,Z)

	if ( type == RESPONSE )
	{
		ADD_TO_HEAD4("%s %d %s\r\n", 
			protocol == (char*) 0 ? "HTTP/1.1" : protocol, 
			status == 0 ? 200 : status, 
			title == (char*) 0 ? "OK":title );
	} else {
		ADD_TO_HEAD6("%s %s %s%s%s\r\n", 
			method == (char*) 0 ? "GET" : method, 
			path == 0 ? "/": path, 
			protocol == (char*) 0 ? "HTTP/1.1" : protocol, 
			query == (char*) 0 ? "": "?",
			query == (char*) 0 ? "": query);
	}
    	
	fld = &field_values[0];
	for ( int k = 0 ; k < field_num && fld->name ; k++, fld++ )
	{
		switch (fld->type)
		{
		case 0: /* 一般字符串处理 */
			ADD_TO_HEAD3( "%s: %s\r\n", fld->name, fld->str);
			break;
					
		case 1: /* 整型量处理 */
			ADD_TO_HEAD3( "%s: %ld\r\n", fld->name, fld->val);
			break;

		case 2:	/* 时间值 */
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
			gmtime_s(&tdate, &(fld->when) );
#else
			tdatePtr = gmtime( &(fld->when) );
#endif
    			(void) strftime( timebuf, sizeof(timebuf), rfc1123_fmt, tdatePtr );
			ADD_TO_HEAD3("%s: %s\r\n", fld->name, timebuf);
			break;
		default:
			break;				
		}
	}
	
	/* 头的最后结尾 */
	ADD_TO_HEAD ("\r\n");

	return (res_head_len) ;
}
