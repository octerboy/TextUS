/* Copyright (c) 2016-2018 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 Title:PacIns
 Build: created by octerboy, 2018/03/14 Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "PacData.h"
#include "BTool.h"
#include "casecmp.h"
#include "textus_string.h"
#include "DBFace.h"
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#if defined(__APPLE__)
#define COMMON_DIGEST_FOR_OPENSSL
#include <CommonCrypto/CommonDigest.h>
#else
#include <openssl/md5.h>
#endif
#include "WayData.h"

/* 报文交换方式 */
enum PAC_MODE { PAC_NONE = 0, PAC_FIRST =1, PAC_SECOND = 4, PAC_BOTH=5};
/* 报文日志方式 */
enum PAC_LOG { PAC_LOG_NONE = 0, PAC_LOG_STR =0x11, PAC_LOG_HEX =0x12, PAC_LOG_BOTH = 0x13, PAC_LOG_STR_ERR = 0x21, PAC_LOG_HEX_ERR = 0x22, PAC_LOG_BOTH_ERR = 0x23};

/* 命令分几种，INS_Normal：标准， */
enum PacIns_Type { INS_None = 0, INS_Normal=1,  INS_SetPeer=3, INS_GetPeer=4, INS_Get_CertNo=5, INS_Pro_DBFace=6, INS_Cmd_Ordo=7};
enum ACT_DIR { FACIO=0, SPONTE=1 };

void get_req_pac( PacketObj *req_pac, struct DyVarBase **psnap, struct InsData *insd)
{
	int i,j;
	unsigned long t_len;
	t_len = 0;

	for ( i = 0 ; i < insd->snd_num; i++ ) 
	{
		if ( insd->snd_lst[i].dy_num ==0 ) 
			t_len += insd->snd_lst[i].cmd_len;	//这是在pacdef中send元素定义的
		 else {
			for ( j = 0; j < insd->snd_lst[i].dy_num; j++ )
			{
				if (  insd->snd_lst[i].dy_list[j].dy_pos < 0 ) 
					t_len += insd->snd_lst[i].dy_list[j].len;
				 else 
					t_len += psnap[insd->snd_lst[i].dy_list[j].dy_pos]->c_len;
			}
		}
	}
	req_pac->grant(t_len);
	for ( i = 0 ; i < insd->snd_num; i++ ) 
	{
		t_len = 0;
		if ( insd->snd_lst[i].dy_num ==0 )
		{	/* 这是在pacdef中send元素定义的一个域的内容 */
			memcpy(req_pac->buf.point, insd->snd_lst[i].cmd_buf, insd->snd_lst[i].cmd_len);
			t_len = insd->snd_lst[i].cmd_len;
		} else {
			for ( j = 0; j < insd->snd_lst[i].dy_num; j++ )
			{
				/* 由一个列表来构成一个域的内容 */
				if (  insd->snd_lst[i].dy_list[j].dy_pos < 0 ) 
				{
					/* 静态内容*/
					memcpy(&req_pac->buf.point[t_len], insd->snd_lst[i].dy_list[j].con, insd->snd_lst[i].dy_list[j].len);
					t_len += insd->snd_lst[i].dy_list[j].len;
				} else {
					/* 动态内容*/
					memcpy(&req_pac->buf.point[t_len], psnap[insd->snd_lst[i].dy_list[j].dy_pos]->val_p, psnap[insd->snd_lst[i].dy_list[j].dy_pos]->c_len);
					t_len += psnap[insd->snd_lst[i].dy_list[j].dy_pos]->c_len;
				}
			}
		}
		req_pac->commit(insd->snd_lst[i].fld_no, t_len);	//域的确认
	}
	return ;
};

struct PacInsData {
	PacIns_Type type;
	int subor;	//指示指令报文送给哪一个下级模块

	enum ACT_DIR fac_spo;	//动作方向
	TEXTUS_ORDO ordo;	//其它动作
	const char *dbface_name;
	DBFace *dbface;

	enum PAC_MODE pac_mode;	/* 报文模式, 一般不交换 */
	bool pac_cross;	/* 是否交叉, 如第一个换到第二个 */
	enum PAC_LOG pac_log;

