/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: TCP Service to textus
 Build: created by octerboy, 2005/06/10
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */
#include "Tcpsrv.h"
#include "DPoll.h"

#include "BTool.h"
#include <assert.h>

#define SLOG(Z) { Amor::Pius log_pius; \
		log_pius.ordo = Notitia::LOG_##Z; \
		log_pius.indic = &errMsg[0]; \
		aptus->sponte(&log_pius); \
		}

#include "Notitia.h"
#include "Describo.h"

#include "Amor.h"
#include "textus_string.h"
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

class Tcpsrvuna: public Amor
{
public:
	void ignite(TiXmlElement *);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
	Tcpsrvuna();
	~Tcpsrvuna();
	
private:
	Amor::Pius clr_timer_pius, alarm_pius;	/* 清超时, 设超时 */
	bool has_config;
	struct G_CFG {
		int down_delay; 
		bool on_start ;
		bool lonely;
		int back_log;
		bool use_epoll;
		Amor *sch;
		struct DPoll::PollorBase lor; /* 探询 */
		G_CFG() {
			down_delay = 3000; 	//3 seconds
			on_start = true;
			back_log = 100;
			use_epoll = false;
			lor.type = DPoll::NotUsed;
			sch = 0;
			lonely = false;
		};
	};
	struct G_CFG *gCFG;
	void *arr[3];

	Amor::Pius local_pius;
	char errMsg[1024];
	
	Describo::Criptor my_tor; /* 保存套接字, 各子实例不同 */
	DPoll::Pollor pollor; /* 保存事件句柄, 各子实例不同 */
	Amor::Pius epl_set_ps, epl_clr_ps, pro_tbuf_ps;
	TBuffer *tb_arr[3];

	Tcpsrv *tcpsrv;
	Tcpsrvuna *last_child;	/* 最近一次连接的子实例 */

	bool isPioneer, isListener;
	void child_begin();
	void parent_begin();
	void end_service();	/* 关闭侦听套接字 */
	void end(bool at_once=true);	/* 关闭连接 或 只释放套接字 */
	void end_release();		/* 释放套接字 */

	Tcpsrvuna* parent_accept();
	void child_transmit_err(int);
	void child_transmit_ep_err(int);
	void deliver(Notitia::HERE_ORDO aordo);
	Tcpsrvuna* new_conn_pro();
#if defined (_WIN32)	
	void do_accept_ex();
	bool shuting;
	bool shutted;
	bool zeroed;
#endif	//for WIN32
#include "wlog.h"
};

void Tcpsrvuna::ignite(TiXmlElement *cfg)
{
	const char  *eth_str, *ip_str, *on_start_str, *comm_str;
	assert(cfg);

	if (!cfg) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
	}

	if ( (on_start_str = cfg->Attribute("start") ) && strcmp(on_start_str, "no") ==0 )
		gCFG->on_start = false; /* 并非一开始就启动 */

	if ( (comm_str = cfg->Attribute("lonely") ) && strcmp(comm_str, "yes") ==0 )
		gCFG->lonely = true; /* 在建立一个连接后, 即关闭 */

	if( (comm_str = cfg->Attribute("delay")) )
	{
		if ( atoi(comm_str) > 0 )
			gCFG->down_delay = atoi(comm_str);
	}

	/* 开始对Tcpsrv类的变量进行赋值 */
	tcpsrv->srvport = 0;
	tcpsrv->setPort( cfg->Attribute("port"));

	if ( (ip_str = cfg->Attribute("ip")) )
		TEXTUS_STRNCPY(tcpsrv->srvip, ip_str, sizeof(tcpsrv->srvip)-1);

	if ( (eth_str = cfg->Attribute("eth")) )
	{ 	/* 确定服务于哪个网口，网口设置有较高优先权 */
		char ethfile[512];
		char *myip = (char*)0;
		TEXTUS_SNPRINTF(ethfile,sizeof(ethfile), "/etc/sysconfig/network-scripts/ifcfg-%s",eth_str);
		myip = BTool::getaddr(ethfile,"IPADDR");
		if (myip)
			TEXTUS_STRNCPY(tcpsrv->srvip, myip, sizeof(tcpsrv->srvip)-1);
	}
	/* End of 设置Tcpsrv类变量 */

	isPioneer = true;	/* 以此标志自己为父实例 */
	isListener = true;	/* listener object */
}

