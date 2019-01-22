/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: TCP client to Textus
 Build: created by octerboy, 2005/06/10
 $Id$
*/
#define SCM_MODULE_ID	"$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Tcpcli.h"
#include "DPoll.h"
#include "Amor.h"
#include "Notitia.h"
#include "textus_string.h"
#include <time.h>
#include "Describo.h"
#include <stdarg.h>

#ifndef TINLINE
#define TINLINE inline
#endif 

#if defined( _MSC_VER ) && (_MSC_VER < 1400 )
typedef unsigned int* ULONG_PTR;
typedef struct _OVERLAPPED_ENTRY {
	ULONG_PTR lpCompletionKey;
	LPOVERLAPPED lpOverlapped;
	ULONG_PTR Internal;
	DWORD dwNumberOfBytesTransferred;
} OVERLAPPED_ENTRY, *LPOVERLAPPED_ENTRY;
#endif	//for WIN32

class Tcpcliuna: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	Tcpcliuna();
	~Tcpcliuna();

private:
	Amor::Pius clr_timer_pius, alarm_pius;	/* �峬ʱ, �賬ʱ */
	Amor::Pius local_pius;
	Amor::Pius info_pius;
	Describo::Criptor mytor; //�����׽���, ��ʵ����ͬ
	DPoll::Pollor pollor; /* �����¼����, ����ʵ����ͬ */
	Amor::Pius epl_set_ps, epl_clr_ps, pro_tbuf_ps;

	Tcpcli *tcpcli;
	char errMsg[2048];

	bool has_config;
	struct G_CFG {
		bool block_mode;	//�Ƿ�Ϊ����, Ĭ��Ϊ������
		bool on_start;
		bool on_start_poineer;
		int try_interval;	//����ʧ�ܺ���һ���ٷ������ӵ�ʱ����(��)
		bool use_epoll;
		struct DPoll::PollorBase lor; /* ̽ѯ */
		inline G_CFG() {
			block_mode = false;
			on_start =  true;
			try_interval = 0;
			on_start_poineer = true;
			lor.type = DPoll::NotUsed;
		};
	};
	struct G_CFG *gCFG;
	void *arr[3];

	TINLINE void transmit();
	TINLINE void transmit_ep();
	TINLINE void establish_done();
	TINLINE void establish();
	TINLINE void deliver(Notitia::HERE_ORDO aordo);
	TINLINE void end(bool outer=false);
	TINLINE void release();
	TINLINE void errpro();
	TINLINE void rcv_pro(long len, const char *msg, bool outer=false);
	inline void do_recv_ex();

#include "wlog.h"
};

#include <assert.h>
#include <stdio.h>
#include "textus_string.h"
#include "casecmp.h"

#define SLOG(Z) { Amor::Pius log_pius; \
		log_pius.ordo = Notitia::LOG_##Z; \
		log_pius.indic = &errMsg[0]; \
		aptus->sponte(&log_pius); }

void Tcpcliuna::ignite(TiXmlElement *cfg)
{
	const char *ip_str, *on_start_str, *try_str;
	const char *comm_str;

	if ( !gCFG) 
	{
		gCFG = new struct G_CFG;
		has_config = true;
	}
	
	tcpcli->setPort( cfg->Attribute("port"));
	if( (ip_str = cfg->Attribute("ip")) )
	{
		memset(tcpcli->server_ip, 0, sizeof(tcpcli->server_ip));
		TEXTUS_STRNCPY(tcpcli->server_ip, ip_str, sizeof(tcpcli->server_ip)-1);
	}

	if ( (on_start_str = cfg->Attribute("start") ) && strcasecmp(on_start_str, "no") ==0 )
		gCFG->on_start = false;	/* ����һ��ʼ������ */

	gCFG->on_start_poineer = gCFG->on_start;
	if ( (on_start_str = cfg->Attribute("start_poineer") ) && strcasecmp(on_start_str, "no") ==0 )
		gCFG->on_start_poineer = false;	/* ����һ��ʼ������ */

	if( (try_str = cfg->Attribute("try")) && atoi(try_str) > 0 )
		gCFG->try_interval = atoi(try_str);

	if ( (comm_str = cfg->Attribute("block") ) && strcasecmp(comm_str, "yes") ==0 )
		gCFG->block_mode = true;	/* ����һ��ʼ������ */
}