	PacInsData() 
	{
		type = INS_None;
		subor = 0;
		fac_spo = SPONTE;
		pac_mode = PAC_NONE;
		pac_cross = false;
		pac_log = PAC_LOG_NONE;
		dbface_name = 0;
		dbface = 0;
		ordo = Notitia::TEXTUS_RESERVED;
	};

	void set_def(TiXmlElement *def_ele, struct InsData *insd) 
	{
		static int i=0;
		const char *p;
		char *q;
		if ( !insd->err_code ) 
			insd->err_code = def_ele->Attribute("error");
		if ( !insd->err_code ) 
			insd->err_code = def_ele->GetDocument()->RootElement()->Attribute("error"); //取整个文档的根元素定义，如"unknown error"
		if ( (p = def_ele->Attribute("counted")) && ( *p == 'y' || *p == 'Y') )
			insd->counted = true;
		else 
			insd->counted = false;

		if ((p = def_ele->Attribute("function")) && ( *p == 'y' || *p == 'Y') )	//这是函数扩展，出现回调
			insd->isFunction = true;
		else
			insd->isFunction = false;

		if ( !insd->log_str ) insd->log_str = def_ele->Attribute("log");
		p = insd->log_str;
		if ( p ) 
		{
			if ( strcasecmp( p, "str") ==0 )
				pac_log = PAC_LOG_STR;
			else if ( strcasecmp( p, "estr") ==0 )
				pac_log = PAC_LOG_STR_ERR;
			else if ( strcasecmp( p, "hex") ==0 )
				pac_log = PAC_LOG_HEX;
			else if ( strcasecmp( p, "ehex") ==0 )
				pac_log = PAC_LOG_HEX_ERR;
			else if ( strcasecmp( p, "both") ==0 )
				pac_log = PAC_LOG_BOTH;
			else if ( strcasecmp( p, "eboth") ==0 )
				pac_log = PAC_LOG_BOTH_ERR;
		}
	
		p = def_ele->Attribute("dir");	
		if ( p ) 
		{
			if ( strcasecmp( p, "facio") ==0 )
				fac_spo = FACIO;
			else if ( strcasecmp( p, "sponte") ==0 )
				fac_spo = SPONTE;
		}
	
		p = def_ele->Attribute("type");	
		if ( !p ) 
		{
			type = INS_Normal;
		} else if ( strcasecmp( p, "GetPeer") ==0 )
		{
			type =  INS_GetPeer;
			fac_spo = SPONTE;
		} else if ( strcasecmp( p, "SetPeer") ==0 )
		{
			type =  INS_SetPeer;
			fac_spo = FACIO;
		} else if ( strcasecmp( p, "GetCertNo") ==0 )
		{
			type =  INS_Get_CertNo;
			fac_spo = SPONTE;
		} else if ( strcasecmp( p, "ProDB") ==0 )
		{
			type =  INS_Pro_DBFace;
			fac_spo = FACIO;
			dbface_name = def_ele->Attribute("dbface");
		} else if ( (p = def_ele->Attribute("ordo")) )
		{
			type =  INS_Cmd_Ordo;
			fac_spo = FACIO;
			ordo = Notitia::get_ordo(p);
		}
	
		subor=Amor::CAN_ALL; 
		def_ele->QueryIntAttribute("subor", &subor);
		if ( subor < Amor::CAN_ALL ) 
			insd->isFunction = true;
	
		if ( (p = def_ele->Attribute("exchange")) )
		{
			size_t len;
			q = (char*)strpbrk(p, ":" );
			if (q) {
				len = q-p;
				q++;
			} else {
				len = strlen(p);
			}
			if (strncasecmp ( p, "first", len) == 0 ) 
				pac_mode = PAC_FIRST;
			else if ( strncasecmp (p, "second", len) == 0 )
				pac_mode = PAC_SECOND;
			else if ( strncasecmp (p, "both", len) ==0 ) 
				pac_mode = PAC_BOTH;
	
			if ( q ) {
				if (strcasecmp ( q, "X") == 0 ) 
					pac_cross = true;
			}
		}
	};

