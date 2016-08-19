/**
 标题:MiniCA类的定义
 标识:XMLHTTP-MiniCA.h
 版本:B001
	B001:created by octerboy 2005/02/25
*/
#ifndef MINICA_H
#define	MINICA_H
#include "fastdb.h"
#include <openssl/x509.h>
#include <openssl/x509v3.h>

/* CA参数定义 */
class X509v3_extensions_conf { 
	public:   
		const char* name;     
		const char* value;   
		int type;	//0: CA证书的, 1:用户证书的, 2:CRL的
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

/* 此表只有一条记录,记录CA当前工作参数 */
class CA_status { 
	public:   
		int days;   	/* 默认的签证有效天数 */
		int crl_days;   /* 默认的CRL有效天数 */
		const char* md; /* 签证所用的HASH算法,md5,sha1等 */
		bool sign_auto;	/* 是否自动签证 */
		bool preserve;  /* */
		const char* private_key_file;   /* 外部私钥文件 */
		const char* rand_file;   /* 保存随机数文件 */
		const char* pass;  	/* 私钥口令 */
		const char* cert_pem;   /* CA证书内容，PEM格式 */
		const char* crl_pem;  	/* CRL内容，PEM格式 */ 
		const char* serial;     /* 下一次签证的序列号 */
		const char* serial_old;   /* 最近一次的序列号 */
		int req_id;   		/* 下一次请求证书的标识 */
		int req_id_old;   	/* 最近一次的请求证书的标识 */
		TYPE_DESCRIPTOR(( FIELD(days),FIELD(crl_days), FIELD(md), FIELD(sign_auto), FIELD(preserve), FIELD(private_key_file), FIELD(rand_file), FIELD(pass), FIELD(cert_pem), FIELD(crl_pem), FIELD(serial), FIELD(serial_old), FIELD(req_id), FIELD(req_id_old)
		)) ;
	};

/* 用户证书 */
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
	/* 返回 < 0 :错误, 0：正常, 1:需要重载防火墙规则, */
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
