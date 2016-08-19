/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
Title: Wsdl parse
Build: created by octerboy, 2006/12/01, Guangzhou
 $Header: /textus/tsoap/Wsdl.cpp 13    08-01-02 0:00 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: Wsdl.cpp $"
#define TEXTUS_MODTIME  "$Date: 08-01-02 0:00 $"
#define TEXTUS_BUILDNO  "$Revision: 13 $"
#include "version_2.c"
/* $NoKeywords: $ */

#include "Wsdl.h"
#include "textus_string.h"
#include <string.h>
VName::VName(const char *nm) 
{
	int len;
	if (nm)
	{
		len = strlen(nm);
		name = new char[len+1];
		memcpy(name, nm, len);
		name[len] = 0;
	}
}

NString::NString (const char *n) 
{
	clear();
	if (n)
	{
		int len;
		if ( nm ) delete[] nm;
		len = strlen(n);
		nm = new char[len+1];
		memset(nm, 0, len+1);
		memcpy(nm, n, len);
	}
}

void NString::operator = (const char *val) 
{
	if (val && val != value)
	{
		int len;
		if ( value ) delete[] value;
		len = strlen(val);
		value = new char[len+1];
		memset(value, 0, len+1);
		memcpy(value, val, len);
	}
}
void NString::operator = (NString &n) 
{
	if ( value ) delete[] value;
	if (n.value)
	{
		int len;
		len = strlen(n.value);
		value = new char[len+1];
		memset(value, 0, len+1);
		memcpy(value, n.value, len);
	} else
		value = 0;
}

char* NString::operator + (const char *val) 
{
	int len, l;
	if ( !val ) 
		return value;

	if ( value ) 
		len = strlen(value);
	else 	
		len = 0;

	l = len;
	len += strlen(val);
	
	if ( result ) delete[] result;
	result = new char[len+1];
	memset(result, 0, len+1);
	if ( value)
		memcpy(result, value, l);
	memcpy(&result[l], val, len-l);

	return result ;
}

#define ANYSet(Z,Y,X) \
void Z::set##Y (const char *val) 	\
{					\
	if ( X) delete[] X;		\
	X = 0;				\
	if (val && X != val)		\
	{				\
		int len;		\
		len = strlen(val);	\
		X = new char[len+1];	\
		memset(X, 0, len+1);	\
		memcpy(X, val, len);	\
	}				\
}

#define PSet(Y,X) ANYSet(Part,Y,X)

PSet(Type,type)
PSet(Ns,ns)
PSet(Elem,elem)

void HDRInfo::parse ( Wsdl *dl, TiXmlElement *def )
{
	const char *sUs, *cstr, *cstr2;
	Message *amh;
	ns = def->Attribute("namespace");
	es = def->Attribute("encodingStyle");
	
	sUs = def->Attribute("use");
	fLiteral = (sUs && strcasecmp(sUs, "literal") == 0 );
	cstr = def->Attribute("required");
	fRequired = (cstr && strcasecmp(cstr, "true") == 0 );
	
	cstr = def->Attribute("message");
	if ( cstr)
	{
		cstr2 = def->Attribute("part");
		amh = &dl->msgs[Wsdl::getBaseName(cstr)];
		type = &amh->args[Wsdl::getBaseName(cstr2)];
	}
}

/* getBaseName: 比如对于"xs:Good", "Good" 就是BaseName, 本函数就返回 "Good", 如果就是"Good", 那就是"Good" */
const char *Wsdl::getBaseName( const char *str)
{
	const char *p = str;
	if (!p) return (char*) 0;
	while ( *p && *p != ':' ) p++;
	if ( *p == ':' ) 
		p++;
	else
		p = str;
	return p;
}

/* getQualifier: 比如对于"xs:Good", "xs" 就是Qualifier, 本函数就返回 "xs", 如果就是"Good", 那就返回"" */
char *Wsdl::getQualifier(const char *str)
{
	static char qua[128];
	const char *p = str;
	char *q = qua;
	int i;

	if (!p) return qua;

	i = 0;
	while ( *p && *p != ':' && i < 120 )  {
		*q = *p;	
		p++; q++; i++;
	}

	if ( *p == ':' ) 
		*q = '\0';
	else
		qua[0] = 0;

	return qua;
}

