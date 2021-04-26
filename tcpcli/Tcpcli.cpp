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
#if defined(_MSC_VER) && (_MSC_VER < 1400 )
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EAGAIN WSAEWOULDBLOCK
#define EINPROGRESS WSAEWOULDBLOCK
#define EINTR WSAEINTR
#endif
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

	connfd = INVALID_SOCKET ;

	wr_blocked = false; 	/* 刚开始, 最近一次写当然不阻塞 */
	isConnecting = false;	/* 并非在连接进行中 */

	rcv_buf = 0;
	snd_buf = 0;
	errstr_len = 0;
	errMsg = 0;
	rcv_frame_size = RCV_FRAME_SIZE;
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
	SOCKET fd;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR)
	{
		ERROR_PRO("Error at WSAStartup()");
		return false;
	}
	
	lpfnConnectEx = NULL;

#if defined(_MSC_VER) && (_MSC_VER >= 1800 )
	if ((fd = WSASocketW(AF_INET,SOCK_STREAM, IPPROTO_TCP, NULL,0,WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET )
#else
	if ((fd = WSASocket(AF_INET,SOCK_STREAM, IPPROTO_TCP, NULL,0,WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET )
#endif
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

bool Tcpcli::annecto_ex()
{
	DWORD dwBytes =0 ;
	BOOL bRetVal = FALSE;

	// Empty our overlapped structure and accept connections.
	memset(&rcv_ovp, 0, sizeof(OVERLAPPED));

	bRetVal = lpfnConnectEx(connfd,  (struct sockaddr *)&servaddr, sizeof(servaddr), 
				(PVOID)snd_buf->base, static_cast<DWORD>(snd_buf->point - snd_buf->base), &dwBytes, &rcv_ovp);
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
		snd_buf->commit(-(TEXTUS_LONG)dwBytes);	//提交所读出的数据
	return true;
}
#endif

bool Tcpcli::clio( bool block)
{
	if ( connfd != INVALID_SOCKET )	/* 已经连接或是正在连接, 则不再发起连接 */
		return true;

	err_lev = -1;

	/* 初始化套接口的地址结构 */
	memset(&servaddr, 0, sizeof (servaddr));
	servaddr.sin_family = AF_INET;

	/* 设定端口 */
	servaddr.sin_port = htons(server_port);

	/* 设定地址 */
	if ( server_ip != (char*) 0 && server_ip[0] != '\0') 
	{
		struct in_addr me;
#if defined (_WIN32)
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
#define	HAS_ADDR inet_pton(AF_INET, server_ip, &me) != 1
#else
#define	HAS_ADDR (me.s_addr = inet_addr(server_ip)) == INADDR_NONE
#endif
#else
#define HAS_ADDR inet_aton(server_ip, &me) == 0
#endif
		if ( HAS_ADDR )
		{
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
			int ret;
			bool ok= true;
			struct addrinfo hints;
			struct addrinfo *result=0;
			memset(&hints, 0, sizeof(struct addrinfo));
			hints.ai_family = AF_INET;    /* Allow IPv4 */
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_flags = AI_ALL;
			hints.ai_protocol = IPPROTO_TCP;
			ret = getaddrinfo(server_ip, 0, &hints, &result);
			if ( ret !=0 )
			{
				ERROR_PRO("Invalid address")
				err_lev = 3;
				ok = false;
			} else {
				if ( result->ai_family == AF_INET ) {
		   	 		(void) memcpy(&servaddr.sin_addr, &(((struct sockaddr_in *) result->ai_addr)->sin_addr), sizeof(servaddr.sin_addr));
				} else {
					if ( errMsg ) TEXTUS_SNPRINTF(errMsg, errstr_len, "%s", "not ipv4 address" );
					err_lev = 3;
					ok = false;
				}
			}
			if ( !result ) freeaddrinfo(result);
			if ( !ok ) return false;
#else
			struct hostent* he;
			he = gethostbyname(server_ip);
			if ( he == (struct hostent*) 0 )
			{	/* 错误 */
				ERROR_PRO("Invalid address")
				err_lev = 3;
				return false;
		   	} else {
		   	 	(void) memcpy(&servaddr.sin_addr, he->h_addr, he->h_length );
		   	 }
#endif
		} else {
			servaddr.sin_addr.s_addr = me.s_addr;
		}
	} else
	{
		if ( errMsg ) TEXTUS_STRCPY(errMsg, "annecto server ip is null!");
		err_lev = 4;
		return false;
	}

/* 建立套接字 */
#if defined(_WIN32)
#if defined(_MSC_VER) && (_MSC_VER >= 1800 )
	if ((connfd = WSASocketW(AF_INET,SOCK_STREAM, IPPROTO_TCP, NULL,0,WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET )
#else
	if ((connfd = WSASocket(AF_INET,SOCK_STREAM, IPPROTO_TCP, NULL,0,WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET )
#endif
#else
	if ((connfd = socket (AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET )
#endif 
	{
		ERROR_PRO("create socket")
		err_lev = 3;
		connfd = INVALID_SOCKET ;
		return false;
	}

	if ( !block )
	{	//设为NONBLOCK方式
		int ret;
#if defined(_WIN32)
		u_long flags;
		flags = 1;
		ret = ioctlsocket(connfd, FIONBIO, &flags);
#else
		int flags;
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

/* 开始连接, 如果成功, 则connfd != INVALID_SOCKET */
bool Tcpcli::annecto()
{
	int n;
	n = connect (connfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if ( n == SOCKET_ERROR) 
	{	//通常, 对于非阻塞方式, 这里的返回是EINPROGRESS
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
		isConnecting = true;	//该连接正在进行

	return true;
}

/* 一个TCP连接完成, nonblock方式下用 */
bool Tcpcli::annecto_done()
{
	int error = 0;
	err_lev = -1;
#if defined(__linux__) || defined(_AIX) || defined(__APPLE__) || defined(__SUNPRO_CC)
	socklen_t len = sizeof(error);
#else
	int len = sizeof(error); //WIN32, SCO都这样
#endif
	bool ret = true;
	int value;
	if ( getsockopt(connfd ,SOL_SOCKET,  SO_RCVBUF,  (GETSOCK_OPT_TYPE)&value, &len) < 0)
	{
		ERROR_PRO("getsockopt")
		err_lev = 3;
		ret = false;
	}
	rcv_frame_size = value;
#if defined(_WIN32)
	wsa_rcv.len = rcv_frame_size = value;
#endif
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
		if ( errMsg ) TEXTUS_SNPRINTF(errMsg, 512, "annecto_done success for %s:%d, fd:" TLONG_FMTu, server_ip, server_port, (connfd));
	}
#endif

	isConnecting = false;
	return ret;
}

void Tcpcli::end(bool down)
{
	if ( connfd !=INVALID_SOCKET ) 
	{
#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif
		if ( down && !isConnecting ) 	//这是已经建立连接的, 所以关闭以通知对方.
			shutdown(connfd, SHUT_RDWR);
		CLOSE(connfd);	
		connfd = INVALID_SOCKET ;
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
bool Tcpcli::recito_ex()
{	
	int rc;

	m_rcv_buf.grant(rcv_frame_size);	//保证有足够空间
	wsa_rcv.buf = (char *)m_rcv_buf.point;
	flag = 0;
	memset(&rcv_ovp, 0, sizeof(OVERLAPPED));
	rc = WSARecv(connfd, &wsa_rcv, 1, NULL, &flag, &rcv_ovp, NULL);
	if ( rc != 0 )
	{
		if ( WSA_IO_PENDING != WSAGetLastError() ) {
			ERROR_PRO ("WSARecv when connected");
			return false;
		}
	}
	return true;
}

int Tcpcli::transmitto_ex()
{
	int rc;
	TBuffer::pour(m_snd_buf, *snd_buf);
	wsa_snd.len = static_cast<DWORD>(m_snd_buf.point - m_snd_buf.base);   //发送长度
	wsa_snd.buf = (char *)m_snd_buf.base;
	memset(&snd_ovp, 0, sizeof(OVERLAPPED));
	rc = WSASend(connfd, &wsa_snd, 1, NULL, 0, &snd_ovp, NULL);

	if ( rc != 0 )
	{
		if ( WSA_IO_PENDING == WSAGetLastError() ) {
			//snd_buf->commit(-(TEXTUS_LONG)wsa_snd.len);	//已经到了系统
			return 1; //回去再试, 
		} else {
			ERROR_PRO ("WSASend");
			return -1;
		}
	}
	//snd_buf->commit(-(TEXTUS_LONG)wsa_snd.len);	//已经到了系统
	return 0;
}
#endif

/* 接收发生错误时, 建议关闭这个套接字 */
TEXTUS_LONG Tcpcli::recito()
{	
	TEXTUS_LONG len;

	rcv_buf->grant(rcv_frame_size);	//保证有足够空间
ReadAgain:
	if( (len = recv(connfd, (char *)rcv_buf->point, rcv_frame_size, MSG_NOSIGNAL)) == 0) /* (char*) for WIN32 */
	{	//对方关闭套接字
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
		{	 //有信号而已,再试
			goto ReadAgain;
		} else if ( error == EAGAIN || error == EWOULDBLOCK )
		{	//还在进行中, 回去再等.
			ERROR_PRO("recving encounter EAGAIN")
			return 0;
		} else	
		{	//的确有错误
			ERROR_PRO("recv socket")
			return -2;
		}
	}
	rcv_buf->commit(len);	/* 指针向后移 */
	return len;
}

/* 发送有错误时, 返回-1, 建议关闭这个套接字 */
int Tcpcli::transmitto()
{
#if defined(_WIN32)
	DWORD len, snd_len ;
	snd_len = static_cast<DWORD>(snd_buf->point - snd_buf->base);	//发送长度
#else
	TEXTUS_LONG len, snd_len ;
	snd_len = snd_buf->point - snd_buf->base;	//发送长度
#endif
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
		{	//有信号而已,再试
			goto SendAgain;
		} else if (error ==EWOULDBLOCK || error == EAGAIN)
		{	//回去再试, 用select, 要设wrSet
			ERROR_PRO("sending encounter EAGAIN")
			if ( wr_blocked ) 	//最近一次阻塞
			{
				return 3;
			} else {	//刚发生的阻塞
				wr_blocked = true;
				return 1;
			}
		} else {
			ERROR_PRO("send")
			return -1;
		}
	}
	snd_buf->commit(-(TEXTUS_LONG)len);	//提交所读出的数据
	if (snd_len > len )
	{	
		TEXTUS_SNPRINTF(errMsg, errstr_len, "sending not completed.");
		if ( wr_blocked ) 	//最近一次还是阻塞
		{
			return 3;
		} else {	//刚发生的阻塞
			wr_blocked = true;
			return 1; //回去再试, 用select, 要设wrSet
		}
	} else 
	{	//不发生阻塞了
		if ( wr_blocked ) 	//最近一次阻塞
		{
			wr_blocked = false;
			return 2; //发送完成, 不用再设wrSet啦
		} else
		{	//一直没有阻塞
			return 0; //发送完成
		}
	}
}

