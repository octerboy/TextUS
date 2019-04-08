/* Copyright (c) 2019-2022 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
/**
 Title: Buffer for Timer
 Build: created by octerboy, 2019/03/19, Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "BTool.h"
#include "PacData.h"
#include "casecmp.h"
#include "textus_string.h"
#include <time.h>
#include <ctype.h>
#include <stdarg.h>
#include <assert.h>

class XmlConf: public Amor
{
public:
	int count;
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	XmlConf();
	~XmlConf();

private:
	Amor::Pius pro_pac;  /* 清超时, 设超时 */
	PacketObj *rcv_pac, *snd_pac;	/* 来自左节点的PacketObj */
	TiXmlDocument req_doc, res_doc;	//config request
	TiXmlComment comment;
	TiXmlDeclaration *dec;
	
	void wfile();

	struct G_CFG {
		TiXmlDocument doc_cfg;	//xml config file
		TiXmlElement *cfg_ref;	//xml reference element
		const char *cfg_file;
		int act_fld, xml_fld;
		inline G_CFG ( TiXmlElement *cfg ) {
			cfg_file = cfg->Attribute("file");
			cfg_ref = cfg->FirstChildElement("refer");
			act_fld = 2;
			xml_fld =3 ;
			cfg->QueryIntAttribute( "action", &act_fld );
			cfg->QueryIntAttribute( "xml", &xml_fld );
		};
	};
	struct G_CFG *gCFG;	/* Shared for all objects in this node */
	bool has_config;

#include "wlog.h"
};

void XmlConf::ignite(TiXmlElement *cfg) 
{
	const char *ver_str, *code_str, *alone_str, *comm_str;
	char xmlVersion[10];
	char xmlEncode[100];
	char xmlStandalone[10];

	if (!cfg) return;

	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
	ver_str = cfg->Attribute("version");
	if ( ver_str )
		TEXTUS_SNPRINTF(xmlVersion, sizeof(xmlVersion), "%s", ver_str);
	else
		TEXTUS_STRCPY(xmlVersion, "1.0");

	code_str = cfg->Attribute("encode");
	if ( code_str )
		TEXTUS_SNPRINTF(xmlEncode, sizeof(xmlEncode), "%s", code_str);
	else
		TEXTUS_STRCPY(xmlEncode, "ISO-8859-1");

	alone_str = cfg->Attribute("alone");
	if ( code_str )
		TEXTUS_SNPRINTF(xmlStandalone, sizeof(xmlStandalone), "%s", alone_str);
	else
		memset(xmlStandalone, 0, sizeof(xmlStandalone));

	comm_str = cfg->Attribute("comment");
	if ( comm_str )
		comment.SetValue(comm_str);
	else
		comment.SetValue("TinyXml Generate");
	
	if ( dec ) delete dec;
	dec = new TiXmlDeclaration(xmlVersion, xmlEncode, xmlStandalone);
}

bool XmlConf::facio( Amor::Pius *pius)
{
	PacketObj **tmp;
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::IGNITE_ALL_READY:	
		WBUG("facio IGNITE_ALL_READY" );			
		gCFG->doc_cfg.SetTabSize( 8 );
		if ( !gCFG->doc_cfg.LoadFile (gCFG->cfg_file) || gCFG->doc_cfg.Error()) 
		{
			WLOG(ERR,"Loading %s file failed in row %d and column %d, %s", gCFG->cfg_file, gCFG->doc_cfg.ErrorRow(), gCFG->doc_cfg.ErrorCol(), gCFG->doc_cfg.ErrorDesc());
			break; ;
		} 
		break; ;
		
	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE(IGNITE)_ALL_READY" );			
		break;

	case Notitia::PRO_UNIPAC:
		WBUG("facio PRO_UNIPAC");
		if ( !rcv_pac)
		{
			WLOG(NOTICE,"facio PRO_UNIPAC null.");
		} else {
			wfile();
		}
		break;

	case Notitia::SET_UNIPAC:
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) rcv_pac = *tmp; 
			else
				WLOG(WARNING, "facio SET_UNIPAC rcv_pac null");
			tmp++;
			if ( *tmp) snd_pac = *tmp;
			else
				WLOG(WARNING, "facio SET_UNIPAC snd_pac null");
			WBUG("facio SET_UNIPAC rcv_pac(%p) snd_pac(%p)", rcv_pac, snd_pac);
		} else 
			WLOG(WARNING, "facio SET_UNIPAC null");
		
		break;

	default:
		return false;
	}
	return true;
}

void XmlConf::wfile()
{
	TiXmlPrinter printer;
	int len;

	req_doc.Clear();
	res_doc.Clear();
	res_doc.InsertEndChild(*dec);
	res_doc.InsertEndChild(comment);
	req_doc.Parse((const char*) rcv_pac->getfld(gCFG->xml_fld));

	if (req_doc.Error())
	{
		WLOG(ERR, "xml data parse error: %s", req_doc.ErrorDesc());
		goto ERR_PRO;
		return ;
	}

End:
	res_doc.Accept( &printer );
	len = (int) printer.Size();
	snd_pac->input(gCFG->xml_fld, (unsigned char*) printer.CStr(), len);
	res_doc.Clear();
	aptus->sponte(&pro_pac);
	return;
ERR_PRO:
	goto End;

}

bool XmlConf::sponte( Amor::Pius *pius) { return false; }

XmlConf::XmlConf()
{
	pro_pac.indic = 0;
	pro_pac.ordo = Notitia::PRO_UNIPAC;

	gCFG = 0;
	has_config = false ;
}

XmlConf::~XmlConf() 
{
	if ( has_config && gCFG)
		delete gCFG;
}

Amor* XmlConf::clone()
{
	XmlConf *child;
	child = new XmlConf();
	child->gCFG = gCFG;
	child->dec = dec->Clone()->ToDeclaration();
	child->comment.SetValue(comment.Value());
	return (Amor*)child;
}

#include "hook.c"