	PacketObj *pac_cross_after(PacketObj *req_pac, PacketObj *rply_pac, PacketObj *first_pac, PacketObj *second_pac)
	{
		PacketObj *n_pac = rply_pac;
		if ( pac_cross) {
			if ( pac_mode == PAC_SECOND || pac_mode == PAC_BOTH )
				second_pac->exchange(req_pac);
	
			if ( pac_mode == PAC_FIRST || pac_mode == PAC_BOTH ) 
			{
				first_pac->exchange(rply_pac);
				n_pac = first_pac;
			}
		} else {
			if ( pac_mode == PAC_SECOND || pac_mode == PAC_BOTH ) 
			{
				second_pac->exchange(rply_pac);
				n_pac = second_pac;
			}
			if ( pac_mode == PAC_FIRST || pac_mode == PAC_BOTH )
				first_pac->exchange(req_pac);
		}
		return n_pac;
	};
	
	void pac_cross_before(PacketObj *req_pac, PacketObj *rply_pac, PacketObj *first_pac, PacketObj *second_pac)
	{
		if ( pac_cross) {
			if ( pac_mode == PAC_SECOND || pac_mode == PAC_BOTH )
				second_pac->exchange(req_pac);
			if ( pac_mode == PAC_FIRST || pac_mode == PAC_BOTH )
				first_pac->exchange(rply_pac);
		} else {
			if ( pac_mode == PAC_FIRST || pac_mode == PAC_BOTH)
				first_pac->exchange(req_pac);
			if ( pac_mode == PAC_SECOND || pac_mode == PAC_BOTH )
				second_pac->exchange(rply_pac);
		}
	};
};

class PacIns: public Amor {
public:
void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	PacIns();
	~PacIns();

private:
	char my_err_str[1024];
	struct InsWay *cur_insway;
	void load_pac_def();
	void log_ins ();
	struct G_CFG { 	//全局定义
		TiXmlDocument doc_pac_def;	//pacdef：报文定义
		TiXmlElement *pac_def_root;
		TiXmlElement *prop;

		int maxium_fldno;		/* 最大域号 */
		int subor;      //指示指令报文送给哪一个下级模块

		inline G_CFG() {
			maxium_fldno = 64;
			subor = Amor::CAN_ALL;
		};	
	};

	PacketObj hi_req, hi_reply; /* 向右传递的  */
	PacketObj *hi_req_p, *hi_reply_p, *last_ans_pac; /* 向右传递的 */
	PacketObj *hipa[3];
	PacketObj *rcv_pac, *snd_pac; //左节点的
	Amor::Pius loc_pro_pac, prodb_ps, other_ps,ans_ins_ps;

	struct G_CFG *gCFG;     /* 全局共享参数 */
	bool has_config;
	void set_peer(PacketObj *pac, int sub);
	void get_cert(PacketObj *pac, int sub);
	void get_peer(PacketObj *pac, int sub);

	void set_ins(struct InsData *insd);
	void pro_ins();
	void ans_ins(bool should_spo);

	void log_pac(PacketObj *pac,const char *prompt, enum PAC_LOG mode);
	const char* pro_rply_pac(PacketObj *rply_pac, struct DyVarBase **psnap, struct InsData *insd);
	#include "wlog.h"
};

void PacIns::load_pac_def()
{
	const char *nm =0;
	if ( !( gCFG->pac_def_root = gCFG->prop->FirstChildElement("Pac")))	
	{
		if ( (nm = gCFG->prop->Attribute("pac"))) 
		{
			if (load_xml(nm,  gCFG->doc_pac_def,   gCFG->pac_def_root, gCFG->prop->Attribute("pac_md5"), my_err_str))
			{
				WLOG(WARNING, "%s", my_err_str);	
			}
		} else {
			WLOG(WARNING, "no packet definition file!");
		}		
	}
};

