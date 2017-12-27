/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: fork process and admin
 Build:created by octerboy 2008/01/07, Panyu
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "textus_string.h"
#include "casecmp.h"
#include <assert.h>
#include <time.h>
#if !defined(_WIN32)
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#endif

#define FACIO 0x01
#define SPONTE 0x02 
#define ACT_NONE 0x00

typedef struct _ProcInfo {
	pid_t id;
	time_t start;
	
} ProcInfo;

typedef struct _Array {
	void **filius;
	int fil_size;
	int top;

	inline _Array () {
		top = 0;
		fil_size = 8;
		filius = new void* [fil_size];
		memset(filius, 0, sizeof(void*)*fil_size);
	};

	
	inline void put(void *p) {
		int i;
		for (i =0; i < top; i++)
		{
			if ( p == filius[i] ) 
				return ;
		}
		filius[top] = p;
		top++;
		if ( top == fil_size) 
		{
			void **tmp;
			tmp = new void *[fil_size*2];
			memcpy(tmp, filius, fil_size*(sizeof(void *)));
			fil_size *=2;
			delete[] filius;
			filius = tmp;
		}
	};

	inline void* get(int i) { return filius[i]; };

	inline void remove(void *p) {
		int i, j, k;
		k = 0;
		for (i =0; i < top; i++)
		{
			if ( p == filius[i] ) 
			{
				for (  j = i; j+1 < top; j++)
				{
					filius[j] = filius[j+1];
				}
				filius[j] = (void *) 0;
				k++;
			}
		}
		top -= k;
	};

} Array; 
		

static int maxium_procs = 0;
static int current_procs = 0;

static Array *sub_procs = 0;
void pro_child(int signo,  siginfo_t *info, void *u);
class Process :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	Process();
	~Process();

	struct G_CFG {
		TEXTUS_ORDO fac_do, spo_do;	/* facio(sponte)中进行fork的条件 */
		char current;	/* 是否只在本对象facio或sponte FORKED消息 */

		Array objects;

		int maxProcs;	/* 最大并发进程数 */

		struct sigaction act;

		inline G_CFG (TiXmlElement *c) {
			TEXTUS_ORDO another ;
			const char *comm_str;

			fac_do = spo_do = another = Notitia::TEXTUS_RESERVED;
			current = ACT_NONE;
			//BTool::get_textus_ordo(&fac_do, c->Attribute("facio"));
			//BTool::get_textus_ordo(&spo_do, c->Attribute("sponte"));
			//BTool::get_textus_ordo(&another, c->Attribute("ordo"));
			fac_do = Notitia::get_ordo(c->Attribute("facio"));
			spo_do = Notitia::get_ordo(c->Attribute("sponte"));
			another = Notitia::get_ordo(c->Attribute("ordo"));
		
			if ( another > 0 )
				fac_do = spo_do = another;

			comm_str = c->Attribute("this");	/* this表明哪个方向发送消息 只是在本对象进行, 而非所有对象 */
			if ( !comm_str) goto NEXT;
			if ( strcasecmp(comm_str, "facio") == 0 ) current = FACIO;
			if ( strcasecmp(comm_str, "sponte") == 0 ) current = SPONTE;
			if ( strcasecmp(comm_str, "both") == 0 ) current = FACIO | SPONTE;
		NEXT:
			maxProcs = 2;
			c->QueryIntAttribute("maxium", &maxProcs);

			act.sa_handler = 0;
			act.sa_sigaction = pro_child;
 
			/* 在这个范例程序里，我们不想阻塞其它信号 */
			sigemptyset(&act.sa_mask);
    
			/*
			* 我们只关心被终止的子进程，而不是被中断
			* 的子进程 (比如用户在终端上按Control-Z)
			*/
			act.sa_flags = SA_NOCLDSTOP | SA_SIGINFO;
		};

		inline ~G_CFG () { };

	};
	struct G_CFG *gCFG;
	bool has_config;

	bool do_fork(Amor::Pius*, const char*, int dir);

private:
	pid_t pid;
	Amor::Pius fps;

#include "wlog.h"
};

void Process::ignite(TiXmlElement *cfg) 
{
	if (!cfg) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
	gCFG->objects.put(this);
	if ( gCFG->maxProcs > 0 )
	{
		maxium_procs = gCFG->maxProcs;
	}
	if ( !sub_procs)
		sub_procs = new Array;
}

