/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title:处理CA操作请求
 Build: created by octerboy octerboy 2005/02/25
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Notitia.h"
#include "Amor.h"
#include "fastdb.h"
#include "MiniCA.h"
#include <time.h>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/txt_db.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/objects.h>
#include <openssl/pem.h>

static char *ca_types[] = {
		"countryName", 
		"stateOrProvinceName",
		"localityName", 
		"organizationName", 
		"organizationalUnitName", 
		"commonName",
		"emailAddress"
		};

static char *ca_types2[] = {
		"country",
		"state",
		"city",
		"orgnization", 
		"unit", 
		"caname", 
		"mail"
		 };

#define BZERO(X) memset(X, 0, sizeof(X));
class CaLet :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	CaLet();
	~CaLet();

	/* false:未处理请求; true:已处理请求 */
	bool handle();
	bool init(int, char *[]);
	bool down();
	bool up();

private:
	bool has_config;

	struct G_CFG {
		char db_file[128], sys_file[128];
		inline G_CFG() {
			BZERO(db_file)
			BZERO(sys_file)
		};

		inline ~G_CFG() {
		};
	};
	struct G_CFG *gCFG;	/* 全局共享参数 */

	TiXmlElement root;
};

void CaLet::ignite(TiXmlElement *cfg) 
{ 
	const char *db_str;
	if (!cfg) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
	}

	if ( (db_str = cfg->Attribute("dbname")))
	{
		int len = strlen(db_str);
		if ( len > sizeof(db_file) -2 ) 
			len = sizeof(db_file)-2;
		BZERO(gCFG->db_file)
		memcpy(gCFG->db_file, db_str, len);
	}
}

bool Calet::facio( Amor::Pius *pius)
{
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::PRO_HTTP_REQUEST:	/* 有HTTP请求 */
		WBUG("facio PRO_HTTP_REQUEST");
		handle(getHead("Path"));
		goto XML_BACK;

	case Notitia::PRO_SOAP_BODY:	/* 有SOAP请求 */
		WBUG("facio PRO_SOAP_BODY");
		if ( pius->indic )
		{
			if ( handle((TiXmlElement *)pius->indic) )
			{
				aptus->sponte(&fault_ps);
				break;
			}
		} else {
			handle((char*)0);
		}

XML_BACK:
		if ( pius->ordo == Notitia::PRO_SOAP_BODY )
			aptus->sponte(&local_pius);
		else {
			TiXmlPrinter printer;	
			root.Accept(&printer);
			setContentSize(printer.Size());
			output((char*)printer.CStr(), printer.Size());
			setHead("Content-Type", "text/xml");
			aptus->sponte(&direct_pius);
		}
		break;

	case Notitia::IGNITE_ALL_READY:	
	case Notitia::CLONE_ALL_READY:
		break;
		
	   default:
		return false;
	}
	return true;
}

bool Calet::sponte( Amor::Pius *pius) 
{
	switch ( pius->ordo )
	{
	case Notitia::ERR_SOAP_FAULT:
		WBUG("sponte ERR_SOAP_FAULT");
		setSoap(gCFG->bodyVal, pius, true);
		goto XML_BACK;

	case Notitia::PRO_SOAP_BODY:
		WBUG("sponte PRO_SOAP_BODY");
		setSoap(gCFG->bodyVal, pius);
XML_BACK:
		local_pius.ordo = Notitia::PRO_TINY_XML;
		local_pius.indic = 0;
		aptus->sponte(&local_pius);
		break;
	default:
		return false;
	}
	return true;
}

CaLet::CaLet():root("CA")
{
	head_p.ordo = Notitia::PRO_SOAP_HEAD;
	head_p.indic = 0;
	body_p.ordo = Notitia::PRO_SOAP_BODY;
	body_p.indic = 0;

	gCFG = 0;
	has_config = false;
}

CaLet::~CaLet()
{
	if ( has_config  )
	{	
		if(gCFG) delete gCFG;
	}
}

