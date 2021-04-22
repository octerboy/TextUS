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

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
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
	Amor::Pius clr_timer_pius, alarm_pius;	/* 清超时, 设超时 */
	Amor::Pius local_pius;
	Describo::Criptor mytor; //保存套接字, 各实例不同
	DPoll::Pollor pollor; /* 保存事件句柄, 各子实例不同 */
	Amor::Pius epl_set_ps, epl_clr_ps, pro_tbuf_ps;

	Tcpcli *tcpcli;
	char errMsg[2048];

	bool has_config;
	struct G_CFG {
		bool block_mode;	//是否为阻塞, 默认为非阻塞
		bool on_start;
		bool on_start_poineer;
		int try_interval;	//连接失败后，下一次再发起连接的时间间隔(秒)
		bool use_epoll;
		Amor *sch;
		struct DPoll::PollorBase lor; /* 探询 */
		G_CFG() {
			block_mode = false;
			on_start =  true;
			try_interval = 0;
			on_start_poineer = true;
			lor.type = DPoll::NotUsed;
			sch = 0;
		};
	};
	struct G_CFG *gCFG;
	void *arr[3];

	void transmit();
	void transmit_ep();
	void establish_done();
	void establish();
	void deliver(Notitia::HERE_ORDO aordo);
	void end(bool outer=false);
	void release();
	void errpro();
	void rcv_pro(TEXTUS_LONG len, const char *msg, bool outer=false);

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
		gCFG->on_start = false;	/* 并非一开始就启动 */

	gCFG->on_start_poineer = gCFG->on_start;
	if ( (on_start_str = cfg->Attribute("start_poineer") ) && strcasecmp(on_start_str, "no") ==0 )
		gCFG->on_start_poineer = false;	/* 并非一开始就启动 */

	if( (try_str = cfg->Attribute("try")) )
	{
		if ( atoi(try_str) > 0 )
			gCFG->try_interval = atoi(try_str);
	}

	if ( (comm_str = cfg->Attribute("block") ) && strcasecmp(comm_str, "yes") ==0 )
		gCFG->block_mode = true;	/* 并非一开始就启动 */
}

