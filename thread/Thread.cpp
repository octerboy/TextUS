/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Aptus extension, to stop or continue calling the next noed.
 Build:created by octerboy, 2007/02/23, Panyu
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Aptus.h"
#include "Notitia.h"
#if !defined(_WIN32)
#include <pthread.h>
#else
#include <process.h>
#endif
#include <errno.h>

class Thread;
struct ParaTh {
	Thread *ap;
	Amor::Pius *pius;
	unsigned int from;	
};

class Thread: public Aptus {
public:
	void ignite_t (TiXmlElement *wood, TiXmlElement *);
	Amor *clone();

	bool laeve( Amor::Pius *pius, unsigned int from);	//向左调用，调用aptus的父节点
	bool dextra( Amor::Pius *pius, unsigned int from);	//向左调用，调用aptus的父节点

	Amor::Pius info_start,	/* 向右通知即将启动线程 */
			info_end;	/* 向右通知即将最后结束线程 */

	bool shouldInfo;	/* 是否通知线程的启动和结束 */
	Thread();
protected:

	TEXTUS_ORDO lae_do, dex_do;

	ParaTh dex_param;	/* 向线程入口函数传入的结构 */
	ParaTh lae_param;

#if !defined(_WIN32)
	pthread_t dth;
	pthread_t lth;
#endif

#include "tbug.h"
};

#include "textus_string.h"
#include "casecmp.h"
#include <stdio.h>

#if !defined(_WIN32)
typedef void* (*my_thread_func)(void*);
#else
typedef void (_cdecl *my_thread_func)(void*);
#endif
static void  dex_thrd(ParaTh  *arg)
{
	Aptus *pap;
#if !defined(_WIN32)
	pthread_detach(pthread_self());
#endif
	pap = (Aptus *) (arg->ap->aptus);
	if ( arg->ap->shouldInfo )	/* 线程刚启动, 即将执行一系列操作 */
		pap->dextra(&(arg->ap->info_start), arg->from+1);
		
	pap->dextra(arg->pius, arg->from+1);
	if ( arg->ap->shouldInfo )	/* 线程即将结束 */
		pap->dextra(&(arg->ap->info_end), arg->from+1);
}

static void  lae_thrd(ParaTh  *arg)
{
	Aptus *pap;
#if !defined(_WIN32)
	pthread_detach(pthread_self());
#endif
	pap = (Aptus *) (arg->ap->aptus);
	if ( arg->ap->shouldInfo )	/* 线程刚启动, 即将执行一系列操作 */
		pap->dextra(&(arg->ap->info_start), arg->from+1);
		
	pap->laeve(arg->pius, arg->from+1);
	if ( arg->ap->shouldInfo )	/* 线程即将结束 */
		pap->dextra(&(arg->ap->info_end), arg->from+1);
}

Thread::Thread()
{
	lae_do = Notitia::TEXTUS_RESERVED;
	dex_do = Notitia::TEXTUS_RESERVED;

	info_start.ordo = Notitia::JUST_START_THREAD;
	info_start.indic = 0;
	info_end.ordo = Notitia::FINAL_END_THREAD;
	info_end.indic =  0;
	shouldInfo = false;
}

void Thread::ignite_t (TiXmlElement *cfg, TiXmlElement *cf_ele)
{
	const char *comm_str;

	WBUG("this %p, prius %p, aptus %p, cfg %p", this, prius, aptus, cfg);
	if ( !cf_ele) return;

	if ( (comm_str = cf_ele->Attribute("info")) )	/* 是否通知线程的启动与结束 */
	{
		if ( strcasecmp(comm_str, "yes") == 0 )
			shouldInfo = true;
	}

//#define WHORDO(Y, Z) \
	comm_str = cf_ele->Attribute(Z);	\
		BTool::get_textus_ordo(&Y, comm_str);	
#define WHORDO(Y, Z) Y = Notitia::get_ordo(cf_ele->Attribute(Z));

	WHORDO(lae_do, "sponte");
	WHORDO(dex_do, "facio");

	canAccessed = true;	/* 至此可以认为此应用模块需要Thread */
	if ( lae_do != Notitia::TEXTUS_RESERVED )
		need_lae = true;

	if ( dex_do != Notitia::TEXTUS_RESERVED )
		need_dex = true;
}

Amor *Thread::clone()
{	
	Thread *child = 0;
	child = new Thread();
	Aptus::inherit( (Aptus*) child );
#define Inherit(x) child->x = x;

	Inherit(dex_do);
	Inherit(lae_do);
	Inherit(shouldInfo);
	return  (Amor*)child;
}

bool Thread::dextra( Amor::Pius *pius, unsigned int from)
{	/* 进入owner->facio()之前 */
	
	WBUG("dextra Notitia::%lu owner is %p", pius->ordo, owner);
	if ( pius->ordo == dex_do )
	{
		/* 线程执行 */
		dex_param.ap = this;
		dex_param.pius = pius;
		dex_param.from = from;

#if !defined(_WIN32)
		if (pthread_create(&dth, NULL, (my_thread_func)dex_thrd, (void*)&dex_param) )
			WLOG_OSERR("pthread_create")
#else
		if ( _beginthread((my_thread_func)dex_thrd, 0, &dex_param) == -1 )
			WLOG_OSERR("_beginthread")
#endif
		return true;
	} else
		return false;
}

bool Thread::laeve( Amor::Pius *pius, unsigned int from)
{	/* 进入owner->sponte()之前的处理 */
	WBUG("laeve Notitia::%lu owner is %p", pius->ordo, owner);
	if ( pius->ordo == dex_do )
	{
		/* 线程执行 */
		lae_param.ap = this;
		lae_param.pius = pius;
		lae_param.from = from;
		
#if !defined(_WIN32)
		if (pthread_create(&lth, NULL, (my_thread_func)lae_thrd, (void*)&lae_param) )
			WLOG_OSERR("pthread_create")
#else
		if ( _beginthread((my_thread_func)lae_thrd, 0, &lae_param) == -1 )
			WLOG_OSERR("_beginthread")
#endif
	
		return true;
	} else
		return false;
}
#define TEXTUS_APTUS_TAG { 'T','h','r','e','a','d',0}
#include "hook.c"
