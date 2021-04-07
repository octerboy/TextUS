/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: TiXmlElement to Unipac 
 Build: createcd by octerboy on 2006/12/28, Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "textus_string.h"
#include "PacData.h"
#include "BTool.h"
#include <stdarg.h>
#define TINLINE inline
class X2pac: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Amor::Pius*);
	bool sponte( Amor::Pius*);
	Amor *clone();
	
	X2pac();
	~X2pac();

private:
	TBuffer fld_store;
	PacketObj f_pac;	/* facio 处理 */
	PacketObj s_pac;	/* sponte 处理 */
	PacketObj *pa[3];	/* 保存上两个指针 */

	PacketObj *first_pac;	/* facio 处理 */
	PacketObj *second_pac;	/* sponte 处理 */

	Amor::Pius pac_ps;	/* 向右传递 PRO_UNIPAC */
	Amor::Pius bdy_ps;	/* 向左传递 PRO_SOAP_BODY */
	Amor::Pius fault_ps;	/* 向左传递 ERR_SOAP_FAULT */
	TiXmlElement ans_rt;

	struct PField {
		int no;
		unsigned char *f_val;	/* 该域赋常量的值 */
		unsigned TEXTUS_LONG f_len;	/* 长度 */
		inline PField () {
			no = -1;
			f_val = 0;
			f_len = 0;
		};

		inline ~PField () {
			if ( f_val) 
				delete[] f_val;
		};
	};

	struct Where {
		const char *tag;	/* 元素名 */
		int index;		/* 同名的第几个元素, 默认1, 表示First, -1表示总是添加 */
		int num;		/* where所指示的个数, =0表示到了叶节点 */
		Where *where;		/* =null 表示到了叶节点 */
		struct PField *fdef;	/* 域定义, 与where不能同时有值, 但可同时为null */
		inline Where () {
			index = 1;
			where = 0;
			num = 0;
			fdef = 0;
		};

		inline ~Where () {
			if (where)
				delete[] where;
			where = 0;

			if (fdef)
				delete fdef;
			fdef = 0;
		};
	};

	
	bool has_config;

	struct G_CFG {
		Where *fac, *spo;
		const char* fld_tag;	/* 在叶节点定义中, 用以说明field号的属性名, 默认为field */

		int err_code_fld;	/* 包含错误信息的响应定义, faultCode */
		int err_str_fld;	/* 包含错误信息的响应定义, faultString  */

		int max_fldNo;		/* 最大域号 */
		int offset;		/* 域号偏移量 */
		bool multiPac;

		inline G_CFG() {
			multiPac = false; 
			fac = spo = 0;
			fld_tag = 0;

			max_fldNo = -1;	
			offset = 0;
	
			err_code_fld = -1;
			err_str_fld = -1;
		};

		inline ~G_CFG() {
			if (fac )
				delete fac;

			if (spo )
				delete spo;
		};
	};
	struct G_CFG *gCFG;

	/* 将Packet根据定义对应到 TiXmlElement 中*/
	TINLINE void mapdo(TiXmlElement *, PacketObj &, struct Where &);
	TINLINE TiXmlElement *getThat(struct Where &wh, TiXmlElement *ele, bool create=true);
	TINLINE void do_fault(TiXmlElement &, const char *code, const char *str, PacketObj *pac= 0);

	/* 将TiXmlElement根据定义对应到 Packet 中*/
	TINLINE void domap(TiXmlElement *, PacketObj &, struct Where &);
	void def_map(struct Where *, TiXmlElement *);
#include "wlog.h"
};

#include "casecmp.h"
#include <assert.h>