bool Tcpsrvuna::facio( Amor::Pius *pius)
{
	TiXmlElement *cfg;
	Amor::Pius tmp_p;
	TEXTUS_LONG tlen, len;
	int ret;

#if defined (_WIN32)	
	OVERLAPPED_ENTRY *aget;
#endif	//for WIN32

	assert(pius);
	switch(pius->ordo)
	{
	case Notitia::FD_PRORD:
		WBUG("facio FD_PRORD");
		if (isListener)	
		{
			parent_accept(); /* 既然由侦听实例, 必是建立新连接 */
		} else {
			switch ( (len = tcpsrv->recito()) ) 
			{
			case 0:	//Pending
				SLOG(INFO)
				break;

			case -1://disconnected
				SLOG(INFO)
				end(false);	//!at once down & close
				break;

			case -2://Error
				SLOG(NOTICE)
				end(true);	//失败即关闭, at once
				break;

			default:	
				WBUG(" FD_PROFD recv " TLONG_FMT " bytes", len);
				aptus->facio(&pro_tbuf_ps);
				break;
			}
		}
		break;

	case Notitia::ACCEPT_EPOLL:
		WBUG("facio ACCEPT_EPOLL");
#if defined (_WIN32)	
		aget = (OVERLAPPED_ENTRY *)pius->indic;
		/* 已经建立连接 */
		if ( aget->lpOverlapped == (void*)&(tcpsrv->rcv_ovp) )
		{
			if( tcpsrv->post_accept_ex() )
			{
				new_conn_pro();
			} else {
				SLOG(INFO)
				tcpsrv->release();
			}
		} else {
			WLOG(ALERT, "parent_pro: not my overlap");
			break;
		}
		do_accept_ex();
#else
		last_child = parent_accept(); /* 既然由侦听实例, 必是建立新连接 */
		/* action flags and filter for event remain unchanged */
		gCFG->sch->sponte(&epl_set_ps);	//向tpoll,  再一次注册
		tmp_p.ordo = Notitia::RD_EPOLL;
		tmp_p.indic= 0;
		last_child->facio(&tmp_p);
#endif
		break;

	case Notitia::ERR_EPOLL:
		WBUG("facio ERR_EPOLL");
		WLOG(WARNING, (char*)pius->indic);	
		end(true);	//at once
		break;

#if defined (_WIN32)	
	case Notitia::MORE_DATA_EPOLL:
		WBUG("facio MORE_DATA_EPOLL");
		WLOG(WARNING, (char*)pius->indic);	
		tcpsrv->wsa_rcv.len *= 2;
		tcpsrv->rcv_frame_size =  tcpsrv->wsa_rcv.len;
		if ( !tcpsrv->recito_ex())
		{
			SLOG(ERR)
			end(true);	//at once
		}
		break;

	case Notitia::PRO_EPOLL:
		WBUG("facio PRO_EPOLL");
		aget = (OVERLAPPED_ENTRY *)pius->indic;
		if ( aget->lpOverlapped == &(tcpsrv->rcv_ovp) )
		{	//已读数据,  不失败并有数据才向接力者传递
			if ( aget->dwNumberOfBytesTransferred ==0 ) 
			{
				WLOG(INFO, "IOCP recv 0 byte");
				zeroed = true;
				end(false);	
			} else {
				WBUG("child PRO_EPOLL recv %d bytes", aget->dwNumberOfBytesTransferred);
				tcpsrv->m_rcv_buf.commit_ack(aget->dwNumberOfBytesTransferred);
				TBuffer::pour(*tcpsrv->rcv_buf, tcpsrv->m_rcv_buf);
				if ( !tcpsrv->recito_ex())
				{
					SLOG(ERR)
					end(true);	//at once
				}
				aptus->facio(&pro_tbuf_ps);
				return true;
			}
		} else if ( aget->lpOverlapped == &(tcpsrv->snd_ovp) ) {
			WBUG("child PRO_EPOLL sent %d bytes", aget->dwNumberOfBytesTransferred); //写数据完成
			tcpsrv->m_snd_buf.commit_ack(-(TEXTUS_LONG)aget->dwNumberOfBytesTransferred);
			if ( tcpsrv->snd_buf->point != tcpsrv->snd_buf->base )
			{
				if( (ret = tcpsrv->transmitto_ex()) ) child_transmit_ep_err(ret);
			}
		} else if ( aget->lpOverlapped == &(tcpsrv->fin_ovp) ) {
			WBUG("IOCP DisconnectEx finished"); //disconnect完成
			shutted = true;
			end(false);	//!at once
		} else {
			WLOG(ALERT, "not my overlap");
		}
		break;
#endif

	case Notitia::RD_EPOLL:
		WBUG("facio RD_EPOLL");
		tlen = 0;
LOOP:
		switch ( (len = tcpsrv->recito()) ) 
		{
		case 0:	//Pending
			SLOG(INFO)
			/* action flags and filter for event remain unchanged */
			gCFG->sch->sponte(&epl_set_ps);	//向tpoll,  再一次注册
			break;

		case -1://disconnected
			SLOG(INFO)
			end(false);	//down & close
			break;

		case -2://Error
			SLOG(NOTICE)
			end(true);	//at once
			break;

		default:	
			WBUG("RD_EPOLL recv " TLONG_FMT " bytes", len);
			if ( len <  tcpsrv->rcv_frame_size ) { 
				/* action flags and filter for event remain unchanged */
				gCFG->sch->sponte(&epl_set_ps);	//向tpoll,  再一次注册, 最多就是这个情况
				aptus->facio(&pro_tbuf_ps);
				return true;
			} else if (  len == tcpsrv->rcv_frame_size ) {
				tlen += len;
				goto LOOP;
			} else if (  len > tcpsrv->rcv_frame_size ) {
				tlen += len;
				WLOG(EMERG, "child recv %ld bytes > rcv_size %d", len, tcpsrv->rcv_frame_size);
			}
			break;
		}
		if ( tlen > 0 ) aptus->facio(&pro_tbuf_ps);
		break;

	case Notitia::WR_EPOLL:
		WBUG("facio WR_EPOLL");
#if defined (_WIN32)	
		if( (ret = tcpsrv->transmitto_ex()) )
#else
		if( (ret = tcpsrv->transmitto()) )
#endif
			child_transmit_ep_err(ret);
		break;

	case Notitia::EOF_EPOLL:
		WBUG("facio EOF_EPOLL");
		WLOG(INFO, "peer disconnected.");
		end(false); //down & close
		break;

	case Notitia::CMD_NEW_SERVICE:
		WBUG("facio CMD_NEW_SERVICE");
		if (!isPioneer)			/* 须从父实例派生出侦听实例 */
			break;
		cfg = (TiXmlElement *)(pius->indic);
		if( !cfg)
		{
			WLOG(WARNING, "CMD_NEW_SERVICE cfg is null");
			break;
		}
		tmp_p.ordo = Notitia::CMD_ALLOC_IDLE;
		tmp_p.indic = this;
		aptus->sponte(&tmp_p);
		/* 请求空闲的子实例, 新服务 */
		if ( !(tmp_p.indic) || this == ( Amor* ) (tmp_p.indic) )
		{	/* 实例没有增加, 已经到达最大连接数，故关闭刚才的连接 */
			WLOG(NOTICE, "limited connections for neo_service, to max");
		} else {
			const char *port_str, *ip_str, *eth_str;
			Tcpsrvuna *neo = (Tcpsrvuna*)(tmp_p.indic);
			neo->tcpsrv->srvport = 0;
			if ( (port_str = cfg->Attribute("port")) )
			{
				neo->tcpsrv->setPort(port_str);
			} else {
				neo->tcpsrv->srvport = tcpsrv->srvport;
			}

			if ( (ip_str = cfg->Attribute("ip")) )
				TEXTUS_STRNCPY(neo->tcpsrv->srvip, ip_str, sizeof(neo->tcpsrv->srvip)-1);
			else
				TEXTUS_STRNCPY(neo->tcpsrv->srvip, tcpsrv->srvip, sizeof(neo->tcpsrv->srvip));

			if ( (eth_str = cfg->Attribute("eth")) )
			{ 	/* 确定服务于哪个网口，网口设置有较高优先权 */
				char ethfile[512];
				char *myip = (char*)0;
				TEXTUS_SNPRINTF(ethfile,sizeof(ethfile), "/etc/sysconfig/network-scripts/ifcfg-%s",eth_str);
				myip = BTool::getaddr(ethfile,"IPADDR");
				if (myip)
				TEXTUS_STRNCPY(neo->tcpsrv->srvip, myip, sizeof(tcpsrv->srvip)-1);
			}
			neo->isListener = true;
			neo->parent_begin();
			cfg->SetAttribute("port", neo->tcpsrv->getSrvPort());	
			ip_str = neo->tcpsrv->getSrvIP();
			if ( ip_str) {
				cfg->SetAttribute("ip", ip_str);	
			}
		}
		break;

	case Notitia::FD_PROWR:
		WBUG("facio FD_PROWR");	
		if (!isListener)	 /* 子实例, 应当是写 */
		{
			if( (ret = tcpsrv->transmitto()) )
				child_transmit_err(ret);
		}
		break;
		
	case Notitia::FD_PROEX:
		WBUG("facio FD_PROEX");	
		if (!isListener)	 /* 子实例, 应当是写 */
			end(true);	//at once
		break;
		
	case Notitia::CMD_TIMER_TO_RELEASE:
		WBUG("facio CMD_TIMER_TO_RELEASE");
		gCFG->sch->sponte(&alarm_pius); /* 定时后关闭 */
		break;

	case Notitia::TIMER:
		WBUG("facio TIMER");
		end(true);	//have shut_down
		break;

	case Notitia::TIMER_HANDLE:
		WBUG("facio TIMER_HANDLE");
		clr_timer_pius.indic = pius->indic;
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");		
		arr[0] = this;
		arr[1] = &(gCFG->down_delay);
		//arr[2] = &(gCFG->down_delay);
		arr[2] = 0;
		tmp_p.ordo = Notitia::CMD_GET_SCHED;
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
		
#if defined (_WIN32)
		if ( !tcpsrv->sock_start() ) 
		{
			SLOG(EMERG);
		}
#endif
		gCFG->sch->sponte(&tmp_p);	//向tpoll, 取得TPOLL
		if ( tmp_p.indic == gCFG->sch)
			gCFG->use_epoll = true;
		else
			gCFG->use_epoll = false;

		if ( isPioneer && gCFG->on_start)
			parent_begin();		/* 开始服务 */
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY " TLONG_FMTu, pius->ordo);			
		arr[0] = this;
		arr[1] = &(gCFG->down_delay);
		//arr[2] = &(gCFG->down_delay);
		arr[2] = 0;
		deliver(Notitia::SET_TBUF);
		break;

	default:
		return false;
	}
	return true;
}

