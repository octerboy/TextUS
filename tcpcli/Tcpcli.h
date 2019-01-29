/**
 Title: TCP服务端
 Build: created by octerboy, 2005/08/1
 $Id$
 $Date$
 $Revision$
*/

/* $NoKeywords: $ */

#ifndef TCPCLI__H
#define TCPCLI__H
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
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
//#include <windows.h>
#endif 
#include "TBuffer.h"
#include <stdio.h>
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#define RCV_FRAME_SIZE 8192
class Tcpcli {
public:
	Tcpcli();
	~Tcpcli();

	char server_ip[128];		
	int server_port;		//每个子实例相同

	int connfd; 	//-1表示本实例空闲, 每个子实例不同

	bool annecto(); /* 发起一个连接 */
	bool annecto_done(); /* 处理未完成的一个连接 */
	bool isConnecting;	//true:连接中, false:已经连接完成或未开始连接

	int recito();		//接收数据, 返回false时建议关闭套接字 
	int transmitto();	/* 发送数据, 返回
				   0:  发送OK, 也不要设wrSet了.
				   1:  没发完, 要设wrSet
				   -1: 有错误, 建议关闭套接字, 但自己不关,
					要由调用者使用end().
				*/
					
	void end(bool down=true);	/*  结束 或 释放当前的连接 */
	void herit(Tcpcli *son); //继承
	void  setPort(const char *portname);    /* 其实是端口名称 */

	//以下几行每个实例不同
	TBuffer *rcv_buf;
	TBuffer *snd_buf;

	char *errMsg;
	int err_lev;
	int errstr_len;
	struct sockaddr_in servaddr;
	bool clio( bool block); /* 准备connfd */

#if defined (_WIN32 )
	bool sock_start();
	bool annecto_ex(); /* 发起一个连接 */
	OVERLAPPED rcv_ovp, snd_ovp;
	WSABUF wsa_snd, wsa_rcv;
	DWORD rb, flag;
	bool recito_ex();		//接收数据, 返回<0时建议关闭套接字 
	int transmitto_ex();
#if defined( _MSC_VER ) && (_MSC_VER < 1400 )
typedef
BOOL (PASCAL FAR * LPFN_CONNECTEX) (
    IN SOCKET s,
    IN const struct sockaddr FAR *name,
    IN int namelen,
    IN PVOID lpSendBuffer OPTIONAL,
    IN DWORD dwSendDataLength,
    OUT LPDWORD lpdwBytesSent,
    IN LPOVERLAPPED lpOverlapped
    );
#endif
	LPFN_CONNECTEX lpfnConnectEx;
#endif	//for WIN32

private:
	bool wr_blocked;	//1: 最近一次写阻塞, 0: 最近一次写没有阻塞
};
#endif

