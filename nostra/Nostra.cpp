/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Instance Manager
 Build: created by octerboy, 2005/06/10
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Aptus.h"
#include "Animus.h"
#include "textus_string.h"
class Nostra : public Aptus {
public:
	void ignite_t (TiXmlElement *wood, TiXmlElement *);
	Amor *clone();

	bool facio (Amor::Pius *);
	bool sponte_n (Amor::Pius *, unsigned int);
	Nostra();
	~Nostra();

protected:
	Nostra ***filius;	/* ��ջ,���������ʵ��,��˳���ŷ� */
	int *cursor;	/* ջ��,��ʼΪ-1 */	
	int *child_num;	/* �����Ӷ�����, ��ʼ0 */

	int growth_rate;	/* ������ */
	int maxium;	/* �����ʵ���� */
	bool radical;
	bool isPoineer;
	bool isHunter;
	Nostra *last_neo;	/* ���һ���½��Ķ��� */
	int seed;	/* ϵͳ����ʱԤ�����Ķ�����, Ĭ��1 */
	
	enum {IDLE, BUSY, RECLAIM } status;
	int refs;	/* �������߳����òο��� */
	inline Amor *get();	/* ��filius��ȡ��һ�����ж��� */
	inline bool put(Nostra *);	/* ����һ������ */

#include "tbug.h"
};
#include "Notitia.h"
#include <string.h>
#define NOSTRA_MAX 2048 /* Ĭ�������ʵ����������ζ��ͬʱ��ά�ֵ���������� */
#define INSTANCE_ADD 3  /* ÿ��ʵ���������� */

void Nostra::ignite_t (TiXmlElement *cfg, TiXmlElement *nos_ele)
{
	const char* rad, *add_str, *max_str, *comm_str;
	int max_old;

	WBUG("this %p, aptus %p, cfg %p", this, aptus,  cfg);

	if ( !nos_ele) return;

	growth_rate = INSTANCE_ADD;
	add_str = nos_ele->Attribute("increase");
	if ( add_str && atoi(add_str) >=0 )
	{/* ʵ�����õ�������������ܺ�С, �������ʵʱ����ֻ���ܸ����ֵ */
		growth_rate = atoi(add_str);
	} 

	comm_str = nos_ele->Attribute("seed");
	if ( comm_str && atoi(comm_str) >=1 )
		seed = atoi(comm_str);

	rad = nos_ele->Attribute("radic");
	if ( rad && strcmp(rad, "yes") == 0)
		radical = true;

	max_old = maxium;
	max_str = nos_ele->Attribute("maxium");
	if ( max_str && atoi(max_str) > max_old ) /*ʵ������ֵ���ܺ�С,�������ʵʱ����ֻ���ܸ����ֵ*/
		maxium = atoi(max_str);
	else
		maxium = NOSTRA_MAX;

	canAccessed = true;	/* ���˿�����Ϊ��Ӧ��ģ����ҪNostra */
	need_spo = true;

	if ( !filius)
	{
		filius = new Nostra**;
		*filius = (Nostra**) 0;
	}

	if ( !cursor )
	{
		cursor = new int;
		*cursor = -1;
	}

	if ( !child_num )
	{
		child_num = new int;
		*child_num = 0;
	}

	if ( *filius )
	{	/* ��ô����Ϊ���ܹ�֧��ignite���ٴα�����, �Ӷ�ʵ�ֲ�����ʵʱ���²���Ч */
		Nostra** neo=  new Nostra* [maxium];
		memset(neo, 0, maxium * sizeof(Nostra*) );
		memcpy(neo, *filius, max_old);
		delete[] (*filius);
		*filius = neo;
	} else
	{	/* ���ε��� */ 
		*filius = new Nostra *[maxium];
		memset(*filius, 0, maxium * sizeof(Nostra*));
	}
	isPoineer = true;
}

bool Nostra::facio( Amor::Pius *pius)
{
	Amor::Pius ready2;
	ready2.ordo = Notitia::CLONE_ALL_READY;
	switch(pius->ordo)
	{
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY %d", seed);
		if ( radical )
		for ( int i = 1 ; i < seed && i < maxium ; i++)
		{
			Animus *ans;
			isHunter = true;
			ans = (Animus*) aptus->clone();
			isHunter = false;
			if ( ans ) 
				ans->info(ready2);
		}
		break;

	default :
		return false;
	}
	return true;
}