#define BZERO(X) memset(X, 0 ,sizeof(X))
void X2pac::ignite(TiXmlElement *cfg)
{
	const char *comm_str;
	TiXmlElement *f_ele;

	if (!cfg) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
	}

	if (gCFG->spo )
	{
		delete gCFG->spo;
		gCFG->spo = 0;
	}

	if (gCFG->fac )
	{
		delete gCFG->fac;
		gCFG->fac = 0;
	}

	comm_str = cfg->Attribute("offset");
	if ( comm_str )
	{
		if ( atoi(comm_str) > 0 )
			gCFG->offset = atoi(comm_str);
	}

	comm_str = cfg->Attribute("max");
	if ( comm_str )
	{
		if ( atoi(comm_str) >= 0 )
			gCFG->max_fldNo = atoi(comm_str);
	}

	comm_str = cfg->Attribute("response");
	if ( comm_str && strcasecmp(comm_str, "multi") == 0 )
	{
			gCFG->multiPac = true;
	}

	cfg->QueryIntAttribute("errcode", &(gCFG->err_code_fld));
	cfg->QueryIntAttribute("errstr", &(gCFG->err_str_fld));

	f_ele = cfg->FirstChildElement("facio");
	if( f_ele) 
	{
		gCFG->fac = new struct Where;
		def_map(gCFG->fac, f_ele);
	}
	
	f_ele = cfg->FirstChildElement("sponte");
	if( f_ele) 
	{
		gCFG->spo = new struct Where;
		def_map(gCFG->spo, f_ele);
	}
	
	f_pac.produce(gCFG->max_fldNo);
	s_pac.produce(gCFG->max_fldNo);
}

bool X2pac::facio( Amor::Pius *pius)
{
	TiXmlElement *req_rt;
	Amor::Pius tmp_pius;
	
	assert(pius);

	switch (pius->ordo )
	{
	case Notitia::PRO_SOAP_BODY:	/* 有SOAP请求 */
		WBUG("facio PRO_SOAP_BODY");
		req_rt = (TiXmlElement *)(pius->indic);
		if ( req_rt && gCFG->fac)
		{
			first_pac->reset();
			domap(req_rt, *first_pac, *gCFG->fac);
			aptus->facio(&pac_ps);	/* 向右发出PRO_UNIPAC */
		}
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		goto DEL_PAC;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY, 1st pac %p, 2nd pac %p", pa[0], pa[1]);
	DEL_PAC:
		tmp_pius.ordo = Notitia::SET_UNIPAC;
		tmp_pius.indic = pa;
		aptus->facio(&tmp_pius);
		break;

	default:
		return false;
	}
	return true;
}

bool X2pac::sponte( Amor::Pius *pius) 
{ 
	PacketObj **tmp;
	switch (pius->ordo )
	{
	case Notitia::PRO_UNIPAC:	/* 有unipac响应 */
		WBUG("sponte PRO_UNIPAC");
		if( gCFG->spo)
		{
			if ( !gCFG->multiPac )
				ans_rt.Clear();

			mapdo(&ans_rt, *second_pac, *gCFG->spo);
			second_pac->reset();	/* 右边数据清空 */
		}
		if ( !gCFG->multiPac )
			aptus->sponte(&bdy_ps);	/* 向左发出PRO_SOAP_BODY */
		break;

	case Notitia::MULTI_UNIPAC_END:	/* 多个packet结束 */
		WBUG("sponte MULTI_UNIPAC_END");
		if ( gCFG->multiPac )
		{
			aptus->sponte(&bdy_ps);	/* 向左发出PRO_SOAP_BODY */
			ans_rt.Clear(); 	/* 有多pacObj响应的, 在这里清空 */
		}
			
		break;

	case Notitia::ERR_UNIPAC_COMPOSE:	/* unipac组包错误 */
		WBUG("sponte ERR_UNIPAC_COMPOSE");
		do_fault(ans_rt, "ERR_UNIPAC_COMPOSE", "the request has invalid data, may be null or of invalid length.");
		aptus->sponte(&fault_ps);
		break;

	case Notitia::ERR_UNIPAC_RESOLVE:	/* unipac解包错误 */
		WBUG("sponte ERR_UNIPAC_RESOLVE");
		do_fault(ans_rt, "ERR_UNIPAC_RESOLVE", "The response has invalid data, can't to client.");
		aptus->sponte(&fault_ps);
		break;

	case Notitia::ERR_UNIPAC_INFO:		/* packetobj中包含一般错误信息 */
		WBUG("sponte ERR_UNIPAC_INFO");
		do_fault(ans_rt, 0, 0, second_pac);
		aptus->sponte(&fault_ps);
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
		} else 
			WLOG(WARNING, "sponte SET_UNIPAC null");

		break;
	default:
		return false;
	}
	return true; 
}

