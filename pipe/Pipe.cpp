/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Pipe
 Descr: 
	1、multi request pipe to a server, response has the original order.
	2、respone will be delivered to the original object
 Build: created by octerboy, 2005/06/20, Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#define P_IDLE 0
#define REQUESTING 1
#define CANCELLED 2
#include "Amor.h"
#include "Notitia.h"
#include "textus_string.h"
#include <stdarg.h>

struct AppcData
{
	Amor *pupa;
	Amor::Pius pius;
	int status;
	struct AppcData *next;
	inline void clear() {
		pupa = 0;
		next = 0;
		status = P_IDLE;
		pius.ordo = Notitia::TEXTUS_RESERVED;
		pius.indic = (void*) 0;
	};
	inline AppcData() {
		clear();
	};
};

class DataStack
{
public:
	int top;
	int max;
	AppcData **appcDataArray;
	DataStack();
	~DataStack();
	AppcData * pop();
	void push(AppcData * element);
};

#include <assert.h>
#include "casecmp.h"

/* 以堆栈的方式来实现一个对象池, 这段代码除了AppcData这个类名外，其他是通用，也即可以作为模板 */
DataStack::DataStack()
{
	top = 0;
	max = 128;
	appcDataArray = new AppcData* [max];
	for(int i = 0; i < max; i++)
	{
		appcDataArray[i] = new AppcData;
 	}
}

DataStack::~DataStack()
{
	for(int i = 0; i < max; i++)
		delete appcDataArray[i];

	delete [] appcDataArray;
}

AppcData * DataStack::pop()// 出栈
{
	int mid = top;
	if (mid >= max)
	{
		int old = max;
		AppcData **o_arr = appcDataArray;
		max += 128;
		appcDataArray = new AppcData* [max];
		memcpy(appcDataArray, o_arr, sizeof(AppcData*) * old );	/* 原数据保存 */
		for(int i = old; i < max; i++)
		{
			appcDataArray[i] = new AppcData;
			assert(appcDataArray[i] != (AppcData *) 0);
 		}
		delete[] o_arr;
	}
	top++;
	return appcDataArray[mid];
}

void DataStack::push(AppcData * element)// 入栈
{
	if (top > 0)
	{
		top--;
		appcDataArray[top] = element;
	}
}

/* 队列, 这里除了AppcData这个对象名外，其他是通用的, 即这可以作用模板 */
class Queue:public AppcData
{
public:
	AppcData * remove();
	AppcData * peek();
	bool append(AppcData * data);
	void clear();
};

bool Queue::append(AppcData * data)
{
	/* 入队, DR#:CppFifo-002 */
	if (next!= 0)
	{
		data->next = next->next;
		next->next = data;
	} else
	{
		data->next = data;
	}
	next = data;	
	/* End of 入队 */

	return true;
}

AppcData * Queue::remove()
{
	/* 出队, DR#:CppFifo-002 */
	AppcData *list = next;
	if (list != 0)
	{
		list =list->next;
		if (list != next)
		{
			next->next = list->next;
		} else
		{
			next = 0;
		}
		list->next = 0;
	}
	/* End of 出队 */
	return list;
}

AppcData * Queue::peek()
{
	AppcData *list = next;
	if (list != 0)
	{
		list =list->next;
	}
	return list;
}

void Queue::clear()
{
	next = (AppcData*) 0;
}

class Pipe: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Amor::Pius*);
	bool sponte( Amor::Pius*);
	Amor *clone();
	
	Pipe();
	~Pipe();

