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
class Sched: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor* clone();

	Sched();
private:
	int maxfd;
	int timer_milli; /* 定时器毫秒数 */
	int timer_usec;	/* 轮询间隔微秒数 */
	int timer_sec;	/* 轮询间隔秒数 */

	struct Tor_Pool {
		Amor::Pius wpius;
		Describo::Criptor wtor;
		int  top; /* 堆栈顶 */
		int  house[TOR_SIZE]; /* 套接字被清后, pool中被置"空"的索引, 这是一个堆栈. */
		int cur;		/* 指示其最大索引值, 在这之后, 其Criptor为空 */
		Describo::Criptor *pool[TOR_SIZE];	/* 保存被置set的描述符,数组尺寸为FD_SETSIZE */

		fd_set rwSet;

		inline int setup(Describo::Criptor * ,int*);
		inline void clear(Describo::Criptor *, int*);
		inline void isset(fd_set*, Notitia::HERE_ORDO, int& nready);
		inline Tor_Pool ()
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
		Amor *pupa;	/* 要求给予TIMER/ALARM信号的对象指针 */
		struct timeb since;	/* 设置时间 */
		unsigned int interval;/* 时间间隔,毫秒数 */
		int status;	/* 	0: 每个时间片通知(since和interval就用着了), 
					1: 要超时通知, 只通短一次
					2: 已经超时通知过了, 以后不会再通知, 如果使用者清除则一直占着空间
				*/
		inline void clear ()
		{
			pupa = 0;
			since.time = 0;
			since.millitm = 0;
			since.timezone = 0;
			since.dstflag = 0;
			interval = 0;
			status = 0;
		}
		inline Timer_info ()
		{
			clear();
		}
	};	
	Timer_info *timer_infor;/* 要求给予定时信号的数组, 以空指针为结束标志 */
	int infor_size;		/* timer_infor尺寸 */
	
	void run();
	void sort();		//整理
	bool shouldEnd;
	struct Describo::Pendor *pendors;	//延后调用的数组；
	int pendor_size;
	int pendor_top;
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
		timer_milli = atoi(timer_str); //time_str为毫秒数

	timer_sec = timer_milli/1000;
	timer_usec = (timer_milli % 1000) * 1000;
	cfg->QueryIntAttribute("ponder", &(pendor_size));
	pendors = new struct Describo::Pendor[pendor_size];
}

