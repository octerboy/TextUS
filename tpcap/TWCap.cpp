/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title:Internet Protocol Head pro
 Build: created by octerboy, 2007/08/02, Panyu
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include <winsock2.h>
#include <mstcpip.h>
#include <stdio.h>

#define TINLINE inline

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include "Amor.h"
#include "Notitia.h"
#include "Describo.h"
#include "DPoll.h"
#include "TBuffer.h"
#include "BTool.h"
#include "casecmp.h"
#include "textus_string.h"

#define RCV_FRAME_SIZE 8192
class TWCap: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	TWCap();
	~TWCap();

private:
	Amor::Pius pro_tbuf;
	Amor::Pius local_pius;
	Describo::Criptor my_tor; /* 保存套接字, 各子实例不同 */

	TBuffer rcv_buf, snd_buf;	/* 向右(左)传递 */
	SOCKET sock_fd ;
	int bufsize;
	OVERLAPPED rcv_ovp;
	WSABUF wsa_rcv;
	DWORD flag;
	DPoll::Pollor pollor; /* 保存事件句柄, 各子实例不同 */
	Amor::Pius epl_set_ps, epl_clr_ps;

	struct G_CFG {
		const char *eth;
		bool use_epoll;
		Amor *sch;
		struct DPoll::PollorBase lor; /* 探询 */

		inline G_CFG(TiXmlElement *cfg) {
			eth = (const char*) 0;
			eth = cfg->Attribute("device");
			lor.type = DPoll::NotUsed;
			sch = 0;
		};

		inline ~G_CFG() {
		};
	};
	struct G_CFG *gCFG;     /* 全局共享参数 */
	bool has_config;

	TINLINE bool handle();
	void init();	
	void init_ex();	
	void recito_ex();	
	void end();	
	void deliver(Notitia::HERE_ORDO aordo);
	
	#include "wlog.h"
};

#include <assert.h>

void TWCap::ignite(TiXmlElement *prop)
{
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
	if (!prop) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(prop);
		has_config = true;
	}
}

TWCap::TWCap():rcv_buf(8192)
{
	pollor.pupa = this;
	pollor.type = DPoll::Sock;
	epl_set_ps.ordo = Notitia::SET_EPOLL;
	epl_set_ps.indic = &pollor;
	epl_clr_ps.ordo = Notitia::CLR_EPOLL;
	epl_clr_ps.indic = &pollor;

	pro_tbuf.ordo = Notitia::PRO_TBUF;
	pro_tbuf.indic = 0;

	my_tor.pupa = this;
	local_pius.ordo = -1;	/* 未定, 可有Notitia::FD_CLRWR等多种可能 */
	local_pius.indic = &my_tor;

	gCFG = 0;
	has_config = false;
	bufsize = 8192;
}

TWCap::~TWCap() 
{
	if ( has_config  )
	{	
		if(gCFG) delete gCFG;
	}
}

Amor* TWCap::clone()
{
	TWCap *child = new TWCap();
	child->gCFG = gCFG;
	return (Amor*) child;
}

bool TWCap::facio( Amor::Pius *pius)
{
	OVERLAPPED_ENTRY *aget;
	Amor::Pius tmp_p;
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::FD_PRORD:
		WBUG("facio FD_PRORD");
		if ( handle() )
			aptus->facio(&pro_tbuf);
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		deliver(Notitia::SET_TBUF);
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
		
		gCFG->sch->sponte(&tmp_p);	//向tpoll, 取得TPOLL
		if ( tmp_p.indic == gCFG->sch) {
			gCFG->use_epoll = true;
			wsa_rcv.len = RCV_FRAME_SIZE;
			init_ex();
		} else {
			gCFG->use_epoll = false;
			init();
		}

		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
		deliver(Notitia::SET_TBUF);
		break;

	case Notitia::ERR_EPOLL:
		WBUG("facio ERR_EPOLL");
		WLOG(WARNING, (char*)pius->indic);	
		end();	//直接关闭就可.
		break;

	case Notitia::PRO_EPOLL:
		WBUG("facio PRO_EPOLL");
		aget = (OVERLAPPED_ENTRY *)pius->indic;
		if ( aget->lpOverlapped == &(rcv_ovp) )
		{	//已读数据,  不失败并有数据才向接力者传递
			if ( aget->dwNumberOfBytesTransferred ==0 ) 
			{
				WLOG(INFO, "IOCP recv 0 disconnected");
				end();
			} else {
				WBUG("child PRO_EPOLL recv %d bytes", aget->dwNumberOfBytesTransferred);
				rcv_buf.commit(aget->dwNumberOfBytesTransferred);
				aptus->facio(&pro_tbuf);
				recito_ex();
			}
		} else {
			WLOG(ALERT, "not my overlap");
		}
		break;

	default:
		return false;
	}
	return true;
}

bool TWCap::sponte( Amor::Pius *pius)
{
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF:
		WBUG("sponte PRO_TBUF");
		break;

	default:
		return false;
	}
	return true;
}

