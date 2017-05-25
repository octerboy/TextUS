/**
 Title: TCP服务端
 Build: created by octerboy, 2005/08/1
 $Header: /textus/tcpcli/Tcpcli.h 8     08-01-16 20:02 Octerboy $
 $Date$
 $Revision$
*/

/* $NoKeywords: $ */

#ifndef TCPCLI__H
#define TCPCLI__H
#include "TBuffer.h"
class Tcpcli {
public:
	Tcpcli();
	~Tcpcli();

	char server_ip[128];		
	int server_port;		//每个子实例相同

	int connfd; 	//-1表示本实例空闲, 每个子实例不同

	bool annecto(bool block = false); /* 发起一个连接 */
	bool annecto_done(); /* 处理未完成的一个连接 */
	bool isConnecting;	//true:连接中, false:已经连接完成或未开始连接

	bool recito();		//接收数据, 返回false时建议关闭套接字 
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

private:
	bool wr_blocked;	//1: 最近一次写阻塞, 0: 最近一次写没有阻塞
};
#endif

