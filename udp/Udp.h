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

	int sockfd; 		/* -1��ʾ��ʵ������, ÿ����ʵ����ͬ */

	bool loc_bind(bool block = false);	/* �趨���ص�IP��˿� */
	bool peer_bind (const char *ip, const char *port);	/* �趨Զ�˵�IP��˿� */

	int recito();		//��������, ����-1��0ʱ����ر��׽��� 
	int transmitto();	/* ��������, ����
				   0:  ����OK, Ҳ��Ҫ��wrSet��.
				   1:  û����, Ҫ��wrSet
				   -1: �д���, ����ر��׽���, ���Լ�����,
					Ҫ�ɵ�����ʹ��end().
				*/
					
	void end();	/*  ������ǰ������ */
	void herit(Udp *son); //�̳�

	TBuffer *rcv_buf;
	TBuffer *snd_buf;

private:
	bool wr_blocked;	//1: ���һ��д����, 0: ���һ��дû������
};
#endif