bool Tcpcliuna::facio( Amor::Pius *pius)
{
	const char *ip_str;
	TiXmlElement *cfg;
	TBuffer **tb;
	Amor::Pius tmp_p;
#if defined (_WIN32 )	
	OVERLAPPED_ENTRY *aget;
#endif	//for WIN32

	assert(pius);
	switch (pius->ordo)
	{
	case Notitia::PRO_TBUF :
		WBUG("facio PRO_TBUF");
		if ( tcpcli->connfd < 0 || tcpcli->isConnecting )
		{
			info_pius.ordo = Notitia::CHANNEL_NOT_ALIVE;
			info_pius.indic = 0;
			aptus->sponte(&info_pius);
		} else if ( gCFG->use_epoll)
		{
			transmit_ep();
		} else {
			transmit();
		}
		break;

	case Notitia::ACCEPT_EPOLL:
		WBUG("facio ACCEPT_EPOLL");
#if defined (_WIN32 )	
		aget = (OVERLAPPED_ENTRY *)pius->indic;
		/* �Ѿ��������� */
		if ( aget->lpOverlapped == (void*)&(tcpcli->rcv_ovp) )
		{
			establish_done();
		} else {
			WLOG(ALERT, "parent_pro: not my overlap");
		}
#else
		establish_done();
#endif
		break;

	case Notitia::FD_PRORD:
		WBUG("facio FD_PRORD");
		/* ������, �������, ֻ�в�ʧ�ܲ������ݲ�����ڵ㴫�� */
		if ( tcpcli->isConnecting) //��ͼ�������
			establish_done();
		else {
			rcv_pro(tcpcli->recito(), "FD_PROFD recv bytes", true);
		}
		break;

	case Notitia::ERR_EPOLL:
		WBUG("facio ERR_EPOLL");
		WLOG(WARNING, (char*)pius->indic);	
		end(true);	//ֱ�ӹرվͿ�.
		break;

	case Notitia::PRO_EPOLL:
		WBUG("facio PRO_EPOLL");
#if defined (_WIN32 )	
		aget = (OVERLAPPED_ENTRY *)pius->indic;
		if ( aget->lpOverlapped == &(tcpcli->rcv_ovp) )
		{	//�Ѷ�����,  ��ʧ�ܲ������ݲ�������ߴ���
			if ( aget->dwNumberOfBytesTransferred ==0 ) 
			{
				WLOG(INFO, "IOCP recv 0 disconnected");
				end();
			} else {
				rcv_pro( aget->dwNumberOfBytesTransferred , "PRO_EPOLL recv bytes", true);
			}
		} else if ( aget->lpOverlapped == &(tcpcli->snd_ovp) ) {
			//д�������
			if ( tcpcli->snd_buf->point > tcpcli->snd_buf->base )	//����д
				transmit_ep();
		} else {
			WLOG(ALERT, "not my overlap");
			break;
		}
		do_recv_ex();
#endif
		break;

	case Notitia::RD_EPOLL:
		WBUG("facio RD_EPOLL");
		do_recv_ex();
		break;

	case Notitia::WR_EPOLL:
		WBUG("facio WR_EPOLL");
		if ( tcpcli->isConnecting) //��ͼ�������
			establish_done();
		else 
			transmit_ep();
		break;

	case Notitia::EOF_EPOLL:
		WBUG("facio EOF_EPOLL");
		WLOG(INFO, "peer disconnected.");
		end();
		break;

	case Notitia::FD_PROWR:
		WBUG("facio FD_PROWR");
		//д, ���ټ�, ����ϵͳ��æ
		if ( tcpcli->isConnecting) //��ͼ�������
			establish_done();
		else
			transmit();
		break;

	case Notitia::FD_PROEX:
		WBUG("facio FD_PROEX");
		if ( tcpcli->isConnecting) //��ͼ�������
			establish_done();
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		end(true);
		break;

	case Notitia::CMD_RELEASE_SESSION:
		WBUG("facio CMD_RELEASE_SESSION");
		release();
		break;

	case Notitia::TIMER:
		WBUG("facio TIMER");
		if ( tcpcli->connfd == -1 )
		{	//�������һ������, ��������ʧ��, ���ʱ�䵽���趨ֵ
			establish();		//��ʼ��������
		}
		break;

	case Notitia::TIMER_HANDLE:
		WBUG("facio TIMER_HANDLE");
		clr_timer_pius.indic = pius->indic;
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		arr[0] = this;
		arr[1] = &(gCFG->try_interval);
		arr[2] = &(gCFG->try_interval);
#if defined (_WIN32)
		if ( !tcpcli->sock_start() ) 
		{
			SLOG(EMERG);
		}
#endif
		tmp_p.ordo = Notitia::POST_EPOLL;
		tmp_p.indic = &gCFG->lor;
		gCFG->lor.pupa = this;
		
		aptus->sponte(&tmp_p);	//��tpoll, ȡ��TPOLL
		if ( tmp_p.indic )
			gCFG->use_epoll = true;
		else
			gCFG->use_epoll = false;


		if ( gCFG->on_start_poineer )
			establish();		//��ʼ��������

		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		arr[0] = this;
		arr[1] = &(gCFG->try_interval);
		arr[2] = &(gCFG->try_interval);
		if ( gCFG->on_start )
			establish();		//��ʼ��������

		break;

	case Notitia::SET_TBUF:	/* ȡ������TBuffer��ַ */
		WBUG("facio SET_TBUF");
		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//��Ȼtb����Ϊ��
			if ( *tb) 
			{	//�µ������TBuffer
				tcpcli->snd_buf = *tb;
			}
			tb++;
			if ( *tb) tcpcli->rcv_buf = *tb;
		} else 
			WLOG(NOTICE,"facio PRO_TBUF null.");
		break;

	case Notitia::DMD_START_SESSION:
		WBUG("facio DMD_START_SESSION");
		establish();		//��ʼ��������
		break;

	case Notitia::CMD_GET_FD:	//�����׽���������
		WBUG("sponte CMD_GET_FD");		
		pius->indic = &(tcpcli->connfd);
		break;

	case Notitia::CMD_SET_PEER:
		ip_str = 0;
		cfg = (TiXmlElement *)(pius->indic);
		if( !cfg)
		{
			WLOG(WARNING, "CMD_SET_PEER cfg is null");
			break;
		}

		tcpcli->setPort(cfg->Attribute("port"));
		ip_str = cfg->Attribute("ip");
		if( ip_str )
		{
			memset(tcpcli->server_ip, 0, sizeof(tcpcli->server_ip));
			TEXTUS_STRNCPY(tcpcli->server_ip, ip_str, sizeof(tcpcli->server_ip)-1);
		}

		WLOG(INFO, "facio CMD_SET_PEER, %s:%d", (ip_str == 0) ? "(null)":ip_str, tcpcli->server_port);
		break;

	case Notitia::CMD_CHANNEL_PAUSE :
		WBUG("facio CMD_CHANNEL_PAUSE");
		if ( gCFG->use_epoll)
		{
		} else {
			deliver(Notitia::FD_CLRRD);
		}
		break;

	case Notitia::CMD_CHANNEL_RESUME :
		WBUG("sponte CMD_CHANNEL_RESUME");
		if ( gCFG->use_epoll)
		{
		} else {
			deliver(Notitia::FD_SETRD);
		}
		break;

	default:
		return false;
	}	
	return true;
}