bool Tcpsrvuna::sponte( Amor::Pius *pius)
{
	int ret;
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF :	//处理一帧数据而已
		WBUG("sponte PRO_TBUF");	
		assert(!isListener);	//侦听实例不干这事儿.
#if defined (_WIN32)	
		if( (ret = tcpsrv->transmitto_ex()) )
#else
		if( (ret = tcpsrv->transmitto()) )
#endif
		{
			if ( gCFG->use_epoll)
			{
				child_transmit_ep_err(ret);
			} else {
				child_transmit_err(ret);
			}
		}
		break;
		
	case Notitia::CMD_GET_FD:	//给出套接字描述符
		WBUG("sponte CMD_GET_FD");		
		if ( isListener)
			pius->indic = &(tcpsrv->listenfd);
		else
			pius->indic = &(tcpsrv->connfd);
		break;

	case Notitia::DMD_END_SESSION:	//强制关闭，等同主动关闭，要通知别人
		WLOG(INFO,"DMD_END_SESSION, close %d", tcpsrv->connfd);
		end(false);	//have down process
		break;

	case Notitia::CMD_RELEASE_SESSION:	/* 释放连接, 如果还有进程引用此套接字, 则连接不关闭 */
		if ( !isListener)
		{
			WLOG(INFO,"CMD_RELEASE_SESSION, close %d", tcpsrv->connfd);
			end_release();
		}
		break;

	case Notitia::DMD_END_SERVICE:	//强制关闭
		if ( isListener)
		{
			WLOG(NOTICE, "DMD_END_SERVICE, close %d", tcpsrv->listenfd);
			end_service();
		}
		break;
	
	case Notitia::DMD_BEGIN_SERVICE: //强制开启
		if ( isListener)
		{
			WLOG(NOTICE, "DMD_BEGIN_SERVICE");
			parent_begin();
		}
		break;
	
	case Notitia::CMD_CHANNEL_PAUSE :
		WBUG("sponte CMD_CHANNEL_PAUSE");
		if (gCFG->use_epoll ) {
			gCFG->sch->sponte(&epl_clr_ps);	//向tpoll,  注销
		} else {
			local_pius.ordo = Notitia::FD_CLRRD;
			gCFG->sch->sponte(&local_pius);	//向Sched, 以清rdSet.
		}
		break;

	case Notitia::CMD_CHANNEL_RESUME :
		WBUG("sponte CMD_CHANNEL_RESUME");
		if (gCFG->use_epoll ) {
			gCFG->sch->sponte(&epl_set_ps);	//向tpoll,  注册
		} else {
			local_pius.ordo = Notitia::FD_SETRD;
			gCFG->sch->sponte(&local_pius);	//向Sched, 以设置rdSet.
		}
		break;

	case Notitia::CMD_TIMER_TO_RELEASE:
		WBUG("sponte CMD_TIMER_TO_RELEASE");
		gCFG->sch->sponte(&alarm_pius); /* 定时后关闭 */
		break;

	default:
		return false;
	}
	return true;
}

