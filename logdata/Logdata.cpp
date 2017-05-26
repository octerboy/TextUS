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
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <stdio.h>
#include <stdarg.h>

#if defined(_WIN32)
#include <process.h>
#define GETPID _getpid()
#else
#include <unistd.h>
#define GETPID getpid()
#endif

#define LOG_BUF_SIZE 1280	/* ��־����, 1280�ַ�һ�У� Ӧ�ò��ٰ� */
#define LOG_BUF_TMP_SIZE 1024
class Logdata: public Aptus {
public:
	void ignite_t (TiXmlElement *wood, TiXmlElement *);
	Amor *clone();

	bool sponte_n ( Amor::Pius *, unsigned int from);

	Logdata();
	~Logdata();
	
protected:
	static bool hasPid;		/* �Ƿ�Ҫ�ǽ��̵�pid */
	static bool detect_modified;		/* �Ƿ���۸����� */
	static char hostname[64];	/* ���������� logdata ��� Attachment Ԫ����������˵�� */
	static time_t lastSec;		/* �������� */
	static unsigned short lastMilli;/* ���ĺ����� */
	static int serialNo;		/* �������������ͬ����������к� */
	static char last_md_sum[8];	/* ��һ����־��MD5ֵ */
	static char md_magic[64];	/*Ĭ����MD_MAGIC */
	static int md_magic_len;
	
	int aliusID;		/* ÿһ��ģ���Ψһ��־��Ĭ��Ϊ -1 */
	char alius[128];	/* ÿ��ģ��ı��� */
	int instance_id;	/* ÿ��ģ���ʵ���� */
	int *instance_last_id;	/* ���ʵ����ָ�룬����ʵ������ */
	
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
#include "md5.h"
#define CHNNL_NUM 0

char Logdata::hostname[] = "";
time_t Logdata::lastSec = 0;
bool Logdata::hasPid = false;
unsigned short Logdata::lastMilli = 0;
int Logdata::serialNo = 0;
Aptus** Logdata::channels = (Aptus **)0;
int Logdata::chnnls_num = CHNNL_NUM;

char Logdata::last_md_sum[] = {0};
bool Logdata::detect_modified = false;
char Logdata::md_magic[] = {0};
int Logdata::md_magic_len = 0;
#define MD_MAGIC "1E19A89ADAFGAD"  // ÿ����־���ټ�һ�����ݣ��ټ�MD5���Լ�ʵ�ֵķ��ġ�

Logdata::Logdata()
{
	aliusID = -1;
	memset(alius, 0, sizeof(alius));
	
	instance_id = 0;
	instance_last_id = (int*) 0;
	isPioneer = false;
	if ( last_md_sum[0] == 0 ) 	//�Դ��ж�, ����������, �����ʼΪ"00000000"
	{
		memset(last_md_sum, '0', sizeof(last_md_sum));
	}
}

Logdata::~Logdata() { 
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
	if ( isPioneer && instance_last_id )
		delete instance_last_id;
}

void Logdata::ignite_t (TiXmlElement *cfg, TiXmlElement *log_ele)
{	/* ��cfg��ȡhostname����, aliusID�ȴ�carbo��� */
	const char *host_str, *comm_str;
	TiXmlElement *level_ele;
		
	WBUG("this %p , prius %p, aptus %p, cfg %p", this, prius, aptus, cfg);
	if ( !log_ele) return;

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
	md_magic_len = strlen(md_magic);

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

	if ( !instance_last_id )
	{
		instance_last_id = new int;
		*instance_last_id = 1;
	}
	isPioneer = true;
}

Amor *Logdata::clone()
{	
	Logdata *child = 0;
	child = new Logdata();
	Aptus::inherit( (Aptus*) child);
	for (unsigned int j = 0; j < sizeof(levelActive); j++)
		child->levelActive[j] = levelActive[j];
	child->aliusID = aliusID;
	TEXTUS_STRCPY(child->alius, alius);

	child->instance_last_id = instance_last_id;
	child->instance_id = (*instance_last_id)++;

	return  (Amor*) child;
}

bool Logdata::sponte_n ( Amor::Pius *pius, unsigned int from)
{	/* ��owner���󷢳�����ǰ�Ĵ��� */
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
		case Notitia::LOG_VAR_EMERG:
		case Notitia::LOG_VAR_ALERT:
		case Notitia::LOG_VAR_CRIT:
		case Notitia::LOG_VAR_ERR:
		case Notitia::LOG_VAR_WARNING:
		case Notitia::LOG_VAR_NOTICE:
		case Notitia::LOG_VAR_INFO:
		case Notitia::LOG_VAR_DEBUG:
			sp_format = 0;
			h_va = (va_list*)pius->indic;
			sp_format = va_arg(*h_va, char* );
			TEXTUS_VSNPRINTF(log_tmp_str_buf, LOG_BUF_TMP_SIZE, sp_format, *h_va); 
			log_tmp_ptr = &log_tmp_str_buf[0];
			break;
		case Notitia::LOG_EMERG:
		case Notitia::LOG_ALERT:
		case Notitia::LOG_CRIT:
		case Notitia::LOG_ERR:
		case Notitia::LOG_WARNING:
		case Notitia::LOG_NOTICE:
		case Notitia::LOG_INFO:
		case Notitia::LOG_DEBUG:
			log_tmp_ptr = (char*)pius->indic;
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
				levelstr, timestr, hostname, instance_id, owner, GETPID, alius, log_tmp_ptr);
		else 
			TEXTUS_SNPRINTF(log_ptr, log_size, "(%d)<%s>%s %s(%d,%p)%s%s", aliusID,
				levelstr, timestr, hostname, instance_id, owner, alius, log_tmp_ptr);
	}

	if ( detect_modified ) 
	{	/*����MD5У��ֵ, ���۸���־����, �Ϳɷ��� */
		MD5_CTX Md5Ctx;
		unsigned char md[16];
		char md_sum[16];
		int i,l;
		
		l = strlen(log_ptr);

		MD5Init (&Md5Ctx);
		MD5Update (&Md5Ctx, last_md_sum, 8);
		MD5Update (&Md5Ctx, log_ptr, l);
		MD5Update (&Md5Ctx, md_magic, md_magic_len);
		MD5Final ((char *) &md[0], &Md5Ctx);
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
#define TEXTUS_APTUS_TAG { 'L', 'o', 'g', 0};
#include "hook.c"