void PacIns::ignite(TiXmlElement *prop) 
{
	const char *comm_str, *ret;
	if (!prop) return;
	if ( !gCFG ) {
		gCFG = new struct G_CFG();
		gCFG->prop = prop;
		has_config = true;
	}

	if ( (comm_str = prop->Attribute("max_fld")) )
		gCFG->maxium_fldno = atoi(comm_str);

	if ( (comm_str = prop->Attribute("my_subor")) )
		gCFG->subor = atoi(comm_str);

	if ( gCFG->maxium_fldno <= 0 )
		gCFG->maxium_fldno = 16;
	hi_req.produce(gCFG->maxium_fldno) ;
	hi_reply.produce(gCFG->maxium_fldno) ;
	return;
}

PacIns::PacIns() {
	hipa[0] = &hi_req;
	hipa[1] = &hi_reply;
	hipa[2] = 0;
	hi_req_p = &hi_req;
	hi_reply_p = &hi_reply;

	gCFG = 0;
	has_config = false;
	loc_pro_pac.ordo = Notitia::PRO_UNIPAC;
	loc_pro_pac.indic = 0;
	loc_pro_pac.subor = Amor::CAN_ALL;

	prodb_ps.ordo =  Notitia::PRO_DBFACE;
	prodb_ps.indic = 0;
	prodb_ps.subor = Amor::CAN_ALL;

	ans_ins_ps.ordo =  Notitia::Ans_InsWay;
	ans_ins_ps.indic = 0;
	ans_ins_ps.subor = Amor::CAN_ALL;
}

PacIns::~PacIns() {
	if ( has_config  ) {	
		if(gCFG) delete gCFG;
		gCFG = 0;
	}
}

Amor* PacIns::clone() {
	PacIns *child = new PacIns();
	child->gCFG = gCFG;
	child->hi_req.produce(hi_req.max);
	child->hi_reply.produce(hi_reply.max);
	return (Amor*) child;
}

bool PacIns::facio( Amor::Pius *pius) {
	PacketObj **tmp;
	Amor::Pius tmp_pius;

	switch ( pius->ordo ) {
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
		break;

	case Notitia::Set_InsWay:    /* 设置 */
		WBUG("facio Set_InsWay, InsData %p, tag %s", pius->indic, ((struct InsData*)pius->indic)->ins_tag);
		set_ins((struct InsData*)pius->indic);
		break;

	case Notitia::Pro_InsWay:    /* 处理 */
		WBUG("facio Pro_InsWay, tag %s", ((struct InsWay*)pius->indic)->dat->ins_tag);
		cur_insway = (struct InsWay*)pius->indic;
		pro_ins();
		break;

	case Notitia::Log_InsWay:    /* 记录报文, 往往是在错误情况 */
		WBUG("facio Log_InsWay");
		log_ins();
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		load_pac_def();
		tmp_pius.ordo = Notitia::SET_UNIPAC;
		tmp_pius.indic = &hipa[0];
		aptus->facio(&tmp_pius);
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
		tmp_pius.ordo = Notitia::SET_UNIPAC;
		tmp_pius.indic = &hipa[0];
		aptus->facio(&tmp_pius);
		break;

	default:
		return false;
	}
	return true;
}

bool PacIns::sponte( Amor::Pius *pius) {
	PacketObj **tmp;
	assert(pius);
	if (!gCFG ) return false;

	switch ( pius->ordo ) {
	case Notitia::SET_UNIPAC:
		WBUG("sponte SET_UNIPAC");
		if ( (tmp = (PacketObj **)(pius->indic))) {
			if ( *tmp) 
				hi_req_p = *tmp; 
			else {
				WLOG(WARNING, "sponte SET_UNIPAC rcv_pac null");
			}
			tmp++;
			if ( *tmp) 
				hi_reply_p = *tmp;
			else {
				WLOG(WARNING, "sponte SET_UNIPAC snd_pac null");
			}
		} else {
			WLOG(WARNING, "sponte SET_UNIPAC null");
		}
		break;

	case Notitia::MULTI_UNIPAC_END:
		WBUG("sponte MULTI_UNIPAC_END");
		break;

	case Notitia::ERR_UNIPAC_RESOLVE:
		WBUG("sponte ERR_UNIPAC_RESOLVE");
		TEXTUS_SPRINTF(my_err_str, "err_unipac_resolve");
		goto ERR_UNIPAC;

	case Notitia::ERR_UNIPAC_COMPOSE:
		WBUG("sponte ERR_UNIPAC_COMPOSE");
		TEXTUS_SPRINTF(my_err_str, "err_unipac_compose");
		ERR_UNIPAC:
		{
			struct InsReply *rep = (struct InsReply *)cur_insway->reply;
			rep->err_str = my_err_str;
			rep->err_code = cur_insway->dat->err_code;
			ans_ins_ps.indic = rep;
			aptus->sponte(&ans_ins_ps);     //向左发出指令?, 前一指令未返回, 这里又重入?
		}
		break;

	case Notitia::PRO_UNIPAC:
		WBUG("sponte PRO_UNIPAC");
		if ( cur_insway )
		{
			if ( ((struct PacInsData *)(cur_insway->dat->ext_ins))->subor != pius->subor )
			{
				WLOG(WARNING, "error right_subor=%d != pius->subor=%d", ((struct PacInsData *)(cur_insway->dat->ext_ins))->subor, pius->subor);
			} else 
				ans_ins(true);
		} else {
			WLOG(WARNING, "cur_insway is null");		
		}
		break;

	default:
		return false;
		break;
	}
	return true;
}