bool Tcpcliuna::sponte( Amor::Pius *pius) 
{ 
	switch (pius->ordo)
	{
	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		end(true);
		break;

	case Notitia::CMD_RELEASE_SESSION:
		WBUG("sponte CMD_RELEASE_SESSION");
		release();
		break;

	default:
		return false;
	}	
	return true;
}

Tcpcliuna::Tcpcliuna()
{
	pollor.pupa = this;
	pollor.type = DPoll::Sock;
	epl_set_ps.ordo = Notitia::SET_EPOLL;
	epl_set_ps.indic = &pollor;
	epl_clr_ps.ordo = Notitia::CLR_EPOLL;
	epl_clr_ps.indic = &pollor;
	pro_tbuf_ps.ordo = Notitia::PRO_TBUF;
	pro_tbuf_ps.indic = 0;

	mytor.pupa = this;
	local_pius.ordo = Notitia::TEXTUS_RESERVED;
	local_pius.indic = &mytor;

	tcpcli = new Tcpcli();
	tcpcli->errMsg = &errMsg[0];
	tcpcli->errstr_len = 2048;
	memset(tcpcli->server_ip, 0 , sizeof(tcpcli->server_ip));

	gCFG = 0;
	has_config = false;
	clr_timer_pius.ordo = Notitia::DMD_CLR_TIMER;
	clr_timer_pius.indic = this;

	alarm_pius.ordo = Notitia::DMD_SET_ALARM;
	alarm_pius.indic = &arr[0];
#if  defined(__linux__)
	pollor.ev.data.ptr = &pollor;
#endif	//for linux

}

