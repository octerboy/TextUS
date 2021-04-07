/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Aptus extension, call this module depending on uri
 Build: created by octerboy, 2006/07/14，Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Aptus.h"
#include "BTool.h"
#if defined(_MSC_VER) && (_MSC_VER >= 1900 )
#include <regex>
using namespace std;
#else
#include <regex.h>
#endif
#include "TBuffer.h"
#include "Notitia.h"
#include "PacData.h"

#ifndef TINLINE
#define TINLINE inline
#endif
class Ramify: public Aptus {
public:
	void ignite_t (TiXmlElement *wood, TiXmlElement *);
	Amor *clone();

	Ramify();
	~Ramify();
	bool dextra	(Amor::Pius *, unsigned int);
	bool laeve	(Amor::Pius *, unsigned int);
	bool sponte_n	(Amor::Pius *, unsigned int);
	bool facio_n	(Amor::Pius *, unsigned int);

	Amor::Pius stop;
	Amor::Pius goon;

#include "tbug.h"

enum WHDir {	/* 处理方向 */
	DEXTRA, LAEVE, SPONTE
};

enum MatchType {	/* 匹配类型 */
	NO_MATCH=0,	/* 无 */	
	VAR_ANY	=1,	/* 任意 */	
	CONSTANT=2,	/* 完全相同 */
	BEGIN_W	=3,	/* 以某个开头 */
	END_W	=4,	/* 以某个结尾 */
	REGEX	=5	/* regular表达式 */
};

typedef struct _FldDef {
	bool hnot;	/* true: 此域判断结果取反 */
	int no;			/* 域号, < 0 :未定义, >=0 : 有定义, 与数组下标相同 */
	unsigned int m_num; 	/* 规则数, 若为0, 则表明此域必须存在而不管内容如何  */
	enum { PFirst, PSecond } what;	/* 对哪个packet的判断? */
	struct Match {
		MatchType type;  /* 域内容的匹配类型  */
		unsigned int len;	/* 内容长度 */
		unsigned char *val;	/*  匹配规则内容  */
		bool NOT;	/* 结果取反 */
#if defined(_MSC_VER) && (_MSC_VER >= 1900 )
		regex rgx;
#else
		regex_t rgx;	//规则
#endif
		Match () {
			NOT = false;
		};
	} *match;

	inline ~_FldDef () {
		if ( m_num > 0 )
		{	
			delete [] match[0].val;
			delete [] match;
		}
	}
} FldDef;

private:
	PacketObj *rcv_pac;	/* 来自左节点的PacketObj */
	PacketObj *snd_pac;
	PacketObj *r1st_pac;	/* 来自右节点的PacketObj */
	PacketObj *r2nd_pac;

	typedef struct _PacDef {
		FldDef *fldDef;
		unsigned int fldNum;
		bool last_not;		/* 判断结果最后取反 */
		inline _PacDef () {
			fldDef= 0;
			fldNum = 0;
			last_not = false;
		};
		inline ~_PacDef () {
			if ( fldDef ) 
				delete []fldDef;
		};
	} PacDef;

	typedef struct _RDef {
		PacDef *mdef;		/* fields集合阵列 */
		unsigned int setn;	/* fields集合数,通常为1 */
		int cycle;	/* 重复执行的次数 */
		TEXTUS_ORDO ordo;	/* 除了PRO_UNIPAC外的一个被抓的ordo */
		inline _RDef() {
			setn = 0;
			mdef = 0;
			cycle = 1;
			ordo = Notitia::TEXTUS_RESERVED;
		};
		inline ~_RDef() {
			if (mdef)
				delete[] mdef;

		};
	} RDef;

	typedef struct _GCFG {
		RDef dex_def;
		RDef spo_def;
		RDef fac_def;
		RDef lae_def;
		bool should_stop;	/* dextra 时, 在匹配之后是否停止对邻节点的调用 */
		bool should_goon;	/* sponte, laeve 时, 若不匹配, 是否转到对邻节点的访问 */
		bool inverse;
		inline _GCFG() {
			should_stop = false;
			should_goon = false;
			inverse = false;
		};
	} GCFG;