void PacIns::get_cert(PacketObj *pac, int sub)
{
	Amor::Pius peer_ps;

	/* 外部配置文件中, 必须设置第1域 */
	peer_ps.ordo = Notitia::CMD_GET_CERT_NO;
	peer_ps.subor = sub;
	peer_ps.indic = 0;
	aptus->facio(&peer_ps);
	if (peer_ps.indic)
		pac->input(1, (unsigned char*)peer_ps.indic, strlen((const char*)peer_ps.indic));
}

void PacIns::set_peer(PacketObj *pac, int sub)
{
	TiXmlElement peer_xml("peer");
	Amor::Pius peer_ps;
	char ip[64], port[32];
	unsigned char *p;
	unsigned long rlen=0;

	/* 外部配置文件中, 必须将ip设置1域, port设置为2域 */
	p = pac->getfld(1, &rlen);
	if ( rlen > 30 ) rlen = 30;
	memcpy(ip, p, rlen); ip[rlen] = 0;

	p = pac->getfld(2, &rlen);
	if ( rlen > 30 ) rlen = 30;
	memcpy(port, p, rlen); port[rlen] = 0;

	peer_xml.SetAttribute("ip", ip);
	peer_xml.SetAttribute("port",port);
	peer_ps.ordo = Notitia::CMD_SET_PEER;
	peer_ps.subor = sub;
	peer_ps.indic = &peer_xml;
	aptus->facio(&peer_ps);
}

void PacIns::get_peer(PacketObj *pac, int sub)
{
	TiXmlElement *peer = 0;
	Amor::Pius g_peer;
	const char *p;

	/* 外部配置文件中, 必须将ip设置1域, port设置为2域 */
	g_peer.ordo = Notitia::CMD_GET_PEER;
	g_peer.subor = sub;
	g_peer.indic = 0;
	aptus->sponte(&g_peer);
	peer = (TiXmlElement *) g_peer.indic;
	if ( peer ) {
		p = peer->Attribute("cliip");
		pac->input(1, p, strlen(p));
		p = peer->Attribute("cliport");
		pac->input(2, p, strlen(p));
		p = peer->Attribute("srvip");
		pac->input(3, p, strlen(p));
		p = peer->Attribute("srvport");
		pac->input(4, p, strlen(p));
	} else {
		WBUG("get_peer return null");
	}
}

