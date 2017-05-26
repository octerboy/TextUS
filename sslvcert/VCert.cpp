/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: ���SSL�ͻ��˵�֤��
 Build::created by octerboy, 2006/07/21
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#define CERT_MAX 10
class VCert: public Amor
{
public:
	void ignite(TiXmlElement *);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
	VCert();
	~VCert();
private:
	char ssl_cert_no[CERT_MAX][128];/* WEB�ͻ��˵�֤���, ���� */
	SSL* ssl;	/* */
	char* current_no;	/* ��ǰ֤��� */

	Amor::Pius local_pius;	//��������mary��������
	bool sessioning;
#include "wlog.h"
};

#include <assert.h>

#define BZERO(X) memset(X, 0 ,sizeof(X))
void VCert::ignite(TiXmlElement *cfg)
{
	TiXmlElement *usrno_ele;	
	const char *str;
	int i = 0;

	usrno_ele = cfg->FirstChildElement("client");
	while ( usrno_ele )
	{
		str = usrno_ele->Attribute("certno");
		if( str )
		{
			TEXTUS_SPRINTF(ssl_cert_no[i], str, 127);
		}
		if ( i == CERT_MAX) break;
		i++;
		usrno_ele = usrno_ele->NextSiblingElement("client");
	}
}

bool VCert::facio( Amor::Pius *pius)
{
	long verify_result;

	assert(pius);
	
	switch ( pius->ordo )
	{
	case Notitia::DMD_END_SESSION:	
		WBUG("facio DMD_END_SESSION");
		sessioning = false;
		current_no = (char *) 0;
		break;

	case Notitia::START_SESSION:	
		WBUG("facio START_SESSION");	/* һ��SSL�Ự�ս���, �����ȡ��֤�鲢�����ж� */
		local_pius.ordo = Notitia::CMD_GET_SSL;
		local_pius.indic = 0;
		aptus->sponte(&local_pius);
		ssl = (SSL *)local_pius.indic;
		if ( !ssl ) {
			WLOG(INFO, "CMD_GET_SSL get 0");
			break;	/* ǰ��û��ʵ��, ����Ź� */
		}

		current_no = (char *) 0;
		if( (verify_result = SSL_get_verify_result(ssl)) != X509_V_OK) 
		{
			WLOG(ALERT, "SSL_get_verify_result %s", ERR_error_string(ERR_get_error(), (char *)NULL));
			goto FAIL;
		} else {
			int i ;
			X509 *peer;
			ASN1_STRING *serial;
			char nostring[64];
			char tmp[64];

			peer = SSL_get_peer_certificate(ssl);
			if ( !peer )
			{
				WLOG(ALERT, "SSL_get_peer_certificate %s", ERR_error_string(ERR_get_error(), (char *)NULL));
				goto FAIL;
			}

			serial = X509_get_serialNumber(peer); 
			memset(nostring, 0, sizeof(nostring));
			for ( i =0; i < serial->length; i++ )
			{
				TEXTUS_SPRINTF(tmp,"%02X",serial->data[i]);
				memcpy(&nostring[2*i],tmp,2);
			}
			WBUG("SSL_get_serialNumber %s",nostring);
			X509_free(peer);

			for ( i = 0 ; i < CERT_MAX; i++)
			{
				if ( strcmp(nostring, ssl_cert_no[i]) == 0 ) 
				{
					current_no = ssl_cert_no[i];
					break;
				}
			}

			if ( i >= CERT_MAX )
			{
				/* û�пɷ��ϵ�֤��, �ȹر���, ����ʲô�´��ٴ���  */
				WLOG(ERR, "invalid cert");
				goto FAIL;
			}
		}
		sessioning = true;
		goto End;
FAIL:
		local_pius.ordo = Notitia::DMD_END_SESSION;
		aptus->sponte(&local_pius);
End:
		break;

	default:
		return false;
	}
	return true;
}

bool VCert::sponte( Amor::Pius *pius) 
{ 
	switch ( pius->ordo )
	{
	case Notitia::CMD_GET_CERT_NO:	
		WBUG("sponte CMD_GET_CERT_NO");
		pius->indic = current_no;
		break;

	default:
		return false;
	}

	return true;
}

Amor* VCert::clone()
{
	VCert *child = new VCert();
	memcpy(child->ssl_cert_no, ssl_cert_no, sizeof(ssl_cert_no));
	return (Amor*)child;
}

VCert::VCert()
{
	local_pius.indic = 0;
	sessioning = false;
}

VCert::~VCert()
{ }

#include "hook.c"
