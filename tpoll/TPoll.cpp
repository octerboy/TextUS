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
#include <sys/timerfd.h>
#endif
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
#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
	int kq;
#endif	//for bsd

#if defined(__linux__)
	int epfd;
	struct itimerspec	itimeout;
#endif	//for linux

#if defined(__sun)
	int ev_port;
	port_notify_t		pnotif;
	struct sigevent		sigev;
	struct itimerspec	itimeout;
#endif	//for sun

private:
	char errMsg[2048];
	int errstr_len;
	struct timeb t_now;
	Amor::Pius timer_pius,tm_hd_ps, poll_ps;

	int number_threads;
	int max_evs;
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

	bool init_ok;

	struct Timor : DPoll::PollorBase {
#if defined(__linux__)
		int fd;
		struct epoll_event ev;
#endif	//for linux

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
		struct kevent events[1];
#endif

#if defined(__sun)
		timer_t timerid;
#endif	//for sun
#if defined (_WIN32)
		HANDLE timer_hnd;
#endif
		inline Timor() {
			pupa = 0;
			type = DPoll::NotUsed ;
#if defined(__linux__)
			ev.data.ptr=this;
			ev.events = EPOLLIN;
#endif
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
			for ( i = 0 ; i < stack_top; i++) {
				timer_infor[i] = &tpool[i];
#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
				EV_SET(&(tpool[i].events[0]), i, EVFILT_TIMER, EV_ADD, 0, 0,timer_infor[i]);
#endif
			}
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
			for ( i = 0 ; i < stack_top; i++) {
				timer_infor[i] = &tpool[i];
#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
				 EV_SET(&(tpool[i].events[0]), i+infor_size, EVFILT_TIMER, EV_ADD, 0, 0,timer_infor[i]);
#endif
			}
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

	cfg->QueryIntAttribute("maxevents", &max_evs);
	infor_size = 128;
	cfg->QueryIntAttribute("timer_num", &infor_size);

	timer_sec = timer_milli/1000;
	timer_usec = (timer_milli % 1000) * 1000;
	timer_nsec = (timer_milli % 1000) * 1000000;
#if defined (_WIN32)
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
	pnotif.portnfy_port = ev_port;
	pnotif.portnfy_user = (void *)0;
	sigev.sigev_notify = SIGEV_PORT;
	sigev.sigev_value.sival_ptr = &pnotif;
	itimeout.it_value.tv_sec = timer_sec;
	itimeout.it_value.tv_nsec = timer_nsec;
	itimeout.it_interval.tv_sec = timer_sec;
	itimeout.it_interval.tv_nsec = timer_nsec;
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
	kq = kqueue();
	if ( kq == -1 ) 
	{
		ERROR_PRO("kqueue failed for an event queue");
		return ;
	}
#endif	//for bsd

#if defined(__linux__)
	epfd = epoll_create1(EPOLL_CLOEXEC);
	if ( epfd == -1 ) 
	{
		ERROR_PRO("epoll_create1(EPOLL_CLOEXEC) failed for an new descriptor");
		return ;
	}
	itimeout.it_value.tv_sec = timer_sec;
	itimeout.it_value.tv_nsec = timer_nsec;
	itimeout.it_interval.tv_sec = timer_sec;
	itimeout.it_interval.tv_nsec = timer_nsec;
#endif	//for linux

	timor_init();	//初始化
	init_ok = true;
}

bool TPoll::sponte( Amor::Pius *apius)
{
	int i;
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
	int ret;
	struct itimerspec tmp_timeout;
#endif
#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
	uint16_t flg1,flg2;	
#endif
#if defined(__linux__)
	struct itimerspec tmp_timeout;
#endif	//for linux

	struct Timor *aor;
	struct DPoll::PollorBase *baspo=0;
	assert(apius);

	switch ( apius->ordo )
	{
	case Notitia::CLR_EPOLL :	/* clear epoll  */
		ppo = (DPoll::Pollor *)apius->indic;	
		assert(ppo);
		WBUG("%p %s", ppo->pupa, "sponte CLR_EPOLL");
#if defined(__sun)
		if( !port_dissociate(ev_port, PORT_SOURCE_FD, ppo->fd) )
		{
			ERROR_PRO("port_dissociate(PORT_SOURCE_FD) failed");
			WLOG(WARNING, errMsg);
		}
#endif

#if defined(__linux__)
		if( !epoll_ctl(epfd, EPOLL_CTL_DEL, ppo->fd, &ppo->ev) )
		{
			ERROR_PRO("epoll_ctl failed");
			WLOG(WARNING, errMsg);
		}
#endif	//for linux
#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
		flg1 = ppo->events[0].flags ;
		flg2 = ppo->events[1].flags ;
		ppo->events[0].flags = EV_DELETE;
		ppo->events[1].flags = EV_DELETE;
		if ( -1 == kevent(kq, &ppo->events[0], 2, NULL, 0, NULL) ) 
		{
			ERROR_PRO("kevent(CLR_EPOLL) failed");
			WLOG(WARNING, errMsg);
		}
		ppo->events[0].flags = flg1;
		ppo->events[1].flags = flg2;
#endif	//for bsd

		break;
	case Notitia::AIO_EPOLL :	/*  AIO transaction */
		break;

	case Notitia::POST_EPOLL :	/* user post  */
		baspo = (DPoll::PollorBase *)apius->indic;	
		WBUG("%p %s", baspo->pupa, "sponte POST_EPOLL");
		if ( baspo->type == DPoll::NotUsed)
		{
			apius->indic = this;
			break;
		}
		break;

	case Notitia::SET_EPOLL :	/* IOCP,epoll  */
		ppo = (DPoll::Pollor *)apius->indic;	
		assert(ppo);

#if defined(__linux__)
		if( !epoll_ctl(epfd, ppo->op, ppo->fd, &ppo->ev) )
		{
			ERROR_PRO("epoll_ctl failed");
			WLOG(WARNING, errMsg);
		}
#endif	//for linux

#if defined(__sun)
		WBUG("%p %s(fd=%d) events(%d)", ppo->pupa, "sponte SET_EPOLL", ppo->fd, ppo->events );
		ret = port_associate(ev_port, PORT_SOURCE_FD, ppo->fd, ppo->events, ppo);
		if( ret !=0)
		{
			ERROR_PRO("port_associate failed");
			WLOG(WARNING, errMsg);
		}		
#endif

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
		if( kevent(kq, &(ppo->events[0]), 2, NULL, 0, NULL) == - 1 )
		{
			ERROR_PRO("kevent(SET_EPOLL) failed");
			WLOG(WARNING, errMsg);
		}		
#endif	//for bsd

#if defined (_WIN32)
		switch (ppo->type ) {
		case DPoll::File:
			port_hnd = ppo->hnd.file;
			break;
		case DPoll::Sock:
			port_hnd = (HANDLE)ppo->hnd.sock;
			break;
		default:
			break;
		}

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
		aor->pupa = (Amor*) (apius->indic);
		aor->type = DPoll::Timer;
		tm_hd_ps.indic = aor;

#if defined(__linux__)
		aor->fd = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC);
		if( aor->fd == -1)
		{
			ERROR_PRO("timerfd_create failed");
			WLOG(WARNING, errMsg);
			goto END_TIMER_PRO;
		}
		if (timerfd_settime(aor->fd, 0, &itimeout, NULL) == -1) 
		{
			ERROR_PRO("timerfd_settime failed");
			WLOG(WARNING, errMsg);
			close(aor->fd);
			goto END_TIMER_PRO;
		}
		if( !epoll_ctl(epfd, EPOLL_CTL_ADD, aor->fd, &aor->ev) )
		{
			ERROR_PRO("epoll_ctl(SET_TIMER) failed");
			WLOG(WARNING, errMsg);
		}
#endif	//for linux

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
		aor->events[0].data = timer_milli ;
		aor->events[0].flags = EV_ADD ;
		if( kevent(kq, &(aor->events[0]), 1, NULL, 0, NULL) == - 1 )
		{
			ERROR_PRO("kevent(DMD_SET_TIMER) failed");
			WLOG(WARNING, errMsg);
			goto END_TIMER_PRO;
		}		
#endif	//for bsd

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

		if (!timer_settime(aor->timerid, 0, &itimeout, NULL))
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
#if defined(__linux__)
		aor->fd = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC);
		if( aor->fd == -1)
		{
			ERROR_PRO("timerfd_create failed");
			WLOG(WARNING, errMsg);
			goto END_ALARM_PRO;
		}
		tmp_timeout.it_value.tv_sec = interval/1000;
		tmp_timeout.it_value.tv_nsec = (interval%1000)*1000;
		tmp_timeout.it_interval.tv_sec = interval2/1000;
		tmp_timeout.it_interval.tv_nsec = (interval2%1000)*1000;
		if (timerfd_settime(aor->fd, 0, &tmp_timeout, NULL) == -1) 
		{
			ERROR_PRO("timerfd_settime failed");
			WLOG(WARNING, errMsg);
			close(aor->fd);
			goto END_ALARM_PRO;
		}
		if( !epoll_ctl(epfd, EPOLL_CTL_ADD, aor->fd, &aor->ev) )
		{
			ERROR_PRO("epoll_ctl(SET_ALARM) failed");
			WLOG(WARNING, errMsg);
		}
#endif	//for linux
#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
		aor->events[0].data = interval;
		if ( interval2 > 0 ){ 
			aor->events[0].flags = EV_ADD ;
		} else {
			aor->events[0].flags = EV_ADD | EV_ONESHOT ;
		}
		if( kevent(kq, &(aor->events[0]), 1, NULL, 0, NULL) == - 1 )
		{
			ERROR_PRO("kevent(DMD_SET_ALARM) failed");
			WLOG(WARNING, errMsg);
			goto END_ALARM_PRO;
		}		
#endif

#if defined(__sun)
		/* Setup the port notification structure */
		pnotif.portnfy_user = (void *)aor;
		/* Create a timer using the realtime clock */
		if (timer_create(CLOCK_REALTIME, &sigev, &(aor->timerid))!=0)
		{
			ERROR_PRO("timer_create failed");
			WLOG(WARNING, errMsg);
			goto END_ALARM_PRO;
		}
		tmp_timeout.it_value.tv_sec = interval/1000;
		tmp_timeout.it_value.tv_nsec = (interval%1000)*1000;
		tmp_timeout.it_interval.tv_sec = interval2/1000;
		tmp_timeout.it_interval.tv_nsec = (interval2%1000)*1000;

		if (timer_settime (aor->timerid, 0, &tmp_timeout, NULL) != 0 )
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
#if defined(__linux__)
		if ( -1 == close(aor->fd) )
		{
			ERROR_PRO("close(DMD_CLR_TIMER) failed");
			WLOG(WARNING, errMsg);
		}
#endif
#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
		aor->events[0].flags = EV_DELETE ;
		if( kevent(kq, &(aor->events[0]), 1, NULL, 0, NULL) == - 1 )
		{
			ERROR_PRO("kevent(DMD_CLR_TIMER) failed");
			WLOG(WARNING, errMsg);
		}		
#endif

#if defined(__sun)
		/* delete the timer */
		if (timer_delete(aor->timerid)!=0)
		{
			ERROR_PRO("timer_delete(DMD_CLR_TIMER) failed");
			WLOG(WARNING, errMsg);
		}
#endif

#if defined (_WIN32)
		if ( !DeleteTimerQueueTimer(timer_queue, aor->timer_hnd, INVALID_HANDLE_VALUE) )
		{
			ERROR_PRO("CreateTimerQueueTimer(DMD_CLR_TIMER) failed ");
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
		if ( !init_ok )
		{ 
			WLOG(ERR, errMsg);
		}
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
	max_evs = 16;
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
	init_ok = false;

#if defined(_WIN32)
	iocp_port = NULL;
#endif

#if defined(__linux__)
	epfd = -1;
#endif

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
	kq = -1;
#endif

#if defined(__sun)
	ev_port = -1;
#endif

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
	Amor *pupa;
#if defined(__linux__)
#define A_GET pev[geti]
#define AKEY A_GET.data.ptr
	int nget, geti;
	struct epoll_event *pev=new struct epoll_event[max_evs];
	if ( epfd == -1) return;
#endif	//for linux

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
#define A_GET kev[geti]
#define AKEY A_GET.udata
	int nget, geti;
	struct kevent *kev=new struct kevent[max_evs];
	int ret;
	if ( kq == -1) return;
#endif	//for bsd

#if defined(__sun)
#define A_GET pev[geti]
#define AKEY A_GET.portev_user
	uint_t nget, geti;
	int ret;
	port_event_t *pev =new port_event_t[max_evs]  ;
	if ( ev_port == -1) return;
#endif

#if defined(_WIN32XX)
#define A_GET pov[geti]
#define AKEY A_GET.lpCompletionKey
	BOOL success;
	ULONG nget, geti;
	OVERLAPPED_ENTRY *pov = new OVERLAPPED_ENTRY[max_evs];
	if (!iocp_port) return;
#endif

#if defined(_WIN32)
#define A_GET a_en
#define AKEY A_GET.lpCompletionKey
	BOOL success;
	ULONG nget, geti;
	OVERLAPPED_ENTRY a_en;
	poll_ps.indic = &A_GET;
	if (!iocp_port) return;
#endif

	poll_ps.indic = 0;
LOOP:
	if ( pendor_top > -1 ) run_pendors();

#if defined(__sun)
	nget = 1;
	ret = port_getn(ev_port, pev, max_evs, &nget, 0);
	if ( ret != 0 ) 
	{
		ERROR_PRO("port_getn");
		WLOG(ERR,errMsg);
		goto LOOP;
	}
#endif

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
	ret = kevent(kq, NULL, 0, kev, max_evs, NULL);
	if ( ret != 0 ) 
	{
		ERROR_PRO("kevent");
		WLOG(ERR,errMsg);
		goto LOOP;
	}
#endif	//for bsd

#if defined(__linux__)
	if ( epfd == -1) return;
	nget = epoll_pwait(epfd, pev, max_evs, -1, 0);
	if ( nget <=0 ) 
	{
		ERROR_PRO("epoll_pwait");
		WLOG(ERR,errMsg);
		goto LOOP;
	}
#endif	//for linux


#if defined(_WIN32XX)
	nget = 0;
	success =  GetQueuedCompletionStatusEx(iocp_port,         // Completion port handle
			pov, // pre-allocated array of OVERLAPPED_ENTRY structures
			(ULONG)max_evs,	//The maximum number of entries to remove
			&nget,	//receives the number of entries actually removed
			INFINITE,      // for ever
			FALSE       //does not return until the time-out period has elapsed or an entry is retrieved
                    );  
	if ( !success)  
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

#if defined(_WIN32)
	memset(&A_GET, 0, sizeof(A_GET));
	success = GetQueuedCompletionStatus(iocp_port,         // Completion port handle  
			&(A_GET.dwNumberOfBytesTransferred),  // Bytes transferred  
			&(A_GET.lpCompletionKey),  
			&(A_GET.lpOverlapped),          // OVERLAPPED structure  
			INFINITE       // for ever
                    );  
	if ( A_GET.lpCompletionKey == NULL)  
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
	nget=1;
#endif

#define AOR ((struct DPoll::PollorBase *)AKEY)
#define PPO  ((struct DPoll::Pollor *)AKEY)
#define TOR  ((struct Timor *)AKEY)

	WBUG("nget %d", nget);
	for ( geti = 0 ; geti < nget; geti++)
	{
		switch (AOR->type ) 
		{
		case DPoll::Alarm:
			WBUG("get DPoll:Alarm");
			pupa = TOR->pupa ;
			TOR->type = DPoll::NotUsed;
			TOR->pupa = 0;
#if defined(__linux__)
			if ( -1 == close(TOR->fd) )
			{
				ERROR_PRO("close(DPoll:Alarm) failed");
				WLOG(WARNING, errMsg);
			}
#endif

#if defined(__sun)
			/* delete the timer */
			if (timer_delete(TOR->timerid)!=0)
			{
				ERROR_PRO("timer_delete(DPoll:Alarm) failed");
				WLOG(WARNING, errMsg);
			}
#endif

#if defined (_WIN32)
			if ( !DeleteTimerQueueTimer(timer_queue, TOR->timer_hnd, INVALID_HANDLE_VALUE) )
			{
				ERROR_PRO("CreateTimerQueueTimer(DPoll:Alarm) failed ");
				WLOG(WARNING, errMsg);
			}
#endif
			put_timor(TOR);
			tm_hd_ps.indic = 0;
			pupa->facio(&tm_hd_ps);		//clear timer_handle 
			pupa->facio(&timer_pius);	//TIMER
			break;

		case DPoll::Timer:
			WBUG("get DPoll:Timer");
			TOR->pupa->facio(&timer_pius);
			break;

		case DPoll::Aio:
			break;

		case DPoll::File:
			break;

		case DPoll::Sock:
			WBUG("get DPoll:Sock");
#if  defined(__sun)
			if (A_GET.portev_events & (POLLIN | POLLRDNORM )) {
				PPO->pupa->facio(&(PPO->pro_ps));
			} else if (A_GET.portev_events & POLLOUT ) {
				poll_ps.ordo = Notitia::WR_EPOLL;
				PPO->pupa->facio(&poll_ps);
			} else if (A_GET.portev_events & POLLHUP) {
				poll_ps.ordo = Notitia::EOF_EPOLL;
				PPO->pupa->facio(&poll_ps);
			} else 	if (A_GET.portev_events & (POLLERR |POLLNVAL )) {
				poll_ps.ordo = Notitia::ERR_EPOLL;
				poll_ps.indic = errMsg;
				TEXTUS_SPRINTF(errMsg, "port_get(POLLERR)");
				PPO->pupa->facio(&poll_ps);
				poll_ps.indic = 0;
			} else {
				WLOG(WARNING, "unknown events %08X", A_GET.portev_events);
			}
#endif	//for sun

#if  defined(__linux__)
			if (A_GET.events & EPOLLIN ) {
				PPO->pupa->facio(&(PPO->pro_ps));
			} else if (A_GET.events & EPOLLOUT) {
				poll_ps.ordo = Notitia::WR_EPOLL;
				PPO->pupa->facio(&poll_ps);
			} else if (A_GET.events & EPOLLRDHUP) {
				poll_ps.ordo = Notitia::EOF_EPOLL;
				PPO->pupa->facio(&poll_ps);
			} else if (A_GET.events & EPOLLHUP ) {
				poll_ps.ordo = Notitia::ERR_EPOLL;
				poll_ps.indic = errMsg;
				TEXTUS_SPRINTF(errMsg, "epoll_wait(EPOLLHUP)");
				PPO->pupa->facio(&poll_ps);
				poll_ps.indic = 0;
			} else if (A_GET.events & (EPOLLERR)) {
				poll_ps.ordo = Notitia::ERR_EPOLL;
				poll_ps.indic = errMsg;
			 	TEXTUS_SPRINTF(errMsg, "epoll_wait(EPOLLERR)");
				PPO->pupa->facio(&poll_ps);
				poll_ps.indic = 0;
			} else {
				WLOG(WARNING, "unknown events %08X", A_GET.events);
			}
#endif	//for linux

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
			switch (A_GET.filter ) 
			{
			case EVFILT_READ:
				PPO->pupa->facio(&(PPO->pro_ps));
				break;

			case EVFILT_WRITE:
				poll_ps.ordo = Notitia::WR_EPOLL;
				PPO->pupa->facio(&poll_ps);
				break;
			default:
				if (A_GET.flags & EV_EOF) {
					poll_ps.ordo = Notitia::EOF_EPOLL;
					PPO->pupa->facio(&poll_ps);
				} else if (A_GET.flags & EV_ERROR) {
					poll_ps.ordo = Notitia::ERR_EPOLL;
					poll_ps.indic = errMsg;
					TEXTUS_SNPRINTF(errMsg, errstr_len, "kevent(EV_ERROR) system error(0x%08lX): %s.", A_GET.data, strerror(A_GET.data));
					PPO->pupa->facio(&poll_ps);
					poll_ps.indic = 0;
				} else {
					WLOG(WARNING, "unknown events %08 or flag %08X", A_GET.filter, A_GET.flags);
				}
				break;
			}
#endif	//for bsd

#if defined (_WIN32XX)
			poll_ps.ordo = pov[geti].dwNumberOfBytesTransferred< 0 ? Notitia::ERR_EPOLL : PPO->pro_ps.ordo;
			poll_ps.indic = &A_GET;
			PPO->pupa->facio(&poll_ps);
#endif
#if defined (_WIN32)
			if ( success ) {
				poll_ps.ordo = PPO->pro_ps.ordo;
				poll_ps.indic = &A_GET;
			} else {
				poll_ps.ordo = Notitia::ERR_EPOLL;
				ERROR_PRO("GetIOCP");
				poll_ps.indic = errMsg;
			}
			PPO->pupa->facio(&poll_ps);
#endif
			break;
		default:
			break;
		}
	}

	if ( !shouldEnd) goto LOOP;
}
#include "hook.c"

