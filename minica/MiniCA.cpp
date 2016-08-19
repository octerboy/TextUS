/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
Title: CA²Ù×÷
Build: created by octerboy, 2005/02/25
 $Header: /textus/tsoap/Wsdl.cpp 11    06-12-29 11:28 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: Wsdl.cpp $"
#define TEXTUS_MODTIME  "$Date: 06-12-29 11:28 $"
#define TEXTUS_BUILDNO  "$Revision: 11 $"
#include "version_2.c"
/* $NoKeywords: $ */

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/objects.h>
#include <openssl/pem.h>
#include <openssl/ocsp.h>
#include <openssl/rand.h>
#include "MiniCA.h"
#include "log.h"
#define APPID 103

REGISTER(X509v3_extensions_conf);
REGISTER(CA_policy_conf);
REGISTER(CA_subject);
REGISTER(CA_status);
REGISTER(User_certs);

#define FOR_CA 0
#define FOR_USER 1
#define FOR_CRL 2
#define TYPE_RSA        1
#define TYPE_DSA        2
#define TYPE_DH         3

static int seeded = 0;
static int egdsocket = 0;

#define NUM_REASONS (sizeof(crl_reasons) / sizeof(char *))
static int make_revoked(X509_REVOKED *rev, char *str);
static int unpack_revinfo(ASN1_TIME **prevtm, int *preason, ASN1_OBJECT **phold, ASN1_GENERALIZEDTIME **pinvtm, char *str);

static char *crl_reasons[] = {
	/* CRL reason strings */
	"unspecified",
	"keyCompromise",
	"CACompromise",
	"affiliationChanged",
	"superseded", 
	"cessationOfOperation",
	"certificateHold",
	"removeFromCRL",
	/* Additional pseudo reasons */
	"holdInstruction",
	"keyTime",
	"CAkeyTime"
};
static void app_RAND_allow_write_file(void)
{
	seeded = 1;
}

static int app_RAND_load_file(const char *file, int dont_warn)
{
	SecLog *log = SecLog::instance();
	int consider_randfile = (file == NULL);
	char buffer[200];
	
#ifdef WINDOWS
	BIO_printf(bio_e,"Loading 'screen' into random state -");
	BIO_flush(bio_e);
	RAND_screen();
	BIO_printf(bio_e," done\n");
#endif

	if (file == NULL)
		file = RAND_file_name(buffer, sizeof buffer);
	else if (RAND_egd(file) > 0)
	{
		/* we try if the given filename is an EGD socket.
		   if it is, we don't write anything back to the file. */
		egdsocket = 1;
		return 1;
	}
	if (file == NULL || !RAND_load_file(file, -1))
	{
		if (RAND_status() == 0 && !dont_warn)
		{
			log->w(SecLog::WARNING, APPID, "unable to load 'random state' This means that the random number generator has not been seeded with much random data.");
			if (consider_randfile) /* explanation does not apply when a file is explicitly named */
			{
				log->w(SecLog::WARNING, APPID, "Consider setting the RANDFILE environment variable to point at a file that 'random' data can be kept in (the file will be overwritten).");
			}
		}
		return 0;
	}
	seeded = 1;
	return 1;
}

#if (defined(WINDOWS) || defined(MSDOS)) && !defined(__CYGWIN32__)
# define LIST_SEPARATOR_CHAR ';'
#else
# define LIST_SEPARATOR_CHAR ':'
#endif

static long app_RAND_load_files(char *name)
{
	char *p,*n;
	int last;
	long tot=0;
	int egd;
	
	for (;;)
	{
		last=0;
		for (p=name; ((*p != '\0') && (*p != LIST_SEPARATOR_CHAR)); p++);
		if (*p == '\0') last=1;
		*p='\0';
		n=name;
		name=p+1;
		if (*n == '\0') break;

		egd=RAND_egd(n);
		if (egd > 0)
			tot+=egd;
		else
			tot+=RAND_load_file(n,-1);
		if (last) break;
	}
	if (tot > 512)
		app_RAND_allow_write_file();
	return(tot);
}

static int app_RAND_write_file(const char *file)
{
	SecLog *log = SecLog::instance();
	char buffer[200];
	
	if (egdsocket || !seeded)
		/* If we did not manage to read the seed file,
		 * we should not write a low-entropy seed file back --
		 * it would suppress a crucial warning the next time
		 * we want to use it. */
		return 0;

	if (file == NULL)
		file = RAND_file_name(buffer, sizeof buffer);
	if (file == NULL || !RAND_write_file(file))
	{
		log->w(SecLog::WARNING, APPID, "unable to write 'random state'");
		return 0;
	}
	return 1;
}

int make_revoked(X509_REVOKED *rev, char *str)
	{
	char *tmp = NULL;
	int reason_code = -1;
	int i, ret = 0;
	ASN1_OBJECT *hold = NULL;
	ASN1_GENERALIZEDTIME *comp_time = NULL;
	ASN1_ENUMERATED *rtmp = NULL;

	ASN1_TIME *revDate = NULL;

	i = unpack_revinfo(&revDate, &reason_code, &hold, &comp_time, str);

	if (i == 0)
		goto err;

	if (rev && !X509_REVOKED_set_revocationDate(rev, revDate))
		goto err;

	if (rev && (reason_code != OCSP_REVOKED_STATUS_NOSTATUS))
		{
		rtmp = ASN1_ENUMERATED_new();
		if (!rtmp || !ASN1_ENUMERATED_set(rtmp, reason_code))
			goto err;
		if (!X509_REVOKED_add1_ext_i2d(rev, NID_crl_reason, rtmp, 0, 0))
			goto err;
		}

	if (rev && comp_time)
		{
		if (!X509_REVOKED_add1_ext_i2d(rev, NID_invalidity_date, comp_time, 0, 0))
			goto err;
		}
	if (rev && hold)
		{
		if (!X509_REVOKED_add1_ext_i2d(rev, NID_hold_instruction_code, hold, 0, 0))
			goto err;
		}

	if (reason_code != OCSP_REVOKED_STATUS_NOSTATUS)
		ret = 2;
	else ret = 1;

	err:

	if (tmp) OPENSSL_free(tmp);
	ASN1_OBJECT_free(hold);
	ASN1_GENERALIZEDTIME_free(comp_time);
	ASN1_ENUMERATED_free(rtmp);
	ASN1_TIME_free(revDate);

	return ret;
	}

