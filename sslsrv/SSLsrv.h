/**
 Title: 	SSLͨѶ�����ݴ���
 Description:	��������ݴ���, ���ֱ�Ӷ�fd����Ҫ��Щ, ������?��֪��.
 Build:	created by octerboy, 2005/06/10
 $Header: /textus/sslsrv/SSLsrv.h 11    15-01-25 17:43 Octerboy $
 $Date: 15-01-25 17:43 $
 $Revision: 11 $
*/

/* $NoKeywords: $ */

#ifndef SSLSRV__H
#define SSLSRV__H
#include "TBuffer.h"
#include <openssl/err.h>
#include <openssl/ssl.h>

class SSLsrv
{
public:
	SSLsrv();
	~SSLsrv();

	bool initio();	//SSL��ʼ��, Ѱ��һϵ�е�֤���
	int decrypt();	/* ����, ����>0: ���������, -1�д� */
	bool encrypt();			/* ����,������ʱ�Զ��� */

	char my_cert_file[256];		//��������������ڸ�ʵ����, ����Ϊ��
	char my_key_file[256];		
	char ca_cert_file[256];	
	char capath[256];	
	char crl_file[256];
	char engine_id[256];
	char dso[512];
	
	char errMsg[1024];	//������Ϣ������	
	int err_lev;

	void herit(SSLsrv *child);
	//���¼���ÿ��ʵ����ͬ, ������ʵ��, ��ʵ������
	TBuffer *bio_in_buf;	/* ������������� */
	TBuffer *bio_out_buf;	/* ������������� */

	TBuffer *rcv_buf;	/* �ѽ��յ����� */
	TBuffer *snd_buf;	/* ׼�����ص����� */

	void endssl();	/* ����SSL�Ự */
	void endctx();	/* ��������SSL���� */
	SSL* ssl;	//��ʵ����ͬ, Ҳ���������ж�
	bool isVpeer;	/* �Ƿ�Ҫ��ͻ���֤�� */
	void ssl_down();	//ssl�ر�ʱ, ��ͻ���֪ͨ
	bool handshake_ok;	/* false: ����ҪSSL_accept()
				   true:����SSL_read��SSL_write()
				   �˱���, ����ʵ����ͬ
				*/

private:
	void outwbio();	/* ��wbio������ */
	int clasp();	//�µ�SSL�Ự����, ����0: ��������, 1: ���ֳɹ�, -1: ���ִ���,Ӧ����ֹ(�������������ֹssl)
	void novo();	//�µ�SSL�Ự
	SSL_CTX* ssl_ctx;	//��ʵ������
	BIO *rbio, *wbio;	/* �ֱ�����ssl�Ự���������������
				   rbio: ���д��ԭʼ����, ͨ��������
				   wbio: �Ӵ˶�������, ����
				*/
	bool matris;
};
#endif

