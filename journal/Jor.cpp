/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Aptus extension, generate log for all modules and output to a module
 Build: created by octerboy, 2006/04/01, Wuhan
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Aptus.h"
#include "Logger.h"
#include "BTool.h"
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <stdio.h>

#if defined(_WIN32)
#include <process.h>
#define GETPID _getpid()
#else
#include <unistd.h>
#define GETPID getpid()
#endif

#if defined (MULTI_PTHREAD) 
#if defined (_WIN32)
#include <synchapi.h>
	typedef CRITICAL_SECTION Spin_Type;
	#define Do_Spin_Lock(X) EnterCriticalSection(&X);
	#define Do_Spin_UnLock(X) LeaveCriticalSection(&X);
	#define Spin_Init(X) InitializeCriticalSectionAndSpinCount(&X, 1000);

#elif  defined(__APPLE__)  
#include <os/lock.h>
	typedef os_unfair_lock Spin_Type;
	#define Do_Spin_Lock(X) os_unfair_lock_lock(&X);
	#define Do_Spin_UnLock(X) os_unfair_lock_unlock(&X);
	#define Spin_Init(X) X=OS_UNFAIR_LOCK_INIT;
#else	/*  for pthread, BSD, Linux, Solaris etc */

#include <sys/types.h>
#include <pthread.h>
	typedef pthread_spinlock_t Spin_Type;
	#define Do_Spin_Lock(X) pthread_spin_lock(&X);
	#define Do_Spin_UnLock(X) pthread_spin_unlock(&X);
	#define Spin_Init(X) pthread_spin_init(&X, PTHREAD_PROCESS_SHARED);
#endif
#endif

#define LOG_BUF_SIZE 1280	/* 日志内容, 1280字符一行， 应该不少吧 */
#define LOG_BUF_TMP_SIZE 1024
class Jor: public Aptus {
public:
	void ignite_t (TiXmlElement *wood, TiXmlElement *);
	Amor *clone();
	Aptus *clone_p(Aptus *p);
	bool sponte ( Amor::Pius *);

	Jor();
	~Jor();
	
protected:
#if defined (MULTI_PTHREAD) 
	static Spin_Type time_tick_spin;	
#endif
	static bool hasPid;		/* 是否要记进程的pid */
	static bool detect_modified;		/* 是否防篡改内容 */
	static char hostname[64];	/* 主机名，在 logdata 这个 Attachment 元素中用属性说明 */
	static time_t lastSec;		/* 最后的秒数 */
	static unsigned short lastMilli;/* 最后的毫秒数 */
	static int serialNo;		/* 如果毫秒数都相同，则采用序列号 */
	static char last_md_sum[8];	/* 上一次日志的MD5值 */
	static char md_magic[64];	/*默认是MD_MAGIC */
	static short md_magic_len;
	
	int aliusID;		/* 每一个模块的唯一标志，默认为 -1 */
	char alius[128];	/* 每个模块的别名 */
	unsigned int instance_id;	/* 每个模块的实例号 */
	unsigned int instance_last_id;	/*  最后实例号 */
	
	static Aptus **channels;	/* 日志输出渠道 */
	static int chnnls_num;

	bool isPioneer;
#ifdef DEBUG
#undef DEBUG
#endif
	enum LogLevel {EMERG=0, ALERT=1, CRIT=2, ERR=3, WARNING=4, NOTICE=5, INFO=6, DEBUG=7};
	bool levelActive[8];	
	
#include "tbug.h"
};
#include "Notitia.h"
#include "casecmp.h"
#include "textus_string.h"
#include <stdio.h>
#include <string.h>
#include <sys/timeb.h>
#define CHNNL_NUM 0

char Jor::hostname[] = "";
time_t Jor::lastSec = 0;
bool Jor::hasPid = false;
unsigned short Jor::lastMilli = 0;
int Jor::serialNo = 0;
Aptus** Jor::channels = (Aptus **)0;
int Jor::chnnls_num = CHNNL_NUM;