private:
	bool alive;	/* 通道是否打开 */
	bool demanding;	/* 正在要求通道, 还未有响应 */

	bool isWorking;
	int *weight;
	AppcData *worker;
	TEXTUS_ORDO concern_ans[16];	/* 响应的ordo, 设定为16个, 最后一个为-1 */
	Amor::Pius chn_timeout;
	Amor::Pius dmd_start;
	Amor::Pius clr_timer_pius, alarm_pius;	/* 清超时, 设超时 */
	void *arr[3];

	Queue que;
	struct G_CFG
	{
		DataStack *pool;
		bool async ;	/* 同步异步标志 */
		int expired;	/* 超时时间。0: 不设超时 */
		bool once;

		inline ~G_CFG() { 
			delete[] pool;
		};

		inline G_CFG(TiXmlElement *cfg) 
		{
			const char *async_str;
			const char *comm_str;

			expired = 0;
			once = false;

			pool = new DataStack;

			async = true;	/* 默认为异步 */
			async_str = cfg->Attribute("async");
			if ( async_str )
			{
				if ( strcasecmp(async_str, "yes") == 0 )
					async = true;
				if ( strcasecmp(async_str, "no") == 0 )
					async = false;
			}

			comm_str = cfg->Attribute("once");	//通道在结束交易时关闭，也即一次性
			if ( comm_str && strcasecmp(comm_str, "yes" ) ==0 )
				once = true;

			cfg->QueryIntAttribute("expired", &(expired));
		};
	};

	struct G_CFG *gcfg;  
	bool has_config;
	
	inline void worker_begin_trans();
	inline void demand_pro();
	inline void deliver(Notitia::HERE_ORDO aordo);
#include "wlog.h"
};

Pipe::Pipe()
{
	isWorking = false;
	worker = 0;
	gcfg = 0;
	has_config = false;
	for ( int i = 0; i < 16; i++ )
		concern_ans[i] = Notitia::TEXTUS_RESERVED;

	chn_timeout.ordo =  Notitia::CHANNEL_TIMEOUT;
	chn_timeout.indic = 0;

	dmd_start.indic = 0;
	dmd_start.ordo = Notitia::DMD_START_SESSION;

	clr_timer_pius.ordo = Notitia::DMD_CLR_TIMER;
	clr_timer_pius.indic = 0;

	alarm_pius.ordo = Notitia::DMD_SET_ALARM;
	alarm_pius.indic = &arr[0];
}

Pipe::~Pipe()
{
	if ( has_config  )
	{	
		if(gcfg) delete gcfg;
	}
}

Amor* Pipe::clone()
{
	Pipe *child;

	child = new Pipe();
	WBUG("cloned child %p\n", child);

	child->gcfg = gcfg;
	return (Amor*)child;
}

void Pipe::ignite(TiXmlElement *cfg)
{
	if ( !cfg ) 
		return;
	if ( !gcfg ) 
	{
		gcfg = new struct G_CFG(cfg);
		has_config = true;
	}
}

