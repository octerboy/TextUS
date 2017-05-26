/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: HTTP directory of file system
 Build:created by octerboy 2006/04/12
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#if !defined( _WIN32)
#include <dirent.h>
#endif

#define DEFAULT_PAGE "index.htm"
#define HTTP_DIR_DEFAULT_CHARSET "iso-8859-1"
#if defined(_WIN32) 
#define	ISMYDIR(x) (_S_IFDIR & x)
#else
#define	ISMYDIR(x) (S_ISDIR(x))
#endif

#include "Amor.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "casecmp.h"
#include "textus_string.h"
class DirLet :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	DirLet();
	~DirLet();

private:
#include "httpsrv_obj.h"
#include "wlog.h"
	Amor::Pius local_pius;  //仅用于向mary传回数据

	TBuffer *req_body;	/* 请求体 */
	TBuffer *res_entity;	/* 响应体 */

	char home_dir[256];	/* 主目录, 所有文件都在这个目录及其子目录下 */
	char charset[64];	/* 字符集, 比如gb2312等 */
	bool handle(); /* false:未处理请求; true:已处理请求 */
	bool sec_check;	/* if true, the request having '/..' is invalid */
};

#include "textus_string.h"
void DirLet::ignite(TiXmlElement *cfg)
{
	const char *dir_str, *set_str, *comm_str;

	if ( (dir_str = cfg->Attribute("home")))
		TEXTUS_STRNCPY(home_dir, dir_str, sizeof(home_dir)-1);

	if ( (set_str = cfg->Attribute("charset")))
		TEXTUS_STRNCPY(charset, set_str, sizeof(charset)-1);

	if ( (comm_str = cfg->Attribute("security")) && strcasecmp(comm_str, "yes") == 0 )
		sec_check = true;

	if(strlen(home_dir) < 1 ) 
		TEXTUS_STRCPY(home_dir, "/");

	if ( home_dir[0] != '/' ) 
	{
		char tmp[512];
		TEXTUS_STRCPY(tmp,home_dir);
		TEXTUS_STRCPY(&home_dir[1], tmp);
		home_dir[0] = '/';
	}

	if( home_dir[strlen(home_dir)-1] != '/' )
		TEXTUS_STRCAT(home_dir,"/");
}

bool DirLet::facio( Amor::Pius *pius)
{
	TBuffer **tb = 0;
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::PRO_HTTP_HEAD:	/* 有HTTP请求 */
		WBUG("facio PRO_HTTP_HEAD ");
		if ( !req_body || !res_entity )
		{/* 当然输入输出缓冲区得已经准备好 */
			WLOG(ERR,"req_body buf or res_entity buf is null!");
			break;
		}
		(void) handle();
		break;

	case Notitia::SET_TBUF:	/* 第一个是http请求体, 第二个是http响应体 */
		WBUG("facio SET_TBUF");
		if ( (tb = (TBuffer **)(pius->indic)))
		{	//tb应当不为NULL，*tb是rcv_buf
			if ( *tb) req_body = *tb; 
			tb++;
			if ( *tb) res_entity = *tb;
		} else 
		{	
			WLOG(WARNING, "facio SET_TBUF null");
		}
		break;
		
	default:
		return false;
	}
	return true;
}

bool DirLet::sponte( Amor::Pius *pius) { return false; }

DirLet::DirLet()
{
	local_pius.ordo = 0;
	local_pius.indic = 0;

	memset(home_dir, 0, sizeof(home_dir));
	memset(charset, 0, sizeof(charset));
	TEXTUS_STRCPY(home_dir, "/");
	TEXTUS_STRCPY(charset, HTTP_DIR_DEFAULT_CHARSET);
}

Amor* DirLet::clone()
{
	DirLet *child = new DirLet();
	memcpy(child->home_dir, home_dir, sizeof(home_dir));
	memcpy(child->charset, charset, sizeof(charset));
	child->sec_check = sec_check;
	return (Amor*)child;
}

DirLet::~DirLet() { }

