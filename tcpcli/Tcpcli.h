/**
 Title: TCP�����
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
	int server_port;		//ÿ����ʵ����ͬ

	int connfd; 	//-1��ʾ��ʵ������, ÿ����ʵ����ͬ

	bool annecto(bool block = false); /* ����һ������ */
	bool annecto_done(); /* ����δ��ɵ�һ������ */
	bool isConnecting;	//true:������, false:�Ѿ�������ɻ�δ��ʼ����

	bool recito();		//��������, ����falseʱ����ر��׽��� 
	int transmitto();	/* ��������, ����
				   0:  ����OK, Ҳ��Ҫ��wrSet��.
				   1:  û����, Ҫ��wrSet
				   -1: �д���, ����ر��׽���, ���Լ�����,
					Ҫ�ɵ�����ʹ��end().
				*/
					
	void end(bool down=true);	/*  ���� �� �ͷŵ�ǰ������ */
	void herit(Tcpcli *son); //�̳�
	void  setPort(const char *portname);    /* ��ʵ�Ƕ˿����� */

	//���¼���ÿ��ʵ����ͬ
	TBuffer *rcv_buf;
	TBuffer *snd_buf;

	char *errMsg;
	int err_lev;
	int errstr_len;

private:
	bool wr_blocked;	//1: ���һ��д����, 0: ���һ��дû������
};
#endif