X2pac::X2pac():ans_rt("Response")
{
	pac_ps.ordo = Notitia::PRO_UNIPAC;
	pac_ps.indic = 0;

	bdy_ps.ordo = Notitia::PRO_SOAP_BODY;
	bdy_ps.indic = &ans_rt;

	fault_ps.ordo = Notitia::ERR_SOAP_FAULT;
	fault_ps.indic = &ans_rt;

	pa[0] = &f_pac;
	pa[1] = &s_pac;
	pa[2] = 0;

	first_pac = &f_pac;
	second_pac = &s_pac;

	gCFG = 0;
	has_config = false;
}

X2pac::~X2pac()
{
	if (has_config ) 
	{
		if(gCFG) delete gCFG;
	}	
}

Amor* X2pac::clone()
{
	X2pac *child;
	child = new X2pac();
	child->gCFG = gCFG;

	child->f_pac.produce(gCFG->max_fldNo);
	child->s_pac.produce(gCFG->max_fldNo);
	return (Amor*)child;
}

void X2pac::def_map(struct Where *wh, TiXmlElement *cfg)
{
	TiXmlElement *f_ele;
	int i, num;

	if ( !cfg) return ;
	wh->tag = cfg->Value();	

	/* sub表明是第几个节点 */
	if ( cfg->QueryIntAttribute("sub", &(wh->index)) != TIXML_SUCCESS)
		wh->index = 1;


	/* 计数, 有多少个子节点 */
	f_ele = cfg->FirstChildElement(); num = 0;
	while(f_ele)
	{
		f_ele = f_ele->NextSiblingElement();
		num++;
	}

	if ( num > 0 )
	{
		wh->where = new struct Where[num];
		wh->num = num;

		f_ele = cfg->FirstChildElement(); i = 0;
		while(f_ele) 
		{	/* 递归调用 */
			def_map( &(wh->where[i]), f_ele);
			i++;
			f_ele = f_ele->NextSiblingElement();
		}

	} else { /* 这是一个叶节点 */
		const char *tag = gCFG->fld_tag;
		if ( !tag)  tag = "field";

		wh->fdef = new struct PField;
		cfg->QueryIntAttribute(tag, &(wh->fdef->no)) ;	/* 取得域号 */

		{
			/* 检测常量, 这样当一个FieldObj没有内容或不存在时, xml元素从这里获得 */
			/* 当不存在这样一个xml元素时, FieldObj从这里获得 */
			const char *c_val = cfg->GetText();
			TEXTUS_LONG len;
			if ( c_val )
			{
				len = strlen(c_val);
				wh->fdef->f_val = new unsigned char[len+1];
				wh->fdef->f_len = BTool::unescape(c_val, wh->fdef->f_val);
			}
		}
		
	}
}

/* 将Packet根据定义对应到 TiXmlElement 中*/
void X2pac::mapdo(TiXmlElement *elem, PacketObj &packet, struct Where &wh)
{
	int i;
	FieldObj *fobj;
	struct PField *fdef;
	TiXmlElement *m_ele;
	
	if ( !wh.where )	/* 定义指明到了叶元素 */
	{	unsigned char *cval = 0;
		unsigned TEXTUS_LONG clen = 0;

		fdef = wh.fdef;
		if ( fdef && fdef->no >=0 && fdef->no <= packet.max ) 
		{	/* 取packet中某field的内容 */
			fobj = &packet.fld[fdef->no];
			if ( fobj->no == fdef->no )
			{
				cval = fobj->val;
				clen = fobj->range;
			}
		} else {	/* 取常量 */
			cval = fdef->f_val;
			clen = fdef->f_len;
		}

		/* 有值才 */
		if ( clen > 0 ) 
		{
			TiXmlText *tval = new TiXmlText("");

			fld_store.reset();
			fld_store.grant(clen+1);
			fld_store.input(cval, clen);
			*(fld_store.point) = '\0';

			tval->SetValue( (const char*)fld_store.base);
			elem->LinkEndChild(tval);	
		}
	} else {
		for ( i = 0 ; i  < wh.num; i++ )
		{
			struct Where &m_wh = wh.where[i];
			m_ele = getThat(m_wh, elem);
			mapdo(m_ele, packet, m_wh);
		}
	}
}

