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
	int srvport;		//ÿ����ʵ����ͬ

#if defined(_WIN64)
	SOCKET connfd; 	//-1��ʾ��ʵ������, ÿ����ʵ����ͬ
	SOCKET listenfd;	//����
#else
	int connfd; 	//-1��ʾ��ʵ������, ÿ����ʵ����ͬ,����������ʵ���������һ�ε�����
	int listenfd;	//����
#endif

	int rcv_frame_size;

	bool servio(bool block = false);	/* �趨�����׽���, ����ɹ�,��:
				 listenfd��ֵ��*/

	bool accipio(bool block = false);
				/* ����һ������, ����ɹ�, ��:
				   1��connfd��ֵ.
				   2��client_ip��ֵ
				   3��client_port��ֵ
				   4��client_mac��ֵ
				*/
#if defined (_WIN32 )
	int accept_ex();
	bool post_accept_ex();
	OVERLAPPED rcv_ovp, snd_ovp;
	char accept_buf[8];
	WSABUF wsa_snd, wsa_rcv;
	DWORD flag;
	bool sock_start();
	LPFN_ACCEPTEX lpfnAcceptEx ;
	bool recito_ex();		//��������, ����<0ʱ����ر��׽��� 
	int transmitto_ex();	
#endif

	TEXTUS_LONG recito();		//��������, ����-1��0ʱ����ر��׽��� 
	int transmitto();	/* ��������, ����
				   0:  ����OK, Ҳ��Ҫ��wrSet��.
				   1:  û����, Ҫ��wrSet
				   -1: �д���, ����ر��׽���, ���Լ�����,
					Ҫ�ɵ�����ʹ��end().
				*/
	void end();	/*  ������ǰ������  */
	void release();	/*  �ͷ�����, ������ö����, ������һ������Ҫ�ͷ�  */
	void endListen();	/* ��������, ��Ȼ�����п��ܼ��� */
	unsigned short getSrvPort();
	char*  getSrvIP();
	void  setPort(const char *portname);	/* ��ʵ�Ƕ˿����� */
	void herit(Tcpsrv *son); //�̳�

	//���¼���ÿ��ʵ����ͬ, ������ʵ��, ��ʵ������
	TBuffer *rcv_buf;
	TBuffer *snd_buf;

private:
	bool wr_blocked;	//1: ���һ��д����, 0: ���һ��дû������
};
#endif

