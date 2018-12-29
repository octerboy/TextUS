/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Select Descriptors
 Build: created by octerboy, 2005/06/10
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#define FD_SETSIZE 1024
#if defined(_WIN32)
#include <winsock2.h>
#endif

#include "Describo.h"
#include "DPoll.h"
#include "Amor.h"
#include "Notitia.h"
#include "textus_string.h"
#include <stdarg.h>
#if !defined(_WIN32)
#include <sys/types.h>
#include <unistd.h>
	#if !defined(__linux__) && !defined(_AIX)
	#include <sys/times.h>
	#include <sys/select.h>
	#endif
#endif
#if defined(__linux__)
#endif	//for linux

#if defined(__sun)
#include <port.h>
#include <signal.h>
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
#endif	//for bsd

#include <sys/timeb.h>
#include <time.h>
#include <errno.h>
#include <assert.h>

#if defined (_WIN32 )
#define ERROR_PRO(X) { \
	char *s; \
	char error_string[1024]; \
	DWORD dw = GetLastError(); \
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw, \
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) error_string, 1024, NULL );\
	s= strstr(error_string, "\r\n") ; \
	if (s )  *s = '\0';  \
	if ( errMsg ) \
		TEXTUS_SNPRINTF(errMsg, errstr_len, "%s errno %d, %s", X,dw, error_string);\
	}
#else
#define ERROR_PRO(X)  if ( errMsg ) \
		TEXTUS_SNPRINTF(errMsg, errstr_len, "%s errno %d, %s.", X, errno, strerror(errno));
#endif

#define TOR_SIZE FD_SETSIZE

class TPoll: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor* clone();

	TPoll();
#if defined (_WIN32)
	HANDLE iocp_port,timer_queue;
#endif
#if defined(__sun)
	int ev_port;
	port_notify_t		pnotif;
	struct sigevent		sigev;
	timespec_t		timeout;
#endif	//for sun

private:
	char errMsg[2048];
	int errstr_len;
	struct timeb t_now;
	Amor::Pius timer_pius,tm_hd_ps, poll_ps;

	int number_threads;
	int maxfd;
	int timer_milli; /* 定时器毫秒数 */
	int timer_usec;	/* 轮询间隔微秒数 */
	int timer_sec;	/* 轮询间隔秒数 */
	int timer_nsec;	/* 轮询间隔纳秒数 */

	void run();
	bool shouldEnd;

	struct Describo::Pendor *pendors;	//延后调用的数组；
	int pendor_size;
	int pendor_top;
	void run_pendors();

	struct Timor : DPoll::PollorBase {
#if defined (_WIN32)
		HANDLE timer_hnd;
#endif
#if defined(__sun)
		timer_t timerid;
#endif	//for sun

		inline Timor() {
			pupa = 0;
			type = DPoll::NotUsed ;
		}
	};
	struct Timor *tpool;/* 要求给予定时信号的数组 */
	struct Timor **timer_infor;/* 要求给予定时信号的指针数组 */
	int infor_size;		/* timer_infor尺寸 */
	int stack_top;		//
	inline void timor_init()
	{
		int i;
		if (stack_top < 0 ) 
		{
			timer_infor = new struct Timor* [infor_size];
			tpool = new struct Timor [infor_size];
			stack_top = infor_size;
			for ( i = 0 ; i < stack_top; i++)
				timer_infor[i] = &tpool[i];
			stack_top = infor_size;
		}
	}
	inline struct Timor* get_timor ()
	{
		int i;
		stack_top--;
		if ( -1 == stack_top ) 	/* 空间不够，扩张之 */
		{	
			delete[] timer_infor;
			delete[] tpool;
			timer_infor = new struct Timor* [infor_size*2];	/* 分配2倍的空间 */
			tpool = new struct Timor [infor_size];
			stack_top = infor_size;
			for ( i = 0 ; i < stack_top; i++)
				timer_infor[i] = &tpool[i];
			infor_size = infor_size*2;	/* 尺寸值加倍 */
		}
		return timer_infor[stack_top];
	};

	inline void put_timor(struct Timor* aor)
	{
		timer_infor[stack_top] = aor;
		stack_top++;
	};