/* 将TiXmlElement根据定义对应到 Packet 中*/
void X2pac::domap( TiXmlElement *elem, PacketObj &packet, struct Where &wh)
{
	int i;
	struct PField *fdef;
	TiXmlElement *m_ele;

	
	fdef = wh.fdef;
	if ( !wh.where && fdef && fdef->no >=0 && fdef->no <= packet.max ) 
	{	 /* 定义指明到了叶元素 */
		unsigned char *cval = 0;
		unsigned TEXTUS_LONG clen ;
		if ( elem) 
			cval = (unsigned char*) elem->GetText();

		if( cval)
		{
			clen = strlen((char*)cval);
		} else  {
			cval = fdef->f_val;
			clen = fdef->f_len;
		}
		
		if ( clen > 0 )
			packet.input(fdef->no, (unsigned char*)cval, clen);
	} else {
		for ( i = 0 ; i  < wh.num; i++ )
		{
			struct Where &m_wh = wh.where[i];
			if ( elem) 
				m_ele = getThat(m_wh, elem, false);	/* 寻找相应的ele */
			else
				m_ele = 0;
			domap(m_ele, packet, m_wh);
		}
	}
}

void X2pac::do_fault(TiXmlElement &root, const char *code, const char *str, PacketObj *pac)
{
	TiXmlText *c_val;
	TiXmlText *s_val;
	TiXmlElement *c_ele = new TiXmlElement("faultcode");
	TiXmlElement *s_ele = new TiXmlElement("faultstring");
	
	if ( pac )
	{
		unsigned char *v;
		unsigned TEXTUS_LONG l;
		
		c_val = new TiXmlText("");
		s_val = new TiXmlText("");

		if ( (v =pac->getfld( gCFG->err_code_fld, &l )) )
		{
			fld_store.reset();
			fld_store.grant(l+1);
			fld_store.input(v, l);
			*(fld_store.point) = '\0';

			c_val->SetValue( (const char*)fld_store.base);
		}

		if ( (v =pac->getfld( gCFG->err_str_fld, &l )) )
		{
			fld_store.reset();
			fld_store.grant(l+1);
			fld_store.input(v, l);
			*(fld_store.point) = '\0';

			s_val->SetValue( (const char*)fld_store.base);
		}

	} else {
		if ( code )
			c_val = new TiXmlText(code);
		else
			c_val = new TiXmlText("");

		if ( str )
			s_val = new TiXmlText(str);
		else
			s_val = new TiXmlText("");
	}

	root.Clear();
	c_ele->LinkEndChild(c_val);	
	s_ele->LinkEndChild(s_val);	
	root.LinkEndChild(c_ele);	
	root.LinkEndChild(s_ele);	
}

TiXmlElement * X2pac::getThat(Where &wh, TiXmlElement *ele, bool create)
{
	int j, k;
	TiXmlElement *tmp_ele = 0;

	if ( wh.index < 0 ) 
	{
		if( !create )
			return ele->LastChild(wh.tag)->ToElement();	/* 取最后一个 */

		tmp_ele = new TiXmlElement(wh.tag);
		ele->LinkEndChild(tmp_ele);	
		return tmp_ele;
	}

	/* 先寻找相应的ele */
	for ( 	j = 1, tmp_ele = ele->FirstChildElement(wh.tag); 
		j < wh.index && tmp_ele;
		j++, tmp_ele = tmp_ele->NextSiblingElement(wh.tag) );

	if ( tmp_ele ) 	/* 找到了 */
		return tmp_ele;

	if ( !create )	/* 未找到, 也不创建 */
		return 0;

	/* 没有, 则创建 */
	for ( k = j ; k <= wh.index ; k++ )
	{
		tmp_ele = new TiXmlElement(wh.tag);
		ele->LinkEndChild(tmp_ele);	
	}
	
	return tmp_ele;
}
#include "hook.c"