char Jor::last_md_sum[] = {0};
bool Jor::detect_modified = false;
char Jor::md_magic[] = {0};
short Jor::md_magic_len = 0;
#define MD_MAGIC "1E19A89ADAFGAD"  // 每条日志中再加一点内容，再加MD5，以简单实现的防改。

Jor::Jor()
{
	aliusID = -1;
	memset(alius, 0, sizeof(alius));
	
	instance_id = 0;
	instance_last_id = 0;
	isPioneer = false;
	if ( last_md_sum[0] == 0 ) 	//以此判断, 整个进程中, 这里初始为"00000000"
	{
		memset(last_md_sum, '0', sizeof(last_md_sum));
	}
}

Jor::~Jor() { 
	/* 本对象被释放, owner->aptus也要 (或已经被释放), 
	所以，如果这个owner是记录日志的channel，
	则要将之从中去掉 */

	for ( int j = 0 ; channels[j]; j++)
	{
		if ( owner->aptus == channels[j] )
		{
			for ( int i = j ; channels[i]; i++)
			{
				channels[i] = channels[i+1]; /* 最后一项为null */
			}
		}
	}

/*
	if ( !channels[0] )	
		delete[] channels;
*/
}

void Jor::ignite_t (TiXmlElement *cfg, TiXmlElement *log_ele)
{	/* 从cfg获取hostname参数, aliusID等从carbo获得 */
	const char *host_str, *comm_str;
	TiXmlElement *level_ele;
		
	WBUG("this %p , prius %p, aptus %p, cfg %p", this, prius, aptus, cfg);
	if ( !log_ele) return;
	TusLogger *o = reinterpret_cast<TusLogger*>(this->owner);
	o->give_logger(this);

	if ( (comm_str = cfg->Attribute("pid")) && (comm_str[0] == 'y' || comm_str[0] == 'Y') )
		hasPid = true;

	if ( (comm_str = cfg->Attribute("md5")) && (comm_str[0] == 'y' || comm_str[0] == 'Y') )
		detect_modified = true;

	if ( (comm_str = cfg->Attribute("md5_magic")) )
	{
		memset(md_magic, 0, sizeof(md_magic));
		TEXTUS_STRNCPY(md_magic, comm_str, sizeof(comm_str)-2);
	} else 
		TEXTUS_STRCPY(md_magic, MD_MAGIC);
	md_magic_len = (short)strlen(md_magic);

	if ( (host_str = cfg->Attribute("host")) )
	{
		memset(hostname, 0, sizeof(hostname));
		TEXTUS_STRNCPY(hostname, host_str, sizeof(hostname)-2);
		TEXTUS_STRCAT(hostname, " ");
	}

	if ( (comm_str = cfg->Attribute("maxium")) && atoi(comm_str) > CHNNL_NUM )
		chnnls_num = atoi(comm_str);

	if ( !channels)
	{
		channels = new Aptus*[chnnls_num+1]; /* 以null指针作为输出渠道指针组的结尾标志,故要多加一个 */
		memset( channels, 0, (chnnls_num+1) * sizeof(Amor *));
	}
	
	canAccessed = true;	/* 至此可以认为此模块需要日志 */
	need_spo = true;

	/* 取得别名 */
	if ( (comm_str = log_ele->Attribute("alias")) )
	{
		TEXTUS_STRNCPY(alius, comm_str, sizeof(alius)-2);
		TEXTUS_STRCAT(alius, " ");
	}

	/* 取得ID */
	if ( (comm_str =log_ele->Attribute("ID")) )
		aliusID = atoi( comm_str);

	/* 取得输出渠道 */
	if ( (comm_str =log_ele->Attribute("channel")) && strcmp(comm_str, "yes") == 0 )
	{
		int i;
		for (i=0 ; i < chnnls_num; i++ )
		if (!channels[i]) 
		{
			channels[i] = (Aptus*)owner->aptus;
			break;
		}
	}

	/* 默认所有日志等级都记 */
	for (unsigned int j = 0; j < sizeof(levelActive); j++)
		levelActive[j] = true;

	level_ele = log_ele->FirstChildElement("exclude");
	while(level_ele)
	{
		comm_str = level_ele->Attribute("level");
		if (comm_str) 
		{	/* 扫描不要记的日志 */
#define UNSETLOG(X) if ( strcasecmp ( comm_str, #X ) == 0 ) \
				levelActive[X] = false;

			UNSETLOG(ERR);	UNSETLOG(DEBUG);UNSETLOG(WARNING);
			UNSETLOG(CRIT);	UNSETLOG(EMERG);UNSETLOG(ALERT);
			UNSETLOG(INFO);	UNSETLOG(NOTICE); 
		}
		if ( strcmp(comm_str, "all") == 0 ) 
		{
			levelActive[ERR] 	= false;
			levelActive[DEBUG] 	= false;
			levelActive[WARNING] 	= false;
			levelActive[CRIT] 	= false;
			levelActive[EMERG] 	= false;
			levelActive[ALERT] 	= false;
			levelActive[INFO] 	= false;
			levelActive[NOTICE]	= false;
			break;
		}
			
#undef UNSETLOG
		level_ele = level_ele->NextSiblingElement("exclude");
	}

	isPioneer = true;
}

