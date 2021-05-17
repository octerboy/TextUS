/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
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
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
#include "version_1.c"
/* $NoKeywords: $ */

#include "SSLcli.h"
#include <assert.h>

#ifdef USE_WINDOWS_SSPI

#elif defined(__APPLE__)

#else

#endif

#define SND_LEN (snd_buf->point - snd_buf->base)
#define RCV_LEN (rcv_buf->point - rcv_buf->base)

SSLcli::SSLcli():bio_in_buf(8192),bio_out_buf(8192)
{
#ifdef USE_WINDOWS_SSPI
	memset(&ssl, 0, sizeof(CtxtHandle));
	shake_st = 0;

#elif defined(__APPLE__)

#else
	ssl = (SSL*) 0;
	rbio = 0;
	wbio = 0;
#endif
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
#ifdef USE_WINDOWS_SSPI

#elif defined(__APPLE__)

#else

#endif

}

int SSLcli::encrypt(bool &has_ciph)
{
	int ret = 1;
	int len;
	err_lev = -1;

#ifdef USE_WINDOWS_SSPI
	SECURITY_STATUS	scRet;
	size_t data_len = 0;

#elif defined(__APPLE__)

#else
	int how, err;
	int puttedLen;
	if (!ssl) novo();
	if (!ssl ) return -1;		//出现严重错误
#endif

	has_ciph=false;
	if (!handshake_ok)
	{	//没有握手就握手
		ret = clasp( has_ciph);
		//printf("--- encrypt clasp has_ciph %d , ret %d\n", has_ciph, ret);
		if ( ret < 0 )
			return ret;
		else if ( ret == 0 )
			return 1;
	}

	/* 灌明文,从snd_buf取出数据 */	
	len = static_cast<int>SND_LEN;
	if ( len == 0 )
		return 1;

#ifdef USE_WINDOWS_SSPI

	/* setup output buffers (header, data, trailer, empty) */
	//InitSecBuffer(&outbuf[0], SECBUFFER_STREAM_HEADER, ptr, BACKEND->stream_sizes.cbHeader);
	data_len = outSize.cbHeader + len + outSize.cbTrailer;
	bio_out_buf.grant(data_len);
	outBuffers[0].BufferType= SECBUFFER_STREAM_HEADER;
	outBuffers[0].pvBuffer	= bio_out_buf.point;
	outBuffers[0].cbBuffer	= outSize.cbHeader;

	//InitSecBuffer(&outbuf[1], SECBUFFER_DATA, ptr + BACKEND->stream_sizes.cbHeader, curlx_uztoul(len));
	outBuffers[1].BufferType= SECBUFFER_DATA;
	outBuffers[1].pvBuffer	= bio_out_buf.point + outSize.cbHeader;
	outBuffers[1].cbBuffer	= len;
	memcpy( outBuffers[1].pvBuffer, snd_buf->base, len);

	//InitSecBuffer(&outbuf[2], SECBUFFER_STREAM_TRAILER, ptr + BACKEND->stream_sizes.cbHeader + len, BACKEND->stream_sizes.cbTrailer);
	outBuffers[2].BufferType= SECBUFFER_STREAM_TRAILER;
	outBuffers[2].pvBuffer	= bio_out_buf.point + outSize.cbHeader + len;
	outBuffers[2].cbBuffer	= outSize.cbTrailer;

	//InitSecBuffer(&outbuf[3], SECBUFFER_EMPTY, NULL, 0);
	outBuffers[3].BufferType= SECBUFFER_EMPTY;
	outBuffers[3].pvBuffer	= NULL;
	outBuffers[3].cbBuffer	= 0;

	//InitSecBufferDesc(&outbuf_desc, outbuf, 4);
	outMessage.ulVersion	= SECBUFFER_VERSION;
	outMessage.cBuffers	= 4;
	outMessage.pBuffers	= outBuffers;

	scRet = gCFG->pSecFun->EncryptMessage(&ssl, 0, &outMessage, 0);
	if (  scRet != SEC_E_OK )
		goto ErrorPro;

	snd_buf->commit_ack(-len);	/* 提交已取出的数据 */
	bio_out_buf.commit_ack(data_len);  /* 指针向后移 */
	has_ciph = true;
#elif defined(__APPLE__)

#else
	puttedLen = SSL_write( ssl, snd_buf->base, len);
	if ( puttedLen < 0 )
		goto ErrorPro;
	
	snd_buf->commit_ack(-puttedLen);	/* 提交已取出的数据 */
	if ( puttedLen != len)
	{	/* 如果没有取出全部数据, 奇怪! */
		TEXTUS_SPRINTF(errMsg, "encrypt BIO_write %d, left %d",len, len-puttedLen);
		err_lev = 5;
	}
	outwbio ( has_ciph); /* 输出密文 */
#endif
	return 1;

ErrorPro:
	/* 有错误了 */
#ifdef USE_WINDOWS_SSPI

	err_lev = 3;
	ret = -1;
	disp_err("EncryptMessage", scRet, -1);
#elif defined(__APPLE__)

#else
	how = SSL_get_error(ssl, puttedLen);
	switch ( how )
	{
	case SSL_ERROR_WANT_READ:
		/* 想读数据, 回去再等就是了 */
		ret = 1;
#ifndef NDEBUG
		TEXTUS_SPRINTF(errMsg, "SSL_write encounter SSL_ERROR_WANT_READ" );
		err_lev = 7;
#endif
		break;
	case SSL_ERROR_NONE:
		break;

	case SSL_ERROR_WANT_WRITE:
		/* 要出数据? 奇怪 */
		TEXTUS_SPRINTF(errMsg, "SSL_write SSL_ERROR_WANT_WRITE");
		err_lev = 5;
		outwbio( has_ciph);
		ret = 1;
		break;
	case SSL_ERROR_ZERO_RETURN:
		/*  a closure alert has occurred in the protocol*/
		TEXTUS_SPRINTF(errMsg,"SSL_write SSL_ERROR_ZERO_RETURN(%d)", how);
		err_lev = 3;
		ret = 0;
		break;
	default:
		/* 真的严重错误 */
		err = ERR_get_error();
		TEXTUS_SPRINTF(errMsg,"SSL_write err=%d: %s", how, ERR_error_string(err, (char *)NULL));
		err_lev = 3;
		ret = -1;
	}
#endif
	return  ret;
}

