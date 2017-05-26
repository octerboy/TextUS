/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: ssl服务端接入textus
 Build: created by octerboy, 2005/06/10
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "SSLsrv.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "casecmp.h"
#ifndef TINLINE
#define TINLINE inline
#endif
class SSLsrvuna: public Amor
{
public:
	void ignite(TiXmlElement *);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
	SSLsrvuna();
	~SSLsrvuna();
private:
	char *errMsg;

	Amor::Pius local_pius;	//仅用于向mary传回数据
	bool isPoineer;
	bool sessioning;

	SSLsrv *sslsrv;
	TINLINE void deliver(Notitia::HERE_ORDO aordo);
	TINLINE void end();
	TINLINE void errpro();
#include "wlog.h"
};

#include "textus_string.h"
#include <assert.h>

#define SLOG(Z) { Amor::Pius log_pius; \
		log_pius.ordo = Notitia::LOG_##Z; \
		log_pius.indic = errMsg; \
		aptus->sponte(&log_pius); \
		}

#define BZERO(X) memset(X, 0 ,sizeof(X))
void SSLsrvuna::ignite(TiXmlElement *cfg)
{
	const char *str ;
	
#define NCOPY(X) TEXTUS_STRNCPY(X, str, sizeof(X)-1)
	if( (str = cfg->Attribute("ca") ) )
		NCOPY(sslsrv->ca_cert_file);

	if( ( str = cfg->Attribute("cert") )) 
		NCOPY(sslsrv->my_cert_file);

	if( (str = cfg->Attribute("key") ))
		NCOPY(sslsrv->my_key_file);

	if( (str = cfg->Attribute("crl") ))
		NCOPY(sslsrv->crl_file);

	if( (str = cfg->Attribute("path") ))
		NCOPY(sslsrv->capath);

	if( (str = cfg->Attribute("engine") ))
		NCOPY(sslsrv->engine_id);

	if( (str = cfg->Attribute("dso") ))
		NCOPY(sslsrv->dso);

	if( (str = cfg->Attribute("vpeer") ) )
		if ( strcasecmp(str, "no") ==0 )
			sslsrv->isVpeer = false;
	/* 自身设定结束 */
	
	isPoineer = true;	/* 认为自己是开拓者 */
}

bool SSLsrvuna::facio( Amor::Pius *pius)
{
	TBuffer **tb = 0;
	int len = 0;

	assert(pius);
	
	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF:	/* TBuffer中有数据,进行处理 */
		WBUG("facio PRO_TBUF");
		if ( !sslsrv->bio_in_buf || !sslsrv->bio_out_buf )
		{	//当然输入输出得已经准备好
			WLOG(NOTICE,"facio PRO_TBUF null.");
			break;
		}
	
		len = sslsrv->decrypt();	//将输入数据解密
		WBUG("decrypted len is %d", len);
		errpro();
		if ( sslsrv->bio_out_buf->point > sslsrv->bio_out_buf->base ) //SSL有密文数据要发回
			aptus->sponte(&local_pius);

		if ( len < 0 )
		{
			sslsrv->ssl_down();
			errpro();
			if ( sslsrv->bio_out_buf->point > sslsrv->bio_out_buf->base )
			{	//SSL有密文数据要发回
				aptus->sponte(&local_pius);
			}
		}

		if ( sslsrv->handshake_ok && !sessioning ) 
		{	//ssl握手成功, 但这里还未标识, 所以设置会话标志, 并通知高层(右节点)
			sessioning = true;
			deliver( Notitia::START_SESSION );
		}

		if ( len > 0 ) /* SSL有明文输出,由接力者处理 */
			deliver( Notitia::PRO_TBUF );

		if ( len < 0 ) 
			end();
		break;

	case Notitia::SET_TBUF:	/* 取得输入TBuffer地址 */
		WBUG("facio SET_TBUF");
		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//当然tb不能为空
			if ( *tb) 
			{	//新到请求的TBuffer
				sslsrv->bio_in_buf = *tb;
			}
			tb++;
			if ( *tb) sslsrv->bio_out_buf = *tb;
		} else 
			WLOG(NOTICE,"facio PRO_TBUF null.");
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		assert(isPoineer);
		/* 向下一级传递sslsrv->rcv_buf和sslsrv->snd_buf对象地址 */
		deliver(Notitia::SET_TBUF);
		if (!sslsrv->initio())
			SLOG(ALERT)
		else
			WBUG("initio succeeded!");
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		assert(!isPoineer);
		deliver(Notitia::SET_TBUF);
		break;

	case Notitia::DMD_END_SESSION:	
		WBUG("facio DMD_END_SESSION");
		end();
		break;

	case Notitia::START_SESSION:	
		WBUG("facio START_SESSION");	/* 阻止START的传递, 向后传递由本类特别决定 */
		break;

	default:
		return false;
	}
	return true;
}

