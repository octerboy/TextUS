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
#include <sys/eventfd.h>
#endif
#include <sys/timeb.h>
#include <time.h>
#include <errno.h>
#include <assert.h>

#if defined (_WIN32 )
#define ERROR_PRO(X) { \
	char *s; \
	char error_string[1024]; \
	dw_error = GetLastError(); \
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw_error, \
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) error_string, 1024, NULL );\
	s= strstr(error_string, "\r\n") ; \
	if (s )  *s = '\0';  \
	if ( errMsg ) \
		TEXTUS_SNPRINTF(errMsg, errstr_len, "%s errno %d, %s", X, dw_error, error_string);\
	}
#else
#define ERROR_PRO(X)  if ( errMsg ) \
		TEXTUS_SNPRINTF(errMsg, errstr_len, "%s errno %d, %s.", X, errno, strerror(errno));
#endif

class TPoll: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor* clone();

	TPoll();
	struct DPoll::PollorBase  a_basp;
#if defined (_WIN32)
	HANDLE iocp_port,timer_queue;
	DWORD dw_error;
	typedef NTSTATUS (CALLBACK* NTSETTIMERRESOLUTION)
	(
		IN ULONG DesiredTime,
		IN BOOLEAN SetResolution,
		OUT PULONG ActualTime
	);
	NTSETTIMERRESOLUTION NtSetTimerResolution;

	typedef NTSTATUS (CALLBACK* NTQUERYTIMERRESOLUTION)
	(
		OUT PULONG MaximumTime,
		OUT PULONG MinimumTime,
		OUT PULONG CurrentTime
	);
	NTQUERYTIMERRESOLUTION NtQueryTimerResolution;
	ULONG cur_time_res;
#endif
#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
	int kq;
#endif	//for bsd

#if defined(__linux__)
	int epfd;
	struct itimerspec	itimeout;
	bool use_evtfd;
	int evt_fd;	/* event fd , for aio */
	aio_context_t aio_ctx;
#define NUM_EVENTS 32
	struct io_event io_evs[NUM_EVENTS];
	struct Eventor : DPoll::PollorBase {
			struct epoll_event ev;
			inline Eventor() {
			pupa = 0;
			type = DPoll::EventFD ;
			ev.data.ptr=this;
			ev.events = EPOLLIN | EPOLLET;
		}
	} evtor;
	inline int io_setup(unsigned nr, aio_context_t *ctxp)
	{
		return syscall(__NR_io_setup, nr, ctxp);
	}

	inline int io_destroy(aio_context_t ctx) 
	{
		return syscall(__NR_io_destroy, ctx);
	}

	inline int io_getevents(aio_context_t ctx, TEXTUS_LONG min_nr, TEXTUS_LONG max_nr,
		struct io_event *events, struct timespec *timeout)
	{
		return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout);
	}
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
	ULONG timer_resolution;
	int timer_res = 0;
	timer_resolution = 0;
	cfg->QueryIntAttribute("timer_resolution", &timer_res);
	timer_resolution = timer_res;
	cur_time_res  =0;

	if (timer_resolution > 0 )  
	{
		NTSTATUS nStatus;
		HMODULE hNtDll = LoadLibrary(TEXT("NtDll.dll"));
		if (hNtDll)
		{
			NtQueryTimerResolution = (NTQUERYTIMERRESOLUTION)GetProcAddress(hNtDll,"NtQueryTimerResolution");
			NtSetTimerResolution = (NTSETTIMERRESOLUTION)GetProcAddress(hNtDll,"NtSetTimerResolution");
			FreeLibrary(hNtDll);
		}
		if (NtQueryTimerResolution == NULL || NtSetTimerResolution  ==  NULL) {
			ERROR_PRO("NtSetTimerResolution not found");
			return ;
		}
		//nStatus = NtSetTimerResolution(10000, true, &cur_time_res);
		nStatus = NtSetTimerResolution(timer_resolution, true, &cur_time_res);
		//printf("nStatus %d\n", nStatus);
		
	}
	//cur_time_res = ExSetTimerResolution(timer_resolution, true); wdm.h is for driver
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
	if( cfg->Attribute("no_aio") )
		use_evtfd = false;
	else
		use_evtfd = true;

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
	if ( use_evtfd ) {
		evt_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC); 	/* event fd , for aio */
		if (evt_fd == -1) {
				ERROR_PRO("eventfd");
				return ;
		}

		memset(&aio_ctx, 0, sizeof(aio_ctx));
		if (io_setup(16384, &aio_ctx)) {
				ERROR_PRO("io_setup");
				return ;
		}
	}
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
#if defined(__sun)
		WBUG("%p %s fd=%d", ppo->pupa, "sponte CLR_EPOLL(port_dissociate)", ppo->fd);
		if( !port_dissociate(ev_port, PORT_SOURCE_FD, ppo->fd) )
		{
			ERROR_PRO("port_dissociate(PORT_SOURCE_FD) failed");
			WLOG(WARNING, errMsg);
		}
