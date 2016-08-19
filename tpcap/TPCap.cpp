/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
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
 $Header: /textus/tpcap/TPCap.cpp 4     12-04-04 17:52 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: TPCap.cpp $"
#define TEXTUS_MODTIME  "$Date: 12-04-04 17:52 $"
#define TEXTUS_BUILDNO  "$Revision: 4 $"
/* $NoKeywords: $ */

#include "Notitia.h"
#include "Describo.h"
#include "Amor.h"
#include "TBuffer.h"
#include "BTool.h"
#include "casecmp.h"
#include "textus_string.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if_arp.h>

#include <linux/if_ether.h>
#include <linux/if_packet.h>

#ifdef SO_ATTACH_FILTER
#include <linux/types.h>
#include <linux/filter.h>
#endif

#define PCAP_ERRBUF_SIZE 	100
#define MAX_LINKHEADER_SIZE	256

static struct sock_filter	total_insn
	= BPF_STMT(BPF_RET | BPF_K, 0);
static struct sock_fprog	total_fcode
	= { 1, &total_insn };


static int reset_kernel_filter(int sock_fd);
static int set_kernel_filter(int sock_fd, struct sock_fprog *fcode, char *ebuf);
static int iface_get_mtu(int fd, const char *device, char *ebuf);
static int iface_bind(int fd, int ifindex, char *ebuf);
static int iface_get_id(int fd, const char *device, char *ebuf);
static int iface_get_arptype(int fd, const char *device, char *ebuf);


#define TINLINE inline

class TPCap: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	TPCap();
	~TPCap();

private:
	Amor::Pius pro_tbuf;
	Amor::Pius local_pius;
	Describo::Criptor my_tor; /* 保存套接字, 各子实例不同 */

	TBuffer rcv_buf, snd_buf;	/* 向右(左)传递 */
	int sock_fd , arptype, ifindex, mtu, bufsize;
	char ebuf[PCAP_ERRBUF_SIZE];

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

void TPCap::ignite(TiXmlElement *prop)
{
	if (!prop) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(prop);
		has_config = true;
	}
}

TPCap::TPCap():rcv_buf(8192)
{
	pro_tbuf.ordo = Notitia::PRO_TBUF;
	pro_tbuf.indic = 0;

	my_tor.pupa = this;
	local_pius.ordo = Notitia::TEXTUS_RESERVED;	/* 未定, 可有Notitia::FD_CLRWR等多种可能 */
	local_pius.indic = &my_tor;

	gCFG = 0;
	has_config = false;
}

TPCap::~TPCap() 
{
	if ( has_config  )
	{	
		if(gCFG) delete gCFG;
	}
}

Amor* TPCap::clone()
{
	TPCap *child = new TPCap();
	child->gCFG = gCFG;
	return (Amor*) child;
}

bool TPCap::facio( Amor::Pius *pius)
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
		goto DEL_BUF;
		
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
DEL_BUF:
		deliver(Notitia::SET_TBUF);
		break;

	default:
		return false;
	}
	return true;
}

bool TPCap::sponte( Amor::Pius *pius)
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

void TPCap::init()
{
	int err;
	assert(gCFG->eth != (const char*) 0 );
	if ( !gCFG->eth)
		return ;

	sock_fd = -1;
	sock_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if ( sock_fd == -1 )
	{
		WLOG_OSERR("socket");
		return;
	}

	arptype = iface_get_arptype(sock_fd, gCFG->eth, ebuf);
	if ( arptype == - 1 )
	{
		WLOG(ERR, "iface_get_arptype %s", ebuf);
		goto ERROR_PRO;
	}

	ifindex = iface_get_id(sock_fd, gCFG->eth, ebuf);
	if ( arptype == - 1 )
	{
		WLOG(ERR, "iface_get_id %s", ebuf);
		goto ERROR_PRO;
	}

	if ((err = iface_bind(sock_fd, ifindex, ebuf)) < 0)
	{
		WLOG(ERR, "iface_bind %s", ebuf);
		goto ERROR_PRO;
	}

	if ( (mtu = iface_get_mtu(sock_fd, gCFG->eth, ebuf)) == - 1)
	{
		WLOG(ERR, "iface_get_mtu %s", ebuf);
		goto ERROR_PRO;
	}
	bufsize = MAX_LINKHEADER_SIZE + mtu;

	my_tor.scanfd = sock_fd;
	local_pius.ordo = Notitia::FD_SETRD;
	aptus->sponte(&local_pius);	//向Sched, 以设置rdSet.
	return ;

ERROR_PRO:
	if ( sock_fd != -1 )
	{
		close(sock_fd);
		return ;
	}
}

