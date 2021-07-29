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
#include <winsock2.h>
#include <ws2tcpip.h>
#endif 
#include <stdio.h>
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#if defined(_WIN32)
#define CLOSE closesocket
#if defined(_MSC_VER) && (_MSC_VER < 1400 )
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EAGAIN WSAEINPROGRESS
#define EINTR WSAEINTR
#endif
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

	connfd = INVALID_SOCKET ;
	listenfd = INVALID_SOCKET ;

	wr_blocked = false; 	//刚开始, 最近一次写当然不阻塞

	/* 下面两行仅用于子实例 */
	rcv_buf = new TBuffer(RCV_FRAME_SIZE);
	snd_buf = new TBuffer(RCV_FRAME_SIZE);
	errMsg = (char*) 0;
	errstr_len = 0;
	rcv_frame_size = RCV_FRAME_SIZE;
#if defined (_WIN32)
	memset(&rcv_ovp, 0, sizeof(OVERLAPPED));
	memset(&snd_ovp, 0, sizeof(OVERLAPPED));
	memset(&fin_ovp, 0, sizeof(OVERLAPPED));
	wsa_rcv.len = rcv_frame_size;
	wsa_snd.buf = (char*)snd_buf->base;
	gCFG = 0;
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
	GUID GuidDisconnectEx = WSAID_DISCONNECTEX;
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes =0;
	BOOL bRetVal = FALSE;
	if (!gCFG ) 
		gCFG = new struct G_CFG();
	gCFG->fnfd = 0;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR)
	{
		ERROR_PRO("Error at WSAStartup()");
		return false;
	}
	gCFG->fnAcceptEx = NULL;
	gCFG->fnDisconnectEx = NULL;
