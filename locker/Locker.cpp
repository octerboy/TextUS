/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
/**
 Title: 锁
 Desc: Aptus扩展, 利用pthread的锁进行线程间的同步
 Build: created by octerboy, 2007/03/05, Guangzhou
 $Header: /textus/locker/Locker.cpp 13    08-01-10 1:14 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: Locker.cpp $"
#define TEXTUS_MODTIME  "$Date: 08-01-10 1:14 $"
#define TEXTUS_BUILDNO  "$Revision: 13 $"
/* $NoKeywords: $ */

#include "Aptus.h"
#include "Notitia.h"
#include "textus_string.h"
#include "casecmp.h"
#if defined(_WIN32)
#include <process.h>
#define OS_MUTEX HANDLE
#else
#define OS_MUTEX pthread_mutex_t
#include <errno.h>
#include <pthread.h>
#endif

#define HAS_NO_HOOK
#include "../trend/Trend.cpp"
#undef HAS_NO_HOOK

class Locker: public Trend {
public:
	/* Trend的被重定义 */
	Amor *clone();	

	void ignite_t	(TiXmlElement *wood, TiXmlElement *);	
	bool facio_n	( Amor::Pius *, unsigned int );
	bool dextra	( Amor::Pius *, unsigned int );
	bool sponte_n	( Amor::Pius *, unsigned int );
	bool laeve( Amor::Pius *, unsigned int );

	/* 以下为本类特别定义 */
	Locker();
	~Locker();
	
	/* 锁结构 */
	struct  Mutex { 
		char idStr[256];	/* 对于某个节点, 每个对象都不同. 不同节点的相同idStr的对象使用相同锁 */
		
		OS_MUTEX me; 
		Mutex* next; 

		inline Mutex () {
			idStr[0] = '0';
			idStr[1] = '\0';
			next = 0;
#if defined(_WIN32)
			me = NULL;
			me = CreateMutex( NULL,	// default security attributes
    				FALSE, // initially not owned
    				NULL);
			if ( me == NULL )
				printf("CreateMutex error %d\n", GetLastError());
				
#else
			pthread_mutex_init(&me, NULL);
#endif
		};

		inline ~Mutex () {
			if (next)
				delete next;

		#if defined(_WIN32)
			if ( me !=NULL )
			{
				ReleaseMutex(me);
				CloseHandle(me);
			}
		#else
			pthread_mutex_unlock(&me);
			pthread_mutex_destroy(&me);
		#endif
		};
	};

protected:
	static int g_type_ref;	/* 全局类型参考值, ignite时将此值加1, 复制到*typeID中 */
	int *typeID;            /* ignite时得到新值, clone时传递 */

	static Mutex *g_mutexes;	/* 全局锁表 */
	OS_MUTEX *loc_mut; 		/* 本对象内的指针, 指向锁表中的某一个互斥量 */

	char idStr[256];	/* 层次标识, ignite的为"", clone时, child的为前者的idStr+"-"+id 
				如："1-1-22"等 */
	int child_num;		/* clone时加一传递到子实例的serial_no */
	unsigned long serial_no;		/* 在同一个层次中的序列 */

	unsigned int depth;	/* 深度,如果设定深度2,则深度3及以上的都共享深度2的那个锁 */

	typedef struct _suit {
		Condition con_fac;
		Condition con_spo;
		Condition con_dex;
		Condition con_lae;
	} Suit;

	Suit *dolock;
	Suit *unlock;

	void joint();

	inline unsigned int deep(char *str) {
		char *p = str;
		unsigned int howd = 0;

		if ( !p) return 0;

		for(; *p; p++)
		{
			if ( *p == '-' )
				howd++;
		}
		return howd;

	};

	inline void append(Mutex *e) {
		Mutex *n, *m;
		n = g_mutexes;
		if ( !n || !e ) return;
		while (n) { 
			if ( n->next == e ) return;	/* 已有, 不再加载 */
			m = n;	/* 保留最近一个 */
			n = n->next;
		}
		m->next  = e;
	};

	#include "tbug.h"
};

int Locker::g_type_ref = 0;
Locker::Mutex* Locker::g_mutexes = new Mutex ;

Locker::Locker() {
	WBUG("new this %p", this);

	typeID = (int*) 0;
	serial_no = 0;
	child_num = 0;

	loc_mut = (OS_MUTEX *)0;
	dolock = (Suit *)0;
	unlock = (Suit *)0;

	isPoineer = false;
}

Locker::~Locker() {
	WBUG("delete this %p", this );
	if ( isPoineer )
	{
		if ( dolock ) delete dolock;
		if ( unlock ) delete unlock;
	}
}

