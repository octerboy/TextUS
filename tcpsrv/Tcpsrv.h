/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 Title: TCP Service class
 Build: created by octerboy, 2005/06/10
 $Header: /textus/tcpsrv/Tcpsrv.h 11    08-01-16 20:02 Octerboy $
 $Date$
 $Revision$
*/

/* $NoKeywords: $ */

#ifndef TCPSRV__H
#define TCPSRV__H
#include "TBuffer.h"

class Tcpsrv {
public:
	Tcpsrv();
	~Tcpsrv();
	char *errMsg;
	int errstr_len;

	char srvip[50];		
	int srvport;		//ÿ����ʵ����ͬ

	int listenfd;	//����
	int connfd; 	//-1��ʾ��ʵ������, ÿ����ʵ����ͬ,����������ʵ���������һ�ε�����

	bool servio(bool block = false);	/* �趨�����׽���, ����ɹ�,��:
				 listenfd��ֵ��*/

	bool accipio(bool block = false);
				/* ����һ������, ����ɹ�, ��:
				   1��connfd��ֵ.
				   2��client_ip��ֵ
				   3��client_port��ֵ
				   4��client_mac��ֵ
				*/

	int recito();		//��������, ����-1��0ʱ����ر��׽��� 
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