	GCFG *gCFG;
	bool hasConfig;
	
	TINLINE bool do_match(FieldObj &field, FldDef &fld_def);
	TINLINE bool tryAllSets( PacDef *mdef, unsigned setn );
	TINLINE bool tryOneSet( PacDef *pdef);
	TINLINE bool dirPro(RDef &, Amor::Pius *pius, unsigned int from, WHDir dir);

	TINLINE bool getPacDef(RDef &, TiXmlElement *);
	TINLINE void setDef(PacDef *pdef, TiXmlElement *fds_ele, const char*);
};

#include "tbug.h"
#include "casecmp.h"
#include "textus_string.h"

Ramify::Ramify()
{
	hasConfig = false;
	gCFG = 0;

	rcv_pac = 0;
	snd_pac = 0;

	r1st_pac = 0;
	r2nd_pac = 0;

	stop.ordo = Notitia::DMD_STOP_NEXT;
	stop.indic = 0;
	
	goon.ordo = Notitia::DMD_CONTINUE_NEXT;
}

Ramify::~Ramify()
{
	if (hasConfig && gCFG)
		delete gCFG;
}

void Ramify::ignite_t (TiXmlElement *cfg, TiXmlElement *prop)
{	
	const char *comm_str;

	WBUG("prius %p, aptus %p, cfg %p", prius, aptus, cfg);
	if ( !prop) return;

	if ( !gCFG )
	{
		gCFG = new GCFG;
		hasConfig = true;
	}
	
	comm_str = prop->Attribute("stop");
	if ( comm_str && strcasecmp(comm_str, "yes") == 0 )
		gCFG->should_stop = true;

	comm_str = prop->Attribute("continue");
	if ( comm_str && strcasecmp(comm_str, "yes") == 0 )
		gCFG->should_goon = true;

	gCFG->inverse = false;
	comm_str = prop->Attribute("inverse");
	if ( comm_str && strcasecmp(comm_str, "yes") == 0 )
		gCFG->inverse = true;

	need_dex = getPacDef( gCFG->dex_def, prop->FirstChildElement("dextra"));
	need_spo = getPacDef( gCFG->spo_def, prop->FirstChildElement("sponte"));

	need_lae = getPacDef( gCFG->lae_def, prop->FirstChildElement("laeve"));
	need_fac = getPacDef( gCFG->fac_def, prop->FirstChildElement("facio"));

	canAccessed = need_dex || need_spo || need_lae || need_fac;
/*
	if ( prop->Attribute("bug"))
	{
		int *a =0; *a = 0;
	}
*/
}

bool Ramify::getPacDef(RDef &rd, TiXmlElement *fds_ele)
{
	TiXmlElement *fset_ele;
	unsigned int fset_num;
	bool has_sub;
	int i;

	if ( !fds_ele )
		return false;

	has_sub = false;
	fds_ele->QueryIntAttribute("cycle", &rd.cycle);
	rd.ordo = Notitia::get_ordo(fds_ele->Attribute("ordo"));

	/* 先看一下有多少个域判断集 */
	fset_ele = fds_ele->FirstChildElement("fields"); fset_num = 0;
	while ( fset_ele)
	{
		fset_num++;
		fset_ele = fset_ele->NextSiblingElement("fields");
	}

	if ( fset_num > 0 )
		has_sub = true;
	else 
		fset_num = 1;	/* 以兼容没有fields元素而直接就是一系列field的情况: 即相当于只有一个fields */
	
	rd.setn = fset_num;
	rd.mdef = new PacDef[fset_num];

	if ( !has_sub )
	{	/* 非fields 的情况 */
		setDef(&rd.mdef[0], fds_ele, fds_ele->Attribute("packet"));
	} else {
		/* 有fields的情况 */
		for (	i= 0 , fset_ele = fds_ele->FirstChildElement("fields"); 
			fset_ele;
			i++, fset_ele = fset_ele->NextSiblingElement("fields") )
		{
			setDef(&rd.mdef[i], fset_ele, fds_ele->Attribute("packet"));
		}
	}
	return true;
}