TINLINE bool TPCap::handle()
{
	int rlen;

	rcv_buf.grant(bufsize);	 //保证有足够空间
ReadAgain:
	rlen = recv(sock_fd, rcv_buf.point, bufsize, MSG_TRUNC);
	if( rlen > 0 ) 
	{
		rcv_buf.commit(rlen);   /* 指针向后移 */	
		return true;
	} else {
		int error = errno;
		if (error == EINTR)
		{	 //有信号而已,再试
			goto ReadAgain;
		} else if ( error == EAGAIN || error == EWOULDBLOCK )
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
void TPCap::deliver(Notitia::HERE_ORDO aordo)
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
		WBUG("deliver Notitia::%d", aordo);
		break;
	}
	aptus->facio(&tmp_pius);
	return ;
}

static int
set_kernel_filter(int sock_fd, struct sock_fprog *fcode, char *ebuf)
{
	int total_filter_on = 0;
	int save_mode;
	int ret;
	int save_errno;

	/*
	 * The socket filter code doesn't discard all packets queued
	 * up on the socket when the filter is changed; this means
	 * that packets that don't match the new filter may show up
	 * after the new filter is put onto the socket, if those
	 * packets haven't yet been read.
	 *
	 * This means, for example, that if you do a tcpdump capture
	 * with a filter, the first few packets in the capture might
	 * be packets that wouldn't have passed the filter.
	 *
	 * We therefore discard all packets queued up on the socket
	 * when setting a kernel filter.  (This isn't an issue for
	 * userland filters, as the userland filtering is done after
	 * packets are queued up.)
	 *
	 * To flush those packets, we put the socket in read-only mode,
	 * and read packets from the socket until there are no more to
	 * read.
	 *
	 * In order to keep that from being an infinite loop - i.e.,
	 * to keep more packets from arriving while we're draining
	 * the queue - we put the "total filter", which is a filter
	 * that rejects all packets, onto the socket before draining
	 * the queue.
	 *
	 * This code deliberately ignores any errors, so that you may
	 * get bogus packets if an error occurs, rather than having
	 * the filtering done in userland even if it could have been
	 * done in the kernel.
	 */
	if (setsockopt(sock_fd, SOL_SOCKET, SO_ATTACH_FILTER,
		       &total_fcode, sizeof(total_fcode)) == 0) {
		char drain[1];

		/*
		 * Note that we've put the total filter onto the socket.
		 */
		total_filter_on = 1;

		/*
		 * Save the socket's current mode, and put it in
		 * non-blocking mode; we drain it by reading packets
		 * until we get an error (which is normally a
		 * "nothing more to be read" error).
		 */
		save_mode = fcntl(sock_fd, F_GETFL, 0);
		if (save_mode != -1 &&
		    fcntl(sock_fd, F_SETFL, save_mode | O_NONBLOCK) >= 0) {
			while (recv(sock_fd, &drain, sizeof drain,
			       MSG_TRUNC) >= 0)
				;
			save_errno = errno;
			fcntl(sock_fd, F_SETFL, save_mode);
			if (save_errno != EAGAIN) {
				/* Fatal error */
				reset_kernel_filter(sock_fd);
				snprintf(ebuf, sizeof(ebuf),
				 "recv: %s", strerror(save_errno));
				return -2;
			}
		}
	}

	/*
	 * Now attach the new filter.
	 */
	ret = setsockopt(sock_fd, SOL_SOCKET, SO_ATTACH_FILTER,
			 fcode, sizeof(*fcode));
	if (ret == -1 && total_filter_on) {
		/*
		 * Well, we couldn't set that filter on the socket,
		 * but we could set the total filter on the socket.
		 *
		 * This could, for example, mean that the filter was
		 * too big to put into the kernel, so we'll have to
		 * filter in userland; in any case, we'll be doing
		 * filtering in userland, so we need to remove the
		 * total filter so we see packets.
		 */
		save_errno = errno;

		/*
		 * XXX - if this fails, we're really screwed;
		 * we have the total filter on the socket,
		 * and it won't come off.  What do we do then?
		 */
		reset_kernel_filter(sock_fd);

		errno = save_errno;
	}
	return ret;
}