int unpack_revinfo(ASN1_TIME **prevtm, int *preason, ASN1_OBJECT **phold, ASN1_GENERALIZEDTIME **pinvtm, char *str)
	{
	SecLog *log = SecLog::instance();
	char *tmp = NULL;
	char *rtime_str, *reason_str = NULL, *arg_str = NULL, *p;
	int reason_code = -1;
	int i, ret = 0;
	ASN1_OBJECT *hold = NULL;
	ASN1_GENERALIZEDTIME *comp_time = NULL;
	tmp = BUF_strdup(str);

	p = strchr(tmp, ',');

	rtime_str = tmp;

	if (p)
		{
		*p = '\0';
		p++;
		reason_str = p;
		p = strchr(p, ',');
		if (p)
			{
			*p = '\0';
			arg_str = p + 1;
			}
		}

	if (prevtm)
		{
		*prevtm = ASN1_UTCTIME_new();
		if (!ASN1_UTCTIME_set_string(*prevtm, rtime_str))
			{
			log->w(SecLog::WARNING, APPID, "invalid revocation date %s\n", rtime_str);
			goto err;
			}
		}
	if (reason_str)
		{
		for (i = 0; i < NUM_REASONS; i++)
			{
			if(!strcasecmp(reason_str, crl_reasons[i]))
				{
				reason_code = i;
				break;
				}
			}
		if (reason_code == OCSP_REVOKED_STATUS_NOSTATUS)
			{
			log->w(SecLog::WARNING, APPID, "invalid reason code %s\n", reason_str);
			goto err;
			}

		if (reason_code == 7)
			reason_code = OCSP_REVOKED_STATUS_REMOVEFROMCRL;
		else if (reason_code == 8)		/* Hold instruction */
			{
			if (!arg_str)
				{ 	
				log->w(SecLog::WARNING, APPID, "missing hold instruction\n");
				goto err;
				}
			reason_code = OCSP_REVOKED_STATUS_CERTIFICATEHOLD;
			hold = OBJ_txt2obj(arg_str, 0);

			if (!hold)
				{
				log->w(SecLog::WARNING, APPID, "invalid object identifier %s\n", arg_str);
				goto err;
				}
			if (phold) *phold = hold;
			}
		else if ((reason_code == 9) || (reason_code == 10))
			{
			if (!arg_str)
				{	
				log->w(SecLog::WARNING, APPID, "missing compromised time\n");
				goto err;
				}
			comp_time = ASN1_GENERALIZEDTIME_new();
			if (!ASN1_GENERALIZEDTIME_set_string(comp_time, arg_str))
				{ 	
				log->w(SecLog::WARNING, APPID, "invalid compromised time %s\n", arg_str);
				goto err;
				}
			if (reason_code == 9)
				reason_code = OCSP_REVOKED_STATUS_KEYCOMPROMISE;
			else
				reason_code = OCSP_REVOKED_STATUS_CACOMPROMISE;
			}
		}

	if (preason) *preason = reason_code;
	if (pinvtm) *pinvtm = comp_time;
	else ASN1_GENERALIZEDTIME_free(comp_time);

	ret = 1;

	err:

	if (tmp) OPENSSL_free(tmp);
	if (!phold) ASN1_OBJECT_free(hold);
	if (!pinvtm) ASN1_GENERALIZEDTIME_free(comp_time);

	return ret;
	}

MiniCA::MiniCA()
{
	dbh = (dbDatabase *) 0;
	memset(errMsg, 0 ,sizeof(errMsg));
}

MiniCA::~MiniCA()
{
	if(dbh && dbh->isOpen() ) 
	{
		dbh->close();
		delete (dbDatabase *) dbh;
	}
}

void MiniCA::closedb()
{
	if(dbh && dbh->isOpen() ) 
	{
		dbh->close();
		delete (dbDatabase *) dbh;
		dbh = (dbDatabase *) 0;
	}
	return;
}

bool MiniCA::opendb(char *filename)
{
	SecLog *log = SecLog::instance();
	if ( !filename) return false;
	dbh = new dbDatabase();
	if ( !dbh->open(filename))
	{
		log->w(SecLog::EMERG, APPID, "Opening database failed from %s", filename);
		delete (dbDatabase *)dbh;
		dbh = (dbDatabase *) 0;
		return false;
	}

#ifndef NDEBUG
	log->w(SecLog::DEBUG, APPID, "Opened database from %s", filename);
#endif
	OpenSSL_add_all_digests();
	return true;
}

/* char *value:  Value    */
X509_EXTENSION * MiniCA::do_v3_ext(X509V3_CTX *ctx, int ext_nid, int crit, char *value)
{
	SecLog *log = SecLog::instance();
	X509V3_EXT_METHOD *method;
	X509_EXTENSION *ext;
	STACK_OF(CONF_VALUE) *nval;
	void *ext_struc;

	if(ext_nid == NID_undef) {
		return NULL;
	}
	if(!(method = X509V3_EXT_get_nid(ext_nid))) {
		log->w(SecLog::ERR, APPID, "Unkown extension id %d", ext_nid);
		return NULL;
	}
	/* Now get internal extension representation based on type */
	if(method->v2i) {
		nval = X509V3_parse_list(value);
		if(!nval) {
			log->w(SecLog::ERR, APPID,"Invalid extension string name=%s, section=%s", OBJ_nid2sn(ext_nid), value);
			return NULL;
		}
		ext_struc = method->v2i(method, ctx, nval);
		sk_CONF_VALUE_pop_free(nval, X509V3_conf_free);
		if(!ext_struc) 
		{
			log->w(SecLog::ERR, APPID, "Invalid extension value=%s (from method->v2i)",value);
			return NULL;
		}
	} else if(method->s2i) {
		if(!(ext_struc = method->s2i(method, ctx, (char*)value)))
		{
			log->w(SecLog::ERR, APPID, "Invalid extension value=%s (from method->s2i)",value);
			return NULL;
		}
	} else if(method->r2i) {
		if(!ctx->db) {
			log->w(SecLog::ERR, APPID, "No config database.");
			return NULL;
		}
		if(!(ext_struc = method->r2i(method, ctx, (char*)value))) 
		{
			log->w(SecLog::ERR, APPID,"Invalid extension value=%s (from method->r2i)",value);
			return NULL;
		}
	} else {
		log->w(SecLog::ERR, APPID, "Not supported extension setting name=%s",OBJ_nid2sn(ext_nid));
		return NULL;
	}

	ext  = this->do_v3_ext_i2d(method, ext_nid, crit, ext_struc);
	if(method->it) ASN1_item_free((ASN1_VALUE *)ext_struc, ASN1_ITEM_ptr(method->it));
	else method->ext_free(ext_struc);
	return ext;
}

