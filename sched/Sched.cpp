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
#include <sys/timeb.h>
#include <time.h>
#include <errno.h>
#include <assert.h>


#define TOR_SIZE FD_SETSIZE
#if defined(_WIN64)
typedef SOCKET MY_FD_TYPE;
#else
typedef int MY_FD_TYPE;
#endif
class Sched: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor* clone();

	Sched();
private:
	MY_FD_TYPE maxfd;
	int timer_milli; /* ??ʱ???????? */
	int timer_usec;	/* ??ѯ????΢???? */
	int timer_sec;	/* ??ѯ???????? */

	struct Tor_Pool {
		Amor::Pius wpius;
		Describo::Criptor wtor;
		int  top; /* ??ջ?? */
		int  house[TOR_SIZE]; /* ?׽??ֱ?????, pool?б???"??"??????, ????һ????ջ. */
		int cur;		/* ָʾ??????????ֵ, ????֮??, ??CriptorΪ?? */
		Describo::Criptor *pool[TOR_SIZE];	/* ???汻??set????????,?????ߴ?ΪFD_SETSIZE */

		fd_set rwSet;

		MY_FD_TYPE setup(Describo::Criptor * ,int*);
		void clear(Describo::Criptor *, int*);
		void isset(fd_set*, Notitia::HERE_ORDO, int& nready);
		Tor_Pool ()
		{
			wpius.ordo = 0;
			wpius.indic = &wtor;
			cur = 0;
			top = -1;
			FD_ZERO(&rwSet);
			memset(pool, 0, sizeof(Describo::Criptor *)*TOR_SIZE);
		}
	} ;

	struct Tor_Pool rd_tors;
	struct Tor_Pool wr_tors;
	struct Tor_Pool ex_tors;

	struct Timer_info {
		Amor *pupa;	/* Ҫ??????TIMER/ALARM?źŵĶ???ָ?? */
		struct timeb since;	/* ????ʱ?? */
		unsigned int interval;/* ʱ??????,?????? */
		int status;	/* 	0: ÿ??ʱ??Ƭ֪ͨ(since??interval????????), 
					1: Ҫ??ʱ֪ͨ, ֻͨ??һ??
					2: ?Ѿ???ʱ֪ͨ????, ?Ժ󲻻???֪ͨ, ????ʹ??????????һֱռ?ſռ?
				*/
		void clear ()
		{
			pupa = 0;
			since.time = 0;
			since.millitm = 0;
			since.timezone = 0;
			since.dstflag = 0;
			interval = 0;
			status = 0;
		}
		Timer_info ()
		{
			clear();
		}
	};	
	Timer_info *timer_infor;/* Ҫ?????趨ʱ?źŵ?????, ?Կ?ָ??Ϊ??????־ */
	int infor_size;		/* timer_infor?ߴ? */
	
	void run();
	void sort();		//????
	bool shouldEnd;
	struct Describo::Pendor *pendors;	//?Ӻ????õ????飻
	int pendor_size;
	MY_FD_TYPE pendor_top;
	void run_pendors();
	Amor::Pius tm_hd_ps;
#include "wlog.h"
};

#define DEFAULT_TIMER_MILLI 1000

void Sched::ignite(TiXmlElement *cfg)
{
	const char *timer_str;

#if defined (_WIN32)
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD( 2, 2 );

	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) 
	{ /* Couldn't find a usable  WinSock DLL. */
		WLOG(ERR,"can't initialize socket library");
		return;
	}

	if ( LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 2 ) 
	{ /* Tell the user that we could not find a usable WinSock DLL. */
    		WSACleanup( );
		WLOG_OSERR("WSAStartup");
	}
#endif 
	timer_milli = DEFAULT_TIMER_MILLI;
	if( (timer_str = cfg->Attribute("timer")) && atoi(timer_str) > 1 )
		timer_milli = atoi(timer_str); //time_strΪ??????

	timer_sec = timer_milli/1000;
	timer_usec = (timer_milli % 1000) * 1000;
	cfg->QueryIntAttribute("ponder", &(pendor_size));
	pendors = new struct Describo::Pendor[pendor_size];
}