bool DirLet::handle()
{
	char path[1024], path2[200];
	char *file, file2[1536], *end_c;
	char t;
	struct stat sb;	/* 文件状态 */
	int contents_size = 0, buflen;

	memset(path2, 0, sizeof(path2));
	TEXTUS_STRNCPY(path2, getHead("Path"), sizeof(path2)-2);
	if ( path2[0] != '/')
	{
		sendError(400);
		local_pius.ordo = Notitia::PRO_HTTP_HEAD ;
		WBUG("path2[0] != '/', the path is %s", path2);
		aptus->sponte(&local_pius);
		return true;	/* 已经发现错误,返回内容已定,故返回真 */
	} else
 	{	
		TEXTUS_SNPRINTF(path, 1000, "%s%s", home_dir, &path2[1]);

		file = &(path[1]);

		if ( file[0] == '\0' )
			TEXTUS_STRCPY(file, "./");

		if ( sec_check && ( strstr( file, "/.." ) != (char*) 0 || strstr( file, "\\.." ) != (char*) 0))
		{
			sendError(400);
			setContentSize(0);
			WBUG("file name is invalid, ths file is %s", file);
			goto Last;
		}

		/* if the URI's last character is the '\\' or '/', remove it because of WIN32 */
		end_c = &file[strlen(file)-1];	
		if ( *end_c == '\\' || *end_c == '/' )
		{
			*end_c = '\0';
		}

		if ( stat( file, &sb ) < 0 )
		{
			WBUG("Not found the file of %s", file);
			return false;	/* 被认为找不到文件，另行处理 */
		}
		if ( !(ISMYDIR(sb.st_mode)) )
		{
			WBUG("it's not directory of %s", file);
			return false;	/* 被认为找不到文件，另行处理 */
		}
	}

	res_entity->grant(1024);
	buflen = TEXTUS_SNPRINTF( (char*)res_entity->point, 1024, "<HTML><HEAD><TITLE>Index of %s</TITLE></HEAD>\n<BODY BGCOLOR=\"#99cc99\"><H4>Index of %s</H4>\n<PRE>\n", file, file );
	res_entity->commit(buflen);
	contents_size += buflen;

	t=path2[strlen(path2)-1];
	if ( t != '\\' && t != '/' )
		TEXTUS_STRCAT(path2,"/");

	if ( strchr( file, '\'' ) == (char*) 0 )
	{
#if !defined(_WIN32)
		DIR  *dir;
		struct dirent *dt;
	#define	myname dt->d_name
		dir=opendir(file);
		if (!dir ) goto End;

		for(;(dt=readdir(dir)) != NULL;)
#else
		HANDLE hFile;
		WIN32_FIND_DATA FindFileData;
		char search[1024];
	#define	myname FindFileData.cFileName
    
		TEXTUS_STRNCPY(search, file, sizeof(search)-10);
		TEXTUS_STRCAT(search, "\\*.*");
    		hFile=::FindFirstFile(search, &FindFileData);

		if (hFile==INVALID_HANDLE_VALUE)
		{
            		goto End;	
		}

		for (;	::FindNextFile( hFile,&FindFileData); )
#endif
		{
			if(myname[0] == '.') continue;
			res_entity->grant(2048);
			TEXTUS_SNPRINTF(file2, sizeof(file2), "%s/%s", file, myname);

			if ( stat( file2, &sb ) >=0  && (ISMYDIR(sb.st_mode)) )
				buflen = TEXTUS_SNPRINTF((char*) res_entity->point, 2048, 
					"<A HREF=\"%s%s/\">%s</A>\n",  path2, myname, myname);
			else
				buflen = TEXTUS_SNPRINTF((char*) res_entity->point, 2048, 
					"<A HREF=\"%s%s\">%s</A>\n",  path2, myname, myname);
			contents_size += buflen;
			res_entity->commit(buflen);
		}

#if defined(_WIN32)
		::FindClose(hFile);
#endif
	}
End:
	res_entity->grant(1024);
	buflen = TEXTUS_SNPRINTF(  (char*) res_entity->point, 1024, "</PRE>\n<HR>\n<ADDRESS><A HREF=\"%s\">%s</A></ADDRESS>\n</BODY></HTML>\n", "http://www.sourceforge.net/projects/textus", "Textus Httpd" );
	res_entity->commit(buflen);
	contents_size += buflen;

	WBUG("sent the directory content of %s", file);

Last:
	setContentSize(contents_size);
	/* 响应头已经准备完毕, 将此发送出去 */
	local_pius.ordo = Notitia::PRO_HTTP_HEAD ;	
	aptus->sponte(&local_pius);

	local_pius.ordo = Notitia::PRO_TBUF ;	
	aptus->sponte(&local_pius);
	return true;
}

#include "hook.c"

