/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: PyPort
 Build:created by octerboy 2018/07/19
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "textus_string.h"
#include "PacData.h"
#include "TBuffer.h"
#include "DBFace.h"
#include "casecmp.h"
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <python2.7/Python.h>

class PyPort :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	PyPort();
	~PyPort();

private:
	PyObject *pInstance;
	PyObject * fun_ignite, *fun_facio, *fun_sponte, *fun_clone;
	struct G_CFG {
		PyObject * pModule, *pClass;
	 	TiXmlElement *run_simpleStr;
		const char *run_tag;
		const char *pyMod_str, *pyClass_str;
		inline G_CFG () {
			run_tag="run";
			run_simpleStr = 0;
			pyMod_str = pyClass_str = 0;
			pModule = pClass = 0;
		};
	};
	struct G_CFG *gCFG;
	bool has_config;
#include "wlog.h"
};

void PyPort::ignite(TiXmlElement *cfg) 
{ 
	if (!cfg) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
	}

	gCFG->run_simpleStr = cfg->FirstChildElement(gCFG->run_tag);
	gCFG->pyMod_str = cfg->Attribute("module");
	gCFG->pyClass_str = cfg->Attribute("class");
}

bool PyPort::facio( Amor::Pius *pius)
{
	assert(pius);
	const char *run_str;
	TiXmlElement *run_ele;
	switch ( pius->ordo )
	{
	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		pInstance = PyInstance_New(gCFG->pClass, NULL, NULL);
		if ( !pInstance) 
		{
			WLOG(WARNING,"PyInstance_New of class (%s) failed", gCFG->pyClass_str);
			break;
		}
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		Py_Initialize();
		if ( !Py_IsInitialized() )
		{
			WLOG(WARNING,"Py_IsInitialized failed");
			break;
    		}
		run_ele =  gCFG->run_simpleStr;
		while ( run_ele ) 
		{
			run_str= gCFG->run_simpleStr->GetText();
			if ( run_str && strlen(run_str) > 0 )
			{
				PyRun_SimpleString(run_str);
				//PyRun_SimpleString("import sys");
				//PyRun_SimpleString("sys.path.append('./')");
			}
			run_ele = run_ele->NextSiblingElement(gCFG->run_tag);
		}

		gCFG->pModule =PyImport_ImportModule(gCFG->pyMod_str);      
		if ( !gCFG->pModule) 
		{
			WLOG(WARNING,"PyImport_ImportModule (%s) failed", gCFG->pyMod_str);
			break;
		}
		gCFG->pClass = PyObject_GetAttrString(gCFG->pModule, gCFG->pyClass_str);  
		if ( !gCFG->pClass) 
		{
			WLOG(WARNING,"PyObject_GetAttrString of class (%s) failed", gCFG->pyClass_str);
			break;
		}
		pInstance = PyInstance_New(gCFG->pClass, NULL, NULL);
		if ( !pInstance) 
		{
			WLOG(WARNING,"PyInstance_New of class (%s) failed", gCFG->pyClass_str);
			break;
		}
		break;
	default:
		return false;
	}

	return true;
}

bool PyPort::sponte( Amor::Pius *pius) { 
	assert(pius);
	WBUG("sponte Notitia::%lu", pius->ordo);
	return false; 
}

Amor* PyPort::clone()
{
	PyPort *child = new PyPort();
	child->gCFG = gCFG;	
	return  (Amor*)child;
}

PyPort::PyPort() 
{
	gCFG=0;
	has_config = false;
	pInstance = 0;
}

PyPort::~PyPort() 
{ 
	Py_DECREF(pInstance);
	if ( has_config  )
	{	
		if(gCFG) delete gCFG;
		Py_Finalize();
	}
} 
#include "hook.c"

