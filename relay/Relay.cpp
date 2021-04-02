/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Aptus extension, to stop or continue calling the next noed.
 Build:created by octerboy, 2006/04/01, Wuhan
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Aptus.h"
#include "BTool.h"
#include "Notitia.h"
#include "Animus.h"

class Relay: public Aptus {
public:
	void ignite_t (TiXmlElement *wood, TiXmlElement *);
	Amor *clone();

	bool laeve( Amor::Pius *pius, unsigned int from);	//������ã�����aptus�ĸ��ڵ�
	bool facio(Amor::Pius *);		/* ����ת�Ƶ��õĺ��� */

	Relay();
protected:
	bool want_stop;
	bool stop_pass;
	Amor::Pius stop_pius;	

	bool goon_pass;
	enum {GET, VIRTUAL, NONE} how_pius;
	enum {NEXT, SELF} goon_target;

	Amor::Pius vps;
	
	inline void leftgo();
	inline Amor::Pius* getP();
#include "tbug.h"
};

#include "textus_string.h"
#include "casecmp.h"
#include <stdio.h>

Relay::Relay()
{
	want_stop = false;
	stop_pius.ordo = Notitia::DMD_STOP_NEXT ; 
	stop_pius.indic = 0;

	stop_pass = false;
	goon_pass = false;
	how_pius = NONE;	
	goon_target = NEXT;	
	
	vps.ordo = Notitia::TEXTUS_RESERVED;
	vps.indic = 0;
}

void Relay::ignite_t (TiXmlElement *cfg, TiXmlElement *rel_ele)
{
	const char *comm_str, *pass_str;
	TiXmlElement *ct_ele;

	WBUG("this %p, prius %p, aptus %p, cfg %p",  this, prius, aptus, cfg);
	if ( !rel_ele) return;

	if ( (pass_str = rel_ele->Attribute("stop")) 
		&& strcasecmp(pass_str, "pass") == 0 )
		stop_pass = true;

	ct_ele = rel_ele->FirstChildElement("continue");
	if (!ct_ele) goto END_CFG;

	if ( (pass_str = ct_ele->Attribute("pass")) 
		&& strcasecmp(pass_str, "yes") == 0 )
		goon_pass = true;

	how_pius = NONE;
	if ( (comm_str = ct_ele->Attribute("pius")) ) 
	{
		if ( strcasecmp(comm_str, "get") == 0 )
			how_pius = GET;
		else if ( strcasecmp(comm_str, "virtual") == 0 )
			how_pius = VIRTUAL;
	}

	//comm_str = ct_ele->Attribute("vordo");
	//BTool::get_textus_ordo(&vps.ordo, comm_str);
	vps.ordo = Notitia::get_ordo(ct_ele->Attribute("vordo"));


END_CFG:
	canAccessed = true;	/* ���˿�����Ϊ��Ӧ��ģ����ҪRelay */
	need_lae = true;
}

Amor *Relay::clone()
{	
	Relay *child = 0;
	child = new Relay();
	Aptus::inherit( (Aptus*) child );
#define Inherit(x) child->x = x;

	Inherit(stop_pass);

	Inherit(goon_pass);
	Inherit(how_pius);

	Inherit(vps);
	return  (Amor*)child;
}

