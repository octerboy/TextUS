/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: TCP Service
 Build: created by octerboy, 2005/06/10
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
#include "version_1.c"
/* $NoKeywords: $ */

#include "Tcpsrv.h"
#include "textus_string.h"
#include <stdlib.h>
#include <assert.h>
#if !defined (_WIN32)
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
 #if !defined(__linux__) && !defined(_AIX)
 #include <strings.h>
 #endif
#else
#include <windows.h>
#endif 
#include <stdio.h>
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#if defined(_WIN32)
#define CLOSE closesocket
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EAGAIN WSAEINPROGRESS
#define EINTR WSAEINTR
#define SETSOCK_OPT_TYPE const char*
#else
#define CLOSE close
#define INVALID_SOCKET -1
#define SETSOCK_OPT_TYPE const void*
#define SOCKET_ERROR -1
#endif

#if defined (_WIN32 )
#define ERROR_PRO(X) { \
	char *s; \
	char error_string[1024]; \
	DWORD dw = GetLastError(); \
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw, \
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) error_string, 1024, NULL );\
	s= strstr(error_string, "\r\n") ; \
	if (s )  *s = '\0';  \
	if ( errMsg ) \
		TEXTUS_SNPRINTF(errMsg, errstr_len, "%s errno %d, %s", X,dw, error_string);\
	}
#else
#define ERROR_PRO(X)  if ( errMsg ) \
		TEXTUS_SNPRINTF(errMsg, errstr_len, "%s errno %d, %s.", X, errno, strerror(errno));
#endif

Tcpsrv::Tcpsrv()
{
	memset(srvip, 0, sizeof(srvip));
	srvport = 0;

	connfd = -1;
	listenfd = -1;

	wr_blocked = false; 	//�տ�ʼ, ���һ��д��Ȼ������

	/* �������н�������ʵ�� */
	rcv_buf = new TBuffer(RCV_FRAME_SIZE);
	snd_buf = new TBuffer(RCV_FRAME_SIZE);
	errMsg = (char*) 0;
	errstr_len = 0;
#if defined (_WIN32)
	memset(&rcv_ovp, 0, sizeof(OVERLAPPED));
	memset(&snd_ovp, 0, sizeof(OVERLAPPED));
	wsa_rcv.len = RCV_FRAME_SIZE;
	wsa_snd.buf = (char*)snd_buf->base;
#endif
}

Tcpsrv::~Tcpsrv()
{
	delete rcv_buf;
	delete snd_buf;
}

#if defined (_WIN32)
bool Tcpsrv::sock_start()
{
	WSADATA wsaData;
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes;
	BOOL bRetVal = FALSE;
	int fd;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR)
	{
		ERROR_PRO("Error at WSAStartup()");
		return false;
	}
	lpfnAcceptEx = NULL;

	if ((fd = WSASocket(AF_INET,SOCK_STREAM, IPPROTO_TCP, NULL,0,WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET )
	{
		ERROR_PRO("create socket")
		return false;
	}
	iResult = WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,  &GuidAcceptEx, sizeof (GuidAcceptEx),  
						&lpfnAcceptEx, sizeof (lpfnAcceptEx), &dwBytes, NULL, NULL);
	if (iResult == SOCKET_ERROR) {
		ERROR_PRO("WSAIoctl");
		return false;
	}

	return true;
}
#endif

