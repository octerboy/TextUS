/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: HTTP NTLM Authentication
 Build:created by octerboy 2007/09/10, Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "PacData.h"
#include "casecmp.h"
#include "textus_string.h"
#include "digcalc.h"

#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#define xisspace(x) isspace((unsigned char)(x))
extern "C" {
int strListGetItem(const char * str, char del, const char **item, int *ilen, const char **pos);
}

class Digest :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	Digest();
	~Digest();

private:
#include "httpsrv_obj.h"
	Amor::Pius local_pius;
	#define ONCE_LEN 64
	struct Nonce {
		char value[ONCE_LEN];
		inline Nonce () {
			memset(value, 0, sizeof(value));
		};
	};
	
	struct G_CFG {
		const char *realm;
		const char *domain;
		
		int domain_fld;	/* 域名所在field */
		int user_fld;	/* 用户名所在field */
		int pass_fld;	/* 口令所在field */
		int sess_fld;	/* */

		int max_fld;

		struct Nonce *once;
		int oNum;

		inline void expand(int n) {
			struct Nonce *old = once;
			int om = oNum;

			while ( oNum < n)
				oNum *=2;
			once = new struct Nonce[oNum];
			memcpy(once, old, sizeof(struct Nonce) *om );
			delete []old;
		};


		inline G_CFG (TiXmlElement *cfg) {
			TiXmlElement *ele;
			realm = 0;
			domain = 0;
			domain_fld = user_fld = pass_fld = sess_fld = max_fld = -1;

			realm = cfg->Attribute("realm");
			domain = cfg->Attribute("domain");
			ele = cfg->FirstChildElement("field");
			if ( ele )
			{
				ele->QueryIntAttribute("domain", &domain_fld);
				ele->QueryIntAttribute("user", &user_fld);
				ele->QueryIntAttribute("password", &pass_fld);
				ele->QueryIntAttribute("session", &sess_fld);
				ele->QueryIntAttribute("maxium", &max_fld);
			}
			if ( max_fld == -1 )
			{
				#define MAX(Y) max_fld = max_fld < Y ? Y:max_fld
				MAX(domain_fld);
				MAX(user_fld);
				MAX(pass_fld);
				MAX(sess_fld);
				max_fld = max_fld + max_fld/2;
			}
			srand((unsigned int)time(0));

			oNum = 16;
			once = new struct Nonce[oNum];
		};

		inline ~G_CFG() {
		}
	};
	struct G_CFG *gCFG;
	bool has_config;

	TBuffer *req_body;
	TBuffer *res_entity;

	PacketObj req_pac;
	PacketObj ans_pac;	/* 传递至右节点的PacketObj */

	PacketObj *pac[3];

	Amor::Pius pro_unipac;

	#define VAL_MAX 128
	struct DigestRequest {
		char username[VAL_MAX];
		char realm[VAL_MAX];
		char qop[VAL_MAX];
		char algorithm[VAL_MAX];
		char uri[VAL_MAX];
		char nonce[VAL_MAX];
		char nc[VAL_MAX];
		char cnonce[VAL_MAX];
		char response[VAL_MAX];
	};

	struct DigestRequest _request; 
	
	void deliver(Notitia::HERE_ORDO aordo);
	inline bool authenticate(char *pass, int);
	inline bool parse();
	void prompt(int);
	
#include "wlog.h"
};


void Digest::ignite(TiXmlElement *cfg)
{
	if (!cfg) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
	req_pac.produce(gCFG->max_fld);
	ans_pac.produce(gCFG->max_fld);
}

