/**
 标题:网络论断
 标识:Textus-Netdiag.cpp
 版本:B01
	B01:created by octerboy 2006/07/15
*/
#include "Amor.h"
#include "Notitia.h"
//#include <unistd.h>
//#include <string.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <sys/utsname.h>
//#include <sys/mman.h>
//#include <fcntl.h>
//#include <time.h>
//#include <sys/time.h>
#include <assert.h>
//#include <errno.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
#include <net/if.h>
//#include <arpa/inet.h>

#include <time.h>
#include <sys/time.h>
#include "diag.h"
#include "wlog.h"

#define HaveValue(x) ((x) \
	&& (x)->FirstChild() \
	&& (x)->FirstChild()->ToText() \
	&& (x)->FirstChild()->ToText()->Value())

#define GetValue(x) ((x)->FirstChild()->ToText()->Value())

class Netdiag :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	Netdiag();
	~Netdiag();

private:
	Amor::Pius local_pius;  //仅用于向mary传回数据
	inline void handle();
	TiXmlDocument *req_doc;	
	TiXmlDocument *res_doc;	
};

void Netdiag::ignite(TiXmlElement *cfg) { }

bool Netdiag::facio( Amor::Pius *pius)
{
	TiXmlDocument **doc=0;
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::PRO_TINY_XML:	/* 有HTTP请求 */
		WBUG("facio PRO_TINY_XML");
		handle();
		aptus->sponte(&local_pius);
		break;

	case Notitia::SET_TINY_XML:	/* 取得输入XML doc地址 */
		WBUG("facio SET_TINY_XML")
		if ( (doc = (TiXmlDocument **)(pius->indic)))
		{	//tb应当不为NULL，*tb是rcv_buf
			if ( *doc) req_doc = *doc; 
			else
				WLOG(WARNING, "facio SET_TINY_XML req_doc null");
			doc++;
			if ( *doc) res_doc = *doc;
			else
				WLOG(WARNING, "facio SET_TINY_XML res_doc null");
		} else 
			WLOG(WARNING, "facio SET_TINY_XML null");
		break;

	case Notitia::IGNITE_ALL_READY:	
	case Notitia::CLONE_ALL_READY:
		break;
		
	default:
		return false;
	}
	return true;
}

bool Netdiag::sponte( Amor::Pius *pius) { return false; }

Netdiag::Netdiag()
{
	local_pius.ordo = Notitia::PRO_TINY_XML;
	local_pius.indic = 0;
	res_doc = req_doc = 0;
}

Amor* Netdiag::clone()
{
	return  (Amor*)new Netdiag();
}

Netdiag::~Netdiag() { }

void Netdiag::handle()
{
	TiXmlElement root( "System" );
	TiXmlElement result( "result" );
	TiXmlElement description( "description" );
	TiXmlText resu( "");
	TiXmlText desc( "" );
	
	root.SetAttribute( "content", "diag" );
	root.SetAttribute( "action", "response");
	
	Diag dg;
	
	TiXmlElement *reqele = req_doc->RootElement();
	int act;
	char tmpstr[300];
	TiXmlElement *actele;

	if (!reqele )
		return ;	/* 由扩展函数处理 */

	tmpstr[0] = '\0';
	if (strcmp (reqele->Value(), "System") != 0
		|| reqele->Attribute("content") == (char*) NULL
		|| strcmp( reqele->Attribute("content"), "diag") != 0
		|| reqele->Attribute("action") == (char*) NULL
		|| strcmp( reqele->Attribute("action"), "request") != 0
	)
	{
		desc.SetValue("no request xml data.");
		resu.SetValue("fail");
		goto SystemDiagEnd;
	}

	actele = reqele->FirstChildElement("ping");
	act = 1;
	if (!actele) 
	{
		actele = reqele->FirstChildElement("dns");
		act = 2;
	}

	if (!actele) 
	{
		actele = reqele->FirstChildElement("traceroute");
		act = 3;
	}

	if (!actele)
	{
		desc.SetValue("no request xml data.");
		resu.SetValue("fail");
		goto SystemDiagEnd;
	}

	if ( act == 1 )
	{
		/* ping 诊断 */
		if ( HaveValue(actele) )
		{
			if ( dg.ping(GetValue(actele)) == 0 )
			{
				/* ping 成功 */
				sprintf(tmpstr, "%s is alive", GetValue(actele));
				desc.SetValue(tmpstr);
				resu.SetValue("success");
			} else {
				/* ping 失败 */
				sprintf(tmpstr, "%s is not alive", GetValue(actele));
				desc.SetValue(tmpstr);
				resu.SetValue("fail");
			}
		} else {
			/* 没有ping的目标主机 */
			desc.SetValue("no ping destination.");
			resu.SetValue("fail");
		}
	} else if ( act == 2 ) 
	{
		/* dns 诊断 */
		if ( HaveValue(actele) )
		{
			char *dnsstr = dg.dns(GetValue(actele));
			if ( dnsstr ) 
			{
				/* 域名解析成功 */
				desc.SetValue(dnsstr);
				resu.SetValue("success");
			} else {
				/* 域名解析失败 */
				sprintf(tmpstr, "%s can not be resolved!", GetValue(actele));
				desc.SetValue(tmpstr);
				resu.SetValue("fail");
			}
		} else 
		{
			/* 没有域名 */
			desc.SetValue("no host name");
			resu.SetValue("fail");
		}
	} else if ( act == 3 ) 
	{
		/* traceroute 诊断 */
		if ( HaveValue(actele) )
		{
			char cmdstr[256];
			time_t now; 
			char filename[256],aline[256];
			FILE *fp;

			now = time(&now);
			sprintf(filename, "/tmp/trace.%lu", now);
			sprintf(cmdstr,"traceroute %s > %s",GetValue(actele), filename);
			system(cmdstr);

			resu.SetValue("success");
			result.InsertEndChild(resu);
			root.InsertEndChild(result);

			fp = fopen(filename,"r");
			while ( fgets(aline,256,fp))
			{
				char *tail = strpbrk(aline,"\r\n");
				*tail = '\0';
				TiXmlElement aroute( "description" );
				desc.SetValue(aline);
				aroute.InsertEndChild(desc);
				root.InsertEndChild(aroute);
			}
			fclose(fp);
			
			goto SystemDiagEndTwo;
		} else 
		{
			/* 没有主机名 */
			desc.SetValue("no host name");
			resu.SetValue("fail");
		}
	}

SystemDiagEnd:
	result.InsertEndChild(resu);
	description.InsertEndChild(desc);
	root.InsertEndChild(result);
	root.InsertEndChild(description);

SystemDiagEndTwo:
	res_doc->InsertEndChild(root);
}
#include "hook.c"


