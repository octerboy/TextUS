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
	Nostra ***filius;	/* 堆栈,保存空闲子实例,按顺序排放 */
	int *cursor;	/* 栈顶,初始为-1 */	
	int *child_num;	/* 已生子对象数, 初始0 */

	int growth_rate;	/* 增长率 */
	int maxium;	/* 最大子实例数 */
	bool radical;
	bool isPoineer;
	bool isHunter;
	Nostra *last_neo;	/* 最近一次新建的对象 */
	int seed;	/* 系统启动时预创建的对象数, 默认1 */
	
	enum {IDLE, BUSY, RECLAIM } status;
	int refs;	/* 被其它线程引用参考数 */
	inline Amor *get();	/* 从filius中取出一个空闲对象 */
	inline bool put(Nostra *);	/* 回收一个对象 */

#include "tbug.h"
};
#include "Notitia.h"
#include <string.h>
#define NOSTRA_MAX 2048 /* 默认最大子实例数，它意味着同时可维持的最多连接数 */
#define INSTANCE_ADD 3  /* 每次实例的增加数 */

void Nostra::ignite_t (TiXmlElement *cfg, TiXmlElement *nos_ele)
{
	const char* rad, *add_str, *max_str, *comm_str;
	int max_old;

	WBUG("this %p, aptus %p, cfg %p", this, aptus,  cfg);

	if ( !nos_ele) return;

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

	max_old = maxium;
	max_str = nos_ele->Attribute("maxium");
	if ( max_str && atoi(max_str) > max_old ) /*实际设置值可能很小,该置如果实时更新只接受更大的值*/
		maxium = atoi(max_str);
	else
		maxium = NOSTRA_MAX;

	canAccessed = true;	/* 至此可以认为此应用模块需要Nostra */
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
	{	/* 这么做是为了能够支持ignite的再次被调用, 从而实现参数的实时更新并生效 */
		Nostra** neo=  new Nostra* [maxium];
		memset(neo, 0, maxium * sizeof(Nostra*) );
		memcpy(neo, *filius, max_old);
		delete[] (*filius);
		*filius = neo;
	} else
	{	/* 初次调用 */ 
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
		{	/* 待回收, 已经不再被引用, 所就进入空闲组 */
			put(this);
		}
		break;

	case Notitia::DMD_CLONE_OBJ:
		WBUG("sponte DMD_CLONE_OBJ" );
		last_neo = 0;
		if ( radical ) /* 这里是根,所以这里就进行clone() */
		{
			Animus *ans;
			ans = (Animus *)aptus->clone();
			if ( ans ) 
				ans->info(ready2);

		} else if( ((Aptus*)aptus)->prius) { 	//本aptus不是根,所以向左边传
			((Aptus*)aptus)->prius->sponte(pius);
		}

		if (last_neo ) 
			pius->indic = last_neo->owner;
		break;

	case Notitia:: CMD_FREE_IDLE:
		WBUG("sponte CMD_FREE_IDLE %p, owner %p", pius->indic,  owner);
		if ( (Amor*)pius->indic != owner ) 
		{	/* 不是本owner发出的, 给以中断 */
			pius->indic = (void*) 0;
			break;
		}

		if ( !put(this))
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

		if ( (pius->indic = get()) )
		{
			WBUG("new owner is %p", pius->indic);
			break;
		}

		if ( *child_num == maxium   )
		{	/* 已达最大数, 不再新生了 */
			WBUG("to limit %d", maxium);
			pius->indic = 0;
			break;
		}
		
		isHunter = true;	/* 使用isHunter这一标志, 以使clone()函数区分是来自本类的CMD_ALLOC_IDLE
					请求, 还是来自左节点的clone()。只有本类的CMD_ALLOC_IDLE所要求的, 其clone
					产生的对象才进入filius */
		here_num = maxium - *child_num;
		if ( here_num > growth_rate ) here_num = growth_rate;
		for ( i =0; i < here_num; i++)
		if ( radical )
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
		pius->indic = get();

		isHunter = false;	/* 标志复原 */
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
	(*child_num)++;

	Aptus::inherit( (Aptus*) child);

	child->growth_rate = growth_rate;
	child->maxium = maxium;

	child->filius = filius;
	child->cursor = cursor;
	child->child_num = child_num;
	child->radical = radical;

	if (isHunter)
	{	/* 这个clone()是由CMD_ALLOC_IDLE引起的, 所以要进入filius */
		put(child);
	}

	child->seed = seed; 
	last_neo = child;
	return  (Amor*)child;
}

bool Nostra::put (Nostra *busy)
{
	if ( busy->refs > 0 )
	{	/* 有其它线程还在运行, 此对象不能进入filius, 但状态改变 */
		busy->status = RECLAIM;
		return false;
	}
	
	busy->status = IDLE;
	if ( (*cursor) + 1 == maxium   )
	{	/* 已达最大数, '还'不进去了 */
		return false;
	}

	/* 进入空闲栈 */
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

	refs = 0;	/* 并未有其它线程引用 */
	status = IDLE;	/* 初生, 当然没有被用 */
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