void Ramify::setDef(PacDef *pdef, TiXmlElement *fdset_ele, const char *which_pac)
{
	const char *comm_str;
	TiXmlElement *p_ele, *m_ele;
	FldDef *fdef;
	int total_len, i;
	unsigned int k;

	pdef->last_not = false;
	comm_str = fdset_ele->Attribute("last_not");
	if ( comm_str && strcasecmp(comm_str, "yes") == 0 )
		pdef->last_not = true;

	p_ele = fdset_ele->FirstChildElement("field"); pdef->fldNum = 0; 
	while ( p_ele)
	{
		comm_str = p_ele->Attribute("no");
		if ( comm_str && atoi(comm_str) >= 0 )
			pdef->fldNum++;

		p_ele = p_ele->NextSiblingElement("field");
	}

	if ( pdef->fldNum > 0 )
		pdef->fldDef = new FldDef[pdef->fldNum];
	else 
		goto END;

	p_ele = fdset_ele->FirstChildElement("field"); i = 0; fdef = &pdef->fldDef[0];
	while(p_ele)
	{
		comm_str = p_ele->Attribute("no");
		if ( !( comm_str && atoi(comm_str) >= 0 ) )
			goto NEXTELE;

		fdef->no = atoi(comm_str);

		if ( !gCFG->inverse )
			fdef->what = FldDef::PFirst;	/* 默认为判断第一个Pac */
		else 
			fdef->what = FldDef::PSecond;	/* 默认为判断第二个Pac */

		comm_str = fdset_ele->Attribute("packet");	/* 全局定义 */
		if ( !comm_str )
			comm_str = which_pac;		/* 如果没有, 取更上一层定义 */

		if ( comm_str && strcasecmp(comm_str, "second") == 0 )
			fdef->what = FldDef::PSecond;

		if ( comm_str && strcasecmp(comm_str, "first") == 0 )
			fdef->what = FldDef::PFirst;

		comm_str = p_ele->Attribute("packet");	/* 局部定义 */
		if ( comm_str && strcasecmp(comm_str, "second") == 0 )
			fdef->what = FldDef::PSecond;

		if ( comm_str && strcasecmp(comm_str, "first") == 0 )
			fdef->what = FldDef::PFirst;

		fdef->hnot = false;
		comm_str = p_ele->Attribute("NOT");	/* 逻辑取反 */
		if ( comm_str && strcasecmp(comm_str, "yes") == 0 )
			fdef->hnot = true;

		/* 处理匹配规则 */
		total_len = 0;
		fdef->m_num = 0;
		for ( m_ele = p_ele->FirstChildElement(); m_ele;
			m_ele = m_ele->NextSiblingElement())
		{
			fdef->m_num++;
			total_len += 1;
			if ( m_ele->GetText())
				total_len += static_cast<int>(strlen(m_ele->GetText()));
		}
		if ( fdef->m_num == 0)
			goto NEXTFLD;
		
		fdef->match = new FldDef::Match [fdef->m_num];
		fdef->match[0].val = new unsigned char[total_len];
		memset(fdef->match[0].val, 0 , total_len);
		for ( m_ele = p_ele->FirstChildElement(), k=0; m_ele;
			m_ele = m_ele->NextSiblingElement())
		{
			FldDef::Match &match = fdef->match[k];
			match.type = NO_MATCH;
			match.NOT = false;
			if ( strcasecmp ( m_ele->Value(), "match" ) == 0  ) {
				match.NOT = false;
				match.type = CONSTANT;	/* 默认为恒相等 */
			}
			if ( strcasecmp ( m_ele->Value(), "dismatch" ) == 0 ) {
				match.NOT = true;
				match.type = CONSTANT;	/* 默认为恒相等 */
			}
			if ( strcasecmp ( m_ele->Value(), "any" ) == 0 ) {
				match.NOT = false;
				match.type = VAR_ANY; /* 任何的存在 */
			}
			if ( strcasecmp ( m_ele->Value(), "not_any" ) == 0 ) {
				match.NOT = true;
				match.type = VAR_ANY; /* 任何的存在 */
			}
			if ( strcasecmp ( m_ele->Value(), "begin" ) == 0 ) {
				match.NOT = false;
				match.type = BEGIN_W; 
			}
			if ( strcasecmp ( m_ele->Value(), "not_begin" ) == 0 ) {
				match.NOT = true;
				match.type = BEGIN_W; 
			}
			if ( strcasecmp ( m_ele->Value(), "end" ) == 0 ) {
				match.NOT = false;
				match.type = END_W; 
			}
			if ( strcasecmp ( m_ele->Value(), "not_end" ) == 0 ) {
				match.NOT = true;
				match.type = END_W; 
			}
			if ( strcasecmp ( m_ele->Value(), "regex" ) == 0 ) {
				match.NOT = false;
				match.type = REGEX; 
			}
			if ( strcasecmp ( m_ele->Value(), "not_regex" ) == 0 ) {
				match.NOT = true;
				match.type = REGEX; 
			}
			if ( match.type == REGEX) 
			{
#if defined(_MSC_VER) && (_MSC_VER >= 1900 )
				try {
					match.rgx = (char*) match.val;
				} catch (regex_error& e ) {
					WBUG("regex error %s at %s", e.what(), (char*) match.val);
					match.type = VAR_ANY;	/* 如果规则不合法，就设为无限制 */
					//printf("%s\n", e.what());
				}
#else
				if ( regcomp(&match.rgx, (const char*)match.val, 0) != 0 )
				{
					match.type = VAR_ANY;	/* 如果规则不合法，就设为无限制 */
					WBUG("regex error in %s", (char*) match.val);
				}
#endif
			}
	
			if ( match.type == NO_MATCH) continue;
			match.len = BTool::unescape(m_ele->GetText(), match.val) ;
			if ( match.len == 0 && match.type != VAR_ANY ) continue; /* 没有内容就无效, except for VAR_ANY */

			if ( k+1 < fdef->m_num )
				fdef->match[k+1].val  = match.val + match.len + 1;
			k++;	/* 下一个*/
		}
		fdef->m_num = k;	//再次更新
NEXTFLD:
			
		i++; fdef++;
NEXTELE:
		p_ele = p_ele->NextSiblingElement("field"); 
	}
END:	
	return ;
}

