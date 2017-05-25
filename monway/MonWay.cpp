/* Copyright (c) 2016-2018 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title:monitor business to  insway
 Build: created by octerboy, 2016/04/22, Guangzhou
 $Header: /textus/monway/MonWay.cpp     14-03-10 8:13 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: MonWay.cpp $"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "md5.h"
#include "TBuffer.h"
#include "BTool.h"
#include "casecmp.h"
#include "textus_string.h"
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>

#define Obtainx(s)   "0123456789abcdef"[s]
#define ObtainX(s)   "0123456789ABCDEF"[s]
#define Obtainc(s)   (s >= 'A' && s <='F' ? s-'A'+10 :(s >= 'a' && s <='f' ? s-'a'+10 : s-'0' ) )

static char* byte2hex(const unsigned char *byte, size_t blen, char *hex) 
{
	size_t i;
	for ( i = 0 ; i < blen ; i++ )
	{
		hex[2*i] =  ObtainX((byte[i] & 0xF0 ) >> 4 );
		hex[2*i+1] = ObtainX(byte[i] & 0x0F );
	}
//	hex[2*i] = '\0';
	return hex;
}

static unsigned char* hex2byte(unsigned char *byte, size_t blen, const char *hex)
{
	size_t i;
	const char *p ;	

	p = hex; i = 0;

	while ( i < blen )
	{
		byte[i] =  (0x0F & Obtainc( hex[2*i] ) ) << 4;
		byte[i] |=  Obtainc( hex[2*i+1] ) & 0x0f ;
		i++;
		p +=2;
	}
	return byte;
}

#define FIELD_NUM 65
static char* fld_string[] = { "bitmap", "bitmap2", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", 
"f11", "f12", "f13", "f14", "f15", "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23", "f24", "f25", "f26","f27", "f28", "f29", "f30",
"f41", "f42", "f43", "f44", "f45", "f46", "f47", "f48", "f49", "f50", "f51", "f52", "f53", "f54", "f55", "f56","f57", "f58", "f59", "f60",
"f61", "f62", "f63", "f64" };

class MonWay: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	MonWay();
	~MonWay();

private:
	Amor::Pius local_pius;

	struct G_CFG {
		enum Work_Mode work_mode;
		int business_fldno;		/* 域号 */
		unsigned char tst_code[64];
		int tst_len;

		int channelID_fldno;		/* 域号 */
		char chan_idstr[32];
	
		int err_fldno;		/* 错误代码域号 */
		int msg_fldno;		/* 错误代码域号 */
		inline G_CFG() {
			business_fldno = 3;
			memcpy(tst_code, "\x00\x00\x00\x00", 4);
			tst_len =4 ;
			channelID_fldno = 58;
			TEXTUS_SPRINTF( chan_idstr, "f%s", channelID_fldno);
			err_fldno = 39;
		};

		inline void prop(TiXmlElement *cfg)
		{
			cfg->QueryIntAttribute("business", &(business_fldno));
			cfg->QueryIntAttribute("channel", &(channelID_fldno));
			cfg->QueryIntAttribute("error", &(err_fldno));
			cfg->QueryIntAttribute("message", &(msg_fldno));
		};
		

		inline ~G_CFG() {
		};
	};
	struct G_CFG *gCFG;     /* 全局共享参数 */
	bool has_config;

	TiXmlDocument *req_doc;	
	TiXmlDocument *res_doc;	

	PacketObj do_pac;
	PacketObj un_pac;	/* 向右传递的 */
	PacketObj *pa[3];

	enum  WK_STATUS {WS_Idle = 0, WS_Getting_Chan = 2, WS_Channle_OK=4}  wk_status;

	bool handle2();
	bool test_chan_ask();
	bool test_chan_res();
	void handle3();
	inline void deliver(Notitia::HERE_ORDO aordo);
	#include "wlog.h"
	#include "httpsrv_obj.h"
};


#include <assert.h>

long MonWay::token_num=0 ;
void MonWay::ignite(TiXmlElement *prop)
{
	if (!prop) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
		gCFG->prop(prop);
	}
	do_pac.produce(128) ;
	un_pac.produce(128) ;
}

MonWay::MonWay()
{
	local_pius.ordo = Notitia::PRO_UNIPAC;
	local_pius.indic = 0;

	gCFG = 0;
	has_config = false;
	has_channel = false;

	pa[0] = &do_pac;
	pa[1] = &un_pac;
	pa[2] = 0;

}

MonWay::~MonWay() 
{
	if ( has_config  )
	{	
		if(gCFG) delete gCFG;
	}
}

Amor* MonWay::clone()
{
	MonWay *child = new MonWay();
	child->gCFG = gCFG;
	return (Amor*) child;
}