Wsdl::Wsdl ()
{
	asmx = 0;

	_st["negativeInteger"	]=0;
	_st["unsignedByte"	]=0;
	_st["unsignedLong"    	]=0;
	_st["unsignedInt"     	]=0;
	_st["decimal"         	]=0;
	_st["boolean"         	]=0;
	_st["integer"         	]=0;
	_st["double"          	]=0;
	_st["float"           	]=0;
	_st["short"           	]=0;
	_st["byte"            	]=0;
	_st["long"            	]=0;
	_st["int"             	]=0;
	_st["QName"           	]=1;
	_st["string"          	]=1;
	_st["normalizedString"	]=2;
	_st["timeInstant"     	]=3;
	_st["dateTime"        	]=3;
	_st["date"            	]=4;
	_st["time"            	]=5;
	_st["base64Binary"    	]=6;
	_st["base64"          	]=7;

	qlt["soapenc"] = "http://schemas.xmlsoap.org/soap/encoding/";
	qlt["wsdl"] = "http://schemas.xmlsoap.org/wsdl/";
	qlt["soap"] = "http://schemas.xmlsoap.org/wsdl/soap/";
	qlt["SOAP-ENV"] = "http://schemas.xmlsoap.org/soap/envelope/";
	
	ns["xsd"] = "http://www.w3.org/2001/XMLSchema";

	namespaceURI = 0;
	targetns = 0;
	defportname = 0;
}

Wsdl::~Wsdl ()
{
	if ( asmx ) delete[] asmx;
	asmx = 0;
}

