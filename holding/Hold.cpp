/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
/**
 Title:Holding Pro
 Build: created by octerboy, 2006/11/01, Hebi(Henan)
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "SetCookie.h"
#include "Notitia.h"
#include "casecmp.h"
#include "textus_string.h"
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

#define VAL_MAX 256
class Hold: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	Hold();
	~Hold();

private:
	Amor::Pius timer_ps;
	enum Sess_Status { IDLE = 0, INVALID =1, VALID = 2 };
	struct Sess {
		char name[VAL_MAX];	/* 所在域 */
		char domain[VAL_MAX];	/* 所在域 */
		char value[VAL_MAX];	/* 内容 */
		int  id;		/* 即下标 */
		Sess_Status status;	/* IDLE: 可再用; INVALID: 未认证; VALID: 已认证 */
		time_t when_create;
		bool matched;
		inline Sess() {
			memset(value, 0, VAL_MAX);
			memset(domain, 0, VAL_MAX);
			id = -1;
			status = IDLE;
			when_create = 0;
			matched = false;
		};

		inline void setnew(char *m_domain, char *sm)
		{
			memset(domain, 0,  VAL_MAX);
			memcpy(domain, m_domain, strlen(m_domain));
			memset(name, 0,  VAL_MAX);
			memcpy(name, sm, strlen(sm));
			status = INVALID;	/* 未认证 */
			matched = false; 	/* 还未被匹配 */
			time( &when_create);	/* 创立时间 */
			TEXTUS_SNPRINTF(value, 64, "%X%X%X", rand(), rand(), rand());
		};

	};
	
	struct G_CFG {
		TEXTUS_ORDO event_do;
		const char *sname;	/* cookie变量名 */
		int snlen;		/* sname的长度 */
		const char *path;
		time_t expired;
		unsigned int random_seed;

		Sess *sess;
		int seNum;
		
		const char** check_domains;
		int check_num;

		inline G_CFG (TiXmlElement *cfg) {
			const char *comm_str;
			TiXmlElement *ele;
			int i;

			sess = 0;
			seNum = 0;	
			snlen = 0;
	
			expired = 0;
			random_seed = 0;

			path = cfg->Attribute("path");
			sname = cfg->Attribute("name");
			if ( sname ) snlen = static_cast<int>(strlen(sname));

			comm_str = cfg->Attribute("expired");
			if (  comm_str && atoi(comm_str) > 1 )
				expired = atoi(comm_str);
			else
				expired = 30;

			event_do = Notitia::PRO_HTTP_HEAD;
			//comm_str = cfg->Attribute("event");
			//BTool::get_textus_ordo(&event, comm_str);
			event_do = Notitia::get_ordo (cfg->Attribute("event"));

			comm_str = cfg->Attribute("many");
			if (  comm_str && atoi(comm_str) > 0)
				seNum = atoi(comm_str);
			else
				seNum = 16;

			sess = new Sess[seNum];
			for ( i = 0 ; i < seNum; i++)
				sess[i].id = i;
	
			ele = cfg->FirstChildElement("domain");
			check_num = 0;
			while (ele)
			{
				if (ele->GetText()) check_num++;
				ele = ele->NextSiblingElement("domain");
			}

			if ( check_num > 0 )
				check_domains = new const char*[check_num];
			ele = cfg->FirstChildElement("domain"); i = 0;
			while (ele)
			{
				if (ele->GetText()) 
				{
					check_domains[i] = ele->GetText();
					i++;
				}
				ele = ele->NextSiblingElement("domain");
			}

			random_seed = static_cast<unsigned int> (time(0) & 0xFFFFFFFFL);
			srand(random_seed);
		};

		inline void expand() {
			struct Sess *old = sess;
			int oNum = seNum;
			int j;

			seNum *=2;
			sess = new Sess[seNum];
			memcpy(sess, old, sizeof(struct Sess) *oNum );

			for ( j = oNum ; j < seNum; j++)
				sess[j].id = j;
			delete []old;
		};

		inline ~G_CFG() {
			if ( sess )	
				delete[] sess;
		};
	};
	struct G_CFG *gCFG;	/* Shared for all objects in this node */
	bool has_config;

	int* addSession(char *domain); //返回下标值的指针
	void getsum(char *output, char *domain, const char *sn, int snl);

#include "wlog.h"
};

#include <assert.h>

void Hold::ignite(TiXmlElement *cfg)
{

	if (!cfg) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}

}

Hold::Hold()
{
	has_config = false;	/* 开始认为自己不拥有全局参数表 */
	gCFG = 0;
	timer_ps.ordo = Notitia::DMD_SET_TIMER;
	timer_ps.indic = this;
}

Hold::~Hold() 
{
	if ( has_config && gCFG)
		delete gCFG;
}

Amor* Hold::clone()
{
	Hold *child = new Hold();

	child->gCFG = gCFG;
	return (Amor*) child;
}