bool Pipe::facio( Amor::Pius *pius)
{
	Amor::Pius set_pius;
	void **p, **q;
	AppcData *ne;
	int i;
	AppcData **array;

	assert(pius);
	switch (pius->ordo )
	{
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		arr[0] = this;
		arr[1] = &(gcfg->expired);
		arr[2] = 0;
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		arr[0] = this;
		arr[1] = &(gcfg->expired);
		arr[2] = 0;
		break;

	case  Notitia::SET_WEIGHT_POINTER:	/* indic指向一个指针数组, 第一个为Amor *obj, 第二个为pius*/
		WBUG(" Notitia::SET_WEIGHT_POINTER");
		weight = (int*)(pius->indic);
		break;

	case Notitia::CMD_CANCEL_TRANS:	/* indic指向一个指针数组, 第一个为Amor *obj, 第二个为pius*/
		WBUG("CMD_CANCLE_TRANS(%p)", (Amor*)(pius->indic));
		array = gcfg->pool->appcDataArray;
		for ( i = 0 ; i < gcfg->pool->top; i++)
		{
			if ( array[i]->pupa == (Amor*)(pius->indic)) //该pupa发出了CANCEL
			{
				array[i]->status = CANCELLED;
			}
		}
		break;

	case Notitia::DMD_START_SESSION:	
		WBUG("facio DMD_START_SESSION alive=%d", alive);
		if (!alive)
			demand_pro();
		break;

	case Notitia::CMD_BEGIN_TRANS:	/* indic指向Amor *obj 及后续的一系列ordo, */
		WBUG("CMD_BEGIN_TRANS");
		q = p = (void**)(pius->indic);
		assert(p);
		q++;	
		ne = gcfg->pool->pop();
		ne->pupa = (Amor*)*p;
		assert(ne->pupa);
		ne->status = REQUESTING;
		for ( i = 0 ; i < 16; i++ )	/* 后续ordo是响应时判别, 直接给pupa */
		{
			TEXTUS_ORDO *oo;
			oo = (TEXTUS_ORDO *)*q;
			if ( oo )
				concern_ans[i] = *oo;
			else 
			{
				concern_ans[i] = Notitia::TEXTUS_RESERVED;
				break;
			}
			if ( concern_ans[i] == Notitia::TEXTUS_RESERVED ) break;
		}
		//ne->pius.ordo = gcfg->concern_req;
		//ne->pius.indic = ((Amor::Pius *)q)->indic;
		que.append(ne);	
		(*weight)++;	//负载增加
		
		if (!alive )
		{
			demand_pro();
		} else {
			worker_begin_trans();
		}
		break;

	case Notitia::TIMER:	/* 连接超时 */
		WBUG("facio TIMER" );
		if ( demanding)
		{
			WLOG(WARNING, "channel time out");
			//aptus->sponte(&clr_timer_pius);	/* tpoll先前已经清了, 清除定时 */
			aptus->sponte(&chn_timeout);	/* 向左通知 */
		}
		break;

	case Notitia::TIMER_HANDLE:
		WBUG("facio TIMER_HANDLE");
		clr_timer_pius.indic = pius->indic;
		break;

	case Notitia::CMD_END_TRANS:	/* indic指向Amor *obj */
		WBUG("CMD_END_TRANS %p", (Amor*)(pius->indic));

		isWorking = false;
		ne = que.remove();
		if (!ne ) 
		{ 
			WLOG(ALERT, "First is null!"); //不该
			goto HI_END;
		}

		if ( ne->pupa != worker->pupa || ne->pupa != (Amor*)(pius->indic) )
		{
			WLOG(ALERT, "End object is not equal from worker(%p), pupa(%p)", worker->pupa, ne->pupa);
			goto HI_END;
		}

		if ( ne->status != REQUESTING )
		{
			WLOG(NOTICE, "Ended object(%p) is not REQUESTING"); //已经取消, 提示一下
		}
		
		ne->clear();	/* 清空 */
		gcfg->pool->push(ne);		/* 将之还给池 */
		(*weight)--;	//负载减

		worker = que.peek();	//准备下一个处理
		if ( !worker ) goto HI_END;	//队列已空

		if ( gcfg->async )	//异步, 先前已经发出数据, 这里等待接收
		{
			if ( worker->status != REQUESTING )
			{
				WLOG(NOTICE, "Async next object(%p) is not REQUESTING"); //即便已经取消, 也要接收
			}
			set_pius.ordo = Notitia::TRANS_TO_RECV;	
			set_pius.indic = this;
			worker->pupa->sponte(&set_pius);
		} else {		//同步方式, 轮到他开始交易
			while ( worker != 0 && worker->status != REQUESTING )
			{
				ne = que.remove();	//worker就是ne
				ne->clear();	/* 清空 */
				gcfg->pool->push(ne);		/* 将之还给池 */
				(*weight)--;	//负载减
				set_pius.ordo = Notitia::CMD_RETAIN_TRANS;		//前面的sequence知道该释放了
				set_pius.indic = this;
				ne->pupa->sponte(&set_pius);
				worker = que.peek();	//看下一个的情况
			}
			if ( worker ) 
			{
				set_pius.ordo = Notitia::TRANS_TO_HANDLE;/* 通知请求源, this要求设置数据区并发送等 */
				set_pius.indic = this;
				worker->pupa->sponte(&set_pius);
			}
		}
		if ( gcfg->once)	//一次性使用通道, 关闭
			 deliver(Notitia::DMD_END_SESSION);
	HI_END:
		break;
	default:
		return false;
	}
	return true;
}

