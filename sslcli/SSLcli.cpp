/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: SSL客户端的数据处理, 不涉及通信
 Build: created by octerboy, 2007/12/18
 $Header: /textus/sslcli/SSLcli.cpp 5     10-12-14 19:58 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: SSLcli.cpp $"
#define TEXTUS_MODTIME  "$Date: 10-12-14 19:58 $"
#define TEXTUS_BUILDNO  "$Revision: 5 $"
#include "version_1.c"
/* $NoKeywords: $ */

#include "SSLcli.h"
#include <assert.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string.h>

#define SND_LEN (snd_buf->point - snd_buf->base)
#define RCV_LEN (rcv_buf->point - rcv_buf->base)

SSLcli::SSLcli():bio_in_buf(8192),bio_out_buf(8192)
{
	ssl_ctx = (SSL_CTX*) 0;
	ssl = (SSL*) 0;
	rbio = 0;
	wbio = 0;
	handshake_ok = false;
	rcv_buf = 0;	/* 明文, 从底层收到的数据 */
	snd_buf = 0;	/* 明文, 要发送的数据 */

	matris = false;
	gCFG = 0 ;
}

SSLcli::~SSLcli()
{
	if ( matris)	//父实例才销
		endctx();
}

int SSLcli::encrypt()
{
	int ret = 1;
	int how;
	int puttedLen, len;
	err_lev = -1;

	if (!ssl) novo();
	if (!ssl ) return -1;		//出现严重错误

	if (!handshake_ok)
	{	//没有握手就握手
		ret = clasp();
		if ( ret <= 0 )
			return ret;
	}

	/* 灌明文,从snd_buf取出数据 */	
	len = SND_LEN;
	if ( len == 0 )
		return 1;

	puttedLen = SSL_write( ssl, snd_buf->base, len);

	if ( puttedLen < 0 )
		goto ErrorPro;
	
	snd_buf->commit(-puttedLen);	/* 提交已取出的数据 */
	if ( puttedLen != len)
	{	/* 如果没有取出全部数据, 奇怪! */
		TEXTUS_SPRINTF(errMsg, "encrypt BIO_write %d, left %d",len, len-puttedLen);
		err_lev = 5;
	}

	outwbio(); /* 输出密文 */
	return 1;

ErrorPro:
	/* 有错误了 */
	how = SSL_get_error(ssl, puttedLen);
	if ( how ==  SSL_ERROR_WANT_READ )
	{	/* 想读数据, 回去再等就是了 */
		ret = 1;
#ifndef NDEBUG
		TEXTUS_SPRINTF(errMsg, "SSL_write encounter SSL_ERROR_WANT_READ" );
		err_lev = 7;
#endif
	} else if ( how == SSL_ERROR_WANT_WRITE)
	{	/* 要出数据? 奇怪 */
		TEXTUS_SPRINTF(errMsg, "SSL_write encounter SSL_ERROR_WANT_WRITE" );
		err_lev = 5;
		outwbio();
		ret = 1;
	} else 
	{
		int err = ERR_get_error();
		TEXTUS_SPRINTF(errMsg, "SSL_write %s", ERR_error_string(err, (char *)NULL));
		err_lev = 3;
		ret = -1;
	}
	return  ret;
}

int SSLcli::decrypt()
{
	int reqLen;
	int how;
	int len, puttedLen;
	int ret;	/* 最后返回值 */

	err_lev = -1;
	if ( !ssl) return -1;

	/* 灌密文, 从bio_in_buf取出数据 */	
	len = (bio_in_buf.point) - (bio_in_buf.base);
	puttedLen = BIO_write(rbio, bio_in_buf.base, len);
	
	assert( puttedLen >= 0 );

	bio_in_buf.commit(-puttedLen);	
	if ( puttedLen != len )
	{	/* 没有全部进去, 奇怪 */
		TEXTUS_SPRINTF(errMsg, "BIO_write %d, left %d", len, len-puttedLen);
		err_lev = 5;
	}

	if (!handshake_ok)
	{	//没有握手就握手
		ret = clasp();
		if ( ret <= 0 )
			return ret;
	}

	/* 现在开始试图读取明文 */
	rcv_buf->grant(8192);   //保证有足够空间
	ret = 0;
	while ( (reqLen = SSL_read( ssl, rcv_buf->point, 8192)) > 0 )
	{
		rcv_buf->commit(reqLen);	/* 提交已入的明文 */
		ret += reqLen;
		rcv_buf->grant(8192);   //保证有足够空间
	}

	/* 有错误了 */
	how = SSL_get_error(ssl, reqLen);
	if ( how ==  SSL_ERROR_WANT_READ )
	{	/* 还想读数据, 回去再等就是了 */
	} else if ( how == SSL_ERROR_WANT_WRITE)
	{	/* 要出数据? 奇怪 */
		TEXTUS_SPRINTF(errMsg, "SSL_read SSL_ERROR_WANT_WRITE");
		err_lev = 5;
	} else 
	{	/* 真的严重错误 */
		int err = ERR_get_error();
		TEXTUS_SPRINTF(errMsg,"SSL_read %s", ERR_error_string(err, (char *)NULL));
		err_lev = 3;
		ret = -1;
	}
	//也许有数据要发出去的, 
	outwbio();
	return  ret ;	//ret > 0: 有明文啦
}