X509_EXTENSION * MiniCA::do_v3_ext_i2d(X509V3_EXT_METHOD *method, int ext_nid,
						 int crit, void *ext_struc)
{
	SecLog *log = SecLog::instance();

	unsigned char *ext_der;
	int ext_len;
	ASN1_OCTET_STRING *ext_oct;
	X509_EXTENSION *ext;
	/* Convert internal representation to DER */
	if (method->it)
		{
		ext_der = NULL;
		ext_len = ASN1_item_i2d((ASN1_VALUE *)ext_struc, &ext_der, ASN1_ITEM_ptr(method->it));
		if (ext_len < 0) goto merr;
		}
	 else
		{
		unsigned char *p;
		ext_len = method->i2d(ext_struc, NULL);
		if(!(ext_der = (unsigned char*)OPENSSL_malloc(ext_len))) goto merr;
		p = ext_der;
		method->i2d(ext_struc, &p);
		}
	if (!(ext_oct = M_ASN1_OCTET_STRING_new())) goto merr;
	ext_oct->data = ext_der;
	ext_oct->length = ext_len;

	ext = X509_EXTENSION_create_by_NID(NULL, ext_nid, crit, ext_oct);
	if (!ext) goto merr;
	M_ASN1_OCTET_STRING_free(ext_oct);

	return ext;

merr:
	log->w(SecLog::ALERT, APPID,"X509V3_F_DO_EXT_I2D,ERR_R_MALLOC_FAILURE");
	return NULL;
}

/* Check the extension string for critical flag */
int MiniCA::v3_check_critical(char **value)
{
	char *p = *value;
	if((strlen(p) < 9) || strncmp(p, "critical,", 9)) return 0;
	p+=9;
	while(isspace((unsigned char)*p)) p++;
	*value = p;
	return 1;
}

/* Check extension string for generic extension and return the type */
int MiniCA::v3_check_generic(char **value)
{
	char *p = *value;
	if((strlen(p) < 4) || strncmp(p, "DER:,", 4)) return 0;
	p+=4;
	while(isspace((unsigned char)*p)) p++;
	*value = p;
	return 1;
}

/* Create a generic extension: for now just handle DER type */
X509_EXTENSION * MiniCA::v3_generic_extension(const char *ext, char *value,
	     int crit, int type)
{
	SecLog *log = SecLog::instance();
	unsigned char *ext_der=NULL;
	long ext_len;
	ASN1_OBJECT *obj=NULL;
	ASN1_OCTET_STRING *oct=NULL;
	X509_EXTENSION *extension=NULL;
	if(!(obj = OBJ_txt2obj(ext, 0)))
	{
		log->w(SecLog::ERR, APPID, "Invalid extension name=%s", ext);
		goto err;
	}

	if(!(ext_der = string_to_hex(value, &ext_len)))
	{
		log->w(SecLog::ERR, APPID, "Invalid Extension value=%s", value);
		goto err;
	}

	if(!(oct = M_ASN1_OCTET_STRING_new())) 
	{
		log->w(SecLog::ERR, APPID, "M_ASN1_OCTET_STRING_new failed.");
		goto err;
	}

	oct->data = ext_der;
	oct->length = ext_len;
	ext_der = NULL;

	extension = X509_EXTENSION_create_by_OBJ(NULL, obj, crit, oct);

err:
	ASN1_OBJECT_free(obj);
	M_ASN1_OCTET_STRING_free(oct);
	if(ext_der) OPENSSL_free(ext_der);
	return extension;
}

/* char *name:  Name    */
/* char *value:  Value    */
X509_EXTENSION * MiniCA::x509v3_ext(X509V3_CTX *ctx, char *name, char *value,
	STACK_OF(X509_EXTENSION) *req_ext)
{
	SecLog *log = SecLog::instance();
	int crit;
	int ext_type;
	int ext_nid;
	X509_EXTENSION *ret = NULL;

	int i;
	X509_EXTENSION *ex, *found_ex = NULL;

	crit = this->v3_check_critical(&value);
	if((ext_type = this->v3_check_generic(&value))) 
		return this->v3_generic_extension(name, value, crit, ext_type);

	if( (ext_nid = OBJ_sn2nid(name)) == NID_undef) {
		log->w(SecLog::ERR, APPID, "Unkown extension name=%s", name);
		return NULL;
	}
	
	if ( req_ext) for(i = 0; i < sk_X509_EXTENSION_num(req_ext); i++)
	{
		ex = sk_X509_EXTENSION_value(req_ext, i);
		if(OBJ_obj2nid(ex->object) == ext_nid) 
		{
			ret = ex;
			break;
		}
	}

	if ( !ret ) ret = this->do_v3_ext(ctx, ext_nid, crit, value);
	if(!ret) {
		log->w(SecLog::ERR, APPID, "Error in extension name=%s, value=%s", name, value);
	}
	return ret;
}

bool MiniCA::x509v3_ext_add_conf(X509V3_CTX *ctx, X509 *cert, int type, 	
	STACK_OF(X509_EXTENSION) *req_ext )
{
	X509_EXTENSION *ext;
	bool ret = true;

	dbCursor<X509v3_extensions_conf> val;
	dbQuery query;

	query = "type =",type;

	if (val.select(query) > 0 ) { do 
	{
		ext = this->x509v3_ext (ctx, (char*)(val->name), (char*)(val->value), req_ext);
		if(!ext)
		{
			ret = false;
			break;
		}
		if(cert)X509v3_add_ext(&(cert->cert_info->extensions), ext, -1);
		X509_EXTENSION_free(ext);

	} while (val.next()); } else 
	{
		ret = false;
	}

	return ret;
}

