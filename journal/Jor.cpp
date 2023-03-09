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

#define LOG_BUF_SIZE 1280	/* ��־����, 1280�ַ�һ�У� Ӧ�ò��ٰ� */
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
	static bool hasPid;		/* �Ƿ�Ҫ�ǽ��̵�pid */
	static bool detect_modified;		/* �Ƿ���۸����� */
	static char hostname[64];	/* ���������� logdata ��� Attachment Ԫ����������˵�� */
	static time_t lastSec;		/* �������� */
	static unsigned short lastMilli;/* ���ĺ����� */
	static int serialNo;		/* �������������ͬ����������к� */
	static char last_md_sum[8];	/* ��һ����־��MD5ֵ */
	static char md_magic[64];	/*Ĭ����MD_MAGIC */
	static short md_magic_len;
	
	int aliusID;		/* ÿһ��ģ���Ψһ��־��Ĭ��Ϊ -1 */
	char alius[128];	/* ÿ��ģ��ı��� */
	unsigned int instance_id;	/* ÿ��ģ���ʵ���� */
	unsigned int instance_last_id;	/*  ���ʵ���� */
	
	static Aptus **channels;	/* ��־������� */
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
#define MD_MAGIC "1E19A89ADAFGAD"  // ÿ����־���ټ�һ�����ݣ��ټ�MD5���Լ�ʵ�ֵķ��ġ�

Jor::Jor()
{
	aliusID = -1;
	memset(alius, 0, sizeof(alius));
	
	instance_id = 0;
	instance_last_id = 0;
	isPioneer = false;
	if ( last_md_sum[0] == 0 ) 	//�Դ��ж�, ����������, �����ʼΪ"00000000"
	{
		memset(last_md_sum, '0', sizeof(last_md_sum));
	}
}

Jor::~Jor() { 
	/* �������ͷ�, owner->aptusҲҪ (���Ѿ����ͷ�), 
	���ԣ�������owner�Ǽ�¼��־��channel��
	��Ҫ��֮����ȥ�� */

	for ( int j = 0 ; channels[j]; j++)
	{
		if ( owner->aptus == channels[j] )
		{
			for ( int i = j ; channels[i]; i++)
			{
				channels[i] = channels[i+1]; /* ���һ��Ϊnull */
			}
		}
	}

/*
	if ( !channels[0] )	
		delete[] channels;
*/
}

void Jor::ignite_t (TiXmlElement *cfg, TiXmlElement *log_ele)
{	/* ��cfg��ȡhostname����, aliusID�ȴ�carbo��� */
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
		channels = new Aptus*[chnnls_num+1]; /* ��nullָ����Ϊ�������ָ����Ľ�β��־,��Ҫ���һ�� */
		memset( channels, 0, (chnnls_num+1) * sizeof(Amor *));
	}
	
	canAccessed = true;	/* ���˿�����Ϊ��ģ����Ҫ��־ */
	need_spo = true;

	/* ȡ�ñ��� */
	if ( (comm_str = log_ele->Attribute("alias")) )
	{
		TEXTUS_STRNCPY(alius, comm_str, sizeof(alius)-2);
		TEXTUS_STRCAT(alius, " ");
	}

	/* ȡ��ID */
	if ( (comm_str =log_ele->Attribute("ID")) )
		aliusID = atoi( comm_str);

	/* ȡ��������� */
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

	/* Ĭ��������־�ȼ����� */
	for (unsigned int j = 0; j < sizeof(levelActive); j++)
		levelActive[j] = true;

	level_ele = log_ele->FirstChildElement("exclude");
	while(level_ele)
	{
		comm_str = level_ele->Attribute("level");
		if (comm_str) 
		{	/* ɨ�費Ҫ�ǵ���־ */
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
	
	if (!channels[0] ) return true;	/* û�����������, ���� */
	if ( !levelActive[level] ) /* ��levelδ���� */
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
	{	/*����MD5У��ֵ, ���۸���־����, �Ϳɷ��� */
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
		channels[j]->dextra(&log_pius, 0); /* �����־ */	

	return true;
}

#define TEXTUS_APTUS_TAG { 'J', 'o', 'r', 0};
#include "hook.c"