bool Tcpcliuna::facio( Amor::Pius *pius)
{
	const char *ip_str;
	TBuffer **tb;
	TiXmlElement *cfg;
	Amor::Pius tmp_p;
	TEXTUS_LONG len;

#if defined (_WIN32 )	
	OVERLAPPED_ENTRY *aget;
#endif	//for WIN32

	assert(pius);
	switch (pius->ordo)
	{
	case Notitia::PRO_TBUF :
		WBUG("facio PRO_TBUF");
		if ( tcpcli->connfd == INVALID_SOCKET || tcpcli->isConnecting )
		{
			Amor::Pius info_pius;
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
		/* 已经建立连接 */
		if ( aget->lpOverlapped == (void*)&(tcpcli->rcv_ovp) )
		{
			establish_done();
		} else {
			WLOG(ALERT, "accept_epoll: not my overlap");
		}
#else
		establish_done();
#endif
		break;

	case Notitia::FD_PRORD:
		WBUG("facio FD_PRORD");
		/* 读数据, 这是最常的, 只有不失败并有数据才向左节点传递 */
		if ( tcpcli->isConnecting) //试图完成连接
			establish_done();
		else {
			rcv_pro(tcpcli->recito(), "FD_PROFD recv bytes", true);
		}
		break;

	case Notitia::ERR_EPOLL:
		WBUG("facio ERR_EPOLL");
		WLOG(WARNING, (char*)pius->indic);	
		end(true);	//直接关闭就可.
		break;

#if defined (_WIN32 )	
	case Notitia::MORE_DATA_EPOLL:
		aget = (OVERLAPPED_ENTRY *)pius->indic;
		WLOG(WARNING, "facio MORE_DATA_EPOLL received %d bytes", aget->dwNumberOfBytesTransferred);
		if ( aget->lpOverlapped == &(tcpcli->rcv_ovp) )
		{	
			tcpcli->wsa_rcv.len *= 2;
			tcpcli->rcv_frame_size =  tcpcli->wsa_rcv.len;
			tcpcli->rcv_buf->commit(aget->dwNumberOfBytesTransferred);
			aptus->sponte(&pro_tbuf_ps);
			if ( !tcpcli->recito_ex())
			{
				SLOG(ERR)
				end();	//失败即关闭
			}
		} else {
			WLOG(ALERT, "not my overlap");
		}
		break;
#endif
	case Notitia::PRO_EPOLL:
		WBUG("facio PRO_EPOLL");
#if defined (_WIN32 )	
		aget = (OVERLAPPED_ENTRY *)pius->indic;
		if ( aget->lpOverlapped == &(tcpcli->rcv_ovp) )
		{	//已读数据,  不失败并有数据才向接力者传递
			if ( aget->dwNumberOfBytesTransferred ==0 ) 
			{
				WLOG(INFO, "IOCP recv 0 disconnected");
				end();
			} else {
				WBUG("PRO_EPOLL recv %d bytes", aget->dwNumberOfBytesTransferred);
				tcpcli->rcv_buf->commit(aget->dwNumberOfBytesTransferred);
				aptus->sponte(&pro_tbuf_ps);
				if ( !tcpcli->recito_ex())
				{
					SLOG(ERR)
					end();	//失败即关闭
				}
			}
		} else if ( aget->lpOverlapped == &(tcpcli->snd_ovp) ) {
			WBUG("client PRO_EPOLL sent %d bytes", aget->dwNumberOfBytesTransferred); //写数据完成
		} else {
			WLOG(ALERT, "not my overlap");
		}
#endif
		break;

	case Notitia::RD_EPOLL:
		WBUG("facio RD_EPOLL");
LOOP:
		switch ( (len = tcpcli->recito()) ) 
		{
		case 0:	//Pending
			SLOG(INFO)
			/* action flags and filter for event remain unchanged */
			gCFG->sch->sponte(&epl_set_ps);	//向tpoll,  再一次注册
			break;

		case -1://Close
			SLOG(INFO)
			end();	//失败即关闭
			break;

		case -2://Error
			SLOG(NOTICE)
			end();	//失败即关闭
			break;

		default:	
			WBUG("client recv " TLONG_FMT " bytes", len);
			if ( len <  tcpcli->rcv_frame_size ) { 
				/* action flags and filter for event remain unchanged */
				gCFG->sch->sponte(&epl_set_ps);	//向tpoll,  再一次注册
				aptus->sponte(&pro_tbuf_ps);
			} else if (  len == tcpcli->rcv_frame_size ) {
				aptus->sponte(&pro_tbuf_ps);
				goto LOOP;
			} else if (  len > tcpcli->rcv_frame_size ) {
				WLOG(EMERG, "client recv %ld bytes > rcv_size %d", len, tcpcli->rcv_frame_size);
			}
			break;
		}
		break;

	case Notitia::WR_EPOLL:
		WBUG("facio WR_EPOLL");
		if ( tcpcli->isConnecting) //试图完成连接
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
		//写, 很少见, 除非系统很忙
		if ( tcpcli->isConnecting) //试图完成连接
			establish_done();
		else
			transmit();
		break;

	case Notitia::FD_PROEX:
		WBUG("facio FD_PROEX");
		if ( tcpcli->isConnecting) //试图完成连接
			establish_done();
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		gCFG->sch->sponte(&clr_timer_pius); /* 清除定时 */
		end(true);
		break;

	case Notitia::CMD_RELEASE_SESSION:
		WBUG("facio CMD_RELEASE_SESSION");
		release();
		break;

	case Notitia::TIMER:
		WBUG("facio TIMER");
		if ( tcpcli->connfd == INVALID_SOCKET )
		{	//最近发生一次连接, 而且连接失败, 间隔时间到达设定值
			establish();		//开始建立连接
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
		//arr[2] = &(gCFG->try_interval);
		arr[2] = 0;
#if defined (_WIN32)
		if ( !tcpcli->sock_start() ) 
		{
			SLOG(EMERG);
		}
#endif
		tmp_p.ordo = Notitia::CMD_GET_SCHED;
		tmp_p.indic = 0;
		aptus->sponte(&tmp_p);	//向tpoll, 取得sched
		gCFG->sch = (Amor*)tmp_p.indic;
		if ( !gCFG->sch ) 
		{
			WLOG(ERR, "no sched or tpoll");
			break;
		}
		tmp_p.ordo = Notitia::POST_EPOLL;
		tmp_p.indic = &gCFG->lor;
		gCFG->lor.pupa = this;
		
		gCFG->sch->sponte(&tmp_p);	//向tpoll, 取得TPOLL
		if ( tmp_p.indic == gCFG->sch )
			gCFG->use_epoll = true;
		else
			gCFG->use_epoll = false;


		if ( gCFG->on_start_poineer )
			establish();		//开始建立连接

		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		arr[0] = this;
		arr[1] = &(gCFG->try_interval);
		//arr[2] = &(gCFG->try_interval);
		arr[2] = 0;
		if ( gCFG->on_start )
			establish();		//开始建立连接

		break;

	case Notitia::SET_TBUF:	/* 取得输入TBuffer地址 */
		WBUG("facio SET_TBUF");
		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//当然tb不能为空
			if ( *tb) 
			{	//新到请求的TBuffer
				tcpcli->snd_buf = *tb;
			}
			tb++;
			if ( *tb) tcpcli->rcv_buf = *tb;
#if defined(_WIN32)
			tcpcli->wsa_snd.buf = (char *)tcpcli->snd_buf->base;
			tcpcli->wsa_rcv.len = RCV_FRAME_SIZE;
#endif
		} else 
			WLOG(NOTICE,"facio PRO_TBUF null.");
		break;

	case Notitia::DMD_START_SESSION:
		WBUG("facio DMD_START_SESSION");
		gCFG->sch->sponte(&clr_timer_pius); /* 清除定时 */
		establish();		//开始建立连接
		break;

	case Notitia::CMD_GET_FD:	//给出套接字描述符
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
			gCFG->sch->sponte(&epl_clr_ps); //向tpoll,  注销
		} else {
			 deliver(Notitia::FD_CLRRD);
		}
		break;

	case Notitia::CMD_CHANNEL_RESUME :
		WBUG("sponte CMD_CHANNEL_RESUME");
		if ( gCFG->use_epoll)
		{
			gCFG->sch->sponte(&epl_set_ps); //向tpoll,  注册
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
		gCFG->sch->sponte(&clr_timer_pius); /* 清除定时 */
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
	epl_clr_ps.ordo = Notitia::CLR_EPOLL;
	epl_clr_ps.indic = &pollor;
	epl_set_ps.ordo = Notitia::SET_EPOLL;
	epl_set_ps.indic = &pollor;
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
	clr_timer_pius.indic = 0;

	alarm_pius.ordo = Notitia::DMD_SET_ALARM;
	alarm_pius.indic = &arr[0];
#if  defined(__linux__)
	pollor.ev.data.ptr = &pollor;
#endif	//for linux

}

Tcpcliuna::~Tcpcliuna()
{	
	gCFG->sch->sponte(&clr_timer_pius); /* 清除定时 */
	delete tcpcli;
	if (has_config )
		delete gCFG;
}

void Tcpcliuna::establish()
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
		gCFG->sch->sponte(&epl_set_ps);	//向tpoll
		if ( tcpcli->annecto_ex() )
		{
			if ( tcpcli->isConnecting) return;
			establish_done();
			if ( !tcpcli->recito_ex())
			{
				SLOG(ERR)
				end();	//失败即关闭
				return;
			}
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

		gCFG->sch->sponte(&epl_set_ps);	//向tpoll

#if  defined(__linux__)
		pollor.op = EPOLL_CTL_MOD; //以后操作就是修改了。
#endif	//for linux

#endif	//end for WIN32
	} else {	//select model
		if ( tcpcli->clio(gCFG->block_mode) && tcpcli->annecto() )
		{
			if ( tcpcli->isConnecting) 
			{	//正连接中,则向schedule登记
				mytor.scanfd = tcpcli->connfd;
				deliver(Notitia::FD_SETWR);
				deliver(Notitia::FD_SETRD);
#if defined (_WIN32 )	
				deliver(Notitia::FD_CLREX);
#endif
			} else  { /* 连接完成 */
				establish_done();
			}
		} else {
			errpro();
		}
	}
}

void Tcpcliuna::establish_done()
{
	if (tcpcli->isConnecting ) if ( !tcpcli->annecto_done())
	{	//在建立连接的过程中出错
		errpro();
		end();	/* 自动重连 */
		return ;
	}

	if ( gCFG->use_epoll ) 
	{
#if defined (_WIN32 )	
		pollor.hnd.sock = tcpcli->connfd;
		pollor.pro_ps.ordo = Notitia::PRO_EPOLL;
#else //other unix like 
		pollor.pro_ps.ordo = Notitia::RD_EPOLL;
#endif	//for WIN32

#if  defined(__linux__)
		pollor.ev.events &= ~EPOLLOUT;
#endif	//for linux

#if  defined(__sun)
		pollor.events &= ~POLLOUT;
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		pollor.events[1].flags = EV_ADD | EV_DISABLE;
#endif	//for bsd

		gCFG->sch->sponte(&epl_set_ps);	//向tpoll
#if defined (_WIN32 )	
		/* 主动去接收, 如果一开始有数据, 则先接收; 另外实现IOCP投递 */
		if ( !tcpcli->recito_ex())
		{
			SLOG(ERR)
			end();	//失败即关闭
			return;
		}
#endif	//for WIN32
	} else {
		mytor.scanfd = tcpcli->connfd;
		deliver(Notitia::FD_SETRD);
		deliver(Notitia::FD_CLRWR);
		deliver(Notitia::FD_CLREX);
	}

	/* TCP接收(发送)缓冲区清空 */
	//if ( tcpcli->rcv_buf) tcpcli->rcv_buf->reset();	
	//if ( tcpcli->snd_buf) tcpcli->snd_buf->reset();
	gCFG->sch->sponte(&clr_timer_pius); /* 清除定时 */
	WLOG(INFO, "estabish %s:%d ok!", tcpcli->server_ip, tcpcli->server_port);
	deliver(Notitia::START_SESSION); //向接力者发出通知, 本对象开始
}

void Tcpcliuna::transmit()
{
	int ret;
	ret = tcpcli->transmitto() ;
	WBUG("transmit ret %d", ret);
	switch ( ret )
	{
	case 0: //没有阻塞, 保持不变
		break;
	case 2: //原有阻塞, 没有阻塞了, 清一下
		errpro();
		local_pius.ordo =Notitia::FD_CLRWR;
		gCFG->sch->sponte(&local_pius);	
		break;
	case 1:	//新写阻塞, 需要设一下了
		errpro();
		local_pius.ordo =Notitia::FD_SETWR;
		gCFG->sch->sponte(&local_pius);	
		break;
	case 3:	//还是写阻塞, 不变
		errpro();
		break;
	case -1://有严重错误, 关闭
		errpro();
		end(true);	/* 不再自动重连 */
		break;
	default:
		break;
	}

	return ;
}

void Tcpcliuna::transmit_ep()
{
#if defined (_WIN32 )
	switch ( tcpcli->transmitto_ex() )
#else
	switch ( tcpcli->transmitto() )
#endif
	{
	case 0: //没有阻塞, 不变
		break;
	
	case 2: //原有阻塞, 没有阻塞了, 清一下
#if  defined(__linux__)
		pollor.ev.events &= ~EPOLLOUT;	//等下一次设置POLLIN时不再设
#endif	//for linux

#if  defined(__sun)
		pollor.events &= ~POLLOUT;	//等下一次设置POLLIN时不再设
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		pollor.events[1].flags = EV_ADD | EV_DISABLE;	//等下一次设置时不再设
#endif	//for bsd
		break;
		
	case 1:	//新写阻塞, 需要设一下了
		SLOG(INFO)
	case 3:	//还是写阻塞, 再设
#if  defined(__linux__)
		pollor.ev.events |= EPOLLOUT;
#endif	//for linux

#if  defined(__sun)
		pollor.events |= POLLOUT;
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		pollor.events[1].flags = EV_ADD | EV_ONESHOT;
#endif	//for bsd

#if !defined(_WIN32 )
		gCFG->sch->sponte(&epl_set_ps);	//向tpoll, 以设置kqueue等
#endif
		break;
	
	case -1://有严重错误, 关闭
		SLOG(WARNING)
		end(true);
		break;

	default:
		break;
	}
}

void Tcpcliuna::end(bool outer)
{
	WBUG("end%s", outer? " (won't connect again)" : (gCFG->try_interval > 0 ? " (will connect again ...)" : "ed." ));
	if ( tcpcli->connfd == INVALID_SOCKET ) return;	/* 不重复关闭 */
	if ( !gCFG->use_epoll ) 
	{
		deliver(Notitia::FD_CLRWR);
		deliver(Notitia::FD_CLREX);
		deliver(Notitia::FD_CLRRD);
	}
	
	tcpcli->end();		//Tcpcli也关闭
	if (gCFG->try_interval > 0 ) {	//时间值大于0时, 才有是否重连的考虑
		if (outer )
		{
			gCFG->sch->sponte(&clr_timer_pius); /* 清除定时, 不再重连服务端 */
		} else {
			gCFG->sch->sponte(&alarm_pius); /* 这将使得重连服务端 */
		}
	}

	deliver(Notitia::END_SESSION);/* 向左、右传递本类的会话关闭信号 */
}

void Tcpcliuna::release()
{
	WBUG("release().....");
	if ( tcpcli->connfd == INVALID_SOCKET ) return;	/* 不重复 */
	deliver(Notitia::FD_CLRWR);
	deliver(Notitia::FD_CLREX);
	deliver(Notitia::FD_CLRRD);
	
	tcpcli->end(false);		//Tcpcli也关闭
}

Amor* Tcpcliuna::clone()
{
	Tcpcliuna *child;

	child = new Tcpcliuna();
	child->gCFG = gCFG;
	tcpcli->herit(child->tcpcli);
	
	return (Amor*)child;
}

/* 向接力者提交 */
void Tcpcliuna::deliver(Notitia::HERE_ORDO aordo)
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
		gCFG->sch->sponte(&local_pius);	//向Sched
		return ;

	default:
		break;
	}
	aptus->sponte(&tmp_pius);
}

void Tcpcliuna::errpro()
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

void Tcpcliuna::rcv_pro(TEXTUS_LONG len, const char *msg, bool outer)
{
	if ( len > 0 ) 
	{
		WBUG("%s " TLONG_FMT, msg, len);
		aptus->sponte(&pro_tbuf_ps);
	} else {
		if ( len == 0 || len == -1)	/* 记日志 */
		{
			SLOG(INFO)
		} else
		{
			SLOG(NOTICE)
		}
		if ( len < 0 )  end(outer);	//失败即关闭
	}
}
#include "hook.c"