Amor *Ramify::clone()
{	
	Ramify *child = 0;
	child = new Ramify();
	Aptus::inherit( (Aptus*)child );

	child->gCFG = gCFG;
	
	return  (Amor*)child;
}

bool Ramify::dirPro(RDef &rd, Amor::Pius *pius, unsigned int from, WHDir dir)
{
	int k;
	bool matched;
	
	/* 通常这里会执行1次 */
	for  ( k = 0, matched = false; k < rd.cycle ; k++)
	{
		if ( tryAllSets(rd.mdef, rd.setn) )
		{
			matched = true;
			switch ( dir )
			{
			case DEXTRA:
				WBUG("dextra matched for owner %p", owner );
				((Aptus *) aptus)->dextra(pius, from+1);	/* 继续完成对本owner的访问, 不影响其它Assistant */
				break;

			case LAEVE:
				WBUG("laeve matched for owner %p", owner );
				((Aptus *) aptus)->laeve(pius, from+1);	/* 继续完成对本owner的访问, 不影响其它Assistant */
				break;
			
			case SPONTE:
				WBUG("sponte matched for owner %p", owner );
				((Aptus *) aptus)->sponte_n (pius, from+1);	/* 继续完成对本owner的访问, 不影响其它Assistant */
				break;
			
			default:
				break;
			}
		} else
			break;	/* 一旦不匹配, 即终止 */
	}

	return matched;
}

