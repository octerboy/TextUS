/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 Title: udp to textus
 Build: created by octerboy, 2006/10/10
 $Header: /textus/udp/Udpuna.cpp 6     06-12-29 11:41 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: Udpuna.cpp $"
#define TEXTUS_MODTIME  "$Date: 06-12-29 11:41 $"
#define TEXTUS_BUILDNO  "$Revision: 6 $"
#include "version_1.c"
/* $NoKeywords: $ */

#include "BTool.h"
#include <assert.h>
#include "wlog.h"

#define SLOG(Z) { Amor::Pius log_pius; \
		log_pius.ordo = Notitia::LOG_##Z; \
		log_pius.indic = &errMsg[0]; \
		aptus->sponte(&log_pius); \
		}

#include "Udp.h"
#include "Notitia.h"
#include "Describo.h"
#include "Amor.h"
#include "textus_string.h"

#define UDPINLINE inline
class Udpuna: public Amor
{
public:
	void ignite(TiXmlElement *);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
	Udpuna();
	~Udpuna();
	
private:
	Amor::Pius local_pius;
	char errMsg[1024];
	
	Describo::Criptor my_tor; /* 保存套接字, 各子实例不同 */

	Udp *udp;
	Udpuna *last_child;	/* 最近一次连接的子实例 */

	bool isPioneer;
	bool isServer, isSingle;
	bool isRecving;
	UDPINLINE void transmit();
	UDPINLINE void begin_rcv();
	UDPINLINE void end();
	UDPINLINE void rw_pro(Amor::Pius *);
	UDPINLINE void parent_pro();
	UDPINLINE void deliver(Notitia::HERE_ORDO aordo);
};

void Udpuna::ignite(TiXmlElement *cfg)
{
	const char  *eth_str, *ip_str, *port_str, *on_start_str, *type_str;
	TiXmlElement *loc_ele, *rem_ele;
	assert(cfg);

	isServer = ( (type_str = cfg->Attribute("type") ) && strcasecmp( type_str, "server") == 0 );
	isSingle = ( (type_str = cfg->Attribute("single") ) && strcasecmp( type_str, "yes") == 0 );

	loc_ele = cfg->FirstChildElement("local");
	if ( !loc_ele )
		goto STEP1;

	if ( (port_str = loc_ele->Attribute("port")) )
	{
		int port = atoi(port_str);
		if ( port > 0 && port < 65535 )
			udp->local_port = port;
	}

	if ( (ip_str = loc_ele->Attribute("ip")) )
		TEXTUS_STRNCPY(udp->local_ip, ip_str, sizeof(udp->local_ip)-1);

	if ( (eth_str = loc_ele->Attribute("eth")) )
	{ 	/* 确定从哪个网口接收，网口设置有较高优先权 */
		char ethfile[512];
		char *myip = (char*)0;
		TEXTUS_SNPRINTF(ethfile,sizeof(ethfile), "/etc/sysconfig/network-scripts/ifcfg-%s",eth_str);
		myip = BTool::getaddr(ethfile,"IPADDR");
		if (myip)
			TEXTUS_STRNCPY(udp->local_ip, myip, sizeof(udp->local_ip)-1);
	}

STEP1:
	rem_ele = cfg->FirstChildElement("remote");
	if ( !loc_ele )
		goto STEP2;
	ip_str = loc_ele->Attribute("ip");
	port_str = cfg->Attribute("port");
	udp->peer_bind(ip_str, port_str);

STEP2:
	if ( !udp->loc_bind() )
		SLOG(EMERG);		

	isPioneer = true;	/* 以此标志自己为父实例 */
}

bool Udpuna::facio( Amor::Pius *pius)
{
	assert(pius);
	
	switch(pius->ordo)
	{
	case Notitia::FD_PRORD:
		WBUG("facio FD_PRORD");
		rw_pro(pius); /* 读写处理 */
		break;

	case Notitia::FD_PROWR:
		WBUG("facio FD_PROWR");	
		rw_pro(pius); /* 读写处理 */
		break;
		
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");		
		if ( isServer ) 
			begin_rcv();
		deliver(Notitia::SET_TBUF);
	
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY %d" , pius->ordo);			
		deliver(Notitia::SET_TBUF);
		break;

	default:
		return false;
	}
	return true;
}

bool Udpuna::sponte( Amor::Pius *pius)
{
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF :	//处理一帧数据而已
		WBUG("sponte PRO_TBUF");	
		transmit();
		begin_rcv();
		break;
		
	case Notitia::CMD_GET_FD:	//给出套接字描述符
		WBUG("sponte CMD_GET_FD");		
		pius->indic = &(udp->sockfd);
		break;

	case Notitia::DMD_END_SESSION:	//强制关闭
		WLOG(NOTICE, "DMD_END_SESSION, close %d", udp->sockfd);
		end();
		break;
	
	case Notitia::DMD_BEGIN_SERVICE: //强制开启
		WLOG(NOTICE, "DMD_BEGIN_SERVICE");
		begin_rcv();
		break;
	
	default:
		return false;
	}
	return true;
}