int SSLcli::decrypt(bool &has_plain, bool &has_ciph)
{
	int ret;	/* 最后返回值 */
	int len;

	err_lev = -1;
	has_ciph = false;
	has_plain = false;
	/* 灌密文, 从bio_in_buf取出数据 */	
	len = static_cast<int>(bio_in_buf.point - bio_in_buf.base);
#ifdef USE_WINDOWS_SSPI
	SECURITY_STATUS	scRet;
	size_t data_len = 0;

	if (!handshake_ok)
	{	//没有握手就握手
		ret = clasp (has_ciph);
		//printf("--- decrypt clasp has_ciph %d , ret %d\n", has_ciph, ret);
		if ( ret < 0 )
			return ret;
		else if ( ret >= 0 )
			return 1;
	}
D_AGAIN:
	ret = 1;
	inBuffers[0].BufferType= SECBUFFER_DATA;
	inBuffers[0].pvBuffer	= bio_in_buf.base;
	inBuffers[0].cbBuffer	= len;
	inBuffers[1].BufferType = SECBUFFER_EMPTY;
	inBuffers[2].BufferType = SECBUFFER_EMPTY;
	inBuffers[3].BufferType = SECBUFFER_EMPTY;

	inMessage.ulVersion	= SECBUFFER_VERSION;
	inMessage.cBuffers	= 4;
	inMessage.pBuffers	= inBuffers;

	scRet = gCFG->pSecFun->DecryptMessage(&ssl, &inMessage, 0, NULL);
	switch ( scRet ) {
	case SEC_I_RENEGOTIATE:
	case SEC_I_CONTEXT_EXPIRED: 
	case SEC_E_OK:
		if (inBuffers[1].BufferType == SECBUFFER_DATA)
		{
			//printf("plain %d\n",  inBuffers[1].cbBuffer);
			rcv_buf->input((unsigned char*)inBuffers[1].pvBuffer, (unsigned TEXTUS_LONG) inBuffers[1].cbBuffer);
			has_plain = true;
		}
		if (inBuffers[3].BufferType == SECBUFFER_EXTRA)
		{
			//printf("cipher remain %d\n",  inBuffers[3].cbBuffer);
			bio_in_buf.commit(-((TEXTUS_LONG)(len - inBuffers[3].cbBuffer)));
			TEXTUS_SPRINTF(errMsg, "DecryptMessage %d, left %d", len, inBuffers[3].cbBuffer);
			err_lev = 5;
			ret = 2;
		} else {
			err_lev = -1;
			bio_in_buf.reset();
		}
		switch ( scRet ) {
		case SEC_I_RENEGOTIATE:
			TEXTUS_SPRINTF(errMsg, "DecryptMessage %s", "remote party requests renegotiation");
			handshake_ok = false;
			err_lev = 5;
			break;

		case SEC_I_CONTEXT_EXPIRED: 
			TEXTUS_SPRINTF(errMsg, "DecryptMessage %s", "server closed the connection");
			err_lev = 3;
			ret = 0;
			break;
		}

		break;
	
	case SEC_E_INCOMPLETE_MESSAGE:
		ret = 1;
		disp_err("DecryptMessage", scRet,0);
		break;

	default:	/* other error */
		ret = -1;
		disp_err("DecryptMessage", scRet);
		break;
	}
	if ( ret == 2 ) { 
		//printf("again-----\n"); 
		len = static_cast<int>(bio_in_buf.point - bio_in_buf.base);
		goto D_AGAIN;
	}
#elif defined(__APPLE__)

#else
	int how, err;
	int reqLen;
	int puttedLen;
	if ( !ssl) return -1;
	puttedLen = BIO_write(rbio, bio_in_buf.base, len);	//write cipher data

	assert( puttedLen >= 0 );

	bio_in_buf.commit(-puttedLen);	
	if ( puttedLen != len )
	{	/* 没有全部进去, 奇怪 */
		TEXTUS_SPRINTF(errMsg, "BIO_write %d, left %d", len, len-puttedLen);
		err_lev = 5;
	}

	if (!handshake_ok)
	{	//没有握手就握手
		ret = clasp( has_ciph);
		if ( ret < 0 )
			return ret;
		else if ( ret == 0 )
			return 1;
	}

	/* 现在开始试图读取明文 */
	rcv_buf->grant(8192);   //保证有足够空间
	ret = 1;
	while ( (reqLen = SSL_read( ssl, rcv_buf->point, 8192)) > 0 )
	{
		has_plain = true;
		rcv_buf->commit_ack(reqLen);	/* 提交已入的明文 */
		rcv_buf->grant(8192);   //保证有足够空间
	}

	/* 有错误了 */
	how = SSL_get_error(ssl, reqLen);
	switch ( how )
	{
	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_NONE:
		/* 还想读数据, 回去再等就是了 */
		break;
	case SSL_ERROR_WANT_WRITE:
		/* 要出数据? 奇怪 */
		TEXTUS_SPRINTF(errMsg, "SSL_read SSL_ERROR_WANT_WRITE");
		err_lev = 5;
		break;
	case SSL_ERROR_ZERO_RETURN:
		/*  a closure alert has occurred in the protocol*/
		TEXTUS_SPRINTF(errMsg,"SSL_read SSL_ERROR_ZERO_RETURN(%d)", how);
		err_lev = 3;
		ret = 0;
		break;
	default:
		/* 真的严重错误 */
		err = ERR_get_error();
		TEXTUS_SPRINTF(errMsg,"SSL_read err=%d: %s", how, ERR_error_string(err, (char *)NULL));
		err_lev = 3;
		ret = -1;
	}

	//也许有数据要发出去的, 
	outwbio( has_ciph);
#endif
	return  ret ;	//ret > 0: 有明文啦
}

