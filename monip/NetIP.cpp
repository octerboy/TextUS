/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title:Internet Protocol Head pro
 Build: created by octerboy, 2007/08/02, Panyu
 $Header: /textus/monip/NetIP.cpp 3     07-08-15 0:23 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: NetIP.cpp $"
#define TEXTUS_MODTIME  "$Date: 07-08-15 0:23 $"
#define TEXTUS_BUILDNO  "$Revision: 3 $"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "BTool.h"
#include "casecmp.h"
#include "textus_string.h"
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

#define TINLINE inline

class NetIP: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	NetIP();
	~NetIP();


#include "PacData.h"
private:
	Amor::Pius pro_unipac;

	PacketObj *first_pac;	/* 来自左(右)节点的PacketObj, 同时向右(左)传递 */
	PacketObj *second_pac;

	struct G_CFG {
		int version_fld;	/* IP版本域号 */
		int total_fld;		/* IP总长度的域号 */
		int flags_fld;		/* fragment 域号 */
		int frag_off_fld;	/* fragment offset域号 */
		
		int data_fld;		/* 数据域号， 包括option， 经过本模块处理后，这个域将不包括option*/
		int option_fld;		/* option域号, 如果IP头中有option，处理后，该域有这个数据 */

		inline G_CFG(TiXmlElement *cfg) {
			version_fld = -1;	
			total_fld = -1;	
                        flags_fld  = -1;		
			frag_off_fld = -1;	
			                    			                    
			data_fld  = -1;					                    
			option_fld  = -1;
			
			cfg->QueryIntAttribute("version", &(version_fld));
			cfg->QueryIntAttribute("length", &(total_fld));
			cfg->QueryIntAttribute("flags", &(flags_fld));
			cfg->QueryIntAttribute("fragment_offset", &(frag_off_fld));
			cfg->QueryIntAttribute("data", &(data_fld));
			cfg->QueryIntAttribute("option", &(option_fld));
		};

		inline ~G_CFG() {
		};
	};
	struct G_CFG *gCFG;     /* 全局共享参数 */
	bool has_config;

	TINLINE bool handle(PacketObj *rpac);

	#include "wlog.h"
};

#include <assert.h>

void NetIP::ignite(TiXmlElement *prop)
{
	if (!prop) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(prop);
		has_config = true;
	}
}


NetIP::NetIP()
{
	first_pac = 0;
	second_pac = 0;

	pro_unipac.ordo = Notitia::PRO_UNIPAC;
	pro_unipac.indic = 0;

	gCFG = 0;
	has_config = false;
}

NetIP::~NetIP() 
{
	if ( has_config  )
	{	
		if(gCFG) delete gCFG;
	}
}

Amor* NetIP::clone()
{
	NetIP *child = new NetIP();
	child->gCFG = gCFG;
	return (Amor*) child;
}

bool NetIP::facio( Amor::Pius *pius)
{
	PacketObj **tmp;
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::PRO_UNIPAC:
		WBUG("facio PRO_UNIPAC");
		if ( handle(first_pac) )
			aptus->facio(&pro_unipac);
		break;

	case Notitia::SET_UNIPAC:
		WBUG("facio SET_UNIPAC");
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) first_pac = *tmp; 
			else
				WLOG(WARNING, "facio SET_UNIPAC rcv_pac null");
			tmp++;
			if ( *tmp) second_pac = *tmp;
			else
				WLOG(WARNING, "facio SET_UNIPAC snd_pac null");
		} else 
			WLOG(WARNING, "facio SET_UNIPAC null");
		
		aptus->facio(pius);
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
		break;

	default:
		return false;
	}
	return true;
}

bool NetIP::sponte( Amor::Pius *pius)
{
	PacketObj **tmp;
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::PRO_UNIPAC:
		WBUG("sponte PRO_UNIPAC");
		if ( handle(second_pac) )
			aptus->sponte(&pro_unipac);
		break;

	case Notitia::SET_UNIPAC:
		WBUG("sponte SET_UNIPAC");

		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) first_pac = *tmp; 
			else
				WLOG(WARNING, "sponte SET_UNIPAC rcv_pac null");
			tmp++;
			if ( *tmp) second_pac = *tmp;
			else
				WLOG(WARNING, "sponte SET_UNIPAC snd_pac null");

			aptus->sponte(pius);
		} else 
			WLOG(WARNING, "sponte SET_UNIPAC null");

		break;
	default:
		return false;
	}
	return true;
}

TINLINE bool NetIP::handle(PacketObj *rpac)
{
	unsigned char ver;
	unsigned short head_len, total_len;
	int opt_len;
	FieldObj *fld_tmp, *fld_d;

	assert(rpac);
	assert(gCFG->version_fld >=0 );
	assert(gCFG->data_fld >=0 );
	assert(gCFG->total_fld >=0 );
	if ( rpac->max < gCFG->version_fld || rpac->max < gCFG->data_fld )
		return false;

	if ( rpac->max < gCFG->total_fld )
		return false;

	/* total_len: IP包总长 */
	fld_tmp = &rpac->fld[gCFG->total_fld];
	total_len = (fld_tmp->val[0]) & 0xff;
	total_len <<= 8;
	total_len += (fld_tmp->val[1]) & 0xff;
	fld_d = &rpac->fld[gCFG->data_fld];

	memcpy(&ver, rpac->fld[gCFG->version_fld].val, 1);
	if ( (ver & 0xf0 ) != 0x40 )	/* 只接受 IPv4 */
	{
		WLOG(WARNING, "the ip version is not IPv4.");
		return false;
	}

	head_len = ( ver & 0x0f ) << 2;
	
	opt_len = head_len - 20;	/* 这便是IP option的长度 */
	if ( opt_len < 0 )	/* IP头错误 */
	{
		WLOG(WARNING, "the ip head < 20 bytes.");
		return false;
	}

	if ( fld_d->range + head_len < (unsigned int)(total_len + opt_len) )	/* 没有收到完整的IP包 */
	{
		WLOG(WARNING, "the ip packet is not complete.");
		return false;
	}

	if ( head_len > 20 && gCFG->option_fld >= 0 )
	{
		FieldObj &fld_o = rpac->fld[gCFG->option_fld];

		fld_o.val = fld_d->val;
		fld_o.range = opt_len;

		fld_o.raw = fld_d->raw;
		fld_o._rlen = opt_len;
		fld_o.no = gCFG->option_fld;
		fld_o.len = head_len;
		
		fld_d->val += opt_len;
		fld_d->raw = fld_d->val;
		fld_d->range -= opt_len;
		fld_d->_rlen = fld_d->range;
		fld_d->len = fld_d->range;
	}

	fld_d->range = (total_len - head_len);	/* 除去 尾加的数据*/
	fld_d->_rlen = fld_d->range;
	fld_d->len = fld_d->range;

	return true;
}

#include "hook.c"
