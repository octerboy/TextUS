
/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
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
 $Header: /textus/udp/Udp.cpp 6     07-01-25 23:59 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: Udp.cpp $"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */


#include "Udp.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

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

Udp::Udp()
{
	memset(local_ip, 0, sizeof(local_ip));
	local_port = 0;

	sockfd = -1;

	wr_blocked = false; 	//刚开始, 最近一次写当然不阻塞

	rcv_buf = new TBuffer(512);
	snd_buf = new TBuffer(512);
	errMsg = (char*) 0;
	errstr_len = 0;
}

Udp::~Udp()
{
	delete rcv_buf;
	delete snd_buf;
}

/* 本地设定, 成功则sockfd>0 */
bool Udp::loc_bind ( bool block)
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
	servaddr.sin_port = htons(local_port);

	/* 设定地址 */
	if (local_ip[0] == '\0')
	{
		servaddr.sin_addr.s_addr  = htonl( INADDR_ANY );
	} else
	{
		struct in_addr me;
#if defined (_WIN32)
#define	HAS_ADDR (me.s_addr = inet_addr(local_ip)) == INADDR_NONE
#else
#define HAS_ADDR inet_aton(local_ip, &me) == 0
#endif
		if ( HAS_ADDR )
		{
			struct hostent* he;
			he = gethostbyname(local_ip);
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
	if ((sockfd = socket (AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET )
	{
		ERROR_PRO("create socket")
		return false;
	}

	if ((bind (sockfd, (struct sockaddr *)&servaddr, sizeof (servaddr)) ) != 0 )
	{
		ERROR_PRO("bind socket")
		CLOSE(sockfd);
		sockfd = -1;
		return false;
	}

	if ( !block )
	{
		int flags;
#if defined(_WIN32)
		flags = 1;
		ioctlsocket(sockfd, (long) FIONBIO, (u_long* ) &flags);
#else
		flags=fcntl(sockfd,F_GETFL,0);	//设为NONBLOCK方式
		fcntl(sockfd,F_SETFL,O_NONBLOCK|flags);
#endif
	}

	return true;
}

/* 远程设定 */
bool Udp::peer_bind (const char *ip, const char *port)
{
	/* 设定端口 */

	if ( !port ) 
		goto NEXT;
	if (isdigit(*port)) 
	{
		int number = atoi(port);
		if (number >= 0)
			peer.sin_port = htons((unsigned short) number);
	} else {
		struct servent *servent = getservbyname(port, "udp");
		if (servent)
			peer.sin_port = servent->s_port;
	}

NEXT:
	/* 设定地址 */
	if (ip[0] == '\0')
	{
		peer.sin_addr.s_addr  = htonl( INADDR_ANY );
	} else
	{
		struct in_addr me;
#if defined (_WIN32)
#define	HAS_ADDR (me.s_addr = inet_addr(local_ip)) == INADDR_NONE
#else
#define HAS_ADDR inet_aton(local_ip, &me) == 0
#endif
		if ( HAS_ADDR )
		{
			struct hostent* he;
			he = gethostbyname(local_ip);
			if ( he == (struct hostent*) 0 )
			{
				ERROR_PRO("Invalid address")
				return false;
		   	} else
		   	{
		   	 	(void) memcpy(&peer.sin_addr, he->h_addr, he->h_length );
		   	 }
		   } else
		   {
		   	peer.sin_addr.s_addr = me. s_addr;
		   }
	} 	
}

void Udp::end()
{
	if ( sockfd > 0 ) 
	{	
		CLOSE(sockfd);	
		sockfd =-1;
	}

	return ;
}

/* 接收发生错误时, 建议关闭这个套接字 */
int Udp::recito()
{	
	int len;
#if defined(__linux__) || defined(_AIX)
#define	CLI_TYPE socklen_t 
#else
#define	CLI_TYPE int 
#endif
	CLI_TYPE pl;
	pl = sizeof(peer);

	rcv_buf->grant(8192);	/* 保证有足够空间 */
ReadAgain:
	len = recvfrom (sockfd, (char *)rcv_buf->point, 8192, MSG_NOSIGNAL,
		(struct sockaddr *)&peer, &pl) ; /* (char*) for WIN32 */

	if( len == 0 )
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
int Udp::transmitto()
{
	int len;
	int snd_len = snd_buf->point - snd_buf->base;	//发送长度

SendAgain:
	len = sendto(sockfd, (char *)snd_buf->base, snd_len, MSG_NOSIGNAL, 
		(struct sockaddr *)&peer, sizeof (peer));  /* (char*) for WIN32 */

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

void Udp::herit(Udp *child)
{
	assert(child);
	//memcpy (child->local_ip, local_ip, sizeof(local_ip));
	//child->local_port = local_port;
	memcpy(&(child->peer), &peer, sizeof(peer));
	return ;
}
#include "version_1.c"