bool Ramify::dextra(Amor::Pius *pius, unsigned int from)
{
	PacketObj **tmp;
	bool matched;

	switch (pius->ordo)
	{
	case Notitia::SET_UNIPAC:
		WBUG("dextra SET_UNIPAC");
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) rcv_pac = *tmp; 
			else {
				WBUG("dextra SET_UNIPAC rcv_pac null");
			}
			tmp++;
			if ( *tmp) snd_pac = *tmp;
			else {
				WBUG("dextra SET_UNIPAC snd_pac null");
			}
		} else {
			WBUG("dextra SET_TBUF null");
		}
		return false;

	case Notitia::PRO_UNIPAC:
		WBUG("dextra PRO_UNIPAC owner is %p", owner);
	MA:
		matched = dirPro(gCFG->dex_def, pius, from, DEXTRA);

		if ( matched && gCFG->should_stop)	/* 如果有匹配后要求不再访问下一个 */
			if( prius)  prius->laeve(&stop, 0) ;	/* 终止对下一个节点的访问 */

		break;
			
	default:
		if ( pius->ordo == gCFG->dex_def.ordo )
		{
			WBUG("dextra Notitia::" TLONG_FMT " owner is %p", pius->ordo, owner);
			goto MA;
		}
		return false;
	}
	return true;
}

bool Ramify::facio_n (Amor::Pius *pius, unsigned int from)
{
	PacketObj **tmp;

	switch (pius->ordo)
	{
	case Notitia::SET_UNIPAC:
		WBUG("facio SET_UNIPAC");
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) r1st_pac = *tmp; 
			else
				WBUG("laeve SET_UNIPAC rcv_pac null");
			tmp++;
			if ( *tmp) r2nd_pac = *tmp;
			else
				WBUG("laeve SET_UNIPAC snd_pac null");
		} else 
			WBUG("facio SET_UNIPAC null");

		return false;
	default:
		return false;
	}
	return true;
}

bool Ramify::sponte_n (Amor::Pius *pius, unsigned int from)
{
	bool matched;
	void *gpius[4];

	switch (pius->ordo)
	{
	case Notitia::PRO_UNIPAC:
		WBUG("sponte PRO_UNIPAC owner is %p", owner);
	MA:
		matched = dirPro(gCFG->spo_def, pius, from, SPONTE);
		if ( !matched && gCFG->should_goon && prius )	/* 如果一个都不匹配 */
		{
			goon.indic = gpius;;
			gpius[0] =  this->owner;
			gpius[1] =  pius;
			gpius[2] =  0;		/* 这里为0, relay模块会以dextra方式调用下一个邻节点 */
			gpius[3] =  0;

			prius->laeve(&goon, 0) ;	/* 要求上级节点访问下一个邻节点 */
		}
		break;
	default:
		if ( pius->ordo == gCFG->spo_def.ordo )
		{
			WBUG("sponte Notitia::" TLONG_FMT " owner is %p", pius->ordo, owner);
			goto MA;
		}
		return false;
	}

	return true;
}
			
bool Ramify::laeve(Amor::Pius *pius, unsigned int from)
{
	PacketObj **tmp;
	bool matched;
	void *gpius[4];

	switch (pius->ordo)
	{
	case Notitia::PRO_UNIPAC:
		WBUG("laeve PRO_UNIPAC owner is %p", owner);
	MA:
		matched = dirPro(gCFG->lae_def, pius, from, LAEVE);
		if ( !matched && gCFG->should_goon && prius )	/* 如果一个都不匹配 */
		{
			goon.indic = gpius;;
			gpius[0] = this->owner;
			gpius[1] = pius;
			gpius[2] = (gCFG->inverse ? (void*)0x1: (void*)0x0);	/* 这与relay模块相应, inverse (0x1) 情况下,
									relay 或以laeve方式调用下一个邻节点,
									否则是以dextra方式调用.
									*/
			gpius[3] = 0;
				
			prius->laeve(&goon, 0) ;	/* 发求上级节点访问下一个邻节点 */
		}

		break;
			
	case Notitia::SET_UNIPAC:
		WBUG("laeve SET_UNIPAC");
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) r1st_pac = *tmp; 
			else
				WBUG("laeve SET_UNIPAC rcv_pac null");
			tmp++;
			if ( *tmp) r2nd_pac = *tmp;
			else
				WBUG("laeve SET_UNIPAC snd_pac null");
		} else 
			WBUG("sponte SET_UNIPAC null");

		return false;
	default:
		if ( pius->ordo == gCFG->lae_def.ordo )
		{
			WBUG("laeve Notitia::" TLONG_FMT " owner is %p", pius->ordo, owner);
			goto MA;
		}
		return false;
	}
	return true;
}