Tcpsrvuna::Tcpsrvuna()
{
	pollor.pupa = this;
#if defined(_WIN32)
	pollor.type = DPoll::IOCPSock;
	pollor.hnd.sock =  INVALID_SOCKET;
#endif
	epl_set_ps.ordo = Notitia::SET_EPOLL;
	epl_set_ps.indic = &pollor;
	epl_clr_ps.ordo = Notitia::CLR_EPOLL;
	epl_clr_ps.indic = &pollor;
	pro_tbuf_ps.ordo = Notitia::PRO_TBUF;
	pro_tbuf_ps.indic = &tb_arr[0];

	my_tor.pupa = this;
	local_pius.ordo = Notitia::TEXTUS_RESERVED;	/* 未定, 可有Notitia::FD_CLRWR等多种可能 */
	local_pius.indic = &my_tor;

	tcpsrv = new Tcpsrv();

	isPioneer = false;	/* 默认自己不是父节点 */
	isListener = false;
	tcpsrv->errMsg = &errMsg[0];	/* 设置相同的错误信息缓冲 */
	tcpsrv->errstr_len = 1024;
	memset(tcpsrv->srvip, 0, sizeof(tcpsrv->srvip));
	last_child = (Tcpsrvuna*)0;	

	gCFG = 0;
	has_config = false;
	alarm_pius.ordo = Notitia::DMD_SET_ALARM;
	alarm_pius.indic = &arr[0];
	clr_timer_pius.ordo = Notitia::DMD_CLR_TIMER;
	clr_timer_pius.indic = 0;
#if  defined(__linux__)
	pollor.ev.data.ptr = &pollor;
#endif	//for linux
}