bool SSLsrvuna::sponte( Amor::Pius *pius)
{
	bool ret;
	assert(pius);
		
	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF :	//有明文回来了
		WBUG("sponte PRO_TBUF");
		ret = sslsrv->encrypt();	//将输入数据加密
		WBUG("encrypt ret=%d", ret);
		errpro();

		if ( ret )	//将输入数据加密
		{
			if ( sslsrv->bio_out_buf->point > sslsrv->bio_out_buf->base )
			{	//SSL有密文数据要发回
				aptus->sponte(&local_pius);
			}
		}  else {
			sslsrv->ssl_down();
			errpro();
			if ( sslsrv->bio_out_buf->point > sslsrv->bio_out_buf->base )
			{	//SSL有密文数据要发回
				aptus->sponte(&local_pius);
			}
		}
		if (!ret) //有错误, 会话被关闭
			end();
		break;

	case Notitia::DMD_END_SESSION:	/* 高级会话关闭了 */
		WBUG("sponte DMD_END_SESSION");
		end();
		break;

	case Notitia::CMD_GET_SSL:	/* 取SSL通讯句柄 */
		WBUG("sponte CMD_GET_SSL");
		pius->indic = sslsrv->ssl;
		break;
	default:
		return false;
	}
	return true;
}

Amor* SSLsrvuna::clone()
{
	SSLsrvuna *child = new SSLsrvuna();
	sslsrv->herit(child->sslsrv); //继承父实例创造的SSL环境.
	return (Amor*)child;
}

SSLsrvuna::SSLsrvuna()
{
	local_pius.ordo = Notitia::PRO_TBUF;
	local_pius.indic = 0;

	sslsrv = new SSLsrv();
	isPoineer = false;	/* 开始认为自己不是开拓者 */
	sessioning = false;	/* 刚开始，当然不在会话中 */
	this->errMsg = &sslsrv->errMsg[0];
}

SSLsrvuna::~SSLsrvuna()
{
	if ( sslsrv ) delete sslsrv;
}

/* 向接力者提交 */
TINLINE void SSLsrvuna::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	TBuffer *tb[3];
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	
	switch ( aordo)
	{
	case Notitia::SET_TBUF:
		WBUG("deliver SET_TBUF");
		tb[0] = sslsrv->rcv_buf;
		tb[1] = sslsrv->snd_buf;
		tb[2] = 0;
		tmp_pius.indic = &tb[0];
		break;

	default:
		WBUG("deliver Notitia::%d", aordo);
		break;
	}
	aptus->facio(&tmp_pius);
	return ;
}

TINLINE void SSLsrvuna::end()
{
	Amor::Pius tmp_pius;
	tmp_pius.ordo = Notitia::END_SESSION;
	tmp_pius.indic = 0;

	if ( !isPoineer)
		sslsrv->endssl();
	else
		sslsrv->endctx();
	sslsrv->rcv_buf->reset();
	sslsrv->snd_buf->reset();

	if (sessioning )
	{
		sessioning = false;
		deliver(Notitia::END_SESSION);
	}
	aptus->sponte(&tmp_pius);	/* send END_SESSION to left node */
}

TINLINE void SSLsrvuna::errpro()
{
	switch( sslsrv->err_lev )
	{
	case 3:
		SLOG(ERR)
		break;

	case 5:
		SLOG(NOTICE)
		break;

	case 7:
		SLOG(DEBUG)
		break;

	default:
		break;
	}
}
		
#include "hook.c"