bool Nostra::sponte_n (Amor::Pius *pius, unsigned int from)
{
	int here_num;
	Amor::Pius ready2,cln_ps;
	ready2.ordo = Notitia::CLONE_ALL_READY;
	cln_ps.ordo = Notitia::DMD_CLONE_OBJ;
	int i;

	switch(pius->ordo)
	{
	case Notitia::CMD_INCR_REFS:
		WBUG("sponte CMD_INCR_REFS" );
		refs++;
		break;

	case Notitia::CMD_DECR_REFS:
		WBUG("sponte CMD_DECR_REFS" );
		if ( refs > 0 )
			refs--;

		if ( refs == 0 && status == RECLAIM)
		{	/* ������, �Ѿ����ٱ�����, ���ͽ�������� */
			put(this);
		}
		break;

	case Notitia::DMD_CLONE_OBJ:
		WBUG("sponte DMD_CLONE_OBJ" );
		last_neo = 0;
		if ( radical ) /* �����Ǹ�,��������ͽ���clone() */
		{
			Animus *ans;
			ans = (Animus *)aptus->clone();
			if ( ans ) 
				ans->info(ready2);

		} else if( ((Aptus*)aptus)->prius) { 	//��aptus���Ǹ�,��������ߴ�
			((Aptus*)aptus)->prius->sponte(pius);
		}

		if (last_neo ) 
			pius->indic = last_neo->owner;
		break;

	case Notitia:: CMD_FREE_IDLE:
		WBUG("sponte CMD_FREE_IDLE %p, owner %p", pius->indic,  owner);
		if ( (Amor*)pius->indic != owner ) 
		{	/* ���Ǳ�owner������, �����ж� */
			pius->indic = (void*) 0;
			break;
		}

		if ( !put(this))
			pius->indic = 0;
		break;

	case Notitia:: CMD_ALLOC_IDLE:
		WBUG("sponte CMD_ALLOC_IDLE" );
		if ( (Amor*)pius->indic != owner ) 
		{	/* ���Ǳ�owner������, �����ж� */
			WBUG("not my owner, mine is %p, he is %p", owner, pius->indic);
			pius->indic = (void*) 0;
			break;
		}

		if ( (pius->indic = get()) )
		{
			WBUG("new owner is %p", pius->indic);
			break;
		}

		if ( *child_num == maxium   )
		{	/* �Ѵ������, ���������� */
			WBUG("to limit %d", maxium);
			pius->indic = 0;
			break;
		}
		
		isHunter = true;	/* ʹ��isHunter��һ��־, ��ʹclone()�������������Ա����CMD_ALLOC_IDLE
					����, ����������ڵ��clone()��ֻ�б����CMD_ALLOC_IDLE��Ҫ���, ��clone
					�����Ķ���Ž���filius */
		here_num = maxium - *child_num;
		if ( here_num > growth_rate ) here_num = growth_rate;
		for ( i =0; i < here_num; i++)
		if ( radical )
		{	/* ���Ǹ�, ��������ͽ���clone(), prius���� */
			Animus *ans;
			ans = (Animus *)aptus->clone();
			if ( ans ) 
				ans->info(ready2);
		} else if( ((Aptus*)aptus)->prius) 	/* ��aptus���Ǹ�,��������ߴ� */
		{
			((Aptus*)aptus)->prius->sponte(&cln_ps);	/* ����laeve(), ֱ�ӵ��ø��ڵ� */
		}	

		/* ���ҿ���ʵ�� */
		pius->indic = get();

		isHunter = false;	/* ��־��ԭ */
		break;
	default: /* �������󴫵� */
		return false;
	}

	return true;
}

Amor *Nostra::clone()
{	
	Nostra *child = 0;
	child = new Nostra();
	(*child_num)++;

	Aptus::inherit( (Aptus*) child);

	child->growth_rate = growth_rate;
	child->maxium = maxium;

	child->filius = filius;
	child->cursor = cursor;
	child->child_num = child_num;
	child->radical = radical;

	if (isHunter)
	{	/* ���clone()����CMD_ALLOC_IDLE�����, ����Ҫ����filius */
		put(child);
	}

	child->seed = seed; 
	last_neo = child;
	return  (Amor*)child;
}

bool Nostra::put (Nostra *busy)
{
	if ( busy->refs > 0 )
	{	/* �������̻߳�������, �˶����ܽ���filius, ��״̬�ı� */
		busy->status = RECLAIM;
		return false;
	}
	
	busy->status = IDLE;
	if ( (*cursor) + 1 == maxium   )
	{	/* �Ѵ������, '��'����ȥ�� */
		return false;
	}

	/* �������ջ */
	(*cursor)++;
	(*filius)[*cursor] = busy;
	return true;
}

Amor* Nostra::get ()
{
	Amor *neo = (Amor*) 0;
	
	if ( *cursor >= 0 ) 
	{
		Nostra *idle;
		idle = (*filius)[*cursor];
		idle->status = BUSY;
		neo= idle->owner;
		(*cursor)--;
	}
	return neo;
}

Nostra::Nostra()
{
	filius = (Nostra***)0;
	cursor = (int*) 0;
	child_num = 0;

	radical = false;
	maxium = 0;
	isPoineer = false;
	isHunter = false;
	seed = 1;

	refs = 0;	/* ��δ�������߳����� */
	status = IDLE;	/* ����, ��Ȼû�б��� */
}

Nostra::~Nostra()
{
	if ( isPoineer)
	{
		if( cursor) delete cursor;
		if ( filius )
		{
			if ( *filius )
				delete[] *filius;
			delete filius;
		}
	}
}
#define TEXTUS_APTUS_TAG { 'M', 'u', 'l', 't','i',0 }
#include "hook.c"