Tcpsrvuna::~Tcpsrvuna()
{
	if (has_config ) 
	{
		if(gCFG) delete gCFG;
	}	
	delete tcpsrv;
}

void Tcpsrvuna::parent_begin()
{	/* 服务开启 */
	if ( tcpsrv->listenfd !=INVALID_SOCKET )
		return ;
	if (!tcpsrv->servio(false))
	{
		SLOG(EMERG)
		return ;
	}
	if ( !gCFG->use_epoll ) {
		my_tor.scanfd = tcpsrv->listenfd;
		local_pius.ordo = Notitia::FD_SETRD;
		local_pius.indic = &my_tor;
		gCFG->sch->sponte(&local_pius);	//向Sched, 以设置rdSet.
	} else {
		pollor.pro_ps.ordo = Notitia::ACCEPT_EPOLL;
#if defined (_WIN32)	
		pollor.hnd.sock = tcpsrv->listenfd;
#endif

#if  defined(__linux__)
		pollor.fd = tcpsrv->listenfd;
		pollor.ev.events = EPOLLIN | EPOLLET |EPOLLONESHOT | EPOLLRDHUP ;
		pollor.op = EPOLL_CTL_ADD;
#endif	//for linux

#if  defined(__sun)
		pollor.fd = tcpsrv->listenfd;
		//pollor.events = POLLIN|POLLOUT;
		pollor.events = POLLIN;
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		EV_SET(&(pollor.events[0]), tcpsrv->listenfd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, &pollor);
		EV_SET(&(pollor.events[1]), tcpsrv->listenfd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, &pollor);
#endif	//for bsd
		gCFG->sch->sponte(&epl_set_ps);	//向tpoll

#if  defined(__linux__)
		pollor.op = EPOLL_CTL_MOD; //以后操作就是修改了。
#endif	//for linux

#if defined (_WIN32)	
		do_accept_ex();
#endif
	}
	deliver(Notitia::START_SERVICE); //向接力者发出通知, 本对象开始服务
	return ;
}