/* Same as above but for a CRL */
bool MiniCA::x509v3_ext_add_conf(X509V3_CTX *ctx, X509_CRL *crl)
{
	X509_EXTENSION *ext;
	bool ret = true;

	dbCursor<X509v3_extensions_conf> val;
	dbQuery query;

	query = "type =",FOR_CRL;

	if (val.select(query) > 0 ) { do 
	{
		ext = this->x509v3_ext (ctx, (char*)(val->name), (char*)(val->value), NULL);
		if(!ext)
		{
			ret = false;
			break;
		}
		if(crl) X509v3_add_ext(&(crl->crl->extensions),ext,-1);
		X509_EXTENSION_free(ext);

	} while (val.next()); } else 
	{
		ret = false;
	}
	return ret;
}

bool MiniCA:: getSubject(const char *req_pem,  char subject[])
{
	X509_NAME *subj = NULL;
	X509_REQ *req = NULL;
	BIO *in;

	in = BIO_new(BIO_s_mem());
	BIO_write(in, req_pem, strlen(req_pem));
	req = PEM_read_bio_X509_REQ(in, NULL, NULL, NULL);
	BIO_free(in);

	if ( !req ) 
		return false;

	subj = X509_REQ_get_subject_name(req);
	if ( !X509_NAME_oneline(subj, subject, 1023) ) 
		return false;

	return true;
}

