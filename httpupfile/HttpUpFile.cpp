/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: HTTP Upload File
 Build: created by octerboy, 2006/04/27, Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
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

class HttpUpFile: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	HttpUpFile();
	~HttpUpFile();

private:
	int data_type;
	const char* content_type;	
	
	struct subparameter {
		char *name;
		int  nameLen;
		char *value;
		size_t valueLen;
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
	
	Amor::Pius local_pius;	//仅用于向左边传回数据
	
	TBuffer *rcv_buf;	/* 在http头完成后，这将是http体的内容 */
	TBuffer *snd_buf;

	 void reset();
	 short parse_multipart_form(const char* ,char *, TEXTUS_LONG);
	 void deliver(Notitia::HERE_ORDO aordo);
#include "httpsrv_obj.h"
#include "wlog.h"
};

#include <assert.h>
#include "casecmp.h"
#define METHOD_OTHER    0
#define METHOD_GET      1
#define METHOD_POST     2

#define HTTP_NONE 0
#define HTTP_FILE  1
#define HTTP_UNKNOWN  2

void HttpUpFile::ignite(TiXmlElement *cfg) { }

bool HttpUpFile::facio( Amor::Pius *pius)
{
	TBuffer **tb = 0;
	TEXTUS_LONG content_length;
	switch ( pius->ordo )
	{
	case Notitia::PRO_HTTP_REQUEST:	/* 等待整个文件内容已经上传完毕 */
		WBUG("facio PRO_HTTP_REQUEST");
		reset();
		content_type = getHead("Content-Type");	
		content_length = getContentSize();
		if ( content_type &&  getHeadInt("Method") == 2 /* POST method */
				&& ( strncasecmp( content_type , "multipart/form-data;", 20 ) == 0  
				|| strcasecmp( content_type , "multipart/form-data") == 0 ) ) 
		{
			data_type = HTTP_FILE;
			if (parse_multipart_form(content_type, (char*)rcv_buf->base, content_length) != 0)
				data_type = HTTP_NONE;
		} else 
			data_type = HTTP_UNKNOWN;

		deliver(Notitia::PRO_HTTP_REQUEST); /* HTTP请求数据已经OK */
		break;

	case Notitia::SET_TBUF:	/* 取得输入TBuffer地址 */
		WBUG("facio SET_TBUF");
		if ( (tb = (TBuffer **)(pius->indic)))
		{	//tb应当不为NULL，*tb是rcv_buf
			if ( *tb) rcv_buf = *tb; 
			else
				WLOG(WARNING, "facio SET_TBUF rcv_buf null");
			tb++;
			if ( *tb) snd_buf = *tb;
			else
				WLOG(WARNING, "facio SET_TBUF snd_buf null");
			deliver(Notitia::SET_TINY_XML); /* 设置XML DOC数据 */
		} else 
			WLOG(WARNING, "facio SET_TBUF null");
		break;

	case Notitia::DMD_END_SESSION:	/* 强制关闭 */
		WBUG("facio DMD_END_SESSION");
		reset();
		break;

	default:
		return false;
	}

	return true;
}

bool HttpUpFile::sponte( Amor::Pius *pius)
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
		case GetRequestCmd::GetFileLenType :
			req_cmd->filename = (char*) 0;
			req_cmd->type = (char*) 0;
		case GetRequestCmd::GetFileLen:
			req_cmd->len = 0;
		case GetRequestCmd::GetFile:
			req_cmd->valStr = (char*) 0;
			if ( data_type == HTTP_FILE )
			for ( int i = 0; i < form_some.para_num ; i +=2 ) 
			{
				if ( form_some.subpara[i].name 
					&& strcmp(form_some.subpara[i].name,req_cmd->name) ==0 )
				{
					req_cmd->valStr = form_some.subpara[i].value;
					if ( req_cmd->fun ==  GetRequestCmd::GetFileLenType ) 
					{
						req_cmd->filename = form_some.subpara[i+1].name;
						req_cmd->type = form_some.subpara[i+1].value;
						req_cmd->len = form_some.subpara[i].valueLen;
					}

					if ( req_cmd->fun == GetRequestCmd::GetFileLen) 
						req_cmd->len = form_some.subpara[i].valueLen;
					break;
				}
			}
			WBUG("sponte CMD_HTTP_GET GetFileLen(\"%s\")", req_cmd->name);
			break;

		default:
			aptus->sponte(pius);	/* turn to HttpSrvHead for unsupported */
			break;
		}
		break;

	case Notitia::DMD_END_SESSION:	/* forced to end */
		reset();
		break;
		
	default:
		return false;
	}
	return true;
}