#endif

#if defined(__linux__)
		WBUG("%p %s", ppo->pupa, "sponte CLR_EPOLL(epoll_ctl)");
		if( epoll_ctl(epfd, EPOLL_CTL_DEL, ppo->fd, &ppo->ev) != 0 )
		{
			ERROR_PRO("epoll_ctl failed");
			WLOG(WARNING, errMsg);
		}
#endif	//for linux
#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
		WBUG("%p %s", ppo->pupa, "sponte CLR_EPOLL(kevent)");
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
#if defined(_WIN32)
		WBUG("%p %s", ((DPoll::Pollor *)apius->indic)->pupa, "sponte AIO_EPOLL");
		hPort = CreateIoCompletionPort(((DPoll::Pollor *)apius->indic)->hnd.file, iocp_port, (ULONG_PTR)apius->indic /* completion key */, number_threads);  
		if (hPort == NULL)  
		{ 
			ERROR_PRO("CreateIoCompletionPort failed to associate");
			WLOG(WARNING, errMsg);
		}
#else
		WBUG("%p %s", ((DPoll::PollorAio *)apius->indic)->pupa, "sponte AIO_EPOLL");
#endif
#if defined(__sun)
		((DPoll::PollorAio *)apius->indic)->pn.portnfy_port = ev_port;
		((DPoll::PollorAio *)apius->indic)->pn.portnfy_user = apius->indic;
		((DPoll::PollorAio *)apius->indic)->aiocb_R.aio_sigevent.sigev_notify = SIGEV_PORT;
		((DPoll::PollorAio *)apius->indic)->aiocb_W.aio_sigevent.sigev_notify = SIGEV_PORT;
#endif
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)  
		((DPoll::PollorAio *)apius->indic)->aiocb_R.aio_sigevent.sigev_notify = SIGEV_KEVENT;
		((DPoll::PollorAio *)apius->indic)->aiocb_W.aio_sigevent.sigev_notify = SIGEV_KEVENT;
		((DPoll::PollorAio *)apius->indic)->aiocb_R.aio_sigevent.sigev_notify_kqueue = kq;
		((DPoll::PollorAio *)apius->indic)->aiocb_W.aio_sigevent.sigev_notify_kqueue = kq;
		((DPoll::PollorAio *)apius->indic)->aiocb_R.aio_sigevent.sigev_value.sival_ptr = apius->indic;
		((DPoll::PollorAio *)apius->indic)->aiocb_W.aio_sigevent.sigev_value.sival_ptr = apius->indic;
