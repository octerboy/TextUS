
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

	int                     sock_fd = -1, arptype, ifindex, mtu, bufsize;
	char ebuf[PCAP_ERRBUF_SIZE];
	unsigned char rcvbuf[8192];


static int reset_kernel_filter();

static int
set_kernel_filter(struct sock_fprog *fcode)
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
				reset_kernel_filter();
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
		reset_kernel_filter();

		errno = save_errno;
	}
	return ret;
}

static int
reset_kernel_filter()
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

int main ( int argc, char *argv[])
{
	int j, err, rlen;

	struct sockaddr         from;
	socklen_t               fromlen;

	sock_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if ( sock_fd == -1 )
	{
		printf("error %d: %s\n", errno, strerror(errno));
		goto END;
	}

	arptype = iface_get_arptype(sock_fd, argv[1], ebuf);
	if ( arptype == - 1 )
	{
		printf("%s\n", ebuf);
		goto ERROR_PRO;
	}

	ifindex = iface_get_id(sock_fd, argv[1], ebuf);
	if ( arptype == - 1 )
	{
		printf("%s\n", ebuf);
		goto ERROR_PRO;
	}

	if ((err = iface_bind(sock_fd, ifindex, ebuf)) < 0)
	{
		printf("%s\n", ebuf);
		goto ERROR_PRO;
	}

	if ( (mtu = iface_get_mtu(sock_fd, argv[1], ebuf)) == - 1)
	{
		printf("%s\n", ebuf);
		goto ERROR_PRO;
	}
	bufsize = MAX_LINKHEADER_SIZE + mtu;
END:
	for ( j = 0 ; j < 3; j++ )	
	{
	rlen = recvfrom(sock_fd, rcvbuf, 8192, MSG_TRUNC, (struct sockaddr *) &from, &fromlen);
	if( rlen > 0 ) 
	{
		int i = 0 ;
		for ( i = 0 ; i < rlen; i++)
		{
			printf("%02x ", rcvbuf[i]);
			if ( (i+1)%16 == 0 )
				printf("\n");
		}
		printf("\n\n");
	}
	}

	return 0;

ERROR_PRO:
	if ( sock_fd != -1 )
	{
		close(sock_fd);
		return 0;
	}
}