void PacIns::log_pac(PacketObj *pac,const char *prompt, enum PAC_LOG mode)
{
	TBuffer tbuf;
	size_t plen;
	int i;
	size_t j;
	char *rstr, max_str[16];
	size_t o, o_base;;
	bool has_str=false, has_hex=false;

	if ( !pac ) return ;
	if (mode &0x1) has_str = true;
	if (mode &0x2) has_hex = true;
	plen = strlen(prompt);
	TEXTUS_SPRINTF(max_str, "%d", pac->max); //最大的域号所显示的字符数
	tbuf.grant(plen + 2 + (pac->buf.point - pac->buf.base)*((has_str ? 1:0) + (has_hex ? 2:0))+(7+strlen(max_str))*(pac->max));
	tbuf.input((unsigned char*)prompt, plen);
	tbuf.input((unsigned char*)" ", 1);
	for ( i = 0; i < pac->max; i++)
	{
		if ( pac->fld[i].no < 0 || !pac->fld[i].val ) continue;
		rstr = (char*) tbuf.point;
		TEXTUS_SNPRINTF(rstr, 7, "{%d [", i);
		o_base = strlen(rstr);
		for ( j = 0 ; j < pac->fld[i].range; j++) {
			unsigned char c = pac->fld[i].val[j];
			if ( has_hex) {	//需要16进制显示
				o = o_base + j*2;
				rstr[o++] = Obtainx((c & 0xF0 ) >> 4 );
				rstr[o++] = Obtainx(c & 0x0F );
			} 
			if ( has_str ) {//需要字符显示
				o = o_base +  (pac->fld[i].range+1)*(has_hex ?2:0) + j;
				if ( c >= 0x20 && c <= 0x7e )
					rstr[o] =  c;
				else
					rstr[o] =  '.';
			}
		}
		if ( has_hex) { //需要16进制显示
			o = o_base + j*2;
			rstr[o++] = ']';
			rstr[o] = '[';
		}
		o = o_base + (pac->fld[i].range+1)* (has_hex ?2:0) + j;
		rstr[o++] = ']';
		rstr[o++] = '}';
		tbuf.commit(o);
		//tbuf.point[0] = 0;
		//printf("--- %s\n", tbuf.base);
	}
	tbuf.point[0] = 0;
	WLOG(INFO, (char*)tbuf.base);
}

void PacIns::set_ins (struct InsData *insd)
{
	TiXmlElement *e_tmp, *p_ele, *def_ele;
	const char *p=0, *pp=0; 
	char *q;

	int i = 0,a_num,j;
	struct CmdSnd *a_snd_lst;
	size_t lnn; 
	int lnn2;

	const char *tag;
	struct PacInsData *paci;
	bool isFunction;

	if ( insd->ext_ins ) return;	//已经定义过了,  不理
	def_ele = gCFG->pac_def_root->FirstChildElement(insd->ins_tag);	
	if ( !def_ele ) return; //在基础报文中没有定义, 不理

	insd->up_subor = gCFG->subor;
	paci = (struct PacInsData *) new struct PacInsData;
	insd->ext_ins = (void*)paci;

	paci->set_def(def_ele,insd);
	/* 先预置发送的每个域，设定域号*/
	a_num = 0;
	for (p_ele= def_ele->FirstChildElement(); p_ele; p_ele = p_ele->NextSiblingElement())
	{
		p = p_ele->Value();
		if ( !p ) continue;
		if ( strcasecmp(p, "send") == 0 || p_ele->Attribute("to")) 
			a_num++;
	}
	if ( a_num ==0 ) goto RCV_PRO;	
	insd->snd_lst  = new struct CmdSnd[a_num];
	for (p_ele= def_ele->FirstChildElement(),i = 0; p_ele; p_ele = p_ele->NextSiblingElement())
	{
		p = p_ele->Value();
		if ( !p ) continue;
		if ( strcasecmp(p, "send") == 0 || p_ele->Attribute("to")) 
		{
			tag = p_ele->Value();
			insd->snd_lst[i].tag =tag;
			if ( strcasecmp(tag, "send") == 0 ) 
			{
				p_ele->QueryIntAttribute("field", &(insd->snd_lst[i].fld_no));
				p = p_ele->GetText();
				if ( p ) {
					lnn = strlen(p);
					insd->snd_lst[i].cmd_buf = new unsigned char[lnn+1];
					insd->snd_lst[i].cmd_len = BTool::unescape(p, insd->snd_lst[i].cmd_buf) ;
				}
			} else 
				p_ele->QueryIntAttribute("to", &(insd->snd_lst[i].fld_no));
			i++;
		}
	}
	insd->snd_num = i;	//最后再更新一次发送域的数目

	/* 预置接收的每个域，设定域号*/
RCV_PRO:
	insd->rcv_num = 0;
	for (p_ele= def_ele->FirstChildElement(); p_ele; p_ele = p_ele->NextSiblingElement())
	{
		tag = p_ele->Value();
		if ( !tag ) continue;
		if ( strcasecmp(tag, "recv") == 0 ||p_ele->Attribute("from") ) 
		{
			insd->rcv_num++;
		}
	}
	if ( insd->rcv_num ==0 ) goto LAST_CON;	
	insd->rcv_lst = new struct CmdRcv[insd->rcv_num];
	for (p_ele= def_ele->FirstChildElement(),i = 0; p_ele; p_ele = p_ele->NextSiblingElement())
	{
		tag = p_ele->Value();
		if ( !tag ) continue;
		if ( strcasecmp(tag, "recv") == 0 || p_ele->Attribute("from")) 
		{
			insd->rcv_lst[i].tag = tag;
			p_ele->QueryIntAttribute("field", &(insd->rcv_lst[i].fld_no));
			p_ele->QueryIntAttribute("from", &(insd->rcv_lst[i].fld_no));
			p = p_ele->GetText();
			if ( p ) 
			{
				lnn = strlen(p);
				insd->rcv_lst[i].must_con = new unsigned char[lnn+1];
				insd->rcv_lst[i].must_len = BTool::unescape(p, insd->rcv_lst[i].must_con) ;
				insd->rcv_lst[i].err_code = p_ele->Attribute("error");	//接收域若有不符合，设此错误码
			}
			p = p_ele->Attribute("disp");
			if (!p) p = def_ele->Attribute("disp");
			if (!p) p = def_ele->GetDocument()->RootElement()->Attribute("disp");
			if ( p && strcasecmp(p, "hex") == 0 )
				insd->rcv_lst[i].err_disp_hex = true;
			i++;
		}
	}	/* 结束返回元素的定义*/

LAST_CON:
	return ;
}