#endif
#if defined(__linux__)
		((DPoll::PollorAio *)apius->indic)->aiocb_W.aio_flags = IOCB_FLAG_RESFD;
		((DPoll::PollorAio *)apius->indic)->aiocb_W.aio_resfd = evt_fd;
		((DPoll::PollorAio *)apius->indic)->aiocb_W.aio_data = (__u64)apius->indic;
		((DPoll::PollorAio *)apius->indic)->aiocb_R.aio_flags = IOCB_FLAG_RESFD;
		((DPoll::PollorAio *)apius->indic)->aiocb_R.aio_resfd = evt_fd;
		((DPoll::PollorAio *)apius->indic)->aiocb_R.aio_data = (__u64)apius->indic;
		((DPoll::PollorAio *)apius->indic)->ctx = aio_ctx;
#endif
		break;

	case Notitia::POST_EPOLL :	/* user post  */
		baspo = (DPoll::PollorBase *)apius->indic;	
		WBUG("%p %s", baspo->pupa, "sponte POST_EPOLL");
		switch ( baspo->type )
		{
		case DPoll::NotUsed:
			apius->indic = this;
			break;
		case DPoll::User:
#if defined (_WIN32)
			PostQueuedCompletionStatus(g_poll->iocp_port, 0, (ULONG_PTR)baspo, 0);
#endif
			break;
		default:
			break;
		}
		break;

	case Notitia::SET_EPOLL :	/* IOCP,epoll  */
		ppo = (DPoll::Pollor *)apius->indic;	
		assert(ppo);

#if defined(__linux__)
		WBUG("%p %s(fd=%d) events(%08x)", ppo->pupa, "sponte SET_EPOLL", ppo->fd, ppo->ev.events );
		if( epoll_ctl(epfd, ppo->op, ppo->fd, &ppo->ev)  != 0 )
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
		WBUG("%p %s(fd=%d) events(%p)", ppo->pupa, "sponte SET_EPOLL", ppo->fd, ppo->events);
		if( kevent(kq, &(ppo->events[0]), 2, NULL, 0, NULL) == - 1 )
		{
			ERROR_PRO("kevent(SET_EPOLL) failed");
			WLOG(WARNING, errMsg);
		}		
#endif	//for bsd

#if defined (_WIN32)
		switch (ppo->type ) {
		case DPoll::IOCPFile:
			port_hnd = ppo->hnd.file;
			break;
		case DPoll::IOCPSock:
			port_hnd = (HANDLE)ppo->hnd.sock;
			break;
		default:
			break;
		}

		WBUG("%p %s(handle=%p) ", ppo->pupa, "sponte SET_EPOLL", port_hnd );
