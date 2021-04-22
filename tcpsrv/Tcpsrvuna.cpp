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
	bool has_config;
	struct G_CFG {
		bool on_start ;
		bool lonely;
		int back_log;
		bool use_epoll;
		Amor *sch;
		struct DPoll::PollorBase lor; /* ̽ѯ */
		G_CFG() {
			on_start = true;
			back_log = 100;
			use_epoll = false;
			lor.type = DPoll::NotUsed;
			sch = 0;
			lonely = false;
		};
	};
	struct G_CFG *gCFG;

	Amor::Pius local_pius;
	char errMsg[1024];
	
	Describo::Criptor my_tor; /* �����׽���, ����ʵ����ͬ */
	DPoll::Pollor pollor; /* �����¼����, ����ʵ����ͬ */
	Amor::Pius epl_set_ps, epl_clr_ps, pro_tbuf_ps;
	TBuffer *tb_arr[3];

	Tcpsrv *tcpsrv;
	Tcpsrvuna *last_child;	/* ���һ�����ӵ���ʵ�� */

	bool isPioneer, isListener;
	void child_begin();
	void parent_begin();
	void end_service();	/* �ر������׽��� */
	void end(bool down=true);		/* �ر����� �� ֻ�ͷ��׽��� */

	void child_rcv_pro(TEXTUS_LONG len, const char *msg);
	void parent_accept();
	void child_transmit();
	void child_transmit_ep();
	void deliver(Notitia::HERE_ORDO aordo);
	void new_conn_pro();
#if defined (_WIN32 )	
	void do_accept_ex();
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
	TiXmlElement *cfg;
	Amor::Pius tmp_p;
	TEXTUS_LONG len;

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
			parent_accept(); /* ��Ȼ������ʵ��, ���ǽ��������� */
		} else {
			child_rcv_pro(tcpsrv->recito(), "child_pro FD_PROFD recv bytes"); /* ��ʵ��, Ӧ���Ƕ� */		
		}
		break;

	case Notitia::ACCEPT_EPOLL:
		WBUG("facio ACCEPT_EPOLL");
#if defined (_WIN32 )	
		aget = (OVERLAPPED_ENTRY *)pius->indic;
		/* �Ѿ��������� */
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
		parent_accept(); /* ��Ȼ������ʵ��, ���ǽ��������� */
		/* action flags and filter for event remain unchanged */
		gCFG->sch->sponte(&epl_set_ps);	//��tpoll,  ��һ��ע��
#endif
		break;

	case Notitia::ERR_EPOLL:
		WBUG("facio ERR_EPOLL");
		WLOG(WARNING, (char*)pius->indic);	
		end();	//ֱ�ӹرվͿ�.
		break;

#if defined (_WIN32 )	
	case Notitia::MORE_DATA_EPOLL:
		WBUG("facio MORE_DATA_EPOLL");
		WLOG(WARNING, (char*)pius->indic);	
		tcpsrv->wsa_rcv.len *= 2;
		tcpsrv->rcv_frame_size =  tcpsrv->wsa_rcv.len;
		if ( !tcpsrv->recito_ex())
		{
			SLOG(ERR)
			end();	//ʧ�ܼ��ر�
		}
		break;
#endif
	case Notitia::PRO_EPOLL:
		WBUG("facio PRO_EPOLL");
#if defined (_WIN32 )	
		aget = (OVERLAPPED_ENTRY *)pius->indic;
		if ( aget->lpOverlapped == &(tcpsrv->rcv_ovp) )
		{	//�Ѷ�����,  ��ʧ�ܲ������ݲ�������ߴ���
			if ( aget->dwNumberOfBytesTransferred ==0 ) 
			{
				WLOG(INFO, "IOCP recv 0 disconnected");
				end();
			} else {
				WBUG("child PRO_EPOLL recv %d bytes", aget->dwNumberOfBytesTransferred);
				tcpsrv->rcv_buf->commit(aget->dwNumberOfBytesTransferred);
				aptus->facio(&pro_tbuf_ps);
				if ( !tcpsrv->recito_ex())
				{
					SLOG(ERR)
					end();	//ʧ�ܼ��ر�
				}
			}
		} else if ( aget->lpOverlapped == &(tcpsrv->snd_ovp) ) {
			WBUG("child PRO_EPOLL sent %d bytes", aget->dwNumberOfBytesTransferred); //д�������
		} else {
			WLOG(ALERT, "not my overlap");
		}
