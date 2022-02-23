/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Signal process
 Build: created by octerboy, 2006/12/26
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "casecmp.h"
#include <signal.h>
class Signal: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Amor::Pius*);
	bool sponte( Amor::Pius*);
	Amor *clone();
};

void Signal::ignite(TiXmlElement *cfg)
{
	const char *comm_str;
	TiXmlElement *sig_ele;

	for ( 	sig_ele = cfg->FirstChildElement("ignore");
		sig_ele;
		sig_ele = sig_ele->NextSiblingElement("ignore"))
	{
		comm_str = sig_ele->Attribute("name");
		if ( comm_str )
		{
	#define MYIGNORE(X)	\
			if ( strcasecmp(comm_str, #X) == 0 )	\
			{					\
				 signal(X, SIG_IGN);		\
			}

			MYIGNORE(	SIGILL );       
			MYIGNORE(	SIGABRT);       
			//MYIGNORE(	SIGEMT);        
			MYIGNORE(	SIGFPE);        
			MYIGNORE(	SIGSEGV);       
			MYIGNORE(	SIGTERM);       

		#ifdef _WIN32
			MYIGNORE(SIGBREAK);       
		#else
		#if !defined(__APPLE__)  && !defined(__FreeBSD__)  && !defined(__NetBSD__)  && !defined(__OpenBSD__)
			MYIGNORE(	SIGCLD );       
			MYIGNORE(	SIGPOLL);       
			MYIGNORE(	SIGPWR );       
		#endif

			MYIGNORE(	SIGQUIT);       
			MYIGNORE(	SIGPROF);       
			MYIGNORE(	SIGVTALRM);     
			MYIGNORE(	SIGTTOU);       
			MYIGNORE(	SIGCONT);       
			MYIGNORE(	SIGTTIN);       
			MYIGNORE(	SIGTSTP);       
			MYIGNORE(	SIGWINCH);      
			MYIGNORE(	SIGCHLD);       
			MYIGNORE(	SIGUSR1);       
			MYIGNORE(	SIGUSR2);       
			MYIGNORE(	SIGINT );       
			MYIGNORE(	SIGHUP );       
			MYIGNORE(	SIGTRAP);       
			MYIGNORE(	SIGIOT );       
			MYIGNORE(	SIGBUS);        
			MYIGNORE(	SIGSYS );       
			MYIGNORE(	SIGPIPE);       
			MYIGNORE(	SIGALRM);       
		#endif
		}
	}
}

bool Signal::facio( Amor::Pius *pius) { return false; }

bool Signal::sponte( Amor::Pius *pius) { return false; }
Amor* Signal::clone()
{
	return (Amor*) 0;
}
#include "hook.c"