Amor* CaLet::clone()
{
	CaLet *child = new Calet();
	child->gCFG = gCFG;
	return  (Amor*) child;
}

bool CaLet::up()
{
	return this->opendb(db_file);
}

bool CaLet::down()
{
	this->closedb();
	return true;
}

bool CaLet::handle(char *path)
{
	root.Clear();
    if ( strcmp (path, "CAinfo.xml") == 0 )
    {
	char expr[10];
	int i;

	dbCursor<CA_subject> ca_subj;	
	dbCursor<CA_status> ca_status;
	dbQuery query;

	TiXmlElement *dist[7]; 
	TiXmlElement rsabit("rsabit");
	TiXmlElement days("days");
	TiXmlElement crldays("crldays");
	TiXmlElement sauto("sign_auto");
	TiXmlElement suto("sign_auto");
	TiXmlText text( "" );
	
	for ( i = 0 ; i < 7; i++ )
	{
		dist[i] = new TiXmlElement(ca_types2[i]);
	}

	for ( i = 0 ; i < 7; i++ )
	{
		query = "type= ", ca_types[i];
		if ( ca_subj.select(query) > 0) 
		{	
			text.SetValue(ca_subj->value);
			dist[i]->InsertEndChild(text);
			root.InsertEndChild(*(dist[i]));
		}
	}

	if ( ca_status.select() > 0 )
	{
		sprintf(expr,"%d", ca_status->days);
		text.SetValue(expr);
		days.InsertEndChild(text);
		root.InsertEndChild(days);

		sprintf(expr,"%d", ca_status->crl_days);
		text.SetValue(expr);
		crldays.InsertEndChild(text);
		root.InsertEndChild(crldays);

		sprintf(expr,"%d", ca_status->sign_auto);
		text.SetValue(expr);
		sauto.InsertEndChild(text);
		root.InsertEndChild(sauto);

		sprintf(expr,"%d", max_mod_num);
		printf("max_mod is %d\n", max_mod_num);
		text.SetValue(expr);
		suto.InsertEndChild(text);
		root.InsertEndChild(suto);
	}
	return true;

    } else if ( strcmp (path, "CACerts.xml") == 0 )
    {
	char expr[10];

	dbCursor<User_certs> usr_certs;
	dbQuery query;

	query = "status = ", MiniCA::UNBOUND, " or status = ", MiniCA::BOUND;
	if ( usr_certs.select(query) > 0 ) { do
	{
		TiXmlElement cert("cert");
		TiXmlElement id("id");
		TiXmlElement subj("subj");
		TiXmlText text( "" );

		BIO *in;
		X509 *x509 = NULL;
		char buf[256];

		memset(buf, 0 ,sizeof(buf));

		text.SetValue(usr_certs->serial_hex);
		id.InsertEndChild(text);
		cert.InsertEndChild(id);

		in = BIO_new(BIO_s_mem());
		BIO_write(in, usr_certs->cert_pem, strlen(usr_certs->cert_pem));
		x509=PEM_read_bio_X509(in,NULL,NULL,NULL);
		if ( x509 )
		{
			text.SetValue(X509_NAME_oneline( 
				X509_get_subject_name(x509),buf,sizeof(buf)-1));
			subj.InsertEndChild(text);
			cert.InsertEndChild(subj);

			root.InsertEndChild(cert);
		}

		BIO_free(in);
		if ( x509 ) X509_free(x509);
	} while (usr_certs.next());}

	return true;
     }
	return false;
}
bool CaLet::handle(TiXmlElement *body)
{
	char content[20];
	const char *path = body->Value();
	if (!dbh ) return false;	
    if (strcmp (path, "CAinit") == 0 )
    {
	bool succ = true;
	char sigdays[12],cmdstr[64] ;
	
	TiXmlElement result( "result" );
	TiXmlElement description( "description" );
	TiXmlText res( "");
	TiXmlText desc( "" );

	TiXmlElement *reqele = request->getXmlElement();

	int i;
	TiXmlElement *dist[7]; 
	bool all_have ;
	TiXmlElement *rsabit;
	TiXmlElement *days;

	dbCursor<CA_subject> update_subject  (dbCursorForUpdate) ;
	dbCursor<User_certs> update_usr  (dbCursorForUpdate) ;
	dbCursor<CA_status> update_ca_stat  (dbCursorForUpdate) ;
	
	dbQuery query;
	
	if (!reqele 
		|| strcmp (reqele->Value(), "CA") != 0
		|| reqele->Attribute("content") == (char*) NULL
		|| strcmp( reqele->Attribute("content"), "init") != 0
		|| reqele->Attribute("action") == (char*) NULL
		|| strcmp( reqele->Attribute("action"), "request") != 0
	)
	{
		desc.SetValue("no request xml data.");
		succ = false;
		goto CAinitEnd;
	}

	all_have = true;
	for ( i = 0 ; i < 7 ; i++)
	{
		dist[i] = reqele->FirstChildElement(ca_types2[i]);
		if ( ! (HaveValue(dist[i])) ) all_have = false;
	}

	rsabit = reqele->FirstChildElement("rsabit");
	days = reqele->FirstChildElement("days");

	if ( !(HaveValue(rsabit) && HaveValue(days) && all_have) 
		|| atoi(GetValue(days) ) <= 0  )
	{
		desc.SetValue("no request enough data or invalid data");
		succ = false;
		goto CAinitEnd;
	}

	for ( i =0 ; i < 7 ; i++ )
	{
		query = "type= ", ca_types[i];
		if ( update_subject.select(query) > 0) {	
			update_subject->value =  GetValue(dist[i]);
			update_subject.update();
		}
	}

	if ( update_ca_stat.select() > 0)
	{
		update_ca_stat->serial =  "01";
		update_ca_stat->serial_old =  "00";
		update_ca_stat->cert_pem = "";
		update_ca_stat->crl_pem = "";
		update_ca_stat->req_id = 1;
		update_ca_stat->req_id_old = 0;
		update_ca_stat.update();
	}

	if ( update_usr.select() > 0 )
	{
		
		update_usr.removeAllSelected();
		dbh->commit();
	} 
	
	dbh->commit();

	if ( !newCA( atoi(GetValue(rsabit)), atoi(GetValue(days)) ) )
	{
		desc.SetValue("Initializing CA failed");
		succ = false;
		goto CAinitEnd;
	}

CAinitEnd:
	if ( !succ )
	{
		res.SetValue("fail");
	} else
	{
		res.SetValue("success");
	}
	result.InsertEndChild(res);
	description.InsertEndChild(desc);
	root.InsertEndChild(result);
	root.InsertEndChild(description);
	return true;

    } else if ( strcmp (path, "CAcertRequestIE") == 0 )
    {

	bool succ = true;
	//char sigdays[12],cmdstr[64] ;
	
	TiXmlElement result ( "result" );
	TiXmlElement reqid ( "id" );
	TiXmlElement description( "description" );
	TiXmlText res( "");
	TiXmlText desc( "" );
	TiXmlText txt( "" );
	char expr[10];

	TiXmlElement *reqele = request->getXmlElement();
	TiXmlElement *content;

	char req_pem[8192];
	int req_len,req_idx;
	int req_id = 0;
	bool sign_auto;

	User_certs new_cert;
	char subject[1024];
	dbCursor<CA_status>   ca_status;
	dbCursor<CA_status> update_ca_status (dbCursorForUpdate) ;

	if (!reqele 
		|| strcmp (reqele->Value(), "CA") != 0
		|| reqele->Attribute("content") == (char*) NULL
		|| strcmp( reqele->Attribute("content"), "cert") != 0
		|| reqele->Attribute("action") == (char*) NULL
		|| strcmp( reqele->Attribute("action"), "request") != 0
	)
	{
		desc.SetValue("no request xml data.");
		succ = false;
		goto CAGetReqEnd;
	}

	content = reqele->FirstChildElement("content");

	if ( !( HaveValue(content)) )
	{
		desc.SetValue("no request data ");
		succ = false;
		goto CAGetReqEnd;
	}

	req_idx = 0; req_len = 0; memset(req_pem, 0, 8192);
	do {
		const char *con =  GetValue(content);
		int len =  strlen(con);
		req_len += len;
		if ( req_len > 8190 ) break;
		if ( req_idx > 0 )
		{
			req_pem[req_idx] = '\n';
			req_idx++;
		}

		memcpy(&req_pem[req_idx], con, len);
		req_idx += len;
		
		content = content->NextSiblingElement();
	} while ( HaveValue (content) );
	
	if ( !getSubject(req_pem, subject) )
	{
		desc.SetValue("Not valid x509 request of pem.");
		succ = false;
		goto CAGetReqEnd;
	}

	if ( ca_status.select() <= 0)
	{
		desc.SetValue("no CA parameters");
		succ = false;
		goto CAGetReqEnd;
	}	

	req_id = ca_status->req_id;
	sign_auto = ca_status->sign_auto;

	new_cert.serial_hex = "";
	new_cert.revoke_date = "";
	new_cert.status = REQUEST;
	new_cert.subject= subject;
	new_cert.cert_pem = "";
	new_cert.req_id = req_id;
	new_cert.req_pem = req_pem;
	insert(new_cert);

	/* 更新CA的参数 */
	if ( update_ca_status.select() > 0)
	{
		update_ca_status->req_id_old = req_id;
		update_ca_status->req_id = req_id+1;
		update_ca_status.update();
	}

	dbh->commit();
	if (sign_auto) 
	{
		if  (sign_req("today", NULL,  req_id) )
			desc.SetValue("install");
	}

CAGetReqEnd:
	if ( !succ )
	{
		res.SetValue("fail");
	} else
	{
		res.SetValue("success");
	}
	sprintf(expr, "%d", req_id);
	txt.SetValue(expr);
	reqid.InsertEndChild(txt);
	result.InsertEndChild(res);
	description.InsertEndChild(desc);
	root.InsertEndChild(result);
	root.InsertEndChild(reqid);
	root.InsertEndChild(description);
	return true;

    } else if ( strcmp (path, "CAgetCertIE") == 0 )
    {
	bool succ = true;
	
	TiXmlElement result ( "result" );
	TiXmlElement description( "description" );
	TiXmlText res( "");
	TiXmlText desc( "" );

	TiXmlElement *reqele = request->getXmlElement();
	TiXmlElement *reqid;
	
	int req_id = 0;
	char *p7 = (char*) 0;

	if (!reqele 
		|| strcmp (reqele->Value(), "CA") != 0
		|| reqele->Attribute("content") == (char*) NULL
		|| strcmp( reqele->Attribute("content"), "cert") != 0
		|| reqele->Attribute("action") == (char*) NULL
		|| strcmp( reqele->Attribute("action"), "get") != 0
	)
	{
		desc.SetValue("no request xml data.");
		succ = false;
		goto CAGetP7End;
	}

	reqid = reqele->FirstChildElement("reqid");

	if ( !( HaveValue(reqid)) )
	{
		desc.SetValue("no request data ");
		succ = false;
		goto CAGetP7End;
	}

	req_id = atoi(GetValue(reqid));
		
	p7 = getP7(req_id);


CAGetP7End:
	if ( !succ )
	{
		res.SetValue("fail");
	} else
	{
		res.SetValue("success");
	}
	result.InsertEndChild(res);
	description.InsertEndChild(desc);
	root.InsertEndChild(result);
	root.InsertEndChild(description);
	if ( p7 )
	{
		char *q = (char*) 0;
		char *p = p7;
		while ( p && *p)
		{
			TiXmlElement pkcs7( "p7" );
			TiXmlText text( "" );
			q = strpbrk ( p, "\r\n\t");
			if ( q) 
				*q++ = '\0';
			text.SetValue(p);
			pkcs7.InsertEndChild(text);
			root.InsertEndChild(pkcs7);
			if ( q) 
				q += strspn( q, " \r\n\t" );		
			p = q;
		}

		free(p7);
	}
	return true;

    } else if ( strcmp (path, "CAemCert") == 0 )
    {
	char *pem_cert;

	dbCursor<CA_status> ca_status;

	if ( ca_status.select() > 0 )
	{
		pem_cert = (char* ) ca_status->cert_pem;
	} else
	{
		response->output("");
		return true;
	}

	response->data_type = XmlResponse::HTTP_OTHER;
	if ( request->getPara("form") && strcmp(request->getPara("form"), "DER") == 0 )
	{
		char der[16384];
		int len;
		X509 *x;
		BIO *in = BIO_new(BIO_s_mem());
		BIO *out = BIO_new(BIO_s_mem());
		BIO_write(in, pem_cert, strlen(pem_cert));
		x=PEM_read_bio_X509(in,NULL,NULL,NULL);
		i2d_X509_bio(out,x);
		len = BIO_read(out, der, 16384);
		response->output(der, len);
		X509_free(x);
		BIO_free(in);
		BIO_free(out);	
	} else
	{
		response->output(pem_cert);
	}

	setContentType ("application/octet-stream");
	addHead("Content-disposition","attachment ; filename=emcert.cer");
	return true;
    } else if ( strcmp (path, "CAemCRL") == 0 )
    {
	char *pem_crl;

	dbCursor<CA_status> ca_status;
	gencrl();
	if ( ca_status.select() > 0 )
	{
		pem_crl = (char* ) ca_status->crl_pem;
	} else
	{
		response->output("");
		return true;
	}

	response->data_type = XmlResponse::HTTP_OTHER;
	if ( request->getPara("form") && strcmp(request->getPara("form"), "DER") == 0 )
	{
		char der[16384];
		int len;
		X509_CRL *x;
		BIO *in = BIO_new(BIO_s_mem());
		BIO *out = BIO_new(BIO_s_mem());
		BIO_write(in, pem_crl, strlen(pem_crl));
		x = PEM_read_bio_X509_CRL(in,NULL,NULL,NULL);
		i2d_X509_CRL_bio(out,x);
		len = BIO_read(out, der, 16384);
		response->output(der, len);
		X509_CRL_free(x);
		BIO_free(in);
		BIO_free(out);	
	} else
	{
		response->output(pem_crl);
	}

	response->setContentType ("application/octet-stream");
	response->addHead("Content-disposition","attachment ; filename=emcrl.crl");
	return true;
    } else if ( strcmp (path, "CArevokeCert") == 0 )
    {
	char expr[10];

	TiXmlElement result( "result" );
	TiXmlElement description( "description" );
	TiXmlText res( "");
	TiXmlText desc( "" );

	dbCursor<User_certs> upd_usr_cert  (dbCursorForUpdate) ;
	dbQuery query;
	char *serial;

	serial = request->getPara("certid");
	if ( !serial ) goto RevokeEnd;
	query = "serial_hex = ", serial, " and status != ", MiniCA::REVOKED;
	if ( upd_usr_cert.select(query) > 0 )
	{
		char tstr[128];
		time_t now;
		struct tm *mt;
		time(&now);
		mt = localtime(&now);
		strftime(tstr, 128, "%y%m%d%H%M%S",mt);

		upd_usr_cert->status = MiniCA::REVOKED;
		upd_usr_cert->revoke_date = tstr;
		upd_usr_cert.update();
		dbh->commit();
		
		res.SetValue("success");
	} else
	{
		desc.SetValue("no such certid");
		res.SetValue("fail");
	}

RevokeEnd:
	result.InsertEndChild(res);
	description.InsertEndChild(desc);
	root.InsertEndChild(result);
	root.InsertEndChild(description);
	return true;
    }

	return false;
}

#include "hook.c"