#endif
		break;

	case Notitia::RD_EPOLL:
		WBUG("facio RD_EPOLL");
LOOP:
		switch ( (len = tcpsrv->recito()) ) 
		{
		case 0:	//Pending
			SLOG(INFO)
			/* action flags and filter for event remain unchanged */
			gCFG->sch->sponte(&epl_set_ps);	//��tpoll,  ��һ��ע��
			break;

		case -1://Close
			SLOG(INFO)
			end();	//ʧ�ܼ��ر�
			break;

		case -2://Error
			SLOG(NOTICE)
			end();	//ʧ�ܼ��ر�
			break;

		default:	
			WBUG("child recv " TLONG_FMT " bytes", len);
			if ( len <  tcpsrv->rcv_frame_size ) { 
				/* action flags and filter for event remain unchanged */
				gCFG->sch->sponte(&epl_set_ps);	//��tpoll,  ��һ��ע��
				aptus->facio(&pro_tbuf_ps);
			} else if (  len == tcpsrv->rcv_frame_size ) {
				aptus->facio(&pro_tbuf_ps);
				goto LOOP;
			} else if (  len > tcpsrv->rcv_frame_size ) {
				WLOG(EMERG, "child recv %ld bytes > rcv_size %d", len, tcpsrv->rcv_frame_size);
			}
			break;
		}
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
		if (!isPioneer)			/* ��Ӹ�ʵ������������ʵ�� */
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
		/* ������е���ʵ��, �·��� */
		if ( !(tmp_p.indic) || this == ( Amor* ) (tmp_p.indic) )
		{	/* ʵ��û������, �Ѿ�����������������ʹرողŵ����� */
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
			if ( ip_str) {
				cfg->SetAttribute("ip", ip_str);	
			}
		}
		break;

	case Notitia::FD_PROWR:
		WBUG("facio FD_PROWR");	
		if (!isListener)	 /* ��ʵ��, Ӧ����д */
			child_transmit();
		break;
		
	case Notitia::FD_PROEX:
		WBUG("facio FD_PROEX");	
		if (!isListener)	 /* ��ʵ��, Ӧ����д */
			end();
		break;
		
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");		
		tmp_p.ordo = Notitia::CMD_GET_SCHED;
		aptus->sponte(&tmp_p);	//��tpoll, ȡ��sched
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
		gCFG->sch->sponte(&tmp_p);	//��tpoll, ȡ��TPOLL
		if ( tmp_p.indic == gCFG->sch)
			gCFG->use_epoll = true;
		else
			gCFG->use_epoll = false;

		if ( isPioneer && gCFG->on_start)
			parent_begin();		/* ��ʼ���� */
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY " TLONG_FMTu, pius->ordo);			
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
		if ( gCFG->use_epoll)
		{
			child_transmit_ep();
		} else {
			child_transmit();
		}
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
		if (gCFG->use_epoll ) {
			gCFG->sch->sponte(&epl_clr_ps);	//��tpoll,  ע��
		} else {
			local_pius.ordo = Notitia::FD_CLRRD;
			gCFG->sch->sponte(&local_pius);	//��Sched, ����rdSet.
		}
		break;

	case Notitia::CMD_CHANNEL_RESUME :
		WBUG("sponte CMD_CHANNEL_RESUME");
		if (gCFG->use_epoll ) {
			gCFG->sch->sponte(&epl_set_ps);	//��tpoll,  ע��
		} else {
			local_pius.ordo = Notitia::FD_SETRD;
			gCFG->sch->sponte(&local_pius);	//��Sched, ������rdSet.
		}
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
	pro_tbuf_ps.indic = &tb_arr[0];

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
{	/* ������ */
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
		gCFG->sch->sponte(&local_pius);	//��Sched, ������rdSet.
	} else {
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
		//pollor.events = POLLIN|POLLOUT;
		pollor.events = POLLIN;
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		EV_SET(&(pollor.events[0]), tcpsrv->listenfd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, &pollor);
		EV_SET(&(pollor.events[1]), tcpsrv->listenfd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, &pollor);
#endif	//for bsd
		gCFG->sch->sponte(&epl_set_ps);	//��tpoll

#if  defined(__linux__)
		pollor.op = EPOLL_CTL_MOD; //�Ժ���������޸��ˡ�
#endif	//for linux

#if defined (_WIN32 )	
		do_accept_ex();
#endif
	}
	deliver(Notitia::START_SERVICE); //������߷���֪ͨ, ������ʼ����
	return ;
}

