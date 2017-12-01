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
#include "TBuffer.h"
#include "BTool.h"
#include "casecmp.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/timeb.h>
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
#define ROW_SPACE 5
#define ROW_SIZE (65+ROW_SPACE)
#define FACIO 0
#define SPONTE 1

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

	unsigned long instance_id;
	enum OUT_FORM { DIRECT_VIEW = 0 ,  DEBUG_VIEW =1, DEBUG_VIEW_X=2 };
	enum SHOW_DEV { NONE_SHOW = 0 ,  STDOUT_SHOW =1, STDERR_SHOW=2 };
	enum SPLIT { SP_NONE = 0 ,  SP_DATE =1};

private:
	struct G_CFG {
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
#if !defined (_WIN32)
		struct flock lock, unlock;
#endif
		inline G_CFG() {
			split = SP_NONE;
			filename = 0;
			form = DIRECT_VIEW;
			multi = false;

			interval = 0; 
			hasLock = false;
			show = NONE_SHOW;
			toClear = true;

			fileD = -1;
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

	TINLINE void deliver(Notitia::HERE_ORDO aordo);
	TINLINE void output(TBuffer *, int direct);
	TINLINE int bug_view(TBuffer *, char*);

	char *out_buf; int out_len;
#include "wlog.h"

};

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

	return ;
}

