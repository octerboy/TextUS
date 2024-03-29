/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: TBuffer to file
 Build: created by octerboy, 2007/05/28
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "textus_string.h"
#include "TBuffer.h"
#include "BTool.h"
#include "casecmp.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#if defined(TEXTUS_PLATFORM_64) && !defined(_WIN32)
#include <sys/time.h>
#else
#include <sys/timeb.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>

#if !defined (_WIN32)
#include <unistd.h>
#else
#include <io.h>
#include <share.h>
#include <sys/locking.h>
#endif

#include <fcntl.h>

#ifndef TINLINE
#define TINLINE inline
#endif

#define BZERO(X) memset(X, 0 ,sizeof(X))
//#define ROW_SPACE 5
//#define ROW_SIZE (65+row_space)
#define FACIO 0
#define SPONTE 1

#if defined (_WIN32 )
#define ERROR_PRO(X) { \
	char *s; \
	char error_string[1024]; \
	DWORD dw = GetLastError(); \
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw, \
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) error_string, 1024, NULL );\
	s= strstr(error_string, "\r\n") ; \
	if (s )  *s = '\0';  WLOG(WARNING,  errMsg, "%s errno %d, %s", X,dw, error_string); \
	}
#else
#define ERROR_PRO(X)  WLOG(WARNING, errMsg, "%s errno %d, %s.", X, errno, strerror(errno));
#endif

class Bu2File: public Amor
{
public:
	Bu2File();
	~Bu2File();

	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
	
	TBuffer *first_buf;
	TBuffer *second_buf;

	unsigned int instance_id;
	enum OUT_FORM { DIRECT_VIEW = 0 ,  DEBUG_VIEW =1, DEBUG_VIEW_X=2 };
	enum SHOW_DEV { NONE_SHOW = 0 ,  STDOUT_SHOW =1, STDERR_SHOW=2 };
	enum SPLIT { SP_NONE = 0 ,  SP_DATE =1};

private:
	Amor::Pius alarm_pius;  /* 设超时 */
	void *arr[3];
	char errMsg[256];
	char id_str[16];
	struct G_CFG {
		unsigned int instance_id;
		const char *filename;	//文件名, 当SPLIT不为0时, 这个文件名就成为一个格式符
		OUT_FORM form;		/* 输出形式, 0: 标准, 直接输出; 1: 16进制, 并输出ASCII */
		bool multi;		/* 多实例 */
		int interval;		/* 文件写入时间间隔 */
		bool hasLock;
		SHOW_DEV show;
		bool toClear;
		SPLIT split;

		int fileD;
		int last_day;	//当日志按日期写时，这里记一下当时的日子数。
		int disp_len;
		int row_space;
		int row_size;
#if !defined (_WIN32)
		struct flock lock, unlock;
#endif
		inline G_CFG() {
			instance_id = 0;;
			split = SP_NONE;
			filename = 0;
			form = DIRECT_VIEW;
			multi = false;

			interval = 0; 
			hasLock = false;
			show = NONE_SHOW;
			toClear = true;

			fileD = -1;
			disp_len = 16;
#if !defined (_WIN32)
			lock.l_type = F_WRLCK;
			lock.l_start = 0;
			lock.l_whence = SEEK_END;
			lock.l_len = 0;
			lock.l_pid = getpid();

			unlock.l_type = F_UNLCK;
			unlock.l_start = 0;
			unlock.l_whence = SEEK_END;
			unlock.l_len = 0;
			unlock.l_pid = getpid();
#endif
		};

		inline ~G_CFG() {
			if ( fileD >= 0 )
#if !defined (_WIN32)
				close(fileD);
#else
				_close(fileD);
#endif
		};
	};

	struct G_CFG *gCFG;
	bool has_config;
	bool alarmed;

	TINLINE void output(TBuffer *, int direct);
	TINLINE int bug_view(TBuffer *, char*);

	char *out_buf; 
	TEXTUS_LONG out_len;
	bool get_file_name();
	char real_fname[128];
	
#include "wlog.h"

};
#if !defined (_WIN32)
#define MY_CLOSE	\
		if (close(gCFG->fileD) != 0 ) 	\
			ERROR_PRO("close file")
#else
#define MY_CLOSE	\
		if (_close(gCFG->fileD) != 0) \
			ERROR_PRO("close file")
#endif