#include "wlog.h"
};

#define DEFAULT_TIMER_MILLI 1000
static TPoll *g_poll = 0;
#if defined (_WIN32)
VOID CALLBACK timer_routine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	PostQueuedCompletionStatus(g_poll->iocp_port, 0, (ULONG_PTR)lpParam, 0);
}
#endif

void TPoll::ignite(TiXmlElement *cfg)
{
	const char *timer_str;

	number_threads= 2;
	cfg->QueryIntAttribute("iocp_thread_num", &number_threads);
	timer_milli = DEFAULT_TIMER_MILLI;

	if( (timer_str = cfg->Attribute("timer")) && atoi(timer_str) > 1 )
		timer_milli = atoi(timer_str); //time_str为毫秒数

	cfg->QueryIntAttribute("ponder", &(pendor_size));
	pendors = new struct Describo::Pendor[pendor_size];

#if defined (_WIN32)
	iocp_port = NULL;
	iocp_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, number_threads); 
	if (iocp_port == NULL)  
	{
		ERROR_PRO("CreateIoCompletionPort failed for a new port");
		return ;
	}
	timer_queue = CreateTimerQueue();
	if (NULL == timer_queue)
	{
		ERROR_PRO("CreateTimerQueue failed");
		return ;
	}
#endif 
#if defined(__sun)
	ev_port = port_create();
	if ( ev_port < 0 ) 
	{
		ERROR_PRO("port_create failed for a new port");
		return ;
	}
#endif	//for sun

	infor_size = 128;
	cfg->QueryIntAttribute("timer_num", &infor_size);
	timor_init();	//初始化

	timer_sec = timer_milli/1000;
	timer_usec = (timer_milli % 1000) * 1000;
	timer_nsec = (timer_milli % 1000) * 1000000;
	pnotif.portnfy_port = ev_port;
	pnotif.portnfy_user = (void *)0;
	sigev.sigev_notify = SIGEV_PORT;
	sigev.sigev_value.sival_ptr = &pnotif;
}

