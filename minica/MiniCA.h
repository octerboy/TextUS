/**
 ����:MiniCA��Ķ���
 ��ʶ:XMLHTTP-MiniCA.h
 �汾:B001
	B001:created by octerboy 2005/02/25
*/
#ifndef MINICA_H
#define	MINICA_H
#include "fastdb.h"
#include <openssl/x509.h>
#include <openssl/x509v3.h>

/* CA�������� */
class X509v3_extensions_conf { 
	public:   
		const char* name;     
		const char* value;   
		int type;	//0: CA֤���, 1:�û�֤���, 2:CRL��
		TYPE_DESCRIPTOR((FIELD(name),FIELD(value), FIELD(type)
		)) ;
	};

class CA_subject {
	public:
		const char* type;
		const char* value;
		TYPE_DESCRIPTOR((FIELD(type),FIELD(value)
		)) ;
	};

class CA_policy_conf { 
	public:   
		const char* name;   
		const char* value; 
		TYPE_DESCRIPTOR((FIELD(name),FIELD(value)
		)) ;
	};

/* �˱�ֻ��һ����¼,��¼CA��ǰ�������� */
class CA_status { 
	public:   
		int days;   	/* Ĭ�ϵ�ǩ֤��Ч���� */
		int crl_days;   /* Ĭ�ϵ�CRL��Ч���� */
		const char* md; /* ǩ֤���õ�HASH�㷨,md5,sha1�� */
		bool sign_auto;	/* �Ƿ��Զ�ǩ֤ */
		bool preserve;  /* */
		const char* private_key_file;   /* �ⲿ˽Կ�ļ� */
		const char* rand_file;   /* ����������ļ� */
		const char* pass;  	/* ˽Կ���� */
		const char* cert_pem;   /* CA֤�����ݣ�PEM��ʽ */
		const char* crl_pem;  	/* CRL���ݣ�PEM��ʽ */ 
		const char* serial;     /* ��һ��ǩ֤�����к� */
		const char* serial_old;   /* ���һ�ε����к� */
		int req_id;   		/* ��һ������֤��ı�ʶ */
		int req_id_old;   	/* ���һ�ε�����֤��ı�ʶ */
		TYPE_DESCRIPTOR(( FIELD(days),FIELD(crl_days), FIELD(md), FIELD(sign_auto), FIELD(preserve), FIELD(private_key_file), FIELD(rand_file), FIELD(pass), FIELD(cert_pem), FIELD(crl_pem), FIELD(serial), FIELD(serial_old), FIELD(req_id), FIELD(req_id_old)
		)) ;
	};

/* �û�֤�� */
class User_certs { 
	public:   
		const char* serial_hex;
		const char* revoke_date;
		int1 status;	
		const char* subject;
		const char* cert_pem;   
		int req_id;
		const char* req_pem;
		TYPE_DESCRIPTOR((KEY(serial_hex, INDEXED|HASHED),
				KEY(req_id, INDEXED),
				FIELD(status),FIELD(revoke_date),
				FIELD(subject), FIELD(req_pem), FIELD(cert_pem)
		)) ;
	};

class MiniCA
{
public:
	MiniCA();
	/* ���� < 0 :����, 0������, 1:��Ҫ���ط���ǽ����, */
	enum {REQUEST, DENIED, UNBOUND, BOUND, REVOKED} cert_status;
	~MiniCA();
	char errMsg[1024];

protected:
	dbDatabase *dbh;	
	bool opendb(char *db_file);
	void closedb();
	bool sign_req(char *startdate, char *enddate, int req_id);
	bool newCA(int, int);
	bool gencrl();
	bool getSubject(const char *pem, char subj[]);
	char *getP7(int req_id);

private:
	
	X509_EXTENSION * do_v3_ext(X509V3_CTX *ctx, int ext_nid, int crit, char * value);
	X509_EXTENSION * do_v3_ext_i2d(X509V3_EXT_METHOD *method, int ext_nid, int crit, void *ext_struc);
	int v3_check_critical(char **value);
	int v3_check_generic(char **value);
	X509_EXTENSION * v3_generic_extension(const char *ext, char *value, int crit, int type);
	X509_EXTENSION * x509v3_ext(X509V3_CTX *ctx, char *name, char *value, STACK_OF(X509_EXTENSION) * req_ext);
	bool x509v3_ext_add_conf(X509V3_CTX *ctx, X509 *cert, int who, STACK_OF(X509_EXTENSION) *req_ext);
	bool x509v3_ext_add_conf(X509V3_CTX *ctx, X509_CRL *crl);

};
#endif