void PacIns::pro_ins ()
{
	struct PacInsData *paci;
	struct InsReply *rep;
	paci = (struct PacInsData *)cur_insway->dat->ext_ins;
	rep = (struct InsReply *)cur_insway->reply;
	//if ( strcmp(cur_insway->dat->ins_tag, "InitPac") == 0 )
	//	{int *a =0 ; *a = 0; };
		
	hi_req_p->reset();	//请求复位
	paci->pac_cross_before(hi_req_p, hi_reply_p, rcv_pac, snd_pac);
	loc_pro_pac.subor = paci->subor;
	get_req_pac(hi_req_p, cur_insway->psnap, cur_insway->dat);

	switch ( paci->type) {
	case INS_Normal:
	case INS_Pro_DBFace:
		if ( paci->pac_log & 0x10) 
			log_pac(hi_req_p, paci->type == INS_Normal ? "INS_Normal" : "INS_Pro_DBFace", paci->pac_log); //本模块做
		if (  paci->subor < Amor::CAN_ALL ) 	//仅仅是报文域赋值
		{
			ans_ins(false);
			goto END_PRO;
		}
		if ( paci->type == INS_Normal )
			aptus->facio(&loc_pro_pac);
		else if ( paci->type == INS_Pro_DBFace) 
		{ //DB操作
			if ( !paci->dbface ) 
			{
				Pius get_face;
				void *ind[2];
				get_face.ordo = Notitia::CMD_GET_DBFACE;
				ind[0] = (void*) paci->dbface_name;
				ind[1] = 0;
				get_face.indic = ind;
				get_face.subor = paci->subor;
				aptus->facio(&get_face);
				paci->dbface = 	(DBFace*) ind[1];
				if ( !paci->dbface ) 
				{
					TEXTUS_SPRINTF(my_err_str, "no dbface");
					WLOG(WARNING, "%s error ",  my_err_str);
					rep->err_str = my_err_str;
					rep->err_code = cur_insway->dat->err_code;
					return ;
				}
			}
			prodb_ps.indic = paci->dbface;
			prodb_ps.subor = paci->subor;
			aptus->facio(&prodb_ps);
		}
		if(cur_insway->dat->isFunction) ans_ins(false);
END_PRO:
		break;

	case INS_SetPeer:
		set_peer(hi_req_p, loc_pro_pac.subor);
		ans_ins(false);
		break;

	case INS_GetPeer:
		get_peer(hi_reply_p, paci->subor);
		ans_ins(false);
		break;

	case INS_Get_CertNo:
		get_cert(hi_reply_p, paci->subor);
		ans_ins(false);
		break;

	case INS_Cmd_Ordo:
		other_ps.indic = 0;
		other_ps.ordo = paci->ordo;
		other_ps.subor = paci->subor;
		if ( paci->fac_spo == FACIO ) 
			aptus->facio(&other_ps);     //向右发出指令, 右节点不再sponte
		else
			aptus->sponte(&other_ps);     //向左发出指令, 
		return ;
		break;

	default :
		break;
	}
}

