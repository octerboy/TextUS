/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
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
 $Header: /textus/formauth/FormAuth.cpp 4     08-01-01 20:28 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: FormAuth.cpp $"
#define TEXTUS_MODTIME  "$Date: 08-01-01 20:28 $"
#define TEXTUS_BUILDNO  "$Revision: 4 $"
/* $NoKeywords: $ */

#include "BTool.h"
#include "Notitia.h"
#include "Amor.h"
#include "TBuffer.h"
#include "PacData.h"
#include "casecmp.h"
#include "textus_string.h"

#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>

class FormAuth :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	FormAuth();
	~FormAuth();

private:
#include "httpsrv_obj.h"
	Amor::Pius local_pius;
	#define NAME_LEN 64
	struct Login {
		char user[NAME_LEN];
		bool logged;
		inline Login () {
			memset(user, 0, sizeof(user));
			logged = false;
		};
	};
	
	struct G_CFG {
		const char *user_para_name;	/* 用户名参数名 */
		const char *passwd_para_name;	/* 口令参数名 */
		const char *uri_para_name;	/* path参数名 */
		const char *login_url;	/* 登录用path */
		const char *sub_pattern;	/* 登录网页中用以替换uri的特征字符串 */
		
		int user_fld;	/* 用户名所在field */
		int pass_fld;	/* 口令所在field */
		int sess_fld;	/* 口令所在field */
		int max_fld;

		struct Login *logins;
		int logNum;

		inline void expand(int n) {
			struct Login *old = logins;
			int om = logNum;

			while ( logNum < n)
				logNum *=2;
			logins = new struct Login[logNum];
			memcpy(logins, old, sizeof(struct Login) *om );
			delete []old;
		};

		inline G_CFG (TiXmlElement *cfg) {
			TiXmlElement *ele;
			user_fld = pass_fld = sess_fld = max_fld = -1;
			uri_para_name = user_para_name = passwd_para_name = (char *)0;
			login_url = (char *)0;
			logNum = 0;
			logins = 0;
			sub_pattern = (char*) 0;
			
			if (!cfg)
				return;

			login_url = cfg->Attribute("path");
			sub_pattern = cfg->Attribute("pattern");

			ele = cfg->FirstChildElement("form");
			if ( ele )
			{
				user_para_name = ele->Attribute("user");
				passwd_para_name = ele->Attribute("password");
				uri_para_name = ele->Attribute("uri");
			}

			ele = cfg->FirstChildElement("field");
			if ( ele )
			{
				ele->QueryIntAttribute("user", &user_fld);
				ele->QueryIntAttribute("password", &pass_fld);
				ele->QueryIntAttribute("session", &sess_fld);
				ele->QueryIntAttribute("maxium", &max_fld);
			}

			if ( max_fld == -1 )
			{
				#define MAX(Y) max_fld = max_fld < Y ? Y:max_fld
				MAX(user_fld);
				MAX(pass_fld);
				MAX(sess_fld);
				max_fld = max_fld + max_fld/2;
			}

			logNum = 16;
			logins = new struct Login [logNum];
		};

		inline ~G_CFG() {
			delete []logins;
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

	void deliver(Notitia::HERE_ORDO aordo);
	inline bool authenticate(char *pass, int);
	inline const char* getUser();
	void prompt();
	int authing_index;
	
#include "wlog.h"
};


void FormAuth::ignite(TiXmlElement *cfg)
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

bool FormAuth::facio( Amor::Pius *pius)
{
	TBuffer **tb;
	int ulen;
	const char *usr;
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

	case Notitia::HAS_HOLDING:	/* 已认证 */
		WBUG("facio HAS_HOLDING, %d", *((int*) pius->indic));
		break;

	case Notitia::NEW_HOLDING:	/* 建立新会话 */
		WBUG("facio NEW_HOLDING, %d", *((int*) pius->indic));
		prompt();
		break;

	case Notitia::AUTH_HOLDING:	/* 未认证 */
		WBUG("facio AUTH_HOLDING, %d", *((int*) pius->indic));
		authing_index = *((int*) pius->indic);
		if ( strcmp(getHead("path"), gCFG->login_url) != 0 )
			prompt();
		break;

	case Notitia::PRO_HTTP_REQUEST:
		if ( authing_index < 0 || !(usr= getUser()) )
		{
			prompt();
			goto END;
		}

		if ( authing_index > gCFG->logNum )
			gCFG->expand(authing_index);

		memset(gCFG->logins[authing_index].user, 0, NAME_LEN);
		ulen = strlen(usr);
		memcpy(gCFG->logins[authing_index].user, usr, ulen > NAME_LEN-1 ? NAME_LEN-1:ulen);

		req_pac.reset();
		ans_pac.reset();

		if ( gCFG->user_fld >=0 )
			req_pac.input(gCFG->user_fld, (unsigned char*)usr, strlen(usr));

		if ( gCFG->sess_fld >=0 )
			req_pac.input(gCFG->sess_fld, (unsigned char*)&authing_index, sizeof(authing_index));

		authing_index = -1;
		aptus->facio(&pro_unipac);	
	END:
		break;
		
	case Notitia::SET_TBUF:	/* 第一个是http请求体, 第二个是http响应体 */
		WBUG("facio SET_TBUF");
		if ( (tb = (TBuffer **)(pius->indic)))
		{	//tb应当不为NULL，*tb是rcv_buf
			if ( *tb) req_body = *tb; 
			tb++;
			if ( *tb) res_entity = *tb;
		} else {	
			WLOG(WARNING, "facio SET_TBUF null");
		}
		aptus->facio(pius);	
		break;
		
	default:
		return false;
	}
	return true;
}

