/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Aptus extend, dispaly version infomation of all modules
 Build: created by octerboy, 2006/04/01, in Wuhan
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Aptus.h"
#include "textus_load_mod.h"
#include "hook.h"
static char ld_lib_path[2048] = {0};
class Version: public Aptus {
public:
	void ignite(TiXmlElement *);
	void ignite_t (TiXmlElement *, TiXmlElement *);
	Amor *clone();

	bool facio_n (Amor::Pius *, unsigned int from);
	Version();
	~Version();

protected:
	void put_version ( const char *str, const char *str2, const char *ver, const char *so_file,  const char *sub);
	static 	TiXmlElement *modules;
	void get_module_version(TiXmlElement*);
	#include "tbug.h"
};
#include "Notitia.h"
#include "textus_string.h"
#include "Animus.h"
#include <stdio.h>

static char * r_share(const char*);

#define BUF_SIZE 512
Version::Version()
{
	WBUG("new this is %p",  this);
}

Version::~Version()
{
	WBUG("delete this is %p", this);
}

void Version::ignite_t (TiXmlElement *cfg, TiXmlElement *ver_ele)
{
	const char *ver_str;
	if ( (ver_str = ver_ele->Attribute("get")) && strcmp(ver_str, "yes") == 0)
	{
		canAccessed = true;
		need_fac = true;
	}
}

void Version::ignite(TiXmlElement *cfg)
{
	WBUG("this %p , prius %p, aptus %p, cfg %p\n", this, prius, aptus, cfg);

	Notitia::env_sub(cfg->GetDocument()->RootElement()->Attribute("path"), ld_lib_path);
	get_module_version(cfg);
}

Amor *Version::clone()
{	
	Version *child = 0;
	child = new Version();
	child->canAccessed = canAccessed;
	child->need_fac = need_fac;
	child->need_spo = need_spo;
	child->need_lae = need_lae;
	child->need_dex = need_dex;
	return  (Amor*)child;
}

bool Version::facio_n ( Amor::Pius *pius, unsigned int from)
{
	if (pius->ordo == Notitia::CMD_GET_VERSION )
	{	
		WBUG("facio GET_VERSION (Version), %p", modules);
		pius->indic = modules;
		return true;
	} else 
		return false;
}

TiXmlElement *Version::modules=0;

void  Version::put_version ( const char *scm_id, const char *time_str, const char *ver_no, const char *so_file, const char *sub_id)
{
	TiXmlElement *aver;
	const char *new_sub;
	bool found ;
	unsigned int i;
	
	if (!modules)
	{
		char *p, *q;
		TiXmlElement ver( "module" );
		TiXmlElement nm( "name" );
		TiXmlElement tm( "time" );
		TiXmlElement build( "version" );
		TiXmlElement dc( "program" );
		TiXmlText text( "" );
		modules=new TiXmlElement("Module");
		
		ver.SetAttribute("no", "0");

		text.SetValue("libanimus");
		nm.InsertEndChild(text);

		p = strpbrk(Animus::ver_buf, "\n");
		*p++ = '\0';
		q = strpbrk(p, "\n");
		*q++ = '\0';

		text.SetValue(Animus::ver_buf);
		dc.InsertEndChild(text);

		text.SetValue(p);
		tm.InsertEndChild(text);

		text.SetValue(q);
		build.InsertEndChild(text);

		ver.InsertEndChild(nm);
		ver.InsertEndChild(dc);
		ver.InsertEndChild(tm);
		ver.InsertEndChild(build);
		
		modules->InsertEndChild(ver);

		for ( i = 0 ; i < Animus::num_extension; i++)
			this->get_module_version(Animus::positor[i].carbo);
	}

	found = false; aver = modules->FirstChildElement("module");
	i = 0;
	while ( aver && !found)
	{
		if ( strcmp(aver->FirstChildElement("program")->GetText(), scm_id) == 0 
			&& strcmp(aver->FirstChildElement("name")->GetText(), so_file) == 0

		) 
		{
			found = true;
			break;
		}
		aver = aver->NextSiblingElement("module"); i++;
	}

	new_sub= sub_id;
	if (!found)
		aver = modules;
	else if ( sub_id ) 
	{	/* look for sub */
		bool has = false; 
		TiXmlElement * aSub = aver->FirstChildElement("sub");
		while ( aSub && !has)
		{
			if ( strcmp(aSub->FirstChildElement("program")->GetText(), sub_id) == 0 
				&& strcmp(aSub->FirstChildElement("version")->GetText(), ver_no) == 0 ) 
			{
				has = true;
				break;
			}
			aSub = aSub->NextSiblingElement("sub");
		}
		if ( has ) 
			new_sub = 0;	/* 已经有了子模块, 置为0, 使其不再增加 */
	}

	if ( (found && new_sub) || !found )
	{
		char str[10];
		TiXmlElement ver( "module" );
		TiXmlElement nm( "name" );
		TiXmlElement tm( "time" );
		TiXmlElement build( "version" );
		TiXmlElement dc( "program" );
		TiXmlText text( "" );

		if ( !sub_id )
		{
			TEXTUS_SNPRINTF(str, sizeof(str)-1, "%d", i);
			ver.SetAttribute("no", str);
			text.SetValue(so_file);
			nm.InsertEndChild(text);
			ver.InsertEndChild(nm);
		} else {
			ver.SetValue("sub");
		}

		if ( sub_id )
			text.SetValue(sub_id);
		else 
			text.SetValue(scm_id);

		dc.InsertEndChild(text);

		text.SetValue(time_str);
		tm.InsertEndChild(text);

		text.SetValue(ver_no);
		build.InsertEndChild(text);

		ver.InsertEndChild(dc);
		ver.InsertEndChild(tm);
		ver.InsertEndChild(build);

		aver->InsertEndChild(ver);
	}
}

