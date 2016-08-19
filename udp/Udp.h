/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 Title: Udp class 
 Build: created by octerboy, 2005/06/10
 $Header: $
*/

/* $NoKeywords: $ */

#ifndef UDP__H
#define UDP__H
#include "TBuffer.h"
#include "textus_string.h"
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

class Udp {
public:
	Udp();
	~Udp();
	char *errMsg;
	int errstr_len;

	char local_ip[50];		
	int local_port;	

	struct sockaddr_in peer;

	int sockfd; 		/* -1表示本实例空闲, 每个子实例不同 */

	bool loc_bind(bool block = false);	/* 设定本地的IP与端口 */
	bool peer_bind (const char *ip, const char *port);	/* 设定远端的IP与端口 */

	int recito();		//接收数据, 返回-1或0时建议关闭套接字 
	int transmitto();	/* 发送数据, 返回
				   0:  发送OK, 也不要设wrSet了.
				   1:  没发完, 要设wrSet
				   -1: 有错误, 建议关闭套接字, 但自己不关,
					要由调用者使用end().
				*/
					
	void end();	/*  结束当前的连接 */
	void herit(Udp *son); //继承

	TBuffer *rcv_buf;
	TBuffer *snd_buf;

private:
	bool wr_blocked;	//1: 最近一次写阻塞, 0: 最近一次写没有阻塞
};
#endif

