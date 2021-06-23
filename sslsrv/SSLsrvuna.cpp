/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: ssl service
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
#define SLOG(Z) { Amor::Pius log_pius; \
		log_pius.ordo = Notitia::LOG_##Z; \
		log_pius.indic = errMsg; \
		aptus->sponte(&log_pius); \
		}

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
	enum ACT_TYPE { FromSelf=0, FromFac=1, FromSpo=2 };
	char *errMsg;

	Amor::Pius local_pius;	//��������mary��������
	bool isPoineer;
	bool sessioning;

	SSLsrv *sslsrv;
	void deliver(Notitia::HERE_ORDO aordo);
	void end(enum ACT_TYPE);
	void errpro()
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
	};
#include "wlog.h"
};

#include "textus_string.h"
#include <assert.h>

#define BZERO(X) memset(X, 0 ,sizeof(X))
void SSLsrvuna::ignite(TiXmlElement *cfg)
{
	const char *str ;
	if ( !sslsrv->gCFG )
	{
		sslsrv->gCFG = new struct SSLsrv::G_CFG();
	}

#define NCOPY(X) TEXTUS_STRNCPY(X, str, sizeof(X)-1)
#ifdef USE_WINDOWS_SSPI
	if( (str = cfg->Attribute("security_dll") ) )
		NCOPY(sslsrv->gCFG->secdll_fn);

	if( (str = cfg->Attribute("security_face") ) )
		NCOPY(sslsrv->gCFG->secface_fn);

	if( (str = cfg->Attribute("security_provider") ) )
		NCOPY(sslsrv->gCFG->provider);

	if( (str = cfg->Attribute("protocol") ) )
		NCOPY(sslsrv->gCFG->proto_str);

	if( (str = cfg->Attribute("alog") ) )
		NCOPY(sslsrv->gCFG->alg_str);

	if( (str = cfg->Attribute("subject") ) )
		NCOPY(sslsrv->gCFG->cert_sub);
#elif defined(__APPLE__)

#else
	if( (str = cfg->Attribute("engine") ))
		NCOPY(sslsrv->gCFG->engine_id);

	if( (str = cfg->Attribute("dso") ))
		NCOPY(sslsrv->gCFG->dso);
#endif
	if( (str = cfg->Attribute("ca") ) )
		NCOPY(sslsrv->gCFG->ca_cert_file);

	if( ( str = cfg->Attribute("cert") )) 
		NCOPY(sslsrv->gCFG->my_cert_file);

	if( (str = cfg->Attribute("key") ))
		NCOPY(sslsrv->gCFG->my_key_file);

	if( (str = cfg->Attribute("crl") ))
		NCOPY(sslsrv->gCFG->crl_file);

	if( (str = cfg->Attribute("path") ))
		NCOPY(sslsrv->gCFG->capath);

	if( (str = cfg->Attribute("vpeer") ) )
		if ( strcasecmp(str, "no") ==0 )
			sslsrv->gCFG->isVpeer = false;
	/* �����趨���� */
	
	isPoineer = true;	/* ��Ϊ�Լ��ǿ����� */
}