bool MiniCA:: sign_req(char *startdate, char *enddate,  int req_id)
{
	SecLog *log = SecLog::instance();
	X509_NAME *name=NULL,*ca_name=NULL,*subject=NULL;
	ASN1_STRING *str,*str2;
	ASN1_OBJECT *obj;
	X509 *new_cert = NULL;
	X509_CINF *ci;
	X509_NAME_ENTRY *ne;
	X509_NAME_ENTRY *tne,*push;
	EVP_PKEY *pktmp;
	int ok= -1,i,j,last;

	X509_REQ *req = NULL;
	BIGNUM *serial = NULL;
	X509 *x509 = NULL;
	EVP_PKEY *pkey = NULL;
	const EVP_MD *dgst = NULL;
	int preserve = 0;
	int days = 2;

	if ( !dbh ) return false;
	dbCursor<CA_policy_conf> val_policy;
	dbCursor<CA_status> val_status;
	dbCursor<User_certs> val_cert;
	dbQuery query;
	query = "req_id =",req_id, " and status =", REQUEST;

	dbCursor<User_certs> update_cert  (dbCursorForUpdate) ;
	dbCursor<CA_status> update_ca_status (dbCursorForUpdate) ;

	/* È¡µÃCAµÄ²ÎÊý */
	if ( val_status.select() > 0)
	{
		BIO *in,*in2;
		serial = BN_new();  /* ÐÂÖ¤ÊéµÄÐòÁÐºÅ */
		if ( BN_hex2bn(&serial, val_status->serial) <=0 )
		{
			log->w(SecLog::ALERT, APPID, "Invalid new serial %s .", val_status->serial);
			goto err;
		}

		/* ÕªÒªËã·¨ */
		dgst = EVP_get_digestbyname(val_status->md); 
		if ( !dgst )
		{
			log->w(SecLog::ALERT, APPID, "Invalid digest %s .", val_status->md);
			goto err;
		}

		/* ÓÐÐ§ÌìÊý */
		days = val_status->days;

		/* CAË½Ô¿ */
		in2 = BIO_new(BIO_s_file());
		if ( BIO_read_filename(in2, val_status->private_key_file) <= 0 )
		{
			log->w(SecLog::ALERT, APPID, "Trying to load CA private key...");
			BIO_free(in2);
			goto err;
		}

		pkey = PEM_read_bio_PrivateKey(in2, NULL, NULL, (char*) (val_status->pass));
		BIO_free(in2);
		
		if ( !pkey ) 
		{
			log->w(SecLog::ALERT, APPID, "Unable to load CA private key.");
			goto err;
		}

		/* È¡µÃCAÖ¤Êé */
		in = BIO_new(BIO_s_mem());
		BIO_write(in, val_status->cert_pem, strlen(val_status->cert_pem));
		x509=PEM_read_bio_X509(in,NULL,NULL,NULL);
		if (x509 == NULL)
		{
			BIO_free(in);
			log->w(SecLog::ALERT, APPID, "unable to load CA certificate.");
			goto err;
		}

		BIO_free(in);
		
		/* ¼ì²éÁ½ÕßÊÇ·ñÆ¥Åä */
		if ( !X509_check_private_key(x509,pkey) )
		{
			log->w(SecLog::ALERT, APPID, "CA certificate and CA private key do not match");
			goto err;
		}

				
		if ( val_status->preserve )
			preserve = 1;

	} else {
		log->w(SecLog::ALERT, APPID, "CA has no para!");
		goto err;
	}
	
	/* È¡µÃÖ¤ÊéÉêÇëÄÚÈÝ */
	if ( val_cert.select(query) > 0)
	{
		BIO *in;
		in = BIO_new(BIO_s_mem());
		BIO_write(in, val_cert->req_pem, strlen(val_cert->req_pem));
		req = PEM_read_bio_X509_REQ(in, NULL, NULL, NULL);
		BIO_free(in);
		if ( !req ) goto err;
	} else {
		log->w(SecLog::NOTICE, APPID, "No such request %d", req_id);
		goto err;
	}

	name = X509_REQ_get_subject_name(req);
	for (i=0; i<X509_NAME_entry_count(name); i++)
	{
		ne=(X509_NAME_ENTRY *)X509_NAME_get_entry(name,i);
		obj=X509_NAME_ENTRY_get_object(ne);
		str=X509_NAME_ENTRY_get_data(ne);

		/* check some things */
		if ((OBJ_obj2nid(obj) == NID_pkcs9_emailAddress) &&
			(str->type != V_ASN1_IA5STRING))
		{
			log->w(SecLog::NOTICE, APPID,"emailAddress type needs to be of type IA5STRING");
			goto err;
		}
		j = ASN1_PRINTABLE_type(str->data,str->length);
		if (	((j == V_ASN1_T61STRING) &&
			 (str->type != V_ASN1_T61STRING)) ||
			((j == V_ASN1_IA5STRING) &&
			 (str->type == V_ASN1_PRINTABLESTRING)))
		{
			log->w(SecLog::NOTICE, APPID,"The string contains characters that are illegal for the ASN.1 type");
			goto err;
		}
	}

	/* Ok, now we check the 'policy' stuff. */
	if ((subject=X509_NAME_new()) == NULL)
	{
		log->w(SecLog::EMERG, APPID, "Memory allocation failure");
		goto err;
	}

	/* take a copy of the issuer name before we mess with it. */
	ca_name=X509_NAME_dup(x509->cert_info->subject);
	if (ca_name == NULL) goto err;
	str=str2=NULL;

	if (val_policy.select() > 0 ) { do 
	{	
		if ((j=OBJ_txt2nid((char*) (val_policy->name))) == NID_undef)
		{
			log->w(SecLog::ALERT, APPID,"%s:unknown object type in 'policy' configuration", val_policy->name);
			goto err;
		}
		obj=OBJ_nid2obj(j);

		last= -1;
		for (;;)
		{
			/* lookup the object in the supplied name list */
			j=X509_NAME_get_index_by_OBJ(name,obj,last);
			if (j < 0)
			{
				if (last != -1) break;
				tne=NULL;
			}
			else
			{
				tne=X509_NAME_get_entry(name,j);
			}
			last=j;

			/* depending on the 'policy', decide what to do. */
			push=NULL;
			if (strcmp(val_policy->value,"optional") == 0)
			{
				if (tne != NULL)
					push=tne;
			} else if (strcmp(val_policy->value,"supplied") == 0)
			{
				if (tne == NULL)
				{
					log->w(SecLog::NOTICE, APPID, "The %s field needed to be supplied and was missing", val_policy->name);
					goto err;
				}
				else
					push=tne;
			} else if (strcmp(val_policy->value,"match") == 0)
			{
				int last2;

				if (tne == NULL)
				{
					log->w(SecLog::NOTICE, APPID, "The mandatory %s field was missing", val_policy->name);
					goto err;
				}

				last2= -1;

again2:
				j=X509_NAME_get_index_by_OBJ(ca_name,obj,last2);
				if ((j < 0) && (last2 == -1))
				{
					log->w(SecLog::ALERT, APPID, "The %s field does not exist in the CA certificate", val_policy->name);
					goto err;
				}
				if (j >= 0)
				{
					push=X509_NAME_get_entry(ca_name,j);
					str=X509_NAME_ENTRY_get_data(tne);
					str2=X509_NAME_ENTRY_get_data(push);
					last2=j;
					if (ASN1_STRING_cmp(str,str2) != 0)
						goto again2;
				}
				if (j < 0)
				{
					log->w(SecLog::NOTICE, APPID, "The %s field needed to be the same in the CA certificate (%s) and the request (%s)", val_policy->name,((str2 == NULL)?"NULL":(char *)str2->data),((str == NULL)?"NULL":(char *)str->data));
					goto err;
				}
			} else
			{
				log->w(SecLog::ALERT, APPID, "%s:invalid type in 'policy' configuration\n", val_policy->value);
				goto err;
			}

			if (push != NULL)
			{
				if (!X509_NAME_add_entry(subject,push, -1, 0))
				{
					if (push != NULL)
						X509_NAME_ENTRY_free(push);
					log->w(SecLog::EMERG, APPID, "Memory allocation failure");
					goto err;
				}
			}
			if (j < 0) break;
		}
	} while (val_policy.next()); }

	if (preserve)
	{
		X509_NAME_free(subject);
		subject=X509_NAME_dup(X509_REQ_get_subject_name(req));
		if (subject == NULL) goto err;
	}

	if ((new_cert=X509_new()) == NULL) goto err;
	ci=new_cert->cert_info;

	if (BN_to_ASN1_INTEGER(serial,ci->serialNumber) == NULL)
	{
		log->w(SecLog::ALERT, APPID, "BN_to_ASN1_INTEGER failed in %s, %s", __FILE__, __LINE__);
		goto err;
	}

	if (!X509_set_issuer_name(new_cert ,X509_get_subject_name(x509)))
	{
		log->w(SecLog::ALERT, APPID, "X509_set_issuer_name failed in %s, %s", __FILE__, __LINE__);
		goto err;
	}

	if (strcmp(startdate,"today") == 0)
		X509_gmtime_adj(X509_get_notBefore(new_cert),0);
	else ASN1_UTCTIME_set_string(X509_get_notBefore(new_cert),startdate);

	if (enddate == NULL)
		X509_gmtime_adj(X509_get_notAfter(new_cert),(long)60*60*24*days);
	else ASN1_UTCTIME_set_string(X509_get_notAfter(new_cert),enddate);

	if (!X509_set_subject_name(new_cert,subject)) goto err;

	pktmp=X509_REQ_get_pubkey(req);
	i = X509_set_pubkey(new_cert ,pktmp);
	EVP_PKEY_free(pktmp);
	if (!i)
	{
		log->w(SecLog::ALERT, APPID, "X509_set_pubkey failed in %s, %s", __FILE__, __LINE__);
		goto err;
	}
	/* Lets add the extensions, if there are any */
	{
		X509V3_CTX ctx;
		STACK_OF(X509_EXTENSION) * req_ext = NULL;
		req_ext = X509_REQ_get_extensions(req);
		if (ci->version == NULL)
			if ((ci->version=ASN1_INTEGER_new()) == NULL)
				goto err;
		ASN1_INTEGER_set(ci->version,2); /* version 3 certificate */

		/* Free the current entries if any, there should not
		 * be any I believe */
		if (ci->extensions != NULL)
			sk_X509_EXTENSION_pop_free(ci->extensions,
						   X509_EXTENSION_free);

		ci->extensions = NULL;

		X509V3_set_ctx(&ctx, x509, new_cert, req, NULL, 0);
		X509V3_set_ctx_nodb((&ctx));	

		this->x509v3_ext_add_conf(&ctx, new_cert, FOR_USER, req_ext);
	}


#ifndef NO_DSA
	if (pkey->type == EVP_PKEY_DSA) dgst=EVP_dss1();
	pktmp=X509_get_pubkey(new_cert);
	if (EVP_PKEY_missing_parameters(pktmp) &&
		!EVP_PKEY_missing_parameters(pkey))
		EVP_PKEY_copy_parameters(pktmp,pkey);
	EVP_PKEY_free(pktmp);
#endif

	if (!X509_sign(new_cert, pkey,dgst))
	{
		log->w(SecLog::ALERT, APPID, "X509_sign failed in %s, %s", __FILE__, __LINE__);
		goto err;
	}
	ok=1;
err:
	if (ok >=1 )
	{
		char hep[8192];
		char aline[1024];
		hep[0] = '\0';
		BIO *myout = BIO_new(BIO_s_mem());	
		PEM_write_bio_X509(myout, new_cert);
		while ( BIO_gets(myout, aline , 1023) > 0 )
		{
			strcat(hep, aline);
		}
       		BIO_free(myout);

		if ( update_cert.select(query) > 0)
		{
			char *serial_str;
			BIGNUM *ser_bignum;
			update_cert->cert_pem = hep;
			update_cert->status =  UNBOUND;
			ser_bignum = ASN1_INTEGER_to_BN(new_cert->cert_info->serialNumber, NULL);
			serial_str = BN_bn2hex(ser_bignum);
			update_cert->serial_hex = serial_str;
			update_cert.update();
			OPENSSL_free(serial_str);
			BN_free(ser_bignum);
		}

		/* ¸üÐÂCAµÄ²ÎÊý */
		if ( update_ca_status.select() > 0)
		{
			BIGNUM *one, *new_serial;
			char *new_str, old_str[64];
			one = BN_new(); 
			BN_hex2bn(&one, "1");
			new_serial = BN_new(); 
			BN_add(new_serial, serial, one);
			new_str = BN_bn2hex(new_serial);
			strncpy( old_str, update_ca_status->serial,63);
			update_ca_status->serial_old = old_str;
			update_ca_status->serial =  new_str;
			update_ca_status.update();
			OPENSSL_free(new_str);
			BN_free(one);
			BN_free(new_serial);
		}
	}

	if ( ca_name ) X509_NAME_free(ca_name);
	if ( subject ) X509_NAME_free(subject);
	if ( req ) X509_REQ_free(req);
	if ( serial ) BN_free(serial);
	if ( x509 ) X509_free(x509);
	if ( pkey ) EVP_PKEY_free(pkey);
	if ( new_cert) X509_free( new_cert);

	dbh->commit();
	if ( ok >= 1 ) 
		return true;
	else
		return false;
}