#if defined (_WIN32 )	
void Tcpsrvuna::do_accept_ex()
{
	int ret;
	while ( (ret = tcpsrv->accept_ex()) != 0 ) 	//异步操作， 先接收，以投递到IOCP
	{
		switch ( ret) {
		case -1:
			SLOG(ALERT)
			end(true);	//at once
			break;
		case 1:
			if( tcpsrv->post_accept_ex() )
			{
				new_conn_pro();
			} else {
				SLOG(INFO)
				tcpsrv->release();
			}
			break;
		}
	}
}
#endif

void Tcpsrvuna::child_begin()
{	
	tcpsrv->rcv_buf->reset();	//TCP接收(发送)缓冲区清空
	tcpsrv->snd_buf->reset();
	if (gCFG->use_epoll)
	{
#if defined (_WIN32 )	
		tcpsrv->m_rcv_buf.reset();	//for OVERLAP
		tcpsrv->m_snd_buf.reset();
		pollor.hnd.sock = tcpsrv->connfd;
		pollor.pro_ps.ordo = Notitia::PRO_EPOLL;
		shuting = false;
		shutted = false;
		zeroed = false;
#else
		pollor.pro_ps.ordo = Notitia::RD_EPOLL;
#endif

#if  defined(__linux__)
		pollor.fd = tcpsrv->connfd;
		pollor.ev.events = EPOLLIN | EPOLLET |EPOLLONESHOT;
		pollor.op = EPOLL_CTL_ADD;
#endif	//for linux

#if  defined(__sun)
		pollor.fd = tcpsrv->connfd;
		pollor.events = POLLIN;
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		EV_SET(&(pollor.events[0]), tcpsrv->connfd, EVFILT_READ, EV_ADD|EV_ONESHOT, 0, 0, &pollor);
		EV_SET(&(pollor.events[1]), tcpsrv->connfd, EVFILT_WRITE, EV_ADD|EV_DISABLE, 0, 0, &pollor);
#endif	//for bsd

		gCFG->sch->sponte(&epl_set_ps);	//向tpoll, 以设置iocp等
		/* 以下是后续操作 */

#if  defined(__linux__)
		pollor.op = EPOLL_CTL_MOD; //以后操作就是修改了。
#endif	//for linux

#if defined (_WIN32 )	
		/* 主动去接收, 如果一开始有数据, 则先接收; 另外实现IOCP投递 */
		if ( !tcpsrv->recito_ex())
		{
			SLOG(ERR)
			end(true);	//失败即关闭, at once
			return;
		}
#endif	//for WIN32
	} else {
		my_tor.scanfd = tcpsrv->connfd;
		local_pius.ordo = Notitia::FD_SETRD;
		gCFG->sch->sponte(&local_pius);	//向Sched, 以设置rdSet.
		local_pius.ordo = Notitia::FD_SETEX;
		gCFG->sch->sponte(&local_pius);	//向Sched, 以设置rdSet.
	}
	deliver(Notitia::START_SESSION); //向接力者发出通知, 本对象开始会话
	return;
}

Tcpsrvuna* Tcpsrvuna::parent_accept()
{
	if ( !tcpsrv->accipio(false)) 
	{	/* 接收新连接失败, 回去吧 */
		SLOG(ALERT)
		return 0;	
	}

	if ( tcpsrv->connfd ==INVALID_SOCKET ) 
	{	
		SLOG(INFO)
		return 0;	/* 连接还未建立好, 回去再等 */
	}
	return new_conn_pro();
}