bool Relay::laeve( Amor::Pius *pius, unsigned int from)
{	/* ����owner->sponte()֮ǰ�Ĵ��� */
	void **p;
	Amor *boy;
	Aptus **tor;
	Amor::Pius *n_pius, *m_pius;
	bool has_found ;
	bool inv = false;

	switch (pius->ordo)
	{
	case Notitia::DMD_STOP_NEXT :	/* ĳ�ӽڵ�Ҫ��ֹͣ����һ���ڽڵ�ĵ��� */
		WBUG("laeve Notitia::DMD_STOP_NEXT owner %p", owner);
		want_stop = true;	/* ��һ�±�־ */
		aptus->aptus = this;		/* ʹAnimus�ڵ�����һ��Compactor��������ת�� */
		if ( stop_pass && prius)	/* ����ڵ�������ֹ�ź� */
			prius->laeve(pius, 0) ;
		break;
		
	case Notitia::DMD_CONTINUE_SELF :	//�����ý�����
		WBUG("laeve Notitia::DMD_CONTINUE_SELF owner %p, indic %p", owner, pius->indic);
	case Notitia::DMD_CONTINUE_NEXT :	//�Ӹý����ߵ���һ��������������
		WBUG("laeve Notitia::DMD_CONTINUE_NEXT owner %p, indic %p", owner, pius->indic);
		p = (void**)(pius->indic);
		if (!p) goto PASS;

		boy = (Amor*) (*p);
		p++;
		n_pius = (Amor::Pius*)(*p);
		m_pius = getP();	/* ���ڵ�������Pius, ������ */
		if ( m_pius)		/* �⽫ʹ���ӽڵ����ʹ��STOPһ��, �򵥵ص���һ��CONTINUE������ */
			n_pius = m_pius;	/* ��Ȼ, �ӽڵ㻹��׼�����鼰����ps[0]=owner */
		p++;
		if ( *p == (void *) 0x1)
			inv = true;
		else if ( *p == 0x0 )
			inv = false;
		
		has_found = false;
		tor = compactor; want_stop = false;
		for ( unsigned int i= 0; i < duco_num; i++, tor++)
		{
			if ( !has_found ) 
			{
				if ( (*tor)->owner == boy )
					has_found = true;	/* �ҵ��˸ý����ߣ���continue�����ֵ���һ�� */
				if ( pius->ordo != Notitia::DMD_CONTINUE_SELF )	/* ����SELF���Ǹ�boy��, ����continue */
					continue;
			}

			if ( inv )
				(*tor)->laeve(n_pius, 0);
			else 
				((Animus *)aptus)->to_dextra(n_pius, i); /* ����Animus�е���ز��� */
			break;
		}
PASS:
		if ( pius->ordo == Notitia::DMD_CONTINUE_SELF )	/* ����SELF�Ͳ������� */
			break;
		if (goon_pass && prius)	/* ����ڵ�����CONTINUE�ź� */
			leftgo();
		break;
		
	default:
		return false;
	}
	return true;	
}

bool Relay::facio( Amor::Pius *pius)
{
	/* owner����facio()ʱ, �뿪���ڵ�֮ǰ */
	WBUG("facio Notitia::" TLONG_FMT ", owner %p", pius->ordo, owner);

	if (want_stop ) 
	{	/* ����Animus����Aptus��facio��������ת�ƶ�����, 
		���Ǹ��ڵ���ӽڵ�ĵ��ÿ��� */
		want_stop = false;	/* ��־��λ */
		return true;
	} 

	return false;
}

/* Ҫ����ڵ����������һ���ڽڵ� */
inline void Relay::leftgo()
{
	void *ps[3];
	Amor::Pius c_p;
	c_p.ordo =Notitia::DMD_CONTINUE_NEXT;
	c_p.indic = ps;

	if ( !prius)
		return;
	ps[0] = this->owner;
	ps[1] = getP();
	ps[2] = 0;
	prius->laeve(&c_p, 0) ;
}

inline Amor::Pius* Relay::getP()
{
	Amor::Pius t_p;
	Amor::Pius *sub_ps = 0;
	t_p.ordo = Notitia::CMD_GET_PIUS;

	switch ( how_pius)
	{
	case NONE:
		sub_ps = 0;
		break;

	case GET:
		aptus->sponte(&t_p);
		sub_ps = (Amor::Pius *)t_p.indic;
		break;

	case VIRTUAL:
		sub_ps = &vps;
		break;

	default:
		break;
	}
	return sub_ps;
}
#define TEXTUS_APTUS_TAG { 'R','e','l','a','y',0};
#include "hook.c"
