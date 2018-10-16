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
#include "BTool.h"
#define NOT_LOAD_XML 1
#include "WayData.h"

/* ���״̬, ����, ����������, ���׽����� */
enum LEFT_STATUS { LT_Idle = 0, LT_Working = 3};
class PacTran: public Amor {
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	void handle_pac();	//���״̬����
	bool sponte( Pius *);
	Amor *clone();

	PacTran();
	~PacTran();

private:
	enum LEFT_STATUS left_status;	
	struct G_CFG { 	//ȫ�ֶ���
		int flowID_fld_no;	//����ʶ��, ҵ�������, 

		inline G_CFG() {
			flowID_fld_no = 3;
		};	
	};

	PacketObj *rcv_pac;	/* ������ڵ��PacketObj */
	PacketObj *snd_pac;
	Amor::Pius loc_pro_pac, loc_pro_tran;

	struct G_CFG *gCFG;     /* ȫ�ֹ������� */
	bool has_config;
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

	case Notitia::PRO_UNIPAC:    /* �����Կ���̨������ */
		WBUG("facio PRO_UNIPAC");
		if ( left_status ==  LT_Idle )
		{
			fl.flow_str=rcv_pac->getfld(gCFG->flowID_fld_no, (unsigned long*)&fl.len);		//ȡ��ҵ�����, ������ʶ
			if ( !fl.flow_str) 
			{
				WBUG("business code field null");
			} else {
				loc_pro_tran.indic = &fl;
				aptus->facio(&loc_pro_tran);
			}
		} else if ( left_status ==  LT_Working) { //������, ����Ӧ����
			WLOG(WARNING,"still working!");
		}
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
		break;

	case Notitia::START_SESSION:
		WBUG("facio START_SESSION" );
		left_status = LT_Idle;	
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION" );
		left_status = LT_Idle;	
		break;

	default:
		return false;
	}
	return true;
}

bool PacTran::sponte( Amor::Pius *pius) 
{
	PacketObj **tmp;
	assert(pius);
	if (!gCFG ) return false;

	switch ( pius->ordo ) {
	case Notitia::Ans_TranWay:
		WBUG("sponte Ans_TranWay");
		left_status = LT_Idle;	
		aptus->sponte(&loc_pro_pac);
		break;

	default:
		return false;
		break;
	}
	return true;
}

#include "hook.c"