bool Sched::sponte( Amor::Pius *apius)
{
	MY_FD_TYPE i;
	int tmp_type = 0;
	unsigned interval =0;
	Amor *ask_pu = (Amor *) 0;
	void **p;

	Describo::Criptor *tor;
	assert(apius);

	switch ( apius->ordo )
	{
	case Notitia::FD_SETRD :	/* ?ö? */
		tor = (Describo::Criptor *)(apius->indic);	
		assert(tor);
		WBUG("%p %s " TSOCKET_FMT, tor->pupa, "sponte FD_SETRD", tor->scanfd);
		i = rd_tors.setup(tor, &(tor->rd_index));

		if ( i >= 0 )
			maxfd = (maxfd > i ? maxfd:i);
		else
			WLOG(CRIT, "FD_SETRD to max %d", TOR_SIZE);
		break;

	case Notitia::FD_SETWR :	/* ??д */
		tor = (Describo::Criptor *)(apius->indic);	
		assert(tor);
		WBUG("%p %s " TSOCKET_FMT, tor->pupa, "sponte FD_SETWR", tor->scanfd);
		i = wr_tors.setup(tor, &(tor->wr_index));
		if ( i >= 0 )
			maxfd = (maxfd > i ? maxfd:i);
		else
			WLOG(CRIT, "FD_SETWR to max %d", TOR_SIZE);
		break;

	case Notitia::FD_SETEX :	/* set except fd */
		tor = (Describo::Criptor *)(apius->indic);	
		assert(tor);
		WBUG("%p %s " TSOCKET_FMT, tor->pupa, "sponte FD_SETEX", tor->scanfd);
		i = ex_tors.setup(tor, &(tor->ex_index));
		if ( i >= 0 )
			maxfd = (maxfd > i ? maxfd:i);
		else
			WLOG(CRIT, "FD_SETEX to max %d", TOR_SIZE);
		break;

	case Notitia::FD_CLRRD :	/* ???ɶ? */
		tor = (Describo::Criptor *)(apius->indic);	
		assert(tor);
		WBUG("%p %s " TSOCKET_FMT, tor->pupa, "sponte FD_CLRRD", tor->scanfd);
		rd_tors.clear(tor, &(tor->rd_index));
		break;

	case Notitia::FD_CLRWR :	/* ????д */
		tor = (Describo::Criptor *)(apius->indic);	
		assert(tor);
		WBUG("%p %s " TSOCKET_FMT, tor->pupa, "sponte FD_CLRWR", tor->scanfd);
		wr_tors.clear(tor, &(tor->wr_index));
		break;

	case Notitia::FD_CLREX :	/* clear exception fd */
		tor = (Describo::Criptor *)(apius->indic);	
		assert(tor);
		WBUG("%p %s " TSOCKET_FMT, tor->pupa, "sponte FD_CLREX", tor->scanfd);
		ex_tors.clear(tor, &(tor->ex_index));
		break;

	case Notitia::DMD_SET_TIMER :	/* ??ʱ??Ƭ֪ͨ???? */
	case Notitia::DMD_SET_ALARM :	/* ?ó?ʱ֪ͨ???? */
		switch ( apius->ordo )
		{
		case Notitia::DMD_SET_TIMER :	
			WBUG("%p sponte DMD_SET_TIMER",  apius->indic);
			ask_pu = (Amor*) (apius->indic);
			interval = 0;
			tmp_type = 0;
			break;

		case Notitia::DMD_SET_ALARM :
			p = (void**) (apius->indic);
			ask_pu = (Amor*) (*p);
			p++;
			interval = *(reinterpret_cast<int*>(*p));
			p++;
			if ( *p ) 
				tmp_type = 3;	//repeat
			else
				tmp_type = 1;
			WBUG("%p sponte DMD_SET_ALARM, interval: %d", ask_pu, interval);
			break;

		default: 
			break;
		}

		for( i = 0 ;i < infor_size ; i++)
		{
			if ( timer_infor[i].pupa == (Amor*)0 || timer_infor[i].pupa == ask_pu )
				break;
		}

		if ( i == infor_size ) 	/* ?ռ䲻????��??֮ */
		{	
			struct Timer_info *tmp = timer_infor;
			timer_infor = new struct Timer_info [infor_size*2];	/* ????2???Ŀռ? */
			memcpy(timer_infor, tmp, sizeof(struct Timer_info) * infor_size);/* ?????ɵ????? */
			infor_size = infor_size*2;	/* ?ߴ?ֵ?ӱ? */
		}

		timer_infor[i].pupa = ask_pu;
		timer_infor[i].status = tmp_type;	
		ftime(&(timer_infor[i].since));
		timer_infor[i].interval = interval;	
		tm_hd_ps.indic = ask_pu;
		ask_pu->facio(&tm_hd_ps);
		break;

	case Notitia::DMD_CLR_TIMER :	/* ?嶨ʱ֪ͨ???? */
		WBUG("%p sponte DMD_CLR_TIMER", apius->indic);
		if ( apius->indic == 0  ) break;
		for( i = 0 ;i < infor_size; i++)
		{
			MY_FD_TYPE j;
			if ( timer_infor[i].pupa != (Amor*)(apius->indic) ) continue;
			for ( j = i; j < infor_size-1; j++) /* ????һ????λ??, ǰ?? */
			{
				timer_infor[j].pupa =  timer_infor[j+1].pupa;
				timer_infor[j].since =  timer_infor[j+1].since;
				timer_infor[j].interval =  timer_infor[j+1].interval;
				timer_infor[j].status =  timer_infor[j+1].status;
			}

			timer_infor[j].clear();
			tm_hd_ps.indic = 0;
			((Amor*)(apius->indic))->facio(&tm_hd_ps);
			break;
		}
		break;

	case Notitia::CMD_MAIN_EXIT :	/* ??ֹ???? */
		WBUG("CMD_MAIN_EXIT");
		shouldEnd = true;
		break;

	case Notitia::CMD_GET_SCHED:	/* ȡ?ñ???????ַ */
		WBUG("CMD_GET_SCHED this = %p", this);
		apius->indic = this;
		break;

	case Notitia::CMD_PUT_PENDOR:	/* ??????Ҫ???ȵĶ??? */
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

bool Sched::facio( Amor::Pius *pius)
{
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::WINMAIN_PARA:	/* ??????ϵͳ??, ??Ӧ????????֪ͨ???ġ? */
		WBUG("facio Notitia::WINMAIN_PARA");
		goto MainPro;

	case Notitia::MAIN_PARA:	/* ??????ϵͳ??, ??Ӧ????????֪ͨ???ġ? */
		WBUG("facio Notitia::MAIN_PARA");
MainPro:
		run();
		break;

	default:
		return false;
	}
	return true;
}

Amor* Sched::clone()
{
	return (Amor*)this;
}

MY_FD_TYPE Sched::Tor_Pool::setup (Describo::Criptor *tor, int *index)
{
	if ( *index >= 0 ) goto GIVE;	/* ?????ظ????? */
	if ( top >=0 )
	{	/* house??ָʾ, pool?????б??ճ?��??λ??,????????λ?ü??? */
		*index = house[top]; 
		pool[house[top]] = tor; /* rd_house[rd_top]Ϊ??λ???????? */
		top--;	/* ??ջ */

	} else if (cur < TOR_SIZE) { 	/* ??��ǰ???Ķ?????, ??һ????λ?? */
		*index =  cur;
		pool[*index] = tor;
		cur++;

	} else 	/* ϵͳ?ﵽ????ֵ, ???? */
		return -1;

	FD_SET(tor->scanfd, &rwSet);
GIVE:
	return tor->scanfd;
}

void Sched::Tor_Pool::clear (Describo::Criptor *tor, int *index)
{
	FD_CLR(tor->scanfd, &rwSet);
	if ( *index < 0 ) return;

	pool[*index] = (Describo::Criptor *)0; /* ?ڳ?"??"λ?? */
	if ( cur - *index == 1 ) /* ?պ?????????????һ?? */
	{
		cur--;
	} else {	/* ?ڳ??????м???λ??, ??֮??????ջ, ?Ա㽫��?? */
		top++;
		house[top] = *index;
	}
	*index = -1;	/* ?????ٴβ??? */
}

void Sched::Tor_Pool::isset( fd_set* pset, Notitia::HERE_ORDO ordo, int& nready)
{
	wpius.ordo = ordo;

	int i = 0;
	Describo::Criptor **pidle = &pool[0];
	for (; i < cur && nready > 0; i++,pidle++ )
	{
		if ( (*pidle) && FD_ISSET((*pidle)->scanfd, pset))
		{		
			wtor.scanfd = (*pidle)->scanfd;
			(*pidle)->pupa->facio(&wpius);
			nready--;
		}
	}
}

Sched::Sched()
{
	maxfd = -1;
	timer_usec = 50*1000;	//Ĭ??50????
	timer_sec = 0;	//Ĭ??50????

	infor_size = 64;
	timer_infor = new struct Timer_info [infor_size];
	shouldEnd = false;
	pendors = 0;
	pendor_top = -1;
	pendor_size = 16;
	tm_hd_ps.ordo = Notitia::TIMER_HANDLE;	
}

void Sched::run_pendors()
{
	Amor::Pius aps;
	struct Describo::Pendor pen;
	MY_FD_TYPE i;
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

/* ????Ψһ?����??صĺ???  */
void Sched:: run()
{
bool should_click;
	int nready;
	fd_set rset;
	fd_set wset;
	fd_set eset;
	struct timeval tv;	

	int busy = 0 ;	//æ????, һ???п?, ??????λΪ0 

LOOP:
	if ( pendor_top > -1 ) run_pendors();
	rset = rd_tors.rwSet;
	wset = wr_tors.rwSet;
	eset = ex_tors.rwSet;
	tv.tv_sec = timer_sec;
	tv.tv_usec = timer_usec;
	should_click = false;

	WBUG("select rd %d, write %d, except %d", rd_tors.cur, wr_tors.cur, ex_tors.cur);
#if defined(_WIN32)
	/* In MS Windows, select() will return error if three descriptor parameters were null.
		Here, Sleep() is alternative.
	*/ 
	if ( rd_tors.cur == 0  && wr_tors.cur == 0 && ex_tors.cur == 0 )
	{
		Sleep(timer_milli);
		nready = 0;
	} else
#endif
	nready = select((maxfd+1), 	/* ????win64, ??ôӲת??, ??û?????⣿??????select?????  */
		rd_tors.cur == 0 ? NULL: &rset,
		wr_tors.cur == 0 ? NULL: &wset, 
		ex_tors.cur == 0 ? NULL: &eset, &tv); 
	WBUG("nready %d", nready);
	if (nready > 0)
	{
		int nre = nready;
		/* ???ҿɶ��? */
		rd_tors.isset(&rset, Notitia::FD_PRORD, nready);	
		/* ???ҿ?д?? */
		if ( nready > 0 ) wr_tors.isset(&wset, Notitia::FD_PROWR, nready);	
		/* ?????쳣?? */
		if ( nready > 0 ) ex_tors.isset(&eset, Notitia::FD_PROEX, nready);	

		/* select?ȴ???ȥ????ʱ??, ??????, ?Ƶ?busy?? */
		busy += timer_milli - tv.tv_sec*1000 - tv.tv_usec/1000 ;
		/* ?????????׽?????ȥ????ʱ??, ??????, 40?Ǿ???ֵ */
		busy +=  (nre/40);
		if ( busy > timer_milli) 
		{
			busy = 0;
			should_click = true;
		}
	} else if ( nready == 0) {
		should_click = true;
		busy =0;
	} else {	/* ?????˴??? */
	#if defined(_WIN32 )
		char *s;
		char errstr[1024];
		DWORD err_number = GetLastError(); 

		FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, err_number, 
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)errstr, 1024, NULL );
		s= strstr(errstr, "\r\n") ;
		if (s )  *s = '\0';

		if ( err_number == EINTR )
		{	
			WLOG(INFO, "select errno %d, %s", err_number, errstr);
			goto LOOP;
		} else {
			WLOG(WARNING, "select errno %d, %s All is cleared", err_number, errstr);
		}
	#else
		if ( errno == EINTR )
		{	
			WLOG(INFO, "select errno %d, %s.", errno, strerror(errno));
			goto LOOP;
		} else {
			WLOG(WARNING, "select errno %d, %s. All is cleared", errno, strerror(errno));
		}
	#endif
		FD_ZERO(&(rd_tors.rwSet));
		FD_ZERO(&(wr_tors.rwSet));
		FD_ZERO((&ex_tors.rwSet));
	}

	if ( should_click )/*  1:??ʱ?����?, ??????ǰϵͳ?Ƚ??У?2:æ??һ??????, ??ʱ???? */
		sort();

	if ( !shouldEnd)
		goto LOOP;
}

