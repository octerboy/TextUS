/**
 Title: 	SSLͨѶ�����ݴ���
 Description:	��������ݴ���, ���ֱ�Ӷ�fd����Ҫ��Щ, ������?��֪��.
 Build:	created by octerboy, 2005/06/10
 $Id$
 $Date$
 $Revision$
*/

/* $NoKeywords: $ */

#ifndef SSLSRV__H
#define SSLSRV__H
#include "TBuffer.h"
#ifdef USE_WINDOWS_SSPI
#undef SECURITY_WIN32
#undef SECURITY_KERNEL
#define SECURITY_WIN32 1
//#include <wincrypt.h>
#include <security.h>
//#include <sspi.h>
#include <schannel.h>
#include "textus_load_mod.h"

#elif defined(__APPLE__)
#include <Security/Security.h>
#include <Security/SecureTransport.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CommonCrypto/CommonDigest.h>

#else
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif
#include <string.h>

#define BZERO(X) memset(X, 0 ,sizeof(X))
class SSLcli
{
public:
	SSLcli();
	~SSLcli();

	bool initio();	//SSL��ʼ��, Ѱ��һϵ�е�֤���
	int decrypt(bool &has_plain, bool &has_ciph );	/* ����, >0: ���������, -1�д�, 0: closed */
	int encrypt(bool &has_ciph);			/* ����,������ʱ�Զ��� */
	void *get_ssl() {
#ifdef USE_WINDOWS_SSPI
		return (void*) &ssl;
#elif defined(__APPLE__)
		return ssl_ref;
#else
		return ssl;
#endif
	};

	struct G_CFG {
		char my_cert_file[256];		//��������������ڸ�ʵ����, ����Ϊ��
		char my_key_file[256];		
		char ca_cert_file[256];	
		char capath[256];	
		char crl_file[256]; 
		bool isVpeer;
#ifdef USE_WINDOWS_SSPI
		char secdll_fn[32];
		char provider[256];
		char secface_fn[32];
		PSecurityFunctionTable pSecFun;
		HMODULE secdll ;

		CredHandle  cred_hnd; 	//����
		bool isALPN;
		DWORD   reqFlags;
		SCHANNEL_CRED schCred;

		char alg_str[512]; 
		char proto_str[128]; 
		ALG_ID all_algs[45];
		char cert_store[16];
		char cert_sub[256];

#elif defined(__APPLE__)
		SSLContextRef ssl_ref;

#else
		SSL_CTX* ssl_ctx;	//��ʵ������
#endif
		inline G_CFG () {
			BZERO(my_cert_file);
			BZERO(my_key_file);
			BZERO(ca_cert_file);
			BZERO(crl_file);
			BZERO(capath);
			isVpeer = false;        /* Ĭ��Ҫ��Է�֤�� */
#ifdef USE_WINDOWS_SSPI
			BZERO(secdll_fn);
			BZERO(secface_fn);
			BZERO(provider);
			BZERO(alg_str);
			BZERO(cert_store);
			BZERO(cert_sub);
			memcpy(&secface_fn[0], "InitSecurityInterfaceA", 22);
			//memcpy(&secdll_fn[0], "secur32.dll", 11);
			memcpy(&secdll_fn[0], "security.dll", 12);
			memcpy(&provider[0], "Microsoft Unified Security Protocol Provider", 44);
			memcpy(&cert_store[0], "MY", 2);
			pSecFun = NULL;
			secdll = NULL;
			BZERO(&cred_hnd);
			BZERO(&schCred);
			isALPN = false;
#elif defined(__APPLE__)

#else
			ssl_ctx = (SSL_CTX*) 0;
#endif
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
	void ssl_down(bool &has_ciph);	//ssl�ر�ʱ, ��ͻ���֪ͨ

	bool handshake_ok;	/* false: ����ҪSSL_connect()
				   true:����SSL_read��SSL_write()
				   �˱���, ����ʵ����ͬ
				*/
#ifdef USE_WINDOWS_SSPI
	CtxtHandle  ssl;	// 
	int shake_st;
	DWORD   ansFlags;
	DWORD   reqFlags;
	TimeStamp tsExp;

	SecPkgContext_StreamSizes  outSize;
	SecBufferDesc              outMessage;
	SecBuffer                  outBuffers[4];

	SecBufferDesc              inMessage;
	SecBuffer                  inBuffers[4];
	
	void disp_err(const char *fun_str, SECURITY_STATUS status, int ret=-1);
	void show_err(const char *fun_str, const char *para);
	
#elif defined(__APPLE__)
	SSLContextRef ssl;

#else	/* OPENSSL */
	SSL* ssl;	//��ʵ����ͬ, Ҳ���������ж�
	BIO *rbio, *wbio;	/* �ֱ�����ssl�Ự���������������
				   rbio: ���д��ԭʼ����, ͨ��������
				   wbio: �Ӵ˶�������, ����
				*/
#endif

private:
	void outwbio(bool &has);	/* ��wbio������ */
	int clasp( bool &has_ciph);	//�µ�SSL�Ự����, ����0: ��������, 1: ���ֳɹ�, -1: ���ִ���,Ӧ����ֹ(�������������ֹssl)
	void novo();	//�µ�SSL�Ự
	bool matris;
};
#endif

