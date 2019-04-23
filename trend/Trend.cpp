/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Aptus extension, trend ordo
 Build: created by octerboy, 2007/03/09, Panyu
 $Id$
*/

#ifndef SCM_MODULE_ID
#define SCM_MODULE_ID  "$Id$"
#endif 

#ifndef TEXTUS_MODTIME
#define TEXTUS_MODTIME  "$Date$"
#endif 

#ifndef TEXTUS_BUILDNO
#define TEXTUS_BUILDNO  "$Revision$"
#endif
/* $NoKeywords: $ */

#include "Aptus.h"
#include "Notitia.h"
#include "textus_string.h"
#include "casecmp.h"

class Trend: public Aptus {
public:
	/* 以下是Amor 类定义的 */
	Amor *clone();

	/* 这是在Aptus中定义过的。*/
	void ignite_t	(TiXmlElement *wood, TiXmlElement *);	
	bool sponte_n	( Amor::Pius *, unsigned int );
	bool facio_n	( Amor::Pius *, unsigned int );
	bool dextra( Amor::Pius *, unsigned int );
	bool laeve( Amor::Pius *, unsigned int );

	/* 以下为本类特别定义 */
	Trend();
	~Trend();

	enum DIRECT { 	FACIO = 0 , SPONTE = 1, DEXTRA = 2 , LAEVE = 3 , PRI_LAEVE = 4, LEFT_LAEVE = 5,
			RIGHT_DEXTRA = 6, NEXT = 7, STILL = 8, OWNER = 9, SKIP = 10, NONE_DIR = -1};
	
protected:
	bool isPoineer;

	typedef struct _Action {
		TEXTUS_ORDO ordo;	/* 特别动作ordo, -1表示使用原有Pius */
		TEXTUS_ORDO vordo;	/* 对于ordo为CMD_CONTINUE_NEXT的, 
				vordo设定了(void(*)*indic)[1]指向Pius的ordo;
				vordo若为CMD_GET_PIUS, 则寻找另一个Pius
				 */
		DIRECT dir;	/* 动作方向, 默认STILL, 什么都不做 */
		DIRECT vdir;	/* 对于ordo为CMD_CONTINUE_NEXT的, 要求relay以dextra还是laeve方式调用. 
					FACIO或DEXTRA, 则调用dextra函数； SPONTE或LAEVE则laeve函数
				*/
		inline _Action () {
			ordo = Notitia::TEXTUS_RESERVED;
			dir = STILL;
			vdir = STILL;
		};
	} Action;

	typedef struct _Conie {
		TEXTUS_ORDO ordo;	/* 被抓的ordo 如果分类偏移不为0, 则将此左移NOTITIA_SUB_OFFSET (20)位 就是要的实际值了*/
		Action *actions;
		int actNum;
		bool goon;	/* 在动作之后是否继续, 默认为否 */
		inline _Conie() {
			ordo = Notitia::TEXTUS_RESERVED;
			goon = false;
			actions = (Action *)0;
			actNum = 0;
		};

		inline ~_Conie () 
		{
			actNum = 0;
			if ( actions )
				delete []actions;
		};
	} Conie;

	void get_conie (TiXmlElement *c_ele, Conie &cie) ;

	typedef struct _Condition {
		unsigned int num;	/* ordo数 */
		Conie *conies;
		inline _Condition () 
		{
			num = 0;
			conies = 0;
		};

		inline ~_Condition () 
		{
			num = 0;
			if ( conies )
				delete []conies;
		};
	} Condition; 

	Condition *con_fac;
	Condition *con_spo;
	Condition *con_dex;
	Condition *con_lae;

	bool get_condition (TiXmlElement *, const char*, Condition &);

	inline Conie *match(Condition *, Amor::Pius *);
	inline void doact(Action &, DIRECT, Amor::Pius *, int from );
	Amor::Pius* getP(Action &act);
 
	Amor::Pius ano_ps, ano_ps2;