/* ��ʼ����, ���������˿�, ����ɹ�, ��listenfd>0 */
bool Tcpsrv::servio( bool block)
{
	struct sockaddr_in servaddr;
	const int on = 1;

	/* ��ʼ���׽ӿڵĵ�ַ�ṹ */
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;

	/* �趨�˿� */
	servaddr.sin_port = htons(srvport);

	/* �趨��ַ */
	if (srvip[0] == '\0')
	{
		servaddr.sin_addr.s_addr  = htonl( INADDR_ANY );
	} else
	{
		struct in_addr me;
#if defined (_WIN32)
#define	HAS_ADDR (me.s_addr = inet_addr(srvip)) == INADDR_NONE
#else
#define HAS_ADDR inet_aton(srvip, &me) == 0
#endif
		if ( HAS_ADDR )
		{
			struct hostent* he;
			he = gethostbyname(srvip);
			if ( he == (struct hostent*) 0 )
			{
				ERROR_PRO("Invalid address")
				return false;
		   	} else
		   	{
		   	 	(void) memcpy(&servaddr.sin_addr, he->h_addr, he->h_length );
		   	 }
		   } else
		   {
		   	servaddr.sin_addr.s_addr = me. s_addr;
		   }
	} 	
/* �����׽��� */
#if defined(_WIN32)
	if ((listenfd = WSASocket(AF_INET,SOCK_STREAM, IPPROTO_TCP, NULL,0,WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET )
#else
	if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET )
#endif 
	{
		ERROR_PRO("create socket")
		return false;
	}

	/* ���÷�����׽������ԣ� �Լ��ͻ����Ƿ���� */
	setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, (SETSOCK_OPT_TYPE)&on, sizeof(int));
	/* ��ַ���� */
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (SETSOCK_OPT_TYPE)&on, sizeof(int));

	if ((bind (listenfd, (struct sockaddr *)&servaddr, sizeof (servaddr)) ) != 0 )
	{
		ERROR_PRO("bind socket")
		CLOSE(listenfd);
		listenfd = -1;
		return false;
	}

	/*�����ɵ�����������ת��Ϊ����������*/
	if( listen (listenfd, 100) != 0 )
	{
		ERROR_PRO("listen socket")
		CLOSE(listenfd);
		listenfd = -1;

		return false;
	}

	if ( !block )
	{
		int flags;
#if defined(_WIN32)
		flags = 1;
		ioctlsocket(listenfd, (long) FIONBIO, (u_long* ) &flags);
#else
		flags=fcntl(listenfd,F_GETFL,0);	//��ΪNONBLOCK��ʽ
		fcntl(listenfd,F_SETFL,O_NONBLOCK|flags);
#endif
	}

	return true;
}

#if defined(_WIN32)
int Tcpsrv::accept_ex()
{
	DWORD dwBytes;
	BOOL bRetVal = FALSE;

	// Create an accepting socket
	connfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connfd == INVALID_SOCKET) {
		ERROR_PRO("Create accept socket")
		goto MY_ERROR;
	}

	// Empty our overlapped structure and accept connections.
	memset(&rcv_ovp, 0, sizeof(OVERLAPPED));
	bRetVal = lpfnAcceptEx(listenfd, connfd, accept_buf, 0, /* 0 means AcceptEx completes  without waiting for any data*/
					sizeof (sockaddr_in) + 16, sizeof (sockaddr_in) + 16, &dwBytes, &rcv_ovp);
	if (bRetVal == FALSE) { 
		if (  ERROR_IO_PENDING  == WSAGetLastError() ) {
			return 0;
		} else {
			ERROR_PRO("lpfnAcceptEx")
			//closesocket(listenfd);
			release();
			return -1;
		}
	}
	return 1;

MY_ERROR:
	endListen();
	//WSACleanup();
	return -1;
}

bool Tcpsrv::post_accept_ex()
{
	int iResult = 0;
	iResult =  setsockopt( connfd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&listenfd, sizeof(listenfd) );	
	if ( iResult == 0 ) 
		return true;
	
	ERROR_PRO("setsockopt SO_UPDATE_ACCEPT_CONTEXT")
	return false;
}
#endif

