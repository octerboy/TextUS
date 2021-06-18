/**
 Title: 	SSL通讯的数据处理
 Description:	纯粹的数据处理, 这比直接对fd操作要慢些, 慢多少?不知道.
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

	bool initio();	//SSL初始化, 寻找一系列的证书等
	int decrypt(bool &has_plain, bool &has_ciph );	/* 脱密, >0: 有明文输出, -1有错, 0: closed */
	int encrypt(bool &has_ciph);			/* 加密,有密文时自动发 */
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
		char my_cert_file[256];		//以下连续几项仅在父实例有, 其它为空
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

		CredHandle  cred_hnd; 	//共享
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
		SSL_CTX* ssl_ctx;	//各实例共享
#endif
		inline G_CFG () {
			BZERO(my_cert_file);
			BZERO(my_key_file);
			BZERO(ca_cert_file);
			BZERO(crl_file);
			BZERO(capath);
			isVpeer = false;        /* 默认要验对方证书 */
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
	char errMsg[1024];	//错误信息缓冲区	
	int err_lev;

	void herit(SSLcli *child);
	//以下几行每个实例不同, 用于子实例, 父实例不用
	TBuffer bio_in_buf;	/* 用以输入的密文 */
	TBuffer bio_out_buf;	/* 用以输出的密文 */

	TBuffer *rcv_buf;	/* 已接收的明文 */
	TBuffer *snd_buf;	/* 准备发回的明文 */

	void endssl();	/* 结束SSL会话 */
	void endctx();	/* 结束整个SSL环境 */
	void ssl_down(bool &has_ciph);	//ssl关闭时, 向客户端通知

	bool handshake_ok;	/* false: 还需要SSL_connect()
				   true:可以SSL_read和SSL_write()
				   此变量, 各子实例不同
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
	SSL* ssl;	//各实例不同, 也供调用者判断
	BIO *rbio, *wbio;	/* 分别用于ssl会话的数据输入与输出
				   rbio: 向此写入原始报文, 通常是密文
				   wbio: 从此读出报文, 明文
				*/
#endif

private:
	void outwbio(bool &has);	/* 从wbio出数据 */
	int clasp( bool &has_ciph);	//新的SSL会话握手, 返回0: 还在握手, 1: 握手成功, -1: 出现错误,应当终止(但这个函数不终止ssl)
	void novo();	//新的SSL会话
	bool matris;
};
#endif

