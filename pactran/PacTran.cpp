/* Copyright (c) 2016-2018 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 Title:PacTran
 Build: created by octerboy, 2016/08/04 Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "PacData.h"
#include "casecmp.h"
#include "textus_string.h"
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
//#include "BTool.h"
#include "Describo.h"
#define NOT_LOAD_XML 1
#include "WayData.h"

/* 左边状态, 空闲, 等着新请求, 交易进行中 */
enum LEFT_STATUS { LT_Idle = 0, LT_Working = 3};
class PacTran: public Amor {
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	void handle_pac();	//左边状态处理
	bool sponte( Pius *);
	Amor *clone();

	PacTran();
	~PacTran();

private:
	enum LEFT_STATUS left_status;	
	struct G_CFG { 	//全局定义
		int flowID_fld_no;	//流标识域, 业务代码域, 
		Amor *sch;

		inline G_CFG() {
			flowID_fld_no = 3;
			sch = 0;
		};	
	};

	PacketObj *rcv_pac;	/* 来自左节点的PacketObj */
	PacketObj *snd_pac;
	Amor::Pius loc_pro_pac, loc_pro_tran;

	struct G_CFG *gCFG;     /* 全局共享参数 */
	bool has_config;
	int holding_back;
	void get_sch();
	void put_sch();
	#include "wlog.h"
};

void PacTran::ignite(TiXmlElement *prop) {
	const char *comm_str;
	if (!prop) return;
	if ( !gCFG ) {
		gCFG = new struct G_CFG();
	}

	if ( (comm_str = prop->Attribute("flow_fld")) )
		gCFG->flowID_fld_no = atoi(comm_str);

	return;
}

PacTran::PacTran() 
{
	gCFG = 0;
	has_config = false;
	loc_pro_pac.ordo = Notitia::PRO_UNIPAC;
	loc_pro_pac.indic = 0;
	loc_pro_pac.subor = Amor::CAN_ALL;
	loc_pro_tran.ordo = Notitia::Pro_TranWay;
	loc_pro_tran.indic = 0;
	loc_pro_tran.subor = Amor::CAN_ALL;
	left_status = LT_Idle;	
}

PacTran::~PacTran() 
{
	if (has_config) 
	{	
		if(gCFG) delete gCFG;
		gCFG = 0;
	}
}

Amor* PacTran::clone() 
{
	PacTran *child = new PacTran();
	child->gCFG = gCFG;
	return (Amor*) child;
}

bool PacTran::facio( Amor::Pius *pius) 
{
	struct FlowStr fl;
	PacketObj **tmp;
	Amor::Pius tmp_pius;

	switch ( pius->ordo ) 
	{
	case Notitia::SET_UNIPAC:
		if ( (tmp = (PacketObj **)(pius->indic))) {
			if ( *tmp) rcv_pac = *tmp; 
			else {
				WBUG("facio SET_UNIPAC rcv_pac null");
			}
			tmp++;
			if ( *tmp) snd_pac = *tmp;
			else {
				WBUG("facio SET_UNIPAC snd_pac null");
			}

			WBUG("facio SET_UNIPAC rcv(%p) snd(%p)", rcv_pac, snd_pac);
		} else {
			WBUG("facio SET_TBUF null");
		}
		break;

	case Notitia::PRO_UNIPAC:    /* 有来自控制台的请求 */
		WBUG("facio PRO_UNIPAC");
		if ( left_status ==  LT_Idle )
		{
			fl.flow_str=rcv_pac->getfld(gCFG->flowID_fld_no, (unsigned long*)&fl.len);		//取得业务代码, 即流标识
			if ( !fl.flow_str) 
			{
				WBUG("business code field null");
			} else {
				left_status =  LT_Working;	
				loc_pro_tran.indic = &fl;
				aptus->facio(&loc_pro_tran);
			}
		} else if ( left_status ==  LT_Working) { //不接受, 不响应即可
			WLOG(WARNING,"still working!");
		}
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		get_sch();
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
		break;

	case Notitia::START_SESSION:
		WBUG("facio START_SESSION" );
		left_status = LT_Idle;	
		holding_back = 0;
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION" );
		left_status = LT_Idle;	
		holding_back = 0;
		break;

	case Notitia::DMD_SCHED_RUN:
		WBUG("facio DMD_SCHED_RUN");
#define POR ((struct Describo::Pendor*)pius->indic)
		this->facio(POR->pius);
		break;
	
	default:
	//	printf("ordo %d\n",  pius->ordo);
		return false;
	}
	return true;
}

void PacTran::get_sch()
{
	Amor::Pius get;
	get.ordo = Notitia::CMD_GET_SCHED;
	get.indic = 0;
	aptus->sponte(&get);
	gCFG->sch = (Amor*)get.indic;
	if ( !gCFG->sch ) 
	{
		WLOG(ERR, "no sched or tpoll");
	}
}
void PacTran::put_sch()
{
	Amor::Pius put;
	struct Describo::Pendor por;
	put.ordo = Notitia::CMD_PUT_PENDOR;
	put.indic = &por;
	por.pupa = this;
	por.dir = 0;
	por.from = 0;
	por.pius = &loc_pro_pac;
	if ( gCFG->sch)
		gCFG->sch->sponte(&put);
}
bool PacTran::sponte( Amor::Pius *pius) 
{
	assert(pius);
	struct FlowStr fl;

	switch ( pius->ordo ) {
	case Notitia::Ans_TranWay:
		WBUG("sponte Ans_TranWay");
		if ( pius->indic ) 	/* indic will be null when just sponte and not end the trans */
		{
			left_status = LT_Idle;	
			if ( holding_back > 0 ) 
			{
				holding_back--;
				put_sch();	//wait tpoll (sched) to call this->facio()
				break;		//will not send answer unipac
			}
		}
		aptus->sponte(&loc_pro_pac);
		break;

	case Notitia::Re_TranWay:
		WBUG("sponte Re_TranWay");
		holding_back++;
		break;

	default:
		return false;
		break;
	}
	return true;
}

#include "hook.c"
