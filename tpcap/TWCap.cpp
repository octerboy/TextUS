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

#include "Amor.h"
#include "Notitia.h"
#include "Describo.h"
#include "TBuffer.h"
#include "BTool.h"
#include "casecmp.h"
#include "textus_string.h"


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

	struct G_CFG {
		const char *eth;

		inline G_CFG(TiXmlElement *cfg) {
			eth = (const char*) 0;
			
			eth = cfg->Attribute("device");
		};

		inline ~G_CFG() {
		};
	};
	struct G_CFG *gCFG;     /* 全局共享参数 */
	bool has_config;

	TINLINE bool handle();
	void init();	
	void deliver(Notitia::HERE_ORDO aordo);

	#include "wlog.h"
};

#include <assert.h>

void TWCap::ignite(TiXmlElement *prop)
{
	if (!prop) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(prop);
		has_config = true;
	}
}

TWCap::TWCap():rcv_buf(8192)
{
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
		init();
		deliver(Notitia::SET_TBUF);
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
		deliver(Notitia::SET_TBUF);
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
	aptus->sponte(&local_pius);	//向Sched, 以设置rdSet.
	return ;

ERROR_PRO:
	if ( sock_fd != INVALID_SOCKET)
	{
		closesocket(sock_fd);
	}
	return ;
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
