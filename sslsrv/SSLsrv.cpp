/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: SSL服务的数据处理, 不涉及通信
 Build: created by octerboy, 2005/06/10
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
#include "version_1.c"
/* $NoKeywords: $ */

#include "SSLsrv.h"
#include <assert.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/engine.h>
#include <openssl/conf.h>
#include <string.h>

#define BZERO(X) memset(X, 0 ,sizeof(X))
#define SND_LEN (snd_buf->point - snd_buf->base)
#define RCV_LEN (rcv_buf->point - rcv_buf->base)

SSLsrv::SSLsrv()
{
	BZERO(my_cert_file);
	BZERO(my_key_file);
	BZERO(ca_cert_file);
	BZERO(crl_file);
	BZERO(capath);
	BZERO(engine_id);
	BZERO(dso);

	ssl_ctx = (SSL_CTX*) 0;
	ssl = (SSL*) 0;
	rbio = 0;
	wbio = 0;
	handshake_ok = false;
	rcv_buf = new TBuffer(8192); //明文, 收到的数据
	snd_buf = new TBuffer(8192); //明文, 要发出的数据

	bio_in_buf = 0;
	bio_out_buf = 0;

	matris = false;
	isVpeer = true;	/* 默认要验客户端证书 */
}

SSLsrv::~SSLsrv()
{
	if ( matris)	//父实例才销
		endctx();
}

bool SSLsrv::encrypt()
{
	bool ret = false;
	int how;
	int puttedLen, len;
	err_lev = -1;

	if ( !ssl) return false;
	/* 灌明文,从snd_buf取出数据 */	
	len = static_cast<int> SND_LEN;
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
	return true;

ErrorPro:
	/* 有错误了 */
	how = SSL_get_error(ssl, puttedLen);
	if ( how ==  SSL_ERROR_WANT_READ )
	{	/* 想读数据, 回去再等就是了 */
		ret = true;
#ifndef NDEBUG
		TEXTUS_SPRINTF(errMsg, "SSL_write encounter SSL_ERROR_WANT_READ" );
		err_lev = 7;
#endif
	} else if ( how == SSL_ERROR_WANT_WRITE)
	{	/* 要出数据? 奇怪 */
		TEXTUS_SPRINTF(errMsg, "SSL_write encounter SSL_ERROR_WANT_WRITE" );
		err_lev = 5;
		outwbio();
		ret = true;
	} else 
	{
		int err = ERR_get_error();
		TEXTUS_SPRINTF(errMsg, "SSL_write %s", ERR_error_string(err, (char *)NULL));
		err_lev = 3;
		ret = false;
	}
	return  ret;
}