bool MiniCA::gencrl()
{
	SecLog *log = SecLog::instance();
	dbCursor<CA_status> ca_status;
	dbCursor<CA_status> update_ca_status (dbCursorForUpdate) ;
	dbCursor<User_certs> val_cert;
	dbQuery query;
	
	X509_CRL *crl=NULL;
	X509_CRL_INFO *ci=NULL;
	X509 *x509 = NULL;
	X509_REVOKED *r=NULL;
	X509V3_CTX crlctx;

	int days = 2;
	int crlhours = 0;
	const EVP_MD *dgst = NULL;
	EVP_PKEY *pkey = NULL;
	ASN1_TIME *tmptm = NULL;
	int crl_v2 = 0;
	
	bool ret = true;	

	if ( !dbh ) return false;
	/* È¡µÃCAµÄ²ÎÊý */
	if ( ca_status.select() > 0)
	{
		BIO *in,*in2;

		/* ÕªÒªËã·¨ */
		dgst = EVP_get_digestbyname(ca_status->md); 
		if ( !dgst )
		{
			log->w(SecLog::ALERT, APPID, "Invalid digest=%s .", ca_status->md);
			goto end;
		}

		/* ÓÐÐ§ÌìÊý */
		days = ca_status->crl_days;

		/* CAË½Ô¿ */
		in2 = BIO_new(BIO_s_file());
		if ( BIO_read_filename(in2, ca_status->private_key_file) <= 0 )
		{
			log->w(SecLog::ALERT, APPID, "Loading CA private key... ");
			BIO_free(in2);
			goto end;
		}

		pkey = PEM_read_bio_PrivateKey(in2, NULL, NULL, (char*) (ca_status->pass));
		BIO_free(in2);
		
		if ( !pkey ) 
		{
			log->w(SecLog::ALERT, APPID, "Unable to load CA private key");
			goto end;
		}

		/* È¡µÃCAÖ¤Êé */
		in = BIO_new(BIO_s_mem());
		BIO_write(in, ca_status->cert_pem, strlen(ca_status->cert_pem));
		x509=PEM_read_bio_X509(in,NULL,NULL,NULL);
		if (x509 == NULL)
		{
			BIO_free(in);
			log->w(SecLog::ALERT, APPID, "Unable to load CA certificate");
			goto end;
		}

		BIO_free(in);
		
		/* ¼ì²éÁ½ÕßÊÇ·ñÆ¥Åä */
		if ( !X509_check_private_key(x509,pkey) )
		{
			log->w(SecLog::ALERT, APPID, "CA certificate and CA private key do not match");
			goto end;
		}

	} else {
		log->w(SecLog::ALERT, APPID, "CA has no para!");
		goto end;
	}

	if ((crl=X509_CRL_new()) == NULL) 
	{
		log->w(SecLog::ALERT, APPID, "X509_CRL_new failed");
		goto end;
	}

	if (!X509_CRL_set_issuer_name(crl, X509_get_subject_name(x509)))
	{
		log->w(SecLog::ALERT, APPID, "X509_CRL_set_issuer_name failed");
		goto end;
	}

	tmptm = ASN1_TIME_new();
	X509_gmtime_adj(tmptm,0);
	X509_CRL_set_lastUpdate(crl, tmptm);	
	X509_gmtime_adj(tmptm,(days*24+crlhours)*60*60);
	X509_CRL_set_nextUpdate(crl, tmptm);	

	ASN1_TIME_free(tmptm);

	query = "status =", REVOKED; /* ²éÕÒËùÓÐÒÑ±»µõÏúµÄÖ¤Êé */
	if ( val_cert.select(query) > 0 ) { do
	{
		int j; 
		ASN1_INTEGER *tmpser;
		BIGNUM *num_serial = BN_new();  /* ±»µõÏúÖ¤ÊéµÄÐòÁÐºÅ */
		if ( BN_hex2bn(&num_serial, val_cert->serial_hex) <= 0 ) 
		{
			log->w(SecLog::NOTICE, APPID, "The revoked serial %s is invalid.",val_cert->serial_hex);
			goto Next;
		}

		if ((r=X509_REVOKED_new()) == NULL) 
			goto Next;

		j = make_revoked(r, (char*) val_cert->revoke_date);
		if (j == 2) crl_v2 = 1;
		tmpser = BN_to_ASN1_INTEGER(num_serial, NULL);
		
		X509_REVOKED_set_serialNumber(r, tmpser);
		ASN1_INTEGER_free(tmpser);
		X509_CRL_add0_revoked(crl,r);

	Next:
		BN_free(num_serial);	
	} while (val_cert.next()); }
	
	/* sort the data so it will be written in serial
	 * number order */
	X509_CRL_sort(crl);

	/* we now have a CRL */
	if ((dgst=EVP_get_digestbyname(ca_status->md)) == NULL)
	{
		log->w(SecLog::ALERT, APPID, "%s is an unsupported message digest type",ca_status->md);
	} else
	{

#ifndef NO_DSA
	    if (pkey->type == EVP_PKEY_DSA) 
		dgst=EVP_dss1();
	    else
#endif
		dgst=EVP_md5();
	}

	/* Add any extensions  */
	X509V3_set_ctx(&crlctx, x509, NULL, NULL, crl, 0);
	X509V3_set_ctx_nodb((&crlctx));	

	/* Add extensions */
	if (!this->x509v3_ext_add_conf(&crlctx, crl))
	{
		log->w(SecLog::WARNING, APPID, "CRL adding extension failed");
	}

	X509_CRL_set_version(crl, 1);  /* version 2 CRL */

	if (X509_CRL_sign(crl,pkey,dgst))
	{
		char hep[8192];
		char aline[1024];
		hep[0] = '\0';
		BIO *out = BIO_new(BIO_s_mem());

		PEM_write_bio_X509_CRL(out,crl);
		while ( BIO_gets(out, aline , 1023) > 0 )
		{
			strcat(hep, aline);
		}
	       	BIO_free(out);
	
		/* ¸üÐÂCAµÄcrl_pemÄÚÈÝý */
		if ( update_ca_status.select() > 0)
		{
			update_ca_status->crl_pem = hep;
			update_ca_status.update();
		}
	} else
	{
		log->w(SecLog::ALERT, APPID, "X509_CRL_sign failed");
		ret =false;
	}

end:
	X509_free(x509);
	X509_CRL_free(crl);
	EVP_PKEY_free(pkey);
	dbh->commit();
	return ret;
}