/* ����һ��TCP���� */
bool Tcpsrv::accipio( bool block )
{
#if defined(__linux__) || defined(_AIX) || defined(__APPLE__) || defined(__SUNPRO_CC)
#define	CLI_TYPE socklen_t 
#else
#define	CLI_TYPE int 
#endif

AcceptAgain:
	connfd= accept(listenfd, (struct sockaddr *)0, (CLI_TYPE *)0);
	if ( connfd < 0 )
	{
#if defined (_WIN32 )
		DWORD error;	
		error = GetLastError();
#else
		int error = errno;
#endif 
		if (error == EINTR)
		{	//���źŶ���,����
			goto AcceptAgain;	
		} else if ( error == EAGAIN || error ==  EWOULDBLOCK )
		{	//���ڽ�����, ��ȥ�ٵ�.
			ERROR_PRO("accept encounter EAGAIN")
			return true;
		} else
		{	//��ȷ�д���, �Է������ѹر��׽��ֵ�.
			ERROR_PRO("accept socket")
			return false;
		}
	}

	if ( !block )
	{
		int flags;
#if defined(_WIN32)
		flags = 1;
		if ( ioctlsocket(connfd, (long) FIONBIO, (u_long *)&flags) == SOCKET_ERROR )
		{
			ERROR_PRO ("ioctlsocket FIONBIO");
			return false;
		}
#else
		flags=fcntl(connfd,F_GETFL,0);	//connfd��ΪNONBLOCK��ʽ
		fcntl(connfd,F_SETFL,O_NONBLOCK|flags);
#endif
	}
	
	return true;
}

void Tcpsrv::end()
{
	if ( connfd > 0 ) 
	{
#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif
		shutdown(connfd, SHUT_RDWR);
		CLOSE(connfd);	
		connfd=-1;
	}
	return ;
}

void Tcpsrv::release()
{
	if ( connfd > 0 ) 
	{
		CLOSE (connfd);	
		connfd = -1;
	}
	return ;
}

void Tcpsrv::endListen()
{
	if ( listenfd > 0)
	{	
		CLOSE(listenfd);	
		listenfd =-1;
	}
	return ;
}

#if defined(_WIN32)
int Tcpsrv::recito_ex()
{	
	int rc;

	rcv_buf->grant(RCV_FRAME_SIZE);	//��֤���㹻�ռ�
	wsa_rcv.buf = (char *)rcv_buf->point;
	flag = 0;
	rb = 0;
	memset(&rcv_ovp, 0, sizeof(OVERLAPPED));
	rc = WSARecv(connfd, &wsa_rcv, 1, &rb, &flag, &rcv_ovp, NULL);
	if ( rc == 0 )
	{
		printf("rc recv %d\n", rb);
		for ( int i = 0; i < rb; i++ ) printf("%02x ", rcv_buf->base[i]); printf("\n");
		if ( rb == 0 ) {
			if ( errMsg ) 
				TEXTUS_SNPRINTF(errMsg, errstr_len, "recv 0, disconnected");
			return -1;
		}
		rcv_buf->commit(rb);	/* ָ������� */
		return rb;
	} else {
		if ( WSA_IO_PENDING == WSAGetLastError() ) {
			return 0;
		} else {
			ERROR_PRO ("WSARecv");
			return -2;
		}
	}
}

int Tcpsrv::transmitto_ex()
{
	int rc;
SndAgain:
	wsa_snd.len = snd_buf->point - snd_buf->base;   //���ͳ���
	wsa_snd.buf = (char *)snd_buf->base;
	memset(&snd_ovp, 0, sizeof(OVERLAPPED));
	rc = WSASend(connfd, &wsa_snd, 1, &rb, 0, &snd_ovp, NULL);

	if ( rc == 0 )
	{
		snd_buf->commit(-(long)rb);
		if (wsa_snd.len > rb )
		{	
			goto SndAgain;
		} else 
			return 0;
	} else {
		if ( WSA_IO_PENDING == WSAGetLastError() ) {
			snd_buf->commit(-(long)wsa_snd.len);	//�Ѿ�����ϵͳ
			return 1; //��ȥ����, 
		} else {
			ERROR_PRO ("WSASend");
			return -1;
		}
	}
}
#endif

