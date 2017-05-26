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

	wr_blocked = false; 	//刚开始, 最近一次写当然不阻塞

	/* 下面两行仅用于子实例 */
	rcv_buf = new TBuffer(8192);
	snd_buf = new TBuffer(8192);
	errMsg = (char*) 0;
	errstr_len = 0;
}

Tcpsrv::~Tcpsrv()
{
	delete rcv_buf;
	delete snd_buf;
}

/* 开始服务, 创建侦听端口, 如果成功, 则listenfd>0 */
bool Tcpsrv::servio( bool block)
{
	struct sockaddr_in servaddr;
	const int on = 1;
#if defined (_WIN32)
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR)
	{
		ERROR_PRO("Error at WSAStartup()");
		return false;
	}
#endif
	/* 初始化套接口的地址结构 */
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;

	/* 设定端口 */
	servaddr.sin_port = htons(srvport);

	/* 设定地址 */
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

	/* 建立套接字 */
	if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET )
	{
		ERROR_PRO("create socket")
		return false;
	}

	/* 设置服务端套接字属性， 以检测客户端是否崩溃 */
	setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, (SETSOCK_OPT_TYPE)&on, sizeof(int));
	/* 地址重用 */
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (SETSOCK_OPT_TYPE)&on, sizeof(int));

	if ((bind (listenfd, (struct sockaddr *)&servaddr, sizeof (servaddr)) ) != 0 )
	{
		ERROR_PRO("bind socket")
		CLOSE(listenfd);
		listenfd = -1;
		return false;
	}

	/*把生成的主动描述符转换为被动描述符*/
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
		flags=fcntl(listenfd,F_GETFL,0);	//设为NONBLOCK方式
		fcntl(listenfd,F_SETFL,O_NONBLOCK|flags);
#endif
	}

	return true;
}

/* 接受一个TCP连接 */
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
		{	//有信号而已,再试
			goto AcceptAgain;	
		} else if ( error == EAGAIN || error ==  EWOULDBLOCK )
		{	//还在进行中, 回去再等.
			ERROR_PRO("accept encounter EAGAIN")
			return true;
		} else
		{	//的确有错误, 对方可能已关闭套接字等.
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
		flags=fcntl(connfd,F_GETFL,0);	//connfd设为NONBLOCK方式
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

/* 接收发生错误时, 建议关闭这个套接字 */
int Tcpsrv::recito()
{	
	long len;

	rcv_buf->grant(8192);	//保证有足够空间
ReadAgain:
	if( (len = recv(connfd, (char *)rcv_buf->point, 8192, MSG_NOSIGNAL)) == 0) /* (char*) for WIN32 */
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
int Tcpsrv::transmitto()
{
	long len;
	long snd_len = snd_buf->point - snd_buf->base;	//发送长度

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

	snd_buf->commit(-len);	//提交所读出的数据
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