HttpUpFile::HttpUpFile()
{
	local_pius.ordo = Notitia::PRO_HTTP_RESPONSE;
	local_pius.indic = 0;
	data_type = HTTP_NONE;
	content_type = (char*) 0;
}

void HttpUpFile::reset()
{
	if(form_some.subpara ) 
		delete[] form_some.subpara;
	if(form_some.data)
		delete[] form_some.data;
	form_some.subpara = NULL;
	form_some.data = NULL;
	form_some.para_num = 0;
}

HttpUpFile::~HttpUpFile() { }

Amor* HttpUpFile::clone()
{
	HttpUpFile *child = new HttpUpFile();
	return (Amor*)child;
}

/* 向接力者提交 */
 void HttpUpFile::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	
	aptus->facio(&tmp_pius);
	return ;
}

/* 将上传的文件信息存入结构para中，包括一些非文件信息也在其中.
   对于非文件信息，包括name内容(变量名),变量内容；
   对于文件信息，有name，filename，content-type,文件内容.
   在整个结构中，连续的子结构依次存放(name,文件内容/变量内容),(filename，content-type),对于非文件信息，仅随的下一个子结构的成员变量是空的。

   所以子结构数为2的倍数。
*/
/* 输入: type, buf,len
   输出: *para 
   返回: -1: 失败, 0:成功
*/
short HttpUpFile::parse_multipart_form(const char* type,char *buf, TEXTUS_LONG len)
{
	PARA *para=&form_some;	
	char *boundary;
	char *scan, *mybuf ;
	int bdlen;
	int idx = 0;

	SUBPARA *spara = NULL;
	SUBPARA *tpara = NULL;
	SUBPARA *tmpbuf= NULL;
	int hasStart = 0;	/* 文件已经开始标志 */

	int rnMode = 0;	/* only \r or \n */
	char c ;
	char _type[256];

	WBUG("Start process multipart_formdata");

	if ( len <=0 ) return -1;
	para->data = new char[len];
	assert(para->data);
	mybuf = scan = para->data;
	memcpy ( para->data,buf,len);
	para->para_num = 0;
	para->subpara = NULL;

	TEXTUS_STRNCPY(_type, type, 255);
	_type[255] = '\0';
	boundary = &_type[20];
	boundary +=  strspn( boundary, " \t" );
	if ( strncasecmp ( boundary,"boundary=",9) != 0 ) return -1;
	boundary = &boundary[7];
	boundary[0] = '-';
	boundary[1] = '-';
	bdlen = static_cast<int>(strlen(boundary));

	WBUG("multipar/form-data boundary is %s",boundary);

	idx = 0;
	while ( idx < len ) 
	{
		scan = &mybuf[idx];
		if ( (len-idx >= bdlen && strncmp ( scan,boundary,bdlen) == 0) || hasStart ) 
		{
			if ( hasStart )
				goto NewFileStart;
			else
				hasStart = 1;
			c = scan[bdlen];
			if (  c == '\n' || c == '\r' ) 
			{
				idx += bdlen;	/* 指针后移 */
				if ( c == '\r' && len > idx && scan[bdlen+1] == '\n' ) 
				{
					rnMode = 1;  /* \r and \n */
					idx +=2 ;
					scan += bdlen+2;
				} else {
					rnMode = 0; /* ony \r or \n  */	
					idx +=1;
					scan += bdlen+1;
				}
	NewFileStart:
				/* 新的文件内容开始 */
				WBUG("New file start ...");
				if( para->subpara == NULL ) 
				{
					para->subpara = new SUBPARA [2];
					assert(para->subpara);
					para->para_num = 2;
				} else {
					para->para_num += 2;
					tmpbuf = new SUBPARA [para->para_num];	
					memcpy(tmpbuf, para->subpara, sizeof(SUBPARA)*(para->para_num-2));
					delete[] para->subpara;
					para->subpara=tmpbuf;
					 
					tmpbuf=NULL;
					 
					assert(para->subpara);
				}
				spara = &(para->subpara[para->para_num-2]);
				tpara = &(para->subpara[para->para_num-1]);
				spara->name = (char*)NULL;
				spara->nameLen = 0;
				spara->value = (char*)NULL;
				spara->valueLen = 0;
				tpara->name = (char*)NULL;
				tpara->nameLen = 0;
				tpara->value = (char*)NULL;
				tpara->valueLen = 0;
				
				/* 扫描 \r \n或两个 \r \n 表示文件描述结束 */ 
	LoopFileHead:
				while ( idx < len ) 
				{
					c = mybuf [idx];
					if ( c == '\n' || c == '\r' ) 
					{
						mybuf [idx] = '\0';
						idx++;
						if ( c == '\r' && len > idx && mybuf[idx] == '\n' ) 
						{
							mybuf[idx]= '\0';
							idx++;
							
						}
						break;
					}
					idx++;
				}
				/* scan 已是一个描述内容,下面分析scan内容 */
				if ( strncasecmp (scan,"Content-Disposition:",20) == 0 ) 
				{
					while(*scan != '\0') 
					{
						if ( strncasecmp(scan,"name=",5)== 0 )
						{
							spara->name = &scan[5];
							scan+=6;
							for(;*scan != spara->name[0];scan++);
							*scan++ = '\0';
							spara->name++;
							spara->nameLen = static_cast<int>(strlen(spara->name));
						}
						if ( strncasecmp(scan,"filename=",9)== 0 )
						{
							tpara->name = &scan[9];
							scan+=10;
							for(;*scan != tpara->name[0];scan++);
							*scan++ = '\0';
							tpara->name++;
							tpara->nameLen = static_cast<int>(strlen(tpara->name));
						}
						scan++;
					}	
				}
				if ( strncasecmp (scan,"Content-Type:",13) == 0 ) 
				{
					
					tpara->value = &scan[13];
					tpara->value += strspn(tpara->value," \t");
					tpara->valueLen = strlen(tpara->value);
				}
				/* scan分析结束 */
				scan = &mybuf[idx];
				/* 看idx后面的字符 */
				/* 如果是\r \n表示文件描述结束, 否则再做loop */
				c = *scan;
				if (  c == '\n' || c == '\r' ) 
				{
					idx++;
					if ( c == '\r' && len > idx && scan[1] == '\n' ) 
					{
						idx++;
					}
				} else 
					goto LoopFileHead;
				/* 文件头分析结束 */

				WBUG("\t--name is %s\t--filename is %s\t--type is %s", spara->name,tpara->name,tpara->value);
				/* 寻找文件内容的分界 */
				scan = &mybuf[idx];
				spara->value = scan;
				spara->valueLen = 0;
				while ( idx < len ) 
				{
					scan = &mybuf[idx];
					if ( len-idx >= bdlen && memcmp ( scan,boundary,bdlen) == 0 ) 
					{
						spara->valueLen -=(rnMode + 1);
						spara->value [spara->valueLen] ='\0';
						idx += bdlen;
						break;
					} else { 
						spara->valueLen++;
						idx++;
					}
				}
				
				WBUG("\t--file content len is " TLONG_FMT, spara->valueLen);
#ifndef NDEBUG
				if( spara->valueLen < 70) 
					WBUG("\t--file content is \n=====file begin====\n%s\n=====file end======",spara->value);
#endif
				c = mybuf [idx];
				if ( c == '\n' || c == '\r' ) 
				{
					mybuf [idx] = '\0';
					idx++;
					if ( c == '\r' && len > idx && mybuf[idx] == '\n' ) 
					{
						mybuf[idx]= '\0';
						idx++;
					}
					/* 文件结束,下一文件又开始 */
				} else {
					/* 整个上传内容结束 */
					break;
				}
			} else {
				idx++;
				continue;
			}
		} else { 
			idx++;
			continue;
		}
	}
	
	WBUG("End of processing multipart_formdata");
	return 0;
}
#include "hook.c"