bool Sched::sponte( Amor::Pius *apius)
{
	int i, tmp_type = 0;
	unsigned interval =0;
	Amor *ask_pu = (Amor *) 0;
	void **p;

	Describo::Criptor *tor;
	assert(apius);

	switch ( apius->ordo )
	{
	case Notitia::FD_SETRD :	/* 置读 */
		tor = (Describo::Criptor *)(apius->indic);	
		assert(tor);
		WBUG("%p %s %d", tor->pupa, "sponte FD_SETRD", tor->scanfd);
		i = rd_tors.setup(tor, &(tor->rd_index));

		if ( i >= 0 )
			maxfd = (maxfd > i ? maxfd:i);
		else
			WLOG(CRIT, "FD_SETRD to max %d", TOR_SIZE);
		break;

	case Notitia::FD_SETWR :	/* 置写 */
		tor = (Describo::Criptor *)(apius->indic);	
		assert(tor);
		WBUG("%p %s %d", tor->pupa, "sponte FD_SETWR", tor->scanfd);
		i = wr_tors.setup(tor, &(tor->wr_index));
		if ( i >= 0 )
			maxfd = (maxfd > i ? maxfd:i);
		else
			WLOG(CRIT, "FD_SETWR to max %d", TOR_SIZE);
		break;

	case Notitia::FD_SETEX :	/* set except fd */
		tor = (Describo::Criptor *)(apius->indic);	
		assert(tor);
		WBUG("%p %s %d", tor->pupa, "sponte FD_SETEX", tor->scanfd);
		i = ex_tors.setup(tor, &(tor->ex_index));
		if ( i >= 0 )
			maxfd = (maxfd > i ? maxfd:i);
		else
			WLOG(CRIT, "FD_SETEX to max %d", TOR_SIZE);
		break;

	case Notitia::FD_CLRRD :	/* 清可读 */
		tor = (Describo::Criptor *)(apius->indic);	
		assert(tor);
		WBUG("%p %s %d", tor->pupa, "sponte FD_CLRRD", tor->scanfd);
		rd_tors.clear(tor, &(tor->rd_index));
		break;

	case Notitia::FD_CLRWR :	/* 清可写 */
		tor = (Describo::Criptor *)(apius->indic);	
		assert(tor);
		WBUG("%p %s %d", tor->pupa, "sponte FD_CLRWR", tor->scanfd);
		wr_tors.clear(tor, &(tor->wr_index));
		break;

	case Notitia::FD_CLREX :	/* clear exception fd */
		tor = (Describo::Criptor *)(apius->indic);	
		assert(tor);
		WBUG("%p %s %d", tor->pupa, "sponte FD_CLREX", tor->scanfd);
		ex_tors.clear(tor, &(tor->ex_index));
		break;

	case Notitia::DMD_SET_TIMER :	/* 置时间片通知对象 */
	case Notitia::DMD_SET_ALARM :	/* 置超时通知对象 */
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
			interval = *((int *)(*p));
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

		if ( i == infor_size ) 	/* 空间不够，扩张之 */
		{	
			struct Timer_info *tmp = timer_infor;
			timer_infor = new struct Timer_info [infor_size*2];	/* 分配2倍的空间 */
			memcpy(timer_infor, tmp, sizeof(struct Timer_info) * infor_size);/* 保存旧的数据 */
			infor_size = infor_size*2;	/* 尺寸值加倍 */
		}

		timer_infor[i].pupa = ask_pu;
		timer_infor[i].status = tmp_type;	
		ftime(&(timer_infor[i].since));
		timer_infor[i].interval = interval;	
		tm_hd_ps.indic = ask_pu;
		ask_pu->facio(&tm_hd_ps);
		break;

	case Notitia::DMD_CLR_TIMER :	/* 清定时通知对象 */
		WBUG("%p sponte DMD_CLR_TIMER", apius->indic);
		if ( apius->indic == 0  ) break;
		for( i = 0 ;i < infor_size; i++)
		{
			int j;
			if ( timer_infor[i].pupa != (Amor*)(apius->indic) ) continue;
			for ( j = i; j < infor_size-1; j++) /* 留下一个空位置, 前移 */
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

bool Sched::facio( Amor::Pius *pius)
{
	assert(pius);
	switch ( pius->ordo )
	{
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

Amor* Sched::clone()
{
	return (Amor*)this;
}

int Sched::Tor_Pool::setup (Describo::Criptor *tor, int *index)
{
	if ( *index >= 0 ) goto GIVE;	/* 不怕重复设置 */
	if ( top >=0 )
	{	/* house中指示, pool中已有被空出来的位置,找这个空位置即可 */
		*index = house[top]; 
		pool[house[top]] = tor; /* rd_house[rd_top]为空位置索引号 */
		top--;	/* 退栈 */

	} else if (cur < TOR_SIZE) { 	/* 看来前面的都满了, 排一个新位置 */
		*index =  cur;
		pool[*index] = tor;
		cur++;

	} else 	/* 系统达到最大值, 返错 */
		return -1;

	FD_SET(tor->scanfd, &rwSet);
GIVE:
	return tor->scanfd;
}

void Sched::Tor_Pool::clear (Describo::Criptor *tor, int *index)
{
	FD_CLR(tor->scanfd, &rwSet);
	if ( *index < 0 ) return;

	pool[*index] = (Describo::Criptor *)0; /* 腾出"读"位置 */
	if ( cur - *index == 1 ) /* 刚好是在最后的那一个 */
	{
		cur--;
	} else {	/* 腾出的是中间的位置, 将之置入堆栈, 以便将来用 */
		top++;
		house[top] = *index;
	}
	*index = -1;	/* 避免再次操作 */
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
	timer_usec = 50*1000;	//默认50毫秒
	timer_sec = 0;	//默认50毫秒

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
void Sched:: run()
{
bool should_click;
	int nready;
	fd_set rset;
	fd_set wset;
	fd_set eset;
	struct timeval tv;	

	int busy = 0 ;	//忙计数, 一旦有空, 此数复位为0 

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
	nready = select((maxfd+1), 
		rd_tors.cur == 0 ? NULL: &rset,
		wr_tors.cur == 0 ? NULL: &wset, 
		ex_tors.cur == 0 ? NULL: &eset, &tv); 
	WBUG("nready %d", nready);
	if (nready > 0)
	{
		int nre = nready;
		/* 先找可读的 */
		rd_tors.isset(&rset, Notitia::FD_PRORD, nready);	
		/* 再找可写的 */
		if ( nready > 0 ) wr_tors.isset(&wset, Notitia::FD_PROWR, nready);	
		/* 再找异常的 */
		if ( nready > 0 ) ex_tors.isset(&eset, Notitia::FD_PROEX, nready);	

		/* select等待用去多少时间, 毫秒数, 计到busy中 */
		busy += timer_milli - tv.tv_sec*1000 - tv.tv_usec/1000 ;
		/* 处理各个套接字用去多少时间, 毫秒数, 40是经验值 */
		busy +=  (nre/40);
		if ( busy > timer_milli) 
		{
			busy = 0;
			should_click = true;
		}
	} else if ( nready == 0) {
		should_click = true;
		busy =0;
	} else {	/* 发生了错误 */
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

	if ( should_click )/*  1:定时器触发, 表明当前系统比较闲，2:忙够一定数了, 够时间了 */
		sort();

	if ( !shouldEnd)
		goto LOOP;
}

void Sched::sort()
{	/* 重新算maxfd, 数组整理以提高效率 */
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
				timer_infor[i].status = 2; /* 超时仅作一次通知 */
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

