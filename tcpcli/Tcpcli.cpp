/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: TCP Client
 Build: created by octerboy, 2005/08/1
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
#include "version_1.c"
/* $NoKeywords: $ */

#include "Tcpcli.h"
#include "textus_string.h"
#include <assert.h>
#include <stdlib.h>
#if defined(_WIN32)
#define CLOSE closesocket
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EAGAIN WSAEWOULDBLOCK
#define EINPROGRESS WSAEWOULDBLOCK
#define EINTR WSAEINTR
#define GETSOCK_OPT_TYPE char*
#else
#define CLOSE close
#define INVALID_SOCKET -1
#define GETSOCK_OPT_TYPE void*
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
		TEXTUS_SNPRINTF(errMsg, errstr_len, "%s (to %s:%d) errno %d, %s.", X, server_ip, server_port, errno, strerror(errno));
#endif

#define BZERO(X) memset(X, 0 ,sizeof(X))
Tcpcli::Tcpcli()
{
	BZERO(server_ip);
	server_port = 0;

	connfd = -1;

	wr_blocked = false; 	/* �տ�ʼ, ���һ��д��Ȼ������ */
	isConnecting = false;	/* ���������ӽ����� */

	rcv_buf = 0;
	snd_buf = 0;
	errstr_len = 0;
	errMsg = 0;
#if defined (_WIN32)
	memset(&rcv_ovp, 0, sizeof(OVERLAPPED));
	memset(&snd_ovp, 0, sizeof(OVERLAPPED));
#endif
}

Tcpcli::~Tcpcli() { }

#if defined (_WIN32)
bool Tcpcli::sock_start()
{
	WSADATA wsaData;
#if defined( _MSC_VER ) && (_MSC_VER < 1400 )
	#define WSAID_CONNECTEX \
    {0x25a207b9,0xddf3,0x4660,{0x8e,0xe9,0x76,0xe5,0x8c,0x74,0x06,0x3e}}
#endif
	GUID GuidConnectEx = WSAID_CONNECTEX;
	DWORD dwBytes = 0;
	int fd;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR)
	{
		ERROR_PRO("Error at WSAStartup()");
		return false;
	}
	
	lpfnConnectEx = NULL;

	if ((fd = WSASocket(AF_INET,SOCK_STREAM, IPPROTO_TCP, NULL,0,WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET )
	{
		ERROR_PRO("WSASocket")
		return false;
	}
	iResult = WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,  &GuidConnectEx, sizeof (GuidConnectEx),  
						&lpfnConnectEx, sizeof (lpfnConnectEx), &dwBytes, NULL, NULL);
	if (iResult == SOCKET_ERROR) {
		ERROR_PRO("WSAIoctl");
		return false;
	}

	return true;
}
#endif

