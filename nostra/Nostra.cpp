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
#include "Notitia.h"
#include <string.h>
#define NOSTRA_MAX 2048 /* Ĭ�������ʵ����������ζ��ͬʱ��ά�ֵ���������� */
#define INSTANCE_ADD 3  /* ÿ��ʵ���������� */

class Nostra : public Aptus {
public:
	void ignite_t (TiXmlElement *wood, TiXmlElement *);
	Amor *clone();

	bool facio (Amor::Pius *);
	bool sponte_n (Amor::Pius *, unsigned int);
	Nostra();
	~Nostra();

protected:
	struct DoubleList {
		struct DoubleList *next;
		struct DoubleList *prev;
		Nostra *me;
		DoubleList () {
			next = prev = this;
			me = 0;
		};
		DoubleList *remove()
		{
			DoubleList *list = next;
	
			next = next->next;
			next->prev = this;

			return ((list != this) ? list : (DoubleList *)0);
		};
		void	append(DoubleList *list)
		{
			list->next = this;
			list->prev = prev;

			prev->next = list;
			prev = list;
		};

	};
	struct  G_CFG {
		struct DoubleList que; 
		struct DoubleList *we; 
		unsigned int we_num;
		struct DoubleList **filius;	/* ��ջ,���������ʵ��,��˳���ŷ� */
		int cursor;	/* ջ��,��ʼΪ-1 */	
		int child_num;	/* �����Ӷ�����, ��ʼ0 */
		int growth_rate;	/* ������ */
		int maxium;	/* �����ʵ���� */
		unsigned radical:1;
		unsigned isHunter:1;
		Nostra *last_neo;	/* ���һ���½��Ķ��� */
		int seed;	/* ϵͳ����ʱԤ�����Ķ�����, Ĭ��1 */
		Amor *get() {	/* ��filius��ȡ��һ�����ж��� */
			Nostra *idle;
			Amor *neo = (Amor*) 0;
			struct DoubleList *an_node = que.remove(); 
			if ( an_node ) {
				idle = (Nostra*) an_node->me;
				idle->status = BUSY;
				neo= idle->owner;
			} else {
				return 0;
			}

			cursor++;	/* now may be 0, for -1 initially */
			filius[cursor] = an_node;	/* save the case */
			//an_node->me = 0 ;
			return neo;
		};
		void store(Nostra *born) { /* new */
			we[child_num].me = born; /* child_num is 0 intially */
			que.append(&we[child_num]);
			child_num++;
		};
		bool put(Nostra *busy) {	/* ����һ������ */
			if ( busy->refs > 0 )
			{	/* �������̻߳�������, �˶����ܽ���filius, ��״̬�ı� */
				busy->status = RECLAIM;
				return false;
			}
	
			busy->status = IDLE;
			if ( cursor == -1 )
			{
				return false;
			}

			/* �������ջ */
			filius[cursor]->me = busy;
			que.append(filius[cursor]);
			filius[cursor] = 0;
			cursor--;
			return true;
		};

		G_CFG ( TiXmlElement *nos_ele ) {
			const char* rad, *add_str, *max_str, *comm_str;
			cursor = -1;
			child_num = 0;

			radical = false;
			maxium = NOSTRA_MAX;
			isHunter = false;
			seed = 1;
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

			max_str = nos_ele->Attribute("maxium");
			if ( max_str && atoi(max_str) > 1 ) /*ʵ������ֵ���ܺ�С,�������ʵʱ����ֻ���ܸ����ֵ*/
				maxium = atoi(max_str);

			we = new DoubleList[maxium];
			filius = new  DoubleList* [maxium];
			memset(filius, 0, sizeof(DoubleList*));
			/*
			for ( i = 0 ; i < maxium; i++ )
			{
				filius[i]= &we[i];
			}
			*/
		};
		~G_CFG() {
			delete[] *filius;
			delete[] we;
		};
	};
        struct G_CFG *gCFG;
        bool has_config;