Udpuna::Udpuna()
{
	my_tor.pupa = this;
	local_pius.ordo = -1;	/* 未定, 可有Notitia::FD_CLRWR等多种可能 */
	local_pius.indic = &my_tor;

	udp = new Udp();

	isPioneer = false;	/* 默认自己不是父节点 */
	udp->errMsg = &errMsg[0];	/* 设置相同的错误信息缓冲 */
	udp->errstr_len = 1024;
	isRecving = false;
	isServer = false;
}

Udpuna::~Udpuna()
{
	end();
	delete udp;
}

UDPINLINE void Udpuna::begin_rcv()
{	/* 开启接收 */
	if (udp->sockfd < 0)
		return ;

	if ( isRecving ) 
		return;

	my_tor.scanfd = udp->sockfd;
	local_pius.ordo = Notitia::FD_SETRD;
	aptus->sponte(&local_pius);	/* 向Sched, 以设置rdSet. */
	isRecving = true;
	return ;
}

UDPINLINE void Udpuna::parent_pro()
{
	Amor::Pius tmp_p;
	tmp_p.ordo = Notitia::CMD_ALLOC_IDLE;
	tmp_p.indic = this;
	aptus->sponte(&tmp_p);
	/* 请求空闲的子实例, 将刚建立了连接的信息传过去, 然后由它工作 */
	if ( !(tmp_p.indic) || this == ( Amor* ) (tmp_p.indic) )
	{	/* 实例没有增加, 已经到达最大连接数，故关闭刚才的连接 */
		WLOG(NOTICE, "limited connections, to max");
		udp->end();	/* udp刚建立新连接 */
	} else {
		last_child = (Udpuna*)(tmp_p.indic);
		udp->herit(last_child->udp);
		udp->loc_bind();
		TBuffer::exchange(*last_child->udp->rcv_buf, *udp->rcv_buf);
		last_child->deliver(Notitia::PRO_TBUF);
	}

	return ;
}

UDPINLINE void Udpuna::rw_pro(Amor::Pius *pius)
{	/* 接收和发送数据 */
	int len;	/* 看看读写成功与否 */

	switch ( pius->ordo )
	{
	case Notitia::FD_PRORD: //读数据,  不失败并有数据才向接力者传递
		len = udp->recito();
		if ( len > 0 ) 
		{
			WBUG("rw_pro recv bytes %d", len);
			if ( isPioneer & !isSingle & isServer)
				parent_pro();
			else
				deliver(Notitia::PRO_TBUF);
		} else {
			if ( len == 0 || len == -1)	/* 记日志 */
			{
				SLOG(INFO);
			} else
			{
				SLOG(NOTICE);
			}
			if ( len < 0 )  end();	/* 失败即关闭 */
		}
		break;
		
	case Notitia::FD_PROWR: //写, 少见
		WBUG("FD_PROWR");
		transmit();
		break;
		
	default:
		break;
	}
}

UDPINLINE void Udpuna::transmit()
{
	switch ( udp->transmitto() )
	{
	case 0: //没有阻塞, 保持不变
		break;
		
	case 2: //原有阻塞, 没有阻塞了, 清一下
		local_pius.ordo =Notitia::FD_CLRWR;
		//向Sched, 以设置wrSet.
		aptus->sponte(&local_pius);	
		break;
		
	case 1:	//新写阻塞, 需要设一下了
		SLOG(INFO);
		//向Sched, 以设置wrSet.
		local_pius.ordo =Notitia::FD_SETWR;
		aptus->sponte(&local_pius);
		break;
		
	case 3:	//还是写阻塞, 不变
		break;
		
	case -1://有严重错误, 关闭
		SLOG(WARNING);
		end();
		break;
	default:
		break;
	}
}

UDPINLINE void Udpuna::end()
{
	if ( udp->sockfd == -1 ) 	/* 已经关闭或未开始 */
		return;

	my_tor.scanfd = udp->sockfd;
	local_pius.ordo = Notitia::FD_CLRRD;
	aptus->sponte(&local_pius);	//向Sched, 以清rdSet.
	
	local_pius.ordo = Notitia::FD_CLRWR;
	aptus->sponte(&local_pius);	//向Sched, 以清wrSet.
	
	udp->end();		//Udp也关闭
	isRecving = false;
	if ( !isPioneer)
	{
		Amor::Pius tmp_p;
		tmp_p.ordo = Notitia::CMD_FREE_IDLE;
		tmp_p.indic = this;
		aptus->sponte(&tmp_p);
	}
}

Amor* Udpuna::clone()
{
	Udpuna *child = new Udpuna();

	udp->herit(child->udp);
	child->udp->loc_bind(false);
	return (Amor*)child;
}

/* 向接力者提交 */
UDPINLINE void Udpuna::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	TBuffer *tb[3];
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;

	switch (aordo )
	{
	case Notitia::SET_TBUF:
		WBUG("deliver SET_TBUF");
		tb[0] = udp->rcv_buf;
		tb[1] = udp->snd_buf;
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

