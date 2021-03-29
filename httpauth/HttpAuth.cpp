/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: HTTP Authentication
 Build:created by octerboy 2006/04/12, Shenyang
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "BTool.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "casecmp.h"
#include "textus_string.h"

#include <time.h>
#include <stdarg.h>
class HttpAuth :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	HttpAuth();
	~HttpAuth();

private:
#include "httpsrv_obj.h"
	Amor::Pius local_pius;
	
	bool should_authen;
	char pwd_file_name[256];
	
	char realm[256];

	TBuffer *req_body;
	TBuffer *res_entity;

	static int user_num;
	static char valid_user[1024][256];
	static char valid_pwd[1024][256];
	inline bool authenticate();
	void prompt();
#include "wlog.h"
};

#include "casecmp.h"
#include "textus_string.h"

#include <string.h>
#include <assert.h>

void HttpAuth::ignite(TiXmlElement *cfg)
{
	const char* enable_str,  *file_str, *realm_str;
	if ( (enable_str = cfg->Attribute("enable")) && strcmp(enable_str, "yes") == 0 )
		should_authen = true;

	if ( (file_str = cfg->Attribute("file")))
		TEXTUS_STRNCPY(pwd_file_name, file_str, sizeof(pwd_file_name)-1);

	if ( (realm_str = cfg->Attribute("realm")))
		TEXTUS_STRNCPY(realm, realm_str, sizeof(realm)-1);	
}

bool HttpAuth::facio( Amor::Pius *pius)
{
	TBuffer **tb;
	//int ind;
	assert(pius);
	Amor::Pius set_hold;
	set_hold.ordo = Notitia::CMD_SET_HOLDING;

	switch ( pius->ordo )
	{
	case Notitia::HAS_HOLDING:	/* ����֤ */
		WBUG("facio HAS_HOLDING, %d", *((int*) pius->indic));
		break;

	case Notitia::NEW_HOLDING:	/* ����֤ */
		WBUG("facio NEW_HOLDING, %d", *((int*) pius->indic));
		prompt();
		break;
	case Notitia::AUTH_HOLDING:	/* δ��֤ */
		WBUG("facio NO_HOLDING, %d", *((int*) pius->indic));
		//ind = *((int*) pius->indic);
		/*
		if ( strcasecmp(getHead("host"), "orahttp2.you.com") == 0 )
			break;
		if ( strcasecmp(getHead("host"), "orahttp2.me.com") == 0 )
			break;
		*/
		if (!authenticate()) 
		{ /* ��֤δͨ��, ���ô�����Ϣ */
			prompt();
		}  else {
			set_hold.indic = pius->indic;
			aptus->sponte(&set_hold);
			//aptus->sponte(&local_pius);
		}
		break;
		
	case Notitia::SET_TBUF:	/* ��һ����http������, �ڶ�����http��Ӧ�� */
		WBUG("facio SET_TBUF");
		if ( (tb = (TBuffer **)(pius->indic)))
		{	//tbӦ����ΪNULL��*tb��rcv_buf
			if ( *tb) req_body = *tb; 
			tb++;
			if ( *tb) res_entity = *tb;
		} else 
		{	
			WLOG(WARNING, "facio SET_TBUF null");
		}
		break;
		
	default:
		return false;
	}
	return true;
}

bool HttpAuth::sponte( Amor::Pius *pius) { return false;}

HttpAuth::HttpAuth()
{
	local_pius.ordo = Notitia::PRO_HTTP_HEAD;
	local_pius.indic = 0;

	should_authen = false;
	memset(pwd_file_name, 0, sizeof(pwd_file_name));
	TEXTUS_STRCPY(realm, "Textus Httpd");
	req_body = (TBuffer*) 0;
	res_entity = (TBuffer*) 0;
	memset(pwd_file_name, 0, sizeof(pwd_file_name));
	memset(realm, 0, sizeof(realm));
}

Amor* HttpAuth::clone()
{
	HttpAuth *child = new HttpAuth();
	child->should_authen = should_authen;
	TEXTUS_STRCPY(child->pwd_file_name, pwd_file_name);
	TEXTUS_STRCPY(child->realm, realm);
	return (Amor*)child;
}

HttpAuth::~HttpAuth()
{ }

int HttpAuth::user_num = 0;
char HttpAuth::valid_user[1024][256] = {""};
char HttpAuth::valid_pwd[1024][256] = {""};

void HttpAuth::prompt()
{
	int sc = 401;
	char basic_realm[256];

	setHead("Status", sc);			
	setContentSize(0);			
	TEXTUS_SPRINTF(basic_realm,"Basic realm=\"%s\"", realm);
	setHead("WWW-Authenticate", basic_realm);
	setHead("Connection", "close");
	//addHead("Set-Cookie", "Session=15349;domain=.you.com");

	local_pius.ordo = Notitia::PRO_HTTP_HEAD;
	aptus->sponte(&local_pius);
}

inline bool HttpAuth::authenticate()
{
	char authinfo[500];
    	char* authpass;
    	const char *authorization;
	int l;
	char* colon;
	char* pwd_yes;
	 
	if (!should_authen ) return true;
    	if ( !(authorization = getHead("Authorization")) )
	 	return false;

    	if ( strncmp( authorization, "Basic ", 6 ) != 0 )
		return false;
    	
    	l = BTool::base64_decode(&(authorization[6]), (unsigned char*) authinfo, sizeof(authinfo) - 1 );
    	authinfo[l] = '\0';
 	WBUG("authinfo: %s", authinfo);
    	authpass = strchr( authinfo, ':' );
    	if ( authpass == (char*) 0 )
	{		 
    		return false;
    	}	
    	*authpass++ = '\0';
    	colon = strchr( authpass, ':' );
    	if ( colon != (char*) 0 )
		*colon = '\0';
 
	for ( int j =0 ; j < user_num; j++)
	{
		if ( strcmp(valid_user[j], authinfo) ==0 
		     && strcmp(valid_pwd[j], authpass) ==0)
			return true;
	}
		 
	pwd_yes = BTool::getaddr(pwd_file_name, authinfo);
	if ( !pwd_yes )
	{
		WLOG(NOTICE, "no such user %s", authinfo);
		return false;
	}

 	WBUG("Real password is \"%s\"", pwd_yes);
	if ( strcmp(authpass, pwd_yes) == 0 )
	{
		TEXTUS_STRCPY(valid_user[user_num], authinfo);
		TEXTUS_STRCPY(valid_pwd[user_num], authpass);
		user_num++;
		return true;
	}
	else
		return false;
}
#include "hook.c"

