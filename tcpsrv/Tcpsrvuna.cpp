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
	
	Describo::Criptor my_tor; /* �����׽���, ����ʵ����ͬ */

	Tcpsrv *tcpsrv;
	Tcpsrvuna *last_child;	/* ���һ�����ӵ���ʵ�� */

	bool isPioneer, isListener;
	TINLNE void child_begin();
	TINLNE void parent_begin();
	TINLNE void end_service();	/* �ر������׽��� */
	TINLNE void end(bool down=true);		/* �ر����� �� ֻ�ͷ��׽��� */

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
		gCFG->on_start = false; /* ����һ��ʼ������ */

	if ( (comm_str = cfg->Attribute("lonely") ) && strcmp(comm_str, "yes") ==0 )
		gCFG->lonely = true; /* �ڽ���һ�����Ӻ�, ���ر� */

	/* ��ʼ��Tcpsrv��ı������и�ֵ */
	tcpsrv->srvport = 0;
	tcpsrv->setPort( cfg->Attribute("port"));

	if ( (ip_str = cfg->Attribute("ip")) )
		TEXTUS_STRNCPY(tcpsrv->srvip, ip_str, sizeof(tcpsrv->srvip)-1);

	if ( (eth_str = cfg->Attribute("eth")) )
	{ 	/* ȷ���������ĸ����ڣ����������нϸ�����Ȩ */
		char ethfile[512];
		char *myip = (char*)0;
		TEXTUS_SNPRINTF(ethfile,sizeof(ethfile), "/etc/sysconfig/network-scripts/ifcfg-%s",eth_str);
		myip = BTool::getaddr(ethfile,"IPADDR");
		if (myip)
			TEXTUS_STRNCPY(tcpsrv->srvip, myip, sizeof(tcpsrv->srvip)-1);
	}
	/* End of ����Tcpsrv����� */

	isPioneer = true;	/* �Դ˱�־�Լ�Ϊ��ʵ�� */
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
			parent_pro(pius); /* ��Ȼ������ʵ��, ���ǽ��������� */
		else 
		 	child_pro(pius); /* ��ʵ��, Ӧ���Ƕ� */		
		break;

	case Notitia::CMD_NEW_SERVICE:
		WBUG("facio CMD_NEW_SERVICE");
		if (isPioneer)			/* ��Ӹ�ʵ������������ʵ�� */
			parent_pro(pius); /* new tcp service  */
		break;

	case Notitia::FD_PROWR:
		WBUG("facio FD_PROWR");	
		if (!isListener)	 /* ��ʵ��, Ӧ����д */
			child_pro(pius);
		break;
		
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");		
		if ( isPioneer && gCFG->on_start)
			parent_begin();		/* ��ʼ���� */
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
	case Notitia::PRO_TBUF :	//����һ֡���ݶ���
		WBUG("sponte PRO_TBUF");	
		assert(!isListener);	//����ʵ���������¶�.
		child_transmit();
		break;
		
	case Notitia::CMD_GET_FD:	//�����׽���������
		WBUG("sponte CMD_GET_FD");		
		if ( isListener)
			pius->indic = &(tcpsrv->listenfd);
		else
			pius->indic = &(tcpsrv->connfd);
		break;

	case Notitia::DMD_END_SESSION:	//ǿ�ƹرգ���ͬ�����رգ�Ҫ֪ͨ����
		WLOG(INFO,"DMD_END_SESSION, close %d", tcpsrv->connfd);
		end();
		break;

	case Notitia::CMD_RELEASE_SESSION:	/* �ͷ�����, ������н������ô��׽���, �����Ӳ��ر� */
		if ( !isListener)
		{
			WLOG(INFO,"CMD_RELEASE_SESSION, close %d", tcpsrv->connfd);
			end(false);
		}
		break;

	case Notitia::DMD_END_SERVICE:	//ǿ�ƹر�
		if ( isListener)
		{
			WLOG(NOTICE, "DMD_END_SERVICE, close %d", tcpsrv->listenfd);
			end_service();
		}
		break;
	
	case Notitia::DMD_BEGIN_SERVICE: //ǿ�ƿ���
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
	local_pius.ordo = Notitia::TEXTUS_RESERVED;	/* δ��, ����Notitia::FD_CLRWR�ȶ��ֿ��� */
	local_pius.indic = &my_tor;

	tcpsrv = new Tcpsrv();

	isPioneer = false;	/* Ĭ���Լ����Ǹ��ڵ� */
	isListener = false;
	tcpsrv->errMsg = &errMsg[0];	/* ������ͬ�Ĵ�����Ϣ���� */
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
{	/* ������ */
	if ( tcpsrv->listenfd > 0 )
		return ;
		
	if (!tcpsrv->servio(false))
	{
		SLOG(EMERG)
		return ;
	}
	my_tor.scanfd = tcpsrv->listenfd;
	local_pius.ordo = Notitia::FD_SETRD;
	aptus->sponte(&local_pius);	//��Sched, ������rdSet.
	deliver(Notitia::START_SERVICE); //������߷���֪ͨ, ������ʼ����
	return ;
}

