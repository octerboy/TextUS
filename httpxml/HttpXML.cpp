/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: HTTP XML
 Build: created by octerboy, 2006/04/27, Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "TBuffer.h"
#include "Amor.h"
#include "Notitia.h"
#include "casecmp.h"
#include "textus_string.h"

#include <time.h>
#include <stdarg.h>

class HttpXML: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	HttpXML();
	~HttpXML();
	bool work_tbuf;

private:
	int data_type;
	TEXTUS_LONG content_length;
	const char* content_type;	

	TiXmlDocument req_doc;	
	TiXmlDocument res_doc;	

	TiXmlComment comment;
	TiXmlDeclaration *dec;
	
	Amor::Pius local_pius, local_pius2;
	
	TBuffer *rcv_buf;	/* http body content */
	TBuffer *snd_buf;

	void reset();
	size_t get_xml_content();
	void deliver(Notitia::HERE_ORDO aordo);
#include "httpsrv_obj.h"
#include "wlog.h"
};

#include <assert.h>
#include "textus_string.h"
#include "casecmp.h"

#define METHOD_OTHER    0
#define METHOD_GET      1
#define METHOD_POST     2

#define HTTP_NONE 0
#define HTTP_XML  1
#define HTTP_UNKNOWN  2

void HttpXML::ignite(TiXmlElement *cfg)
{
	const char *ver_str, *code_str, *alone_str, *comm_str;
	char xmlVersion[10];
	char xmlEncode[100];
	char xmlStandalone[10];

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

	comm_str = cfg->Attribute("mode");
	if ( comm_str && strcasecmp(comm_str, "tbuf") ==0 )
		work_tbuf = true;
}

bool HttpXML::facio( Amor::Pius *pius)
{
	TBuffer **tb = 0;
	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF:	
		WBUG("facio PRO_HTTP_REQUEST");
		reset();
		req_doc.Parse((const char*) rcv_buf->base);

		if (req_doc.Error())
		{
			WLOG(ERR, "xml data parse error: %s", req_doc.ErrorDesc());
		} else { 
			deliver(Notitia::PRO_TINY_XML); /* ????XML????????OK */
		}
		break;
	case Notitia::PRO_HTTP_REQUEST:	/* HTTP????OK */
		WBUG("facio PRO_HTTP_REQUEST");
		reset();
		content_length=getContentSize();
		content_type = getHead("Content-Type");	
		if(content_length > 0 && content_type 
			&& (strcasecmp( content_type, "text/xml")== 0 
			|| strncasecmp( content_type, "text/xml;", 9)== 0  
			|| strcasecmp( content_type, "application/soap+xml")== 0 
			|| strncasecmp( content_type, "application/soap+xml;", 22)== 0 ))
		{
			const char *coding;
			const char *xml;
			data_type = HTTP_XML;
			rcv_buf->grant(content_length+1);
			*(rcv_buf->point) = '\0';
			xml = (const char*) rcv_buf->base;

			coding = strpbrk(content_type, ";");
			if (coding )
			{	/* ?????????????? */
				coding++;
				coding += strspn( coding, " \t" );
				if (strcasecmp(coding, "charset=\"UTF-8\"") == 0 )
				{
					WBUG("PRO_HTTP_REQUEST xml is of UTF-8");
					req_doc.Parse(xml, 0, TIXML_ENCODING_UTF8);
				}
				else
					req_doc.Parse(xml);
			} else
				req_doc.Parse(xml);

			if (req_doc.Error())
			{
				WLOG(ERR, "xml data parse error: %s", req_doc.ErrorDesc());
				sendError(400); /* Bad request */
				aptus->sponte(&local_pius);
			} else { 
				deliver(Notitia::PRO_TINY_XML); /* ????XML????????OK */
			}
		} else {
			data_type = HTTP_UNKNOWN;
			deliver(Notitia::PRO_HTTP_REQUEST); /* HTTP????????????OK */
		}
		break;

	case Notitia::SET_TBUF:	/* ????????TBuffer???? */
		WBUG("facio SET_TBUF");
		if ( (tb = (TBuffer **)(pius->indic)))
		{	//tb????????NULL??*tb??rcv_buf
			if ( *tb) rcv_buf = *tb; 
			else
				WLOG(WARNING, "facio SET_TBUF rcv_buf null");
			tb++;
			if ( *tb) snd_buf = *tb;
			else
				WLOG(WARNING, "facio SET_TBUF snd_buf null");
			deliver(Notitia::SET_TINY_XML); /* ????XML DOC???? */
		} else 
			WLOG(WARNING, "facio SET_TBUF null");
		break;

	case Notitia::DMD_END_SESSION:	/* ???????? */
		WBUG("facio DMD_END_SESSION");
		reset();
		break;

	default:
		return false;
	}

	return true;
}

bool HttpXML::sponte( Amor::Pius *pius)
{
	assert(pius);
	
	switch ( pius->ordo )
	{
	case Notitia::PRO_TINY_XML:	/* XML???????? */
		WBUG("sponte PRO_TINY_XML");
		if ( work_tbuf) {
			get_xml_content();
			aptus->sponte(&local_pius2);	/* pro_tbuf */
		} else {
			setHead("Content-Type", "text/xml");
			setHead("Accept-Ranges", "bytes");
			setContentSize( get_xml_content());
			aptus->sponte(&local_pius);	/* HTTP???????? */
		}
		break;

	case Notitia::DMD_END_SESSION:	/* ???????? */
		reset();
		break;
		
	default:
		return false;
	}
	return true;
}

HttpXML::HttpXML()
{
	local_pius.ordo = Notitia::PRO_HTTP_RESPONSE;
	local_pius.indic = 0;
	local_pius.ordo = Notitia::PRO_TBUF;
	local_pius.indic = 0;
	comment.SetValue("");
	dec = 0;
	data_type = HTTP_NONE;
	content_type = (char*) 0;
	work_tbuf = false;
}

void HttpXML::reset()
{
	req_doc.Clear();
	res_doc.Clear();
	res_doc.InsertEndChild(*dec);
	res_doc.InsertEndChild(comment);
}

HttpXML::~HttpXML() {
	if ( dec ) delete dec;
}

Amor* HttpXML::clone()
{
	HttpXML *child = new HttpXML();
	child->dec = dec->Clone()->ToDeclaration();
	child->comment.SetValue(comment.Value());
	return (Amor*)child;
}

/* ???????????? */
void HttpXML::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	TiXmlDocument *doc[3];
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	
	switch (aordo)
	{
		case Notitia::SET_TINY_XML:
			WBUG("deliver SET_TINY_XML");
			doc[0] = &req_doc;
			doc[1] = &res_doc;
			doc[2] = 0;
			tmp_pius.indic = &doc[0];
			break;

		default:
			WBUG("deliver Notitia::%d",aordo);
			break;
	}
	aptus->facio(&tmp_pius);
	return ;
}

size_t HttpXML::get_xml_content()
{
	TiXmlPrinter printer;
	size_t len;
	res_doc.Accept( &printer );
	len = printer.Size();
	snd_buf->input((unsigned char*) printer.CStr(), len);
	res_doc.Clear();
	return len;
}

#include "hook.c"
