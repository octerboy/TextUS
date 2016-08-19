/**
 标题:CaLet类的定义
 标识:XMLHTTP-calet.cpp
 版本:B001
	B001:created by octerboy 2005/02/25
*/
#ifndef CALET_H
#define	CALET_H
#include "XmlRequest.h"
#include "XmlResponse.h"
#include "fastdb.h"
#include "XmlLet.h"
#include "ExtInterface.h"
#include "MiniCA.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/txt_db.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/objects.h>
#include <openssl/pem.h>

class CaLet :public ExtInterface, public XmlLet, public MiniCA
{
public:
	CaLet();
	/* false:未处理请求; true:已处理请求 */
	virtual bool handle(XmlRequest *request, XmlResponse *response);
	virtual void get_versions(char ver[]);
	virtual bool init(int, char *[]);
	bool down();
	bool up();
	~CaLet();

private:
	char db_file[128];
	char sys_file[128];
};
#endif