	#include "tbug.h"
};

Trend::Trend() {
	WBUG("new this %p", this);

	con_fac = 0;
	con_spo = 0;
	con_dex = 0; 
	con_lae = 0;
	isPoineer = false;
}

Trend::~Trend() {
	WBUG("delete this %p", this );
	if ( isPoineer )
	{
		if(con_fac) delete con_fac;
		if(con_spo) delete con_spo;
		if(con_dex) delete con_dex;
		if(con_lae) delete con_lae;
	}
}

void Trend::ignite_t (TiXmlElement *cfg, TiXmlElement *sz_ele)
{	
	WBUG("this %p , prius %p, aptus %p, cfg %p, owner %p", this, prius, aptus, cfg, owner);

	if ( !sz_ele) 
		return;

	canAccessed = true;	/* 至此可以认为此模块处于Trend */
	isPoineer = true;

/* X: "sponte" etc.
   Y: spo etc.
*/

#define GETCON(X,Y)	\
	if (con_##Y)				\
		delete con_##Y;			\
	con_##Y = new Condition;		\
	need_##Y = get_condition(sz_ele, X, *con_##Y);	\

	GETCON("facio",  fac);
	GETCON("sponte", spo);
	GETCON("dextra", dex);
	GETCON("laeve",  lae);
}

bool Trend::get_condition (TiXmlElement *sz_ele, const char *dir, Condition &con) 
{
	TiXmlElement *con_ele;
	unsigned int i;

	if ( !sz_ele)
		return false;

	con_ele = sz_ele->FirstChildElement(dir); i = 0;
	while(con_ele)
	{
		con_ele = con_ele->NextSiblingElement(dir);
		i++;
	}

	if ( i > 0 )
	{
		con.conies = new Conie [i];
		con.num = i;
	} else {
		return false;
	}
	
	WBUG("%s num %d",dir, con.num);
	con_ele = sz_ele->FirstChildElement(dir); i = 0;
	for( ; con_ele; con_ele = con_ele->NextSiblingElement(dir), i++ )
	{
		get_conie(con_ele, con.conies[i]);
	}
	return true;
}

void Trend::get_conie (TiXmlElement *c_ele, Conie &cie) 
{
	const char *comm_str;
	TiXmlElement *act_ele;
	int i;
	cie.ordo = Notitia::get_ordo(c_ele->Attribute("ordo"));

	comm_str = c_ele->Attribute("goon");
	if ( comm_str )  
	{
		if( strcasecmp(comm_str, "yes") == 0 )
			cie.goon = true;
		else
			cie.goon = false;
	}

	act_ele = c_ele->FirstChildElement();
	i = 0;
	while(act_ele)
	{
		if (strcasecmp(act_ele->Value(), "facio") ==0 
		    ||	strcasecmp(act_ele->Value(), "sponte") ==0 
		    ||	strcasecmp(act_ele->Value(), "dextra") ==0 
		    ||	strcasecmp(act_ele->Value(), "laeve") ==0 
		    ||	strcasecmp(act_ele->Value(), "pri_laeve") ==0 
		    ||	strcasecmp(act_ele->Value(), "left_laeve") ==0 
		    ||	strcasecmp(act_ele->Value(), "right_dextra") ==0 
		    ||	strcasecmp(act_ele->Value(), "owner") ==0 
		    ||	strcasecmp(act_ele->Value(), "still") ==0 
		    ||	strcasecmp(act_ele->Value(), "next") ==0 )
		i++;
		act_ele = act_ele->NextSiblingElement();
	}

	if ( i > 0 )
	{
		cie.actions = new Action [i];
		cie.actNum = i;
	} else {
		return;
	}
	
	act_ele = c_ele->FirstChildElement(); i=0;
	for( ; act_ele; act_ele = act_ele->NextSiblingElement())
	{
		DIRECT here_dir = NONE_DIR;
		comm_str = act_ele->Value();
#define WHATDIR(X) if ( comm_str && strcasecmp(comm_str, #X) == 0 ) here_dir = X ;
		WHATDIR(FACIO)
		WHATDIR(SPONTE)
		WHATDIR(DEXTRA)
		WHATDIR(LAEVE)
		WHATDIR(PRI_LAEVE)
		WHATDIR(LEFT_LAEVE)
		WHATDIR(RIGHT_DEXTRA)
		WHATDIR(OWNER)
		WHATDIR(NEXT)
		WHATDIR(STILL)

		if ( here_dir == NONE_DIR )
			continue;

		cie.actions[i].dir = here_dir;
		cie.actions[i].ordo = Notitia::get_ordo(act_ele->Attribute("ordo"));
		cie.actions[i].vordo = Notitia::get_ordo(act_ele->Attribute("vordo"));
		
		here_dir = NONE_DIR;
		comm_str = act_ele->Attribute("vdir");  //vdir is here_dir, facio .. etc., here_dir is assigned to vdir
		WHATDIR(FACIO)
		WHATDIR(SPONTE)
		WHATDIR(DEXTRA)
		WHATDIR(LAEVE)
		cie.actions[i].vdir = here_dir;
		i++;
	}
}

Amor *Trend::clone() 
{
	Trend *child = 0;
	child = new Trend();
	Aptus::inherit((Aptus*) child);
#define Inherit(x) child->x = x;

	Inherit(con_fac);
	Inherit(con_spo);
	Inherit(con_dex);
	Inherit(con_lae);

	return  (Amor*)child;
}

/* X: SPONTE, DEXTRA 等
   Y: con_spo, con_dex 等
   Z: "sponte", "dextra" 等
*/

#define DO_TREND(X, Y, Z)		\
	WBUG("%s ordo %lu, owner %p", Z, pius->ordo, owner);	\
	Conie *cie;				\
						\
	cie =match(Y, pius);			\
	if (cie) 				\
	{					\
		for ( int i = 0; i < cie->actNum; i++ )		\
			doact(cie->actions[i], X, pius, from);	\
		return !(cie->goon);		\
	}					\
						\
	return false;				

bool Trend::sponte_n ( Amor::Pius *pius, unsigned int from)
{	/* 在owner向左发出数据前的处理 */
	DO_TREND(SPONTE, con_spo, "sponte")
}

bool Trend::facio_n ( Amor::Pius *pius, unsigned int from)
{	/* 在owner向右发出数据前的处理 */
	DO_TREND(FACIO, con_fac, "facio")
}

bool Trend::dextra( Amor::Pius *pius, unsigned int from)
{
	DO_TREND(DEXTRA, con_dex, "dextra")
}

bool Trend::laeve( Amor::Pius *pius, unsigned int from )
{
	DO_TREND(LAEVE, con_lae, "laeve")
}

/* 
  dir: 被什么函数调用, FACIO(facio), SPONTE(sponte), DEXTRA(dextra), LAEVE(laeve)
  from:	被调函数的from
*/
void Trend::doact(Action &act, DIRECT dir, Amor::Pius *ori, int from)
{
	bool isO;
	unsigned int i;
	Aptus **tor;

	void *ps[4];

	if ( act.ordo != Notitia::TEXTUS_RESERVED )
	{
		ano_ps.ordo = act.ordo;
		switch ( act.ordo ) 
		{
		case Notitia::DMD_CONTINUE_NEXT :
			ps[0] = this->owner;
			ps[1] = getP(act);

			switch( act.vdir )	/* 让relay是以dextra调用邻模块还是laeve */
			{
			case FACIO:
			case DEXTRA:
				ps[2] = (void*)0x0;
				break;

			case SPONTE:
			case LAEVE:
				ps[2] = (void*)0x1;
				break;

			default:
				if ( dir == LAEVE )
					ps[2] = (void*)0x1;
				else
					ps[2] = 0;
			}

			ps[3] = 0;
			ano_ps.indic = ps;

			break;
		default :
			ano_ps.indic = 0;
			break;
		}
		
		isO = false;

	} else 
		isO = true;

#define NEOP	(isO ? ori : &ano_ps)
	
	switch (act.dir )
	{
	case FACIO:
		((Aptus*)aptus)->facio_n (NEOP, 0);
		break;

	case SPONTE:
		((Aptus*)aptus)->sponte_n (NEOP, 0);
		break;

	case DEXTRA:
		((Aptus*)aptus)->dextra(NEOP, 0);
		break;

	case LAEVE:
		((Aptus*)aptus)->laeve(NEOP, 0);
		break;

	case PRI_LAEVE:
	case LEFT_LAEVE:
		if ( prius) 
			prius->laeve(NEOP, 0);
		break;

	case OWNER:
		switch( act.vdir )	/* 让relay是以dextra调用邻模块还是laeve */
		{
		case FACIO:
			owner->facio(NEOP);
			break;

		case SPONTE:
			owner->sponte(NEOP);
			break;

		default:

			switch ( dir )
			{
			case DEXTRA:
			case FACIO:
				owner->facio(NEOP);
				break;

			case LAEVE:
			case SPONTE:	/* owner发出sopnte, 再调用其sponte, 有意思? */
				owner->sponte(NEOP);
				break;
		
			default:
				break;

			}
			break;
		}
		break;
	
	case NEXT:	/* 往下走, Pius一般保持原值, 也有可能改变 */
		switch ( dir )
		{
		case FACIO:
			((Aptus*)aptus)->facio_n (NEOP, from+1);
			break;

		case SPONTE:
			((Aptus*)aptus)->sponte_n (NEOP, from+1);
			break;

		case DEXTRA:
			((Aptus*)aptus)->dextra(NEOP, from+1);
			break;

		case LAEVE:
			((Aptus*)aptus)->laeve(NEOP, from+1);
			break;

		default:
			break;

		}
		break;

	case RIGHT_DEXTRA:
		for (i= 0,tor = ((Aptus*)aptus)->compactor,((Aptus*)aptus)->aptus = (Aptus *)0; 
			i < ((Aptus*)aptus)->duco_num; i++, tor++)
		{
			(*tor)->dextra(NEOP, 0);
			if ( ((Aptus*)aptus)->aptus != (Aptus *)0 ) 
			{
				((Aptus*)aptus)->aptus->facio(NEOP);	/* the control follow turned to the Aptus extension */
				((Aptus*)aptus)->aptus = (Aptus *)0;
				break;
			}
		}
		break;
	case STILL:	/* 什么都不干 */
		break;

	default:
		break;
	}
}

/* 找到相应的Action */
Trend::Conie* Trend::match(Condition *cond, Amor::Pius *pius)
{
	unsigned int i;
	for ( i = 0 ; i < cond->num; i++ )
	{
		if (cond->conies[i].ordo == pius->ordo)
			return &(cond->conies[i]);
	}
	return (Conie*) 0;
}

Amor::Pius* Trend::getP(Action &act)
{
	Amor::Pius t_p;
	t_p.ordo = Notitia::CMD_GET_PIUS;
	Amor::Pius *sub_ps = 0;

	switch ( act.vordo)
	{
	case Notitia::TEXTUS_RESERVED :
		sub_ps = 0;
		break;

	case Notitia::CMD_GET_PIUS:
		aptus->sponte(&t_p);
		sub_ps = (Amor::Pius *)t_p.indic;
		break;

	default:
		ano_ps2.ordo = act.vordo;;
		ano_ps2.indic = 0;
		sub_ps = &ano_ps2;
		break;
	}
	return sub_ps;
}

#ifndef HAS_NO_HOOK
#define TEXTUS_APTUS_TAG { 'T', 'r','e','n','d',0};
#include "hook.c"
#endif
