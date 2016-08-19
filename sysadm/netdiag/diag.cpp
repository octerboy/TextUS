/**
 标题:系统诊断
 标识:XmlHttp-diag.cpp
 版本:B003
	B001:created by xiezr 2002/07/17
	B002:modified on 2003/11/21,输入参数加const声明
	B003:modified on 2004/03/03,去掉一些定义
*/

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <resolv.h>

#ifdef __linux__
#define SAFE_TO_DROP_ROOT
#endif

#define	DEFDATALEN	(64 - 8)	/* default data length */
#define	MAXIPLEN	60
#define	MAXICMPLEN	76
#define	MAXPACKET	(2048 - 60 - 8)/* max packet size */
#define	MAXWAIT		10		/* max seconds to wait for response */
#define	NROUTES		9		/* number of record route slots */

#define	A(bit)		rcvd_tbl[(bit)>>3]	/* identify byte in array */
#define	B(bit)		(1 << ((bit) & 0x07))	/* identify bit in byte */
#define	SET(bit)	(A(bit) |= B(bit))
#define	CLR(bit)	(A(bit) &= (~B(bit)))
#define	TST(bit)	(A(bit) & B(bit))

/* various options */
int options;
#define	F_FLOOD		0x001
#define	F_INTERVAL	0x002
#define	F_NUMERIC	0x004
#define	F_PINGFILLED	0x008
#define	F_QUIET		0x010
#define	F_RROUTE	0x020
#define	F_SO_DEBUG	0x040
#define	F_SO_DONTROUTE	0x080
#define	F_VERBOSE	0x100

/* multicast options */
int moptions;
#define MULTICAST_NOLOOP	0x001
#define MULTICAST_TTL		0x002
#define MULTICAST_IF		0x004

/*
 * MAX_DUP_CHK is the number of bits in received table, i.e. the maximum
 * number of received sequence numbers we can keep track of.  Change 128
 * to 8192 for complete accuracy...
 */
#define	MAX_DUP_CHK	(8 * 128)
int mx_dup_ck = MAX_DUP_CHK;
char rcvd_tbl[MAX_DUP_CHK / 8];

struct sockaddr whereto;	/* who to ping */
int datalen = DEFDATALEN;
int s;				/* socket file descriptor */
u_char outpack[MAXPACKET];
char BSPACE = '\b';		/* characters written for flood */
char DOT = '.';
static const char *hostname;
static int ident;		/* process id to identify our packets */

/* counters */
static long npackets;		/* max packets to transmit */
static long nreceived;		/* # of packets we got back */
static long nrepeats;		/* number of duplicates */
static long ntransmitted;	/* sequence # for outbound packets = #sent */
static int interval = 1;	/* interval between packets */

/* timing */
static int timing;		/* flag to do timing */
static long tmin = LONG_MAX;	/* minimum round trip time */
static long tmax = 0;		/* maximum round trip time */
static u_long tsum;		/* sum of all times, for doing average */

/* protos */
static void pinger(void);
static int pr_pack(char *buf, int cc, struct sockaddr_in *from);
static int in_cksum(u_short *addr, int len);

/* Ping function:
 *
 *  0: alive
 *  < 0: Error( internel )
 *  > 0: network error
 */

#include "diag.h"
int Diag::ping(const char *ip_addr)
{
	struct timeval timeout;
	struct hostent *hp;
	struct sockaddr_in *to;
	struct protoent *proto;
	struct in_addr ifaddr;
	int ret,count,i;
	int ch, fdmask, hold, packlen, preload;
	u_char *datap, *packet;
	const char *target ;
	char hnamebuf[MAXHOSTNAMELEN];
	u_char ttl, loop;
	int am_i_root;

	static char *null = NULL;

	/*__environ = &null;*/
	am_i_root = (getuid()==0);

	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) 
	{
		return EPERM*(-1);
	}

	preload = 0;

	datap = &outpack[8 + sizeof(struct timeval)];
	
	target = ip_addr;

	memset(&whereto, 0, sizeof(struct sockaddr));
	to = (struct sockaddr_in *)&whereto;
	to->sin_family = AF_INET;

	if (inet_aton(target, &to->sin_addr)) {
		hostname = target;
	}
	else 
	{
		hp = gethostbyname(target);
		if (!hp) {
			return -1001;
		}
		to->sin_family = hp->h_addrtype;
		if (hp->h_length > (int)sizeof(to->sin_addr)) {
			hp->h_length = sizeof(to->sin_addr);
		}
		memcpy(&to->sin_addr, hp->h_addr, hp->h_length);
		(void)strncpy(hnamebuf, hp->h_name, sizeof(hnamebuf) - 1);
		hostname = hnamebuf;
	}


	if (datalen >= (int)sizeof(struct timeval)) /* can we time transfer */
		timing = 1;
	packlen = datalen + MAXIPLEN + MAXICMPLEN;
	packet = (u_char*)malloc((u_int)packlen);
	if (!packet) {
		return -1003;
	}

	ident = getpid() & 0xFFFF;
	hold = 1;

	/* this is necessary for broadcast pings to work */
	setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char *)&hold, sizeof(hold));

	hold = 48 * 1024;
	(void)setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&hold,
	    sizeof(hold));

	pinger();
	pinger();

	ret = 1;

	for (count = 0,i = 0; i < 10 && count < 2 ; i++) 
	{
		struct sockaddr_in from;
		register int cc;
		int fromlen;
/*
		if (options & F_FLOOD) {
			pinger();
			timeout.tv_sec = 0;
			timeout.tv_usec = 10000;
			fdmask = 1 << s;
			if (select(s + 1, (fd_set *)&fdmask, (fd_set *)NULL,
			    (fd_set *)NULL, &timeout) < 1)
				continue;
		}
*/
//	        FD_ZERO(&s);
//               FD_SET(0, &s);

		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		fdmask = 1 << s;
		if (select(s + 1, (fd_set *)&fdmask, (fd_set *)NULL,
		    (fd_set *)NULL, &timeout) < 1)
				continue;

		fromlen = sizeof(from);
		if ((cc = recvfrom(s, (char *)packet, packlen, 0,
		    (struct sockaddr *)&from, (socklen_t *)&fromlen)) < 0) {
			if (errno == EINTR)
				continue;
			perror("ping: recvfrom");
			continue;
		}
		count++;
		ret = pr_pack((char *)packet, cc, &from);
	}
	close(s);
	return ret;
}


