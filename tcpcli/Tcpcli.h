/**
 Title: TCP�����
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
	int server_port;		//ÿ����ʵ����ͬ

	int connfd; 	//-1��ʾ��ʵ������, ÿ����ʵ����ͬ

	bool annecto(); /* ����һ������ */
	bool annecto_done(); /* ����δ��ɵ�һ������ */
	bool isConnecting;	//true:������, false:�Ѿ�������ɻ�δ��ʼ����

	int recito();		//��������, ����falseʱ����ر��׽��� 
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
	struct sockaddr_in servaddr;
	bool clio( bool block); /* ׼��connfd */

#if defined (_WIN32 )
	bool sock_start();
	bool annecto_ex(); /* ����һ������ */
	OVERLAPPED rcv_ovp, snd_ovp;
	WSABUF wsa_snd, wsa_rcv;
	DWORD rb, flag;
	bool recito_ex();		//��������, ����<0ʱ����ر��׽��� 
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
	bool wr_blocked;	//1: ���һ��д����, 0: ���һ��дû������
};
#endif