int SSLcli:: clasp(bool &has_ciph)
{
	int ret;
	err_lev = -1;
	ret =  0;
#ifdef USE_WINDOWS_SSPI
	SECURITY_STATUS scRet = SEC_E_OK;

	//printf("shake_st %d\n", shake_st);
	switch ( shake_st )
	{
	case 0:
		goto SHAKE_ST_Start;
		break;
	case 1:
		goto SHAKE_ST_Doing;
		break;
	case 2:
		goto SHAKE_ST_Done;
		break;
	}

 SHAKE_ST_Start:
//InitSecBuffer(&inbuf, SECBUFFER_EMPTY, NULL, 0);
	inBuffers[0].BufferType = SECBUFFER_EMPTY;
	inBuffers[0].pvBuffer	= NULL;
	inBuffers[0].cbBuffer	= 0;

//InitSecBufferDesc(&inbuf_desc, &inbuf, 1);
	inMessage.ulVersion	= SECBUFFER_VERSION;
	inMessage.cBuffers	= 1;
	inMessage.pBuffers	= inBuffers;

//InitSecBuffer(&outbuf, SECBUFFER_EMPTY, NULL, 0);
	outBuffers[0].BufferType= SECBUFFER_EMPTY;
	outBuffers[0].pvBuffer	= NULL;
	outBuffers[0].cbBuffer	= 0;

//InitSecBufferDesc(&outbuf_desc, &outbuf, 1);
	outMessage.ulVersion	= SECBUFFER_VERSION;
	outMessage.cBuffers	= 1;
	outMessage.pBuffers	= outBuffers;
	reqFlags = gCFG->reqFlags;

	scRet = gCFG->pSecFun->InitializeSecurityContext( &gCFG->cred_hnd, NULL, NULL, reqFlags, 0, 0,
		(gCFG->isALPN ? &inMessage : NULL), 0, &ssl, &outMessage, &ansFlags, &tsExp);

	switch ( scRet )
	{
	case SEC_I_CONTINUE_NEEDED:
		//The client must send the output token to the server and wait for a return token. The returned token is then passed in another call to InitializeSecurityContext (Schannel). The output token can be empty.
		bio_out_buf.grant(outBuf.cbBuffer);
		memcpy(bio_out_buf.point, outBuffers[0].pvBuffer, outBuffers[0].cbBuffer);
		bio_out_buf.commit_ack(outBuffers[0].cbBuffer);
		//printf("------clasp(start) SEC_I_CONTINUE_NEEDED cbBuffer %d\n", outBuffers[0].cbBuffer);
		has_ciph = true;
		ret = 0;
		gCFG->pSecFun->FreeContextBuffer(outBuffers[0].pvBuffer);
		break;

	default:
		disp_err("InitializeSecurityContext(0)", scRet);
		ret = -1;
		break;
	}
	shake_st = 1;
	return ret;	/* END OF 0 */

 SHAKE_ST_Doing:
//InitSecBuffer(&inbuf[0], SECBUFFER_TOKEN, malloc(BACKEND->encdata_offset), curlx_uztoul(BACKEND->encdata_offset));
//InitSecBuffer(&inbuf[1], SECBUFFER_EMPTY, NULL, 0);
	//printf("clasp doing...\n");
	inBuffers[0].BufferType	= SECBUFFER_TOKEN;
	inBuffers[0].pvBuffer	= bio_in_buf.base;
	inBuffers[0].cbBuffer	= (unsigned long ) (bio_in_buf.point - bio_in_buf.base);
	inBuffers[1].BufferType = SECBUFFER_EMPTY;
	inBuffers[1].pvBuffer	= NULL;
	inBuffers[1].cbBuffer	= 0;

//    InitSecBufferDesc(&inbuf_desc, inbuf, 2);
	inMessage.ulVersion	= SECBUFFER_VERSION;
	inMessage.cBuffers	= 2;
	inMessage.pBuffers	= inBuffers;

    /* setup output buffers */
    //InitSecBuffer(&outbuf[0], SECBUFFER_TOKEN, NULL, 0);
    //InitSecBuffer(&outbuf[1], SECBUFFER_ALERT, NULL, 0);
    //InitSecBuffer(&outbuf[2], SECBUFFER_EMPTY, NULL, 0);
    //InitSecBufferDesc(&outbuf_desc, outbuf, 3);

	outBuffers[0].BufferType= SECBUFFER_TOKEN;
	outBuffers[0].pvBuffer	= NULL;
	outBuffers[0].cbBuffer	= 0;

	outBuffers[1].BufferType= SECBUFFER_ALERT;
	outBuffers[1].pvBuffer	= NULL;
	outBuffers[1].cbBuffer	= 0;

	outBuffers[2].BufferType= SECBUFFER_EMPTY;
	outBuffers[2].pvBuffer	= NULL;
	outBuffers[2].cbBuffer	= 0;

	outMessage.ulVersion	= SECBUFFER_VERSION;
	outMessage.cBuffers	= 3;
	outMessage.pBuffers	= outBuffers;

//sspi_status = s_pSecFn->InitializeSecurityContext( &BACKEND->cred->cred_handle, &BACKEND->ctxt->ctxt_handle, host_name, BACKEND->req_flags, 0, 0, &inbuf_desc, 0, NULL, &outbuf_desc, &BACKEND->ret_flags, &BACKEND->ctxt->time_stamp);
	scRet = gCFG->pSecFun->InitializeSecurityContext( &gCFG->cred_hnd, &ssl, NULL, reqFlags, 0, 0,
		&inMessage, 0, NULL, &outMessage, &ansFlags, &tsExp);
	
	switch ( scRet )
	{
	case SEC_I_CONTINUE_NEEDED:
		//printf("Doing SEC_I_CONTINUE_NEEDED\n");
		ret = 0;
		//The client must send the output token to the server and wait for a return token. The returned token is then passed in another call to InitializeSecurityContext (Schannel). The output token can be empty.
#ifndef NDEBUG
		TEXTUS_SPRINTF(errMsg, "InitializeSecurityContext(1) return SEC_I_CONTINUE_NEEDED:  send the output token to the server and wait for a return token");
		goto OK_Pro;
#endif
	case SEC_E_OK:
		//printf("Doing SEC_E_OK\n");
		shake_st = 2;	/*  handshake is complete */
		ret = 1;
		//Curl_verify_certificate
  		/* if the answered attributes */
		if( ansFlags != reqFlags) 
		{
			ret = -1;
			err_lev = 3;
			if(!(ansFlags & ISC_RET_STREAM))
				TEXTUS_SPRINTF(errMsg, "InitializeSecurityContext(2) error: failed to setup stream orientation (ISC_RET_STREAM)");

			if(!(ansFlags & ISC_RET_REPLAY_DETECT))
				TEXTUS_SPRINTF(errMsg, "InitializeSecurityContext(2) error: failed to setup replay detection (ISC_RET_REPLAY_DETECT)");

			if(!(ansFlags & ISC_RET_SEQUENCE_DETECT))
				TEXTUS_SPRINTF(errMsg, "InitializeSecurityContext(2) error: failed to setup sequence detection (ISC_RET_SEQUENCE_DETECT)");

			if(!(ansFlags & ISC_RET_ALLOCATED_MEMORY))
				TEXTUS_SPRINTF(errMsg, "InitializeSecurityContext(2) error: failed to setup memory allocation (ISC_RET_ALLOCATED_MEMORY)");

			if(!(ansFlags & ISC_RET_CONFIDENTIALITY))
				TEXTUS_SPRINTF(errMsg, "InitializeSecurityContext(2) error: failed to setup confidentiality (ISC_RET_CONFIDENTIALITY)");

			goto END_OF_Doing;
		}
#ifndef NDEBUG
		TEXTUS_SPRINTF(errMsg, "InitializeSecurityContext(1) return SEC_E_OK");
		err_lev = 7;
	OK_Pro:
#endif
		for(int i = 0; i < 3; i++) 
		{
  		      /* look  for tokens to be sent */
			if(outBuffers[i].BufferType == SECBUFFER_TOKEN && outBuffers[i].cbBuffer > 0) 
			{
				bio_out_buf.input((unsigned char*)outBuffers[i].pvBuffer, (unsigned TEXTUS_LONG)outBuffers[i].cbBuffer);
				has_ciph = true;
          			gCFG->pSecFun->FreeContextBuffer(outBuffers[i].pvBuffer);
			}
		}
		break;

	case SEC_E_INCOMPLETE_MESSAGE:
		//Data for the whole message was not read from the wire.
		//printf("Doing  SEC_E_INCOMPLETE_MESSAGE\n");
		ret = 0;
		disp_err("InitializeSecurityContext(1)", scRet, ret);
		err_lev = 5;	/* reading ? */
		break;

	case SEC_I_INCOMPLETE_CREDENTIALS:
		//printf("Doing  SEC_I_INCOMPLETE_CREDENTIALS\n");
		//The server has requested client authentication, and the supplied credentials either do not include a certificate or the certificate was not issued by a certification authority (CA) that is trusted by the server. For more information, see Remarks.
		ret = 0;
		disp_err("InitializeSecurityContext(1)", scRet, ret);
		err_lev = 5;
		if ( ! (reqFlags & ISC_REQ_USE_SUPPLIED_CREDS) ) {
			reqFlags |= ISC_REQ_USE_SUPPLIED_CREDS;	/* writing? */
		}
		break;

	default:
		ret = -1;
		disp_err("InitializeSecurityContext(1)", scRet);
		goto END_OF_Doing;
		break;
	}

	if ( inBuffers[1].BufferType == SECBUFFER_EXTRA && inBuffers[1].cbBuffer > 0 )
	{
		//printf("Doing   SECBUFFER_EXTRA\n");
		bio_in_buf.commit(-(TEXTUS_LONG)(( (bio_in_buf.point - bio_in_buf.base) - inBuffers[1].cbBuffer)));
		if ( scRet == SEC_I_CONTINUE_NEEDED )
			goto SHAKE_ST_Doing;	/* should process the data immediately, for server may be done */
	} else {
		bio_in_buf.reset();
	}
	if ( shake_st == 2 )	/*  handshake is complete */
	{
		goto SHAKE_ST_Done;
	}
END_OF_Doing:
	return ret;	/* END OF 1 */

 SHAKE_ST_Done:
	// if (gCFG->isALPN ) { } 

	// scRet = gCFG->pSecFun->QueryContextAttributes(&ssl, SECPKG_ATTR_REMOTE_CERT_CONTEXT, &ccert_context);
/*
    if((scRet != SEC_E_OK) || (ccert_context == NULL)) {
      failf(data, "schannel: failed to retrieve remote cert context");
      return CURLE_PEER_FAILED_VERIFICATION;
    }
*/
	scRet = gCFG->pSecFun->QueryContextAttributes(&ssl, SECPKG_ATTR_STREAM_SIZES, &outSize);
	if( scRet != SEC_E_OK) {
		ret = -1;
		disp_err("QueryContextAttributes for SHAKE_ST_Done", scRet);
		goto END_OF_2;
	}

	handshake_ok = true;
	shake_st = 3;
	ret = 1;
END_OF_2:
	return ret;

#elif defined(__APPLE__)

#else
	int err_ret;
	int how;
	
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
		ret = 0 ;
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
	outwbio( has_ciph);
#endif
	return ret; 	//回去了, 反正还没有handshake结束
}

