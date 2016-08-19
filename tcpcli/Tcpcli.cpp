/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
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
 $Header: /textus/tcpcli/Tcpcli.cpp 23    14-04-14 7:24 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: Tcpcli.cpp $"
#define TEXTUS_MODTIME  "$Date: 14-04-14 7:24 $"
#define TEXTUS_BUILDNO  "$Revision: 23 $"
#include "version_1.c"
/* $NoKeywords: $ */

#include "Tcpcli.h"
#include "textus_string.h"
#include <assert.h>
#include <stdlib.h>
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
}

Tcpcli::~Tcpcli() { }

/* ��ʼ����, ����ɹ�, ��connfd>=0 */
bool Tcpcli::annecto( bool block)
{
	struct sockaddr_in servaddr;

	int n;
#if defined (_WIN32)
	WSADATA wsaData;
#endif
	if ( connfd >= 0)	/* �Ѿ����ӻ�����������, ���ٷ������� */
		return true;

#if defined (_WIN32)
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR)
	{
		ERROR_PRO("Error at WSAStartup()");
		return false;
	}
#endif
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
	if ((connfd = socket (AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET )
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

/* һ��TCP�������, nonblock��ʽ���� */
bool Tcpcli::annecto_done()
{
	int error = 0;
	err_lev = -1;
#if defined(__linux__) || defined(_AIX) || defined(__APPLE__)
	socklen_t len = sizeof(error);
#else
	int len = sizeof(error); //WIN32, SUN, SCO������
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
	WSACleanup();
#endif
	return ;
}

/* ���շ�������ʱ, ����ر�����׽��� */
bool Tcpcli::recito()
{	
	long len;
	err_lev = -1;	
	if ( rcv_buf)
		rcv_buf->grant(8192);	//��֤���㹻�ռ�
ReadAgain:
	if ( rcv_buf )
		len = recv(connfd, (char*)rcv_buf->point, 8192, MSG_NOSIGNAL);
	else {
		char an_buf[8192];
		memset(an_buf, 0, sizeof(an_buf));
		len = recv(connfd, an_buf, 8192, MSG_NOSIGNAL);
	}
	
	if( len == 0 )
	{	//�Է��ر��׽���
		if ( errMsg ) TEXTUS_STRCPY(errMsg, "recv 0, disconnected");
		err_lev = 6;
		return false;
	} else if ( len ==SOCKET_ERROR )
	{ 
#if defined (_WIN32 )
		int error = GetLastError();
#else
		int error = errno;
#endif 
		if (error == EINTR)
		{	 //���źŶ���,����
			goto ReadAgain;
		} else if ( error == EAGAIN || error ==  EWOULDBLOCK )
		{	//���ڽ�����, ��ȥ�ٵ�.
			ERROR_PRO("recv encounter EWOULDBLOCK");
			err_lev = 5;
			return true;
		} else {	/* ��ȷ�д��� */
			ERROR_PRO("recv");
			err_lev = 5;
			return false;
		}
	} 

	if ( rcv_buf)
		rcv_buf->commit(len);	/* ָ������� */
	return true;
}

/* �����д���ʱ, ����-1, ����ر�����׽��� */
int Tcpcli::transmitto()
{
	long len;
	if ( connfd < 0 || isConnecting || !snd_buf ) 
		return 0;
	long snd_len = snd_buf->point - snd_buf->base;	//���ͳ���
	err_lev = -1;	

SendAgain:
	len = send(connfd, (char *)snd_buf->base, snd_len, MSG_NOSIGNAL); /* (char *) for WIN32 */
	if( len < 0)
	{ 
#if defined (_WIN32 )
		int error = GetLastError();
#else
		int error = errno;
#endif 
		if (error == EINTR)	
		{	//���źŶ���,����
			goto SendAgain;
		} else if ( error ==EWOULDBLOCK || error == EAGAIN)
		{	//��ȥ����, ��select, Ҫ��wrSet
			ERROR_PRO("send encounter EWOULDBLOCK");
			err_lev = 5;
			if ( wr_blocked ) 	//���һ������
				return 3;
			else
			{	//�շ���������
				wr_blocked = true;
				return 1;
			}
		} else {
			ERROR_PRO("send");
			err_lev = 3;
			return -1;
		}
	}

	snd_buf->commit(-len);	//�ύ������������
	if (snd_len > len )
	{	
		if ( wr_blocked ) 	//���һ�λ�������
			return 3;
		else
		{	//�շ���������
			wr_blocked = true;
			return 1; //��ȥ����, ��select, Ҫ��wrSet
		}
	} else {	//������������
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