Tcpsrvuna* Tcpsrvuna::new_conn_pro()
{
	Amor::Pius tmp_p;
	WLOG(INFO,"create socket %d", tcpsrv->connfd);
	tmp_p.ordo = Notitia::CMD_ALLOC_IDLE;
	tmp_p.indic = this;
	aptus->sponte(&tmp_p);
	/* 请求空闲的子实例, 将刚建立了连接的信息传过去, 然后由它工作 */
	last_child = 0;
	if ( !(tmp_p.indic) || this == ( Amor* ) (tmp_p.indic) )
	{	/* 实例没有增加, 已经到达最大连接数，故关闭刚才的连接 */
		WLOG(NOTICE, "limited connections for neo_conn, to max");
		tcpsrv->end();	/* tcpsrv刚建立新连接, 其connfd保存最近值, 第一次调用不关闭listenfd */
	} else {
		last_child = (Tcpsrvuna*)(tmp_p.indic);
		tcpsrv->herit(last_child->tcpsrv);
		last_child->child_begin();
		deliver(Notitia::NEW_SESSION); /* 父实例向接力者发出通知, 新建了连接 */
		if ( gCFG->lonely )
			end_service();
	}
	return last_child;
}

void Tcpsrvuna::child_transmit_ep_err(int mret)
{
	switch ( mret )
	{
#if 0
	case 0: //没有阻塞, 不变
		break;
#endif	
	case 2: //原有阻塞, 没有阻塞了, 清一下
#if  defined(__linux__)
		pollor.ev.events &= ~EPOLLOUT;	//等下一次设置POLLIN时再清
#endif	//for linux

#if  defined(__sun)
		pollor.events &= ~POLLOUT;	//等下一次设置POLLIN时再清
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		pollor.events[1].flags = EV_ADD | EV_DISABLE;	//等下一次设置时再清
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

#if !defined(_WIN32)	
		gCFG->sch->sponte(&epl_set_ps);	//向tpoll, 以设置kqueue等
#endif
		break;
	
	case -1://有严重错误, 关闭
		SLOG(WARNING)
		end(true); //at once
		break;

	case 4:	//IOCP wait
		WLOG(INFO, "IOCP last sending is still pending");
		break;
	default:
		break;
	}
}

void Tcpsrvuna::child_transmit_err(int mret)
{
	switch ( mret )
	{
#if 0
	case 0: //没有阻塞, 保持不变
		break;
#endif	
	case 2: //原有阻塞, 没有阻塞了, 清一下
		local_pius.ordo =Notitia::FD_CLRWR;
		//向Sched, 以设置wrSet.

		gCFG->sch->sponte(&local_pius);	
		break;
		
	case 1:	//新写阻塞, 需要设一下了
		SLOG(INFO)
		//向Sched, 以设置wrSet.
		local_pius.ordo =Notitia::FD_SETWR;
		gCFG->sch->sponte(&local_pius);
		break;
		
	case 3:	//还是写阻塞, 不变
		break;
		
	case -1://有严重错误, 关闭
		SLOG(WARNING)
		end(true);	//at ocne
		break;
	default:
		break;
	}
}

void Tcpsrvuna::end_service()
{	/* 服务关闭 */
	Amor::Pius tmp_p;

	if ( tcpsrv->listenfd == INVALID_SOCKET ) 
		return;
	if (!gCFG->use_epoll)
	{
		my_tor.scanfd = tcpsrv->listenfd;
		local_pius.ordo = Notitia::FD_CLRRD;
		gCFG->sch->sponte(&local_pius);	/* 向Sched, 以清除rdSet. */
	}
	tcpsrv->endListen();	/* will close listenfd */
	
		/* BSD: Calling close() on a file descriptor wil remove any kevents that reference the descriptor 
		  Events which are attached to file descriptors are automatically deleted on the last close of the descriptor.
		*/
		/* linux: shuting a file descriptor cause it to be removed from all epoll sets */
		/*sun: The association is also  removed  if  the  port  gets  closed */

	deliver(Notitia::END_SERVICE); //向下一级传递本类的会话关闭信号
	if ( !isPioneer )
	{	/* 根实例不被收回, 且仍是侦听实例 */
		isListener = false;
		tmp_p.ordo = Notitia::CMD_FREE_IDLE;
		tmp_p.indic = this;
		aptus->sponte(&tmp_p);
	}
}