void TWCap::init()
{
	assert(gCFG->eth != (const char*) 0 );
	if ( !gCFG->eth)
		return ;

	DWORD dwBufferLen[10] ;
	DWORD dwBufferInLen= 1 ;
	DWORD dwBytesReturned = 0 ;
	int value;
	SOCKADDR_IN sa;
	struct in_addr me;

	sock_fd = socket( AF_INET , SOCK_RAW , IPPROTO_IP ) ;
	if ( sock_fd == INVALID_SOCKET ) 
	{
		WLOG_OSERR("socket");
		goto ERROR_PRO;
	}

	int rcvtimeo = 5000 ; // 5 sec insteadof 45 as default
        if( setsockopt( sock_fd , SOL_SOCKET , SO_RCVTIMEO , (const char *)&rcvtimeo , sizeof(rcvtimeo) ) == SOCKET_ERROR)
	{
		WLOG_OSERR("setsockopt");
		goto ERROR_PRO;
	}

	sa.sin_family = AF_INET;
 	sa.sin_port = htons(7000);
	me.s_addr = inet_addr(gCFG->eth);
	sa.sin_addr.s_addr = me.s_addr;

	if (bind(sock_fd,(struct sockaddr *)&sa, sizeof(sa)) == SOCKET_ERROR)
	{
		WLOG_OSERR("bind");
		goto ERROR_PRO;
	} 

	value = RCVALL_ON;
	if( SOCKET_ERROR == WSAIoctl(sock_fd, SIO_RCVALL , &value, sizeof(value), &dwBufferLen, sizeof(dwBufferLen), &dwBytesReturned, NULL, NULL ) )
	{
		WLOG_OSERR("WSAIoctl ")
		goto ERROR_PRO;
	}

	my_tor.scanfd = sock_fd;
	local_pius.ordo = Notitia::FD_SETRD;
	gCFG->sch->sponte(&local_pius);	//向Sched, 以设置rdSet.
	return ;

ERROR_PRO:
	if ( sock_fd != INVALID_SOCKET)
	{
		sock_fd = INVALID_SOCKET;
		closesocket(sock_fd);
	}
	return ;
}

void TWCap::init_ex()
{
	assert(gCFG->eth != (const char*) 0 );
	if ( !gCFG->eth)
		return ;

	DWORD dwBufferLen[10] ;
	DWORD dwBufferInLen= 1 ;
	DWORD dwBytesReturned = 0 ;
	int value;
	SOCKADDR_IN sa;
	struct in_addr me;

	if ((sock_fd = WSASocket(AF_INET, SOCK_RAW, IPPROTO_IP, NULL,0,WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET )
	{
		WLOG_OSERR("WSASocket ")
		return ;
	}
	int rcvtimeo = 5000 ; // 5 sec insteadof 45 as default
        if( setsockopt( sock_fd , SOL_SOCKET , SO_RCVTIMEO , (const char *)&rcvtimeo , sizeof(rcvtimeo) ) == SOCKET_ERROR)
	{
		WLOG_OSERR("setsockopt");
		end();
		return;
	}

	sa.sin_family = AF_INET;
 	sa.sin_port = htons(7000);
	me.s_addr = inet_addr(gCFG->eth);
	sa.sin_addr.s_addr = me.s_addr;

        if (bind(sock_fd,(struct sockaddr *)&sa, sizeof(sa)) == SOCKET_ERROR)
	{
		WLOG_OSERR("bind");
		end();
		return;
	} 

	value = RCVALL_ON;
	if( SOCKET_ERROR == WSAIoctl(sock_fd, SIO_RCVALL , &value, sizeof(value), &dwBufferLen, sizeof(dwBufferLen), &dwBytesReturned, NULL, NULL ) )
	{
		WLOG_OSERR("WSAIoctl ")
		end();
		return;
	}

	pollor.hnd.sock = sock_fd;
	pollor.pro_ps.ordo = Notitia::PRO_EPOLL;
	gCFG->sch->sponte(&epl_set_ps); //向tpoll
	recito_ex();
}

void TWCap::end()
{
	if ( sock_fd != INVALID_SOCKET)
	{
		closesocket(sock_fd);
		sock_fd = INVALID_SOCKET;
	}
}

TINLINE bool TWCap::handle()
{
	int rlen;
	rcv_buf.grant(bufsize);	 //保证有足够空间
ReadAgain:
	rlen = recv( sock_fd , (char*)rcv_buf.point, bufsize, 0 ) ;
	if( rlen > 0 ) 
	{
		rcv_buf.commit(rlen);   /* 指针向后移 */	
		return true;
	} else {
		int error = errno;
		if (error == WSAEINTR)
		{	 //有信号而已,再试
			goto ReadAgain;
		} else if ( error == WSAEWOULDBLOCK )
		{	//还在进行中, 回去再等.
			WLOG_OSERR("recving encounter EAGAIN");
			return false;
		} else	
		{	//的确有错误
			WLOG_OSERR("recv socket");
			return false;
		}
	}
}

void TWCap::recito_ex()
{	
	int rc;
	rcv_buf.grant(RCV_FRAME_SIZE);	//保证有足够空间
	wsa_rcv.buf = (char *)rcv_buf.point;
	flag = 0;
	memset(&rcv_ovp, 0, sizeof(OVERLAPPED));
	rc = WSARecv(sock_fd, &wsa_rcv, 1, NULL, &flag, &rcv_ovp, NULL);
	if ( rc != 0 )
	{
		if ( WSA_IO_PENDING != WSAGetLastError() ) {
			WLOG_OSERR("WSARecv")
			end();
		}
	}
}

/* 向接力者提交 */
void TWCap::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	TBuffer *tb[3];
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;

	switch (aordo )
	{
	case Notitia::SET_TBUF:
		WBUG("deliver SET_TBUF");
		tb[0] = &rcv_buf;
		tb[1] = &snd_buf;
		tb[2] = 0;
		tmp_pius.indic = &tb[0];
		break;
	default:
		WBUG("deliver Notitia::%lu", aordo);
		break;
	}
	aptus->facio(&tmp_pius);
	return ;
}

#include "hook.c"