bool TPoll::sponte( Amor::Pius *apius)
{
	int i, tmp_type = 0;
	unsigned interval =0, interval2;
	Amor *ask_pu = (Amor *) 0;
	void **p;

	DPoll::Pollor *ppo; 
#if defined (_WIN32)
	HANDLE hPort = NULL;
	HANDLE hTimer = NULL;
	HANDLE port_hnd = NULL;
#endif
#if defined(__sun)
	int events;
	itimerspec_t	itimeout;
#endif
	struct Timor *aor;
	assert(apius);

	switch ( apius->ordo )
	{
	case Notitia::SET_EPOLL :	/* IOCP,epoll  */
		ppo = (DPoll::Pollor *)apius->indic;	
		assert(ppo);
		WBUG("%p %s", ppo->pupa, "sponte SET_EPOLL");
		switch (ppo->type ) {
#if defined (_WIN32)
		case DPoll::WinFile:
			port_hnd = ppo->hnd.file;
			break;
		case DPoll::WinSock:
			port_hnd = (HANDLE)ppo->hnd.sock;
			break;
#endif
		case DPoll::Writable:
#if defined(__sun)
		if ( !port_associate(ev_port, PORT_SOURCE_FD, ppo->fd, POLLOUT, ppo) )
		{
			ERROR_PRO("port_associate(POLLOUT) failed");
			WLOG(WARNING, errMsg);
		}
		
#endif
			break;
		case DPoll::Readable:
#if defined(__sun)
		if ( !port_associate(ev_port, PORT_SOURCE_FD, ppo->fd, POLLIN, ppo) )
		{
			ERROR_PRO("port_associate(POLLIN) failed");
			WLOG(WARNING, errMsg);
		}
#endif
			break;
		default:
			break;
		}

#if defined (_WIN32)
		if ( !SetFileCompletionNotificationModes(port_hnd, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS) )
		{
			ERROR_PRO("SetFileCompletionNotificationModes failed");
			WLOG(WARNING, errMsg);
		}
		hPort = CreateIoCompletionPort(port_hnd, iocp_port, (ULONG_PTR)ppo /* completion key */, number_threads);  
		if (hPort == NULL)  
		{ 
			ERROR_PRO("CreateIoCompletionPort failed to associate");
			WLOG(WARNING, errMsg);
		}
#endif
		break;

	case Notitia::DMD_SET_TIMER :	/* 置时间片通知对象 */
		WBUG("%p sponte DMD_SET_TIMER",  apius->indic);
		aor = get_timor();
#if defined(__sun)
		/* Setup the port notification structure */
		pnotif.portnfy_user = (void *)aor;
		/* Create a timer using the realtime clock */
		if (!timer_create(CLOCK_REALTIME, &sigev, &aor->timerid))
		{
			ERROR_PRO("timer_create failed");
			WLOG(WARNING, errMsg);
			goto END_TIMER_PRO;
		}
		itimeout.it_value.tv_sec = timer_sec;
		itimeout.it_value.tv_nsec = timer_nsec;
		itimeout.it_interval.tv_sec = timer_sec;
		itimeout.it_interval.tv_nsec = timer_nsec;

		if (!timer_settime(&aor->timerid, 0, &itimeout, NULL)}
		{
			ERROR_PRO("timer_settime");
			WLOG(WARNING, errMsg);
			goto END_TIMER_PRO;
		}

#endif
#if defined (_WIN32)
		if (!CreateTimerQueueTimer( &aor->timer_hnd, timer_queue, (WAITORTIMERCALLBACK)timer_routine, 
						aor, timer_milli, timer_milli, WT_EXECUTEINTIMERTHREAD))
		{
			ERROR_PRO("CreateTimerQueueTimer (SET_TIMER) failed");
			WLOG(WARNING, errMsg);
			goto END_TIMER_PRO;
		} 
#endif
		tm_hd_ps.indic = aor;
		aor->pupa = ask_pu;
		aor->type = DPoll::Timer;
		aor->pupa->facio(&tm_hd_ps);
		break;

END_TIMER_PRO:
			put_timor(aor);//发生错误而回收
		break;

	case Notitia::DMD_SET_ALARM :	/* 置超时通知对象 */
		p = (void**) (apius->indic);
		ask_pu = (Amor*) (*p);
		p++;
		interval = *((int *)(*p));
		p++;
		if ( *p )
			interval2 = *((int *)(*p));
		else
			interval2 = 0;
		WBUG("%p sponte DMD_SET_ALARM, interval: %d", ask_pu, interval);
		aor = get_timor();
		if ( !aor ) break;
#if defined(__sun)
		/* Setup the port notification structure */
		pnotif.portnfy_user = (void *)aor;
		/* Create a timer using the realtime clock */
		if (!timer_create(CLOCK_REALTIME, &sigev, &aor->timerid))
		{
			ERROR_PRO("timer_create failed");
			WLOG(WARNING, errMsg);
			goto END_ALARM_PRO;
		}
		itimeout.it_value.tv_sec = interval/1000;
		itimeout.it_value.tv_nsec = (interval%1000)*1000;
		itimeout.it_interval.tv_sec = interval2/1000;
		itimeout.it_interval.tv_nsec = (interval2%1000)*1000;

		if (!timer_settime(&aor->timerid, 0, &itimeout, NULL)}
		{
			ERROR_PRO("timer_settime");
			WLOG(WARNING, errMsg);
			goto END_ALARM_PRO;
		}
#endif

#if defined (_WIN32)
		if (!CreateTimerQueueTimer( &aor->timer_hnd, timer_queue, (WAITORTIMERCALLBACK)timer_routine, 
						aor, interval, interval2, WT_EXECUTEINTIMERTHREAD))
		{
			ERROR_PRO("CreateTimerQueueTimer (DMD_SET_ALARM) failed ");
			WLOG(WARNING, errMsg);
			goto END_ALARM_PRO;
		}
#endif
		tm_hd_ps.indic = aor;
		aor->pupa = ask_pu;
		if ( interval2 > 0 ) 
			aor->type = DPoll::Timer;
		else
			aor->type = DPoll::Alarm;
		aor->pupa->facio(&tm_hd_ps);
		break;

END_ALARM_PRO:
			put_timor(aor); //发生错误而回收
		break;

	case Notitia::DMD_CLR_TIMER :	/* 清定时通知对象 */
		WBUG("%p sponte DMD_CLR_TIMER", apius->indic);
		aor = (struct Timor *)tm_hd_ps.indic;
		if ( !aor ) break;
#if defined (_WIN32)
		if ( !DeleteTimerQueueTimer(timer_queue, aor->timer_hnd, INVALID_HANDLE_VALUE) )
		{
			ERROR_PRO("CreateTimerQueueTimer (DMD_SET_ALARM) failed ");
			WLOG(WARNING, errMsg);
		}
#endif
		tm_hd_ps.indic = 0;
		aor->pupa->facio(&tm_hd_ps);
		aor->pupa = 0;
#if defined (_WIN32)
		aor->timer_hnd = NULL;
#endif
		aor->type = DPoll::NotUsed;
		put_timor(aor);
		break;

	case Notitia::CMD_MAIN_EXIT :	/* 终止程序 */
		WBUG("CMD_MAIN_EXIT");
		shouldEnd = true;
		break;

	case Notitia::CMD_GET_SCHED:	/* 取得本对象地址 */
		WBUG("CMD_GET_SCHED this = %p", this);
		apius->indic = this;
		break;

	case Notitia::CMD_PUT_PENDOR:	/* 设置需要调度的对象 */
#define POR ((struct Describo::Pendor*)apius->indic)
		WBUG("CMD_PUT_PENDOR pupa=%p, dir=%d, from=%d, pius=%p", POR->pupa, POR->dir, POR->from, POR->pius);
		for ( i = 0 ; i < pendor_size; i++ )
		{
			if ( pendors[i].pupa == 0 )
			{
				pendors[i].pupa = POR->pupa;
				pendors[i].dir = POR->dir;
				pendors[i].from = POR->from;
				pendors[i].pius = POR->pius;
				if ( i > pendor_top ) pendor_top = i;
				break;
			}
		}
		break;

	default:
		return false;
	}
	return true;
}

