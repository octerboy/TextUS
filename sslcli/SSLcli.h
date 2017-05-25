/**
 Title: 	SSLͨѶ�����ݴ���
 Description:	��������ݴ���, ���ֱ�Ӷ�fd����Ҫ��Щ, ������?��֪��.
 Build:	created by octerboy, 2005/06/10
 $Header: /textus/sslcli/SSLcli.h 4     13-10-04 17:25 Octerboy $
 $Date$
 $Revision$
*/

/* $NoKeywords: $ */

#ifndef SSLSRV__H
#define SSLSRV__H
#include "TBuffer.h"
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string.h>

#define BZERO(X) memset(X, 0 ,sizeof(X))
class SSLcli
{
public:
	SSLcli();
	~SSLcli();

	bool initio();	//SSL��ʼ��, Ѱ��һϵ�е�֤���
	int decrypt();	/* ����, ����>0: ���������, -1�д� */
	int encrypt();			/* ����,������ʱ�Զ��� */

	struct G_CFG {
		char my_cert_file[256];		//��������������ڸ�ʵ����, ����Ϊ��
		char my_key_file[256];		
		char ca_cert_file[256];	
		char capath[256];	
		char crl_file[256]; 
		bool isVpeer;
		inline G_CFG () {
			BZERO(my_cert_file);
			BZERO(my_key_file);
			BZERO(ca_cert_file);
			BZERO(crl_file);
			BZERO(capath);
			isVpeer = false;        /* Ĭ��Ҫ��Է�֤�� */
		};
	};

	struct G_CFG *gCFG;
	
	char errMsg[1024];	//������Ϣ������	
	int err_lev;

	void herit(SSLcli *child);
	//���¼���ÿ��ʵ����ͬ, ������ʵ��, ��ʵ������
	TBuffer bio_in_buf;	/* ������������� */
	TBuffer bio_out_buf;	/* ������������� */

	TBuffer *rcv_buf;	/* �ѽ��յ����� */
	TBuffer *snd_buf;	/* ׼�����ص����� */

	void endssl();	/* ����SSL�Ự */
	void endctx();	/* ��������SSL���� */
	SSL* ssl;	//��ʵ����ͬ, Ҳ���������ж�
	bool isVpeer;	/* �Ƿ�Ҫ��ͻ���֤�� */
	void ssl_down();	//ssl�ر�ʱ, ��ͻ���֪ͨ

	bool handshake_ok;	/* false: ����ҪSSL_connect()
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