void Pipe::worker_begin_trans() 
{
	Amor::Pius set_pius;
	worker = que.peek();	//第一个进入的, 
	if ( gcfg->async )
	{
		isWorking = true;	//异步方式, 将数据发出去
		set_pius.ordo = Notitia::TRANS_TO_SEND;	/* 通知请求源, this才是准备发送请求, 要求设置数据区等 */
		set_pius.indic = this;
		worker->pupa->sponte(&set_pius);

		set_pius.ordo = Notitia::TRANS_TO_RECV;	//接收,还是最初的那一个
		set_pius.indic = this;
		//{int *a =0 ; *a = 0; };	
		worker->pupa->sponte(&set_pius);
	} else {
		if ( !isWorking ) 	//对于同步方式, 如果仍在工作, 那么不发送请求, 等当前worker结束了再开始下一个
		{
			isWorking = true;
			set_pius.ordo = Notitia::TRANS_TO_HANDLE;/* 通知请求源, this要求设置数据区并发送等 */
			set_pius.indic = this;
			worker->pupa->sponte(&set_pius);
		}
	}
}

void Pipe::demand_pro() 
{
	if( !demanding )
	{
		demanding = true;			/* 置标志 */
		if( gcfg->expired > 0 )			/* 如果设了超时 */
			aptus->sponte(&alarm_pius);
		aptus->facio(&dmd_start);
	}
}

bool Pipe::sponte( Amor::Pius *pius) 
{ 
	AppcData *ne;
	int i;
	switch (pius->ordo )
	{
	case Notitia::START_SESSION:	/* channel is alive */
		WBUG("sponte START_SESSION");

		aptus->sponte(&clr_timer_pius);	/* 清除定时, 初始为0,  */
		/* 以下就不必要
		if ( demanding )
		{
			if ( gcfg->expired > 0 )
				aptus->sponte(&clr_timer_pius);	
		} 
		*/
		alive = true;
		demanding = false;
		worker_begin_trans();

	case Notitia::DMD_END_SESSION:	/* 本通道关闭，所有队列中应该重新分配 */
		WBUG("sponte DMD_END_SESSION");
		alive = false;
		demanding = false;

		aptus->sponte(&clr_timer_pius);	/* 清除定时, 初始为0,  */
		/* 以下就不必要
		if ( demanding && gcfg->expired > 0 )
		{
			aptus->sponte(&clr_timer_pius);
		}
		*/

		for ( ne = que.remove(); ne; ne=que.remove())
		{
			Amor::Pius tmp;
			ne->clear();	/* 清空 */
			gcfg->pool->push(ne);		/* 将之还给池 */
		
			tmp.ordo = Notitia::CMD_FAIL_TRANS;	//交易失败
			tmp.indic = this;
			ne->pupa->sponte(&tmp);
		}
		break;

	default:
		for ( i = 0 ; i < 16 && concern_ans[i] != Notitia::TEXTUS_RESERVED; i++ )
		{
			if ( pius->ordo == concern_ans[i] )
			{	
				if ( worker )
				{
					worker->pupa->sponte(pius); //不管如何, 都要把响应数据回过去, 其它由sequence处理
				} else {
					WLOG(ALERT, "worker is null"); //这是不应该的
				}
				return true;
			}
		}
		return false;
	}
	return true; 
}

/* 向接力者提交 */
inline void Pipe::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	
	switch (aordo)
	{
	case Notitia::DMD_END_SESSION:
		WBUG("deliver DMD_END_SESSION");
		tmp_pius.indic = 0;
		break;

	default:
		WBUG("deliver Notitia::%d",aordo);
		break;
	}
	aptus->facio(&tmp_pius);
	return ;
}

#include "hook.c"