void Bu2File::ignite(TiXmlElement *prop)
{
	const char *comm_str;

	if (!prop) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
	}

	gCFG->filename = prop->Attribute("file");
	prop->QueryIntAttribute("interval", &gCFG->interval);
	comm_str = prop->Attribute("lock");
	if ( comm_str && strcasecmp( comm_str, "yes") == 0 )
	{
		gCFG->hasLock = true;
	}

	comm_str = prop->Attribute("show");
	if ( comm_str && strcasecmp( comm_str, "yes") == 0 )
	{
		gCFG->show = STDOUT_SHOW;
	} else if ( comm_str && strcasecmp( comm_str, "stdout") == 0 )
	{
		gCFG->show = STDOUT_SHOW;
	} else if ( comm_str && strcasecmp( comm_str, "stderr") == 0 )
	{
		gCFG->show = STDERR_SHOW;
	}

	comm_str = prop->Attribute("clear");
	if ( comm_str && strcasecmp( comm_str, "no") == 0 )
	{
		gCFG->toClear = false;
	}

	comm_str = prop->Attribute("multi");
	if ( comm_str && strcasecmp( comm_str, "yes") == 0 )
		gCFG->multi = true; 

	comm_str = prop->Attribute("split");
	if ( comm_str && strcasecmp( comm_str, "date") == 0 )
		gCFG->split = SP_DATE; 

	comm_str = prop->Attribute("style");
	if ( comm_str ) { 
		if (strcasecmp( comm_str, "debug") == 0 )
			gCFG->form = DEBUG_VIEW; 
		if (strcmp( comm_str, "debugX") == 0 )
			gCFG->form = DEBUG_VIEW_X; 
		if (strcmp( comm_str, "debugx") == 0 )
			gCFG->form = DEBUG_VIEW; 
	}
	prop->QueryIntAttribute("line", &gCFG->disp_len);
	gCFG->row_space = gCFG->disp_len/8 + 3;
	gCFG->row_size = gCFG->disp_len*4 +1+ gCFG->row_space + 2*((gCFG->disp_len-16)/16);

	return ;
}

bool Bu2File::facio( Amor::Pius *pius)
{
	TBuffer **tb;

	switch(pius->ordo )
	{
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		arr[0] = this;
		arr[1] = &(gCFG->interval);
		arr[2] = &(gCFG->interval);
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		arr[0] = this;
		arr[1] = &(gCFG->interval);
		arr[2] = &(gCFG->interval);
		break;

	case Notitia::SET_TBUF:	/* 取得输入TBuffer地址 */
		WBUG("facio SET_TBUF");

		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//当然tb不能为空
			if ( *tb) 
			{	//新到请求的TBuffer
				first_buf = *tb;
			}
			tb++;
			if ( *tb) second_buf = *tb;
		} else 
			WLOG(NOTICE,"facio PRO_TBUF null.");

		break;

	case Notitia::PRO_TBUF :
		WBUG("facio PRO_TBUF");
		if (pius->indic) 
		{	//当然tb不能为空
			tb = (TBuffer **)(pius->indic);
			if ( *tb) 
			{	//新到请求的TBuffer
				first_buf = *tb;
			} else {
				WLOG(WARNING, "first_buf is null");
				break;
			}
			tb++;
			if ( *tb) second_buf = *tb;
		} else if ( !first_buf )
		{
			WLOG(WARNING, "first_buf is null");
			break;
		}

		output(first_buf, FACIO);
		break;

	case Notitia::TIMER:	/* 定时信号 */
		WBUG("facio TIMER" );
		if ( gCFG->interval > 0 )
		{
			if ( gCFG->fileD >= 0 )
			{
				MY_CLOSE
				gCFG->fileD = -1;
				//aptus->sponte(&alarm_pius);
			}
		}

		break;

	case Notitia::CMD_CLOSE_FILE:	/* close file */
		WBUG("facio CMD_CLOSE_FILE" );
		if ( gCFG->fileD >= 0 )
		{
			MY_CLOSE
			gCFG->fileD = -1;
		}
		break;

	case Notitia::CMD_ZERO_FILE:	/* 清零 */
		WBUG("facio CMD_ZERO_FILE" );
		if ( gCFG->fileD >= 0 )
		{
			MY_CLOSE
			gCFG->fileD = -1;
		}
		if (get_file_name())
		{
#if !defined (_WIN32)
			if ( (gCFG->fileD = open(real_fname, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) == -1 )
				ERROR_PRO("open when ZERO_FILE")
#else
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
			if( _sopen_s(&gCFG->fileD, real_fname, O_CREAT|O_RDWR|O_TRUNC, SH_DENYNO, _S_IWRITE )!=0 )
#else
			if ( (gCFG->fileD = _sopen(real_fname, O_CREAT|O_RDWR|O_TRUNC, SH_DENYNO,_S_IWRITE )) < 0 )
#endif
				ERROR_PRO("sopen when ZERO_FILE")
#endif
		}
		break;

	default:
		return false;
	}
	return true;
}

