/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
/**
 Title: HTTP Form 
 Build: created by octerboy, 2006/09/16, Guangzhou
 $Header: /textus/httpform/HttpForm.cpp 10    08-01-10 1:12 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: HttpForm.cpp $"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "casecmp.h"
#include "textus_string.h"

#include <time.h>
#include <stdarg.h>

#define TINLINE inline
class HttpForm: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	HttpForm();

private:
	int data_type;
	int content_length;
	const char* content_type;	

	TBuffer *rcv_buf;
	TBuffer *snd_buf;

	bool dup;
	TBuffer dup_frm;
	
	struct subparameter {
		char *name;
		int  nameLen;
		char *value;
		long  valueLen;
	};
	typedef struct subparameter SUBPARA;

	struct transpara {
		int para_num;	// parameter number
		char *data;
		struct subparameter *subpara;
		inline transpara() {
			subpara = 0;
			data = 0;
			para_num = 0;
		};
	};
	typedef struct transpara PARA;
	
	PARA form_some;
	TINLINE int _parse_para(const char *buf, int len);
	TINLINE void reset();

	/* 取表单内容 */
	TINLINE const char* _getPara(const char* name);
	TINLINE const char* _getPara(const char* name, long *len);
#include "httpsrv_obj.h"
#include "wlog.h"
};

#include <assert.h>

#define HTTP_NONE 0
#define HTTP_FORM 1
#define HTTP_FILE 2
#define HTTP_UNKOWN  3

#define Obtainc(s)   (s >= 'A' && s <='F' ? s-'A'+10 :(s >= 'a' && s <='f' ? s-'a'+10 : s-'0' ) )

static void convert( char* in, char* out)
{
	for ( ; *in; in++, out++)
	{
		if (*in == '%' &&  isxdigit(in[1]) && isxdigit(in[2]) )
		{
			*out = ((Obtainc( in[1] ) << 4) & 0xf0) | Obtainc( in[2] );
			in += 2;
		} else 
			*out = (*in == '+' ? ' ' : *in);
       }
       *out = '\0';
}

void HttpForm::ignite(TiXmlElement *cfg) { 
	const char *dup_str;
	if (cfg && (dup_str = cfg->Attribute("save") )
		&& strcasecmp(dup_str, "yes") == 0 )
	dup = true;
}

bool HttpForm::facio( Amor::Pius *pius)
{
	TBuffer **tb = 0;
	const char *query;
	int q_len, method;
	switch ( pius->ordo )
	{
	case Notitia::PRO_HTTP_REQUEST:
		WBUG("facio PRO_HTTP_REQUEST");
		reset();

		method  = getHeadInt("Method");	
		query = (char*) 0;
		q_len = 0;
		if (  method == 2  || method == 3) /* GET or HEAD method */
		{
			query = getHead("Query");	
			if ( query) q_len = strlen(query);
		} else if ( (content_length=getContentSize()) > 0 && 
			    (content_type= getHead("Content-Type")) )
		{
			if( strcasecmp( content_type, "application/x-www-form-urlencoded")== 0
			|| strncasecmp( content_type , "application/x-www-form-urlencoded;", 34) == 0 )
			{ 
				q_len = content_length ;
				if (dup)
				{
					dup_frm.reset();
					dup_frm.input(rcv_buf->base, content_length);
					query = (char*)dup_frm.base;
				} else
					query = (char*)rcv_buf->base;
			}
		}
		/*  表单内容分析 */
		if ( query && q_len > 0 &&  _parse_para(query, q_len) == 0 )
		{
			data_type = HTTP_FORM;
			aptus->facio(pius);
		}
		else
			data_type = HTTP_NONE;
		break;

	case Notitia::SET_TBUF:
		WBUG("facio SET_TBUF");
		if ( (tb = (TBuffer **)(pius->indic)))
		{
			if ( *tb) rcv_buf = *tb; 
			else
				WLOG(WARNING, "facio SET_TBUF rcv_buf null");
			tb++;
			if ( *tb) snd_buf = *tb;
			else
				WLOG(WARNING, "facio SET_TBUF snd_buf null");
			aptus->facio(pius);
		} else 
			WLOG(WARNING, "facio SET_TBUF null");
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		reset();
		break;

	default:
		return false;
	}

	return true;
}

bool HttpForm::sponte( Amor::Pius *pius) 
{ 
	assert(pius);
	struct GetRequestCmd *req_cmd = 0;
	
	switch ( pius->ordo )
	{
	case Notitia::CMD_HTTP_GET :	/* 取HTTP请求数据 */
		req_cmd = (struct GetRequestCmd *)pius->indic;
		assert(req_cmd);
		switch ( req_cmd->fun)
		{
		case GetRequestCmd::GetPara :
			req_cmd->valStr = _getPara(req_cmd->name);
			WBUG("sponte CMD_HTTP_GET GetPara(\"%s\")=\"%s\"", req_cmd->name,req_cmd->valStr);
			break;
				
		case GetRequestCmd::GetParaLen :
			req_cmd->valStr = _getPara(req_cmd->name, &(req_cmd->len));
			WBUG("sponte CMD_HTTP_GET GetParaLen(\"%s\")=\"%s\"", req_cmd->name, req_cmd->valStr);
			break;

		default:
			aptus->sponte(pius);	/* Not supported.  Httpsrvhead/httpsrvbody handles it */
			break;
		}           
		break;          	
                                
	default:
		return false;
	}
	return true;
}

