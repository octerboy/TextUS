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
#include "casecmp.h"
#include <assert.h>

#define BZERO(X) memset(X, 0 ,sizeof(X))
#define SND_LEN (snd_buf->point - snd_buf->base)
#define RCV_LEN (rcv_buf->point - rcv_buf->base)
#ifdef USE_WINDOWS_SSPI

static int proto_id(char *name)
{
#define GetProtID(X)                         \
  if(strcasecmp(name, #X) == 0)                      \
	{	printf("------------proto %s\n", "SP_PROT_" #X "_SERVER");			\
	return SP_PROT_##X##_SERVER;	\
	}

	GetProtID(PCT1);
	GetProtID(SSL2);
	GetProtID(SSL3);
	GetProtID(TLS1);
	GetProtID(TLS1_0);
	GetProtID(TLS1_1);
	GetProtID(TLS1_2);
	GetProtID(DTLS);
	GetProtID(DTLS1_0);
	GetProtID(DTLS1_2);
	GetProtID(DTLS1_X);
	printf("no such proto!!!!!!!!!\n");
	return 0;
}

#include "sspialg.c"
#endif

SSLsrv::SSLsrv()
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
	rcv_buf = new TBuffer(8192); //明文, 收到的数据
	snd_buf = new TBuffer(8192); //明文, 要发出的数据

	bio_in_buf = 0;
	bio_out_buf = 0;

	matris = false;
	gCFG = 0 ;
}

SSLsrv::~SSLsrv()
{
	if ( matris)	//父实例才销
		endctx();
}

int SSLsrv::encrypt(bool &has_ciph)
{
	int ret = -1;
	int len;
	err_lev = -1;

#ifdef USE_WINDOWS_SSPI
	SECURITY_STATUS	scRet;
	size_t data_len = 0;

#elif defined(__APPLE__)

#else
	int how, err;
	int puttedLen;

	if ( !ssl) return false;
#endif
	/* 灌明文,从snd_buf取出数据 */	
	len = static_cast<int> SND_LEN;
	has_ciph=false;
	if (!handshake_ok)
	{	//没有握手就握手
		return -1;
	}

#ifdef USE_WINDOWS_SSPI

	/* setup output buffers (header, data, trailer, empty) */
	printf("cb %d len %d trail %d\n", outSize.cbHeader, len , outSize.cbTrailer);
	data_len = outSize.cbHeader + len + outSize.cbTrailer;
	bio_out_buf->grant(data_len);
	outBuffers[0].BufferType= SECBUFFER_STREAM_HEADER;
	outBuffers[0].pvBuffer	= bio_out_buf->point;
	outBuffers[0].cbBuffer	= outSize.cbHeader;

	outBuffers[1].BufferType= SECBUFFER_DATA;
	outBuffers[1].pvBuffer	= bio_out_buf->point + outSize.cbHeader;
	outBuffers[1].cbBuffer	= len;
	memcpy( outBuffers[1].pvBuffer, snd_buf->base, len);
	//for ( int kk = 0 ; kk < len ; kk++) printf("%02x ", ((unsigned char*)outBuffers[1].pvBuffer)[kk]); 

	outBuffers[2].BufferType= SECBUFFER_STREAM_TRAILER;
	outBuffers[2].pvBuffer	= bio_out_buf->point + outSize.cbHeader + len;
	outBuffers[2].cbBuffer	= outSize.cbTrailer;

	unsigned char sig[64];
	memset(sig, 0, 64);
	outBuffers[3].BufferType= SECBUFFER_EMPTY;
	outBuffers[3].pvBuffer	= NULL;
	outBuffers[3].cbBuffer	= 0;
	outBuffers[4].BufferType= SECBUFFER_EMPTY;
	outBuffers[4].pvBuffer	= NULL;
	outBuffers[4].cbBuffer	= 0;
	/*
	outBuffers[3].BufferType= SECBUFFER_PADDING;
	outBuffers[3].pvBuffer	= sig;
	outBuffers[3].cbBuffer	= 36;
	*/

	outMessage.ulVersion	= SECBUFFER_VERSION;
	outMessage.cBuffers	= 4;
	outMessage.pBuffers	= outBuffers;

	SecPkgContext_Sizes mz;
	memset(&mz,0,sizeof(mz));
	scRet = gCFG->pSecFun->QueryContextAttributes(&ssl, SECPKG_ATTR_SIZES, &mz);
	printf("----  cbMaxToken %d, cbMaxSignature %d, cbBlockSize %d, cbSecurityTrailer %d\n", mz.cbMaxToken, mz.cbMaxSignature, mz.cbBlockSize, mz.cbSecurityTrailer);
	scRet = gCFG->pSecFun->EncryptMessage(&ssl, 0, &outMessage, 0); 
	//for ( int kk=0; kk < 36; kk++ ) printf("%02X ", sig[kk]);
	if (  scRet != SEC_E_OK )
		goto ErrorPro;

	snd_buf->commit_ack(-len);	/* 原明文清空 */
	bio_out_buf->commit_ack(data_len);  /* 密文指针向后移 */
	has_ciph = true;
#elif defined(__APPLE__)

#else
	puttedLen = SSL_write( ssl, snd_buf->base, len);
	if ( puttedLen < 0 )
		goto ErrorPro;
	
	snd_buf->commit(-puttedLen);	/* 提交已取出的数据 */
	if ( puttedLen != len)
	{	/* 如果没有取出全部数据, 奇怪! */
		TEXTUS_SPRINTF(errMsg, "encrypt BIO_write %d, left %d",len, len-puttedLen);
		err_lev = 5;
	}

	outwbio( has_ciph); /* 输出密文 */
#endif
	return true;

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
		err_lev = 5;
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

int SSLsrv::decrypt(bool &has_plain, bool &has_ciph)
{
	int ret;	/* 最后返回值 */
	int len;

	err_lev = -1;
	has_ciph = false;
	has_plain = false;

	/* 灌密文, 从bio_in_buf取出数据 */	
	assert(bio_in_buf);
	len = static_cast<int>(bio_in_buf->point - bio_in_buf->base);

#ifdef USE_WINDOWS_SSPI
	SECURITY_STATUS	scRet;

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
	inBuffers[0].BufferType	= SECBUFFER_DATA;
	inBuffers[0].pvBuffer	= bio_in_buf->base;
	inBuffers[0].cbBuffer	= len;
	inBuffers[1].BufferType = SECBUFFER_EMPTY;
	inBuffers[1].pvBuffer	= NULL;
	inBuffers[1].cbBuffer	= 0;
	inBuffers[2].BufferType = SECBUFFER_EMPTY;
	inBuffers[2].pvBuffer	= NULL;
	inBuffers[2].cbBuffer	= 0;
	inBuffers[3].BufferType = SECBUFFER_EMPTY;
	inBuffers[3].pvBuffer	= NULL;
	inBuffers[3].cbBuffer	= 0;
	inBuffers[4].BufferType = SECBUFFER_EMPTY;
	inBuffers[4].pvBuffer	= NULL;
	inBuffers[4].cbBuffer	= 0;

	inMessage.ulVersion	= SECBUFFER_VERSION;
	inMessage.cBuffers	= 5;
	inMessage.pBuffers	= inBuffers;

	if ( bio_in_buf->base[0] == 21 ) // Alert message type
	{
		const int shutLen = 26, headLen = 5;
		unsigned char *p = bio_in_buf->base;
		if ( len > (headLen + shutLen) // Could be a shutdown message followed by something else
			&& p[3] == 0 && p[4] == shutLen // the first message is the correct length for a shutdown message
			&& p[5] == 0 // it is a "close notify" (aka shutdown) message
		) {
		//DebugMsg("Looks like a concatenated shutdown message and something else");
			printf("---------Looks like a concatenated shutdown message and something else\n");
		//PrintHexDump(readBufferBytes, readPtr, true);
			inBuffers[0].cbBuffer = shutLen + headLen;
			scRet = gCFG->pSecFun->DecryptMessage(&ssl, &inMessage, 0, NULL);
			if (scRet == SEC_I_CONTEXT_EXPIRED)
			{
				// the unprocessed data in Message.pBuffers[1]
				inBuffers[3].BufferType = SECBUFFER_EXTRA;
				inBuffers[3].pvBuffer = p + headLen + shutLen;
				inBuffers[3].cbBuffer = len - headLen - shutLen;
			}
			goto WHAT_RET;
		}
	}
	scRet = gCFG->pSecFun->DecryptMessage(&ssl, &inMessage, 0, NULL);
WHAT_RET:
	switch ( scRet ) {
	case SEC_I_RENEGOTIATE:
	case SEC_I_CONTEXT_EXPIRED: 
	case SEC_E_OK:
		ret = 1;
		if (inBuffers[1].BufferType == SECBUFFER_DATA)
		{
			rcv_buf->input((unsigned char*)inBuffers[1].pvBuffer, (unsigned TEXTUS_LONG) inBuffers[1].cbBuffer);
			has_plain = true;
		}
		if (inBuffers[3].BufferType == SECBUFFER_EXTRA)
		{
			bio_in_buf->commit(-((TEXTUS_LONG)(len - inBuffers[3].cbBuffer)));
			TEXTUS_SPRINTF(errMsg, "DecryptMessage %d, left %d", len, inBuffers[3].cbBuffer);
			err_lev = 5;
			ret = 2;
		} else {
			err_lev = -1;
			bio_in_buf->reset();
		}
		switch ( scRet ) {
		case SEC_I_RENEGOTIATE:
			TEXTUS_SPRINTF(errMsg, "DecryptMessage %s", "remote party requests renegotiation");
			handshake_ok = false;
			err_lev = 5;
			break;

		case SEC_I_CONTEXT_EXPIRED: 
			TEXTUS_SPRINTF(errMsg, "DecryptMessage %s", "remote party closed the connection");
			err_lev = 5;
			ret = 0;
			break;
		}

		break;
	
	case SEC_E_INCOMPLETE_MESSAGE:
		ret = 1;
		disp_err("DecryptMessage (E_INC)", scRet,0);
		break;

	default:	/* other error */
		ret = -1;
		printf("scRet %08x --\n", scRet);
		disp_err("DecryptMessage (OTHER)", scRet);
		break;
	}
	if ( ret == 2 ) { 
		len = static_cast<int>(bio_in_buf->point - bio_in_buf->base);
		goto D_AGAIN;
	}
#elif defined(__APPLE__)

#else
	int how, err;
	int reqLen;
	int puttedLen;
	if (!ssl) novo();
	if (!ssl ) return -1;		//出现严重错误
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
		ret = clasp( has_ciph );
		if ( ret < 0 )
			return ret;
		else if ( ret == 0 )
			return 1;
	}

	/* 现在开始试图读取明文 */
	rcv_buf->grant(8192);   //保证有足够空间
	ret =1;
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
		err_lev = 5;
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

int SSLsrv:: clasp( bool &has_ciph)
{
	int ret;
	
	err_lev = -1;
	ret = 0;

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
	inBuffers[0].BufferType = SECBUFFER_TOKEN;
	inBuffers[0].pvBuffer	= bio_in_buf->base;
	inBuffers[0].cbBuffer	= (unsigned long ) (bio_in_buf->point - bio_in_buf->base);

	inBuffers[1].BufferType = SECBUFFER_EMPTY;
	inBuffers[1].pvBuffer	= NULL;
	inBuffers[1].cbBuffer	= 0;

	inMessage.ulVersion	= SECBUFFER_VERSION;
	inMessage.cBuffers	= 2;
	inMessage.pBuffers	= inBuffers;

	outBuffers[0].BufferType= SECBUFFER_TOKEN;
	outBuffers[0].pvBuffer	= NULL;
	outBuffers[0].cbBuffer	= 0;

	outMessage.ulVersion	= SECBUFFER_VERSION;
	outMessage.cBuffers	= 1;
	outMessage.pBuffers	= outBuffers;
	reqFlags = gCFG->reqFlags;

	scRet = gCFG->pSecFun->AcceptSecurityContext( &gCFG->cred_hnd, NULL,  &inMessage, reqFlags, 0, 
		&ssl, &outMessage, &ansFlags, &tsExp);

	switch ( scRet )
	{
	case SEC_I_CONTINUE_NEEDED:
		//The client must send the output token to the server and wait for a return token. The returned token is then passed in another call to AcceptSecurityContext (Schannel). The output token can be empty.
		bio_out_buf->grant(outBuffers[0].cbBuffer);
		memcpy(bio_out_buf->point, outBuffers[0].pvBuffer, outBuffers[0].cbBuffer);
		bio_out_buf->commit_ack(outBuffers[0].cbBuffer);
		//printf("------clasp(start) SEC_I_CONTINUE_NEEDED cbBuffer %d\n", outBuffers[0].cbBuffer);
		has_ciph = true;
		ret = 0;
		gCFG->pSecFun->FreeContextBuffer(outBuffers[0].pvBuffer);
		break;

	default:
		disp_err("AcceptSecurityContext(0)", scRet);
		ret = -1;
		break;
	}
	shake_st = 1;
	bio_in_buf->reset();
	return ret;	/* END OF 0 */

 SHAKE_ST_Doing:
	inBuffers[0].BufferType	= SECBUFFER_TOKEN;
	inBuffers[0].pvBuffer	= bio_in_buf->base;
	inBuffers[0].cbBuffer	= (unsigned long) (bio_in_buf->point - bio_in_buf->base);
	inBuffers[1].BufferType = SECBUFFER_EMPTY;
	inBuffers[1].pvBuffer	= NULL;
	inBuffers[1].cbBuffer	= 0;

	inMessage.ulVersion	= SECBUFFER_VERSION;
	inMessage.cBuffers	= 2;
	inMessage.pBuffers	= inBuffers;

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
	outMessage.cBuffers	= 1;
	outMessage.pBuffers	= outBuffers;

	reqFlags = gCFG->reqFlags ;
	scRet = gCFG->pSecFun->AcceptSecurityContext( &gCFG->cred_hnd, &ssl,  &inMessage, reqFlags, 0, 
		NULL, &outMessage, &ansFlags, &tsExp);

	switch ( scRet )
	{
	case SEC_I_CONTINUE_NEEDED:
		printf("--------- sec_i_continue \n");
		ret = 0;
		//The client must send the output token to the server and wait for a return token. The returned token is then passed in another call to AcceptSecurityContext (Schannel). The output token can be empty.
#ifndef NDEBUG
		TEXTUS_SPRINTF(errMsg, "AcceptSecurityContext(1) return SEC_I_CONTINUE_NEEDED:  send the output token to the server and wait for a return token");
		err_lev = 7;
#endif
		goto OK_Pro;
	case SEC_E_OK:
		//printf("--------- sec_ok \n");
		shake_st = 2;	/*  handshake is complete */
		ret = 1;
		if( ansFlags != reqFlags )
		{
			printf("-------ansFlags (%08X) != reqFlagsk (%08X) \n", ansFlags, reqFlags);
			ret = -1;
			err_lev = 3;
			if(!(ansFlags & ASC_RET_STREAM)) {
				TEXTUS_SPRINTF(errMsg, "AcceptSecurityContext(2) error: failed to setup stream orientation (ASC_RET_STREAM)");
			} else if(!(ansFlags & ASC_RET_REPLAY_DETECT)) {
				TEXTUS_SPRINTF(errMsg, "AcceptSecurityContext(2) error: failed to setup replay detection (ASC_RET_REPLAY_DETECT)");
			} else if(!(ansFlags & ASC_RET_SEQUENCE_DETECT)) {
				TEXTUS_SPRINTF(errMsg, "AcceptSecurityContext(2) error: failed to setup sequence detection (ASC_RET_SEQUENCE_DETECT)");
			} else if(!(ansFlags & ASC_RET_ALLOCATED_MEMORY)) {
				TEXTUS_SPRINTF(errMsg, "AcceptSecurityContext(2) error: failed to setup memory allocation (ASC_RET_ALLOCATED_MEMORY)");
			} else if(!(ansFlags & ASC_RET_CONFIDENTIALITY)) {
				TEXTUS_SPRINTF(errMsg, "AcceptSecurityContext(2) error: failed to setup confidentiality (ASC_RET_CONFIDENTIALITY)");
			} else if(!(ansFlags & ASC_RET_MUTUAL_AUTH )) {
				TEXTUS_SPRINTF(errMsg, "AcceptSecurityContext(2) error: failed to setup mutual_auth (ASC_RET_MUTUAL_AUTH )");
			} else if(!(ansFlags & ASC_RET_EXTENDED_ERROR)) {
				TEXTUS_SPRINTF(errMsg, "AcceptSecurityContext(2) error: failed to setup extended error (ASC_RET_EXTENDED_ERROR)");
				err_lev = 7;
				ret = 1;
				goto OK_Pro;
			} else {
				TEXTUS_SPRINTF(errMsg, "AcceptSecurityContext(2) error: failed to setup other error");
			}
			goto END_OF_Doing;
		}
#ifndef NDEBUG
		TEXTUS_SPRINTF(errMsg, "AcceptSecurityContext(1) return SEC_E_OK");
		err_lev = 7;
#endif
	OK_Pro:
		for(int i = 0; i < 1; i++) 
		{
  		      /* look  for tokens to be sent */
			if(outBuffers[i].BufferType == SECBUFFER_TOKEN && outBuffers[i].cbBuffer > 0) 
			{
				bio_out_buf->input((unsigned char*)outBuffers[i].pvBuffer, (unsigned TEXTUS_LONG)outBuffers[i].cbBuffer);
				has_ciph = true;
          			gCFG->pSecFun->FreeContextBuffer(outBuffers[i].pvBuffer);
			}
		}
		if ( ret == -1 ) goto END_OF_Doing; //may from other error 
		break;

	case SEC_E_INCOMPLETE_MESSAGE:
		//printf("--------- sec_i_inc \n");
		//Data for the whole message was not read from the wire.
		ret = 0;
		disp_err("AcceptSecurityContext(1)", scRet, ret);
		err_lev = 5;	/* reading ? */
		if  (0 != (ansFlags & ASC_RET_EXTENDED_ERROR) ) goto OK_Pro;
		break;

	case SEC_I_INCOMPLETE_CREDENTIALS:
		//printf("--------- sec_i_cre \n");
		//The server has requested client authentication, and the supplied credentials either do not include a certificate or the certificate was not issued by a certification authority (CA) that is trusted by the server. For more information, see Remarks.
		ret = 0;
		disp_err("AcceptSecurityContext(1)", scRet, ret);
		err_lev = 5;
		if ( ! (reqFlags & ISC_REQ_USE_SUPPLIED_CREDS) ) {
			reqFlags |= ISC_REQ_USE_SUPPLIED_CREDS;	/* writing? */
		}
		if  (0 != (ansFlags & ASC_RET_EXTENDED_ERROR) ) goto OK_Pro;
		break;

	default:
		//printf("--------- default err\n");
		disp_err("AcceptSecurityContext(defulat 1)", scRet);
		ret = 1;
		if  (0 != (ansFlags & ASC_RET_EXTENDED_ERROR) ) goto OK_Pro;
		ret = -1;
		goto END_OF_Doing;
		break;
	}

	if ( inBuffers[1].BufferType == SECBUFFER_EXTRA && inBuffers[1].cbBuffer > 0 )
	{
		bio_in_buf->commit(-(TEXTUS_LONG)(( (bio_in_buf->point - bio_in_buf->base) - inBuffers[1].cbBuffer)));
		if ( scRet == SEC_I_CONTINUE_NEEDED )
			goto SHAKE_ST_Doing;	/* should process the data immediately, for server may be done */
	} else {
		bio_in_buf->reset();
	}
	if ( shake_st == 2 )	/*  handshake is complete */
	{
		goto SHAKE_ST_Done;
	}
END_OF_Doing:
	return ret;	/* END OF 1 */

 SHAKE_ST_Done:
	scRet = gCFG->pSecFun->QueryContextAttributes(&ssl, SECPKG_ATTR_STREAM_SIZES, &outSize);
	if( scRet != SEC_E_OK) {
		printf("QueryContextAttributes err\n");
		ret = -1;
		disp_err("QueryContextAttributes for SHAKE_ST_Done", scRet);
		goto END_OF_2;
	}
		printf("QueryContextAttributes ok cbMaximumMessage %d,  cBuffers %d,  cbBlockSize %d\n", outSize.cbMaximumMessage, outSize.cBuffers, outSize.cbBlockSize);
	{
			PCERT_CONTEXT pCertContext = nullptr;
			HRESULT hr =  gCFG->pSecFun->QueryContextAttributes(&ssl, SECPKG_ATTR_REMOTE_CERT_CONTEXT, &pCertContext);
			if ( hr  != SEC_E_OK) {
				printf("----- get SECPKG_ATTR_REMOTE_CERT_CONTEXT failed!\n");
			} else {
				ShowCertInfo(pCertContext, "Textus SSL Server");
			}
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
		ret = 0;
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
	outwbio( has_ciph);
#endif
	return ret; 	//回去了, 反正还没有handshake结束
}

void SSLsrv::ssl_down( bool &has_ciph)
{
#ifdef USE_WINDOWS_SSPI
	DWORD	shut = SCHANNEL_SHUTDOWN;
	DWORD	scRet;

	has_ciph = false;
	err_lev = -1;

	inBuffers[0].pvBuffer = &shut;
	inBuffers[0].BufferType = SECBUFFER_TOKEN;
	inBuffers[0].cbBuffer = sizeof(DWORD);

	inMessage.cBuffers = 1;
	inMessage.pBuffers = inBuffers;
	inMessage.ulVersion = SECBUFFER_VERSION;

	scRet = gCFG->pSecFun->ApplyControlToken(&ssl, &inMessage);
	if ( scRet != SEC_E_OK ) {
		disp_err("ApplyControlToken (ssl_down) ", scRet);
		return ;
	}

	outBuffers[0].pvBuffer = NULL;
	outBuffers[0].BufferType = SECBUFFER_EMPTY;
	outBuffers[0].cbBuffer = 0;

	outMessage.cBuffers = 1;
	outMessage.pBuffers = outBuffers;
	outMessage.ulVersion = SECBUFFER_VERSION;

	reqFlags = gCFG->reqFlags;
	scRet = gCFG->pSecFun->AcceptSecurityContext( &gCFG->cred_hnd, &ssl,  &inMessage, reqFlags, 0, 
		NULL, &outMessage, &ansFlags, &tsExp);
	switch ( scRet )
	{
	case SEC_I_CONTINUE_NEEDED:
		printf("------ssl_down SEC_I_CONTINUE_NEEDED cbBuffer %d\n", outBuffers[0].cbBuffer);
	case SEC_E_OK:
		bio_out_buf->grant(outBuffers[0].cbBuffer);
		memcpy(bio_out_buf->point, outBuffers[0].pvBuffer, outBuffers[0].cbBuffer);
		bio_out_buf->commit_ack(outBuffers[0].cbBuffer);
		//printf("------ssl_down SEC_I_CONTINUE_NEEDED cbBuffer %d\n", outBuffers[0].cbBuffer);
		has_ciph = true;
		gCFG->pSecFun->FreeContextBuffer(outBuffers[0].pvBuffer);
		break;

	default:
		disp_err("AcceptSecurityContext(ssl_down)", scRet);
		break;
	}

#elif defined(__APPLE__)

#else
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
	outwbio( has_ciph); /* 输出密文 */
#endif
}

void SSLsrv::outwbio( bool &has)
{
#ifdef USE_WINDOWS_SSPI

#elif defined(__APPLE__)

#else
	int len;

	bio_out_buf->grant(8192);
	while ( (len = BIO_read(wbio, bio_out_buf->point, 8192)) > 0 ) 
	{			
		has = true;
		bio_out_buf->commit_ack(len);  /* 指针向后移 */
		bio_out_buf->grant(8192);
	}
#endif	
	return ;
}

void SSLsrv::novo()
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
		TEXTUS_SPRINTF(errMsg, "novo() SSL_new %s", ERR_error_string(err, (char *)NULL));
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

void SSLsrv::endssl()
{
#ifdef USE_WINDOWS_SSPI
	shake_st = 0;
	gCFG->pSecFun->DeleteSecurityContext(&ssl);
	memset(&ssl, 0, sizeof(CtxtHandle));
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
bool SSLsrv::initio()
{
	err_lev = -1;

#ifdef USE_WINDOWS_SSPI
	SECURITY_STATUS status;
	TimeStamp       expiry;
	typedef PSecurityFunctionTable (APIENTRY *FACE_FUN)(VOID);
	FACE_FUN pface =NULL;
	HCERTSTORE cStore;
	if ( !gCFG ) return false;

	gCFG->secdll = TEXTUS_LOAD_MOD( gCFG->secdll_fn, 0);
	printf("-------- %s\n",  gCFG->secdll_fn);
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
	memset(&gCFG->schCred, 0, sizeof(gCFG->schCred));
	gCFG->schCred.dwVersion = SCHANNEL_CRED_VERSION;
	set_algs(&gCFG->schCred, gCFG->alg_str, &gCFG->all_algs[0]);
	set_proto(&gCFG->schCred, gCFG->proto_str);
	//set_proto(&gCFG->schCred, "tls1.1 tls1.0");
	//set_algs(&gCFG->schCred, "desx des", &gCFG->all_algs[0]);
	/*
	gCFG->schCred.dwMinimumCipherStrength =0;
	gCFG->schCred.dwMaximumCipherStrength =0;
	gCFG->schCred.dwSessionLifespan =0;
	*/
	gCFG->schCred.grbitEnabledProtocols = SP_PROT_TLS1_2_SERVER;
	//gCFG->schCred.dwCredFormat =SCH_CRED_FORMAT_CERT_HASH;

//	gCFG->reqFlags = ASC_REQ_SEQUENCE_DETECT | ASC_REQ_REPLAY_DETECT | ASC_REQ_CONFIDENTIALITY | //ASC_REQ_EXTENDED_ERROR |
//			ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_STREAM ;
			//ASC_REQ_MANUAL_CRED_VALIDATION | ASC_REQ_STREAM |ASC_REQ_DELEGATE // We'll check the certificate ourselves
	//gCFG->reqFlags = ASC_REQ_CONFIDENTIALITY | ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_SEQUENCE_DETECT |  ASC_REQ_REPLAY_DETECT  | ASC_REQ_STREAM ;
	gCFG->reqFlags =  ASC_REQ_ALLOCATE_MEMORY |ASC_REQ_CONFIDENTIALITY |ASC_REQ_STREAM | ASC_REQ_REPLAY_DETECT  | ASC_REQ_SEQUENCE_DETECT |ASC_REQ_EXTENDED_ERROR | 0 ;
	//gCFG->reqFlags = 0x007FFFFF;
	if ( gCFG->isVpeer )
	{
		printf("--------isVpeer-------\n");
		gCFG->reqFlags |=ASC_REQ_MUTUAL_AUTH;
	//	gCFG->reqFlags |= ASC_REQ_PROMPT_FOR_CREDS;
	/*
		gCFG->schCred.dwFlags = SCH_CRED_AUTO_CRED_VALIDATION | SCH_CRED_NO_DEFAULT_CREDS | SCH_USE_STRONG_CRYPTO;
		if ( gCFG->crl_file[0] == 'y' || gCFG->crl_file[0] == 'Y'  ) 
			gCFG->schCred.dwFlags |= SCH_CRED_REVOCATION_CHECK_CHAIN;
		else
			gCFG->schCred.dwFlags |= SCH_CRED_IGNORE_NO_REVOCATION_CHECK;
	*/
	} else {
		//gCFG->schCred.dwFlags = SCH_CRED_MANUAL_CRED_VALIDATION | SCH_CRED_IGNORE_NO_REVOCATION_CHECK | SCH_CRED_IGNORE_REVOCATION_OFFLINE;
	}
	//gCFG->schCred.dwFlags |= SCH_CRED_NO_SERVERNAME_CHECK;
	gCFG->schCred.dwFlags = SCH_USE_STRONG_CRYPTO;

	PCCERT_CONTEXT prevCtx = NULL;
	PCCERT_CONTEXT ctx = NULL;
	char sub[126];
	CERT_NAME_BLOB *pblob =NULL;
#if 0
	gCFG->schCred.hRootStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, 0, (HCRYPTPROV)NULL,
                        CERT_STORE_OPEN_EXISTING_FLAG | CERT_SYSTEM_STORE_LOCAL_MACHINE,  (strlen(gCFG->ca_cert_file) > 0 ? gCFG->ca_cert_file:"Root"));
	if ( gCFG->schCred.hRootStore ) 
	{
	while (1 ) {
		ctx = CertEnumCertificatesInStore(gCFG->schCred.hRootStore, prevCtx);
		if ( !ctx ) break;
		CertNameToStrA(X509_ASN_ENCODING, &(ctx->pCertInfo->Subject), CERT_X500_NAME_STR, sub, 126);
		//printf("root sub %s\n", sub);
		prevCtx = ctx;	
	}
	}
#endif 
	
		printf("cStrore %s\n",  gCFG->cert_store);
        cStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, 0, (HCRYPTPROV)NULL,
                        CERT_STORE_OPEN_EXISTING_FLAG | CERT_SYSTEM_STORE_CURRENT_USER, gCFG->cert_store);
        if(!cStore) {
		show_err("CertOpenStore", gCFG->cert_store);
		return false;
	}

	prevCtx = NULL;
	while (1 ) {
		ctx = CertEnumCertificatesInStore(cStore, prevCtx);
		if ( !ctx ) break;
		CertNameToStrA(X509_ASN_ENCODING, &(ctx->pCertInfo->Subject), CERT_X500_NAME_STR, sub, 126);
		printf("%s sub %s\n", gCFG->cert_store, sub);
		if ( strcasecmp(sub, gCFG->cert_sub) ==0 ) { 
			pblob =  &(ctx->pCertInfo->Subject);
			break;
		}
		prevCtx = ctx;	
	}
	PCCERT_CONTEXT client_certs[1] = { NULL };
	if ( pblob ) {
		client_certs[0] = CertFindCertificateInStore( cStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0,
			//CERT_FIND_SUBJECT_STR, sub, NULL);
			CERT_FIND_SUBJECT_NAME, pblob, NULL);
		if(client_certs[0] == NULL) {
			show_err("CertFindCertificateInStore", sub);
			return false;
		}
		gCFG->schCred.cCreds =1;
		gCFG->schCred.paCred =client_certs;
		printf("has my certfile!!\n");
	} else {
		//printf("no my certfile\n");
		gCFG->schCred.cCreds =0;
		gCFG->schCred.paCred =NULL;
	}
	//status = gCFG->pSecFun->AcquireCredentialsHandle(NULL, gCFG->provider, SECPKG_CRED_INBOUND, NULL, &gCFG->schCred, NULL, NULL,
	status = gCFG->pSecFun->AcquireCredentialsHandle(NULL, gCFG->provider, SECPKG_CRED_BOTH, NULL, &gCFG->schCred, NULL, NULL,
                                         &gCFG->cred_hnd, &expiry);

	if (status != SEC_E_OK)
	{
		disp_err("AcquireCredentialsHandle", status );
		return false;
	}
	CertCloseStore(cStore, 0);
	CertFreeCertificateContext(client_certs[0]);

#elif defined(__APPLE__)

#else
	ENGINE *e;
	unsigned TEXTUS_LONG sid = 0;
	CRYPTO_malloc_init();
	ERR_load_crypto_strings();
	SSL_load_error_strings();
	SSL_library_init();
	OPENSSL_load_builtin_modules();

	if ( strlen(gCFG->engine_id) > 0 )
	{
		ENGINE_load_builtin_engines();
		if ( strlen(gCFG->dso) >0 ) 
		{
			e = ENGINE_by_id("dynamic");
			if ( !e )
			{
				int err = ERR_get_error();
				TEXTUS_SPRINTF(errMsg, "ENGINE_by_id(dynamic) %s", ERR_error_string(err, (char *)NULL));
				err_lev = 3;
				return false;
			} 
			ENGINE_ctrl_cmd_string(e, "SO_PATH", gCFG->dso, 0);
			//ENGINE_ctrl_cmd_string(e, "ID", gCFG->engine_id, 0);
			if ( !ENGINE_ctrl_cmd_string(e, "LOAD", NULL, 0))
			{
				int err = ERR_get_error();
				TEXTUS_SPRINTF(errMsg, "ENGINE_ctrl_cmd_string(LOAD %s) %s", gCFG->dso, ERR_error_string(err, (char *)NULL));

				err_lev = 3;
				return false;
			}
			//ENGINE_add(e);
		} else {
			e = ENGINE_by_id(gCFG->engine_id);
			if ( !e )
			{
				int err = ERR_get_error();
				TEXTUS_SPRINTF(errMsg, "ENGINE_by_id(%s) %s", gCFG->engine_id, ERR_error_string(err, (char *)NULL));
				err_lev = 3;
				return false;
			} 
			//ENGINE_add(e);
		}
		printf("-id-%p-- %s, %s\n", ENGINE_by_id(gCFG->engine_id), ENGINE_get_id(e), ENGINE_get_name(e));
		if (!ENGINE_set_default(e, ENGINE_METHOD_ALL)) 
		{
			int err = ERR_get_error();
			TEXTUS_SPRINTF(errMsg, "ENGINE_set_default(%s) %s", gCFG->engine_id, ERR_error_string(err, (char *)NULL));
			err_lev = 3;
			return false;
		}
		ENGINE_free(e);
	}

	if ( ! gCFG->ssl_ctx )
		 gCFG->ssl_ctx = SSL_CTX_new( SSLv23_server_method() );
		 //gCFG->ssl_ctx = SSL_CTX_new( SSLv23_method() );
		 //gCFG->ssl_ctx = SSL_CTX_new( DTLS_method() );
		 //gCFG->ssl_ctx = SSL_CTX_new( TLSv1_2_server_method() );
	SSL_CTX_set_session_id_context(gCFG->ssl_ctx, (unsigned char *)&sid, sizeof(sid));
	if ( gCFG->isVpeer )
	if ( SSL_CTX_load_verify_locations(  gCFG->ssl_ctx, (strlen( gCFG->ca_cert_file) > 0 ?  gCFG->ca_cert_file:NULL), 
			(strlen( gCFG->capath) > 0 ?  gCFG->capath:NULL) ) != 1 )
	{
		int err = ERR_get_error();
		TEXTUS_SPRINTF(errMsg, "initio() SSL load_verify_locations %s", ERR_error_string(err, (char *)NULL));
		err_lev = 3;
		endctx();
		return false;
	}
			
	if (  gCFG->isVpeer )
		SSL_CTX_set_verify(  gCFG->ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
	if ( SSL_CTX_use_certificate_file(  gCFG->ssl_ctx,   gCFG->my_cert_file, SSL_FILETYPE_PEM ) == 0 )
	{
		int err = ERR_get_error();
		TEXTUS_SPRINTF(errMsg, "initio() SSL use_certificate %s", ERR_error_string(err, (char *)NULL));
		err_lev = 3;
		endctx();
		return false;
	}

	if ( SSL_CTX_use_PrivateKey_file(  gCFG->ssl_ctx,   gCFG->my_key_file, SSL_FILETYPE_PEM) == 0 
		
		|| SSL_CTX_check_private_key(  gCFG->ssl_ctx ) == 0 )
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

void SSLsrv::endctx()
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

void SSLsrv::herit(SSLsrv *child)
{
	if ( !child )  return ;
	//child->ssl_ctx = ssl_ctx;
	child->gCFG = gCFG;
	return ;
}

#ifdef USE_WINDOWS_SSPI
void SSLsrv::show_err(const char* fun_str, const char *info)
{
	DWORD dw = GetLastError();
	char errstr[1024];
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errstr, 1024, NULL );
	TEXTUS_SPRINTF(errMsg, "%s (%s) error: %d %s", fun_str, info, dw, errstr);
	err_lev = 3;
}

void SSLsrv::disp_err(const char* fun_str, SECURITY_STATUS status, int ret )
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
		TEXTUS_SPRINTF(errMsg, "%s error: The fContextReq parameter specified a context attribute flag (ASC_REQ_DELEGATE or ASC_REQ_PROMPT_FOR_CREDS) that was not valid..", fun_str);
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
		//The client must call CompleteAuthToken and then pass the output to the server. The client then waits for a returned token and passes it, in another call, to AcceptSecurityContext (Schannel).
		switch ( ret ) {
		case 0:
			TEXTUS_SPRINTF(errMsg, "%s return SEC_I_COMPLETE_AND_CONTINUE. The client must call CompleteAuthToken and then pass the output to the server. The client then waits for a returned token and passes it, in another call, to AcceptSecurityContext (Schannel).", fun_str);
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
	
	case SEC_E_ALGORITHM_MISMATCH:
		TEXTUS_SPRINTF(errMsg, "%s error: The client and server cannot communicate because they do not possess a common algorithm", fun_str);
		break;

	case SEC_E_BAD_BINDINGS:
		TEXTUS_SPRINTF(errMsg, "%s error: The SSPI channel bindings supplied by the client are incorrect", fun_str);
		break;

	case SEC_E_BAD_PKGID:
		TEXTUS_SPRINTF(errMsg, "%s error: The requested package identifier does not exist.", fun_str);
		break;

	case SEC_E_CANNOT_PACK:
		TEXTUS_SPRINTF(errMsg, "%s error: The package is unable to pack the context.", fun_str);
		break;

	case SEC_E_CANNOT_INSTALL:
		TEXTUS_SPRINTF(errMsg, "%s error: The security package cannot initialize successfully and should not be installed", fun_str);
		break;
	case SEC_E_CERT_EXPIRED:
		TEXTUS_SPRINTF(errMsg, "%s error: The received certificate has expired.", fun_str);
		break;
	case SEC_E_CERT_UNKNOWN:
		TEXTUS_SPRINTF(errMsg, "%s error: An unknown error occurred while processing the certificate.", fun_str);
		break;
	case SEC_E_CERT_WRONG_USAGE:
		TEXTUS_SPRINTF(errMsg, "%s error: The certificate is not valid for the requested usage.", fun_str);
		break;
	case SEC_E_CROSSREALM_DELEGATION_FAILURE:
		TEXTUS_SPRINTF(errMsg, "%s error: The server attempted to make a Kerberos-constrained delegation request for a target outside the server's realm.", fun_str);
		break;
	case CRYPT_E_NO_REVOCATION_CHECK:
		TEXTUS_SPRINTF(errMsg, "%s error: The revocation function was unable to check revocation for the certificate.", fun_str);
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
