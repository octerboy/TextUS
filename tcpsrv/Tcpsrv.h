/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 Title: TCP Service class
 Build: created by octerboy, 2005/06/10
 $Id$
 $Date$
 $Revision$
*/

/* $NoKeywords: $ */

#ifndef TCPSRV__H
#define TCPSRV__H

#if defined (_WIN32 )
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#endif

#include "TBuffer.h"
#define RCV_FRAME_SIZE 8192
class Tcpsrv {
public:
	Tcpsrv();
	~Tcpsrv();
	char *errMsg;
	int errstr_len;

	char srvip[50];		
	int srvport;		//每个子实例相同

#if defined(_WIN64)
	SOCKET connfd; 	//-1表示本实例空闲, 每个子实例不同
	SOCKET listenfd;	//侦听
#else
	int connfd; 	//-1表示本实例空闲, 每个子实例不同,负责侦听的实例保存最近一次的连接
	int listenfd;	//侦听
	//int fnfd;
#endif

	int rcv_frame_size;

	bool servio(bool block , int back);	/* 设定侦听套接字, 如果成功,则:
				 listenfd有值。*/

	bool accipio(bool block = false);
				/* 接受一个连接, 如果成功, 则:
				   1、connfd有值.
				   2、client_ip有值
				   3、client_port有值
				   4、client_mac有值
				*/
#if defined(_WIN32)
	int accept_ex();
	bool post_accept_ex();
	OVERLAPPED rcv_ovp, snd_ovp, fin_ovp;
	char accept_buf[8];
	WSABUF wsa_snd, wsa_rcv;
	DWORD flag;
	bool sock_start();
	bool recito_ex();		//接收数据, 返回<0时建议关闭套接字 
	int transmitto_ex();	
	bool finis_ex();	
	TBuffer m_rcv_buf, m_snd_buf;	/* OVERLAP*/
	struct G_CFG {
		LPFN_ACCEPTEX fnAcceptEx ;
		LPFN_DISCONNECTEX fnDisconnectEx;
#if defined(_WIN64)
		SOCKET fnfd;
#endif
	};
	struct G_CFG *gCFG;
#endif

	TEXTUS_LONG recito();		//接收数据, 返回-1或0时建议关闭套接字 
	int transmitto();	/* 发送数据, 返回
				   0:  发送OK, 也不要设wrSet了.
				   1:  没发完, 要设wrSet
				   -1: 有错误, 建议关闭套接字, 但自己不关,
					要由调用者使用end().
				*/
	void end();	/*  结束当前的连接  */
	void release();	/*  释放连接, 比如采用多进程, 则其中一个进程要释放  */
	void endListen();	/* 结束服务, 当然连接有可能继续 */
	unsigned short getSrvPort();
	char*  getSrvIP();
	void  setPort(const char *portname);	/* 其实是端口名称 */
	void herit(Tcpsrv *son); //继承

	//以下几行每个实例不同, 用于子实例, 父实例不用
	TBuffer *rcv_buf;
	TBuffer *snd_buf;

private:
	bool wr_blocked;	//1: 最近一次写阻塞, 0: 最近一次写没有阻塞
};
#endif

