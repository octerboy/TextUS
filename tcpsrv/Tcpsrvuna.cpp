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

#ifndef TINLNE
#define TINLNE inline
#endif

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
	bool has_config;
	struct G_CFG {
		bool on_start ;
		bool lonely;
		int back_log;
		inline G_CFG() {
			on_start = true;
			back_log = 100;
		};
	};
	struct G_CFG *gCFG;

	Amor::Pius local_pius;
	char errMsg[1024];
	
	Describo::Criptor my_tor; /* 保存套接字, 各子实例不同 */
	DPoll::Pollor pollor; /* 保存事件句柄, 各子实例不同 */
	Amor::Pius epl_set_ps, epl_clr_ps, pro_tbuf_ps;

	Tcpsrv *tcpsrv;
	Tcpsrvuna *last_child;	/* 最近一次连接的子实例 */

	bool isPioneer, isListener;
	TINLNE void child_begin();
	TINLNE void parent_begin();
	TINLNE void end_service();	/* 关闭侦听套接字 */
	TINLNE void end(bool down=true);		/* 关闭连接 或 只释放套接字 */

	TINLNE void child_rcv_pro(long len, const char *msg);
	TINLNE void parent_accept();
	TINLNE void child_transmit();
	TINLNE void child_transmit_ep();
	TINLNE void deliver(Notitia::HERE_ORDO aordo);
	inline void new_conn_pro();
#if defined (_WIN32 )	
	inline void do_accept_ex();
#endif	//for WIN32
	inline void do_recv_ex();
	//DPoll::Pollor *ppo; 
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

	end();
}

bool Tcpsrvuna::facio( Amor::Pius *pius)
{
	TiXmlElement *cfg;
	Amor::Pius tmp_p;

#if defined (_WIN32 )	
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
			child_rcv_pro(tcpsrv->recito(), "child_pro FD_PROFD recv bytes"); /* 子实例, 应当是读 */		
		}
		break;

	case Notitia::ACCEPT_EPOLL:
		WBUG("facio ACCEPT_EPOLL");
#if defined (_WIN32 )	
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
		parent_accept(); /* 既然由侦听实例, 必是建立新连接 */
		/* action flags and filter for event remain unchanged */
		aptus->sponte(&epl_set_ps);	//向tpoll,  再一次注册
#endif
		break;

	case Notitia::ERR_EPOLL:
		WBUG("facio ERR_EPOLL");
#if defined (_WIN32 )	
		tcpsrv->get_error_string("GetIOCP");
		SLOG(WARNING)
#else
		WLOG(WARNING, (char*)pius->indic);	
#endif	//for WIN32

		end();	//对于WINDOWS, 直接关闭就可.
		break;

	case Notitia::PRO_EPOLL:
		WBUG("facio PRO_EPOLL");
#if defined (_WIN32 )	
		aget = (OVERLAPPED_ENTRY *)pius->indic;
		if ( aget->lpOverlapped == &(tcpsrv->rcv_ovp) )
		{	//已读数据,  不失败并有数据才向接力者传递
			if ( aget->dwNumberOfBytesTransferred ==0 ) 
			{
				WLOG(INFO, "IOCP recv 0 disconnected");
				end();
			} else {
				child_rcv_pro( aget->dwNumberOfBytesTransferred , "child_pro PRO_EPOLL recv bytes");
			}
		} else if ( aget->lpOverlapped == &(tcpsrv->snd_ovp) ) {
			//写数据完成
			if ( tcpsrv->snd_buf->point > tcpsrv->snd_buf->base )	//继续写
				child_transmit_ep();
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
		/* action flags and filter for event remain unchanged */
		aptus->sponte(&epl_set_ps);	//向tpoll,  再一次注册
		break;

	case Notitia::WR_EPOLL:
		WBUG("facio WR_EPOLL");
		child_transmit_ep();
		break;

	case Notitia::EOF_EPOLL:
		WBUG("facio EOF_EPOLL");
		WLOG(INFO, "peer disconnected.");
		end();
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
		/* 请求空闲的子实例, 将刚建立了连接的信息传过去, 然后由它工作 */
		if ( !(tmp_p.indic) || this == ( Amor* ) (tmp_p.indic) )
		{	/* 实例没有增加, 已经到达最大连接数，故关闭刚才的连接 */
			WLOG(NOTICE, "limited connections, to max");
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
				TEXTUS_STRNCPY(neo->tcpsrv->srvip, tcpsrv->srvip, sizeof(neo->tcpsrv->srvip)-1);

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
			if ( ip_str)
				cfg->SetAttribute("ip", ip_str);	
		}
		break;

	case Notitia::FD_PROWR:
		WBUG("facio FD_PROWR");	
		if (!isListener)	 /* 子实例, 应当是写 */
			child_transmit();
		break;
		
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");		
		if ( isPioneer && gCFG->on_start)
			parent_begin();		/* 开始服务 */
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY %lu" , pius->ordo);			
		deliver(Notitia::SET_TBUF);
		break;

	default:
		return false;
	}
	return true;
}