/*
		if ( !SetFileCompletionNotificationModes(port_hnd, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS) )
		{
			ERROR_PRO("SetFileCompletionNotificationModes failed");
			WLOG(WARNING, errMsg);
		}
*/
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
		if( epoll_ctl(epfd, EPOLL_CTL_ADD, aor->fd, &aor->ev)  != 0 )
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
		if ( timer_create(CLOCK_REALTIME, &sigev, &aor->timerid) != 0 )
		{
			WLOG(WARNING, "timer_create failed errno=%d (%s) when %p DMD_SET_TIMER", errno, strerror(errno), ask_pu);
			goto END_TIMER_PRO;
		}

		if ( timer_settime(aor->timerid, 0, &itimeout, NULL) !=0 )
		{
			WLOG(WARNING, "timer_settime failed errno=%d (%s) when %p DMD_SET_TIMER", errno, strerror(errno), ask_pu);
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
		//printf(" aor->pupa %p hand\n", aor->pupa);
		aor->pupa->facio(&tm_hd_ps);
		break;

END_TIMER_PRO:
		tm_hd_ps.indic = 0;
		//printf(" error aor->pupa %p hand\n", ((Amor*) (apius->indic)));
		((Amor*) (apius->indic))->facio(&tm_hd_ps);
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
		WBUG("%p sponte DMD_SET_ALARM, interval: %d, interval2: %d", ask_pu, interval, interval2);
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
		tmp_timeout.it_value.tv_nsec = (interval%1000)*1000000;
		tmp_timeout.it_interval.tv_sec = interval2/1000;
		tmp_timeout.it_interval.tv_nsec = (interval2%1000)*1000000;
		if (timerfd_settime(aor->fd, 0, &tmp_timeout, NULL) == -1) 
		{
			ERROR_PRO("timerfd_settime failed");
			WLOG(WARNING, errMsg);
			close(aor->fd);
			goto END_ALARM_PRO;
		}
		if( epoll_ctl(epfd, EPOLL_CTL_ADD, aor->fd, &aor->ev)  != 0 )
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
		if ( timer_create(CLOCK_REALTIME, &sigev, &(aor->timerid)) !=0 )
		{
			WLOG(WARNING, "timer_create failed errno=%d (%s) when %p DMD_SET_ALARM", errno, strerror(errno), ask_pu);
			goto END_ALARM_PRO;
		}
		tmp_timeout.it_value.tv_sec = interval/1000;
		tmp_timeout.it_value.tv_nsec = (interval%1000)*1000;
		tmp_timeout.it_interval.tv_sec = interval2/1000;
		tmp_timeout.it_interval.tv_nsec = (interval2%1000)*1000;

		if (timer_settime (aor->timerid, 0, &tmp_timeout, NULL) != 0 )
		{
			WLOG(WARNING, "timer_settime failed errno=%d (%s) when %p DMD_SET_ALARM", errno, strerror(errno), ask_pu);
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
		//printf(" set_alarm aor->pupa %p hand\n", aor->pupa);
		aor->pupa->facio(&tm_hd_ps);
		break;

END_ALARM_PRO:
		tm_hd_ps.indic = 0;
		//printf(" error set_alarm aor->pupa %p hand\n", aor->pupa);
		WLOG(ERR,"error set_alarm aor->pupa %p hand", aor->pupa);
		aor->pupa->facio(&tm_hd_ps);
		put_timor(aor); //发生错误而回收
		break;

	case Notitia::DMD_CLR_TIMER :	/* 清定时通知对象 */
		aor = (struct Timor *)apius->indic;
		if ( !aor ) break;
		WBUG("%p sponte DMD_CLR_TIMER", aor->pupa);
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
		//printf("%d clear_timer  aor->pupa %p hand\n", __LINE__, aor->pupa);
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
		a_basp.pupa = (Amor*)apius->indic;
#if defined (_WIN32)
		PostQueuedCompletionStatus(g_poll->iocp_port, 0, (ULONG_PTR)&a_basp, 0);
#endif
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
			return true;
		}
#if defined(__linux__)
		if( epoll_ctl(epfd, EPOLL_CTL_ADD, evt_fd, &evtor.ev)  != 0 )
		{
			ERROR_PRO("epoll_ctl(SET_TIMER) failed");
			WLOG(WARNING, errMsg);
		}
#endif	//for linux
#if defined(_WIN32)
		WLOG(INFO, "current timer resolution is %lu(100ns)", cur_time_res);
#endif

		break;

	case Notitia::WINMAIN_PARA:	/* 在整个系统中, 这应是最后被通知到的。 */
		WBUG("facio Notitia::WINMAIN_PARA");
		goto MainPro;

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

	a_basp.type = DPoll::User;
#if defined(_WIN32)
	iocp_port = NULL;
#endif

#if defined(__linux__)
	epfd = -1;
	use_evtfd = false;
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
	u_int64_t obtain;	//for EventFD
	struct epoll_event *pev=new struct epoll_event[max_evs];
	if ( epfd == -1) return;
#endif	//for linux

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
#define A_GET kev[geti]
#define AKEY A_GET.udata
#define Event_ID A_GET.ident
	int nget, geti;
	int my_error;
	struct kevent *kev=new struct kevent[max_evs];
	int ret;
	if ( kq == -1) return;
#endif	//for bsd

#if defined(__sun)
#define A_GET pev[geti]
#define AKEY A_GET.portev_user
#define Event_ID A_GET.portev_object
	uint_t nget, geti;
	int ret;
	int my_error;
	port_event_t *pev =new port_event_t[max_evs]  ;
	if ( ev_port == -1) return;
#endif

#if defined(_WIN32) && !defined(_WIN32XX)
#define A_GET pov[geti]
#define AKEY A_GET.lpCompletionKey
	BOOL success;
	ULONG nget, geti;
	DWORD num_trans, rflag;
	OVERLAPPED_ENTRY *pov = new OVERLAPPED_ENTRY[max_evs];
	if (!iocp_port) return;
#endif

#if defined(_WIN32XX)
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
	nget = kevent(kq, NULL, 0, kev, max_evs, NULL);
	if ( nget == -1 ) 
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


#if defined(_WIN32) && !defined(_WIN32XX)
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
			ERROR_PRO("GetQueuedCompletionStatusEx");
			WLOG(INFO,errMsg);
		} else if(nError != WAIT_TIMEOUT)	//TIME OUT
		{
			ERROR_PRO("GetQueuedCompletionStatusEx");
			WLOG(ERR,errMsg);
		}
		goto LOOP;  
	}