bool TPoll::facio( Amor::Pius *pius)
{
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");		
#if defined (_WIN32)
		if (iocp_port == NULL)  
		{ 
			WLOG(ERR, errMsg);
		}
#endif
		break;

	case Notitia::WINMAIN_PARA:	/* 在整个系统中, 这应是最后被通知到的。 */
		WBUG("facio Notitia::WINMAIN_PARA");
		goto MainPro;
		break;

	case Notitia::MAIN_PARA:	/* 在整个系统中, 这应是最后被通知到的。 */
		WBUG("facio Notitia::MAIN_PARA");
MainPro:
		run();
		break;

	default:
		return false;
	}
	return true;
}

Amor* TPoll::clone()
{
	return (Amor*)this;
}

TPoll::TPoll()
{
	maxfd = -1;
	timer_usec = 50*1000;	//默认50毫秒
	timer_sec = 0;	//默认50毫秒

	shouldEnd = false;
	pendors = 0;
	pendor_top = -1;
	pendor_size = 16;

	errstr_len= sizeof(errMsg)-1;
	g_poll = this;
	tm_hd_ps.ordo = Notitia::TIMER_HANDLE;
	timer_pius.ordo = Notitia::TIMER;
	timer_pius.indic = 0;

	infor_size = 0;
	timer_infor = 0;
	stack_top = -1;
}