bool Tcpcli::clio( bool block)
{
	if ( connfd >= 0)	/* �Ѿ����ӻ�����������, ���ٷ������� */
		return true;

	err_lev = -1;

	/* ��ʼ���׽ӿڵĵ�ַ�ṹ */
	memset(&servaddr, 0, sizeof (servaddr));
	servaddr.sin_family = AF_INET;

	/* �趨�˿� */
	servaddr.sin_port = htons(server_port);

	/* �趨��ַ */
	if ( server_ip != (char*) 0 && server_ip[0] != '\0') 
	{
		struct in_addr me;
#if defined (_WIN32)
#define	HAS_ADDR (me.s_addr = inet_addr(server_ip)) == INADDR_NONE
#else
#define HAS_ADDR inet_aton(server_ip, &me) == 0
#endif
		if ( HAS_ADDR )
		{
			struct hostent* he;
			he = gethostbyname(server_ip);
			if ( he == (struct hostent*) 0 )
			{	/* ���� */
				ERROR_PRO("Invalid address")
				err_lev = 3;
				return false;
		   	} else
			{
		   	 	(void) memcpy(&servaddr.sin_addr, he->h_addr, he->h_length );
		   	 }
		   } else
		   {
		   	servaddr.sin_addr.s_addr = me. s_addr;
		   }
	} else
	{
		if ( errMsg ) TEXTUS_STRCPY(errMsg, "annecto server ip is null!");
		err_lev = 4;
		return false;
	}

/* �����׽��� */
#if defined(_WIN32)
	if ((connfd = WSASocket(AF_INET,SOCK_STREAM, IPPROTO_TCP, NULL,0,WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET )
#else
	if ((connfd = socket (AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET )
#endif 
	{
		ERROR_PRO("create socket")
		err_lev = 3;
		connfd = -1;
		return false;
	}

	if ( !block )
	{	//��ΪNONBLOCK��ʽ
		int flags;
		int ret;
#if defined(_WIN32)
		flags = 1;
		ret = ioctlsocket(connfd, (long) FIONBIO, (u_long* ) &flags);
#else
		flags=fcntl(connfd, F_GETFL, 0);
		if (flags != -1 )
		{
			ret = fcntl(connfd, F_SETFL, O_NONBLOCK|flags);
		} else 
			ret = -1;
		if ( ret !=0 )
		{
			ERROR_PRO("fcntl(connfd, F_SETFL) ")
			err_lev = 3;
			this->end(false);
			return false;
		}
#endif
	}
	return true;
}

/* ��ʼ����, ����ɹ�, ��connfd>=0 */
bool Tcpcli::annecto()
{
	int n;
	n = connect (connfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if ( n == SOCKET_ERROR) 
	{	//ͨ��, ���ڷ�������ʽ, ����ķ�����EINPROGRESS
#if defined (_WIN32 )
		int error = GetLastError();
#else
		int error = errno;
#endif
		if (error != EINPROGRESS) 
		{
			ERROR_PRO("connect() ");
			err_lev = 5;
			this->end(false);
			return false;
		}
	}

	if ( n==0 ) 
	{
		if ( !annecto_done() )
		{
			this->end(false);
			return false;
		}
	} else
		isConnecting = true;	//���������ڽ���

	return true;
}

#if defined (_WIN32)
bool Tcpcli::annecto_ex()
{
	DWORD dwBytes =0 ;
	BOOL bRetVal = FALSE;

	// Empty our overlapped structure and accept connections.
	memset(&rcv_ovp, 0, sizeof(OVERLAPPED));

	bRetVal = lpfnConnectEx(connfd,  (struct sockaddr *)&servaddr, sizeof(servaddr), 
				(PVOID)snd_buf->point, snd_buf->point - snd_buf->base, &dwBytes, &rcv_ovp);
	if (bRetVal == FALSE) { 
		if (  ERROR_IO_PENDING  == WSAGetLastError() ) {
			return true;
		} else {
			ERROR_PRO("lpfnConnectEx")
			this->end(false);
			return false;
		}
	}
	if ( dwBytes > 0 )
		snd_buf->commit(-(long)dwBytes);	//�ύ������������
	return true;
}
#endif

/* һ��TCP�������, nonblock��ʽ���� */
bool Tcpcli::annecto_done()
{
	int error = 0;
	err_lev = -1;
#if defined(__linux__) || defined(_AIX) || defined(__APPLE__) || defined(__SUNPRO_CC)
	socklen_t len = sizeof(error);
#else
	int len = sizeof(error); //WIN32, SCO������
#endif
	bool ret = true;
	if (getsockopt(connfd, SOL_SOCKET, SO_ERROR, (GETSOCK_OPT_TYPE)&error, &len) < 0)
	{
		ERROR_PRO("annecto_done(getsockopt)")
		err_lev = 3;
		ret = false;
	} else if ( error)
	{
#if defined(_WIN32)
		SetLastError(error);
#else
		errno = error;
#endif
		ERROR_PRO("annecto_done")
		err_lev = 3;
		ret = false;
	}

#ifndef NDEBUG
	if ( ret )
	{
		if ( errMsg ) TEXTUS_SNPRINTF(errMsg, 512, "annecto_done success for %s:%d, fd:%d", server_ip, server_port, connfd);
	}
#endif

	isConnecting = false;
	return ret;
}

void Tcpcli::end(bool down)
{
	if ( connfd >= 0 ) 
	{
#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif
		if ( down && !isConnecting ) 	//�����Ѿ��������ӵ�, ���Թر���֪ͨ�Է�.
			shutdown(connfd, SHUT_RDWR);
		CLOSE(connfd);	
		connfd=-1;
	}

#if defined (_WIN32)
	//WSACleanup();
#endif
	return ;
}
void Tcpcli::setPort(const char *port_str)
{
	int port;
	
	if( !port_str) return;
	port = atoi(port_str);
	if ( port > 0 && port < 65535 )
		server_port = port;
	else {
		struct servent *serv_ptr;
		if ((serv_ptr = getservbyname(port_str, "tcp")))
	  		server_port = ntohs(serv_ptr->s_port);
	}
}

void Tcpcli::herit(Tcpcli *child)
{
	assert(child);
	memcpy(child->server_ip, server_ip,sizeof(server_ip));
	child->server_port = server_port;
	return ;
}

#if defined(_WIN32)
int Tcpcli::recito_ex()
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

int Tcpcli::transmitto_ex()
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
int Tcpcli::recito()
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
int Tcpcli::transmitto()
{
	long len, snd_len ;
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


