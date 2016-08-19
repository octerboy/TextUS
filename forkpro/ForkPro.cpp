/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
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
 $Header: /textus/forkpro/ForkPro.cpp 3     08-01-11 23:56 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: ForkPro.cpp $"
#define TEXTUS_MODTIME  "$Date: 08-01-11 23:56 $"
#define TEXTUS_BUILDNO  "$Revision: 3 $"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "casecmp.h"
#include "BTool.h"
#include <assert.h>
#if !defined(_WIN32)
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#endif
#include <stdarg.h>

#define FACIO 0x01
#define SPONTE 0x02 
#define ACT_NONE 0x00

class ForkPro :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	ForkPro();
	~ForkPro();

	struct G_CFG {
		unsigned long fac_do, spo_do;
		char current;
		ForkPro **filius;
		int fil_size;
		int top;
		

		inline G_CFG (TiXmlElement *c) {
			unsigned long another ;
			const char *comm_str;

			fac_do = spo_do = another = Notitia::TEXTUS_RESERVED;
			current = ACT_NONE;
			top = 0;
			fil_size = 8;
			filius = new ForkPro* [fil_size];
			memset(filius, 0, sizeof(ForkPro*)*fil_size);

			BTool::get_textus_ordo(&fac_do, c->Attribute("facio"));
			BTool::get_textus_ordo(&spo_do, c->Attribute("sponte"));
			BTool::get_textus_ordo(&another, c->Attribute("ordo"));
		
			if ( another > 0 )
				fac_do = spo_do = another;

			comm_str = c->Attribute("this");	/* this表明哪个方向发送消息 只是在本对象进行, 而非所有对象 */
			if ( !comm_str) return;
			if ( strcasecmp(comm_str, "facio") == 0 ) current = FACIO;
			if ( strcasecmp(comm_str, "sponte") == 0 ) current = SPONTE;
			if ( strcasecmp(comm_str, "both") == 0 ) current = FACIO | SPONTE;
		};

		inline ~G_CFG () {
			delete[] filius;
			top = 0;
		};

		inline void put(ForkPro *p) {
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
				ForkPro **tmp;
				tmp = new ForkPro *[fil_size*2];
				memcpy(tmp, filius, fil_size*(sizeof(ForkPro *)));
				fil_size *=2;
				delete[] filius;
				filius = tmp;
			}
		};

		inline ForkPro* get(int i) { return filius[i]; };

		inline void remove(ForkPro *p) {
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
					filius[j] = (ForkPro *) 0;
					k++;
				}
			}
			top -= k;
		};
	};
	struct G_CFG *gCFG;
	bool has_config;

	bool do_fork(Amor::Pius*, const char*, int dir);

private:
	pid_t pid;
	Amor::Pius fps;

#include "wlog.h"
};

void ForkPro::ignite(TiXmlElement *cfg) 
{
	if (!cfg) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
	gCFG->put(this);
}

bool ForkPro::facio( Amor::Pius *pius)
{
	assert(pius);
	return do_fork(pius, "facio", FACIO);
}

bool ForkPro::sponte( Amor::Pius *pius) 
{ 
	assert(pius);
	return do_fork(pius, "sponte", SPONTE);
}

Amor* ForkPro::clone()
{
	ForkPro *child = new ForkPro();
	if ( gCFG ) gCFG->put(child);
	child->gCFG = gCFG;
	return  (Amor*)child;
}

ForkPro::ForkPro() { 
	gCFG = 0;
	has_config = false;
}

ForkPro::~ForkPro() {
	if ( gCFG )
	{
		gCFG->remove(this);
		if ( has_config )
			delete gCFG;
	}
} 

bool ForkPro::do_fork( Amor::Pius *pius, const char *str, int dir)
{
	int i;
	switch (pius->ordo)
	{
	case Notitia::CMD_FORK:
		WBUG("%s CMD_FORK", str);
DO:
		pid = fork();
		if ( pid ==0 ) 
		{	/* child */
			fps.ordo = Notitia::FORKED_CHILD;
			fps.indic = 0;

		} else if ( pid > 0 ) {
			/* parent */
			fps.ordo = Notitia::FORKED_PARENT;
			fps.indic = &pid;

		} else {
			WLOG_OSERR("fork");
			goto END;
		}

		if ( gCFG->current & SPONTE )
			aptus->sponte(&fps);

		if ( gCFG->current & FACIO )
			aptus->facio(&fps);

		for ( i = 0 ; i < gCFG->top; i++)
		{
			ForkPro *f = gCFG->get(i);
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

#include "hook.c"