bool Bu2File::sponte( Amor::Pius *pius)
{
	switch(pius->ordo )
	{
	case Notitia::PRO_TBUF :
		WBUG("sponte PRO_TBUF");
		if ( !second_buf )
		{
			WLOG(WARNING, "second_buf is null");
			break;
		}
		output(second_buf, SPONTE);
		break;

	default:
		return false;
	}
	return true;
}

int Bu2File::bug_view(TBuffer *tbuf, char *str)
{
	TEXTUS_LONG len, rows, rest, i,ri;
	unsigned char *p = tbuf->base;
	len = tbuf->point - tbuf->base;
	rows = len/gCFG->disp_len;
	rest = len - rows*gCFG->disp_len;

	int offset = 0;

	for ( ri = 0 ; ri < rows+1 ; ri++, offset += gCFG->row_size)
	{
		char *rstr = &str[offset];
		TEXTUS_LONG left = (ri == rows ? rest : gCFG->disp_len) ;

		memset (rstr, ' ', gCFG->row_size);
		rstr[gCFG->row_size-1] =  '\n';
		for ( i = 0 ; i < left; i++)
		{
			unsigned char e;
			unsigned char c = p[ri*gCFG->disp_len+i];
			TEXTUS_LONG o = i*3;
			for ( e = 1; e <= i/8; e++) o++;  //中间加一空格
			//	if ( i >= e*o )  o++; 	//中间加一空格
			if ( gCFG->form == DEBUG_VIEW_X )
			{
				rstr[o++] = ObtainX((c & 0xF0 ) >> 4 );
				rstr[o++] = ObtainX(c & 0x0F );
			} else {
				rstr[o++] = Obtainx((c & 0xF0 ) >> 4 );
				rstr[o++] = Obtainx(c & 0x0F );
			}

			o = gCFG->disp_len*3+gCFG->row_space+i;
			for ( e = 1; e <= i/16; e++) {o++; o++; }
			if ( c >= 0x20 && c <= 0x7e )
				rstr[o] =  c;
			else
				rstr[o] =  '.';
		}
	}
	return offset;
}

bool Bu2File::get_file_name()
{
	if (!gCFG->filename)
		return false;

	if ( gCFG->split == SP_DATE)
	{
		int here_day;
#if defined(_WIN32) && (_MSC_VER < 1400 )
	struct _timeb now;
#else
#if defined(TEXTUS_PLATFORM_64) && !defined(_WIN32)
	struct timeval now;
#else
	struct timeb now;
#endif
#endif
		struct tm *tdatePtr;
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
		struct tm tdate;
#endif
		char timestr[64];
		char ftmstr[128];

#if defined(_WIN32) && (_MSC_VER < 1400 )
	_ftime(&now);
#else
#if defined(TEXTUS_PLATFORM_64) && !defined(_WIN32)
	gettimeofday(&now,0);
#define NOW_TIME now.tv_sec
#define NOW_MILLITM now.tv_usec
#else
	ftime(&now);
#define NOW_TIME now.time
#define NOW_MILLITM now.millitm
#endif
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
		tdatePtr = &tdate;
		localtime_s(tdatePtr, &NOW_TIME);
#else
		tdatePtr = localtime(&NOW_TIME);
#endif
		here_day=tdatePtr->tm_mday;
		if ( gCFG->last_day != here_day)
		{
			if ( gCFG->fileD >= 0 )
			{
				MY_CLOSE
				gCFG->fileD = -1;
			}
			gCFG->last_day = here_day;
		}
		
		TEXTUS_STRCPY(ftmstr,"%y-%m-%d");
		strftime(timestr, 64, ftmstr, tdatePtr);

		TEXTUS_SPRINTF(real_fname, gCFG->filename, timestr);
	} else
		TEXTUS_STRCPY(real_fname, gCFG->filename);

	return true;
}

