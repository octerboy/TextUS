/**
 Title: 	SSL通讯的数据处理
 Description:	纯粹的数据处理, 这比直接对fd操作要慢些, 慢多少?不知道.
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

	bool initio();	//SSL初始化, 寻找一系列的证书等
	int decrypt();	/* 脱密, 返回>0: 有明文输出, -1有错 */
	int encrypt();			/* 加密,有密文时自动发 */

	struct G_CFG {
		char my_cert_file[256];		//以下连续几项仅在父实例有, 其它为空
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
			isVpeer = false;        /* 默认要验对方证书 */
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
	SSL* ssl;	//各实例不同, 也供调用者判断
	bool isVpeer;	/* 是否要验客户端证书 */
	void ssl_down();	//ssl关闭时, 向客户端通知

	bool handshake_ok;	/* false: 还需要SSL_connect()
				   true:可以SSL_read和SSL_write()
				   此变量, 各子实例不同
				*/
private:
	void outwbio();	/* 从wbio出数据 */
	int clasp();	//新的SSL会话握手, 返回0: 还在握手, 1: 握手成功, -1: 出现错误,应当终止(但这个函数不终止ssl)
	void novo();	//新的SSL会话
	SSL_CTX* ssl_ctx;	//各实例共享
	BIO *rbio, *wbio;	/* 分别用于ssl会话的数据输入与输出
				   rbio: 向此写入原始报文, 通常是密文
				   wbio: 从此读出报文, 明文
				*/
	bool matris;
};
#endif