Amor *Jor::clone()
{	
/*
	Jor *child = 0;
	child = new Jor();
	Aptus::inherit( (Aptus*) child);
	canAccessed = true;
	for (unsigned int j = 0; j < sizeof(levelActive); j++)
		child->levelActive[j] = levelActive[j];
	child->aliusID = aliusID;
	TEXTUS_STRCPY(child->alius, alius);

	child->instance_last_id = instance_last_id;
	child->instance_id = (*instance_last_id)++;

	return  (Amor*) child;
*/
	return 0;
}

Aptus *Jor::clone_p(Aptus *animus) {
	TusLogger *o = reinterpret_cast<TusLogger*>(animus->owner);
	o->instance_id = instance_last_id++;
	//o->which_jor = pthread_self(); //should from sched, to do
	o->which_jor = 0;
	printf ("-f:owner %p ---- owner %p\n", owner, animus->owner);
	return (Aptus*)this->clone();
}

bool Jor::sponte ( Amor::Pius *pius)
{
	struct TusLogger::PiDat *pdat;
	pdat = (struct TusLogger::PiDat *)pius;
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
	char millstr[16];

	char levelstr[16];
	LogLevel level;
	char *log_ptr;
	size_t log_size;

	char log_str_buf[LOG_BUF_SIZE];	
	char log_tmp_str_buf[LOG_BUF_TMP_SIZE];	
	char *log_tmp_ptr = 0;
	char *sp_format;
	va_list *h_va;
	Amor::Pius log_pius;    /* for out log */
	void *indic_arr[3];

	log_pius.ordo = 0;
	indic_arr[0] = &aliusID;
	indic_arr[1] = &log_str_buf[0];
	indic_arr[2] = 0;
	log_pius.indic = &indic_arr[0];	
	switch (pius->ordo)
	{
#define LOGWHAT(X) case Notitia::LOG_##X: \
		case Notitia::LOG_VAR_##X: \
			TEXTUS_STRCPY(levelstr, #X); \
			level = X; \
			log_pius.ordo = Notitia::FAC_LOG_##X; \
		break;
		LOGWHAT(EMERG)	LOGWHAT(ALERT)	LOGWHAT(CRIT)
		LOGWHAT(INFO)	LOGWHAT(DEBUG)	LOGWHAT(NOTICE)
		LOGWHAT(ERR)	LOGWHAT(WARNING)
#undef LOGWHAT		
 	default:
		return false;
	}
	
	if (!channels[0] ) return true;	/* 没有输出的渠道, 返回 */
	if ( !levelActive[level] ) /* 此level未设置 */
		return true;
	
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
//lastSec is critical
#if defined (MULTI_PTHREAD) 
	Do_Spin_Lock(time_tick_spin)
#endif
	if ( lastSec == now.time && lastMilli == now.millitm)
	{
		serialNo++;
	} else
	{
		lastSec = now.time;
		lastMilli = now.millitm;
		serialNo = 0;
	}

	if ( serialNo == 0 )
		TEXTUS_SPRINTF(millstr, ".%03d", now.millitm);
	else 
		TEXTUS_SPRINTF(millstr, ".%03d.%d", now.millitm,serialNo);
//here end
#if defined (MULTI_PTHREAD) 
	Do_Spin_UnLock(time_tick_spin)
#endif
	TEXTUS_STRCPY(ftmstr,"%y-%m-%d %H:%M:%S");
	strftime(timestr, 64, ftmstr, tdatePtr);
	TEXTUS_STRCAT(timestr, millstr);

	if ( detect_modified ) 
	{
		log_ptr = &log_str_buf[10];
		log_size = sizeof(log_str_buf)-1-10;
	} else {
		log_ptr = &log_str_buf[0];
		log_size = sizeof(log_str_buf)-1;
	}

	switch (pius->ordo)
	{
		case Notitia::LOG_EMERG:
		case Notitia::LOG_ALERT:
		case Notitia::LOG_CRIT:
		case Notitia::LOG_ERR:
		case Notitia::LOG_WARNING:
		case Notitia::LOG_NOTICE:
		case Notitia::LOG_INFO:
			sp_format = 0;
			h_va = pdat->h_va;
			sp_format = va_arg(*h_va, char* );
			TEXTUS_VSNPRINTF(log_tmp_str_buf, LOG_BUF_TMP_SIZE, sp_format, *h_va); 
			log_tmp_ptr = &log_tmp_str_buf[0];
			break;
		case Notitia::LOG_DEBUG:
			//log_tmp_ptr = (char*)pius->indic;
			log_tmp_ptr = pdat->msg;
			break;
		default:
			break;
	}

	if ( aliusID >=0 ) 
		TEXTUS_SNPRINTF(log_ptr, log_size, "<%d>%s %s%s%s", 
			level+aliusID*8, timestr, hostname, alius, log_tmp_ptr);
	 else {

		if ( hasPid )
			TEXTUS_SNPRINTF(log_ptr, log_size, "(%d)<%s>%s %s(%d, %p, %d)%s%s", aliusID,
				levelstr, timestr, hostname, pdat->instance_id, pdat->me, GETPID, alius, log_tmp_ptr);
		else 
			TEXTUS_SNPRINTF(log_ptr, log_size, "(%d)<%s>%s %s(%d,%p)%s%s", aliusID,
				levelstr, timestr, hostname, pdat->instance_id, pdat->me, alius, log_tmp_ptr);
	}

	if ( detect_modified ) 
	{	/*生成MD5校验值, 若篡改日志内容, 就可发现 */
		BTool::MD5_CTX Md5Ctx;
		unsigned char md[16];
		char md_sum[16];
		int i,l;
		
		l = (int)strlen(log_ptr);

		BTool::MD5Init (&Md5Ctx);
		BTool::MD5Update (&Md5Ctx, last_md_sum, 8);
		BTool::MD5Update (&Md5Ctx, log_ptr, l);
		BTool::MD5Update (&Md5Ctx, md_magic, md_magic_len);
		BTool::MD5Final ((char *) &md[0], &Md5Ctx);
		for (i = 0; i < 4; i++)
		{
			TEXTUS_SNPRINTF (&md_sum[i * 2], 3, "%02x", md[i]);
		}
		memcpy (last_md_sum, md_sum, 8);
		log_str_buf[0] = '[';
		log_str_buf[9] = ']';
		memcpy(&log_str_buf[1], md_sum, 8);
	}

	for ( int j = 0 ; channels[j]; j++)
		channels[j]->dextra(&log_pius, 0); /* 输出日志 */	

	return true;
}

#define TEXTUS_APTUS_TAG { 'J', 'o', 'r', 0};
#include "hook.c"