bool Bu2File::facio( Amor::Pius *pius)
{
	TBuffer **tb;

	switch(pius->ordo )
	{
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
		if ( !first_buf )
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
#if !defined (_WIN32)
				close(gCFG->fileD);
#else
				_close(gCFG->fileD);
#endif
				gCFG->fileD = -1;
				deliver(Notitia::DMD_SET_ALARM);
			}
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

int Bu2File::bug_view(TBuffer *tbuf, char *str )
{
	int i,ri, len, rows, rest;
	unsigned char *p = tbuf->base;
	len = tbuf->point - tbuf->base;
	rows = len/16;
	rest = len - rows*16;

	int offset = 0;

	for ( ri = 0 ; ri < rows+1 ; ri++, offset += ROW_SIZE)
	{
		char *rstr = &str[offset];
		int left = (ri == rows ? rest : 16) ;

		memset (rstr, ' ', ROW_SIZE);
		rstr[ROW_SIZE-1] =  '\n';
		for ( i = 0 ; i < left; i++)
		{
			unsigned char c = p[ri*16+i];
			int o = i*3;
			if ( i >= 8 )  o++; 	//中间加一空格
			if ( gCFG->form == DEBUG_VIEW_X )
			{
				rstr[o++] = ObtainX((c & 0xF0 ) >> 4 );
				rstr[o++] = ObtainX(c & 0x0F );
			} else {
				rstr[o++] = Obtainx((c & 0xF0 ) >> 4 );
				rstr[o++] = Obtainx(c & 0x0F );
			}

			if ( c >= 0x20 && c <= 0x7e )
				rstr[48+ROW_SPACE+i] =  c;
			else
				rstr[48+ROW_SPACE+i] =  '.';
		}
	}
	return offset;
}

void Bu2File::output(TBuffer *tbuf, int direct )
{
	int wLen, w2Len;
	char *w_buf;
	char r_name[128];
	const char *rn;

	assert(gCFG);
	if ( gCFG->form != DIRECT_VIEW )
	{ 
		int needl = (( tbuf->point - tbuf->base )/16+2)*ROW_SIZE + 1;
		if (out_len < needl )
		{
			delete []out_buf;
			out_len = needl;
			out_buf = new char[out_len];
		}

		memset(out_buf, 0, out_len);
		memset(out_buf, ' ', ROW_SIZE);
		if ( direct == FACIO )
			memcpy(out_buf,	 "\nFACIO", 6);
		else
			memcpy(out_buf,	 "\nSPONTE", 7);

		out_buf[ROW_SIZE-1] = '\n';
		w2Len = ROW_SIZE + bug_view(tbuf, &out_buf[ROW_SIZE] ) ;
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
	}
	else if ( gCFG->show == STDERR_SHOW )
	{
#if defined(_WIN32)
		printf("%s",  w_buf);
#else
		fprintf(stderr, "%s",  w_buf);	//有一天发现, 用vs 2003编译, stderr导致程序崩溃。
#endif
	}
		
	if (!gCFG->filename)
		goto NOFILE_PRO;

	if ( gCFG->split == SP_DATE)
	{
		int here_day;
#if defined(_WIN32) && (_MSC_VER < 1400 )
	struct _timeb now;
#else
	struct timeb now;
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
	ftime(&now);
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
		tdatePtr = &tdate;
		localtime_s(tdatePtr, &now.time);
#else
		tdatePtr = localtime(&now.time);
#endif
		here_day=tdatePtr->tm_mday;
		if ( gCFG->last_day != here_day)
		{
			if ( gCFG->fileD >= 0 )
			{
#if !defined (_WIN32)
				close(gCFG->fileD);
#else
				_close(gCFG->fileD);
#endif
				gCFG->fileD = -1;
			}
			gCFG->last_day = here_day;
		}
		
		TEXTUS_STRCPY(ftmstr,"%y-%m-%d");
		strftime(timestr, 64, ftmstr, tdatePtr);

		rn=&r_name[0];
		TEXTUS_SPRINTF(r_name, gCFG->filename, timestr);
		
	} else
		rn = gCFG->filename;

	if ( gCFG->fileD < 0)
	{
#if !defined (_WIN32)
		if( (gCFG->fileD = open(rn,O_CREAT|O_RDWR|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) ) < 0)
		{
			goto NOFILE_PRO;
		}
#else
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
		if( (_sopen_s(&gCFG->fileD, rn, O_CREAT|O_RDWR|O_APPEND, SH_DENYNO, _S_IWRITE )) !=0 )
#else
		if( (gCFG->fileD = _sopen(rn, O_CREAT|O_RDWR|O_APPEND, SH_DENYNO,_S_IWRITE )) < 0)
#endif
		{
			goto NOFILE_PRO;
		}
#endif
	}

	if (gCFG->hasLock)	/* 文件加锁 */
#if !defined (_WIN32)
		if(fcntl(gCFG->fileD, F_SETLKW, &gCFG->lock)==-1)
#else
		if( _locking( gCFG->fileD, _LK_LOCK, w2Len) != 0 )
#endif
			goto NOFILE_PRO;

#if !defined (_WIN32)
	wLen = write(gCFG->fileD, w_buf, w2Len);
#else
	wLen = _write(gCFG->fileD, w_buf, w2Len);
#endif
	if (gCFG->hasLock)	/* 文件解锁, 对于MS VC,采用CRT的函数, 一个字节 */
#if !defined (_WIN32)
		if(fcntl(gCFG->fileD, F_SETLKW, &gCFG->unlock)==-1)
#else
		if( _locking( gCFG->fileD, LK_UNLCK, wLen) != 0 )
#endif
			goto NOFILE_PRO;

	if ( gCFG->interval <=0 )
	{
#if !defined (_WIN32)
		close(gCFG->fileD);
#else
		_close(gCFG->fileD);
#endif
		gCFG->fileD = -1;
	}

	if ( !alarmed && gCFG->interval > 0 ) 
	{
		alarmed = true;
		deliver(Notitia::DMD_SET_ALARM); /* 设定时 */ 
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
	child->instance_id = instance_id+1;

	return (Amor*)child;
}

/* 向接力者提交 */
TINLINE void Bu2File::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	void *arr[3];

	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0x0;
	switch (aordo )
	{
	case Notitia::DMD_SET_ALARM:
		WBUG("deliver(sponte) DMD_SET_ALARM");
		tmp_pius.indic = &arr[0];
		arr[0] = this;
		arr[1] = &gCFG->interval;
		arr[2] = 0;
		break;

	default:
		WBUG("deliver Notitia::%d", aordo);
		break;
	}

	aptus->sponte(&tmp_pius);
}

#define AMOR_CLS_TYPE Bu2File
#include "hook.c"