void Sched::sort()
{	/* ??????maxfd, ??????????????Ч?? */
	int i;
	Amor::Pius timer_pius;
	struct timeb now;
	TEXTUS_LONG passed ;

	ftime(&now);
	timer_pius.ordo = Notitia::TIMER;
	timer_pius.indic = &(now.time);

	for( i = 0 ;i < infor_size ; i++)
	{
		if ( timer_infor[i].pupa == (Amor*) 0 ) break;
		WBUG("TIMER[%d] %p, status %d, since " TLONG_FMT ", interval %d", i, timer_infor[i].pupa, timer_infor[i].status, timer_infor[i].since.time, (unsigned int)(timer_infor[i].interval) );
		switch ( timer_infor[i].status )
		{
		case 0:
			timer_infor[i].pupa->facio(&timer_pius);
			break;
		case 1:
			passed = (TEXTUS_LONG) (now.time - timer_infor[i].since.time)*1000
				+ (now.millitm - timer_infor[i].since.millitm);
			if ( passed >= (TEXTUS_LONG) timer_infor[i].interval )
			{
				timer_infor[i].pupa->facio(&timer_pius);
				timer_infor[i].status = 2; /* ??ʱ????һ??֪ͨ */
			}
			break;
		case 3:
			passed = (TEXTUS_LONG) (now.time - timer_infor[i].since.time)*1000
				+ (now.millitm - timer_infor[i].since.millitm);
			if ( passed >= (TEXTUS_LONG) timer_infor[i].interval )
			{
				timer_infor[i].pupa->facio(&timer_pius);
			}
			break;
		default:
			break;
		}
	}
}
#include "hook.c"