#if defined(_MSC_VER) && (_MSC_VER >= 1800 )
	if (( gCFG->fnfd = WSASocketW(AF_INET,SOCK_STREAM, IPPROTO_TCP, NULL,0,WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET )
#else
	if (( gCFG->fnfd = WSASocket(AF_INET,SOCK_STREAM, IPPROTO_TCP, NULL,0,WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET )
#endif
	{
		ERROR_PRO("create socket")
		return false;
	}
	iResult = WSAIoctl(gCFG->fnfd, SIO_GET_EXTENSION_FUNCTION_POINTER,  &GuidDisconnectEx, sizeof (GuidDisconnectEx),  
						&gCFG->fnDisconnectEx, sizeof (gCFG->fnDisconnectEx), &dwBytes, NULL, NULL);
	if (iResult == SOCKET_ERROR) {
		ERROR_PRO("WSAIoctl DisconnectEx");
		return false;
	}

	iResult = WSAIoctl(gCFG->fnfd, SIO_GET_EXTENSION_FUNCTION_POINTER,  &GuidAcceptEx, sizeof (GuidAcceptEx),  
						&gCFG->fnAcceptEx, sizeof (gCFG->fnAcceptEx), &dwBytes, NULL, NULL);
	if (iResult == SOCKET_ERROR) {
		ERROR_PRO("WSAIoctl AccpetEx");
		return false;
	}

	return true;
}
#endif

/* 开始服务, 创建侦听端口, 如果成功, 则listenfd != INVALID_SOCKET */
bool Tcpsrv::servio( bool block)
{
	struct sockaddr_in servaddr;
	const int on = 1;

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
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
#define	HAS_ADDR inet_pton(AF_INET, srvip, &me) != 1
#else
#define	HAS_ADDR (me.s_addr = inet_addr(srvip)) == INADDR_NONE
#endif
#else
#define HAS_ADDR inet_aton(srvip, &me) == 0
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
			ret = getaddrinfo(srvip, 0, &hints, &result);
			if ( ret !=0 )
			{
				ERROR_PRO("Invalid address")
				ok = false;
			} else {
				if ( result->ai_family == AF_INET ) {
		   	 		(void) memcpy(&servaddr.sin_addr, &(((struct sockaddr_in *) result->ai_addr)->sin_addr), sizeof(servaddr.sin_addr));
				} else {
					if ( errMsg ) TEXTUS_SNPRINTF(errMsg, errstr_len, "%s", "not ipv4 address" );
					ok = false;
				}
			}
			if ( !result ) freeaddrinfo(result);
			if ( !ok ) return false;
#else
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
#endif
		   } else
		   {
		   	servaddr.sin_addr.s_addr = me. s_addr;
		   }
	} 	
/* 建立套接字 */
#if defined(_WIN32)
#if defined(_MSC_VER) && (_MSC_VER >= 1800 )
	if ((listenfd = WSASocketW(AF_INET,SOCK_STREAM, IPPROTO_TCP, NULL,0,WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET )
#else
	if ((listenfd = WSASocket(AF_INET,SOCK_STREAM, IPPROTO_TCP, NULL,0,WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET )
#endif 
#else
	if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET )
#endif 
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
		listenfd = INVALID_SOCKET;
		return false;
	}

	/*把生成的主动描述符转换为被动描述符*/
	if( listen (listenfd, 100) != 0 )
	{
		ERROR_PRO("listen socket")
		CLOSE(listenfd);
		listenfd = INVALID_SOCKET;

		return false;
	}

	if ( !block )
	{
#if defined(_WIN32)
		u_long flags;
		flags = 1;
		ioctlsocket(listenfd, FIONBIO, (u_long* ) &flags);
#else
		int flags;
		flags=fcntl(listenfd,F_GETFL,0);	//设为NONBLOCK方式
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
	bRetVal = gCFG->fnAcceptEx(listenfd, connfd, accept_buf, 0, /* 0 means AcceptEx completes  without waiting for any data*/
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
	int value, len;
	value = 0; len= sizeof(value);
	if ( getsockopt(connfd ,SOL_SOCKET,  SO_RCVBUF,  (char*)&value, &len) < 0)
	{
		ERROR_PRO("getsockopt SO_RCVBUF")
		return false;
	}
	wsa_rcv.len = rcv_frame_size = value;
	iResult =  setsockopt( connfd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&listenfd, sizeof(listenfd) );	
	if ( iResult == 0 ) 
		return true;
	
	ERROR_PRO("setsockopt SO_UPDATE_ACCEPT_CONTEXT")
	return false;
}
#endif

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
	if ( connfd == INVALID_SOCKET )
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
#if defined(_WIN32)
		u_long flags;
		flags = 1;
		if ( ioctlsocket(connfd, FIONBIO, &flags) == SOCKET_ERROR )
		{
			ERROR_PRO ("ioctlsocket FIONBIO");
			return false;
		}
#else
		int flags;
		flags=fcntl(connfd,F_GETFL,0);	//connfd设为NONBLOCK方式
		fcntl(connfd,F_SETFL,O_NONBLOCK|flags);
#endif
	}
	
	return true;
}

void Tcpsrv::end()
{
	if ( connfd != INVALID_SOCKET ) 
	{
#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif
		shutdown(connfd, SHUT_RDWR);
		CLOSE(connfd);	
		connfd = INVALID_SOCKET;
	}
	return ;
}

void Tcpsrv::release()
{
	if ( connfd != INVALID_SOCKET ) 
	{
		CLOSE (connfd);	
		connfd = INVALID_SOCKET;
	}
	return ;
}

void Tcpsrv::endListen()
{
	if ( listenfd != INVALID_SOCKET )
	{	
		CLOSE(listenfd);	
		listenfd = INVALID_SOCKET;
	}
	return ;
}

#if defined(_WIN32)
bool Tcpsrv::finis_ex()
{	
	BOOL rc;

	memset(&fin_ovp, 0, sizeof(OVERLAPPED));
	shutdown(connfd, SHUT_RDWR);
	rc = gCFG->fnDisconnectEx(connfd, &fin_ovp, 0, 0);
	if ( !rc )
	{
		if ( WSA_IO_PENDING == WSAGetLastError() ) {
			return true; //回去等
		} else {
			ERROR_PRO ("DisconnectEx");
		}
	}
	return rc;
}

bool Tcpsrv::recito_ex()
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
			ERROR_PRO ("WSARecv");
			return false;
		}
	}
	return true;
}

int Tcpsrv::transmitto_ex()
{
	int rc;
	if ( m_snd_buf.point != m_snd_buf.base ) return 4;	/* not empty, wait */
	TBuffer::exchange(m_snd_buf, *snd_buf);
	wsa_snd.len = static_cast<DWORD>(m_snd_buf.point - m_snd_buf.base);   //发送长度
	wsa_snd.buf = (char *)m_snd_buf.base;
	memset(&snd_ovp, 0, sizeof(OVERLAPPED));
	rc = WSASend(connfd, &wsa_snd, 1, NULL, 0, &snd_ovp, NULL);

	if ( rc != 0 )
	{
		if ( WSA_IO_PENDING == WSAGetLastError() ) {
			return 1; //回去再试, 
		} else {
			ERROR_PRO ("WSASend");
			return -1;
		}
	}
	return 0;
}
#endif

/* 接收发生错误时, 建议关闭这个套接字 */
TEXTUS_LONG Tcpsrv::recito()
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
int Tcpsrv::transmitto()
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
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
	static char str[32];
#else
	char *str = 0;
#endif
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
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
		inet_ntop(AF_INET, &(name1.sin_addr), str, sizeof(str));
#else
		str = inet_ntoa(name1.sin_addr);
#endif
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
#if defined(_WIN32)
	child->gCFG = gCFG;
#endif
	return ;
}