void Tcpsrvuna::end(bool at_once)
{
	Amor::Pius tmp_p;
	if ( isListener )
		return;

	if ( tcpsrv->connfd == INVALID_SOCKET ) 	/* 已经关闭或未开始 */
		return;
	if (gCFG->use_epoll) 
	{
#if defined(_WIN32)	
		if ( !at_once ) 
		{
			if ( !shuting ) {
				shuting = true;
				shutted = false;
				if ( !tcpsrv->finis_ex())
				{
					SLOG(ERR)
					tcpsrv->release();	//Tcpsrv放弃
					goto POST;
				}
			} else if ( shutted)  {		//shuting, shutted
				if ( zeroed ) {
					tcpsrv->release();	//Tcpsrv放弃
					goto POST;
				} else {	//shuting, shutted, not zeroed, just waiting
					return ;
				}
			} else if ( zeroed ) {	//shuting, not shutted, zeroed, just waiting
				return ;
			}
			return; //shuting, not shutted, not zeroed, just waiting
		}
#endif
	} else {
		local_pius.ordo = Notitia::FD_CLRRD;
		gCFG->sch->sponte(&local_pius);	//向Sched, 以清rdSet.
	
		local_pius.ordo = Notitia::FD_CLRWR;
		gCFG->sch->sponte(&local_pius);	//向Sched, 以清wrSet.
	
		local_pius.ordo = Notitia::FD_CLREX;
		gCFG->sch->sponte(&local_pius);	//向Sched, 以清exSet.
	}

	// else gCFG->sch->sponte(&epl_clr_ps);	//向tpoll,  注销
	/* just close for all kinds of system ?  */

	if ( at_once)
		tcpsrv->release();	//Tcpsrv放弃
	else
		tcpsrv->end();		//Tcpsrv也关闭
#if defined(_WIN32)	
POST:
#endif
	gCFG->sch->sponte(&clr_timer_pius);
	deliver(Notitia::END_SESSION); //向下一级传递本类的会话关闭信号
	tmp_p.ordo = Notitia::CMD_FREE_IDLE;
	tmp_p.indic = this;
	aptus->sponte(&tmp_p);
}

void Tcpsrvuna::end_release()
{
	Amor::Pius tmp_p;
	if ( isListener )
		return;

	if ( tcpsrv->connfd == INVALID_SOCKET ) 	/* 已经关闭或未开始 */
		return;

	if (!gCFG->use_epoll)
	{
		local_pius.ordo = Notitia::FD_CLRRD;
		gCFG->sch->sponte(&local_pius);	//向Sched, 以清rdSet.
	
		local_pius.ordo = Notitia::FD_CLRWR;
		gCFG->sch->sponte(&local_pius);	//向Sched, 以清wrSet.
	
		local_pius.ordo = Notitia::FD_CLREX;
		gCFG->sch->sponte(&local_pius);	//向Sched, 以清exSet.
	}
	tcpsrv->release();	//Tcpsrv放弃
	tmp_p.ordo = Notitia::CMD_FREE_IDLE;
	tmp_p.indic = this;
	aptus->sponte(&tmp_p);
}

Amor* Tcpsrvuna::clone()
{
	Tcpsrvuna *child = new Tcpsrvuna();
	child->gCFG = gCFG;
	return (Amor*)child;
}

/* 向接力者提交 */
void Tcpsrvuna::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;

	switch (aordo )
	{
	case Notitia::SET_TBUF:
		WBUG("deliver SET_TBUF");
		tb_arr[0] = tcpsrv->rcv_buf;
		tb_arr[1] = tcpsrv->snd_buf;
		tb_arr[2] = 0;
		tmp_pius.indic = &tb_arr[0];
		break;
	case Notitia::END_SESSION:
		WBUG("deliver END_SESSION");
		break;
	default:
		WBUG("deliver Notitia::%d", aordo);
		break;
	}
	aptus->facio(&tmp_pius);
}
#include "hook.c"