bool MiniCA::newCA(int keySize, int days)
{
	SecLog *log = SecLog::instance();
	dbCursor<CA_status> ca_status;
	dbCursor<CA_status> update_ca_status (dbCursorForUpdate) ;
	dbCursor<CA_subject> ca_subj;
#ifndef NO_DSA
	DSA *dsa_params=NULL;
#endif
	int pkey_type = TYPE_RSA;
	FILE *fp;
	EVP_PKEY *pkey = NULL;
	X509 *x509ss = NULL;
	X509_REQ *req = NULL;
	X509_NAME *subj = NULL;

	EVP_PKEY *tmppkey;
	const EVP_MD *dgst = NULL;
	X509V3_CTX ext_ctx;

	BIO *out;
	char hep[8192];
	char aline[1024];
	hep[0] = '\0';

	bool ret = true;

	if ( !dbh ) return false;
	if ( keySize != 2048 &&  keySize !=1024 ) return false;
	if ( days <= 1 || days > 7300) return false;

	/* È¡µÃCAµÄ²ÎÊý */
	if ( ca_status.select() <= 0)
	{
		log->w(SecLog::ALERT, APPID, "No CA parameters");
		return false;
	}

	/* ÏÈ²úÉú¹«Ë½Ô¿¶Ô */

	app_RAND_load_file(ca_status->rand_file, 0);
	
	if ((pkey=EVP_PKEY_new()) == NULL) 
	{
		log->w(SecLog::ALERT, APPID, "Generating key pair...  failed");
		goto end;
	}

#ifndef NO_RSA
	if (pkey_type == TYPE_RSA)
	{
		if (!EVP_PKEY_assign_RSA(pkey,
				RSA_generate_key(keySize, 0x10001,
					NULL, NULL)))
				goto end;
	}
	else
#endif
#ifndef NO_DSA
	if (pkey_type == TYPE_DSA)
	{
		if (!DSA_generate_key(dsa_params)) goto end;
		if (!EVP_PKEY_assign_DSA(pkey,dsa_params)) goto end;
		dsa_params=NULL;
	}
#endif

	app_RAND_write_file(ca_status->rand_file);

	fp = fopen(ca_status->private_key_file, "w");
	if ( !fp )
	{
		/* ÎÄ¼þ´ò¿ª´íÎó */
		log->w(SecLog::ALERT, APPID, "Open private key file error.");
		ret = false;
		goto end;

	}

	if (!PEM_write_PrivateKey(fp,pkey, NULL, NULL,0,NULL, NULL))
	{
		/* Ë½Ô¿Ð´Èë´íÎó */
		log->w(SecLog::ALERT, APPID, "Writing private key file error.");
		fclose(fp);
		ret = false;
		goto end;
	}

	fclose(fp);

	if ((dgst=EVP_get_digestbyname(ca_status->md)) == NULL)
	{
		log->w(SecLog::ALERT, APPID, "%s is an unsupported message digest type.",ca_status->md);
	} else
	{

#ifndef NO_DSA
	    if (pkey->type == EVP_PKEY_DSA) 
		dgst=EVP_dss1();
	    else
#endif
		dgst=EVP_md5();
	}

	/* ´´½¨X.509ÇëÇóé */
	req=X509_REQ_new();
	if (req == NULL)
	{
		log->w(SecLog::ALERT, APPID, "Creating X509 request...  failed");
		ret = false;
		goto end;
	}

	if (!X509_REQ_set_version(req,0L)) 
	{
		log->w(SecLog::ALERT, APPID, "Set X509 request version...  failed");
		ret = false;
		goto end;
	}
	X509_REQ_set_pubkey(req,pkey);	

	subj = X509_REQ_get_subject_name(req);

	/* ¸³Óè´´½¨Õß×ÊÁÏ */
	if (ca_subj.select() > 0 ) do 
	{
		X509_NAME_add_entry_by_txt(subj, (char*) ca_subj->type, MBSTRING_ASC,
			(unsigned char *) ca_subj->value, -1, -1, 0) ;
	}  while (ca_subj.next()); 

	
 	/* ´´½¨X509Ö¤Êé */
	if ((x509ss=X509_new()) == NULL)
	{
		log->w(SecLog::ALERT, APPID, "Creating new X509 ...  failed");
		ret = false;
		goto end;
	}
	

	/* Set version to V3 */
	if(!X509_set_version(x509ss, 2))
	{
		log->w(SecLog::ALERT, APPID, "Set X509 cert version 3...  failed");
		ret = false;
		goto end;
	}
	
	ASN1_INTEGER_set(X509_get_serialNumber(x509ss),0L);

	X509_set_issuer_name(x509ss, X509_REQ_get_subject_name(req));
	X509_gmtime_adj(X509_get_notBefore(x509ss),0);
	X509_gmtime_adj(X509_get_notAfter(x509ss),
		(long)60*60*24*days);
	X509_set_subject_name(x509ss, X509_REQ_get_subject_name(req));
	tmppkey = X509_REQ_get_pubkey(req);
	X509_set_pubkey(x509ss,tmppkey);
	EVP_PKEY_free(tmppkey);

	/* Set up V3 context struct */

	X509V3_set_ctx(&ext_ctx, x509ss, x509ss, NULL, NULL, 0);
	X509V3_set_ctx_nodb((&ext_ctx));	

	/* Add extensions */
	this->x509v3_ext_add_conf(&ext_ctx, x509ss, FOR_CA, NULL);

	if ( !X509_sign(x509ss,pkey,dgst))
	{
		log->w(SecLog::ALERT, APPID, "Signing X509 cert ...  failed");
		ret = false;
		goto end;
	}
	
	out = BIO_new(BIO_s_mem());
	PEM_write_bio_X509(out,x509ss);
	while ( BIO_gets(out, aline , 1023) > 0 )
	{
		strcat(hep, aline);
	}
       	BIO_free(out);
	
	/* ¸üÐÂCAµÄÖ¤ÊéÄÚÈÝý */
	if ( update_ca_status.select() > 0)
	{
		update_ca_status->cert_pem = hep;
		update_ca_status.update();
	}

end:
	EVP_PKEY_free(pkey);
	X509_REQ_free(req);
	X509_free(x509ss);
	dbh->commit();
	return ret;
}

