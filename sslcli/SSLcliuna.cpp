/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: ssl客户端接入textus
 Build: created by octerboy, 2005/06/10
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "SSLcli.h"
#include "Notitia.h"
#include "textus_string.h"
#include "TBuffer.h"
#include "casecmp.h"
#ifndef TINLINE
#define TINLINE inline
#endif
class SSLcliuna: public Amor
{
public:
	void ignite(TiXmlElement *);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
	SSLcliuna();
	~SSLcliuna();
private:
	Amor::Pius clr_timer_pius, alarm_pius;	/* 清超时, 设超时 */
	void *arr[3];
	char *errMsg;
	SSLcli *sslcli;
	TINLINE void deliver(Notitia::HERE_ORDO aordo);
	TINLINE void end();
	TINLINE void errpro();
	TINLINE void letgo();	/* 让明文数据 */

	Amor::Pius pro_tbuf;
	Amor::Pius dmd_start;

	bool alive;	/* 通道是否打开 */
	bool sessioning;	/* 是否建立会话 */
	bool demanding;	/* 正在要求通道, 还未有响应 */
	
	struct G_CFG {
		Amor::Pius chn_timeout;
		int expired;	/* 超时时间。0: 不设超时 */
		
		inline G_CFG ( TiXmlElement *cfg ) {
			chn_timeout.ordo =  Notitia::CHANNEL_TIMEOUT;
			chn_timeout.indic = 0;
			expired = 0;

			cfg->QueryIntAttribute("expired", &(expired));
		};
	};
	struct G_CFG *gCFG;	/* Shared for all objects in this node */
	bool has_config;
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
void SSLcliuna::ignite(TiXmlElement *cfg)
{
	const char *str ;
	
	if (!cfg) return;

	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}

	if ( !sslcli->gCFG )
	{
		sslcli->gCFG = new struct SSLcli::G_CFG();
	}
#define NCOPY(X) TEXTUS_STRNCPY(X, str, sizeof(X)-1)
	if( (str = cfg->Attribute("ca") ) )
		NCOPY(sslcli->gCFG->ca_cert_file);

	if( ( str = cfg->Attribute("cert") )) 
		NCOPY(sslcli->gCFG->my_cert_file);

	if( (str = cfg->Attribute("key") ))
		NCOPY(sslcli->gCFG->my_key_file);

	if( (str = cfg->Attribute("crl") ))
		NCOPY(sslcli->gCFG->crl_file);

	if( (str = cfg->Attribute("path") ))
		NCOPY(sslcli->gCFG->capath);

	if( (str = cfg->Attribute("vpeer") ) )
		if ( strcasecmp(str, "yes") ==0 )
			sslcli->gCFG->isVpeer = true;
	/* 自身设定结束 */
}

bool SSLcliuna::facio( Amor::Pius *pius)
{
	TBuffer **tb = 0;

	assert(pius);
	
	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF:	/* TBuffer中有数据,进行处理 */
		WBUG("facio PRO_TBUF");
		if ( !sslcli->rcv_buf || !sslcli->snd_buf )
		{	//当然输入输出得已经准备好
			WLOG(NOTICE,"facio PRO_TBUF null.");
			break;
		}

		if ( !alive ) 
		{	/* 要求打开通道 */
			if( !demanding )
			{
				aptus->facio(&dmd_start);	/* 要求开始 */
				demanding = true;			/* 置标志 */
				if( gCFG->expired > 0 )			/* 如果设了超时 */
					aptus->sponte(&alarm_pius);
			}
			break;
		}
	
		letgo();
		break;

	case Notitia::SET_TBUF:	/* 取得输入TBuffer地址 */
		WBUG("facio SET_TBUF");
		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//当然tb不能为空
			if ( *tb) 
			{	//新到请求的TBuffer
				sslcli->snd_buf = *tb;
			}
			tb++;
			if ( *tb) sslcli->rcv_buf = *tb;
		} else 
			WLOG(NOTICE,"facio SET_TBUF null.");
		break;

	case Notitia::TIMER:	/* 连接超时 */
		WBUG("facio TIMER" );
		if ( demanding)
		{
			WLOG(WARNING, "channel time out");
			aptus->sponte(&clr_timer_pius);	/* 清除定时 */
			aptus->sponte(&(gCFG->chn_timeout));	/* 向左通知 */
			demanding = false;
		}
		break;

	case Notitia::TIMER_HANDLE:
		WBUG("facio TIMER_HANDLE");
		clr_timer_pius.indic = pius->indic;
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		assert(has_config);
		arr[0] = this;
		arr[1] = &(gCFG->expired);
		arr[2] = 0;
		/* 向下一级传递sslcli->bio_out_buf和sslcli->bio_in_buf的地址 */
		deliver(Notitia::SET_TBUF);
		if (!sslcli->initio())
			SLOG(ALERT)
		else
			WBUG("initio succeeded!");
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		deliver(Notitia::SET_TBUF);
		arr[0] = this;
		arr[1] = &(gCFG->expired);
		arr[2] = 0;
		break;

	case Notitia::DMD_END_SESSION:	
		WBUG("facio DMD_END_SESSION");
		end();
		break;

	default:
		return false;
	}
	return true;
}