bool SSLsrvuna::facio( Amor::Pius *pius)
{
	TBuffer **tb = 0;
	int ret;
	bool has_plain, has_ciph;

	assert(pius);
	
	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF:	/* TBuffer��������,���д��� */
		WBUG("facio PRO_TBUF");
		if ( !sslsrv->bio_in_buf || !sslsrv->bio_out_buf )
		{	//��Ȼ����������Ѿ�׼����
			WLOG(NOTICE,"facio PRO_TBUF null.");
			break;
		}
	
		ret = sslsrv->decrypt(has_plain, has_ciph);	//���������ݽ���
		errpro();
		if ( has_ciph) //SSL����������Ҫ����
			aptus->sponte(&local_pius);

		if ( sslsrv->handshake_ok && !sessioning ) 
		{	//ssl���ֳɹ�, �����ﻹδ��ʶ, �������ûỰ��־, ��֪ͨ�߲�(�ҽڵ�)
			sessioning = true;
			deliver( Notitia::START_SESSION );
		}

		if ( has_plain )	/* �����Ŀɶ� */
			aptus->facio(&local_pius);

		switch ( ret )
		{
		case -1:
			sslsrv->ssl_down(has_ciph);
			errpro();
			if ( has_ciph)
				aptus->sponte(&local_pius);
		case 0: //�д���, �Ự���ر�, ���� -1
			end(FromSelf);
			break;
		default:	//��������
			break;
		} 

		break;

	case Notitia::SET_TBUF:	/* ȡ������TBuffer��ַ */
		WBUG("facio SET_TBUF");
		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//��Ȼtb����Ϊ��
			if ( *tb) 
			{	//�µ������TBuffer
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
		/* ����һ������sslsrv->rcv_buf��sslsrv->snd_buf�����ַ */
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
		end(FromFac);
		break;

	case Notitia::START_SESSION:	
		WBUG("facio START_SESSION");	/* ��ֹSTART�Ĵ���, ��󴫵��ɱ����ر���� */
		if ( !isPoineer)
			sslsrv->endssl();
		else
			sslsrv->endctx();
		sslsrv->rcv_buf->reset();
		sslsrv->snd_buf->reset();
		break;

	default:
		return false;
	}
	return true;
}

bool SSLsrvuna::sponte( Amor::Pius *pius)
{
	int ret;
	bool has_c;
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF :	//�����Ļ�����
		WBUG("sponte PRO_TBUF");
		ret = sslsrv->encrypt(has_c);	//���������ݼ���
		errpro();

		if ( has_c)
		{	//SSL����������Ҫ��
			aptus->sponte(&local_pius);
		}
		switch ( ret )
		{
		case -1:
			sslsrv->ssl_down(has_c);
			errpro();
			if ( has_c)
				aptus->sponte(&local_pius);
		case 0:	//����-1
			end(FromSelf);
			break;
		default:
			break;
		}
		break;

	case Notitia::DMD_END_SESSION:	/* �߼��Ự�ر��� */
		WBUG("sponte DMD_END_SESSION");
		if ( sessioning )
		{
			sslsrv->ssl_down(has_c);
			errpro();
			if ( has_c)
				aptus->facio(&local_pius);
		}
		end(FromSpo);
		break;

	case Notitia::CMD_GET_SSL:	/* ȡSSLͨѶ��� */
		WBUG("sponte CMD_GET_SSL");
		pius->indic = sslsrv->get_ssl();
		break;
	default:
		return false;
	}
	return true;
}

Amor* SSLsrvuna::clone()
{
	SSLsrvuna *child = new SSLsrvuna();
	sslsrv->herit(child->sslsrv); //�̳и�ʵ�������SSL����.
	return (Amor*)child;
}

SSLsrvuna::SSLsrvuna()
{
	local_pius.ordo = Notitia::PRO_TBUF;
	local_pius.indic = 0;

	sslsrv = new SSLsrv();
	isPoineer = false;	/* ��ʼ��Ϊ�Լ����ǿ����� */
	sessioning = false;	/* �տ�ʼ����Ȼ���ڻỰ�� */
	this->errMsg = &sslsrv->errMsg[0];
}

SSLsrvuna::~SSLsrvuna()
{
	if ( sslsrv ) delete sslsrv;
}

/* ��������ύ */
void SSLsrvuna::deliver(Notitia::HERE_ORDO aordo)
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
	case Notitia::START_SESSION:
		WBUG("deliver START_SESSION");
		break;

	default:
		WBUG("deliver Notitia::%d", aordo);
		break;
	}
	aptus->facio(&tmp_pius);
	return ;
}

void SSLsrvuna::end(enum ACT_TYPE act)
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
		switch ( act ) 
		{
		case FromSelf:
			aptus->sponte(&tmp_pius);
			aptus->facio(&tmp_pius);	/* send END_SESSION to right node */
			break;
		case FromFac:
			aptus->facio(&tmp_pius);	/* send END_SESSION to right node */
			break;
		case FromSpo:
			aptus->sponte(&tmp_pius);
			break;
		}
	}
}
/*
void SSLsrvuna::errpro()
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
*/
		
#include "hook.c"