void Bu2File::output(TBuffer *tbuf, int direct)
{
	TEXTUS_LONG wLen;
	size_t w2Len;
	char *w_buf;

	assert(gCFG);
	if ( gCFG->form != DIRECT_VIEW )
	{ 
		TEXTUS_LONG needl = (( tbuf->point - tbuf->base )/gCFG->disp_len+2)*gCFG->row_size + 15;
		if (out_len < needl )
		{
			delete []out_buf;
			out_len = needl;
			out_buf = new char[out_len];
		}

		memset(out_buf, 0, out_len);
		memset(out_buf, ' ', gCFG->row_size);
		if ( direct == FACIO )
		{
			memcpy(out_buf,	 "\nFACIO-", 7);
			memcpy(&out_buf[7], id_str, strlen(id_str));
		} else {
			memcpy(out_buf,	 "\nSPONTE-", 8);
			memcpy(&out_buf[8], id_str, strlen(id_str));
		}

		out_buf[gCFG->row_size-1] = '\n';
		w2Len = gCFG->row_size + bug_view(tbuf, &out_buf[gCFG->row_size]) ;
		w_buf = out_buf;

	} else {
		if ( gCFG->show != NONE_SHOW)
		{
			if ( tbuf->point < tbuf->limit)
				*tbuf->point = 0;
		}
		w2Len = tbuf->point - tbuf->base;
		w_buf = (char*) tbuf->base;
	}
	
	if ( gCFG->show == STDOUT_SHOW )
	{
#if defined(_WIN32)
		printf("%s",  w_buf);
#else
		fprintf(stdout, "%s",  w_buf);	//有一天发现, 用vs 2003编译, stdout导致程序崩溃。
#endif
	} else if ( gCFG->show == STDERR_SHOW )
	{
#if defined(_WIN32)
		printf("%s",  w_buf);
#else
		fprintf(stderr, "%s",  w_buf);	//有一天发现, 用vs 2003编译, stderr导致程序崩溃。
#endif
	}
		
	if (!get_file_name()) goto NOFILE_PRO;
	if ( gCFG->fileD < 0)
	{
#if !defined (_WIN32)
		if( (gCFG->fileD = open(real_fname, O_CREAT|O_RDWR|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) ) < 0)
		{
			ERROR_PRO("open file")
			goto NOFILE_PRO;
		}
#else
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
		if( (_sopen_s(&gCFG->fileD, real_fname, O_CREAT|O_RDWR|O_APPEND, SH_DENYNO, _S_IWRITE )) !=0 )
#else
		if( (gCFG->fileD = _sopen(real_fname, O_CREAT|O_RDWR|O_APPEND, SH_DENYNO,_S_IWRITE )) < 0)
#endif
		{
			ERROR_PRO("sopen file")
			goto NOFILE_PRO;
		}
#endif
	}

	if (gCFG->hasLock)	/* 文件加锁 */
	{
#if defined (_WIN32)
		if( _locking( gCFG->fileD, _LK_LOCK, static_cast<int>(w2Len)) != 0 )
#else
		if(fcntl(gCFG->fileD, F_SETLKW, &gCFG->lock)==-1)
#endif
		{
			ERROR_PRO("lock file")
			goto NOFILE_PRO;
		}
	}

#if defined (_WIN32)
	wLen = _write(gCFG->fileD, w_buf, static_cast<int>(w2Len));
#else
	wLen = write(gCFG->fileD, w_buf, w2Len);
#endif
	if (gCFG->hasLock)	/* 文件解锁, 对于MS VC,采用CRT的函数, 一个字节 */
	{
#if defined (_WIN32)
		if( _locking( gCFG->fileD, LK_UNLCK, static_cast<int>(wLen)) != 0 )
#else
		if(fcntl(gCFG->fileD, F_SETLKW, &gCFG->unlock)==-1)
#endif
		{
			ERROR_PRO("unlock file")
			goto NOFILE_PRO;
		}
	}

	if ( gCFG->interval < 0 )	//对于＝0的情况，文件永不关闭
	{
		MY_CLOSE
		gCFG->fileD = -1;
	}

	if ( !alarmed && gCFG->interval > 0 ) 
	{
		alarmed = true;
		aptus->sponte(&alarm_pius);
	}

	if ( gCFG->toClear && wLen > 0)
	{
		if (  gCFG->form != DIRECT_VIEW )
			tbuf->reset();
		else
			tbuf->commit(-wLen);
	}

	return;
NOFILE_PRO:
	if ( gCFG->toClear )
		tbuf->reset();
}

Bu2File::Bu2File()
{
	gCFG = 0;
	has_config = false;
	alarmed = false;

	instance_id = 0;
	out_len = 256;
	out_buf = new char[out_len];
	id_str[0] = '0';
	id_str[1] = 0;
	alarm_pius.ordo = Notitia::DMD_SET_ALARM;
	alarm_pius.indic = &arr[0];
}

Bu2File::~Bu2File()
{
	if (has_config ) 
	{
		if(gCFG) delete gCFG;
	}	
	first_buf = 0x0;
	second_buf = 0x0;
	delete []out_buf;
}

Amor* Bu2File::clone()
{
	Bu2File *child;
	child = new Bu2File();
	child->gCFG = gCFG;
	gCFG->instance_id++;
	child->instance_id = gCFG->instance_id;
	TEXTUS_SPRINTF(child->id_str, "%d", child->instance_id);

	return (Amor*)child;
}

#define AMOR_CLS_TYPE Bu2File
#include "hook.c"
