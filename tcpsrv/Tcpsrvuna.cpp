/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
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
 $Header: /textus/tcpsrv/Tcpsrvuna.cpp 34    12-04-04 17:49 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: Tcpsrvuna.cpp $"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "BTool.h"
#include <assert.h>

#define SLOG(Z) { Amor::Pius log_pius; \
		log_pius.ordo = Notitia::LOG_##Z; \
		log_pius.indic = &errMsg[0]; \
		aptus->sponte(&log_pius); \
		}

#include "Tcpsrv.h"
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
		inline G_CFG() {
			on_start = true;
			lonely = false;
		};
	};
	struct G_CFG *gCFG;

	Amor::Pius local_pius;
	char errMsg[1024];
	
	Describo::Criptor my_tor; /* 保存套接字, 各子实例不同 */

	Tcpsrv *tcpsrv;
	Tcpsrvuna *last_child;	/* 最近一次连接的子实例 */

	bool isPioneer, isListener;
	TINLNE void child_begin();
	TINLNE void parent_begin();
	TINLNE void end_service();	/* 关闭侦听套接字 */
	TINLNE void end(bool down=true);		/* 关闭连接 或 只释放套接字 */

	TINLNE void child_pro(Amor::Pius *);
	TINLNE void parent_pro(Amor::Pius *);
	TINLNE void child_transmit();
	TINLNE void deliver(Notitia::HERE_ORDO aordo);
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
	assert(pius);
	
	switch(pius->ordo)
	{
	case Notitia::FD_PRORD:
		WBUG("facio FD_PRORD");
		if (isListener)		
			parent_pro(pius); /* 既然由侦听实例, 必是建立新连接 */
		else 
		 	child_pro(pius); /* 子实例, 应当是读 */		
		break;

	case Notitia::CMD_NEW_SERVICE:
		WBUG("facio CMD_NEW_SERVICE");
		if (isPioneer)			/* 须从父实例派生出侦听实例 */
			parent_pro(pius); /* new tcp service  */
		break;

	case Notitia::FD_PROWR:
		WBUG("facio FD_PROWR");	
		if (!isListener)	 /* 子实例, 应当是写 */
			child_pro(pius);
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
		child_transmit();
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
	aptus->sponte(&local_pius);	//向Sched, 以设置rdSet.
	deliver(Notitia::START_SERVICE); //向接力者发出通知, 本对象开始服务
	return ;
}

TINLNE void Tcpsrvuna::child_begin()
{	
	my_tor.scanfd = tcpsrv->connfd;
	local_pius.ordo = Notitia::FD_SETRD;
	aptus->sponte(&local_pius);	//向Sched, 以设置rdSet.
	tcpsrv->rcv_buf->reset();	//TCP接收(发送)缓冲区清空
	tcpsrv->snd_buf->reset();
	deliver(Notitia::START_SESSION); //向接力者发出通知, 本对象开始会话
	return;
}

TINLNE void Tcpsrvuna::parent_pro(Amor::Pius *pius)
{
	TiXmlElement *cfg;
	Amor::Pius tmp_p;
	switch (pius->ordo)
	{
	case Notitia::CMD_NEW_SERVICE:
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
		} else 
		{
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

	case Notitia::FD_PRORD:
		if ( !tcpsrv->accipio(false)) 
		{	/* 接收新连接失败, 回去吧 */
			SLOG(ALERT)
			return;	
		}

		if ( tcpsrv->connfd < 0 ) 
		{	SLOG(INFO)
			return;	/* 连接还未建立好, 回去再等 */
		}
		WLOG(INFO,"create socket %d", tcpsrv->connfd);
		tmp_p.ordo = Notitia::CMD_ALLOC_IDLE;
		tmp_p.indic = this;
		aptus->sponte(&tmp_p);
		/* 请求空闲的子实例, 将刚建立了连接的信息传过去, 然后由它工作 */
		if ( !(tmp_p.indic) || this == ( Amor* ) (tmp_p.indic) )
		{	/* 实例没有增加, 已经到达最大连接数，故关闭刚才的连接 */
			WLOG(NOTICE, "limited connections, to max");
			tcpsrv->end();	/* tcpsrv刚建立新连接, 其connfd保存最近值, 第一次调用不关闭listenfd */
		} else 
		{
			last_child = (Tcpsrvuna*)(tmp_p.indic);
			tcpsrv->herit(last_child->tcpsrv);
			last_child->child_begin();
			deliver(Notitia::NEW_SESSION); /* 父实例向接力者发出通知, 新建了连接 */
			if ( gCFG->lonely )
				end_service();
		}
		break;

	default:
		break;
	}
	return ;
}

TINLNE void Tcpsrvuna::child_pro(Amor::Pius *pius)
{	/* 子实例,接收和发送数据 */
	int len;	//看看读写成功与否

	switch ( pius->ordo )
	{
	case Notitia::FD_PRORD: //读数据,  不失败并有数据才向接力者传递
		len = tcpsrv->recito();
		if ( len > 0 ) 
		{
			WBUG("child_pro FD_PROFD recv bytes %d", len);
			deliver(Notitia::PRO_TBUF);
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
		break;
		
	case Notitia::FD_PROWR: //写, 很少见, 除非系统很忙
		WBUG("child_pro FD_PROWR");
		child_transmit();
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
	my_tor.scanfd = tcpsrv->listenfd;
	local_pius.ordo = Notitia::FD_CLRRD;
	aptus->sponte(&local_pius);	/* 向Sched, 以清除rdSet. */
	tcpsrv->endListen();	/* will close listenfd */

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

	local_pius.ordo = Notitia::FD_CLRRD;
	aptus->sponte(&local_pius);	//向Sched, 以清rdSet.
	
	local_pius.ordo = Notitia::FD_CLRWR;
	aptus->sponte(&local_pius);	//向Sched, 以清wrSet.
	
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
	return ;
}
#include "hook.c"