Tcpcliuna::~Tcpcliuna()
{	
	aptus->sponte(&clr_timer_pius); /* �����ʱ */
	delete tcpcli;
	if (has_config )
		delete gCFG;
}

TINLINE void Tcpcliuna::establish()
{
	WLOG(INFO, "estabish to %s:%d  .....", tcpcli->server_ip, tcpcli->server_port);

	if ( gCFG->use_epoll ) 
	{
		if (!tcpcli->clio(false))
		{
			errpro();
			return ;
		}
		pollor.pro_ps.ordo = Notitia::ACCEPT_EPOLL;
#if defined (_WIN32 )	
		pollor.hnd.sock = tcpcli->connfd;
		aptus->sponte(&epl_set_ps);	//��tpoll
		if ( tcpcli->annecto_ex() )
		{
			if ( tcpcli->isConnecting) return;
			establish_done();
			do_recv_ex();
		} else {
			errpro();
		}
#else	//other unix like

		if ( tcpcli->annecto() )
		{
			if ( !tcpcli->isConnecting)
				establish_done();
		} else {
			errpro();
		}
#if  defined(__linux__)
		pollor.fd = tcpcli->connfd;
		pollor.ev.events = EPOLLIN | EPOLLET |EPOLLONESHOT | EPOLLRDHUP ;
		if ( tcpcli->isConnecting)
			pollor.ev.events |= EPOLLOUT;
		pollor.op = EPOLL_CTL_ADD;
#endif	//for linux

#if  defined(__sun)
		pollor.fd = tcpcli->connfd;
		pollor.events = POLLIN;
		if ( tcpcli->isConnecting)
			pollor.events |= POLLOUT;
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		EV_SET(&(pollor.events[0]), tcpcli->connfd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, &pollor);
		if ( tcpcli->isConnecting)
			EV_SET(&(pollor.events[1]), tcpcli->connfd, EVFILT_WRITE, EV_ADD , 0, 0, &pollor);
		else
			EV_SET(&(pollor.events[1]), tcpcli->connfd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, &pollor);
#endif	//for bsd

		aptus->sponte(&epl_set_ps);	//��tpoll

#if  defined(__linux__)
		pollor.op = EPOLL_CTL_MOD; //�Ժ���������޸��ˡ�
#endif	//for linux

#endif	//end for WIN32
	} else {	//select model
		if ( tcpcli->clio(gCFG->block_mode) && tcpcli->annecto() )
		{
			if ( tcpcli->isConnecting) 
			{	//��������,����schedule�Ǽ�
				mytor.scanfd = tcpcli->connfd;
				deliver(Notitia::FD_SETWR);
				deliver(Notitia::FD_SETRD);
#if defined (_WIN32 )	
				deliver(Notitia::FD_CLREX);
#endif
			} else  { /* ������� */
				establish_done();
			}
		} else {
			errpro();
		}
	}
}

