/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Aptus extension, aux proccess between parent node and child node
 Build: created by octerboy, 2006/04/01, Wuhan
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Aptus.h"
#include "Notitia.h"
#include "Animus.h"
#include "textus_string.h"
#include "casecmp.h"

class Auxi: public Aptus {
public:
	void ignite_t(TiXmlElement *wood, TiXmlElement *);
	Amor *clone();

	bool laeve( Amor::Pius *pius, unsigned int from);
	bool dextra( Amor::Pius *pius, unsigned int from);
	bool facio_n(Amor::Pius *, unsigned int from);
	bool sponte_n(Amor::Pius *, unsigned int from);
	bool facio(Amor::Pius *);

	Auxi();
	~Auxi();
	
protected:
	bool bcall( Amor::Pius *pius, unsigned int from, const char *);
	Animus *assist;		/* assistant */
	struct Oes  {
		unsigned long ordo;	/* laeveʱ��Ҫcatch��ordo */
		bool all_over;		/* true: laeve(��sponte)ʱ�������ÿ��compactor, ����ֻ�Ե�һ����laeve, Ĭ��false */
		inline Oes () {
			ordo = Notitia::TEXTUS_RESERVED;
			all_over = true;
		};
	} ;

	struct G_CFG {
		bool has_pri;		/* true: assist�о����뱾aptus��ͬ��prius, Ĭ��Ϊfalse */

		int oNum;	/* laeveʱ��Ҫcatch��ordo �� */
		struct Oes *oes;
		bool cat_dex;
		bool cat_fac;
		bool cat_spo;
		bool cat_lae;

		inline G_CFG () {
			has_pri = false;
			oNum = 0;	/* ������ */
			oes = 0;

			cat_dex = false;
			cat_fac = true;
			cat_lae = true;
			cat_spo = false;
		};

		inline ~G_CFG() {
			if (oes)	
				delete[] oes;
		}
	};
	struct G_CFG *gCFG;
	bool has_config;
	#include "tbug.h"
};

#include "Notitia.h"
#include <stdio.h>

void Auxi::ignite_t (TiXmlElement *cfg, TiXmlElement *aux_ele)
{	/* ��carbo��֪��Ӧ��ģ���Ƿ���ҪAuxi */
	const char *comm_str;		
	int i;
	TiXmlElement *o_ele;
	WBUG("this %p , prius %p, aptus %p, cfg %p", this, prius, aptus, cfg);
	
	if ( !aux_ele) return;
	/* һ��aux_ele�൱��һ��XML�ļ��е�rootԪ�� */
	if ( assist ) delete assist;
	assist = new Animus;

	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
	}

	o_ele = aux_ele->FirstChildElement("catch"); gCFG->oNum = 0;
	while(o_ele)
	{
		if ( o_ele->Attribute("ordo") )
			gCFG->oNum++;
		o_ele = o_ele->NextSiblingElement("catch");
	}

	if ( gCFG->oNum > 0 )
	{
		gCFG->oes = new struct Oes [gCFG->oNum];
	}

	o_ele = aux_ele->FirstChildElement("catch"); i = 0;
	while(o_ele)
	{
		comm_str = o_ele->Attribute("allover");
		if ( comm_str && strcasecmp(comm_str, "no" ) == 0 )
			gCFG->oes[i].all_over = false;

		comm_str = o_ele->Attribute("ordo");
		if ( comm_str )
		{
			//BTool::get_textus_ordo(&(gCFG->oes[i].ordo), comm_str);
			gCFG->oes[i].ordo = Notitia::get_ordo(comm_str);
			if ( gCFG->oes[i].ordo != Notitia::TEXTUS_RESERVED )
				i++;
		}
		o_ele = o_ele->NextSiblingElement("catch");
	}

	if ( aux_ele->Attribute("tag"))
		TEXTUS_STRNCPY(assist->module_tag, aux_ele->Attribute("tag"), sizeof(assist->module_tag)-2);
	else
		TEXTUS_STRCPY(assist->module_tag, "Module");

	comm_str = aux_ele->Attribute("prius");
	if ( comm_str && strcasecmp(comm_str, "yes" ) == 0 )
		gCFG->has_pri = true;

	comm_str = aux_ele->Attribute("sponte");
	if ( comm_str && strcasecmp(comm_str, "yes" ) == 0 )
		gCFG->cat_spo = true;

	comm_str = aux_ele->Attribute("laeve");
	if ( comm_str && strcasecmp(comm_str, "no" ) == 0 )
		gCFG->cat_lae = false;

	comm_str = aux_ele->Attribute("dextra");
	if ( comm_str && strcasecmp(comm_str, "yes" ) == 0 )
		gCFG->cat_dex = true;

	comm_str = aux_ele->Attribute("facio");
	if ( comm_str && strcasecmp(comm_str, "no" ) == 0 )
		gCFG->cat_fac = false;

	assist->ignite(aux_ele);

	canAccessed = true;	/* ���˿�����Ϊ��Ӧ��ģ����ҪAuxi */
	need_lae = gCFG->cat_lae;
	need_fac = gCFG->cat_fac;
	need_dex = gCFG->cat_dex;
	need_spo = gCFG->cat_spo;
}

