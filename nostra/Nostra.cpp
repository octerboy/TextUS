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
#define NOSTRA_MAX 2048 /* 默认最大子实例数，它意味着同时可维持的最多连接数 */
#define INSTANCE_ADD 3  /* 每次实例的增加数 */

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
		struct DoubleList **filius;	/* 堆栈,保存空闲子实例,按顺序排放 */
		int cursor;	/* 栈顶,初始为-1 */	
		int child_num;	/* 已生子对象数, 初始0 */
		int growth_rate;	/* 增长率 */
		int maxium;	/* 最大子实例数 */
		unsigned radical:1;
		unsigned isHunter:1;
		Nostra *last_neo;	/* 最近一次新建的对象 */
		int seed;	/* 系统启动时预创建的对象数, 默认1 */
		Amor *get() {	/* 从filius中取出一个空闲对象 */
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
		bool put(Nostra *busy) {	/* 回收一个对象 */
			if ( busy->refs > 0 )
			{	/* 有其它线程还在运行, 此对象不能进入filius, 但状态改变 */
				busy->status = RECLAIM;
				return false;
			}
	
			busy->status = IDLE;
			if ( cursor == -1 )
			{
				return false;
			}

			/* 进入空闲栈 */
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
			{/* 实际设置的最大连接数可能很小, 该置如果实时更新只接受更大的值 */
				growth_rate = atoi(add_str);
			} 

			comm_str = nos_ele->Attribute("seed");
			if ( comm_str && atoi(comm_str) >=1 )
				seed = atoi(comm_str);

			rad = nos_ele->Attribute("radic");
			if ( rad && strcmp(rad, "yes") == 0)
				radical = true;

			max_str = nos_ele->Attribute("maxium");
			if ( max_str && atoi(max_str) > 1 ) /*实际设置值可能很小,该置如果实时更新只接受更大的值*/
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
	int refs;	/* 被其它线程引用参考数 */

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

	canAccessed = true;	/* 至此可以认为此应用模块需要Nostra */
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
		{	/* 待回收, 已经不再被引用, 所就进入空闲组 */
			gCFG->put(this);
		}
		break;

	case Notitia::DMD_CLONE_OBJ:
		WBUG("sponte DMD_CLONE_OBJ" );
		gCFG->last_neo = 0;
		if ( gCFG->radical ) /* 这里是根,所以这里就进行clone() */
		{
			Animus *ans;
			ans = (Animus *)aptus->clone();
			if ( ans ) 
				ans->info(ready2);

		} else if( ((Aptus*)aptus)->prius) { 	//本aptus不是根,所以向左边传
			((Aptus*)aptus)->prius->sponte(pius);
		}

		if (gCFG->last_neo ) 
			pius->indic = gCFG->last_neo->owner;
		break;

	case Notitia:: CMD_FREE_IDLE:
		WBUG("sponte CMD_FREE_IDLE %p, owner %p", pius->indic,  owner);
		if ( (Amor*)pius->indic != owner ) 
		{	/* 不是本owner发出的, 给以中断 */
			pius->indic = (void*) 0;
			break;
		}

		if ( !gCFG->put(this))
			pius->indic = 0;
		break;

	case Notitia:: CMD_ALLOC_IDLE:
		WBUG("sponte CMD_ALLOC_IDLE" );
		if ( (Amor*)pius->indic != owner ) 
		{	/* 不是本owner发出的, 给以中断 */
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
		{	/* 已达最大数, 不再新生了 */
			WBUG("to limit %d", gCFG->maxium);
			pius->indic = 0;
			break;
		}
		
		gCFG->isHunter = true;	/* 使用isHunter这一标志, 以使clone()函数区分是来自本类的CMD_ALLOC_IDLE
					请求, 还是来自左节点的clone()。只有本类的CMD_ALLOC_IDLE所要求的, 其clone
					产生的对象才进入filius */
		here_num = gCFG->maxium - gCFG->child_num;
		if ( here_num > gCFG->growth_rate ) here_num = gCFG->growth_rate;
		for ( i =0; i < here_num; i++)
		if ( gCFG->radical )
		{	/* 这是根, 所以这里就进行clone(), prius不变 */
			Animus *ans;
			ans = (Animus *)aptus->clone();
			if ( ans ) 
				ans->info(ready2);
		} else if( ((Aptus*)aptus)->prius) 	/* 本aptus不是根,所以向左边传 */
		{
			((Aptus*)aptus)->prius->sponte(&cln_ps);	/* 不用laeve(), 直接调用父节点 */
		}	

		/* 再找空闲实例 */
		pius->indic = gCFG->get();
		//{printf("her---\n");int *a= 0 ; *a=0;}

		gCFG->isHunter = false;	/* 标志复原 */
		break;
	default: /* 继续向左传递 */
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
	{	/* 这个clone()是由CMD_ALLOC_IDLE引起的, 所以要进入filius */
		gCFG->store(child);
	}

	gCFG->last_neo = child;
	return  (Amor*)child;
}

Nostra::Nostra()
{
	gCFG= 0 ;
        has_config = false;

	refs = 0;	/* 并未有其它线程引用 */
	status = IDLE;	/* 初生, 当然没有被用 */
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
