/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Balance 
 Descr: 根据weight来决定, 从facio来的CMD_BEGIN_TRANS分配至某个loader上
 Build: created by octerboy, 2005/06/20, Panyu
 $Header: /textus/balance/Balance.cpp 12    12-04-04 16:52 Octerboy $
*/
#define SCM_MODULE_ID  "$Workfile: Balance.cpp $"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include <stdarg.h>

#define BALANCEINLINE inline
class Balance: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Amor::Pius*);
	bool sponte( Amor::Pius*);
	Amor *clone();
	
	Balance();
	~Balance();

private:
	int broad_num;	/* 需要向各通道广播的HERE_ORDO数目 */
	TEXTUS_ORDO *broad_ordos;/* 需要向各通道广播的HERE_ORDO集合 */
	int weight;	/* 负重值, -1表示不能负重, 0表示无任何负载, 值越大表示负载越大 */
	struct List { Balance *loader; List *next; };
	struct List l_list;
	struct List *loaders;

	bool isPoineer;
	bool canClone;
	bool power;
	inline void distribute(Amor::Pius *);
	inline bool bcast(Amor::Pius *);
	#include "wlog.h"
};

#include "casecmp.h"
#include <assert.h>

#define BZERO(X) memset(X, 0 ,sizeof(X))
void Balance::ignite(TiXmlElement *cfg)
{
	const char *broad_str, *cacu_str;
	TiXmlElement *b_ele;
	int i;

	cacu_str = cfg->Attribute("power");
	if ( cacu_str )
	{
		if ( strcasecmp(cacu_str, "yes") == 0 )
			power = true;
	}

	b_ele = cfg->FirstChildElement("broad"); broad_num = 0;
	while(b_ele)
	{
		b_ele = b_ele->NextSiblingElement("broad");
		broad_num++;
	}

	if ( broad_num > 0 )
	{
		broad_ordos = new TEXTUS_ORDO [broad_num];
	} else
	{
		broad_ordos = new TEXTUS_ORDO [1];
		broad_num = 1;
		broad_ordos[0] = Notitia::CMD_CANCEL_TRANS;
	}

	b_ele = cfg->FirstChildElement("broad"); i = 0;
	while(b_ele)
	{
		broad_str = b_ele->Attribute("ordo");
		//BTool::get_textus_ordo(&(broad_ordos[i]), broad_str);
		broad_ordos[i] = Notitia::get_ordo(broad_str);
		i++;
		b_ele = b_ele->NextSiblingElement("broad");
	}

	if (!loaders) 
	{
		loaders = new List;
		loaders->loader = this;
		loaders->next = 0;
	}
	isPoineer = true;
}

bool Balance::facio( Amor::Pius *pius)
{
	assert(pius);

	switch (pius->ordo )
	{
	case Notitia::CMD_BEGIN_TRANS:
		WBUG("facio CMD_BEGIN_TRANS");
		distribute(pius);
		break;

	case Notitia::CLONE_ALL_READY:
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE/CLONE_ALL_READY");
		{
			Amor::Pius tmp;
			tmp.ordo = Notitia::SET_WEIGHT_POINTER;
			tmp.indic = &weight;
			aptus->facio(&tmp);
		}
		break;

	default:
		//return bcast(pius);
		return true;
	}
	return true;
}

bool Balance::sponte( Amor::Pius *pius) 
{ 
	switch (pius->ordo )
	{
	case Notitia::START_SESSION:
		WBUG("sponte START_SESSION");
		weight = 0;
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("sponte DMD_END_SESSION");
		weight = -1;
		break;

	case Notitia::DMD_CLONE_OBJ:
		WBUG("sponte DMD_CLONE_OBJ");
		{
		Amor::Pius tmp;
		Amor *neo_child;
		canClone = true;	/* 允许clone */
		neo_child = aptus->clone();		/* 进行clone */
		canClone = false;	/* 禁止clone */
		tmp.ordo = Notitia::CMD_GET_OWNER;
		tmp.indic = 0;
		neo_child->sponte(&tmp);
		pius->indic = tmp.indic;
		}
		break;

	default:
		return false;
	}
	return true; 
}

Balance::Balance()
{
	weight = 0;
	broad_num = 0;
	broad_ordos = 0;
	canClone = false;
	power = false;

	loaders = 0;
	l_list.loader = this;
	l_list.next = 0;
	isPoineer = false;
}

Balance::~Balance()
{
	if (isPoineer ) 
	{
		if ( loaders != (List*) 0 ) 
			delete loaders;

		if ( broad_ordos )
			delete[] broad_ordos;
	}	
}

Amor* Balance::clone()
{
	Balance *child;
	struct List *n;

	if ( !canClone)
		return this; /* 通过canClone将此控制在自己手里 */

	n = loaders;
	child = new Balance();
	WBUG("cloned child %p\n", child);

	child->loaders = loaders;
	child->power = power;
	child->broad_num = broad_num ;
	child->broad_ordos = broad_ordos;

	/* 将child加到全局的负载者表中 */
	if ( n ) 
	{
		while(n->next) n = n->next;
		n->next = &(child->l_list);
	}
	return (Amor*)child;
}

void Balance::distribute(Amor::Pius *pius)
{
	struct List *n ;
	Balance *an, *idlist=0 ;
	int min = -1;
	/* 根据当前负载进行分配 */
	for ( n = loaders; n; n = n->next)
	{
		an = n->loader;
		assert(an);
		if ( an->weight < 0 ) continue;
		if ( an->weight < min  || min == -1 )
		{
			min = an->weight;
			idlist = an;
		}
	}

	if ( idlist )
	{
		if ( power ) 
		{ 	
			if ( min >= 0x00ffffff ) /* 负载值太大, 统一减负 */
			for ( n = loaders; n; n = n->next)
			{
				an = n->loader;
				if ( an->weight < 0 ) continue;
				an->weight -= min ;
			}
			idlist->weight++;
		}
		idlist->aptus->facio(pius);
	} else {
		WLOG(ERR, "no loader available");
	}
}

bool Balance::bcast(Amor::Pius *pius)
{
	struct List *n ;
	Balance *an;
	int i;
	for ( i =0 ; i < broad_num; i++ )
	{
		if ( pius->ordo == broad_ordos[i] )
		{
			for ( n = loaders; n; n = n->next)
			{
				an = n->loader;
				assert(an);
				an->aptus->facio(pius);
			}
			break;
		}
	}

	if ( i < broad_num )
		return true;
	else
		return false;
}
#include "hook.c"