bool SSLcliuna::sponte( Amor::Pius *pius)
{
	int len;
	assert(pius);
		
	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF :	//有密文收到
		WBUG("sponte PRO_TBUF");
		len = sslcli->decrypt(); /* 解密数据 */
		errpro();
		if ( sslcli->bio_out_buf.point > sslcli->bio_out_buf.base ) /* SSL有密文数据要发 */
			aptus->facio(&pro_tbuf);

		if ( len < 0 )
		{
			sslcli->ssl_down();
			errpro();
			if ( sslcli->bio_out_buf.point > sslcli->bio_out_buf.base ) /* SSL有密文数据要发 */
				aptus->facio(&pro_tbuf);
		}

		if ( sslcli->handshake_ok && !sessioning ) 
		{	//ssl握手成功, 但这里还未标识, 所以设置会话标志, 并通知高层(左节点)
			sessioning = true;
			deliver( Notitia::START_SESSION );
		}

		if ( len > 0 )	/* 有明文可读 */
			aptus->sponte(&pro_tbuf);

		if (len < 0) //有错误, 会话被关闭
			end();
		break;

	case Notitia::DMD_END_SESSION:	/* 底层会话关闭了 */
		WBUG("sponte DMD_END_SESSION");
		if ( demanding )
		{
			demanding = false;
			if( gCFG->expired > 0 )			/* 如果设了超时 */
				aptus->sponte(&clr_timer_pius);	/* 清除定时 */
		}
		end();
		alive = false;
		break;

	case Notitia::START_SESSION:	/* 底层通讯建立, 阻止其传递 */
		WBUG("sponte START_SESSION");
		alive = true;
		if ( demanding )
		{
			demanding = false;
			if( gCFG->expired > 0 )			/* 如果设了超时 */
				aptus->sponte(&clr_timer_pius);	/* 清除定时 */
		}
		letgo();	 //即使没有明文数据,也建立SSL连接
		break;

	case Notitia::CMD_GET_SSL:	/* 取SSL通讯句柄 */
		WBUG("sponte CMD_GET_SSL");
		pius->indic = sslcli->ssl;
		break;
	default:
		return false;
	}
	return true;
}

Amor* SSLcliuna::clone()
{
	SSLcliuna *child = new SSLcliuna();
	sslcli->herit(child->sslcli); //继承父实例创造的SSL环境.
	child->gCFG = gCFG;
	return (Amor*)child;
}

SSLcliuna::SSLcliuna()
{
	pro_tbuf.indic = 0;
	pro_tbuf.ordo = Notitia::PRO_TBUF;

	dmd_start.indic = 0;
	dmd_start.ordo = Notitia::DMD_START_SESSION;

	sslcli = new SSLcli();
	sessioning = false;	/* 刚开始，当然不在会话中 */
	alive = false;
	demanding = false;
	this->errMsg = &sslcli->errMsg[0];

	has_config = false;
	gCFG = 0;
	clr_timer_pius.ordo = Notitia::DMD_CLR_TIMER;
	clr_timer_pius.indic = this;

	alarm_pius.ordo = Notitia::DMD_SET_ALARM;
	alarm_pius.indic = &arr[0];
}

SSLcliuna::~SSLcliuna()
{
	if ( has_config)
		sslcli->endctx();
	if ( sslcli ) delete sslcli;
}

TINLINE void SSLcliuna::end()
{
	Amor::Pius tmp_pius;
	tmp_pius.ordo = Notitia::END_SESSION;
	tmp_pius.indic = 0;

	sslcli->endssl();
	//sslcli->rcv_buf->reset();
	//sslcli->snd_buf->reset();

	if (sessioning )
	{
		sessioning = false;
		aptus->sponte(&tmp_pius);
	}
	aptus->facio(&tmp_pius);	/* send END_SESSION to right node */
}

TINLINE void SSLcliuna::letgo()
{
	int ret;
	ret = sslcli->encrypt();	//将输入数据加密,并待发送,这里包括ssl握手
	errpro();

	if ( sslcli->bio_out_buf.point > sslcli->bio_out_buf.base )
	{	//SSL有密文数据要发
		aptus->facio(&pro_tbuf);
	}
	if ( ret < 0 ) 
	{
		sslcli->ssl_down();
		if ( sslcli->bio_out_buf.point > sslcli->bio_out_buf.base )
			aptus->facio(&pro_tbuf);
		end();
	}
}

TINLINE void SSLcliuna::errpro()
{
	switch( sslcli->err_lev )
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
		
/* 向接力者提交 */
TINLINE void SSLcliuna::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	TBuffer *tb[3];
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	
	switch (aordo)
	{
	case Notitia::START_SESSION:
		WBUG("deliver(sponte) START_SESSION");
		tmp_pius.indic = 0;
		aptus->sponte(&tmp_pius);
		goto END ;

	case Notitia::SET_TBUF:
		WBUG("deliver SET_TBUF");
		tb[0] = &sslcli->bio_out_buf;
		tb[1] = &sslcli->bio_in_buf;
		tb[2] = 0;
		tmp_pius.indic = &tb[0];
		break;

	default:
		WBUG("deliver Notitia::%d",aordo);
		break;
	}
	aptus->facio(&tmp_pius);
END:
	return ;
}

#include "hook.c"