void  Version::get_module_version (TiXmlElement *mod_cfg)
{
	const char* so_file;
	TMODULE ext=NULL;
	typedef void (*Get_Ver_fun)(char *, char*, char*, int) ;	
	Get_Ver_fun ver_let;

	if ( (  so_file= mod_cfg->Attribute("name")   )
		&& (  ext =TEXTUS_LOAD_MOD(r_share(so_file), 0) )
		&& (TEXTUS_GET_ADDR(ext, TEXTUS_GET_VERSION_STR, ver_let, Get_Ver_fun))
	) {
		char scm_id[BUF_SIZE], time_str[BUF_SIZE], ver_str[BUF_SIZE];
		int i;
		ver_let(scm_id,time_str, ver_str, sizeof(scm_id)-BUF_SIZE/2);
		WBUG("Load %s is %p , SCM_MODULE_ID %s",  so_file,  ext, scm_id);
		put_version (scm_id,time_str, ver_str, so_file,0);
		for ( i = 1 ; i < 10; i++ )
		{
			char fun[64], sub_id[BUF_SIZE];
			TEXTUS_SNPRINTF(fun, sizeof(fun)-1, "textus_get_version_%d", i);
			ver_let =0;
			if ((TEXTUS_GET_ADDR(ext, fun, ver_let, Get_Ver_fun)))
			{
				ver_let(sub_id, time_str, ver_str, sizeof(sub_id)-BUF_SIZE/2);
				WBUG("Load %s is %p , SCM_MODULE_ID %s",  so_file,  ext, sub_id);
				put_version (scm_id, time_str, ver_str, so_file, sub_id);
			}
		}
	}
}

char* r_share(const char *so_file)
{
	static char r_file[1024];
	int l = 0, n = 0;
	memset(r_file, 0, sizeof(r_file));
	if ( ld_lib_path[0])
	{
		l = static_cast<int>(strlen(ld_lib_path));
		if (l > 512 ) l = 512;
	 	memcpy(r_file, ld_lib_path, l);
		n = l;
	}
	l =static_cast<int>(strlen(so_file));
	if (l > 512 ) l = 512;
	memcpy(&r_file[n], so_file, l);
	TEXTUS_STRCAT(r_file, TEXTUS_MOD_SUFFIX);
	return r_file;
}

#define TEXTUS_APTUS_TAG { 'V','e','r','s','i','o','n',0}
#define TEXTUS_APTUS_MUST_IGNITE 1
#include "hook.c"