bool Tcpsrvuna::sponte( Amor::Pius *pius)
{
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF :	//处理一帧数据而已
		WBUG("sponte PRO_TBUF");	
		assert(!isListener);	//侦听实例不干这事儿.
		if ( tcpsrv->use_epoll)
		{
			child_transmit_ep();
		} else {
			child_transmit();
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
		end();
		break;

	case Notitia::CMD_RELEASE_SESSION:	/* 释放连接, 如果还有进程引用此套接字, 则连接不关闭 */
		if ( !isListener)
		{
			WLOG(INFO,"CMD_RELEASE_SESSION, close %d", tcpsrv->connfd);
			end(false);
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
		deliver(Notitia::FD_CLRRD);
		break;

	case Notitia::CMD_CHANNEL_RESUME :
		WBUG("sponte CMD_CHANNEL_RESUME");
		deliver(Notitia::FD_SETRD);
		break;

	default:
		return false;
	}
	return true;
}

Tcpsrvuna::Tcpsrvuna()
{
	pollor.pupa = this;
	pollor.type = DPoll::Sock;
	epl_set_ps.ordo = Notitia::SET_EPOLL;
	epl_set_ps.indic = &pollor;
	epl_clr_ps.ordo = Notitia::CLR_EPOLL;
	epl_clr_ps.indic = &pollor;
	pro_tbuf_ps.ordo = Notitia::PRO_TBUF;
	pro_tbuf_ps.indic = 0;

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

TINLNE void Tcpsrvuna::parent_begin()
{	/* 服务开启 */
	if ( tcpsrv->listenfd > 0 )
		return ;

	if (!tcpsrv->servio(false))
	{
		SLOG(EMERG)
		return ;
	}
	my_tor.scanfd = tcpsrv->listenfd;
	local_pius.ordo = Notitia::FD_SETRD;
	local_pius.indic = &my_tor;
	aptus->sponte(&local_pius);	//向Sched, 以设置rdSet.
	if ( my_tor.rd_index < 0 ) 
	{
		tcpsrv->use_epoll = false;
	} else {	//try for epoll
		pollor.pro_ps.ordo = Notitia::ACCEPT_EPOLL;
#if defined (_WIN32 )	
		pollor.hnd.sock = tcpsrv->listenfd;
#endif

#if  defined(__linux__)
		pollor.fd = tcpsrv->listenfd;
		pollor.ev.events = EPOLLIN | EPOLLET |EPOLLONESHOT | EPOLLRDHUP ;
		pollor.op = EPOLL_CTL_ADD;
#endif	//for linux

#if  defined(__sun)
		pollor.fd = tcpsrv->listenfd;
		pollor.events = POLLIN;
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		EV_SET(&(pollor.events[0]), tcpsrv->listenfd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, &pollor);
		EV_SET(&(pollor.events[1]), tcpsrv->listenfd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, &pollor);
#endif	//for bsd

		aptus->sponte(&epl_set_ps);	//向tpoll
		tcpsrv->use_epoll = true;

#if  defined(__linux__)
		pollor.op = EPOLL_CTL_MOD; //以后操作就是修改了。
#endif	//for linux

#if defined (_WIN32 )	
		do_accept_ex();
#endif
	}
	deliver(Notitia::START_SERVICE); //向接力者发出通知, 本对象开始服务
	return ;
}

TINLNE void Tcpsrvuna::child_rcv_pro(long len, const char *msg)
{
	if ( len > 0 ) 
	{
		WBUG("%s %ld", msg, len);
		aptus->facio(&pro_tbuf_ps);
	} else {
		if ( len == 0 || len == -1)	/* 记日志 */
		{
			SLOG(INFO)
		} else
		{
			SLOG(NOTICE)
		}
		if ( len < 0 )  end();	//失败即关闭
	}
}

#if defined (_WIN32 )	
inline void Tcpsrvuna::do_accept_ex()
{
	int ret;
	while ( (ret = tcpsrv->accept_ex()) != 0 ) 	//异步操作， 先接收，以投递到IOCP
	{
		switch ( ret) {
		case -1:
			SLOG(ALERT)
			end();
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

inline void Tcpsrvuna::do_recv_ex()
{
	int len;
	while ( (len = tcpsrv->recito()) != 0 ) 
	{	//==0即为Pending
		child_rcv_pro(len, "child_pro recv bytes");
#if !defined (_WIN32 )	
		if ( len < RCV_FRAME_SIZE ) 
			break;	//len 不足8192时, 或有错误即终止
#endif
	}
}

TINLNE void Tcpsrvuna::child_begin()
{	
	if (tcpsrv->use_epoll)
	{
		pollor.pro_ps.ordo = Notitia::RD_EPOLL;
#if defined (_WIN32 )	
		pollor.hnd.sock = tcpsrv->connfd;
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

		aptus->sponte(&epl_set_ps);	//向tpoll, 以设置iocp等
		/* 以下是后续操作 */

#if  defined(__linux__)
		pollor.op = EPOLL_CTL_MOD; //以后操作就是修改了。
#endif	//for linux

#if defined (_WIN32 )	
		/* 主动去接收, 如果一开始有数据, 则先接收; 另外实现IOCP投递 */
		do_recv_ex();
#endif	//for WIN32
	} else {
		my_tor.scanfd = tcpsrv->connfd;
		local_pius.ordo = Notitia::FD_SETRD;
		aptus->sponte(&local_pius);	//向Sched, 以设置rdSet.
	}

	tcpsrv->rcv_buf->reset();	//TCP接收(发送)缓冲区清空
	tcpsrv->snd_buf->reset();
	deliver(Notitia::START_SESSION); //向接力者发出通知, 本对象开始会话
	return;
}

TINLNE void Tcpsrvuna::parent_accept()
{
	if ( !tcpsrv->accipio(false)) 
	{	/* 接收新连接失败, 回去吧 */
		SLOG(ALERT)
		return;	
	}

	if ( tcpsrv->connfd < 0 ) 
	{	
		SLOG(INFO)
		return;	/* 连接还未建立好, 回去再等 */
	}
	new_conn_pro();
}

inline void Tcpsrvuna::new_conn_pro()
{
	Amor::Pius tmp_p;
	WLOG(INFO,"create socket %d", tcpsrv->connfd);
	tmp_p.ordo = Notitia::CMD_ALLOC_IDLE;
	tmp_p.indic = this;
	aptus->sponte(&tmp_p);
	/* 请求空闲的子实例, 将刚建立了连接的信息传过去, 然后由它工作 */
	if ( !(tmp_p.indic) || this == ( Amor* ) (tmp_p.indic) )
	{	/* 实例没有增加, 已经到达最大连接数，故关闭刚才的连接 */
		WLOG(NOTICE, "limited connections, to max");
		tcpsrv->end();	/* tcpsrv刚建立新连接, 其connfd保存最近值, 第一次调用不关闭listenfd */
	} else {
		last_child = (Tcpsrvuna*)(tmp_p.indic);
		tcpsrv->herit(last_child->tcpsrv);
		last_child->child_begin();
		deliver(Notitia::NEW_SESSION); /* 父实例向接力者发出通知, 新建了连接 */
		if ( gCFG->lonely )
			end_service();
	}
}

TINLNE void Tcpsrvuna::child_transmit_ep()
{
	switch ( tcpsrv->transmitto() )
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

		aptus->sponte(&epl_set_ps);	//向tpoll, 以设置kqueue等
		break;
	
	case -1://有严重错误, 关闭
		SLOG(WARNING)
		end();
		break;

	default:
		break;
	}
}

TINLNE void Tcpsrvuna::child_transmit()
{
	switch ( tcpsrv->transmitto() )
	{
	case 0: //没有阻塞, 保持不变
		break;
		
	case 2: //原有阻塞, 没有阻塞了, 清一下
		local_pius.ordo =Notitia::FD_CLRWR;
		//向Sched, 以设置wrSet.

		aptus->sponte(&local_pius);	
		break;
		
	case 1:	//新写阻塞, 需要设一下了
		SLOG(INFO)
		//向Sched, 以设置wrSet.
		local_pius.ordo =Notitia::FD_SETWR;
		aptus->sponte(&local_pius);
		break;
		
	case 3:	//还是写阻塞, 不变
		break;
		
	case -1://有严重错误, 关闭
		SLOG(WARNING)
		end();
		break;
	default:
		break;
	}
}

TINLNE void Tcpsrvuna::end_service()
{	/* 服务关闭 */
	Amor::Pius tmp_p;

	if ( tcpsrv->listenfd <= 0 ) 
		return;
	if (!tcpsrv->use_epoll)
	{
		my_tor.scanfd = tcpsrv->listenfd;
		local_pius.ordo = Notitia::FD_CLRRD;
		aptus->sponte(&local_pius);	/* 向Sched, 以清除rdSet. */
	}
	tcpsrv->endListen();	/* will close listenfd */
	
		/* BSD: Calling close() on a file descriptor wil remove any kevents that reference the descriptor 
		  Events which are attached to file descriptors are automatically deleted on the last close of the descriptor.
		*/
		/* linux: closing a file descriptor cause it to be removed from all epoll sets */
		/*sun: The association is also  removed  if  the  port  gets  closed */

	if ( !isPioneer )
	{	/* 根实例不被收回, 且仍是侦听实例 */
		isListener = false;
		tmp_p.ordo = Notitia::CMD_FREE_IDLE;
		tmp_p.indic = this;
		aptus->sponte(&tmp_p);
	}
	deliver(Notitia::END_SERVICE); //向下一级传递本类的会话关闭信号
}

TINLNE void Tcpsrvuna::end(bool down)
{	/* 服务关闭 */
	Amor::Pius tmp_p;
	if ( isListener )
		return;

	if ( tcpsrv->connfd == -1 ) 	/* 已经关闭或未开始 */
		return;

	if (!tcpsrv->use_epoll)
	{
		local_pius.ordo = Notitia::FD_CLRRD;
		aptus->sponte(&local_pius);	//向Sched, 以清rdSet.
	
		local_pius.ordo = Notitia::FD_CLRWR;
		aptus->sponte(&local_pius);	//向Sched, 以清wrSet.
	}
	/* just close for all kinds of system  */

	if ( down)
		tcpsrv->end();		//Tcpsrv也关闭
	else
		tcpsrv->release();	//Tcpsrv放弃

	tmp_p.ordo = Notitia::CMD_FREE_IDLE;
	tmp_p.indic = this;
	aptus->sponte(&tmp_p);

	if ( down)
		deliver(Notitia::END_SESSION); //向下一级传递本类的会话关闭信号
}

Amor* Tcpsrvuna::clone()
{
	Tcpsrvuna *child = new Tcpsrvuna();
	child->gCFG = gCFG;
	return (Amor*)child;
}

/* 向接力者提交 */
TINLNE void Tcpsrvuna::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	TBuffer *tb[3];
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;

	switch (aordo )
	{
	case Notitia::SET_TBUF:
		WBUG("deliver SET_TBUF");
		tb[0] = tcpsrv->rcv_buf;
		tb[1] = tcpsrv->snd_buf;
		tb[2] = 0;
		tmp_pius.indic = &tb[0];
		break;
	default:
		WBUG("deliver Notitia::%d", aordo);
		break;
	}
	aptus->facio(&tmp_pius);
}
#include "hook.c"