void SSLcli::ssl_down(bool &has_ciph)
{
#ifdef USE_WINDOWS_SSPI

#elif defined(__APPLE__)

#else
	int ret, how;

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
#endif
	outwbio( has_ciph ); /* 输出密文 */
}

void SSLcli::outwbio(bool &has_ciph)
{
#ifdef USE_WINDOWS_SSPI

#elif defined(__APPLE__)

#else
	int ret,how ;
	int len;

	bio_out_buf.grant(8192);
	while ( (len = BIO_read(wbio, bio_out_buf.point, 8192)) > 0 ) 
	{			
		has_ciph = true;
		bio_out_buf.commit(len);  /* 指针向后移 */
	}
	
#endif

	return ;
}

void SSLcli::novo()
{
#ifdef USE_WINDOWS_SSPI

#elif defined(__APPLE__)

#else
	assert ( !ssl );
	if (!gCFG->ssl_ctx ) return;

	ssl = SSL_new( gCFG->ssl_ctx );
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

#endif

	return ;
}

void SSLcli::endssl()
{
#ifdef USE_WINDOWS_SSPI
	shake_st = 0;

#elif defined(__APPLE__)

#else
	if ( ssl ) SSL_free(ssl);
	ssl = 0;
	rbio = 0;
	wbio = 0;
#endif
	handshake_ok = false;
	
	return;
}