int SSLsrv::decrypt()
{
	int reqLen;
	int how;
	int len, puttedLen;
	int ret;	/* 最后返回值 */

	err_lev = -1;
	if (!ssl) novo();
	if (!ssl ) return -1;		//出现严重错误

	/* 灌密文, 从bio_in_buf取出数据 */	
	assert(bio_in_buf);
	len = static_cast<int>(bio_in_buf->point - bio_in_buf->base);
	puttedLen = BIO_write(rbio, bio_in_buf->base, len);
	
	assert( puttedLen >= 0 );

	bio_in_buf->commit(-puttedLen);	
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

int SSLsrv:: clasp()
{
	int err_ret, ret;
	int how;
	
	err_lev = -1;
	ret = 0;
	err_ret = SSL_accept( ssl );
	if ( err_ret > 0 )
	{
		handshake_ok = true;
		ret = 1;
		goto End;
	}

	how = SSL_get_error(ssl, err_ret);
	if ( how ==  SSL_ERROR_WANT_READ )
	{	/* 还想读数据, 回去再等就是了 */
	} else if ( how == SSL_ERROR_WANT_WRITE)
	{	/* 要出数据? 奇怪 */
		TEXTUS_SPRINTF(errMsg, "SSL acccept  SSL_ERROR_WANT_WRITE");
		err_lev = 5;
		ret = -1;
	} else 
	{	/* 其它严重错误 */
		int err = ERR_get_error();
		TEXTUS_SPRINTF(errMsg, "SSL acccept %s", ERR_error_string(err, (char *)NULL));
		err_lev = 3;
		ret = -1;
	}
End:
	//总有数据要发出去的, 
	outwbio();
	return ret; 	//回去了, 反正还没有handshake结束
}

void SSLsrv::ssl_down()
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
	{	/* 还想读数据, 回去再等就是了 */
	} else if ( how == SSL_ERROR_WANT_WRITE)
	{	/* 要出数据? 奇怪 */
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

void SSLsrv::outwbio()
{
	int len;

	bio_out_buf->grant(8192);
	while ( (len = BIO_read(wbio, bio_out_buf->point, 8192)) > 0 ) 
	{			
		bio_out_buf->commit(len);  /* 指针向后移 */
		bio_out_buf->grant(8192);
	}
	
	return ;
}

void SSLsrv::novo()
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

void SSLsrv::endssl()
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
bool SSLsrv::initio()
{
	ENGINE *e;
	unsigned TEXTUS_LONG sid = 0;
	err_lev = -1;
	CRYPTO_malloc_init();
	ERR_load_crypto_strings();
	SSL_load_error_strings();
	SSL_library_init();
	OPENSSL_load_builtin_modules();

	if ( strlen(engine_id) > 0 )
	{
		ENGINE_load_builtin_engines();
		if ( strlen(dso) >0 ) 
		{
			e = ENGINE_by_id("dynamic");
			if ( !e )
			{
				int err = ERR_get_error();
				TEXTUS_SPRINTF(errMsg, "ENGINE_by_id(dynamic) %s", ERR_error_string(err, (char *)NULL));
				err_lev = 3;
				return false;
			} 
			ENGINE_ctrl_cmd_string(e, "SO_PATH", dso, 0);
			//ENGINE_ctrl_cmd_string(e, "ID", engine_id, 0);
			if ( !ENGINE_ctrl_cmd_string(e, "LOAD", NULL, 0))
			{
				int err = ERR_get_error();
				TEXTUS_SPRINTF(errMsg, "ENGINE_ctrl_cmd_string(LOAD %s) %s", dso, ERR_error_string(err, (char *)NULL));

				err_lev = 3;
				return false;
			}
			//ENGINE_add(e);
		} else {
			e = ENGINE_by_id(engine_id);
			if ( !e )
			{
				int err = ERR_get_error();
				TEXTUS_SPRINTF(errMsg, "ENGINE_by_id(%s) %s", engine_id, ERR_error_string(err, (char *)NULL));
				err_lev = 3;
				return false;
			} 
			//ENGINE_add(e);
		}
		printf("-id-%p-- %s, %s\n", ENGINE_by_id(engine_id), ENGINE_get_id(e), ENGINE_get_name(e));
		if (!ENGINE_set_default(e, ENGINE_METHOD_ALL)) 
		{
			int err = ERR_get_error();
			TEXTUS_SPRINTF(errMsg, "ENGINE_set_default(%s) %s", engine_id, ERR_error_string(err, (char *)NULL));
			err_lev = 3;
			return false;
		}
		ENGINE_free(e);
	}

	if ( !ssl_ctx )
		ssl_ctx = SSL_CTX_new( SSLv23_server_method() );
	SSL_CTX_set_session_id_context(ssl_ctx, (unsigned char *)&sid, sizeof(sid));
	if ( isVpeer )
	if ( SSL_CTX_load_verify_locations( ssl_ctx, (strlen(ca_cert_file) > 0 ? ca_cert_file:NULL), 
			(strlen(capath) > 0 ? capath:NULL) ) != 1 )
	{
		int err = ERR_get_error();
		TEXTUS_SPRINTF(errMsg, "initio() SSL load_verify_locations %s", ERR_error_string(err, (char *)NULL));
		err_lev = 3;
		endctx();
		return false;
	}
			
	if ( isVpeer )
		SSL_CTX_set_verify( ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
	if ( SSL_CTX_use_certificate_file( ssl_ctx,  my_cert_file, SSL_FILETYPE_PEM ) == 0 )
	{
		int err = ERR_get_error();
		TEXTUS_SPRINTF(errMsg, "initio() SSL use_certificate %s", ERR_error_string(err, (char *)NULL));
		err_lev = 3;
		endctx();
		return false;
	}

	if ( SSL_CTX_use_PrivateKey_file( ssl_ctx,  my_key_file, SSL_FILETYPE_PEM) == 0 
		
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

void SSLsrv::endctx()
{
	if ( ssl_ctx )
	{
		SSL_CTX_free(ssl_ctx);
		ERR_free_strings();
		ssl_ctx = 0;
	}
	return ;
}

void SSLsrv::herit(SSLsrv *child)
{
	if ( !child )  return ;
	child->ssl_ctx = ssl_ctx;
	return ;
}