TINLNE void Tcpsrvuna::child_begin()
{	
	my_tor.scanfd = tcpsrv->connfd;
	local_pius.ordo = Notitia::FD_SETRD;
	aptus->sponte(&local_pius);	//��Sched, ������rdSet.
	tcpsrv->rcv_buf->reset();	//TCP����(����)���������
	tcpsrv->snd_buf->reset();
	deliver(Notitia::START_SESSION); //������߷���֪ͨ, ������ʼ�Ự
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
		/* ������е���ʵ��, ���ս��������ӵ���Ϣ����ȥ, Ȼ���������� */
		if ( !(tmp_p.indic) || this == ( Amor* ) (tmp_p.indic) )
		{	/* ʵ��û������, �Ѿ�����������������ʹرողŵ����� */
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
			{ 	/* ȷ���������ĸ����ڣ����������нϸ�����Ȩ */
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
		{	/* ����������ʧ��, ��ȥ�� */
			SLOG(ALERT)
			return;	
		}

		if ( tcpsrv->connfd < 0 ) 
		{	SLOG(INFO)
			return;	/* ���ӻ�δ������, ��ȥ�ٵ� */
		}
		WLOG(INFO,"create socket %d", tcpsrv->connfd);
		tmp_p.ordo = Notitia::CMD_ALLOC_IDLE;
		tmp_p.indic = this;
		aptus->sponte(&tmp_p);
		/* ������е���ʵ��, ���ս��������ӵ���Ϣ����ȥ, Ȼ���������� */
		if ( !(tmp_p.indic) || this == ( Amor* ) (tmp_p.indic) )
		{	/* ʵ��û������, �Ѿ�����������������ʹرողŵ����� */
			WLOG(NOTICE, "limited connections, to max");
			tcpsrv->end();	/* tcpsrv�ս���������, ��connfd�������ֵ, ��һ�ε��ò��ر�listenfd */
		} else 
		{
			last_child = (Tcpsrvuna*)(tmp_p.indic);
			tcpsrv->herit(last_child->tcpsrv);
			last_child->child_begin();
			deliver(Notitia::NEW_SESSION); /* ��ʵ��������߷���֪ͨ, �½������� */
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
{	/* ��ʵ��,���պͷ������� */
	int len;	//������д�ɹ����

	switch ( pius->ordo )
	{
	case Notitia::FD_PRORD: //������,  ��ʧ�ܲ������ݲ�������ߴ���
		len = tcpsrv->recito();
		if ( len > 0 ) 
		{
			WBUG("child_pro FD_PROFD recv bytes %d", len);
			deliver(Notitia::PRO_TBUF);
		} else {
			if ( len == 0 || len == -1)	/* ����־ */
			{
				SLOG(INFO)
			} else
			{
				SLOG(NOTICE)
			}
			if ( len < 0 )  end();	//ʧ�ܼ��ر�
		}
		break;
		
	case Notitia::FD_PROWR: //д, ���ټ�, ����ϵͳ��æ
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
	case 0: //û������, ���ֲ���
		break;
		
	case 2: //ԭ������, û��������, ��һ��
		local_pius.ordo =Notitia::FD_CLRWR;
		//��Sched, ������wrSet.
		aptus->sponte(&local_pius);	
		break;
		
	case 1:	//��д����, ��Ҫ��һ����
		SLOG(INFO)
		//��Sched, ������wrSet.
		local_pius.ordo =Notitia::FD_SETWR;
		aptus->sponte(&local_pius);
		break;
		
	case 3:	//����д����, ����
		break;
		
	case -1://�����ش���, �ر�
		SLOG(WARNING)
		end();
		break;
	default:
		break;
	}
}

TINLNE void Tcpsrvuna::end_service()
{	/* ����ر� */
	Amor::Pius tmp_p;

	if ( tcpsrv->listenfd <= 0 ) 
		return;
	my_tor.scanfd = tcpsrv->listenfd;
	local_pius.ordo = Notitia::FD_CLRRD;
	aptus->sponte(&local_pius);	/* ��Sched, �����rdSet. */
	tcpsrv->endListen();	/* will close listenfd */

	if ( !isPioneer )
	{	/* ��ʵ�������ջ�, ����������ʵ�� */
		isListener = false;
		tmp_p.ordo = Notitia::CMD_FREE_IDLE;
		tmp_p.indic = this;
		aptus->sponte(&tmp_p);
	}
	deliver(Notitia::END_SERVICE); //����һ�����ݱ���ĻỰ�ر��ź�
}

TINLNE void Tcpsrvuna::end(bool down)
{	/* ����ر� */
	Amor::Pius tmp_p;
	if ( isListener )
		return;

	if ( tcpsrv->connfd == -1 ) 	/* �Ѿ��رջ�δ��ʼ */
		return;

	local_pius.ordo = Notitia::FD_CLRRD;
	aptus->sponte(&local_pius);	//��Sched, ����rdSet.
	
	local_pius.ordo = Notitia::FD_CLRWR;
	aptus->sponte(&local_pius);	//��Sched, ����wrSet.
	
	if ( down)
		tcpsrv->end();		//TcpsrvҲ�ر�
	else
		tcpsrv->release();	//Tcpsrv����

	tmp_p.ordo = Notitia::CMD_FREE_IDLE;
	tmp_p.indic = this;
	aptus->sponte(&tmp_p);

	if ( down)
		deliver(Notitia::END_SESSION); //����һ�����ݱ���ĻỰ�ر��ź�
}

Amor* Tcpsrvuna::clone()
{
	Tcpsrvuna *child = new Tcpsrvuna();
	child->gCFG = gCFG;
	return (Amor*)child;
}

/* ��������ύ */
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