#if !defined(__GLIBC__) || (__GLIBC__ < 2)
#define icmp_type type
#define icmp_code code
#define icmp_cksum checksum
#define icmp_id un.echo.id
#define icmp_seq un.echo.sequence
#define icmp_gwaddr un.gateway
#endif /* __GLIBC__ */

#define ip_hl ihl
#define ip_v version
#define ip_tos tos
#define ip_len tot_len
#define ip_id id
#define ip_off frag_off
#define ip_ttl ttl
#define ip_p protocol
#define ip_sum check
#define ip_src saddr
#define ip_dst daddr

static void pinger(void)
{
	register struct icmp *icp;
	register int cc;
	int i;

	icp = (struct icmp *)outpack;
	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq = ntransmitted++;
	icp->icmp_id = ident;			/* ID */

	CLR(icp->icmp_seq % mx_dup_ck);

	if (timing)
		(void)gettimeofday((struct timeval *)&outpack[8],
		    (struct timezone *)NULL);

	cc = datalen + 8;			/* skips ICMP portion */

	/* compute ICMP checksum here */
	icp->icmp_cksum = in_cksum((u_short *)icp, cc);

	i = sendto(s, (char *)outpack, cc, 0, &whereto,
	    sizeof(struct sockaddr));

	if (i < 0 || i != cc)  {
		if (i < 0)
			perror("ping: sendto");
		(void)printf("ping: wrote %s %d chars, ret=%d\n",
		    hostname, cc, i);
	}
}

/*
 * pr_pack --
 *	Print out the packet, if it came from us.  This logic is necessary
 * because ALL readers of the ICMP socket get a copy of ALL ICMP packets
 * which arrive ('tis only fair).  This permits multiple copies of this
 * program to be run without having intermingled output (or statistics!).
 */
int pr_pack(char *buf, int cc, struct sockaddr_in *from)
{
	register struct icmp *icp;
	register int i;
	register u_char *cp,*dp;
/*#if 0*/
	register u_long l;
	register int j;
	static int old_rrlen;
	static char old_rr[MAX_IPOPTLEN];
/*#endif*/
	struct iphdr *ip;
	struct timeval tv, *tp;
	long triptime = 0;
	int hlen, dupflag;

	(void)gettimeofday(&tv, (struct timezone *)NULL);

	/* Check the IP header */
	ip = (struct iphdr *)buf;
	hlen = ip->ip_hl << 2;
	if (cc < datalen + ICMP_MINLEN) {
		return 2001;
	}

	/* Now the ICMP part */
	cc -= hlen;
	icp = (struct icmp *)(buf + hlen);
	if (icp->icmp_type == ICMP_ECHOREPLY) 
	{
		if (icp->icmp_id != ident)
			return 2002;			/* 'Twas not our ECHO */
		return 0;
	} 
	else 
	{
		return 2003;
	}
}

/*
 * in_cksum --
 *	Checksum routine for Internet Protocol family headers (C Version)
 */
static int
in_cksum(u_short *addr, int len)
{
	register int nleft = len;
	register u_short *w = addr;
	register int sum = 0;
	u_short answer = 0;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(u_char *)(&answer) = *(u_char *)w ;
		sum += answer;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}

char *Diag::dns(const char *dns_name)
{
	struct hostent *he_ptr;
	static char tmp[INET_ADDRSTRLEN];

	he_ptr = gethostbyname(dns_name);

	if(!he_ptr)
   		return NULL;

	if(inet_ntop(he_ptr ->h_addrtype, he_ptr ->h_addr_list[0],tmp, 
	   sizeof(tmp)) == NULL)
		return NULL;
	return &tmp[0];
}