/* 尝试各个集合 */
bool Ramify::tryAllSets( PacDef *pdef, unsigned n)
{
	PacDef *adef;
	unsigned int i;
	for ( adef = pdef, i=0; i < n; adef++, i++)
	{
		if ( tryOneSet(adef) )	/* 如果匹配一个集, 就算匹配了 */
			return true;
	}
	return false;
}

/* 匹配一个 域集合 */
bool Ramify::tryOneSet( PacDef *pdef)
{
	bool matched ;
	PacketObj *test;
	FldDef *fdef;
	unsigned int i;

	if ( pdef->fldNum == 0 )
	{
		matched = true;
		goto LAST;
	}

	fdef = &pdef->fldDef[0];
	matched = true;
	/* 全部匹配所需的域, 才算匹配当前这个集 */
	for ( i = 0 ; i < pdef->fldNum && matched; i++, fdef++ )
	{
		if(fdef->what == FldDef::PSecond )
		{
			if ( !gCFG->inverse ) 
				test = snd_pac;
			else
				test = r2nd_pac;
		} else {
			if ( !gCFG->inverse ) 
				test = rcv_pac;
			else
				test = r1st_pac;
		}

		if ( !test || test->max < fdef->no ) 
		{
			matched = false;
			break;
		}
		matched = do_match( test->fld[ fdef->no ],  *fdef);
	}
LAST:
	matched = pdef->last_not ? !matched : matched;
	return matched;
}

bool Ramify::do_match(FieldObj &field, FldDef &fld_def)
{
	unsigned int i;
	FldDef::Match *scan;
	bool matched;
	TBuffer m_con;

	matched = false;

	if ( field.no < 0 )	/* 此域不存在, 当然不行 */
	{
		matched = false;
		goto End;
	}

	if ( fld_def.m_num == 0 )
	{
		matched = true;
		goto End;
	}

	/* 匹配所列的其中一项就算可以了 */
	for ( i = 0 , scan = &fld_def.match[0]; i < fld_def.m_num; i++, scan++ )
	{
		switch ( scan->type)
		{
		case VAR_ANY:
			matched = true;
			break;

		case CONSTANT:
			if ( scan->len == field.range 
				&& memcmp(field.val, scan->val, scan->len) == 0 )
				matched = true;
			break;

		case BEGIN_W:
			if ( scan->len <= field.range 
				&& memcmp(field.val, scan->val, scan->len) == 0 )
				matched = true;
			break;

		case END_W:
			if ( scan->len <= field.range 
				&& memcmp(&field.val[field.range - scan->len], scan->val, scan->len) == 0 )
				matched = true;
			break;

		case REGEX:
#if defined(_MSC_VER) && (_MSC_VER >= 1900 )
			matched = regex_match((const char*)field.val, (const char*)(field.val + field.range),  scan->rgx);
#else
			m_con.grant(field.range+1);
			m_con.input(field.val, field.range);
			m_con.base[field.range] = 0;	//null terminating string.
			if ( regexec(&(scan->rgx), (const char*)m_con.base, 0,  0, 0) == 0 )
				matched = true;
#endif
			break;

		default:
			break;
		}

		if ( scan->NOT )
			matched = !matched;

		if ( matched ) 
			break;
	}
End:	
	if ( fld_def.hnot)
		matched = !matched;	/* 这表示此域要不符合列表中的任一项才算满足条件 */

	return matched;
}

#define TEXTUS_APTUS_TAG { 'R', 'a', 'm', 'i', 'f', 'y', 0};
#include "hook.c"
