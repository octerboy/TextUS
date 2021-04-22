/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Aptus extension, Ponder delay call after schedule.
 Build:created by octerboy, 2018/04/11, Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Aptus.h"
#include "Notitia.h"
#include "Describo.h"
class Ponder: public Aptus {
public:
	void ignite_t (TiXmlElement *wood, TiXmlElement *);
	Amor *clone();

	bool laeve( Amor::Pius *pius, unsigned int from);	//向左调用，调用aptus的父节点
	bool dextra( Amor::Pius *pius, unsigned int from);	//向左调用，调用aptus的父节点
	bool sponte_n (Amor::Pius *, unsigned int);
	bool facio_n ( Amor::Pius *, unsigned int );
	bool facio (Amor::Pius *);

	Ponder();
	enum DIRECT { 	FACIO = 0 , SPONTE = 1, DEXTRA = 2 , LAEVE = 3 , PRI_LAEVE = 4, LEFT_LAEVE = 5,
			RIGHT_DEXTRA = 6, NEXT = 7, STILL = 8, OWNER = 9, SKIP = 10, NONE_DIR = -1};
protected:

	TEXTUS_ORDO lae_do, dex_do, spo_do, fac_do;
	void get_sch();
	void put_sch(int dir, int from, Amor::Pius *);
	Amor *sch;

#include "tbug.h"
};

#include "textus_string.h"
#include "casecmp.h"
#include <stdio.h>

Ponder::Ponder()
{
	lae_do = Notitia::TEXTUS_RESERVED;
	dex_do = Notitia::TEXTUS_RESERVED;
	spo_do = Notitia::TEXTUS_RESERVED;
	fac_do = Notitia::TEXTUS_RESERVED;
	sch = 0;
}

void Ponder::ignite_t (TiXmlElement *cfg, TiXmlElement *cf_ele)
{
	WBUG("this %p, prius %p, aptus %p, cfg %p", this, prius, aptus, cfg);
	if ( !cf_ele) return;

#define WHORDO(Y, Z) Y = Notitia::get_ordo(cf_ele->Attribute(Z));

	WHORDO(lae_do, "laeve");
	WHORDO(dex_do, "dextra");
	WHORDO(spo_do, "sponte");
	WHORDO(fac_do, "facio");

	canAccessed = true;	/* 至此可以认为此应用模块需要Ponder */
	if ( lae_do != Notitia::TEXTUS_RESERVED )
		need_lae = true;

	if ( dex_do != Notitia::TEXTUS_RESERVED )
		need_dex = true;

	if ( fac_do != Notitia::TEXTUS_RESERVED )
		need_fac = true;

	if ( spo_do != Notitia::TEXTUS_RESERVED )
		need_spo = true;
	//{int *a=0; *a=0;}
}

bool Ponder::facio( Amor::Pius *pius)
{
	Amor::Pius ready2;
	ready2.ordo = Notitia::CLONE_ALL_READY;
	switch(pius->ordo)
	{
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		get_sch();
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		get_sch();
		break;

	case Notitia::DMD_SCHED_RUN:
		WBUG("facio DMD_SCHED_RUN");
#define POR ((struct Describo::Pendor*)pius->indic)
		switch ( POR->dir )
		{
		case FACIO:
			((Aptus*)aptus)->facio_n (POR->pius, POR->from);
			break;
	
		case SPONTE:
			((Aptus*)aptus)->sponte_n (POR->pius, POR->from);
			break;
	
		case DEXTRA:
			((Aptus*)aptus)->dextra(POR->pius, POR->from);
			break;
	
		case LAEVE:
			((Aptus*)aptus)->laeve(POR->pius, POR->from);
			break;
		default :
			break;
		}
		return true;
	}
	return true;
}

Amor *Ponder::clone()
{	
	Ponder *child = 0;
	child = new Ponder();
	Aptus::inherit( (Aptus*) child );
#define Inherit(x) child->x = x;

	Inherit(dex_do);
	Inherit(lae_do);
	Inherit(fac_do);
	Inherit(spo_do);
	return  (Amor*)child;
}

void Ponder::get_sch()
{
	Amor::Pius get;
	get.ordo = Notitia::CMD_GET_SCHED;
	aptus->sponte(&get);
	sch = (Amor*)get.indic;
}
void Ponder::put_sch(int dir, int from, Amor::Pius *pius)
{
	Amor::Pius put;
	struct Describo::Pendor por;
	put.ordo = Notitia::CMD_PUT_PENDOR;
	put.indic = &por;
	por.pupa = this;
	por.dir = dir;
	por.from = from;
	por.pius = pius;
	if ( sch)
		sch->sponte(&put);
}

bool Ponder::dextra( Amor::Pius *pius, unsigned int from)
{	/* 进入owner->facio()之前 */
	
	WBUG("dextra Notitia::" TLONG_FMTu " owner is %p, pius=%p", pius->ordo, owner, pius);
	if ( pius->ordo == dex_do )
	{
		put_sch(DEXTRA, from+1,pius);
		return true;
	} else
		return false;
}

bool Ponder::laeve( Amor::Pius *pius, unsigned int from)
{	/* 进入owner->sponte()之前的处理 */
	WBUG("laeve Notitia:::" TLONG_FMTu " owner is %p, pius=%p", pius->ordo, owner, pius);
	if ( pius->ordo == dex_do )
	{
		put_sch(LAEVE, from+1,pius);
		return true;
	} else
		return false;
}

bool Ponder::sponte_n ( Amor::Pius *pius, unsigned int from)
{	/* 在owner向左发出数据前的处理 */
	WBUG("sponte Notitia:::" TLONG_FMTu " owner is %p, pius=%p", pius->ordo, owner, pius);
	if ( pius->ordo == spo_do )
	{
		put_sch(SPONTE, from+1,pius);
		return true;
	} else
		return false;
}

bool Ponder::facio_n ( Amor::Pius *pius, unsigned int from)
{	/* 在owner向右发出数据前的处理 */
	WBUG("facio Notitia:::" TLONG_FMTu " owner is %p, pius=%p", pius->ordo, owner, pius);
	if ( pius->ordo == fac_do )
	{
		put_sch(FACIO, from+1, pius);
		return true;
	} else
		return false;
}

#define TEXTUS_APTUS_TAG { 'P','o','n','d','e','r',0}
#include "hook.c"