#endif

#if defined(_WIN32XX)
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
#define AIOR  ((struct DPoll::PollorAio*)AKEY)

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
			//printf(" timer_alarm  pupa %p hand\n", pupa);
			pupa->facio(&tm_hd_ps);		//clear timer_handle 
			pupa->facio(&timer_pius);	//TIMER
			break;

		case DPoll::Timer:
			WBUG("get DPoll:Timer");
			TOR->pupa->facio(&timer_pius);
			break;

#if defined(__linux__)
		case DPoll::EventFD:
			WBUG("get DPoll:EventFD"); /* aio for linux*/
			obtain = 0;
			if (read(evt_fd, &obtain, sizeof(obtain)) != sizeof(obtain))
			{
				WLOG_OSERR("read evt_fd");
				break;
			}
			while (obtain > 0) {
				nget = io_getevents(aio_ctx, 1, obtain > NUM_EVENTS ? NUM_EVENTS: (TEXTUS_LONG) obtain, io_evs, NULL);
				if (nget > 0) {
					for (geti = 0; geti < nget; geti++) {
						switch ( io_evs[geti].res2 ) {
						case 0:
							poll_ps.ordo = Notitia::PRO_EPOLL;
							poll_ps.indic = (void*)(&io_evs[geti]);
							((DPoll::PollorAio *)io_evs[geti].data)->pupa->facio(&poll_ps);
							break;
						case EINPROGRESS:
						case EINVAL:
							TEXTUS_SNPRINTF(errMsg, errstr_len, "errno %lld, %s.", io_evs[geti].res2, strerror(io_evs[geti].res2));
							WLOG(WARNING, errMsg);
							break;
						default:
							poll_ps.ordo = Notitia::ERR_EPOLL;
							if ( (void*)(io_evs[geti].obj) == (void*)&(AIOR->aiocb_W))
								TEXTUS_SNPRINTF(errMsg, errstr_len, "write errno %lld, %s.", io_evs[geti].res2, strerror(io_evs[geti].res2));
							else
								TEXTUS_SNPRINTF(errMsg, errstr_len, "read errno %lld, %s.", io_evs[geti].res2, strerror(io_evs[geti].res2));
							poll_ps.indic = errMsg;
							PPO->pupa->facio(&poll_ps);
							poll_ps.indic = 0;
							break;
						}
					}
					obtain -= nget;
				}
			}
			break;
#endif

