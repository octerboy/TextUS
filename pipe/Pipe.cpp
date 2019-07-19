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
	1��multi request pipe to a server, response has the original order.
	2��respone will be delivered to the original object
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

/* �Զ�ջ�ķ�ʽ��ʵ��һ�������, ��δ������AppcData��������⣬������ͨ�ã�Ҳ��������Ϊģ�� */
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

AppcData * DataStack::pop()// ��ջ
{
	int mid = top;
	if (mid >= max)
	{
		int old = max;
		AppcData **o_arr = appcDataArray;
		max += 128;
		appcDataArray = new AppcData* [max];
		memcpy(appcDataArray, o_arr, sizeof(AppcData*) * old );	/* ԭ���ݱ��� */
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

void DataStack::push(AppcData * element)// ��ջ
{
	if (top > 0)
	{
		top--;
		appcDataArray[top] = element;
	}
}

/* ����, �������AppcData����������⣬������ͨ�õ�, �����������ģ�� */
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
	/* ���, DR#:CppFifo-002 */
	if (next!= 0)
	{
		data->next = next->next;
		next->next = data;
	} else
	{
		data->next = data;
	}
	next = data;	
	/* End of ��� */

	return true;
}

AppcData * Queue::remove()
{
	/* ����, DR#:CppFifo-002 */
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
	/* End of ���� */
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
	bool alive;	/* ͨ���Ƿ�� */
	bool demanding;	/* ����Ҫ��ͨ��, ��δ����Ӧ */

	bool isWorking;
	int *weight;
	AppcData *worker;
	TEXTUS_ORDO concern_ans[16];	/* ��Ӧ��ordo, �趨Ϊ16��, ���һ��Ϊ-1 */
	Amor::Pius chn_timeout;
	Amor::Pius dmd_start;
	Amor::Pius clr_timer_pius, alarm_pius;	/* �峬ʱ, �賬ʱ */
	void *arr[3];

	Queue que;
	struct G_CFG
	{
		DataStack *pool;
		bool async ;	/* ͬ���첽��־ */
		int expired;	/* ��ʱʱ�䡣0: ���賬ʱ */
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

			async = true;	/* Ĭ��Ϊ�첽 */
			async_str = cfg->Attribute("async");
			if ( async_str )
			{
				if ( strcasecmp(async_str, "yes") == 0 )
					async = true;
				if ( strcasecmp(async_str, "no") == 0 )
					async = false;
			}

			comm_str = cfg->Attribute("once");	//ͨ���ڽ�������ʱ�رգ�Ҳ��һ����
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

	case  Notitia::SET_WEIGHT_POINTER:	/* indicָ��һ��ָ������, ��һ��ΪAmor *obj, �ڶ���Ϊpius*/
		WBUG(" Notitia::SET_WEIGHT_POINTER");
		weight = (int*)(pius->indic);
		break;

	case Notitia::CMD_CANCEL_TRANS:	/* indicָ��һ��ָ������, ��һ��ΪAmor *obj, �ڶ���Ϊpius*/
		WBUG("CMD_CANCLE_TRANS(%p)", (Amor*)(pius->indic));
		array = gcfg->pool->appcDataArray;
		for ( i = 0 ; i < gcfg->pool->top; i++)
		{
			if ( array[i]->pupa == (Amor*)(pius->indic)) //��pupa������CANCEL
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

	case Notitia::CMD_BEGIN_TRANS:	/* indicָ��Amor *obj ��������һϵ��ordo, */
		WBUG("CMD_BEGIN_TRANS");
		q = p = (void**)(pius->indic);
		assert(p);
		q++;	
		ne = gcfg->pool->pop();
		ne->pupa = (Amor*)*p;
		assert(ne->pupa);
		ne->status = REQUESTING;
		for ( i = 0 ; i < 16; i++ )	/* ����ordo����Ӧʱ�б�, ֱ�Ӹ�pupa */
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
		(*weight)++;	//��������
		
		if (!alive )
		{
			demand_pro();
		} else {
			worker_begin_trans();
		}
		break;

	case Notitia::TIMER:	/* ���ӳ�ʱ */
		WBUG("facio TIMER" );
		if ( demanding)
		{
			WLOG(WARNING, "channel time out");
			//aptus->sponte(&clr_timer_pius);	/* tpoll��ǰ�Ѿ�����, �����ʱ */
			aptus->sponte(&chn_timeout);	/* ����֪ͨ */
		}
		break;

	case Notitia::TIMER_HANDLE:
		WBUG("facio TIMER_HANDLE");
		clr_timer_pius.indic = pius->indic;
		break;

	case Notitia::CMD_END_TRANS:	/* indicָ��Amor *obj */
		WBUG("CMD_END_TRANS %p", (Amor*)(pius->indic));

		isWorking = false;
		ne = que.remove();
		if (!ne ) 
		{ 
			WLOG(ALERT, "First is null!"); //����
			goto HI_END;
		}

		if ( ne->pupa != worker->pupa || ne->pupa != (Amor*)(pius->indic) )
		{
			WLOG(ALERT, "End object is not equal from worker(%p), pupa(%p)", worker->pupa, ne->pupa);
			goto HI_END;
		}

		if ( ne->status != REQUESTING )
		{
			WLOG(NOTICE, "Ended object(%p) is not REQUESTING"); //�Ѿ�ȡ��, ��ʾһ��
		}
		
		ne->clear();	/* ��� */
		gcfg->pool->push(ne);		/* ��֮������ */
		(*weight)--;	//���ؼ�

		worker = que.peek();	//׼����һ������
		if ( !worker ) goto HI_END;	//�����ѿ�

		if ( gcfg->async )	//�첽, ��ǰ�Ѿ���������, ����ȴ�����
		{
			if ( worker->status != REQUESTING )
			{
				WLOG(NOTICE, "Async next object(%p) is not REQUESTING"); //�����Ѿ�ȡ��, ҲҪ����
			}
			set_pius.ordo = Notitia::TRANS_TO_RECV;	
			set_pius.indic = this;
			worker->pupa->sponte(&set_pius);
		} else {		//ͬ����ʽ, �ֵ�����ʼ����
			while ( worker != 0 && worker->status != REQUESTING )
			{
				ne = que.remove();	//worker����ne
				ne->clear();	/* ��� */
				gcfg->pool->push(ne);		/* ��֮������ */
				(*weight)--;	//���ؼ�
				set_pius.ordo = Notitia::CMD_RETAIN_TRANS;		//ǰ���sequence֪�����ͷ���
				set_pius.indic = this;
				ne->pupa->sponte(&set_pius);
				worker = que.peek();	//����һ�������
			}
			if ( worker ) 
			{
				set_pius.ordo = Notitia::TRANS_TO_HANDLE;/* ֪ͨ����Դ, thisҪ�����������������͵� */
				set_pius.indic = this;
				worker->pupa->sponte(&set_pius);
			}
		}
		if ( gcfg->once)	//һ����ʹ��ͨ��, �ر�
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
	worker = que.peek();	//��һ�������, 
	if ( gcfg->async )
	{
		isWorking = true;	//�첽��ʽ, �����ݷ���ȥ
		set_pius.ordo = Notitia::TRANS_TO_SEND;	/* ֪ͨ����Դ, this����׼����������, Ҫ�������������� */
		set_pius.indic = this;
		worker->pupa->sponte(&set_pius);

		set_pius.ordo = Notitia::TRANS_TO_RECV;	//����,�����������һ��
		set_pius.indic = this;
		//{int *a =0 ; *a = 0; };	
		worker->pupa->sponte(&set_pius);
	} else {
		if ( !isWorking ) 	//����ͬ����ʽ, ������ڹ���, ��ô����������, �ȵ�ǰworker�������ٿ�ʼ��һ��
		{
			isWorking = true;
			set_pius.ordo = Notitia::TRANS_TO_HANDLE;/* ֪ͨ����Դ, thisҪ�����������������͵� */
			set_pius.indic = this;
			worker->pupa->sponte(&set_pius);
		}
	}
}

void Pipe::demand_pro() 
{
	if( !demanding )
	{
		demanding = true;			/* �ñ�־ */
		if( gcfg->expired > 0 )			/* ������˳�ʱ */
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

		aptus->sponte(&clr_timer_pius);	/* �����ʱ, ��ʼΪ0,  */
		/* ���¾Ͳ���Ҫ
		if ( demanding )
		{
			if ( gcfg->expired > 0 )
				aptus->sponte(&clr_timer_pius);	
		} 
		*/
		alive = true;
		demanding = false;
		worker_begin_trans();

	case Notitia::DMD_END_SESSION:	/* ��ͨ���رգ����ж�����Ӧ�����·��� */
		WBUG("sponte DMD_END_SESSION");
		alive = false;
		demanding = false;

		aptus->sponte(&clr_timer_pius);	/* �����ʱ, ��ʼΪ0,  */
		/* ���¾Ͳ���Ҫ
		if ( demanding && gcfg->expired > 0 )
		{
			aptus->sponte(&clr_timer_pius);
		}
		*/

		for ( ne = que.remove(); ne; ne=que.remove())
		{
			Amor::Pius tmp;
			ne->clear();	/* ��� */
			gcfg->pool->push(ne);		/* ��֮������ */
		
			tmp.ordo = Notitia::CMD_FAIL_TRANS;	//����ʧ��
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
					worker->pupa->sponte(pius); //�������, ��Ҫ����Ӧ���ݻع�ȥ, ������sequence����
				} else {
					WLOG(ALERT, "worker is null"); //���ǲ�Ӧ�õ�
				}
				return true;
			}
		}
		return false;
	}
	return true; 
}

/* ��������ύ */
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