/* ���շ�������ʱ, ����ر�����׽��� */
int Tcpsrv::recito()
{	
	long len;

	rcv_buf->grant(RCV_FRAME_SIZE);	//��֤���㹻�ռ�
ReadAgain:
	if( (len = recv(connfd, (char *)rcv_buf->point, RCV_FRAME_SIZE, MSG_NOSIGNAL)) == 0) /* (char*) for WIN32 */
	{	//�Է��ر��׽���
		if ( errMsg ) 
			TEXTUS_SNPRINTF(errMsg, errstr_len, "recv 0, disconnected");
		return -1;
	} else if ( len == SOCKET_ERROR )
	{ 
#if defined (_WIN32 )
		DWORD error;	
		error = GetLastError();
#else
		int error = errno;
#endif 
		if (error == EINTR)
		{	 //���źŶ���,����
			goto ReadAgain;
		} else if ( error == EAGAIN || error == EWOULDBLOCK )
		{	//���ڽ�����, ��ȥ�ٵ�.
			ERROR_PRO("recving encounter EAGAIN")
			return 0;
		} else	
		{	//��ȷ�д���
			ERROR_PRO("recv socket")
			return -2;
		}
	}
	rcv_buf->commit(len);	/* ָ������� */
	return len;
}

/* �����д���ʱ, ����-1, ����ر�����׽��� */
int Tcpsrv::transmitto()
{
	long len,snd_len;
	snd_len = snd_buf->point - snd_buf->base;	//���ͳ���

SendAgain:
	len = send(connfd, (char *)snd_buf->base, snd_len, MSG_NOSIGNAL); /* (char*) for WIN32 */
	if( len == SOCKET_ERROR )
	{ 
#if defined (_WIN32 )
		DWORD error;	
		error = GetLastError();
#else
		int error = errno;
#endif 
		if (error == EINTR)	
		{	//���źŶ���,����
			goto SendAgain;
		} else if (error ==EWOULDBLOCK || error == EAGAIN)
		{	//��ȥ����, ��select, Ҫ��wrSet
			ERROR_PRO("sending encounter EAGAIN")
			if ( wr_blocked ) 	//���һ������
			{
				return 3;
			} else {	//�շ���������
				wr_blocked = true;
				return 1;
			}
		} else {
			ERROR_PRO("send")
			return -1;
		}
	}
	snd_buf->commit(-len);	//�ύ������������
	if (snd_len > len )
	{	
		TEXTUS_SNPRINTF(errMsg, errstr_len, "sending not completed.");
		if ( wr_blocked ) 	//���һ�λ�������
		{
			return 3;
		} else {	//�շ���������
			wr_blocked = true;
			return 1; //��ȥ����, ��select, Ҫ��wrSet
		}
	} else 
	{	//������������
		if ( wr_blocked ) 	//���һ������
		{
			wr_blocked = false;
			return 2; //�������, ��������wrSet��
		} else
		{	//һֱû������
			return 0; //�������
		}
	}
}

unsigned short Tcpsrv::getSrvPort()
{
	struct sockaddr_in name1;
#if defined(__linux__) || defined(_AIX) || defined(__APPLE__) || defined(__SUNPRO_CC)
	socklen_t clilen; 
#else
	int clilen;
#endif
	unsigned short prt = 0;
	clilen= sizeof(name1);					
	if ( getsockname(listenfd, (struct sockaddr *)&name1, &clilen) < 0 )
	{
		ERROR_PRO("getsockname");
	} else {
		prt = ntohs(name1.sin_port);
	}
	return prt;
}

char * Tcpsrv::getSrvIP()
{
	char *str = 0;
	struct sockaddr_in name1;
#if defined(__linux__) || defined(_AIX) || defined(__APPLE__) || defined(__SUNPRO_CC)
	socklen_t clilen; 
#else
	int clilen;
#endif
	clilen= sizeof(name1);					
	if ( getsockname(listenfd, (struct sockaddr *)&name1, &clilen) < 0 )
	{
		ERROR_PRO("getsockname");
	} else {
		str = inet_ntoa(name1.sin_addr);
	}
	return str;
}

void Tcpsrv::setPort(const char *port_str)
{
        int port;

        if( !port_str) return;
	port = atoi(port_str);
	if ( port > 0 && port < 65535 )
		srvport = port;
	else {
		struct servent *serv_ptr;
		if ((serv_ptr = getservbyname(port_str, "tcp")))
	  		srvport = ntohs(serv_ptr->s_port);
	}
}

void Tcpsrv::herit(Tcpsrv *child)
{
	assert(child);
	memcpy (child->srvip, srvip, sizeof(srvip));
	child->srvport = srvport;
	child->connfd = connfd;
	return ;
}