	enum {IDLE, BUSY, RECLAIM } status;
	int refs;	/* �������߳����òο��� */

#include "tbug.h"
};
void Nostra::ignite_t (TiXmlElement *cfg, TiXmlElement *nos_ele)
{
	WBUG("this %p, aptus %p, cfg %p", this, aptus,  cfg);

	if ( !nos_ele) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(nos_ele);
		has_config = true;
	}

	canAccessed = true;	/* ���˿�����Ϊ��Ӧ��ģ����ҪNostra */
	need_spo = true;
}

bool Nostra::facio( Amor::Pius *pius)
{
	Amor::Pius ready2;
	ready2.ordo = Notitia::CLONE_ALL_READY;
	switch(pius->ordo)
	{
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY %d", gCFG->seed);
		if ( gCFG->radical )
		for ( int i = 1 ; i < gCFG->seed && i < gCFG->maxium ; i++)
		{
			Animus *ans;
			gCFG->isHunter = true;
			//if ( i ==4 ) {printf("her---\n");int *a= 0 ; *a=0;}
			ans = (Animus*) aptus->clone();
			gCFG->isHunter = false;
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
			gCFG->put(this);
		}
		break;

	case Notitia::DMD_CLONE_OBJ:
		WBUG("sponte DMD_CLONE_OBJ" );
		gCFG->last_neo = 0;
		if ( gCFG->radical ) /* �����Ǹ�,��������ͽ���clone() */
		{
			Animus *ans;
			ans = (Animus *)aptus->clone();
			if ( ans ) 
				ans->info(ready2);

		} else if( ((Aptus*)aptus)->prius) { 	//��aptus���Ǹ�,��������ߴ�
			((Aptus*)aptus)->prius->sponte(pius);
		}

		if (gCFG->last_neo ) 
			pius->indic = gCFG->last_neo->owner;
		break;

	case Notitia:: CMD_FREE_IDLE:
		WBUG("sponte CMD_FREE_IDLE %p, owner %p", pius->indic,  owner);
		if ( (Amor*)pius->indic != owner ) 
		{	/* ���Ǳ�owner������, �����ж� */
			pius->indic = (void*) 0;
			break;
		}

		if ( !gCFG->put(this))
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

		if ( (pius->indic = gCFG->get()) )
		{
			WBUG("new owner is %p", pius->indic);
			break;
		}

		if ( gCFG->child_num == gCFG->maxium   )
		{	/* �Ѵ������, ���������� */
			WBUG("to limit %d", gCFG->maxium);
			pius->indic = 0;
			break;
		}
		
		gCFG->isHunter = true;	/* ʹ��isHunter��һ��־, ��ʹclone()�������������Ա����CMD_ALLOC_IDLE
					����, ����������ڵ��clone()��ֻ�б����CMD_ALLOC_IDLE��Ҫ���, ��clone
					�����Ķ���Ž���filius */
		here_num = gCFG->maxium - gCFG->child_num;
		if ( here_num > gCFG->growth_rate ) here_num = gCFG->growth_rate;
		for ( i =0; i < here_num; i++)
		if ( gCFG->radical )
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
		pius->indic = gCFG->get();
		//{printf("her---\n");int *a= 0 ; *a=0;}

		gCFG->isHunter = false;	/* ��־��ԭ */
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

	Aptus::inherit( (Aptus*) child);

	child->gCFG = gCFG;
	if (gCFG->isHunter)
	{	/* ���clone()����CMD_ALLOC_IDLE�����, ����Ҫ����filius */
		gCFG->store(child);
	}

	gCFG->last_neo = child;
	return  (Amor*)child;
}

Nostra::Nostra()
{
	gCFG= 0 ;
        has_config = false;

	refs = 0;	/* ��δ�������߳����� */
	status = IDLE;	/* ����, ��Ȼû�б��� */
}

Nostra::~Nostra()
{
	if ( has_config)
	{
		delete gCFG;
	}
}
#define TEXTUS_APTUS_TAG { 'M', 'u', 'l', 't','i',0 }
#include "hook.c"