void Locker::ignite_t (TiXmlElement *cfg, TiXmlElement *sz_ele)
{	
	const char *comm_str;
	TiXmlElement *st_ele;

	WBUG("this %p, prius %p, aptus %p, cfg %p, owner %p", this, prius, aptus, cfg, owner);

	if ( !sz_ele) 
		return;

	depth = 1;
	if ((comm_str = sz_ele->Attribute("depth")) && atoi(comm_str) >= 0 )
		depth = atoi(comm_str);
	
	canAccessed = true;	/* 至此可以认为此模块处于Locker */
	isPoineer = true;

	TEXTUS_STRCPY(idStr, "0");	/* 最初的层次 */
	g_type_ref++;	
	if ( !typeID ) 
		typeID = new int;
	*typeID = g_type_ref;	/* 父实例与子实例共享一个空间, 所以一起改变 */

	joint();	/* 挂上全局的互斥量 */
	
	/*X:变量, Y元素名 */

#define GETSUITE(X,Y)	\
	st_ele = sz_ele->FirstChildElement(Y);	\
	if ( st_ele )				\
	{					\
		X = new Suit;			\
		need_fac = get_condition(st_ele, "facio",  X->con_fac);	\
		need_spo = get_condition(st_ele, "sponte", X->con_spo);	\
		need_dex = get_condition(st_ele, "dextra", X->con_dex);	\
		need_lae = get_condition(st_ele, "laeve",  X->con_lae);	\
	}
		
	GETSUITE(dolock, "lock")	
	GETSUITE(unlock, "unlock")	
}

Amor *Locker::clone() 
{
	char tmpStr[10];
	Locker *child = 0;
	child = new Locker();
	Aptus::inherit((Aptus*) child);
#define Inherit(x) child->x = x;

	Inherit(typeID);	/* 所有子实例有相同typeID */
	Inherit(depth);		/* 所有子实例有相同深度参考 */

	Inherit(unlock);
	Inherit(dolock);

	TEXTUS_STRCPY(child->idStr, idStr);
	child->serial_no = ++child_num;	/* 这样, 第一个子实例serial_no为1, ignite的实例为0 */
	if ( deep(idStr) < depth )
	{	/* 深度未到限 */
		TEXTUS_SPRINTF(tmpStr, "-%lx", child->serial_no);
		TEXTUS_STRCAT(child->idStr, tmpStr);	/* 子实例的层次标识为父实例的idStr+"-"+子实例的serial_no */
	}
	child->joint();	/* 挂上全局的互斥量 */
	return  (Amor*)child;
}

/* 取得全局互斥量 */
void Locker::joint()
{
	Mutex *n, *neo;

	if ( loc_mut)	/* 已经取得一个互斥变量, 不用再取 */
		return;

	for(n = g_mutexes; n; n=n->next) 
	{
		if ( strcmp(n->idStr, idStr) == 0 )
		{	/* 找到相应的互斥变量 */
			loc_mut = &(n->me);
			break;
		}
	}

	if (!n)
	{ /* 没有找到, 我是第一个 */
		neo = new Mutex;
		TEXTUS_STRCPY(neo->idStr, idStr);
		loc_mut = &(neo->me);
		append(neo);
	}
}

/* X: SPONTE, FACIO 
   Y: con_spo, con_fac, con_dex, con_lae
   Z: "sponte", "facio", "dextra", "laeve"
*/
#if defined(_WIN32)
   #define HERE_LOCK(x)  WaitForSingleObject(*x, INFINITE) == WAIT_OBJECT_0 
   #define HERE_UNLOCK(x)  ReleaseMutex(*x)
#else
   #define HERE_LOCK(x) pthread_mutex_lock(x) == 0 
   #define HERE_UNLOCK(x) pthread_mutex_unlock(x) == 0 
#endif

#define DO_LOCK_OR_UN(X, Y, Z)		\
	WBUG("%s ordo %d, owner %p", Z, pius->ordo, owner);	\
	Conie *cie;				\
	bool goon = true;			\
						\
	cie =match(&(dolock->Y), pius);		\
	if (cie) 				\
	{					\
		WBUG("%s ordo %d, owner %p, lock(%p)...", Z, pius->ordo, owner, loc_mut);	\
		if ( HERE_LOCK(loc_mut) ) 	\
		{				\
			WBUG("%s ordo %d, owner %p, lock end", Z, pius->ordo, owner);	\
			for ( int i = 0; i < cie->actNum; i++ )		\
				doact(cie->actions[i], X, pius, from);	\
			goon = cie->goon;		\
		} else {			\
			WLOG_OSERR(Z);		\
		}				\
	}					\
						\
	cie =match(&(unlock->Y), pius);		\
	if (cie) 				\
	{					\
		for ( int i = 0; i < cie->actNum; i++ )		\
			doact(cie->actions[i], X, pius, from);	\
		if ( HERE_UNLOCK(loc_mut) ) 	\
		{				\
			WBUG("%s ordo %d, owner %p, unlock", Z, pius->ordo, owner);	\
			goon = cie->goon;	\
		} else {			\
			WLOG_OSERR(Z);		\
		}				\
	}					\
						\
	return !goon;				

bool Locker::sponte_n ( Amor::Pius *pius, unsigned int from)
{	/* 在owner向左发出数据前的处理 */
	DO_LOCK_OR_UN(SPONTE, con_spo, "sponte")
}

bool Locker::facio_n ( Amor::Pius *pius, unsigned int from)
{	/* 在owner向右发出数据前的处理 */
	DO_LOCK_OR_UN(FACIO, con_fac, "facio")
}

bool Locker::dextra( Amor::Pius *pius, unsigned int from)
{
	DO_LOCK_OR_UN(DEXTRA, con_dex, "dextra")
}

bool Locker::laeve( Amor::Pius *pius, unsigned int from )
{
	DO_LOCK_OR_UN(LAEVE, con_lae, "laeve")
}
#define TEXTUS_APTUS_TAG { 'L','o','c','k','e','r',0};
#include "hook.c"