bool MonWay::facio( Amor::Pius *pius)
{
	TiXmlDocument **doc=0;
	bool h_ret;
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::PRO_TINY_XML:	/* 有XML数据请求 */
		WBUG("facio PRO_TINY_XML");
		switch ( wk_status )
		{
		case WS_Channle_OK:
			h_ret = handle2();
			if ( h_ret )
			{
				aptus->facio(&local_pius);
			}
			break;

		case WS_Idle:
			h_ret = test_chan_ask();	//通道测试
			if ( h_ret )
			{
				wk_status = WS_Getting_Chan;
				aptus->facio(&local_pius);
			}
			break;

		default:
			break;
		}
		break;

	case Notitia::SET_TINY_XML:	/* 取得输入TBuffer地址 */
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
		WBUG("facio IGNITE_ALL_READY" );
		goto DEL_PAC;
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
DEL_PAC:
		deliver(Notitia::SET_UNIPAC);
		break;

	case Notitia::DMD_END_SESSION:  /* 强制关闭 */
		WBUG("facio DMD_END_SESSION");
		wk_status = WS_Idle;
		break;

	default:
		return false;
	}
	return true;
}

bool MonWay::sponte( Amor::Pius *pius)
{
	switch ( pius->ordo )
	{
	case Notitia::PRO_UNIPAC:
		WBUG("sponte PRO_UNIPAC");
		switch ( wk_status )
		{
		case WS_Channle_OK:
			handle3();	//业务响应
			break;

		case WS_Getting_Chan:
			h_ret = test_chan_res();	//通道测试响应
			if ( h_ret )
			{
				wk_status = WS_Channle_OK;
				h_ret = handle2();	//业务请求发过去
				if ( h_ret )
				{
					aptus->facio(&local_pius);
				}
			} else {	//通道测试失败
				wk_status = WS_Idle;
			}
			break;

		default:
			break;
		}
		break;

	case Notitia::DMD_END_SESSION:  /* 强制关闭 */
		WBUG("facio DMD_END_SESSION");
		wk_status = WS_Idle;
		break;

	default:
		return false;
	}
	return true;
}

bool MonWay::test_chan_res() //通道测试响应
{
	TiXmlElement root( "ICTrans" );
	unsigned char *val;
	int len;

	val = un_pac.getfld(gCFG->business_fldno, &len);
	if ( !val ) goto TEnd;
	if ( len != gCFG->tst_len ) goto TEnd;
	if ( memcmp(val, gCFG->tst_code, tst_len) != 0 ) goto TEnd;

	val = un_pac.getfld(gCFG->err_fldno, &len);
	if ( !val ) goto TEnd;
	if ( len != 2 ) goto TEnd;
	if ( memcmp(val, "00", len) != 0 ) goto TEnd;

	return true;
TEnd:
	root.SetAttribute( fld_string[gCFG->err_fldno], "A1");
	root.SetAttribute( fld_string[gCFG->msg_fldno], "Channel not found.");
	res_doc->InsertEndChild(root);
	return false;
}

bool MonWay::test_chan_ask() //通道测试请求
{
	TiXmlElement root( "ICTrans" );
	TiXmlElement *reqele = req_doc->RootElement();

	if (!reqele 
		|| strcmp (reqele->Value(), "ICTrans") != 0
	{
		root.SetAttribute( fld_string[gCFG->err_fldno], "A0");
		root.SetAttribute( fld_string[gCFG->msg_fldno], "no request xml data.");
		goto MonEnd;
	}

	do_pac.reset();
	do_pac.input(gCFG->business_fldno, gCFG->tst_code, gCFG->tst_len);
	do_pac.input(gCFG->channelID_fldno, reqele->Attribute(gCFG->chan_idstr));
	
	return true;
MonEnd:
	res_doc->InsertEndChild(root);
	return false;
}

bool MonWay::handle2() //业务请求
{
	TiXmlElement root( "ICTrans" );
	TiXmlElement *reqele = req_doc->RootElement();

	if (!reqele 
		|| strcmp (reqele->Value(), "ICTrans") != 0
	{
		root.SetAttribute( fld_string[gCFG->err_fldno], "A0");
		root.SetAttribute( fld_string[gCFG->msg_fldno], "no request xml data.");
		goto MonEnd;
	}
	do_pac.reset();
	for ( i = 0; i < FIELD_NUM; i++ )
	{
		const char *fld_val;
		fld_val = reqele->Attribute(fld_string[i]);
		if ( fld_val)
			do_pac.input(i, fld_val);
	}
	return true;

MonEnd:
	res_doc->InsertEndChild(root);
	return false;
}

bool MonWay::handle3()	//业务响应
{
	TiXmlElement root( "ICTrans" );
	int i;
	char buf[256];

	for ( i = 0; i < FIELD_NUM; i++ )
	{
		unsigned char *val;
		int len;
		val = un_pac.getfld(i, &len);
		if ( val )
		{
			if (val[len] != 0x0 ) //不是null结尾的
			{
				if ( len > 250 ) len =250;
				memcpy(buf, val, len);
				buf[len]=0;
				root.SetAttribute( fld_string[i], (char*)buf);
			} else {	//以null结尾的
				root.SetAttribute( fld_string[i], (char*)val);
			}
		}
	}
	res_doc->InsertEndChild(root);
	return true;
}

/* 向接力者提交 */
inline void MonWay::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	
	switch (aordo)
	{
		case Notitia::SET_UNIPAC:
			WBUG("deliver SET_UNIPAC");
			tmp_pius.indic = &pa[0];
			break;

		default:
			WBUG("deliver Notitia::%d",aordo);
			break;
	}
	aptus->facio(&tmp_pius);
	return ;
}
#include "hook.c"