/* 父实例才执行本函数 */
bool SSLcli::initio()
{
	err_lev = -1;

#ifdef USE_WINDOWS_SSPI
	/*
	SCHANNEL_CRED   auth = { 0 };
	auth.dwVersion = SCHANNEL_CRED_VERSION;
	if (pCertContext)
	{
		auth.cCreds = 1;
		auth.paCred = &pCertContext;
	}
	auth.grbitEnabledProtocols = SP_PROT_TLS1_2_CLIENT;
	auth.dwFlags = SCH_CRED_MANUAL_CRED_VALIDATION | SCH_CRED_NO_DEFAULT_CREDS | SCH_USE_STRONG_CRYPTO;
	*/


	SECURITY_STATUS status;
	TimeStamp       expiry;
	typedef PSecurityFunctionTable (APIENTRY *FACE_FUN)(VOID);
	FACE_FUN pface =NULL;
	if ( !gCFG ) return false;
	gCFG->secdll = TEXTUS_LOAD_MOD( gCFG->secdll_fn, 0);
    	if(!gCFG->secdll) {
		TEXTUS_SPRINTF(errMsg, "load %s failed, error %d", gCFG->secdll_fn, GetLastError());
		err_lev = 3;
		return false;
	}
	TEXTUS_GET_ADDR (gCFG->secdll, gCFG->secface_fn, pface, FACE_FUN);

    	if(!pface) {
		TEXTUS_SPRINTF(errMsg, "function %s not found load , error %d", gCFG->secface_fn, GetLastError());
		err_lev = 3;
		return false;
	}

	gCFG->pSecFun = pface();
    	if(!gCFG->pSecFun) {
		TEXTUS_SPRINTF(errMsg, "pInitSecurityInterface return NULL");
		err_lev = 3;
		return false;
	}
	
	status = gCFG->pSecFun->AcquireCredentialsHandle(NULL, gCFG->provider,
                                         SECPKG_CRED_OUTBOUND, NULL,
                                         NULL, NULL, NULL,
                                         &gCFG->cred_hnd, &expiry);
	if (status != SEC_E_OK)
	{
		disp_err("AcquireCredentialsHandle", status );
		return false;
	}

#elif defined(__APPLE__)

#else
	SSL_library_init();
	SSL_load_error_strings();

	if ( !gCFG->ssl_ctx )
		gCFG->ssl_ctx = SSL_CTX_new(SSLv23_client_method());

	if ( !gCFG->ssl_ctx ) return false;
	if ( !gCFG ) return true;

	if ( gCFG->isVpeer && 
	 	SSL_CTX_load_verify_locations( gCFG->ssl_ctx, (strlen(gCFG->ca_cert_file) > 0 ? gCFG->ca_cert_file:NULL), 
			(strlen(gCFG->capath) > 0 ? gCFG->capath:NULL) ) != 1 )
	{
		int err = ERR_get_error();
		TEXTUS_SPRINTF(errMsg, "initio() SSL load_verify_locations %s", ERR_error_string(err, (char *)NULL));
		err_lev = 3;
		endctx();
		return false;
	}
			
	if ( gCFG->isVpeer )
		SSL_CTX_set_verify( gCFG->ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

	if ( strlen(gCFG->my_cert_file) > 0 ) 
	if ( SSL_CTX_use_certificate_file( gCFG->ssl_ctx,  gCFG->my_cert_file, SSL_FILETYPE_PEM ) == 0 )
	{
		int err = ERR_get_error();
		TEXTUS_SPRINTF(errMsg, "initio() SSL use_certificate %s", ERR_error_string(err, (char *)NULL));
		err_lev = 3;
		endctx();
		return false;
	}

	if ( strlen(gCFG->my_key_file) > 0 ) 
	if ( SSL_CTX_use_PrivateKey_file( gCFG->ssl_ctx,  gCFG->my_key_file, SSL_FILETYPE_PEM) == 0 
		
		|| SSL_CTX_check_private_key( gCFG->ssl_ctx ) == 0 )
	{
		int err = ERR_get_error();
		TEXTUS_SPRINTF(errMsg,"initio() SSL private key %s", ERR_error_string(err, (char *)NULL) );
		err_lev = 3;
		endctx();
		return false;
	}

#endif
	matris = true;
	return true;
}

void SSLcli::endctx()
{
#ifdef USE_WINDOWS_SSPI

#elif defined(__APPLE__)

#else
	if ( gCFG->ssl_ctx )
	{
		SSL_CTX_free(gCFG->ssl_ctx);
		ERR_free_strings();

		gCFG->ssl_ctx = 0;
	}
#endif
	return ;
}

void SSLcli::herit(SSLcli *child)
{
	if ( !child )  return ;
	child->gCFG = gCFG;
	return ;
}

#ifdef USE_WINDOWS_SSPI
void SSLcli::disp_err(const char* fun_str, SECURITY_STATUS status, int ret )
{
	DWORD dw = GetLastError();
	err_lev = 3;
     	switch(status) {
	case SEC_E_WRONG_PRINCIPAL:
		TEXTUS_SPRINTF(errMsg, "%s error:  SNI or certificate check failed:", fun_str);
		break;
	case SEC_E_INSUFFICIENT_MEMORY:
		TEXTUS_SPRINTF(errMsg, "%s error: There is not enough memory available to complete the requested action", fun_str);
		break;
	case SEC_E_NO_CREDENTIALS:
		TEXTUS_SPRINTF(errMsg, "%s error: No credentials are available in the constrained delegation.", fun_str);
		break;
	case SEC_E_SECPKG_NOT_FOUND:
		TEXTUS_SPRINTF(errMsg, "%s The requested security package does not exist.", fun_str);
		break;
	case SEC_E_NOT_OWNER:
		TEXTUS_SPRINTF(errMsg, "%s The caller of the function does not have the necessary credentials.", fun_str);
		break;
	case SEC_E_UNKNOWN_CREDENTIALS:
		TEXTUS_SPRINTF(errMsg, "'Unknown Credentials' returned by %s, error %d", fun_str, dw);
		break;
	case SEC_E_INTERNAL_ERROR:
		TEXTUS_SPRINTF(errMsg, "%s An error occurred that did not map to an SSPI error code.", fun_str);
		break;
	 case SEC_E_BUFFER_TOO_SMALL:
		TEXTUS_SPRINTF(errMsg, "%s The output buffer is too small.", fun_str); 
		break;
	case SEC_E_CONTEXT_EXPIRED:
		TEXTUS_SPRINTF(errMsg, "%s The application is referencing a context that has already been closed.", fun_str);
		break;
	case SEC_E_CRYPTO_SYSTEM_INVALID:
		TEXTUS_SPRINTF(errMsg, "%s The cipher chosen for the security context is not supported.", fun_str);
		break;
	case SEC_E_INVALID_HANDLE:
		TEXTUS_SPRINTF(errMsg, "%s A context handle that is not valid was specified in the phContext parameter.", fun_str);
		break;
	case SEC_E_INVALID_TOKEN:
		TEXTUS_SPRINTF(errMsg, "%s No SECBUFFER_DATA type buffer was found.", fun_str);
		break;
	case SEC_E_QOP_NOT_SUPPORTED:
		TEXTUS_SPRINTF(errMsg, "%s Neither confidentiality nor integrity are supported by the security context", fun_str);
		break;

        case SEC_E_LOGON_DENIED:
		TEXTUS_SPRINTF(errMsg, "%s error: No credentials are available in the constrained delegation.", fun_str);
		break;
        case SEC_E_TARGET_UNKNOWN:
		TEXTUS_SPRINTF(errMsg, "%s error: The target was not recognized.", fun_str);
		break;
        case SEC_E_NO_AUTHENTICATING_AUTHORITY:
		TEXTUS_SPRINTF(errMsg, "%s error: No authority could be contacted for authentication. The domain name of the authenticating party could be wrong, the domain could be unreachable, or there might have been a trust relationship failure.", fun_str);
		break;
        case SEC_E_UNSUPPORTED_FUNCTION:
		TEXTUS_SPRINTF(errMsg, "%s error: A context attribute flag that is not valid (ISC_REQ_DELEGATE or ISC_REQ_PROMPT_FOR_CREDS) was specified in the fContextReq parameter.", fun_str);
		break;
        case SEC_E_APPLICATION_PROTOCOL_MISMATCH:
		TEXTUS_SPRINTF(errMsg, "%s error: No common application protocol exists between the client and the server.", fun_str);
		break;
        case SEC_E_UNTRUSTED_ROOT:
		TEXTUS_SPRINTF(errMsg, "%s error: return SEC_E_UNTRUSTED_ROOT", fun_str);
		break;

	case SEC_I_INCOMPLETE_CREDENTIALS:
		//The server has requested client authentication, and the supplied credentials either do not include a certificate or the certificate was not issued by a certification authority (CA) that is trusted by the server. For more information, see Remarks.
		switch ( ret ) {
		case 0:
			TEXTUS_SPRINTF(errMsg, "%s return SEC_I_INCOMPLETE_CREDENTIALS.  The server has requested client authentication, and the supplied credentials either do not include a certificate or the certificate was not issued by a certification authority (CA) that is trusted by the server.", fun_str);
			err_lev = 5;
			break;
		case -1:
			TEXTUS_SPRINTF(errMsg, "%s error: return SEC_I_INCOMPLETE_CREDENTIALS", fun_str);
			err_lev = 3;
			break;
		default:
			break;
		}
		break;

	case SEC_E_INCOMPLETE_MESSAGE:
		//Data for the whole message was not read from the wire.
		switch ( ret ) {
		case 0:
			TEXTUS_SPRINTF(errMsg, "%s return SEC_E_INCOMPLETE_MESSAGE.  Data for the whole message was not read from the wire", fun_str);
			err_lev = 5;
			break;

		case -1:
			TEXTUS_SPRINTF(errMsg, "%s error: return SEC_E_INCOMPLETE_MESSAGE", fun_str);
			err_lev = 3;
			break;
		default:
			break;
		}
		break;

	case SEC_I_COMPLETE_AND_CONTINUE: 
		//The client must call CompleteAuthToken and then pass the output to the server. The client then waits for a returned token and passes it, in another call, to InitializeSecurityContext (Schannel).
		switch ( ret ) {
		case 0:
			TEXTUS_SPRINTF(errMsg, "%s return SEC_I_COMPLETE_AND_CONTINUE. The client must call CompleteAuthToken and then pass the output to the server. The client then waits for a returned token and passes it, in another call, to InitializeSecurityContext (Schannel).", fun_str);
			err_lev = 5;
			break;

		case -1:
			TEXTUS_SPRINTF(errMsg, "%s error: return SEC_I_COMPLETE_AND_CONTINUE", fun_str);
			err_lev = 3;
			break;
		default:
			break;
		}
		break;

	case SEC_I_COMPLETE_NEEDED:
		//The client must finish building the message and then call the CompleteAuthToken function.
		switch ( ret ) {
		case 0:
			TEXTUS_SPRINTF(errMsg, "%s return SEC_I_COMPLETE_NEEDED. The client must finish building the message and then call the CompleteAuthToken function.", fun_str);
			err_lev = 5;
			break;

		case -1:
			TEXTUS_SPRINTF(errMsg, "%s error: return SEC_I_COMPLETE_NEEDED", fun_str);
			err_lev = 3;
			break;
		default:
			break;
		}
		break;

	case SEC_E_OK:
		switch ( ret ) {
		case 0:
			TEXTUS_SPRINTF(errMsg, "%s return SEC_E_OK",  fun_str);
			err_lev = 7;
			break;

		case -1:
			TEXTUS_SPRINTF(errMsg, "%s error: return SEC_E_OK",  fun_str);
			err_lev = 3;
			break;
		default:
			break;
		}
		break;
	
/*
        case :
		TEXTUS_SPRINTF(errMsg, "%s error: ", fun_str);
		break;
*/
	default:
		TEXTUS_SPRINTF(errMsg, "status=0x%x returned by %s, error = %d", status, fun_str, dw);
		break;
      	}
}

#endif
