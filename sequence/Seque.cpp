/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 Title: Sequence
 Build: created by octerboy, 2006/08/18, Guangzhou
 $Header: /textus/sequence/Seque.cpp 5     08-01-10 1:15 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: Seque.cpp $"
#define TEXTUS_MODTIME  "$Date: 08-01-10 1:15 $"
#define TEXTUS_BUILDNO  "$Revision: 5 $"
/* $NoKeywords: $ */

#include "Notitia.h"
#include "TBuffer.h"
#include "Amor.h"
#include <stdarg.h>
#define SEQUEINLINE inline
class Seque: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Amor::Pius*);
	bool sponte( Amor::Pius*);
	Amor *clone();
	
	Seque();
	~Seque();

private:
	Amor::Pius local_pius, start_trans, end_trans, cancel_trans, set_pius;
	void *start_para[3], *set_para[3];
	TBuffer *pri_req, *pri_ans;
	TBuffer post_req, post_ans;
	Seque *gather;
	Amor *pipe;
	Amor *root;
	
	struct List {
		Amor *me; 
		List *prev;
		List *next;
		inline List ()
		{
			me = 0;
			prev = 0;
			next = 0;
		};
		inline void put ( struct List *neo ) 
		{
			if( !neo ) return;
			neo->next = next;
			neo->prev = this;
			if ( next != 0 )
				next->prev = neo;
			next = neo;
		};

		inline void remove( struct List *obj)
		{
			obj->prev->next = obj->next; 
			if ( obj->next )
				obj->next->prev  =  obj->prev;
			obj->prev = 0;
			obj->next = 0;
		}

	};
#define M_PIPE 0
#define M_SELF 1

	List l_ele;

	SEQUEINLINE void retain();
	SEQUEINLINE void reset();
	SEQUEINLINE void appoint();
	SEQUEINLINE void appoint_me();	//自已传
	struct G_CFG
	{
		bool for_pipe;
		bool to_apportion;
		int work_mode ;
		inline ~G_CFG() { 
		};

		inline G_CFG(TiXmlElement *cfg) 
		{
			const char *comm_str, *m_str;
			for_pipe = false;
			to_apportion = true;

			comm_str = cfg->Attribute("pipe");
			if ( comm_str )
			{
				if ( strcasecmp(comm_str, "yes") == 0 )
					for_pipe = true;
				if ( strcasecmp(comm_str, "no") == 0 )
					for_pipe = false;
			}

			comm_str = cfg->Attribute("apportion");
			if ( comm_str )
			{
				if ( strcasecmp(comm_str, "yes") == 0 )
					to_apportion = true;
				if ( strcasecmp(comm_str, "no") == 0 )
					to_apportion = false;
			}

			work_mode = M_SELF;
			m_str =cfg->Attribute("mode");
			if ( m_str && strcasecmp(m_str, "aggregate") == 0 )
				work_mode = M_PIPE;
		};
	};

	struct G_CFG *gcfg;  
	bool has_config;
#include "wlog.h"
};

#include <assert.h>

void Seque::ignite(TiXmlElement *cfg)
{
	if (!cfg) return;

	if ( !gcfg ) 
	{
		gcfg = new struct G_CFG(cfg);
		has_config = true;
	}
	root = this;
}

bool Seque::facio( Amor::Pius *pius)
{
	TBuffer **tb = 0;

	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF:	/* New request come */
		WBUG("facio PRO_TBUF");
		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//如果tb不为NULL，则*tb应当是rcv_buf，否则在以前的SET_TBUF中就应已设置
			if ( *tb) pri_req = *tb; //新到请求的TBuffer
			tb++;
			if ( *tb) pri_ans  = *tb;
		}

		if ( !pri_req || !pri_ans )
		{
			WLOG(WARNING, "PRO_TBUF null");
			break;
		}
		
		if ( gcfg->to_apportion)
		{
			/* An idle instance to proccess it */
			appoint();
		} else {
			appoint_me();
		}

		break;

	case Notitia::SET_TBUF:	/* get TBuffer address */
		WBUG("facio SET_TBUF");
		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	
			if ( *tb) pri_req = *tb;
			tb++;
			if ( *tb) pri_ans  = *tb;
		} else 
			WLOG(WARNING,"SET_TBUF null");

		if ( !gcfg->to_apportion)
		{
			return false;	//让它续传
		}
		break;

	case Notitia::START_SESSION:
		WBUG("facio START_SESSION");
		reset();
		break;

	case Notitia::DMD_END_SESSION:	
		WBUG("facio DMD_END_SESSION");
		reset();
		while ( l_ele.next)
		{ 
			Seque *bee = (Seque *) (l_ele.next->me);
			assert( bee->gather == this);
			if ( gcfg->work_mode == M_PIPE )	//对于PIPE模式, 只能通知, 这里无法归队, 等RETAIN再归队.
			{
				bee->pipe->facio(&cancel_trans);
			} else {
				bee->retain();
			}
		}

		break;

	case Notitia::IGNITE_ALL_READY:
	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE(IGNITE)_ALL_READY");
		if ( gcfg->work_mode != M_PIPE )	//pipe模式下, TBUFFER都是临时传递的
		{
			aptus->facio(&set_pius);//本对象的TBUFFER
		}
		break;

	default:
		return false;
	}
	return true;
}