HttpForm::HttpForm()
{
	rcv_buf = 0;
	snd_buf = 0;
	dup =false;
	reset();
}

Amor* HttpForm::clone()
{
	HttpForm *child;
	child = new HttpForm();
	child->dup = dup;
	return (Amor*) child;
}

TINLINE void HttpForm::reset()
{
	if(form_some.subpara )
		free((void *)form_some.subpara);

	if(form_some.data)
		free((void *)form_some.data);

	form_some.subpara = NULL;
	form_some.data = NULL;
	form_some.para_num = 0;
}

TINLINE const char* HttpForm::_getPara(const char* name)
{
	int i = 0;
	if ( data_type != HTTP_FORM ) 
		return (char*) 0;

	for ( i = 0; i < form_some.para_num ; i ++ ) 
	{
		if ( form_some.subpara[i].name != NULL && strcmp(form_some.subpara[i].name,name) ==0 )
		{
			return form_some.subpara[i].value;
		}
	}

	return (char*)0;
}

TINLINE const char* HttpForm::_getPara(const char* name,long *len)
{
	int i = 0;

	if ( data_type != HTTP_FORM ) 
	{
		return (char*) 0;
	}

	for ( i = 0; i < form_some.para_num ; i ++ ) 
	{
		if ( form_some.subpara[i].name != NULL && strcmp(form_some.subpara[i].name,name) ==0 )
		{
			*len = form_some.subpara[i].valueLen;
			return form_some.subpara[i].value;
		}
	}

	*len = 0 ;
	return (char*)0;
}

/* 分析HTTP请求报文中表单的数据，形成PARA结构 */
/* 输入: buf,len
   输出: *para 
   返回: -1: 失败, 0:成功
*/
int HttpForm::_parse_para(const char *buf, int len)
{
	PARA *para=&form_some;
	int i,count,itemLen,cDataLen;
	char *dPtr,*tmpbuf;
	const char *ptr;
	SUBPARA *spara;

	if(form_some.subpara ) free((void *)form_some.subpara);	//保证多次解析不漏内存
	if(form_some.data) free((void *)form_some.data);
	para->para_num = 0;
	para->subpara = NULL;
	para->data = NULL;

	// caculate the parameter number
	for( count = 0,ptr = buf,i=0; i < len; ptr++, i++)
		if ( *ptr == '=' ) count ++;
	
	WBUG("parse parameter count = %d",count);

	if( count == 0)
		return 0;
	para->para_num = count;

	// Now have count parameter, and alloc parameter to struct
	spara = (SUBPARA *) malloc(sizeof(SUBPARA)*count);
	if(spara == NULL)
	{
		WLOG(EMERG,"%s,%d,malloc %d SUBPARA error",__FILE__,__LINE__,count);
		return -1;
	}

	dPtr =  (char *)malloc( len+4*count);
	if(dPtr == NULL)
	{
		WLOG(EMERG,"%s,%d,malloc %d SUBPARA error",__FILE__,__LINE__,count);
		para->data = NULL;
		return -1;
	}
	
	para->subpara = spara;
	para->data = dPtr;

	// Begin Parse the parameter
	i = 0;itemLen = 0; cDataLen = 0;
	tmpbuf = dPtr;
	for( count = 0; count < len; count++)
	{
		if( buf[count] == '=' ) // name end
		{
			tmpbuf[itemLen] = '\0';
			convert(tmpbuf,tmpbuf);
			itemLen = strlen(tmpbuf);
			spara[i].name = dPtr + cDataLen;
			spara[i].name[itemLen] = '\0';
			spara[i].nameLen = itemLen;
			spara[i].value = (char *)0;
			spara[i].valueLen = 0;
			cDataLen += (itemLen+1);
			itemLen = 0;
			tmpbuf = dPtr+ cDataLen;

		} else if( buf[count] == '&' ) // value end
		{
			tmpbuf[itemLen] = '\0';
			convert(tmpbuf,tmpbuf);
			spara[i].value = dPtr + cDataLen;
			itemLen = strlen(tmpbuf);
			spara[i].value[itemLen] = '\0';
			spara[i].valueLen = itemLen;
			cDataLen += (itemLen+1);
			i++;
			tmpbuf = dPtr+ cDataLen;
			itemLen = 0;
		} else // the value
		{
			tmpbuf[itemLen] = buf[count];
			itemLen++;
		}
	}
			
	// add the last value
	if(itemLen != 0)
	{
		tmpbuf[itemLen] = '\0';
		convert(tmpbuf,tmpbuf);
		itemLen = strlen(tmpbuf);
		spara[i].value = dPtr + cDataLen;
		spara[i].value[itemLen] = '\0';
		spara[i].valueLen = itemLen;
		i++;
		itemLen = 0;
	}
	
	para->para_num = i;	/* 这才是真正的变量数 */
	WBUG("Have %d The Parameter", para->para_num);
#ifndef NDEBUG
	for( i = 0; i < para->para_num; i++)
		WBUG("Name = %s, Value = %s",para->subpara[i].name,para->subpara[i].value);
#endif
	return 0;	
}

#include "hook.c"