bool Process::facio( Amor::Pius *pius)
{
	assert(pius);
	switch (pius->ordo)
	{
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		if (gCFG->maxProcs > 0 && sigaction(SIGCHLD, &gCFG->act, NULL) < 0)
		{
			WLOG_OSERR("sigaction failed");
		} else if ( gCFG->maxProcs > 0 )
		{
			WLOG(INFO,"sigaction success!");
		} else
			WLOG(INFO, "no sigaction");
		break;
	default:
		return do_fork(pius, "facio", FACIO);
	}
	return true;
}

bool Process::sponte( Amor::Pius *pius) 
{ 
	assert(pius);
	return do_fork(pius, "sponte", SPONTE);
}

Amor* Process::clone()
{
	Process *child = new Process();
	if ( gCFG ) gCFG->objects.put(child);
	child->gCFG = gCFG;
	return  (Amor*)child;
}

Process::Process() { 
	gCFG = 0;
	has_config = false;
}

Process::~Process() {
	if ( has_config )
		delete gCFG;
	else if ( gCFG )
		gCFG->objects.remove(this);
} 

bool Process::do_fork( Amor::Pius *pius, const char *str, int dir)
{
	int i;
	switch (pius->ordo)
	{
	case Notitia::CMD_FORK:
		WBUG("%s CMD_FORK", str);
DO:
		if ( gCFG->maxProcs > 0 &&  current_procs >= gCFG->maxProcs   )
		{
			fps.ordo = Notitia::FORKED_PARENT;
			fps.indic = &pid;
			WLOG(WARNING, "children to limit %d, no child will be forked", gCFG->maxProcs);
			goto INFO;
		}	
		pid = fork();
		if ( pid ==0 ) 
		{	/* child */
			fps.ordo = Notitia::FORKED_CHILD;
			fps.indic = 0;

		} else if ( pid > 0 ) {
			/* parent */
			ProcInfo *pi;
			fps.ordo = Notitia::FORKED_PARENT;
			fps.indic = &pid;
			current_procs++;
				
			pi = new ProcInfo;
			pi->id = pid;
			pi->start = time(0);
			sub_procs->put(pi);
			
		} else {
			WLOG_OSERR("fork");
			goto END;
		}

	INFO:
		if ( gCFG->current & SPONTE )
			aptus->sponte(&fps);

		if ( gCFG->current & FACIO )
			aptus->facio(&fps);

		for ( i = 0 ; i < gCFG->objects.top; i++)
		{
			Process *f = (Process*) gCFG->objects.get(i);
			if (f)
			{
				if ( ! ( gCFG->current & SPONTE) )
					f->aptus->sponte(&fps);
				if ( ! ( gCFG->current & FACIO) )
					f->aptus->facio(&fps);
			}
		}
		break;
	default:
		if ( (pius->ordo == gCFG->fac_do && dir == FACIO) 
			|| (pius->ordo == gCFG->spo_do && dir == SPONTE)
		 )
		{
			WBUG("%s Notitia::%lu", str, pius->ordo);
			goto DO;
		} else
			return false;
	}
END:
	return true;
}

void pro_child(int signo,  siginfo_t *info, void *u)
{
	int i, status;//, child_val;
	ProcInfo *pi ;
	pid_t child;
	bool foundChild = false;
AGAIN:
	child = waitpid(-1, &status, WNOHANG);
	if ( child == 0) return;
	if ( child < 0) {
		//perror(strerror(errno));
		return;
	}

	if (WIFEXITED(status))                /* 子进程是正常退出吗? */
	{
		//child_val = WEXITSTATUS(status); /* 获取子进程的退出状态 */
	} else 
		fprintf(stderr, "child's exited abnormally\n");

	foundChild = false;
	for ( i = 0 ; i < sub_procs->top; i++)
	{
		pi = (ProcInfo*) sub_procs->get(i);
		if ( pi->id == child )
		{
			sub_procs->remove(pi);
			delete pi;
			foundChild = true;
			break;
		}
	}
	
	if ( foundChild ) current_procs--;
	assert(current_procs >=0 );
	goto AGAIN;
}

#include "hook.c"