Amor *Auxi::clone()
{	
	Auxi *child = 0;
	child = new Auxi();
	WBUG("clone %p, child %p", this, child);
	Aptus::inherit((Aptus*) child);
	child->gCFG = gCFG;

	if ( assist ) 
		child->assist = (Animus*) (assist->clone());
	
	return  (Amor*)child;
}

bool Auxi::facio(Amor::Pius *pius)
{
	Amor::Pius ready = {Notitia::IGNITE_ALL_READY,0,0};

	Amor::Pius ready2 = {Notitia::CLONE_ALL_READY,0,0};

	switch ( pius->ordo ) 
	{
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY owner %p", owner);
		if ( gCFG->has_pri )
			assist->prius  = ((Aptus*)aptus)->prius;
		assist->info (ready);
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY owner %p", owner);
		if ( gCFG->has_pri )
			assist->prius  = ((Aptus*)aptus)->prius;
		assist->info (ready2);
		break;

	default:
		return false;
	}
	return true;
}

bool Auxi::laeve( Amor::Pius *pius, unsigned int from)
{	/* ����owner->sponte()֮ǰ�Ĵ��� */
	return bcall(pius, from, "laeve");
}

bool Auxi::sponte_n ( Amor::Pius *pius, unsigned int from)
{	/* ����owner->sponte()֮ǰ�Ĵ��� */
	return bcall(pius, from, "sponte");
}

bool Auxi::bcall( Amor::Pius *pius, unsigned int from, const char *act)
{
	int i;
	bool allowed;
	bool all_over;
	WBUG("%s owner %p, num is %d, ordo %d", act, owner, assist->duco_num, pius->ordo);

	/*��ÿ�������߶�����һ�� */
	allowed = true; all_over = true;
	if ( gCFG->oNum > 0 )
	{
		allowed = false;
		for ( i = 0; i < gCFG->oNum; i++)
		{
			if ( gCFG->oes[i].ordo == pius->ordo)
			{
				allowed = true;
				all_over =  gCFG->oes[i].all_over;
				break;
			}
		}
	}

	if ( allowed)
	{
		if  (all_over )
		{
			for ( i = assist->duco_num -1  ; i >= 0; i--)
				(assist->compactor[i])->laeve(pius,0);
		} else {	
			/* ֻ�ǶԵ�һ����laeve�� ���ж��compactor ʱ, Ramify�е�continue="yes"���¶���һ���ڽڵ�ĵ��� */
			if (  assist->duco_num > 0 )
				(assist->compactor[0])->laeve(pius,0);
		}
	}

	WBUG("%s(end) owner %p, duco_num %d, ordo %d", act, owner, assist->duco_num, pius->ordo);
	return false;	/* �����partner�ɼ������� */
}

bool Auxi::facio_n (Amor::Pius *pius, unsigned int from)
{	/* ��owner->facio()֮ʱ */
	WBUG("facio owner %p, duco_num %d, ordo %d", owner, assist->duco_num, pius->ordo);
	assist->facio_n (pius,0);

	return false;
}

bool Auxi::dextra(Amor::Pius *pius, unsigned int from)
{	/* ��owner->facio()֮ʱ */
	WBUG("dextra owner %p, duco_num %d, ordo %d", owner, assist->duco_num, pius->ordo);
	assist->dextra(pius,0);

	return false;
}

Auxi::Auxi()
{
	assist = (Animus*) 0;
	has_config = false;
	gCFG = 0;
}

Auxi::~Auxi()
{ 
	if (assist ) delete assist;
}

#define TEXTUS_APTUS_TAG { 'A', 'u', 'x', 0 }
#include "hook.c"