void Tcpsrvuna::child_rcv_pro(TEXTUS_LONG len, const char *msg)
{
	if ( len > 0 ) 
	{
		WBUG("%s " TLONG_FMT, msg, len);
		aptus->facio(&pro_tbuf_ps);
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
}

#if defined (_WIN32 )	
void Tcpsrvuna::do_accept_ex()
{
	int ret;
	while ( (ret = tcpsrv->accept_ex()) != 0 ) 	//�첽������ �Ƚ��գ���Ͷ�ݵ�IOCP
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

void Tcpsrvuna::child_begin()
{	
	tcpsrv->rcv_buf->reset();	//TCP����(����)���������
	tcpsrv->snd_buf->reset();
	if (gCFG->use_epoll)
	{
#if defined (_WIN32 )	
		pollor.hnd.sock = tcpsrv->connfd;
		pollor.pro_ps.ordo = Notitia::PRO_EPOLL;
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

		gCFG->sch->sponte(&epl_set_ps);	//��tpoll, ������iocp��
		/* �����Ǻ������� */

#if  defined(__linux__)
		pollor.op = EPOLL_CTL_MOD; //�Ժ���������޸��ˡ�
#endif	//for linux

#if defined (_WIN32 )	
		/* ����ȥ����, ���һ��ʼ������, ���Ƚ���; ����ʵ��IOCPͶ�� */
		if ( !tcpsrv->recito_ex())
		{
			SLOG(ERR)
			end();	//ʧ�ܼ��ر�
			return;
		}
#endif	//for WIN32
	} else {
		my_tor.scanfd = tcpsrv->connfd;
		local_pius.ordo = Notitia::FD_SETRD;
		gCFG->sch->sponte(&local_pius);	//��Sched, ������rdSet.
		local_pius.ordo = Notitia::FD_SETEX;
		gCFG->sch->sponte(&local_pius);	//��Sched, ������rdSet.
	}

	deliver(Notitia::START_SESSION); //������߷���֪ͨ, ������ʼ�Ự
	return;
}

void Tcpsrvuna::parent_accept()
{
	if ( !tcpsrv->accipio(false)) 
	{	/* ����������ʧ��, ��ȥ�� */
		SLOG(ALERT)
		return;	
	}

	if ( tcpsrv->connfd ==INVALID_SOCKET ) 
	{	
		SLOG(INFO)
		return;	/* ���ӻ�δ������, ��ȥ�ٵ� */
	}
	new_conn_pro();
}

void Tcpsrvuna::new_conn_pro()
{
	Amor::Pius tmp_p;
	WLOG(INFO,"create socket %d", tcpsrv->connfd);
	tmp_p.ordo = Notitia::CMD_ALLOC_IDLE;
	tmp_p.indic = this;
	aptus->sponte(&tmp_p);
	/* ������е���ʵ��, ���ս��������ӵ���Ϣ����ȥ, Ȼ���������� */
	if ( !(tmp_p.indic) || this == ( Amor* ) (tmp_p.indic) )
	{	/* ʵ��û������, �Ѿ�����������������ʹرողŵ����� */
		WLOG(NOTICE, "limited connections for neo_conn, to max");
		tcpsrv->end();	/* tcpsrv�ս���������, ��connfd�������ֵ, ��һ�ε��ò��ر�listenfd */
	} else {
		last_child = (Tcpsrvuna*)(tmp_p.indic);
		tcpsrv->herit(last_child->tcpsrv);
		last_child->child_begin();
		deliver(Notitia::NEW_SESSION); /* ��ʵ��������߷���֪ͨ, �½������� */
		if ( gCFG->lonely )
			end_service();
	}
}

void Tcpsrvuna::child_transmit_ep()
{
#if defined (_WIN32 )	
	switch ( tcpsrv->transmitto_ex() )
#else
	switch ( tcpsrv->transmitto() )
#endif
	{
	case 0: //û������, ����
		break;
	
	case 2: //ԭ������, û��������, ��һ��
#if  defined(__linux__)
		pollor.ev.events &= ~EPOLLOUT;	//����һ������POLLINʱ����
#endif	//for linux

#if  defined(__sun)
		pollor.events &= ~POLLOUT;	//����һ������POLLINʱ����
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		pollor.events[1].flags = EV_ADD | EV_DISABLE;	//����һ������ʱ����
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

#if !defined(_WIN32)	
		gCFG->sch->sponte(&epl_set_ps);	//��tpoll, ������kqueue��
#endif
		break;
	
	case -1://�����ش���, �ر�
		SLOG(WARNING)
		end();
		break;

	default:
		break;
	}
}

void Tcpsrvuna::child_transmit()
{
	switch ( tcpsrv->transmitto() )
	{
	case 0: //û������, ���ֲ���
		break;
		
	case 2: //ԭ������, û��������, ��һ��
		local_pius.ordo =Notitia::FD_CLRWR;
		//��Sched, ������wrSet.

		gCFG->sch->sponte(&local_pius);	
		break;
		
	case 1:	//��д����, ��Ҫ��һ����
		SLOG(INFO)
		//��Sched, ������wrSet.
		local_pius.ordo =Notitia::FD_SETWR;
		gCFG->sch->sponte(&local_pius);
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

void Tcpsrvuna::end_service()
{	/* ����ر� */
	Amor::Pius tmp_p;

	if ( tcpsrv->listenfd == INVALID_SOCKET ) 
		return;
	if (!gCFG->use_epoll)
	{
		my_tor.scanfd = tcpsrv->listenfd;
		local_pius.ordo = Notitia::FD_CLRRD;
		gCFG->sch->sponte(&local_pius);	/* ��Sched, �����rdSet. */
	}
	tcpsrv->endListen();	/* will close listenfd */
	
		/* BSD: Calling close() on a file descriptor wil remove any kevents that reference the descriptor 
		  Events which are attached to file descriptors are automatically deleted on the last close of the descriptor.
		*/
		/* linux: closing a file descriptor cause it to be removed from all epoll sets */
		/*sun: The association is also  removed  if  the  port  gets  closed */

	if ( !isPioneer )
	{	/* ��ʵ�������ջ�, ����������ʵ�� */
		isListener = false;
		tmp_p.ordo = Notitia::CMD_FREE_IDLE;
		tmp_p.indic = this;
		aptus->sponte(&tmp_p);
	}
	deliver(Notitia::END_SERVICE); //����һ�����ݱ���ĻỰ�ر��ź�
}

void Tcpsrvuna::end(bool down)
{	/* ����ر� */
	Amor::Pius tmp_p;
	if ( isListener )
		return;

	if ( tcpsrv->connfd == INVALID_SOCKET ) 	/* �Ѿ��رջ�δ��ʼ */
		return;

	if (!gCFG->use_epoll)
	{
		local_pius.ordo = Notitia::FD_CLRRD;
		gCFG->sch->sponte(&local_pius);	//��Sched, ����rdSet.
	
		local_pius.ordo = Notitia::FD_CLRWR;
		gCFG->sch->sponte(&local_pius);	//��Sched, ����wrSet.
	
		local_pius.ordo = Notitia::FD_CLREX;
		gCFG->sch->sponte(&local_pius);	//��Sched, ����exSet.
	} 
	// else gCFG->sch->sponte(&epl_clr_ps);	//��tpoll,  ע��
	/* just close for all kinds of system ?  */

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
	default:
		WBUG("deliver Notitia::%d", aordo);
		break;
	}
	aptus->facio(&tmp_pius);
}
#include "hook.c"