bool Hold::facio( Amor::Pius *pius)
{
	int j_count;
	char cienm[VAL_MAX+10];
	char *name = 0, *val =0 , *domain = 0;
	Amor::Pius new_hold, auth_hold, has_hold, clr_hold, ck, dm;
	time_t now;
	void *info[3];
	assert(pius);

	new_hold.ordo = Notitia::NEW_HOLDING;
	auth_hold.ordo = Notitia::AUTH_HOLDING;
	has_hold.ordo = Notitia::HAS_HOLDING;
	clr_hold.ordo = Notitia::CLEARED_HOLDING;
	ck.ordo = Notitia::GET_COOKIE;
	ck.indic = &info;
	dm.ordo =Notitia::GET_DOMAIN;

	if ( pius->ordo == gCFG->event_do)
	{
		WBUG("facio Notitia::" TLONG_FMT , gCFG->event_do);
		if ( !gCFG->sname )
			goto END;

		aptus->sponte(&dm);	/* 先取域名 */
		domain = (char*) dm.indic;
		if ( !domain )	/* 得不到域名 */
			goto END;

		for ( j_count = 0; j_count < gCFG->check_num; j_count++)
		{
			if (strcasecmp(domain, gCFG->check_domains[j_count]) == 0 )
				break;
		}

		if ( gCFG->check_num > 0 && j_count == gCFG->check_num )	/* 什么都未定义, 则都作认证 */
			goto END;

		/* 根据已设的session名称找某个cookie内容 */
		getsum(cienm, domain, gCFG->sname, gCFG->snlen);
		info[0] = (void*) cienm;
		info[1] = 0;
		info[2] = 0;
		aptus->sponte(&ck);
		if ( info[1] == 0 )
		{
			new_hold.indic = addSession(domain);
			aptus->facio(&new_hold);

		} else {
			/* 找到一个, 与当前会话表进行匹配 */
			name = (char*) info[0];
			val = (char*) info[1];
			
			for ( j_count = 0; j_count < gCFG->seNum; j_count++)
			{
				struct Sess &t = gCFG->sess[j_count];
				//printf("domain %s, name %s, val %s, t.value %s\n", domain, name, val, t.value);
				if ( t.status != IDLE && strcmp(val, t.value) ==0 && strcmp(domain, t.domain) ==0 && strcmp(name, t.name) == 0 )
					break;
			}

			if ( j_count == gCFG->seNum )
			{	/* session名有, 但不是当前会话表的, 重新设 */
				new_hold.indic = addSession(domain);
				aptus->facio(&new_hold);

			} else if ( gCFG->sess[j_count].status == INVALID ) {
				auth_hold.indic = &(gCFG->sess[j_count].id);
				aptus->facio(&auth_hold);

			} else if ( gCFG->sess[j_count].status == VALID ) {
				if ( !gCFG->sess[j_count].matched )
					gCFG->sess[j_count].matched = true;
				has_hold.indic = &(gCFG->sess[j_count].id);
				aptus->facio(&has_hold);
			}
		} 
	END:
		return true;
	}

	switch ( pius->ordo )
	{
	case Notitia::TIMER:
		WBUG("facio TIMER");
		time(&now);
		for ( int ik = 0; ik < gCFG->seNum; ik++)
		{
			struct Sess &t = gCFG->sess[ik];
			if ( t.status != IDLE  && t.when_create + gCFG->expired >= now)
			{
				t.status = IDLE;
				clr_hold.indic = &(t.value[0]);
				aptus->facio(&clr_hold);
			}
		}
		break;

	case Notitia::TIMER_HANDLE:
		WBUG("facio TIMER_HANDLE");
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		aptus->sponte(&timer_ps);
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		aptus->sponte(&timer_ps);
		break;

	default:
		return false;
	}
	return true;
}

bool Hold::sponte( Amor::Pius *pius)
{
	int i;

	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::CMD_SET_HOLDING:
		WBUG("sponte CMD_SET_HOLDING %d", *(reinterpret_cast<int*>(pius->indic)));
		i = *(reinterpret_cast<int*>(pius->indic));
		if ( i >= 0 && i < gCFG->seNum )
		{
			gCFG->sess[i].status = VALID;
			gCFG->sess[i].matched = false; 	/* 还未被匹配 */
		} else 
			pius->indic = (void *)0;
			
		break;

	case Notitia::CMD_CLR_HOLDING:
		WBUG("sponte CMD_CLR_HOLDING %d", *(reinterpret_cast<int*>(pius->indic)));
		i = *(reinterpret_cast<int*>(pius->indic));
		if ( i >= 0 && i < gCFG->seNum )
		{
			gCFG->sess[i].status = IDLE;
		} else 
			pius->indic = (void *)0;
			
		break;

	default:
		return false;
	}
	return true;
}

int* Hold::addSession( char *domain)
{
	int i;
	char cienm[VAL_MAX+10];
	struct SetCookie kie;
	Amor::Pius cook ;
	//Amor::Pius cook = {Notitia::SET_COOKIE, &kie};
	cook.ordo = Notitia::SET_COOKIE;
	cook.indic = &kie;

	for ( i = 0 ; i < gCFG->seNum && gCFG->sess[i].status != IDLE; i++ );

	if ( i == gCFG->seNum )
		gCFG->expand();

	getsum(cienm, domain, gCFG->sname, gCFG->snlen);
	gCFG->sess[i].setnew(domain, cienm);

	kie.name  = (char*) gCFG->sess[i].name;
	kie.value =  (char*) gCFG->sess[i].value;
	if ( *domain != '\0' )
		kie.domain = domain;
	kie.expires =  0;
	kie.path = (char*)gCFG->path;
	aptus->sponte(&cook);

	return &(gCFG->sess[i].id);
}

void Hold::getsum(char *output, char *dom, const char *snm, int snlen)
{
	char *p = dom;
	char *q;
	char c;

	memcpy(output, snm, snlen);
	q = &output[snlen];
	c = '\0';
	while ( *p != '\0' )
	{
		c = c ^ (*p);
		p++;
	}
	*q = Obtainx( (c & 0xF0) >> 4 );
	q++;
	*q = Obtainx((c & 0x0f));
	q++;
	*q = '\0';
}


#include "hook.c"