void TPoll::run_pendors()
{
	Amor::Pius aps;
	struct Describo::Pendor pen;
	int i;
	i = pendor_top ; 
	for ( ; i > -1; i-- )
	{
		if ( pendors[i].pupa != 0 )
		{
			aps.ordo = Notitia::DMD_SCHED_RUN;
			
			aps.indic = &pen;
			pen.pupa = pendors[i].pupa;
			pen.dir = pendors[i].dir;
			pen.from = pendors[i].from;
			pen.pius = pendors[i].pius;
			pendors[i].pupa = 0;
			pendor_top = i-1;
			WBUG("run pendor pupa=%p, dir=%d, from=%d, pius=%p", pen.pupa, pen.dir, pen.from, pen.pius);
			pen.pupa->facio(&aps);
		}
	}
}

/* 这是唯一个不返回的函数  */
void TPoll:: run()
{

	
	struct DPoll::PollorBase *aor=0;
	struct Timor *tor=0;
	struct DPoll::Pollor *ppo=0; 
	Amor *pupa;
#if defined(__sun)
	int events;
	port_event_t		pev;
#endif

LOOP:
	if ( pendor_top > -1 ) run_pendors();
#if defined(__sun)
	nready = port_get(port, &pev, 0);
	if ( nready != 0 ) 
	{
		ERROR_PRO("GetQueuedCompletionStatus");
		WLOG(ERR,errMsg);
		goto LOOP;
	}
#endif

#if defined(_WIN32)
	BOOL success;
	DWORD dwNoOfBytes = 0;  
	//ULONG_PTR ulKey = 0;  
	OVERLAPPED* pov = NULL; 
	if (!iocp_port) return;
	WBUG("GetQueuedCompletionStatus %d", timer_milli);
	success = GetQueuedCompletionStatus(iocp_port,         // Completion port handle  
			&dwNoOfBytes,  // Bytes transferred  
			(PULONG_PTR)&aor,  
			&pov,          // OVERLAPPED structure  
			INFINITE       // for ever
                    );  
	if ( aor== NULL)  
	{   // An unrecoverable error occurred in the completion port. Wait for the next notification. 
		DWORD nError = GetLastError();
		if(nError == ERROR_ABANDONED_WAIT_0)	//fd closed
		{
			ERROR_PRO("GetQueuedCompletionStatus");
			WLOG(INFO,errMsg);
		} else if(nError != WAIT_TIMEOUT)	//TIME OUT
		{
			ERROR_PRO("GetQueuedCompletionStatus");
			WLOG(ERR,errMsg);
		}
		goto LOOP;  
	}
#endif

	switch (aor->type ) 
	{
	case DPoll::Alarm:
		tor = (struct Timor *)aor;
		pupa = tor->pupa;
		tor->type = DPoll::NotUsed;
		tor->pupa = 0;
		put_timor(tor);
		tm_hd_ps.indic = 0;
		pupa->facio(&tm_hd_ps);		//clear timer_handle 
		pupa->facio(&timer_pius);	//TIMER
		break;

	case DPoll::Timer:
		tor = (struct Timor *)aor;
		tor->pupa->facio(&timer_pius);
		break;

#if defined (_WIN32)
	case DPoll::WinFile:
	case DPoll::WinSock:
		ppo=(struct DPoll::Pollor *)aor; 
		ppo->overlap = pov;
		ppo->num_of_trans = dwNoOfBytes;
		poll_ps.indic = ppo;
		poll_ps.ordo = success ? Notitia::PRO_EPOLL : Notitia::ERR_EPOLL;
		ppo->pupa->facio(&poll_ps);
		break;
#endif
	default:
		break;
	}

	if ( !shouldEnd) goto LOOP;
}
#include "hook.c"