TINLINE void Tcpcliuna::establish_done()
{
	if (tcpcli->isConnecting ) if ( !tcpcli->annecto_done())
	{	//�ڽ������ӵĹ����г���
		errpro();
		end();	/* �Զ����� */
		return ;
	}

	if ( gCFG->use_epoll ) 
	{
		pollor.pro_ps.ordo = Notitia::RD_EPOLL;
#if defined (_WIN32 )	
		/* ����ȥ����, ���һ��ʼ������, ���Ƚ���; ����ʵ��IOCPͶ�� */
		do_recv_ex();
#else //other unix like 
#if  defined(__linux__)
		pollor.ev.events &= ~EPOLLOUT;
#endif	//for linux

#if  defined(__sun)
		pollor.events &= ~POLLOUT;
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		pollor.events[1].flags = EV_ADD | EV_DISABLE;
#endif	//for bsd

		aptus->sponte(&epl_set_ps);	//��tpoll
#endif	//for WIN32
	} else {
		mytor.scanfd = tcpcli->connfd;
		deliver(Notitia::FD_SETRD);
		deliver(Notitia::FD_CLRWR);
		deliver(Notitia::FD_CLREX);
	}

	/* TCP����(����)��������� */
	//if ( tcpcli->rcv_buf) tcpcli->rcv_buf->reset();	
	//if ( tcpcli->snd_buf) tcpcli->snd_buf->reset();
	aptus->sponte(&clr_timer_pius); /* �����ʱ */
	WLOG(INFO, "estabish %s:%d ok!", tcpcli->server_ip, tcpcli->server_port);
	deliver(Notitia::START_SESSION); //������߷���֪ͨ, ������ʼ
}

TINLINE void Tcpcliuna::transmit()
{
	int ret;
	ret = tcpcli->transmitto() ;
	WBUG("transmit ret %d", ret);
	switch ( ret )
	{
	case 0: //û������, ���ֲ���
		break;
	case 2: //ԭ������, û��������, ��һ��
		errpro();
		local_pius.ordo =Notitia::FD_CLRWR;
		aptus->sponte(&local_pius);	
		break;
	case 1:	//��д����, ��Ҫ��һ����
		errpro();
		local_pius.ordo =Notitia::FD_SETWR;
		aptus->sponte(&local_pius);	
		break;
	case 3:	//����д����, ����
		errpro();
		break;
	case -1://�����ش���, �ر�
		errpro();
		end(true);	/* �����Զ����� */
		break;
	default:
		break;
	}

	return ;
}

inline void Tcpcliuna::transmit_ep()
{
	switch ( tcpcli->transmitto() )
	{
	case 0: //û������, ����
		break;
	
	case 2: //ԭ������, û��������, ��һ��
#if  defined(__linux__)
		pollor.ev.events &= ~EPOLLOUT;	//����һ������POLLINʱ������
#endif	//for linux

#if  defined(__sun)
		pollor.events &= ~POLLOUT;	//����һ������POLLINʱ������
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		pollor.events[1].flags = EV_ADD | EV_DISABLE;	//����һ������ʱ������
#endif	//for bsd
		break;
		
	case 1:	//��д����, ��Ҫ��һ����
		SLOG(INFO)
	case 3:	//����д����, ����
#if  defined(__linux__)
		pollor.ev.events |= EPOLLOUT;
#endif	//for linux

#if  defined(__sun)
		pollor.events |= POLLOUT;
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		pollor.events[1].flags = EV_ADD | EV_ONESHOT;
#endif	//for bsd

		aptus->sponte(&epl_set_ps);	//��tpoll, ������kqueue��
		break;
	
	case -1://�����ش���, �ر�
		SLOG(WARNING)
		end(true);
		break;

	default:
		break;
	}
}

TINLINE void Tcpcliuna::end(bool outer)
{
	WBUG("end(%s).....", outer? "won't connect again" : "will connect again");
	if ( tcpcli->connfd == -1 ) return;	/* ���ظ��ر� */
	if ( gCFG->use_epoll ) 
	{
	} else {
		deliver(Notitia::FD_CLRWR);
		deliver(Notitia::FD_CLREX);
		deliver(Notitia::FD_CLRRD);
	}
	
	tcpcli->end();		//TcpcliҲ�ر�
	if (outer )
	{
		aptus->sponte(&clr_timer_pius); /* �����ʱ, ������������� */
	} else {
		aptus->sponte(&alarm_pius); /* �⽫ʹ����������� */
	}

	deliver(Notitia::END_SESSION);/* �����Ҵ��ݱ���ĻỰ�ر��ź� */
}