bool Seque::sponte( Amor::Pius *pius)
{
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF :	/* as respone */
		WBUG("sponte PRO_TBUF" );
		if ( gather )
		{
			TBuffer::exchange(*(gather->pri_req), post_req);
			TBuffer::exchange(*(gather->pri_ans), post_ans);
			gather->aptus->sponte(&gather->local_pius);
			if ( gcfg->work_mode == M_PIPE )
			{	
				Amor::Pius tmp_pius;
				tmp_pius.ordo =  Notitia::CMD_END_TRANS;
				tmp_pius.indic = this;
				pipe->facio(&tmp_pius);
			}
			retain();
		} else {
			reset();
			WLOG(ERR, "sponte no gather PRO_TBUF!!" );	//这种情况是, 在非PIPE模式下, bee已经归队
		}
		break;

	case Notitia::CMD_RETAIN_TRANS:	
		WBUG("facio CMD_RETAIN_TRANS pipe(%p) from %p", pipe, (Amor*)pius->indic);
		retain();
		break;

	case Notitia::START_SESSION:
		WBUG("sponte START_SESSION");
		//reset();
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("sponte DMD_END_SESSION" );
		if ( gcfg->work_mode != M_PIPE )
		{
			reset();
			if ( gather ) //it is be appointed.
				retain();
		}	
		break;

	case Notitia::TRANS_TO_HANDLE:
		WBUG("sponte TRANS_TO_HANDLE(%p)", pius->indic);
		pipe = (Amor *) pius->indic;
		pipe->aptus->facio(&set_pius);//本对象的TBUFFER, 传递给pipe的下一级
		pipe->aptus->facio(&local_pius);
		break;

	case Notitia::TRANS_TO_SEND:
		WBUG("sponte TRANS_TO_SEND(%p)", pius->indic);
		pipe = (Amor *) pius->indic;
		pipe->aptus->facio(&set_pius);//本对象的TBUFFER, 传递给pipe的下一级
		pipe->aptus->facio(&local_pius);
		break;

	case Notitia::TRANS_TO_RECV:
		WBUG("sponte TRANS_TO_RECV(%p)", pius->indic);
		pipe = (Amor *) pius->indic;
		pipe->aptus->facio(&set_pius);//本对象的TBUFFER, 传递给pipe的下一级, I即是pipe。
		break;
	default:
		return false;
	}
	return true;
}

Seque::Seque()
{
	local_pius.ordo = Notitia::PRO_TBUF;
	local_pius.indic = 0;

	start_trans.ordo = Notitia::CMD_BEGIN_TRANS;
	start_trans.indic = &start_para[0];
	start_para[0] = this;
	start_para[1] = &local_pius.ordo;
	start_para[2] = 0;

	end_trans.ordo = Notitia::CMD_END_TRANS;
	end_trans.indic = this; 

	cancel_trans.ordo = Notitia::CMD_CANCEL_TRANS;
	cancel_trans.indic = this; 

	set_pius.ordo = Notitia::SET_TBUF;
	set_pius.indic = &set_para[0];
	set_para[0] = &post_req;
	set_para[1] = &post_ans;
	set_para[2] = 0;

	pri_req = 0;
	pri_ans = 0;
	gather = 0;
	pipe = 0;
	root = 0;
	l_ele.me = (Amor *) this;

	gcfg = 0;
	has_config = false;
}

Seque::~Seque()
{
	if ( has_config  )
	{	
		if(gcfg) delete gcfg;
	}
}

Amor* Seque::clone()
{
	Seque *child;
	child = new Seque();
	child->gcfg = gcfg;
	child->root = root;
	WBUG("cloned child %p", child);
	return (Amor*)child;
}

SEQUEINLINE void Seque::reset()
{
	post_req.reset();
	post_ans.reset();
}

SEQUEINLINE void Seque::retain()
{
	Amor::Pius tmp_p;

	tmp_p.ordo = Notitia::SET_SAME_PRIUS;	/* 把prius设到root上去 */
	tmp_p.indic = root;		
	aptus->sponte(&tmp_p);

	tmp_p.ordo = Notitia::CMD_FREE_IDLE;
	tmp_p.indic = this;
	gather->l_ele.remove (&l_ele);
	gather = 0;
	pipe = 0;
	aptus->sponte(&tmp_p);
}

SEQUEINLINE void Seque::appoint_me()
{
	if ( gcfg->work_mode == M_PIPE )
	{	/* for mode of PIPE, just BEGIN_TRANS, wait for TRANS_TO_HANDLE(send, recv) */
		aptus->facio(&start_trans);
	} else {
		aptus->facio(&local_pius);
	}
}

SEQUEINLINE void Seque::appoint()
{
	Seque *bee;
	Amor::Pius tmp_p;

	/* apply for an idle instance, then pass tbuf to it */
	tmp_p.ordo = Notitia::CMD_ALLOC_IDLE;
	tmp_p.indic = this;
	aptus->sponte(&tmp_p);

	if ( !(tmp_p.indic) || this == ( Amor* ) (tmp_p.indic) )
	{	/* no ilde instance */
		WLOG(NOTICE, "limited seque, to max");
		reset();
	} else {
		bee = (Seque*)(tmp_p.indic);
		bee->gather = this;
		l_ele.put ( &bee->l_ele);

		tmp_p.ordo = Notitia::SET_SAME_PRIUS;	/* pupa对象与这里相同的prius */
		tmp_p.indic = bee;		
		aptus->sponte(&tmp_p);

		bee->reset();	//右侧数据清空, 然后交换过去
		TBuffer::exchange(*pri_req, bee->post_req);
		TBuffer::exchange(*pri_ans, bee->post_ans);
		if ( gcfg->work_mode == M_PIPE )
		{	/* for mode of PIPE, just BEGIN_TRANS, wait for TRANS_TO_HANDLE(send, recv) */
			bee->aptus->facio(&bee->start_trans);
		} else {
			bee->aptus->facio(&bee->local_pius);
		}
	}
}

#include "hook.c"