char *MiniCA::getP7(int req_id)
{
	char *retstr = (char*) 0;
	SecLog *log = SecLog::instance();

	dbCursor<User_certs> val_cert;
	dbCursor<CA_status> ca_status;
	dbQuery query;

	int i, ok=0;

	BIO *cert_in[2]={NULL,NULL};
	BIO *out=NULL;

	PKCS7 *p7 = NULL;
	PKCS7_SIGNED *p7s = NULL;
	STACK_OF(X509_CRL) *crl_stack=NULL;
	STACK_OF(X509) *cert_stack=NULL;
	STACK_OF(X509_INFO) *sk=NULL;
	X509_INFO *xi;

	if ( !dbh ) return (char*) 0;	

	/* È¡µÃÓÃ»§Ö¤ÊéÄÚÈÝ */
	query = "req_id =",req_id ;
	if ( val_cert.select(query) > 0)
	{
		if ( val_cert->cert_pem == (char*) 0 
			|| strlen(val_cert->cert_pem) < 0 )
		{
			log->w(SecLog::NOTICE, APPID, "No cert of request %d", req_id);
			goto end;
		}
		cert_in[0] = BIO_new(BIO_s_mem());
		BIO_write(cert_in[0], val_cert->cert_pem, strlen(val_cert->cert_pem));
	} else {
		log->w(SecLog::NOTICE, APPID, "No such request %d", req_id);
		goto end;
	}

	/* È¡µÃCAÖ¤ÊéÄÚÈÝ */
	if ( ca_status.select() > 0)
	{
		if ( ca_status->cert_pem == (char*) 0 
			|| strlen(ca_status->cert_pem) < 0 )
		{
			log->w(SecLog::NOTICE, APPID, "No CA cert");
			goto end;
		}
		cert_in[1] = BIO_new(BIO_s_mem());
		BIO_write(cert_in[1], ca_status->cert_pem, strlen(ca_status->cert_pem));
	} else {
		log->w(SecLog::NOTICE, APPID, "No CA parameters");
		goto end;
	}

	out= BIO_new(BIO_s_mem());

	if ((p7=PKCS7_new()) == NULL) goto end;
	if ((p7s=PKCS7_SIGNED_new()) == NULL) goto end;
	p7->type=OBJ_nid2obj(NID_pkcs7_signed);
	p7->d.sign=p7s;
	p7s->contents->type=OBJ_nid2obj(NID_pkcs7_data);

	if (!ASN1_INTEGER_set(p7s->version,1)) goto end;
	if ((crl_stack=sk_X509_CRL_new_null()) == NULL) goto end;
	p7s->crl=crl_stack;

	if ((cert_stack=sk_X509_new_null()) == NULL) goto end;
	p7s->cert=cert_stack;

	/* This loads from a file, a stack of x509/crl/pkey sets */
	for ( i = 0 ; i < 2; i++ )
	{
		sk=PEM_X509_INFO_read_bio(cert_in[i],NULL,NULL,NULL);
		if (sk == NULL) {
			log->w(SecLog::NOTICE, APPID, "Can't read cert_in[%d]",i);
			goto end;
		}

		/* scan over it and pull out the CRL's */
		while (sk_X509_INFO_num(sk))
		{
			xi=sk_X509_INFO_shift(sk);
			if (xi->x509 != NULL)
			{
				sk_X509_push(cert_stack,xi->x509);
				xi->x509=NULL;
			}
			X509_INFO_free(xi);
		}

		sk_X509_INFO_free(sk);
	}

	ok =PEM_write_bio_PKCS7(out,p7);
	if (ok)
	{
		char hep[16384];
		char aline[1024];
		hep[0] = '\0';
		while ( BIO_gets(out, aline , 1023) > 0 )
		{
			if ( memcmp (aline, "-----BEGIN PKCS7" , 16) == 0 )
			{
				strcat(hep, "-----BEGIN CERTIFICATE-----\r\n");
			} else if  ( memcmp(aline, "-----END PKCS7", 14) == 0 )
			{
				strcat(hep, "-----END CERTIFICATE-----");
			} else 
				strcat(hep, aline);
		}
		retstr = (char*) malloc(strlen(hep) +1 );
		strcpy(retstr, hep);
	}
end:
	for ( i = 0 ; i < 2; i++ )
	{
		if ( cert_in[i] != NULL ) BIO_free_all(cert_in[i]);
	}
	if (out != NULL) BIO_free_all(out);
	if (p7 != NULL) PKCS7_free(p7);

	return retstr;
}