TINLINE void Tcpcliuna::release()
{
	WBUG("release().....");
	if ( tcpcli->connfd == -1 ) return;	/* ���ظ� */
	deliver(Notitia::FD_CLRWR);
	deliver(Notitia::FD_CLREX);
	deliver(Notitia::FD_CLRRD);
	
	tcpcli->end(false);		//TcpcliҲ�ر�
}

Amor* Tcpcliuna::clone()
{
	Tcpcliuna *child;

	child = new Tcpcliuna();
	child->gCFG = gCFG;
	tcpcli->herit(child->tcpcli);
	
	return (Amor*)child;
}

/* ��������ύ */
TINLINE void Tcpcliuna::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;

	switch (aordo )
	{
	case Notitia::PRO_TBUF:
		WBUG("deliver PRO_TBUF");
		break;

	case Notitia::END_SESSION:
		WBUG("deliver END_SESSION");
		aptus->facio(&tmp_pius);
		break;

	case Notitia::START_SESSION:
		WBUG("deliver START_SESSION");
		aptus->facio(&tmp_pius);
		break;

	case Notitia::FD_CLRRD:
	case Notitia::FD_CLRWR:
	case Notitia::FD_CLREX:
	case Notitia::FD_SETRD:
	case Notitia::FD_SETWR:
	case Notitia::FD_SETEX:
		local_pius.ordo =aordo;
		aptus->sponte(&local_pius);	//��Sched
		return ;

	default:
		break;
	}
	aptus->sponte(&tmp_pius);
}

TINLINE void Tcpcliuna::errpro()
{
	switch(tcpcli->err_lev) {
	case 0:
		SLOG(EMERG);
		break;
	case 1:
		SLOG(ALERT);
		break;
	case 2:
		SLOG(CRIT);
		break;
	case 3:
		SLOG(ERR);
		break;
	case 4:
		SLOG(WARNING);
		break;
	case 5:
		SLOG(NOTICE);
		break;
	case 6:
		SLOG(INFO);
		break;
	default:
		break;
	}
}

inline void Tcpcliuna::rcv_pro(long len, const char *msg, bool outer)
{
	if ( len > 0 ) 
	{
		WBUG("%s %ld", msg, len);
		aptus->sponte(&pro_tbuf_ps);
	} else {
		if ( len == 0 || len == -1)	/* ����־ */
		{
			SLOG(INFO)
		} else
		{
			SLOG(NOTICE)
		}
		if ( len < 0 )  end(outer);	//ʧ�ܼ��ر�
	}
}

inline void Tcpcliuna::do_recv_ex()
{
	long len;
LOOP:
	len = tcpcli->recito(gCFG->use_epoll);
	switch ( len ) 
	{
	case 0:	//Pending
		SLOG(INFO)
#if !defined (_WIN32 )	
		/* action flags and filter for event remain unchanged */
		aptus->sponte(&epl_set_ps);	//��tpoll,  ��һ��ע��
#endif
		return;
		break;

	case -1://Close
		SLOG(INFO)
		end();	//ʧ�ܼ��ر�
		return;
		break;

	case -2://Error
		SLOG(NOTICE)
		end(true);	//ʧ�ܼ��ر�
		return;
		break;

	default:	
		WBUG("client recv %ld bytes", len);
#if !defined (_WIN32 )	
		if ( len < RCV_FRAME_SIZE ) { 
			/* action flags and filter for event remain unchanged */
			aptus->sponte(&epl_set_ps);	//��tpoll,  ��һ��ע��
			aptus->sponte(&pro_tbuf_ps);
			return;	//len ����8192ʱ����ֹ
		}
		aptus->sponte(&pro_tbuf_ps);
#endif
		break;
	}
	goto LOOP;
}
#include "hook.c"