void PacIns::ans_ins (bool should_spo)
{
	struct PacInsData *paci;
	struct InsReply *rep;
	paci = (struct PacInsData *)cur_insway->dat->ext_ins;
	rep = (struct InsReply *)cur_insway->reply;
	rep->err_code = 0;
	rep->err_str = 0;

	last_ans_pac = paci->pac_cross_after(hi_req_p, hi_reply_p, rcv_pac, snd_pac);
	if ( paci->pac_log & 0x10) 
		log_pac(last_ans_pac,"Reply", paci->pac_log);
	if ( cur_insway->dat->rcv_num > 0 && (rep->err_code = pro_rply_pac(last_ans_pac, cur_insway->psnap, cur_insway->dat))) 
	{
		rep->err_str = my_err_str;
		if ( paci->pac_log & 0x20) 
		{
			log_pac(hi_req_p, my_err_str, paci->pac_log);
			log_pac(last_ans_pac,"Reply", paci->pac_log);
		}
	}
	if (should_spo )
	{
		ans_ins_ps.indic = rep;
		aptus->sponte(&ans_ins_ps);     //向左发出指令, 
	}
}

void PacIns::log_ins ()
{
	log_pac(hi_req_p, "Request", PAC_LOG_BOTH);
	log_pac(last_ans_pac,"Reply", PAC_LOG_BOTH);
}

/* 本指令处理响应报文，匹配必须的内容,出错时置出错代码变量 */
const char* PacIns::pro_rply_pac(PacketObj *rply_pac, struct DyVarBase **psnap, struct InsData *insd)
{
	int ii;
	unsigned char *fc;
	size_t rlen, mlen;
	struct CmdRcv *rply=0;
	char con[512];
	PacketObj *n_pac=rply_pac;
				
	for (ii = 0; ii < insd->rcv_num; ii++)
	{
		rply = &insd->rcv_lst[ii];
		fc = n_pac->getfld(rply->fld_no, (unsigned long *)&rlen);
		if (rply->must_con ) 
		{
			if ( !fc ) 
			{
				TEXTUS_SPRINTF(my_err_str, "recv packet error: field %d does not exist", rply->fld_no);
				goto ErrRet;
			}
			if ( !(rply->must_len == rlen && memcmp(rply->must_con, fc, rlen) == 0 ) ) 
			{
				mlen = rlen > sizeof(con) ? sizeof(con):rlen;
				
				if ( rply->err_disp_hex )
				{
					byte2hex(fc, mlen, con);
					con[mlen*2] = 0;
				} else {
					memcpy(con, fc, mlen);
					con[mlen] = 0;
				}
				TEXTUS_SPRINTF(my_err_str, "recv packet error: field %d should not content %s", rply->fld_no, con);
				goto ErrRet;
			}
		}
		//{int *a =0 ; *a = 0; };
		if ( rply->dyna_pos > 0 && fc) 
		{
			if ( rlen >= (rply->start ) )	
			{
				rlen -= (rply->start-1); //start是从1开始
				if ( rply->length > 0 && (unsigned int)rply->length < rlen)
					rlen = rply->length;
				psnap[rply->dyna_pos]->input(&fc[rply->start-1], rlen);
			}
		}
	}
	return (const char*)0;
ErrRet:
	if ( rply->err_code )
		return rply->err_code;
	else
		return insd->err_code;	//可能对于域不符合的情况，未定义错误码，就取基础报文或map中的定义
};
#include "hook.c"