bool Digest::facio( Amor::Pius *pius)
{
	TBuffer **tb;
	int ind;
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		deliver(Notitia::SET_UNIPAC);	
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
		deliver(Notitia::SET_UNIPAC);	
		break;

	case Notitia::HAS_HOLDING:	/* 刚认证 */
		WBUG("facio HAS_HOLDING, %d", *((int*) pius->indic));
		break;

	case Notitia::NEW_HOLDING:	/* 建立新会话 */
		WBUG("facio NEW_HOLDING, %d", *((int*) pius->indic));
		ind = *((int*) pius->indic);
		prompt(ind);
		break;

	case Notitia::AUTH_HOLDING:	/* 未认证 */
		WBUG("facio AUTH_HOLDING, %d", *((int*) pius->indic));
		ind = *((int*) pius->indic);
		if ( !parse() )
		{
			prompt(ind);
			break;
		}

		req_pac.reset();
		ans_pac.reset();
		if ( gCFG->sess_fld >=0 )
			req_pac.input(gCFG->sess_fld, (unsigned char*)&ind, sizeof(ind));

		if ( gCFG->domain_fld >=0  && gCFG->domain )
			req_pac.input(gCFG->domain_fld, (unsigned char*)gCFG->domain, strlen(gCFG->domain));

		if ( gCFG->user_fld >=0 )
			req_pac.input(gCFG->user_fld, (unsigned char*)_request.username, strlen(_request.username));
		aptus->facio(&pro_unipac);	
		break;
		
	case Notitia::SET_TBUF:	/* 第一个是http请求体, 第二个是http响应体 */
		WBUG("facio SET_TBUF");
		if ( (tb = (TBuffer **)(pius->indic)))
		{	//tb应当不为NULL，*tb是rcv_buf
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

bool Digest::sponte( Amor::Pius *pius) 
{ 
	Amor::Pius set_hold;
	int len;
	int ind;
	char pass[VAL_MAX];

	set_hold.ordo = Notitia::CMD_SET_HOLDING;

	switch ( pius->ordo )
	{
	case Notitia::PRO_UNIPAC:
		WBUG("sponte PRO_UNIPAC");
		memset(pass, 0, VAL_MAX);
		len = ans_pac.fld[gCFG->pass_fld].range;
		memcpy(pass, ans_pac.fld[gCFG->pass_fld].val, len >= VAL_MAX ? VAL_MAX-1:len);
		ind = *((int*)req_pac.fld[gCFG->sess_fld].val);
		
		if (!authenticate(pass, ind) )
		{ /* 认证未通过, 设置错误信息 */
			WBUG("authenticate failed");
			prompt(ind);
		}  else {
			set_hold.indic = &ind;
			aptus->sponte(&set_hold);
		}
		break;

	default:
		return false;
	}
	return true;
}

Digest::Digest()
{
	local_pius.ordo = Notitia::PRO_HTTP_HEAD;
	local_pius.indic = 0;

	req_body = (TBuffer*) 0;
	res_entity = (TBuffer*) 0;

	has_config = false;	/* 开始认为自己不拥有全局参数表 */
	gCFG = 0;
	
	pac[0] = &req_pac;
	pac[1] = &ans_pac;
	pac[2] = 0;

	pro_unipac.ordo = Notitia::PRO_UNIPAC;
	pro_unipac.indic = 0;
}

Amor* Digest::clone()
{
	Digest *child = new Digest();
	
	child->gCFG = gCFG;
	child->req_pac.produce(gCFG->max_fld);
	child->ans_pac.produce(gCFG->max_fld);
	return (Amor*)child;
}

Digest::~Digest()
{ 
	if ( has_config && gCFG)
		delete gCFG;
}

void Digest::prompt(int ind)
{
	int sc = 401;
	char basic_realm[1024];

	setHead("Status", sc);			
	setContentSize(0);			
	if ( ind > gCFG->oNum )
		gCFG->expand(ind);
	TEXTUS_SNPRINTF(gCFG->once[ind].value, ONCE_LEN, "%x%x%x%x", rand(), rand(), rand(), rand());
	
	TEXTUS_SPRINTF(basic_realm,"Digest realm=\"%s\", domain=\"%s\", nonce=\"%s\", algorithm=MD5, qop=\"auth,auth-int\"", gCFG->realm, gCFG->domain, gCFG->once[ind].value);
	//TEXTUS_SPRINTF(basic_realm,"Digest realm=\"%s\", domain=\"%s\", nonce=\"84E0a095cfd25153b2e4014ea87a0980\", algorithm=MD5", gCFG->realm, gCFG->domain);

	setHead("WWW-Authenticate", basic_realm);
	setHead("Connection", "close");

	local_pius.ordo = Notitia::PRO_HTTP_HEAD;
	aptus->sponte(&local_pius);
}

inline bool Digest::parse()
{
	const char *item;
	const char *p;
	const char *pos = 0;
	int ilen;
	const char *authorization;

	struct DigestRequest *digest_request = &_request;
	
    	if ( !(authorization = getHead("Authorization")) )
	 	return false;

    	if ( strncmp( authorization, "Digest ", 7 ) != 0 )
		return false;
    	
	memset(digest_request, 0, sizeof(struct DigestRequest));
	ilen = 0;
	while (strListGetItem(&authorization[7], ',', &item, &ilen, &pos)) 
	{
	if ((p = strchr(item, '=')) && (p - item < ilen))
		ilen = p++ - item;

	if (!strncmp(item, "username", ilen)) {
		/* white space */
		while (xisspace(*p)) p++;
		/* quote mark */
	    	p++;
		memcpy(digest_request->username, p, strchr(p, '"')  - p);

	} else if (!strncmp(item, "realm", ilen)) {
		/* white space */
		while (xisspace(*p)) p++;
		/* quote mark */
		p++;
		memcpy(digest_request->realm, p, strchr(p, '"')  - p);

	} else if (!strncmp(item, "qop", ilen)) {
		/* white space */
		while (xisspace(*p)) p++;
		if (*p == '\"') /* quote mark */ p++;
		memcpy(digest_request->qop, p, strcspn(p, "\" \t\r\n()<>@,;:\\/[]?={}") );

	} else if (!strncmp(item, "algorithm", ilen)) {
		/* white space */
		while (xisspace(*p)) p++;
		if (*p == '\"') /* quote mark */ p++;
		memcpy(digest_request->algorithm, p, strcspn(p, "\" \t\r\n()<>@,;:\\/[]?={}") );

	} else if (!strncmp(item, "uri", ilen)) {
		/* white space */
		while (xisspace(*p)) p++;
		/* quote mark */
		p++;
		memcpy(digest_request->uri, p, strchr(p, '"') - p);

	} else if (!strncmp(item, "nonce", ilen)) {
		/* white space */
		while (xisspace(*p)) p++;
		/* quote mark */
		p++;
		memcpy(digest_request->nonce, p, strchr(p, '"')  - p);

	} else if (!strncmp(item, "nc", ilen)) {
		/* white space */
		while (xisspace(*p)) p++;
		memcpy(digest_request->nc, p, 8);

	} else if (!strncmp(item, "cnonce", ilen)) {
		/* white space */
		while (xisspace(*p)) p++;
		/* quote mark */
		p++;
		memcpy(digest_request->cnonce, p, strchr(p, '"')  - p);

	} else if (!strncmp(item, "response", ilen)) {
		/* white space */
		while (xisspace(*p)) p++;
		/* quote mark */
	    	p++;
		memcpy(digest_request->response, p, strchr(p, '"') - p);
	}
	}

	if ( digest_request->algorithm[0] == '\0' )
		memcpy( digest_request->algorithm, "MD5", 3);

	return true;
}

inline bool Digest::authenticate(char *pass, int ind)
{
	const char *method, *uri;

	HASHHEX HA1;
	HASHHEX HA2 = "";
	HASHHEX response;
	
    	if ( !(method = getHead("Method")) )
	 	return false;

    	if ( !(uri = getHead("Path")) )
	 	return false;

	DigestCalcHA1(_request.algorithm, _request.username, _request.realm, pass, _request.nonce,_request.cnonce, HA1);
	DigestCalcResponse(HA1, _request.nonce, _request.nc, _request.cnonce, _request.qop, method, uri, HA2, response);

	if ( ind >= gCFG->oNum )
		return false;

	if ( strcmp(gCFG->once[ind].value,  _request.nonce) == 0 && strcmp(response, _request.response) == 0 )
		return true;

	return false;
}

/* 向接力者提交 */
void Digest::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	
	switch (aordo)
	{
	case Notitia::SET_UNIPAC:
		WBUG("deliver SET_UNIPAC");
		tmp_pius.indic = &pac[0];
		break;

	default:
		WBUG("deliver Notitia::%d",aordo);
		break;
	}

	aptus->facio(&tmp_pius);
	return ;
}

#include "hook.c"