int SSLcli:: clasp()
{
	int err_ret, ret;
	int how;
	
	err_lev = -1;
	ret =  0;
	err_ret = SSL_connect ( ssl );
	if ( err_ret > 0 )
	{
		handshake_ok = true;
		ret = 1;
		goto End;
	}

	how = SSL_get_error(ssl, err_ret);
	if ( how ==  SSL_ERROR_WANT_READ )
	{	/* 还要输入密文数据, 回去再等就是了 */
	} else if ( how == SSL_ERROR_WANT_WRITE)
	{	/* 要出密文数据? 奇怪, 因为密文数据是输出到snd_buf之中的 */
		TEXTUS_SPRINTF(errMsg, "SSL connect SSL_ERROR_WANT_WRITE");
		err_lev = 5;
		ret = -1;
	} else 
	{	/* 其它严重错误 */
		int err = ERR_get_error();
		TEXTUS_SPRINTF(errMsg, "SSL connect %s", ERR_error_string(err, (char *)NULL));
		err_lev = 3;
		ret = -1;
	}
End:
	//总有数据要发出去的, 
	outwbio();
	return ret; 	//回去了, 反正还没有handshake结束
}

void SSLcli::ssl_down()
{
	int ret,how ;
	err_lev = -1;
	ret = SSL_shutdown(ssl);
	if ( ret > 0 )
	{
		return;
	}

	how = SSL_get_error(ssl, ret);
	if ( how ==  SSL_ERROR_WANT_READ )
	{	/* 还想输入密文数据, 回去再等就是了 */
	} else if ( how == SSL_ERROR_WANT_WRITE)
	{	/* 要出密文数据? 奇怪,因为密文是出到snd_buf中的,　除非空间太小 */
		TEXTUS_SPRINTF(errMsg, "SSL_shutdown  SSL_ERROR_WANT_WRITE");
		err_lev = 5;
	} else 
	{	/* 其它严重错误 */
		err_lev = 3;
		TEXTUS_SPRINTF(errMsg, "SSL_shutdown %s", ERR_error_string(how, (char *)NULL));
		endssl();
		return ;
	}
	outwbio(); /* 输出密文 */
}

void SSLcli::outwbio()
{
	int len;

	bio_out_buf.grant(8192);
	while ( (len = BIO_read(wbio, bio_out_buf.point, 8192)) > 0 ) 
	{			
		bio_out_buf.commit(len);  /* 指针向后移 */
	}
	
	return ;
}

void SSLcli::novo()
{
	assert ( !ssl );
	if (!ssl_ctx ) return;
	ssl = SSL_new( ssl_ctx );
	if (!ssl) 
	{
		int err = ERR_get_error();
		TEXTUS_SPRINTF(errMsg, "initio() novo() SSL_new %s", ERR_error_string(err, (char *)NULL));
		err_lev = 3;
		return;
	}		
	rbio = BIO_new(BIO_s_mem());
	wbio =  BIO_new(BIO_s_mem());
	if ( !rbio || !wbio )
	{
		SSL_free(ssl);
		ssl = (SSL*) 0;
		return;
	}
	SSL_set_bio(ssl, rbio, wbio);

	return ;
}

void SSLcli::endssl()
{
	if ( ssl )
		SSL_free(ssl);
	ssl = 0;
	rbio = 0;
	wbio = 0;
	handshake_ok = false;
	
	return;
}

/* 父实例才执行本函数 */
bool SSLcli::initio()
{
	err_lev = -1;
	SSL_library_init();
	SSL_load_error_strings();

	if ( !ssl_ctx )
		ssl_ctx = SSL_CTX_new(SSLv23_client_method());

	if ( !ssl_ctx ) return false;
	if ( !gCFG ) return true;

	if ( gCFG->isVpeer && 
	 	SSL_CTX_load_verify_locations( ssl_ctx, (strlen(gCFG->ca_cert_file) > 0 ? gCFG->ca_cert_file:NULL), 
			(strlen(gCFG->capath) > 0 ? gCFG->capath:NULL) ) != 1 )
	{
		int err = ERR_get_error();
		TEXTUS_SPRINTF(errMsg, "initio() SSL load_verify_locations %s", ERR_error_string(err, (char *)NULL));
		err_lev = 3;
		endctx();
		return false;
	}
			
	if ( gCFG->isVpeer )
		SSL_CTX_set_verify( ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

	if ( strlen(gCFG->my_cert_file) > 0 ) 
	if ( SSL_CTX_use_certificate_file( ssl_ctx,  gCFG->my_cert_file, SSL_FILETYPE_PEM ) == 0 )
	{
		int err = ERR_get_error();
		TEXTUS_SPRINTF(errMsg, "initio() SSL use_certificate %s", ERR_error_string(err, (char *)NULL));
		err_lev = 3;
		endctx();
		return false;
	}

	if ( strlen(gCFG->my_key_file) > 0 ) 
	if ( SSL_CTX_use_PrivateKey_file( ssl_ctx,  gCFG->my_key_file, SSL_FILETYPE_PEM) == 0 
		
		|| SSL_CTX_check_private_key( ssl_ctx ) == 0 )
	{
		int err = ERR_get_error();
		TEXTUS_SPRINTF(errMsg,"initio() SSL private key %s", ERR_error_string(err, (char *)NULL) );
		err_lev = 3;
		endctx();
		return false;
	}

	matris = true;
	return true;
}

void SSLcli::endctx()
{
	if ( ssl_ctx )
	{
		SSL_CTX_free(ssl_ctx);
		ERR_free_strings();
		ssl_ctx = 0;
	}
	return ;
}

void SSLcli::herit(SSLcli *child)
{
	if ( !child )  return ;
	child->ssl_ctx = ssl_ctx;
	child->gCFG = gCFG;
	return ;
}