static int
reset_kernel_filter(int sock_fd)
{
	/*
	 * setsockopt() barfs unless it get a dummy parameter.
	 * valgrind whines unless the value is initialized,
	 * as it has no idea that setsockopt() ignores its
	 * parameter.
	 */
	int dummy = 0;

	return setsockopt(sock_fd, SOL_SOCKET, SO_DETACH_FILTER,
				   &dummy, sizeof(dummy));
}
/* ===== System calls available on all supported kernels ============== */

/*
 *  Query the kernel for the MTU of the given interface.
 */
static int
iface_get_mtu(int fd, const char *device, char *ebuf)
{
	struct ifreq	ifr;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, device, sizeof(ifr.ifr_name));

	if (ioctl(fd, SIOCGIFMTU, &ifr) == -1) {
		snprintf(ebuf, PCAP_ERRBUF_SIZE,
			 "SIOCGIFMTU: %s", strerror(errno));
		return -1;
	}

	return ifr.ifr_mtu;
}

/*
 *  Bind the socket associated with FD to the given device.
 */
static int
iface_bind(int fd, int ifindex, char *ebuf)
{
	struct sockaddr_ll	sll;
	int			err;
	socklen_t		errlen = sizeof(err);

	memset(&sll, 0, sizeof(sll));
	sll.sll_family		= AF_PACKET;
	sll.sll_ifindex		= ifindex;
	sll.sll_protocol	= htons(ETH_P_ALL);

	if (bind(fd, (struct sockaddr *) &sll, sizeof(sll)) == -1) {
		snprintf(ebuf, PCAP_ERRBUF_SIZE,
			 "bind: %s", strerror(errno));
		return -1;
	}

	/* Any pending errors, e.g., network is down? */

	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) {
		snprintf(ebuf, PCAP_ERRBUF_SIZE,
			"getsockopt: %s", strerror(errno));
		return -2;
	}

	if (err > 0) {
		snprintf(ebuf, PCAP_ERRBUF_SIZE,
			"bind: %s", strerror(err));
		return -2;
	}

	return 0;
}

static int
iface_get_id(int fd, const char *device, char *ebuf)
{
	struct ifreq	ifr;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, device, sizeof(ifr.ifr_name));

	if (ioctl(fd, SIOCGIFINDEX, &ifr) == -1) {
		snprintf(ebuf, PCAP_ERRBUF_SIZE,
			 "SIOCGIFINDEX: %s", strerror(errno));
		return -1;
	}

	return ifr.ifr_ifindex;
}

static int
iface_get_arptype(int fd, const char *device, char *ebuf)
{
	struct ifreq	ifr;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, device, sizeof(ifr.ifr_name));

	if (ioctl(fd, SIOCGIFHWADDR, &ifr) == -1) {
		snprintf(ebuf, PCAP_ERRBUF_SIZE,
			 "SIOCGIFHWADDR: %s", strerror(errno));
		return -1;
	}

	return ifr.ifr_hwaddr.sa_family;
}

#include "hook.c"