#if  defined(__sun) || defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		case DPoll::Aio:
			WBUG("get DPoll:Aio");
			my_error = aio_error((struct aiocb*)(Event_ID));
			switch ( my_error)
			{
			case 0:
				poll_ps.ordo = Notitia::PRO_EPOLL;
				poll_ps.indic = (void*)(Event_ID);
				PPO->pupa->facio(&poll_ps);
				break;
			case EINPROGRESS:
			case EINVAL:
				TEXTUS_SNPRINTF(errMsg, errstr_len, "errno %d, %s.", my_error, strerror(my_error));
				WLOG(WARNING, errMsg);
				break;
			default:
				poll_ps.ordo = Notitia::ERR_EPOLL;
				if ( (void*)(Event_ID) == (void*)&(AIOR->aiocb_W))
					TEXTUS_SNPRINTF(errMsg, errstr_len, "write errno %d, %s.", my_error, strerror(my_error));
				else
					TEXTUS_SNPRINTF(errMsg, errstr_len, "read errno %d, %s.", my_error, strerror(my_error));
				poll_ps.indic = errMsg;
				PPO->pupa->facio(&poll_ps);
				poll_ps.indic = 0;
				break;
			}
			break;
#endif

#if defined (_WIN32)
		case DPoll::IOCPSock:
			
#if !defined(_WIN32XX)
			num_trans = 0;
			success = WSAGetOverlappedResult((PPO->hnd.sock), A_GET.lpOverlapped, &num_trans, FALSE, &rflag);
#endif
			goto WIN_POLL;

		case DPoll::IOCPFile:
			
#if !defined(_WIN32XX)
			num_trans = 0;
			success  = GetOverlappedResult(PPO->hnd.file, A_GET.lpOverlapped, &num_trans, FALSE);
		WIN_POLL:
			if ( success )
			{
				poll_ps.ordo = PPO->pro_ps.ordo;
				poll_ps.indic = &A_GET;
			} else {
				if ( WSAGetLastError() == ERROR_HANDLE_EOF )
				{
					WBUG("GetOverlappedResult return ERROR_HANDLE_EOF");
					poll_ps.ordo = PPO->pro_ps.ordo;
					A_GET.dwNumberOfBytesTransferred = 0;
					poll_ps.indic = &A_GET;
				} else {
					ERROR_PRO("GetIOCPEx");
					WLOG(WARNING, "GetOverlappedResult %s", errMsg);
					if ( dw_error == ERROR_MORE_DATA ) 
					{
						poll_ps.ordo = Notitia::MORE_DATA_EPOLL;
						poll_ps.indic = &A_GET;
					} else {
						poll_ps.ordo = Notitia::ERR_EPOLL;
						poll_ps.indic = errMsg;
					}
				}
			}
			WBUG("get DPoll:%s %s trans(%d) get(%d)", AOR->type ==  DPoll::IOCPFile ? "File" : "Sock", success ? "success" : "failed", num_trans, A_GET.dwNumberOfBytesTransferred);
			PPO->pupa->facio(&poll_ps);
#endif
#if defined (_WIN32XX)
		WIN_POLL:
			if ( success ) {
				poll_ps.ordo = PPO->pro_ps.ordo;
				poll_ps.indic = &A_GET;
			} else {
				ERROR_PRO("GetIOCP");
				WLOG(WARNING, "GetQueuedCompletionStatus return %d %s", success, errMsg);
				if ( dw_error == ERROR_MORE_DATA ) 
				{
					poll_ps.ordo = Notitia::MORE_DATA_EPOLL;
					poll_ps.indic = &A_GET;
				} else {
					poll_ps.ordo = Notitia::ERR_EPOLL;
					poll_ps.indic = errMsg;
				}
			}
			PPO->pupa->facio(&poll_ps);
#endif
			break;

#endif
		case DPoll::FileD:
			WBUG("get DPoll:FileD");
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

			break;

		case DPoll::User:
			WBUG("get DPoll:User");
			poll_ps.ordo = Notitia::PRO_EPOLL;
			poll_ps.indic = 0;
			AOR->pupa->facio(&poll_ps);
			break;
		default:
			WBUG("ngeti %d, unkown type %d", geti, AOR->type);
			break;
		}
	}

	if ( !shouldEnd) goto LOOP;
	WBUG("will exit.... ");
}
#include "hook.c"

