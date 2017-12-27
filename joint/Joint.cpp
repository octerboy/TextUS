/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Joint attachment
 Desc: Extending of Aptus. 
 Build: created by octerboy, 2006/05/14，Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Aptus.h"
#include "Animus.h"

class Joint: public Aptus {
public:
	void ignite_t (TiXmlElement *wood, TiXmlElement *);
	bool facio(Amor::Pius *);
	Amor *clone();

	Joint();
	
	struct List { Joint *me; List *next; };
	void dup(); 

protected:
	static List g_right;	/* the list of right nodes */

	List ele;

	bool isRight;		/* true: this class is for cause node */
	bool dup_top;

	void append(List &l, List &e) {
		List *n;	/* 表的头一元素的me始终为空 */
		n = &l;
		while (n->next) { 
			if ( n->next == &e ) return;	/* 已有, 不再加载 */
			n = n->next;
		}
		n->next  = &e;
	}
	#include "tbug.h"
};

#include "Notitia.h"
#include "textus_string.h"
#include "casecmp.h"
#include <stdio.h>
#include <string.h>

Joint::List Joint::g_right = {0,0};

void Joint::ignite_t (TiXmlElement *cfg, TiXmlElement *sz_ele)
{
	const char *comm_str;
		
	WBUG("this %p , prius %p, aptus %p, cfg %p, owner %p\n", this, prius, aptus, cfg, owner);
	if ( !sz_ele) return;
	
	canAccessed = true;
	dup_top = false;
	if ( (comm_str =sz_ele->Attribute("action")) && strcasecmp(comm_str, "clone") == 0 )
		dup_top = true;

	isRight = false;
	if ( (comm_str =sz_ele->Attribute("location")) && strcasecmp(comm_str, "right") == 0 )
	{	/* right node */
		isRight = true;
		append(g_right, ele);
	}
	
	if ( !isRight && comm_str && strcasecmp(comm_str, "left") !=0 ) /* 要么没有, 默认为left */
		canAccessed = false;
}

bool Joint::facio( Amor::Pius *pius)
{      
	switch ( pius->ordo)
	{
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		dup();
		break;
	default:
		return false;
	}
	return false;
}

Amor *Joint::clone() 
{
	Joint *child = new Joint();
	child->canAccessed = false;
	return  (Amor*)child;
}

void Joint::dup()
{
	Animus *apt, *comp;
	Aptus **o_c;
	List *n;
	unsigned int i, o_d;
	Amor::Pius ready, ready2; 
	ready.ordo = Notitia::IGNITE_ALL_READY;
	ready2.ordo = Notitia::CLONE_ALL_READY;

	if (isRight)  return;

	apt = (Animus* ) aptus;
	n = &g_right;
	while ( n->next )
	{
		o_c = apt->compactor;
		o_d = apt->duco_num ;

		apt->duco_num++;
		apt->compactor = new Aptus* [o_d+1];
		
		if ( o_d > 0 )
		{
			memcpy( apt->compactor, o_c, sizeof(Aptus*) * o_d);
			delete[] o_c;
		}

		if ( dup_top )
		{
			comp = (Animus *)((Animus *) (n->next->me->aptus))->clone_p ((Aptus*)aptus);
			apt->compactor[o_d] = comp;
			comp->info(ready2);
		} else {
			comp = (Animus *) (n->next->me->aptus);
			apt->compactor[o_d] = comp;
			comp->refer_count++;
			comp->info(ready);
		}
		n = n->next;
	}

	for ( i = 0; i < apt->num_real_ext; i++ )	
	{
		apt->consors[i]->duco_num = apt->duco_num;
		apt->consors[i]->compactor = apt->compactor;
	}
}

Joint::Joint()
{
	dup_top	= false;
	isRight= false;
	ele.next = (List *)0;
	ele.me = this;
}

#define TEXTUS_APTUS_TAG {'J', 'o', 'i', 'n', 't', 0};
#include "hook.c"
