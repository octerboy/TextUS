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

#include "Amor.h"
#include "Notitia.h"
#include "textus_string.h"
#include <time.h>
#include "Tcpcli.h"
#include "Notitia.h"
#include "Describo.h"
#include <stdarg.h>

#ifndef TINLINE
#define TINLINE inline
#endif 

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
	Amor::Pius info_pius;
	Describo::Criptor mytor; //保存套接字, 各实例不同
	time_t last_failed_time;	//最近一次的失败时间,秒

	Tcpcli *tcpcli;
	char errMsg[2048];

	bool has_config;
	struct G_CFG {
		bool block_mode;	//是否为阻塞, 默认为非阻塞
		bool on_start;
		int try_interval;	//连接失败后，下一次再发起连接的时间间隔(秒)
		inline G_CFG() {
			block_mode = false;
			on_start =  true;
			try_interval = 0;
		};
	};
	struct G_CFG *gCFG;

	TINLINE void transmit();
	TINLINE void establish_done();
	TINLINE void establish();
	TINLINE void deliver(Notitia::HERE_ORDO aordo);
	TINLINE void end(bool outer=false);
	TINLINE void release();
	TINLINE void errpro();

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

	if( (try_str = cfg->Attribute("try")) && atoi(try_str) > 0 )
		gCFG->try_interval = atoi(try_str);

	if ( (comm_str = cfg->Attribute("block") ) && strcasecmp(comm_str, "yes") ==0 )
		gCFG->block_mode = true;	/* 并非一开始就启动 */
}

bool Tcpcliuna::facio( Amor::Pius *pius)
{
	const char *ip_str;
	TiXmlElement *cfg;
	TBuffer **tb;
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
		} else {
			transmit();
		}
		break;

	case Notitia::FD_PRORD:
		WBUG("facio FD_PRORD");
		/* 读数据, 这是最常的, 只有不失败并有数据才向左节点传递 */
		if ( tcpcli->isConnecting) //试图完成连接
			establish_done();
		else {
			int ret = tcpcli->recito();
			if ( ret && tcpcli->rcv_buf && tcpcli->rcv_buf->point > tcpcli->rcv_buf->base)
				deliver(Notitia::PRO_TBUF);
			else {
				errpro();
				if ( !ret ) end(true);	//失败即关闭, 不自动重连
			}
		}
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
		end(true);
		break;

	case Notitia::CMD_RELEASE_SESSION:
		WBUG("facio CMD_RELEASE_SESSION");
		release();
		break;

	case Notitia::TIMER:
		WBUG("facio TIMER, last %ld", last_failed_time);
		if ( gCFG->try_interval > 0 && tcpcli->connfd == -1 &&  last_failed_time &&
			*((time_t *)(pius->indic)) - last_failed_time >= gCFG->try_interval )
		{	//最近发生一次连接, 而且连接失败, 间隔时间到达设定值
			establish();		//开始建立连接
		}
		break;

	case Notitia::TIMER_HANDLE:
		WBUG("facio TIMER_HANDLE");
		clr_timer_pius.indic = pius->indic;
		break;

	case Notitia::IGNITE_ALL_READY:
	case Notitia::CLONE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY/CLONE_ALL_READY");
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
		} else 
			WLOG(NOTICE,"facio PRO_TBUF null.");
		break;

	case Notitia::DMD_START_SESSION:
		WBUG("facio DMD_START_SESSION");
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
	mytor.pupa = this;
	local_pius.ordo = Notitia::TEXTUS_RESERVED;
	local_pius.indic = &mytor;

	last_failed_time = 0;
	tcpcli = new Tcpcli();
	tcpcli->errMsg = &errMsg[0];
	tcpcli->errstr_len = 2048;
	memset(tcpcli->server_ip, 0 , sizeof(tcpcli->server_ip));

	gCFG = 0;
	has_config = false;
	clr_timer_pius.ordo = Notitia::DMD_CLR_TIMER;
	clr_timer_pius.indic = this;

	alarm_pius.ordo = Notitia::DMD_SET_TIMER;
	alarm_pius.indic = this;
}

Tcpcliuna::~Tcpcliuna()
{	
	aptus->sponte(&clr_timer_pius); /* 清除定时 */
	delete tcpcli;
	if (has_config )
		delete gCFG;
}

TINLINE void Tcpcliuna::establish()
{
	WLOG(INFO, "estabish to %s:%d  .....", tcpcli->server_ip, tcpcli->server_port);
	if (!tcpcli->annecto(gCFG->block_mode))
	{
		errpro();
		return ;
	}

	if ( tcpcli->isConnecting) 
	{	//正连接中,则向schedule登记
		mytor.scanfd = tcpcli->connfd;
		deliver(Notitia::FD_SETWR);
		deliver(Notitia::FD_SETRD);

#if defined(_WIN32)
		deliver(Notitia::FD_SETEX);
#endif
	} else /* 连接完成 */
		establish_done();
}

TINLINE void Tcpcliuna::establish_done()
{
	if (tcpcli->isConnecting ) if ( !tcpcli->annecto_done())
	{	//在建立连接的过程中出错
		errpro();
		end();	/* 自动重连 */
		return ;
	}

	mytor.scanfd = tcpcli->connfd;
	deliver(Notitia::FD_SETRD);
	deliver(Notitia::FD_CLRWR);
	deliver(Notitia::FD_CLREX);

	/* TCP接收(发送)缓冲区清空 */
	//if ( tcpcli->rcv_buf) tcpcli->rcv_buf->reset();	
	//if ( tcpcli->snd_buf) tcpcli->snd_buf->reset();
	aptus->sponte(&clr_timer_pius); /* 清除定时 */
	WLOG(INFO, "estabish %s:%d ok!", tcpcli->server_ip, tcpcli->server_port);
	deliver(Notitia::START_SESSION); //向接力者发出通知, 本对象开始
}

TINLINE void Tcpcliuna::transmit()
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
		aptus->sponte(&local_pius);	
		break;
	case 1:	//新写阻塞, 需要设一下了
		errpro();
		local_pius.ordo =Notitia::FD_SETWR;
		aptus->sponte(&local_pius);	
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

TINLINE void Tcpcliuna::end(bool outer)
{
	WBUG("end().....");
	if ( tcpcli->connfd == -1 ) return;	/* 不重复关闭 */
	deliver(Notitia::FD_CLRWR);
	deliver(Notitia::FD_CLREX);
	deliver(Notitia::FD_CLRRD);
	
	tcpcli->end();		//Tcpcli也关闭
	if (outer )
	{
		last_failed_time = 0;	/* 最近关闭时间清零, 不再重连服务端 */
		aptus->sponte(&clr_timer_pius); /* 清除定时 */
	} else {
		time(&last_failed_time);	/* 记录最近关闭时间, 这将使得重连服务端 */
		aptus->sponte(&alarm_pius);
	}

	deliver(Notitia::END_SESSION);/* 向左、右传递本类的会话关闭信号 */
}

TINLINE void Tcpcliuna::release()
{
	WBUG("release().....");
	if ( tcpcli->connfd == -1 ) return;	/* 不重复 */
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
		aptus->sponte(&local_pius);	//向Sched
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

#include "hook.c"