bool Wsdl::parse ( TiXmlElement *def )
{
	const char *xsi99 = "http://www.w3.org/1999/XMLSchema-instance";
	const char *xsi01 = "http://www.w3.org/2001/XMLSchema-instance";
	//char *xsd01 = "http://www.w3.org/2001/XMLSchema";
	const char *xsd99 = "http://www.w3.org/1999/XMLSchema";

	const TiXmlAttribute *attr;
	NString nsq , nsqMsg  , nsqPort  , nsqBinding , nsqService  , nsqTypes  ;
	TiXmlElement *nMsgs , *nPort    , *nBinding , *nService , *nTypes ;

	char *tmpTag;
	TiXmlPrinter printer;


	if (!def ) return false;

	def->Accept(&printer);
	if ( asmx ) delete[] asmx;
	asm_len = printer.Size();
	asmx = new char [asm_len +1 ];
	TEXTUS_STRCPY( asmx, printer.CStr());

	nsq = getQualifier((char*)def->Value());
	if ( nsq == 0 || nsq == "" ) 
		nsq = "";
	else
		nsq = nsq + ":";

	nsqMsg = nsq;
	nsqPort = nsq;
	nsqBinding = nsq;
	nsqService = nsq;
	nsqTypes = nsq;

#define GETNODES(x,y,z) 			\
	x  = def->FirstChildElement(nsq + y);	\
	if (!x )				\
	{					\
		z = "";				\
	}
	
	GETNODES(nMsgs, "message", nsqMsg);
	GETNODES(nPort, "portType", nsqPort);
	GETNODES(nBinding, "binding", nsqBinding);
	GETNODES(nService, "service", nsqService);
	GETNODES(nTypes, "types", nsqTypes);
	
	targetns = def->Attribute("targetNamespace");
	namespaceURI = def->Attribute("xmlns");

	attr = def->FirstAttribute () ;
	for (; attr; attr = attr->Next() )
	{
		const char *nsn;
		const char *nm;
		if ( strcmp(attr->Name(), "xmlns" ) == 0 )
			continue;
			
		if ( strncmp(attr->Name(), "xmlns:", 6) != 0 )
			continue;

		nm = attr->Name();
		nsn = getBaseName( nm);
		if ( ns[nsn] != 0 && strcmp(nsn, "xsd") !=0 )
			continue;
		ns[nsn] = (char*) attr->Value();	
		nsalias[(attr->Value())] = nsn;
	}

	for ( 	NString *cur = qlt.get(); 
		cur;  
		cur = qlt.get() )
	{
		if (!cur->value ) continue;
		if ( nsalias [cur->value] != 0 )
		{
			(*cur) = nsalias[cur->value].value;
			continue;
		}

		ns[cur->nm] = *cur;
		nsalias[cur->value] = cur->nm;
		*cur = cur->nm;
	}

	if (ns["xsi"] == 0 ) 
		ns["xsi"] = ns["xsd"] == xsd99 ? xsi99 : xsi01;

	env_attr["xmlns"] = "";
	for ( 	NString *cur = ns.get(); 
		cur;  
		cur = ns.get() )
	{
		NString xmlns_name;
		xmlns_name = "xmlns:";
		xmlns_name = xmlns_name + cur->nm;
		env_attr[xmlns_name.value] = cur->value;
	}
/*
	for ( 	NString *cur = nsalias.get(); 
		cur;  
		cur = nsalias.get() )
	{
		printf("nsalias[%s] = %s\n", cur->nm, cur->value);
	}
	for ( 	NString *cur = qlt.get(); 
		cur;  
		cur = qlt.get() )
	{
		printf("qlt[%s] = %s\n", cur->nm, cur->value);
	}
*/
	tmpTag = nsqTypes + "types";
	nTypes  = def->FirstChildElement(tmpTag);
	while ( nTypes )
	{
		TiXmlElement *sch;
		sch = nTypes->FirstChildElement();
		while ( sch)
		{
			const char *uri = sch->Attribute("targetNamespace");	
			if ( uri )
				schemas[uri].parse(sch);
			
			sch = sch->NextSiblingElement();
		}
		nTypes  = nTypes->NextSiblingElement(tmpTag);
	}

	tmpTag = nsqMsg + "message";
	nMsgs  = def->FirstChildElement(tmpTag);
	while ( nMsgs)
	{
		TiXmlElement *ps;
		NString nsqPart;
		char *ptag;

		char *nm = (char*) nMsgs->Attribute("name");	
		if ( !nm) continue;

		nsqPart = nsqMsg;
		ptag = nsqPart + "part" ;

		Message &locMsg = msgs[nm];
		ps = nMsgs->FirstChildElement(ptag);
		while ( ps)
		{
			Part &ap =locMsg.args[(char*)ps->Attribute("name")];
			char *type= (char*)ps->Attribute("type");
			char *elem= (char*)ps->Attribute("element");
			ap.setType(type);
			ap.setElem(elem);
			
			if (ap.elem != 0)
			{
				ap.setNs(getQualifier(elem));
				ap.setElem(getBaseName(elem));
			}

			if (ap.type != 0)
			{
				ap.setNs( getQualifier(type));
				ap.setType (getBaseName(type));
			}

			ap.setNs(getUniqueNsq(ps, ap.ns));
			if (ap.type && strcmp(ap.type, "anyType") == 0 
				&& ns[ap.ns] ==  ns["xsd"] )
				ap.setType("string");

			ps = ps->NextSiblingElement(ptag);
		}

		nMsgs  = nMsgs->NextSiblingElement(tmpTag);
	}

	tmpTag = nsqPort + "portType";
	for ( 	nPort = def->FirstChildElement(tmpTag);
		nPort;		/* for each "portType" */
		nPort = nPort->NextSiblingElement(tmpTag))
	{
		TiXmlElement *nops;
		NString nsqOpr;
		char *ptag;

		char *sName = (char*) nPort->Attribute("name");	
		if ( !sName) continue;

		nsqOpr = nsqPort;
		ptag = nsqOpr + "operation" ;
		for (	nops = nPort->FirstChildElement(ptag);
			nops;	/* for each "operation" */
			nops = nops->NextSiblingElement(ptag))
		{
			TiXmlElement *nInputs, *nOutputs;
			NString nsqPut;
			char *qtag;
			Message *mInput=0, *mOutput = 0;

			char *sOpName = (char*) nops->Attribute("name");	
			if ( !sOpName) continue;

			nsqPut = nsqOpr;
			qtag = nsqPut + "input" ;
			nInputs = nops->FirstChildElement(qtag);
			if ( nInputs )		/* for each "input" */
			{
				Operation &oop = (ports[sName]).ops[sOpName];
				char *s =  (char*) nInputs->Attribute("message");
				const char *sMsgName, *sin;
				char *sNs;
				Part *firstArg = (Part *)0;

				sMsgName = getBaseName(s);
				sNs = getQualifier(s);
				mInput = &msgs[sMsgName];
				oop.msgp = mInput;
				
				mInput->opname = (const char*)sOpName;	

				if (  mInput->args.length > 0 )
					firstArg = &(mInput->args[0]);

				sin = nInputs->Attribute("name");

				if (  sin )
					oop.siname = sin;
				else
					oop.siname = (const char*) sOpName;

				mInput->fWrapped = mInput->args.length == 1 && firstArg && (
					   (firstArg->type && strcmp(firstArg->type, oop.name ) == 0 )
					|| (firstArg->elem && strcmp(firstArg->elem, oop.name) == 0 )
					|| (firstArg->name && strcasecmp(firstArg->name, "parameters" ) == 0 ));
			}

			qtag = nsqPut + "output" ;
			nOutputs = nops->FirstChildElement(qtag);
			if ( nOutputs )		/* for first "output" */
			{
				char *s =  (char*) nOutputs->Attribute("message");
				const char *sMsgName;

				sMsgName = getBaseName(s);
				mOutput = &msgs[sMsgName];
				ports[sName].ops[sMsgName].msgp = mOutput;
				mInput->response = mOutput;
			}
			mInput->fOneWay = (nOutputs == 0);
		}
	}

	tmpTag = nsqBinding + "binding";
	for (	nBinding = def->FirstChildElement(tmpTag);
		nBinding;
		nBinding = nBinding->NextSiblingElement(tmpTag))
	{
		TiXmlElement *osoapb, *nops;
		const char *sStyle, *sName;
		const char *stype;
		NString nsqOpr;
		char *ptag;
		
		osoapb = nBinding->FirstChildElement("soap:binding");
		if ( !osoapb ) continue;

		sName = nBinding->Attribute("name");
		if ( !sName) continue;

		stype = getBaseName((char*)nBinding->Attribute("type"));
		sStyle = osoapb->Attribute("style");

		abinding[(char*)sName].ops = &ports[stype].ops;
		nsqOpr = nsqBinding;
		ptag = nsqOpr + "operation" ;
		nops = nBinding->FirstChildElement(ptag);
		for (; nops ; nops = nops->NextSiblingElement(ptag))	/* for each "operation" */
		{
			TiXmlElement *input, *output, *nsoapops, *nsoapbody;
			TiXmlElement *nheadIn, *nheadOut;
			int count;
			NString nsqPut;
			char *qtag;
			Message *oM = 0;
			Operation *oop;

			char *sin;
			char *sOpStyle;

			char *sOpName = (char*) nops->Attribute("name");	
			if ( !sOpName) continue;

			nsqPut = nsqBinding;
			qtag = nsqPut + "input" ;
			input = nops->FirstChildElement(qtag);
			if ( !input )		/* for "input" */
				continue;

			sin =  (char*) input->Attribute("name");
			if (!sin )
				sin = sOpName;

			oop = &ports[stype].ops[sOpName];
			oM = oop->msgp;
			if ( !oop->siname || !oM || strcmp(oop->siname, sin ) != 0 ) 
				continue;
			
			nsoapops = nops->FirstChildElement("soap:operation");
			if ( !nsoapops ) continue;

			oM->soapAction = nsoapops->Attribute("soapAction");
			if ( oM->soapAction )
			{
				int len = strlen(oM->soapAction);
				oM->soapAction2 = new char[len + 3];
				oM->soapAction2[0] = '"';
				memcpy(&oM->soapAction2[1], oM->soapAction, len);
				oM->soapAction2[len+1] = '"';
				oM->soapAction2[len+2] = '\0';
			}
			nsoapbody =  input->FirstChildElement("soap:body");
			if( nsoapbody )
			{
				const char *sUs;
				oM->ns = nsoapbody->Attribute("namespace");
				oM->es = nsoapbody->Attribute("encodingStyle");
				sUs = nsoapbody->Attribute("use");
				oM->fLiteral = (sUs && strcasecmp(sUs, "literal") == 0);
			}

			nheadIn =  input->FirstChildElement("soap:header");
			count = 0;
			while ( nheadIn )
			{
				oM->hdrsIn[count].parse(this, nheadIn);
				nheadIn =  nheadIn->NextSiblingElement("soap:header");
				count++;
			}
		
			qtag = nsqPut + "output" ;
			output = nops->FirstChildElement(qtag);
			nheadOut =  output->FirstChildElement("soap:header");
			count = 0;
			while ( nheadOut )
			{
				oM->hdrsOut[count].parse(this, nheadOut);
				nheadOut =  nheadOut->NextSiblingElement("soap:header");
				count++;
			}

			sOpStyle= (char*) nsoapops->Attribute("style");
			if ( sOpStyle )
				oM->fRpc = (strcasecmp(sOpStyle, "rpc") == 0 );
			else
				oM->fRpc = ( sStyle && (strcasecmp(sStyle, "rpc") == 0 ) );
		}
	}

	tmpTag = nsqService + "service";
	for (	nService  = def->FirstChildElement(tmpTag);
		nService;
		nService= nService->NextSiblingElement(tmpTag))
	{
		TiXmlElement *nports;
		NString nsqPrt;
		char *ptag;
		
		nsqPrt = nsqService;
		ptag = nsqPrt + "port" ;
		for (	nports = nService->FirstChildElement(ptag);
			nports; /* for each "port" */
			nports = nports->NextSiblingElement(ptag))
		{
			TiXmlElement *oAddress, *oSOAPHdr;
			int j;
			Binding *b;
			char *szname;

			szname = (char*)nports->Attribute("name");
			if ( !szname ) continue;

			oAddress = nports->FirstChildElement("soap:address");
			if ( !oAddress ) continue;

			oSOAPHdr = nports->FirstChildElement("soap:header");
			j = 0;
			while ( oSOAPHdr )		/* for each "header" */
			{
				headers[j].parse(this, oSOAPHdr);
				oSOAPHdr = oSOAPHdr->NextSiblingElement("soap:header");
				j++;
			}

			b = &abinding[getBaseName((char*)nports->Attribute("binding"))];
			if (b->ops == 0 ) continue;
			
			soapPorts[szname].ops = b->ops;
			soapPorts[szname].location = oAddress->Attribute("location");
		}
	}
	return true;
}

char* Wsdl::getUniqueNsq(TiXmlElement *o, char *litNsq)
{
	static int nsqID = 0;
	const char *nsuri = 0;
	char* nsq1 ;
	static char nextNsq[64];
	
	if (litNsq == 0)
		return litNsq;

	if (litNsq[0] == '\0')
		nsuri = this->namespaceURI;
	else {
		TiXmlElement * o1 = o;
		NString np;
		np = "xmlns:";
		np = np+litNsq;
		
		while (o1 != 0) 
		{
			nsuri = o1->Attribute(np.value);
			if (nsuri != 0)
				break;
			o1 = o1->Parent()->ToElement();
		}
		//printf("nsuri ----------- %s\n", nsuri);
	}

	if (nsuri == 0)
		return litNsq;

	nsq1 = nsalias[(char*)nsuri].value;
	if (nsq1 != 0)
		return nsq1;

	//litNsq = getNextNsq(oS);
	TEXTUS_SNPRINTF(nextNsq, sizeof(nextNsq), "tus%d", nsqID); nsqID++;
	ns[nextNsq] = (char*) nsuri;
	nsalias[(char*) nsuri] = nextNsq;

	return nextNsq;
}