bool FormAuth::sponte( Amor::Pius *pius) 
{ 
	Amor::Pius set_hold;
	int len;
	int ind;
	char pass[VAL_MAX];
	const char *uri;
	char *p;
	int lenu, lens, i;

	set_hold.ordo = Notitia::CMD_SET_HOLDING;
	switch ( pius->ordo )
	{
	case Notitia::PRO_UNIPAC:	/* 这是认证返回 */
		WBUG("sponte PRO_UNIPAC");
		memset(pass, 0, VAL_MAX);
		len = ans_pac.fld[gCFG->pass_fld].range;
		memcpy(pass, ans_pac.fld[gCFG->pass_fld].val, len >= VAL_MAX ? VAL_MAX-1:len);
		ind = *((int*)req_pac.fld[gCFG->sess_fld].val);
		
		if (!authenticate(pass, ind) )
		{ /* 认证未通过, 设置错误信息 */
			WBUG("authenticate failed!");
			prompt();
		}  else {
			WBUG("authenticate successfully!");
			/* 设置该session为已认证标志 */
			set_hold.indic = &ind;
			aptus->sponte(&set_hold);

			/* 重定向至根据路径,以后要重定向至原有路径,但不保持那个query */
			res_entity->reset();
			setStatus(303);
    			uri=getPara(gCFG->uri_para_name);
			if ( !uri )
			{
				setHead("Location", "/");
				setContentSize(1);
				res_entity->input((unsigned char*)"/", 1);
			} else {
				setHead("Location", uri);
				setContentSize(strlen(uri));
				res_entity->input((unsigned char*)uri, strlen(uri));
			}
			local_pius.ordo = Notitia::PRO_HTTP_HEAD;
			aptus->sponte(&local_pius);
			local_pius.ordo = Notitia::PRO_TBUF;
			aptus->sponte(&local_pius);
		}
		break;

	case Notitia::PRO_HTTP_HEAD:	/* 截住，等PRO_TBUF时再传 */
		WBUG("sponte PRO_HTTP_HEAD");
		break;

	case Notitia::PRO_TBUF:	/* 这是HTML文件输出 */
		WBUG("sponte PRO_TBUF");
    		uri=getPara( (char*)gCFG->uri_para_name);
		if ( !uri )
		{
			uri = getHead("path");
		}
		if (!uri || !gCFG->sub_pattern )
			break;
		
		lenu = strlen(uri); lens = strlen(gCFG->sub_pattern);
		if ( lens < lenu)
			res_entity->grant ( lenu-lens );	

		p = (char*) res_entity->base;
		len = res_entity->point - res_entity->base - lens;
		for (i = 0 ; i < len; i++, p++)
		{
			if (memcmp(p, gCFG->sub_pattern, lens) == 0 )
			{
				memmove (&p[lenu], &p[lens], res_entity->point - (unsigned char*)&p[lens]);
				memcpy(p, uri, lenu);
				res_entity->point += (lenu - lens);
				setContentSize(res_entity->point - res_entity->base);
				break;
			}
		}
		local_pius.ordo = Notitia::PRO_HTTP_HEAD;
		aptus->sponte(&local_pius);
		aptus->sponte(pius);
		break;

	default:
		return false;
	}
	return true;
}

FormAuth::FormAuth()
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

	authing_index = -1;
}

Amor* FormAuth::clone()
{
	FormAuth *child = new FormAuth();
	
	child->gCFG = gCFG;
	child->req_pac.produce(gCFG->max_fld);
	child->ans_pac.produce(gCFG->max_fld);
	return (Amor*)child;
}

FormAuth::~FormAuth()
{ 
	if ( has_config && gCFG)
		delete gCFG;
}

const char* FormAuth::getUser()
{
    	return getPara( (char*)gCFG->user_para_name);
}

void FormAuth::prompt()
{
	local_pius.ordo = Notitia::PRO_HTTP_HEAD;
	aptus->facio(&local_pius);
}

inline bool FormAuth::authenticate(char *pass, int ind)
{
	const char *pwd;

    	if ( !(pwd = getPara(gCFG->passwd_para_name)) )
	 	return false;

    	if ( strcmp(pwd, pass) == 0 )
		return true;

	return false;
}

/* 向接力者提交 */
void FormAuth::deliver(Notitia::HERE_ORDO aordo)
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

