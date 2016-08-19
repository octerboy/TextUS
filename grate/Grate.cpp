/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
/**
 Title: HTTP Agent
 Build: created by octerboy, 2006/09/13, Guangzhou
 $Header: /textus/grate/Grate.cpp 2     07-08-02 21:22 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: Grate.cpp $"
#define TEXTUS_MODTIME  "$Date: 07-08-02 21:22 $"
#define TEXTUS_BUILDNO  "$Revision: 2 $"
/* $NoKeywords: $ */

#include "Notitia.h"
#include "TBuffer.h"
#include "BTool.h"
#include "casecmp.h"
#include "PacData.h"
#include "textus_string.h"
#include "Amor.h"
#include <time.h>
#include <ctype.h>
#include <stdarg.h>

#ifndef TINLINE
#define TINLINE inline
#endif

class Grate: public Amor
{
public:
	int count;
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	Grate();
	~Grate();

private:
	PacketObj *rcv_pac;	/* 来自左节点的PacketObj */
	PacketObj *snd_pac;

	TBuffer house;		/* 暂存 */
	TBuffer right_rcv;	/* 从右节点接收的数据缓冲 */
	TBuffer right_snd;	/* 向右节点发送的数据缓冲 */

	TINLINE void deliver(Notitia::HERE_ORDO aordo);
	TINLINE void right_reset();
	Amor::Pius pro_tbuf;

	struct G_CFG {
		int pac_fld;	/* PacketObj中那个域进入本缓冲 */
		int flag_fld;	/* PacketObj中哪个域作为判断最后标志 */
		unsigned char *flag;
		unsigned char f_len;
		
		inline G_CFG ( TiXmlElement *cfg ) {
			const char *comm_str;
			TiXmlElement *dat;
			pac_fld = -1;
			flag_fld = -1;

			cfg->QueryIntAttribute("field", &(pac_fld));
			cfg->QueryIntAttribute("flag", &(flag_fld));

			dat = cfg->FirstChildElement("match");
			comm_str = 0;
			if ( dat )
				comm_str = dat->GetText();
			if ( comm_str )
			{
				flag = new unsigned char[ strlen(comm_str) +1 ];
				f_len = BTool::unescape(comm_str, flag);
			}

		};
	};
	struct G_CFG *gCFG;	/* Shared for all objects in this node */
	bool has_config;

#include "wlog.h"
};

#include <assert.h>

void Grate::ignite(TiXmlElement *cfg) 
{
	if (!cfg) return;

	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
}

bool Grate::facio( Amor::Pius *pius)
{
	PacketObj **tmp;

	struct FieldObj *fld;
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::PRO_UNIPAC:
		WBUG("facio PRO_UNIPAC");
		assert(gCFG->pac_fld >= 0);
		assert(rcv_pac != (PacketObj *)0 );
		
		if ( rcv_pac->max >= gCFG->pac_fld ) 
		{
			fld = &rcv_pac->fld[gCFG->pac_fld];
			if (fld->no == gCFG->pac_fld )	/* 这个域有内容 */
			{
				if ( gCFG->flag_fld >= 0 )	/* 需要一个域作为标志, 以判断是否完成将多次域组成 TBuffer */
					house.input(fld->val, fld->range); /* 数据进入暂存 */
				else /* 没有这样一个域作为判断标志 */
				{
					right_snd.input(fld->val, fld->range); /* 数据直接进入右TBuffer */
					aptus->facio(&pro_tbuf);
					break;
				}
			}
		}

		if ( gCFG->flag_fld >= 0 && rcv_pac->max >= gCFG->flag_fld ) 
		{
			fld = &rcv_pac->fld[gCFG->flag_fld];
			if (fld->no == gCFG->flag_fld )	/* 这个域有内容 */
			{
				if ( fld->range == gCFG->f_len  
					&& memcmp(gCFG->flag, fld->val, gCFG->f_len) == 0 )
				{	/* 有结束标志了 */
					TBuffer::pour(right_snd, house);
					aptus->facio(&pro_tbuf);
				}
			}
		}

		break;

	case Notitia::SET_UNIPAC:
		WBUG("facio SET_UNIPAC");
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) rcv_pac = *tmp; 
			else
				WLOG(WARNING, "facio SET_UNIPAC rcv_pac null");
			tmp++;
			if ( *tmp) snd_pac = *tmp;
			else
				WLOG(WARNING, "facio SET_UNIPAC snd_pac null");
		} else 
			WLOG(WARNING, "facio SET_UNIPAC null");
		
		break;


	case Notitia::DMD_END_SESSION:	/* channel to closed */
		WBUG("facio DMD_END_SESSION");
		house.reset();	/* 仅仅是把已经收到的数据清空, 右节点不作处理 */
		break;

	case Notitia::IGNITE_ALL_READY:	
		WBUG("facio IGNITE_ALL_READY" );			
		goto ALL_READY;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE(IGNITE)_ALL_READY" );			
ALL_READY:
		deliver(Notitia::SET_TBUF);
		break;

	default:
		return false;
	}
	return true;
}

bool Grate::sponte( Amor::Pius *pius)
{
	assert(pius);
	
	switch ( pius->ordo )
	{
	case Notitia::DMD_END_SESSION:	/* channel closed */
		WBUG("sponte DMD_END_SESSION");
		right_reset();
		break;

	case Notitia::PRO_TBUF:
		WBUG("sponte PRO_TBUF");
		assert( gCFG->pac_fld >= 0 );
		assert( snd_pac != (PacketObj *)0 );
		snd_pac->input(gCFG->pac_fld, right_rcv.base, right_rcv.point - right_rcv.base);
		
		break;


	default:
		return false;
	}
	return true;
}

Grate::Grate():right_rcv(8192), right_snd(8192)
{
	rcv_pac = 0;
	snd_pac = 0;

	house.reset();
	right_reset();

	pro_tbuf.indic = 0;
	pro_tbuf.ordo = Notitia::PRO_TBUF;

	gCFG = 0;
	has_config = false ;
}

Grate::~Grate() 
{
	if ( has_config && gCFG)
		delete gCFG;
}

Amor* Grate::clone()
{
	Grate *child;
	child = new Grate();
	child->gCFG = gCFG;

	return (Amor*)child;
}

TINLINE void Grate::right_reset()
{
	right_rcv.reset();
	right_snd.reset();
}

/* 向接力者提交 */
TINLINE void Grate::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	TBuffer *tb[3];
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	
	switch (aordo)
	{
	case Notitia::SET_TBUF:
		WBUG("deliver SET_TBUF");
		tb[0] = &right_snd;
		tb[1] = &right_rcv;
		tb[2] = 0;
		tmp_pius.indic = &tb[0];
		break;

	default:
		WBUG("deliver Notitia::%d",aordo);
		break;
	}
	aptus->facio(&tmp_pius);
	return ;
}

#include "hook.c"
