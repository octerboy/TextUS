/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title:Universal Transform Pro
 Build: created by octerboy, 2006/09/25, Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "BTool.h"
#include "casecmp.h"
#include "textus_string.h"
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include <stdarg.h>

#define PACINLINE inline
#define AFTER 0
#define BEFORE 1

class Unifom: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	Unifom();
	~Unifom();

	enum WHAT_PAC {NONE_PACK = 0, LFirst = 1, LSecond = 2 , RFirst = 3, RSecond = 4 };
	enum ACT {ACT_MAP = 0, ACT_FORWARD= 1,  ACT_ANSWER= 2 };

	enum CODE_TYPE {
	BCD	= 1,	/* 前补0, */
	BCDF	= 2,	/* 后补F */
	DBCD	= 3,	/* 第一位为一个符号, 这个暂不处理 */
	BCD0	= 4,	/* 后补0 */

	ASCII	= 8,	/* ASCII码字符串 */

	HEX	= 10,	/* 将每个字节作16进制表达 */
	HEXCAP	= 11,	/* 将每个字节作16进制表达 */

	INTEGER	= 22,
	FLOAT	= 27,
	DOUBLE	= 28,

	DATETIME	= 29,	/* 时间字符串 */
	ASN1_OBJ	= 30,	/* ans.1所定的对象 */
	ASN1_OBJID	= 40,
	ASN1_IP		= 41,
	ASN1_OCT_STR	= 42,
	ASN1_COUNTER32	= 43,
	ASN1_GAUGE32  	= 44,
	ASN1_TIMETICKS	= 45,
	ASN1_OPAQUE   	= 46,
	ASN1_COUNTER64	= 47,
	
	IP_ADDR_STR	= 50,	/* ipaddr for numbers-and-dots */
	IP_ADDR_BIN	= 51,	/* ipaddr for binary */

	ANY	=70,
	BASE64	=71,
	UNDEFINED = -1
	};

	enum PROC_TYPE {
		CONVERT = 0,
		MAP	= 1,
		TRIM	= 2,
		NOW	= 3,
		LET	= 4,
		FILL	= 5,
		SUBSTR	= 6,
		CALC	= 7,
		ObuSNConv= 8,
		Duplicate= 9,
		GetLength=10,
		UNKNOWN	= -1
	};

enum FMethod {		/* 域取值方法 */
	GETVAL	=0,	/* 原始净内容 */	
	GETRAW	=1	/* 原始全内容 */	
};

typedef struct _Proc_Substr {
	unsigned int begin;	/* 起始点 0  */
	unsigned int len;	/* 相对长度, 0:到末 */

	inline _Proc_Substr ()
	{
		begin = 0;
		len = 0;
	};
} Proc_Substr;

typedef struct _Proc_Let {
	unsigned char *src;	/* 常值内容 */
	unsigned int src_len;	/* 常值长度 */
	FMethod method;	/* 取值方法 */

	inline _Proc_Let ()
	{
		src = 0;
		src_len = 0;
		method = GETVAL;
	};
	inline ~_Proc_Let ()
	{
		if ( src )
			delete []src;
	};
} Proc_Let;

struct With_Map {
	unsigned char *from;
	unsigned int len_from;
	unsigned char *to;
	unsigned int len_to;
	inline With_Map ()
	{
		from = 0;
		to = 0;
		len_from = 0;
		len_to = 0;
	};
	inline ~With_Map ()
	{
		if ( from ) delete from;
		if ( to ) delete to;
	}
};

typedef struct _Proc_Map {	/* 对应 */
	struct With_Map *map;
	int mapNum;
} Proc_Map;

typedef struct _Proc_Trim {	/* 填充 */
	char c;		/* 补的字符 */
	int method;	/* 0: 后补, 1: 前补 */
	int len;	/* 满足其长度, 0:使其到一个2的倍数 */
	inline _Proc_Trim () 
	{
		c = 0;
		method = -1;
		len = -1;
	};
} Proc_Trim;

typedef struct _Proc_Now {	/* 取当前时间 */
	/* 取当前时间 */
	char *format;	/* 格式, 如果为null, 则目标是一个int型*/	
	int use_milli;
	inline _Proc_Now ()
	{
		format = 0;
		use_milli = 0;	//不用给出毫秒
	}

	inline ~_Proc_Now ()
	{
		if ( format)
			delete []format;
		format = 0;
	};
} Proc_Now;

enum Calc_Type {	/* 四则运算方法 */
	NONE_CALC	= 0,
	ADD		= 1,	/* 加 */	
	SUBTRACT	= 2,	/* 减 */
	DIVIDE		= 3,	/* 乘 */
	MULTIPLY 	= 4	/* 除 */
};

typedef struct _Proc_Calc {	/* 四则运算 */
	
	Calc_Type type;	/* 计算类型 */
	int source_1;	/* 第一因子, -1则试图取常数, >=0 取某个域  */
	int source_2;

	const char *cons;	/* 常量 */
	int i_cons;		/* 整型 */
	double d_cons;		/* 双精度 */

		
	inline _Proc_Calc ()
	{
		type = NONE_CALC;
		source_1 = source_2 = -1;
	};

	inline ~_Proc_Calc ()
	{
	};
} Proc_Calc;

typedef struct _Proc_Convert {	/* 编码格式转换 */
	CODE_TYPE src_type;	/* 源编码方式 */
	CODE_TYPE dst_type;	/* 目标编码方式 */
	char *format;		/* 格式 */
	int len;		/* 整数参数 */
	/*	支持以下类型, 不合法者不处理 
		BCD(BCD0等) -> ASCII : 
		BCD(BCD0等) -> INTEGER: 
		ASCII -> BCD : 凡16进制表达的, 都转
		ASCII -> INGEGER
		ANY  -> HEX:
		HEX  -> ANY:
		ASN1_INTEGER ->	INTEGER
		ASN1_OBJ -> ASCII
		ASCII -> ASN1_OBJID
		ASCII -> ASN1_IP
		ASCII -> ASN1_OCT_STR
		ASCII -> ASN1_COUNTER32
		ASCII -> ASN1_GAUGE32  
			ASN1_TIMETICKS
			ASN1_OPAQUE   
			ASN1_COUNTER64
	
		INTEGER	-> ASN1_INTEGER
		INTEGER(FLOAT等) -> ASCII, 一定格式的转,无格式则直接是10进制表达
		INTEGER	-> BCD
		INTEGER 、FLOAT、 DOUBLE等之间的互转
	*/
	inline _Proc_Convert () {
		format = 0;
		len = 0;
	};
} Proc_Convert;

typedef struct _Proc_GetLength {	/* 获取长度*/
	char *format;		/* 格式 */
	int unit;		/* 单位 */
	int adjust;		/* 调整量, 某域的长度除以unit，再加上这个adjust（可能加或减) */
} Proc_GetLength;

/* 针对广东国标OBU表面编号的转换, 从16位的表面编号, 得出OBU MAC, 同时还保留原来的编号 */
typedef struct _ObuSn_Convert {	/* 编码格式转换 */
	Proc_Map factory_map;
} Proc_ObuSN;

/* 每对域的转换定义 */
typedef struct _ProcDef {
	PROC_TYPE type;	/* 转换类型 */
	WHAT_PAC src_what, dst_what;	/* 从src_what (PacketObj)取, 变换到dst_what */
	int src_no;	/* 源域号 */
	int dst_no;	/* 目标域号 */
	void *prc;	/* 具体转换过程定义 */
	inline _ProcDef () {
		type = UNKNOWN;
		prc = 0;
		src_no = dst_no = -1;
		src_what = dst_what = NONE_PACK;
	};

	inline ~_ProcDef () {
		if ( prc ) switch ( type ) 
		{
		case CONVERT:
			delete (Proc_Convert *)prc ;
			break;

		case MAP:
			delete (Proc_Map *)prc ;
			break;

		case TRIM:
		case FILL:
			delete (Proc_Trim *)prc ;
			break;

		case NOW:
			delete (Proc_Now *)prc ;
			break;

		case LET:
			delete (Proc_Let *)prc ;
			break;

		case SUBSTR:
			delete (Proc_Substr *)prc ;
			break;

		case CALC:
			delete (Proc_Calc *)prc ;
			break;

		case GetLength:
			delete (Proc_GetLength *)prc ;
			break;

		default:
			break;

		}
	};
} ProcDef;

typedef ProcDef* PProcDef;

#include "PacData.h"
private:
	Amor::Pius local_pius;

	PacketObj *rcv_pac;	/* 来自左节点的PacketObj */
	PacketObj *snd_pac;

	PacketObj *r1st_pac;	/* 来自右节点的PacketObj */
	PacketObj *r2nd_pac;

	PacketObj do_pac;
	PacketObj un_pac;	/* 向右传递的 */
	PacketObj *pa[3];

	struct G_CFG {
		int fldOffset;	/* 处理PacketObj时, 定义中的域号加上此值(偏移量)即实际处理的域, 初始为0 */
		enum ACT what_act;	/* 什么动作, 给以响应, 还是仅map, 或再转 */
		
		int maxium_fldno;		/* 最大域号 */
	
		int both_defNum;		/* 定义处理项目数 */
		ProcDef	*both_defProcs;	/* 处理项数组 */

		int fac_defNum;		/* 定义处理项目数 */
		ProcDef	*fac_defProcs;	/* 处理项数组 */

		int spo_defNum;		/* 定义处理项目数 */
		ProcDef	*spo_defProcs;	/* 处理项数组 */
		inline G_CFG() {
			fldOffset = 0;

			both_defNum = 0;
			both_defProcs = 0;
			fac_defNum = 0;
			fac_defProcs = 0;
			spo_defNum = 0;
			spo_defProcs = 0;

			what_act = ACT_MAP;
			maxium_fldno = 0;
		};	
		inline ~G_CFG() {
			if ( fac_defProcs ) delete[] fac_defProcs;
			if ( spo_defProcs ) delete[] spo_defProcs;
			if ( both_defProcs ) delete[] both_defProcs;
		};
	};
	struct G_CFG *gCFG;     /* 全局共享参数 */
	bool has_config;

	PACINLINE void handle(int defNum, PProcDef defProcs, bool negative=false);
	PACINLINE void convert(FieldObj &fldIn, PacketObj &, unsigned int out, bool &negatvie, Proc_Convert &def);
	PACINLINE void domap(FieldObj &fldIn, PacketObj &, unsigned int out, bool &negatvie, Proc_Map &def);
	PACINLINE void dotrim(FieldObj &fldIn, PacketObj &, unsigned int out, bool &negatvie, Proc_Trim &def);
	PACINLINE void dosubstr(FieldObj &fldIn, PacketObj &, unsigned int out, Proc_Substr &def);
	PACINLINE void dolet(FieldObj *fldIn, PacketObj &, unsigned int out, Proc_Let &def);
	PACINLINE void duplicate(PacketObj &in, PacketObj &out);
	PACINLINE void donow(FieldObj *fldIn, PacketObj &, unsigned int out, Proc_Now &def);
	PACINLINE void docalc(FieldObj *fldIn, PacketObj &, unsigned int out, Proc_Calc &def);
	PACINLINE void do_obu_sn(FieldObj &fldIn, PacketObj &, unsigned int out, bool &negatvie, Proc_ObuSN &def);
	PACINLINE void do_getLen(FieldObj *fldIn, PacketObj &, unsigned int out, Proc_GetLength &def);

	void get_def(TiXmlElement *cfg, int &defNum, PProcDef &defProcs);
	PACINLINE void deliver(Notitia::HERE_ORDO aordo);
	#include "wlog.h"
};

#include <assert.h>

void Unifom::ignite(TiXmlElement *prop)
{
	const char *comm_str;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
	}
	if (!prop) return;

	if ( (comm_str = prop->Attribute("offset")) )
		gCFG->fldOffset = atoi(comm_str);

	if ( (comm_str = prop->Attribute("act")) )
	{
		if (strcasecmp(comm_str, "map") == 0 )
			gCFG->what_act = ACT_MAP;
		if (strcasecmp(comm_str, "forward") == 0 )
			gCFG->what_act = ACT_FORWARD;
		if (strcasecmp(comm_str, "answer") == 0 )
			gCFG->what_act = ACT_ANSWER;
	}

	if ( (comm_str = prop->Attribute("maxium")) )
		gCFG->maxium_fldno = atoi(comm_str);

	if ( gCFG->maxium_fldno < 0 )
		gCFG->maxium_fldno = 0;

	get_def(prop->FirstChildElement("facio"), gCFG->fac_defNum, gCFG->fac_defProcs);
	get_def(prop->FirstChildElement("sponte"), gCFG->spo_defNum, gCFG->spo_defProcs);
	get_def(prop->FirstChildElement("both"), gCFG->both_defNum, gCFG->both_defProcs);

	if ( gCFG->maxium_fldno > 0)
	{
		do_pac.produce(gCFG->maxium_fldno) ;
		un_pac.produce(gCFG->maxium_fldno) ;
	}
}

void Unifom::get_def(TiXmlElement *cfg, int &defNum, PProcDef &defProcs)
{
	const char *comm_str;
 	char *src_txt;
	TiXmlElement *fld_ele, *code_ele, *with_ele;

	ProcDef *def;
	Proc_Convert *procvt;
	Proc_Map *promap;
	Proc_Trim *protrim;
	Proc_Now *pronow;
	Proc_Let *prolet;
	Proc_Substr *prosubstr;
	Proc_GetLength *pro_getlen;
	int i;

	WHAT_PAC from_what = NONE_PACK, to_what = NONE_PACK;	/* 全局的定义 */

	if ( !cfg )
		return;
	
	if ( defProcs ) delete[] defProcs;	/* 清除以前的定义 */
	defProcs = 0;

#define GETONETYPEPAC(TAG, WHAT, VAR, ELE) \
	if ( (comm_str = ELE->Attribute(TAG)) && strcasecmp(comm_str, #WHAT) == 0 ) \
		VAR = WHAT;

#define GETPAC(TAG, VAR, ELE) \
	GETONETYPEPAC(TAG, LFirst,  VAR, ELE); 	\
	GETONETYPEPAC(TAG, LSecond, VAR, ELE);	\
	GETONETYPEPAC(TAG, RFirst,  VAR, ELE);	\
	GETONETYPEPAC(TAG, RSecond, VAR, ELE);	

	GETPAC("from", from_what, cfg);	/* 本段全局定义 */
	GETPAC("to", to_what, cfg);

	/* 计算有多少对域要转换 */
	fld_ele= cfg->FirstChildElement(); defNum = 0;
	while(fld_ele)
	{
		if ( fld_ele->Value() )
		{
			if (	strcasecmp(fld_ele->Value(), "convert") ==0 
			    ||	strcasecmp(fld_ele->Value(), "map") ==0 
			    ||	strcasecmp(fld_ele->Value(), "fill") ==0 
			    ||	strcasecmp(fld_ele->Value(), "trim") ==0 
			    ||	strcasecmp(fld_ele->Value(), "let") ==0 
			    ||	strcasecmp(fld_ele->Value(), "substr") ==0 
			    ||	strcasecmp(fld_ele->Value(), "now") ==0 
			    ||	strcasecmp(fld_ele->Value(), "calc") ==0
			    ||	strcasecmp(fld_ele->Value(), "duplicate") ==0
			    ||	strcasecmp(fld_ele->Value(), "getLength") ==0
			    ||	strcasecmp(fld_ele->Value(), "get_obu_id") ==0 )

				defNum++;
		}
		fld_ele = fld_ele->NextSiblingElement();
	}

	if ( defNum == 0 )
		return;

	defProcs = new ProcDef[defNum];

#define WHATCODE(X,Y)	if ( (comm_str = code_ele->Attribute("code") ) && strcasecmp(comm_str, #X) == 0 ) Y = X;

#define GETCODE(Y)			\
		WHATCODE(BCD,Y)		\
		WHATCODE(BCDF,Y)	\
		WHATCODE(DBCD,Y)	\
		WHATCODE(BCD0,Y)	\
		WHATCODE(HEX,Y)		\
		WHATCODE(HEXCAP,Y)	\
		WHATCODE(ASCII,Y)	\
		WHATCODE(INTEGER,Y)	\
		WHATCODE(FLOAT,Y)	\
		WHATCODE(DOUBLE,Y)	\
		WHATCODE(DATETIME,Y)	\
		WHATCODE(ANY,Y)		\
		WHATCODE(BASE64,Y)	\
		WHATCODE(ASN1_OBJ,Y)	\
		WHATCODE(ASN1_OBJID,Y)	\
		WHATCODE(ASN1_IP,Y)	\
		WHATCODE(ASN1_OCT_STR,Y)	\
		WHATCODE(ASN1_COUNTER32,Y)	\
		WHATCODE(ASN1_GAUGE32,Y)	\
		WHATCODE(ASN1_TIMETICKS,Y)	\
		WHATCODE(ASN1_OPAQUE,Y)		\
		WHATCODE(ASN1_COUNTER64,Y)	\
		WHATCODE(IP_ADDR_STR,Y)		\
		WHATCODE(IP_ADDR_BIN,Y)
	
	/* 每一段定义处理 */
	fld_ele= cfg->FirstChildElement();
	def = &defProcs[0] ; defNum= 0;
	while(fld_ele)
	{
		def->type = UNKNOWN;
#define WHATPROC(X) if ( strcasecmp(fld_ele->Value(), #X) ==0  ) def->type = X;
		WHATPROC(CONVERT)
		WHATPROC(MAP)
		WHATPROC(TRIM)
		WHATPROC(NOW)
		WHATPROC(FILL)
		WHATPROC(SUBSTR)
		WHATPROC(LET)
		WHATPROC(CALC)
		WHATPROC(Duplicate)
		WHATPROC(GetLength)

		if ( strcasecmp(fld_ele->Value(), "get_obu_id") ==0  ) 
			def->type = ObuSNConv;

		def->src_what = from_what;	/* 先取全局 */
		def->dst_what = to_what;
		GETPAC("from", def->src_what, fld_ele);	/* 局部优先 */
		GETPAC("to", def->dst_what, fld_ele);

		switch(def->type )
		{
		case CONVERT:
			code_ele = fld_ele->FirstChildElement("source");
			if (!code_ele ) 
				goto NEXTELE;

			def->prc = procvt = new Proc_Convert;
			procvt->format = (char*) fld_ele->Attribute("format");

			if ( (comm_str = fld_ele->Attribute("length")) )
				procvt->len = atoi(comm_str);

			def->src_no = atoi(code_ele->Attribute("no"));
			GETCODE( procvt->src_type );

			code_ele = fld_ele->FirstChildElement("destination");
			if (!code_ele ) 
				goto NEXTELE;
			def->dst_no = atoi(code_ele->Attribute("no"));
			GETCODE( procvt->dst_type );
			break;

		case GetLength:
			def->prc = pro_getlen = new Proc_GetLength;
			pro_getlen->format = (char*) fld_ele->Attribute("format");
			pro_getlen->unit = 1;
			pro_getlen->adjust = 0;
			fld_ele->QueryIntAttribute( "unit", &pro_getlen->unit );
			fld_ele->QueryIntAttribute( "adjust", &pro_getlen->adjust );
			fld_ele->QueryIntAttribute( "source", &def->src_no );
			fld_ele->QueryIntAttribute( "destination", &def->dst_no );
			
			break;
		case MAP:
			def->prc = promap = new Proc_Map;
			goto MAP_DEF_PRO;

		case ObuSNConv:
			def->prc = new Proc_ObuSN;
			promap = &(((Proc_ObuSN*)(def->prc))->factory_map);
MAP_DEF_PRO:
			if ( (comm_str = fld_ele->Attribute("source")) )
				def->src_no = atoi(comm_str);

			if ( (comm_str = fld_ele->Attribute("destination")) )
				def->dst_no = atoi(comm_str);
			
			promap->mapNum = 0;
			with_ele = fld_ele->FirstChildElement("with");
			while ( with_ele )
			{
				with_ele = with_ele->NextSiblingElement("with");
				promap->mapNum++;
			}
			promap->map = new struct With_Map[promap->mapNum];
			with_ele = fld_ele->FirstChildElement("with");

			i = 0;
			while ( with_ele )
			{
				TiXmlElement *dat;
				struct With_Map &with =promap->map[i];
				dat = with_ele->FirstChildElement("from");
				comm_str = 0;
				if ( dat )
					comm_str = dat->GetText();
				if ( comm_str )
				{
					with.from = new unsigned char[ strlen(comm_str) +1 ];
					with.len_from = BTool::unescape(comm_str, with.from);
				}

				dat = with_ele->FirstChildElement("to");
				comm_str = 0;
				if ( dat )
					comm_str = dat->GetText();
				if ( comm_str )
				{
					with.to = new unsigned char[ strlen(comm_str) +1 ];
					with.len_to = BTool::unescape(comm_str, with.to);
				}

				with_ele = with_ele->NextSiblingElement("with");
				i++;
			}
			
			break;

		case FILL:
		case TRIM:
			def->prc = protrim = new Proc_Trim;
			if ( (comm_str = fld_ele->Attribute("source")) )
				def->src_no = atoi(comm_str);

			if ( (comm_str = fld_ele->Attribute("destination")) )
				def->dst_no = atoi(comm_str);
			
			if ( (comm_str = fld_ele->Attribute("character")) )
				protrim->c = *comm_str;
			
			if ( (comm_str = fld_ele->Attribute("length")) )
				protrim->len = atoi(comm_str);
			
			if ( (comm_str = fld_ele->Attribute("method")) )
			{
				if ( strcasecmp(comm_str, "after") == 0 )
					protrim->method = AFTER;
				else if ( strcasecmp(comm_str, "before") == 0 )
					protrim->method = BEFORE;
				else
					protrim->method = UNDEFINED;
			}
			
			break;

		case NOW:
			def->prc = pronow = new Proc_Now;
			if ( (comm_str = fld_ele->Attribute("source")) )
				def->src_no = atoi(comm_str);

			if ( (comm_str = fld_ele->Attribute("destination")) )
				def->dst_no = atoi(comm_str);

			if ( (comm_str = fld_ele->Attribute("milli")) )
			{
				if ( strcasecmp(comm_str, "yes") ==0 ) 
					pronow->use_milli = -1;
			}

			pronow->format = 0;
			src_txt = (char*) fld_ele->GetText();
			if ( src_txt )
			{
				pronow->format = new char[strlen(src_txt) +1 ];
				BTool::unescape(src_txt, (unsigned char*)pronow->format);
			}
			break;

		case LET:
			def->prc = prolet = new Proc_Let;
			if ( (comm_str = fld_ele->Attribute("source")) )
				def->src_no = atoi(comm_str);

			if ( (comm_str = fld_ele->Attribute("destination")) )
				def->dst_no = atoi(comm_str);
			
			if ( (comm_str = fld_ele->Attribute("method") )  )
			{
				if ( strcasecmp(comm_str, "GETRAW" ) == 0)
					prolet->method = GETRAW;
				else if ( strcasecmp(comm_str, "GETVAL" ) == 0)
					prolet->method = GETVAL;
			}

			src_txt = (char*) fld_ele->GetText();
			if ( src_txt )
			{
				prolet->src = new unsigned char[strlen(src_txt) +1 ];
				prolet->src_len = BTool::unescape(src_txt, prolet->src);
			}
			break;
		
		case SUBSTR:
			def->prc = prosubstr = new Proc_Substr;
			if ( (comm_str = fld_ele->Attribute("source")) )
				def->src_no = atoi(comm_str);

			if ( (comm_str = fld_ele->Attribute("destination")) )
				def->dst_no = atoi(comm_str);
			
			if ( (comm_str = fld_ele->Attribute("begin") ) && atoi(comm_str) >=0 )
				prosubstr->begin = atoi(comm_str);

			if ( (comm_str = fld_ele->Attribute("len") ) && atoi(comm_str) >=0 )
				prosubstr->len = atoi(comm_str);

			if ( (comm_str = fld_ele->Attribute("length") ) && atoi(comm_str) >=0 )
				prosubstr->len = atoi(comm_str);

			break;
		case Duplicate:
			break;
		
		default :
			goto NEXTELE;
		}

		def++;
		defNum++;

	NEXTELE:
		fld_ele = fld_ele->NextSiblingElement();
	}

}
Unifom::Unifom()
{
	rcv_pac = 0;
	snd_pac = 0;

	r1st_pac = 0;
	r2nd_pac = 0;

	pa[0] = &do_pac;
	pa[1] = &un_pac;
	pa[2] = 0;

	local_pius.ordo = Notitia::PRO_UNIPAC;
	local_pius.indic = 0;

	gCFG = 0;
	has_config = false;
}

Unifom::~Unifom() 
{
	if ( has_config  )
	{	
		if(gCFG) delete gCFG;
	}
}

Amor* Unifom::clone()
{
	Unifom *child = new Unifom();
	child->gCFG = gCFG;
	if ( gCFG->maxium_fldno >0 )
	{
		child->do_pac.produce(do_pac.max);
		child->un_pac.produce(un_pac.max);
	}
	return (Amor*) child;
}

bool Unifom::facio( Amor::Pius *pius)
{
	PacketObj **tmp;
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::PRO_UNIPAC:
		WBUG("facio PRO_UNIPAC");

		handle(gCFG->fac_defNum, gCFG->fac_defProcs);
		handle(gCFG->both_defNum, gCFG->both_defProcs);
	
		switch ( gCFG->what_act )
		{
			case ACT_MAP:
				break;
			case ACT_FORWARD:
				aptus->facio(&local_pius);
				break;
			case ACT_ANSWER:
				aptus->sponte(&local_pius);
				break;
			default:
				break;
		}
		break;

	case Notitia::SET_UNIPAC:
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) rcv_pac = *tmp; 
			else
				WLOG(WARNING, "facio SET_UNIPAC rcv_pac null");
			tmp++;
			if ( *tmp) snd_pac = *tmp;
			else
				WLOG(WARNING, "facio SET_UNIPAC snd_pac null");
			WBUG("facio SET_UNIPAC rcv_pac(%p) snd_pac(%p)", rcv_pac, snd_pac);
		} else 
			WLOG(WARNING, "facio SET_UNIPAC null");
		
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		goto DEL_PAC;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
DEL_PAC:
		if ( gCFG->maxium_fldno > 0)
			deliver(Notitia::SET_UNIPAC);
		break;

	default:
		return false;
	}
	return true;
}

bool Unifom::sponte( Amor::Pius *pius)
{
	PacketObj **tmp;
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::PRO_UNIPAC:
		WBUG("sponte PRO_UNIPAC");

		handle(gCFG->spo_defNum, gCFG->spo_defProcs);
		handle(gCFG->both_defNum, gCFG->both_defProcs, true);

		switch ( gCFG->what_act )
		{
			case ACT_MAP:
				break;
			case ACT_FORWARD:
				aptus->sponte(&local_pius);
				break;
			case ACT_ANSWER:
				aptus->facio(&local_pius);
				break;
			default:
				break;
		}
		break;

	case Notitia::SET_UNIPAC:
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) r1st_pac = *tmp; 
			else
				WLOG(WARNING, "sponte SET_UNIPAC rcv_pac null");
			tmp++;
			if ( *tmp) r2nd_pac = *tmp;
			else
				WLOG(WARNING, "sponte SET_UNIPAC snd_pac null");
			WBUG("sponte SET_UNIPAC r1st_pac(%p) r2nd_pac(%p)", r1st_pac, r2nd_pac);
		} else 
			WLOG(WARNING, "sponte SET_UNIPAC null");

		break;
	default:
		return false;
	}
	return true;
}

PACINLINE void Unifom::handle(int defNum, PProcDef defProcs, bool negative)
{
	int i;
	int out;
	bool yes = true;
	ProcDef	*def;
	Proc_Convert *procvt;
	Proc_Map *promap;
	Proc_ObuSN *pro_obuid;
	Proc_Trim *protrim;
	Proc_Substr *prosubstr;
	Proc_Now *pronow;
	Proc_Let *prolet;
	Proc_GetLength *pro_getlen;

	WBUG("defNum is %d", defNum);
/*D:定义, O:包变量 */
#define SW(D, O)	\
		switch (def->D)			\
		{				\
		case LFirst :			\
			if ( !negative ) 	\
				O =  rcv_pac ;	\
			else			\
				O =  snd_pac ;	\
			break;			\
						\
		case LSecond :			\
			if ( !negative ) 	\
				O =  snd_pac ;	\
			else			\
				O =  rcv_pac ;	\
			break;			\
						\
		case RFirst :			\
			if ( !negative ) 	\
				O = (r1st_pac ? r1st_pac : &do_pac);	\
			else						\
				O = (r2nd_pac ? r2nd_pac : &un_pac);	\
			break;						\
									\
		case RSecond :						\
			if ( !negative ) 				\
				O = (r2nd_pac ? r2nd_pac : &un_pac);	\
			else						\
				O = (r1st_pac ? r1st_pac : &do_pac);	\
			break;						\
									\
		default :						\
			break;						\
		}

	def = &defProcs[0];
	for ( i = 0 ; i < defNum; i++, def++ )
	{
		FieldObj *fldIn;
		PacketObj *pacIn = 0, *pacOut = 0;
		int l_dstno = -1 , l_srcno = -1;

		/* 对于negative情况, 首先是first与second相反, 然后是in和out相反 */
		if ( !negative )
		{
			SW(src_what, pacIn)
			SW(dst_what, pacOut)
		} else {
			SW(src_what, pacOut)
			SW(dst_what, pacIn)
		}

		if ( !pacIn || !pacOut )
			continue;

		if ( def->type == Duplicate )
		{
			duplicate(*pacIn, *pacOut);
			continue;
		}
		if ( def->dst_no >=0 )
			l_dstno = def->dst_no + gCFG->fldOffset;

		if ( def->src_no >=0 )
			l_srcno = def->src_no + gCFG->fldOffset;

		fldIn = (FieldObj *)0;
		if ( negative )
		{	/* 域号相反, 但pacIn、pacOut都确定, 不再取反 */
			out = l_srcno;
			if ( pacIn->max >= l_dstno && l_dstno >= 0)
				fldIn = &pacIn->fld[l_dstno];
		} else {
			out = l_dstno;
			if ( pacIn->max >= l_srcno && l_srcno >= 0)
				fldIn = &pacIn->fld[l_srcno];
		}
	
		if ( out > pacOut->max || out < 0 )	/* 越界的不处理 */
			continue;

		if ( !(def->type == LET || def->type == NOW)  )
		{	/* 对于LET NOW, 源可以是空的,从常量取, !fldIn 表明定义越界 */
			if (!fldIn || fldIn->no < 0 || fldIn->range <= 0 )
				continue;	
		}

		switch ( def->type )
		{
		case CONVERT :
			WBUG("def %d CONVERT", i);
			procvt = (Proc_Convert *) def->prc;
			convert(*fldIn, *pacOut, out, negative, *procvt);
			break;

		case MAP:
			WBUG("def %d map", i);
			promap = (Proc_Map *) def->prc;
			domap(*fldIn, *pacOut, out, negative, *promap);
			break;

		case ObuSNConv:
			WBUG("def %d get_obu_id", i);
			pro_obuid = (Proc_ObuSN *) def->prc;
			do_obu_sn(*fldIn, *pacOut, out, negative, *pro_obuid);
			break;

		case FILL:
			protrim = (Proc_Trim *) def->prc;
			dotrim(*fldIn, *pacOut, out, yes, *protrim);
			break;

		case TRIM:
			protrim = (Proc_Trim *) def->prc;
			dotrim(*fldIn, *pacOut, out, negative, *protrim);
			break;

		case SUBSTR:
			prosubstr = (Proc_Substr *) def->prc;
			dosubstr(*fldIn, *pacOut, out, *prosubstr);
			break;

		case NOW:
			WBUG("def %d NOW", i);
			pronow = (Proc_Now *) def->prc;
			donow(fldIn, *pacOut, out, *pronow);
			break;

		case LET:
			prolet = (Proc_Let *) def->prc;
			dolet(fldIn, *pacOut, out, *prolet);
			break;

		case GetLength:
			WBUG("def %d GetLength", i);
			pro_getlen = (Proc_GetLength *) def->prc;
			do_getLen( fldIn, *pacOut, out, *pro_getlen);
			break;

		default:
			WBUG("%d UNKNOWN type %d", i, def->type);
			break;
		}
	}

	return ;
}

void Unifom::convert(FieldObj &fldIn, PacketObj &pac, unsigned int fldOut, bool &negative, Proc_Convert &def)
{
	CODE_TYPE in, out;

	unsigned char *vbuf;

	TEXTUS_LONG result1;
	int result32;
	short result16;
	char result8;

	unsigned int j;
	TEXTUS_LONG len2 =0, i, ii,jj,len;

	if ( negative )
	{
		in = def.dst_type;
		out = def.src_type;
	} else {
		in = def.src_type;
		out = def.dst_type;
	}

	switch ( in )
	{
/* ========================== BCD,BCD0,BCDF   >>>>>  ASCII, INTERGER ============================*/
	case BCD:
	case BCD0:
	case BCDF:
		if ( fldIn.len <= 0 ) break;
		switch ( out )
		{
		/* ========= BCD,BCD0,BCDF   >>>>>  ASCII ============================*/
		case ASCII:
			len = fldIn.range*2;
			pac.grant(len);
			vbuf = pac.buf.point;
			switch ( in )
			{
			case BCD:
				if ( fldIn.len %2 == 0 )
				{	
					vbuf[0] = Obtainx((fldIn.val[0] & 0xF0 ) >> 4 );
					vbuf[1] = Obtainx(fldIn.val[0] & 0x0F );
					len = 2;
				} else	/* 在BCD码个数为奇数时, 第一字节的前半字节被忽略 */
				{
					vbuf[0] = Obtainx(fldIn.val[0] & 0x0F );
					len = 1;
				}
				j = 1;	/* 输入字节从第二个开始 */
				i = fldIn.range;	/* 处理到最后一字节 */
				break;

			default:
				len = 0;/* 输出第一个开始 */
				j = 0;	/* 输入字节从第一个开始 */
				if ( fldIn.len %2 == 0 )
					i = fldIn.range ;
				else
					i = fldIn.range - 1;	/* 最后一字节不处理 */
				break;
			}
			
			for ( ; j < i; j++ )
			{
				vbuf[len] = Obtainx((fldIn.val[j] & 0xF0 ) >> 4 );
				len++;
				vbuf[len] = Obtainx(fldIn.val[j] & 0x0F);
				len++;
			}

			switch ( in )
			{
			case BCD0:
			case BCDF:
				if ( fldIn.len %2 != 0 )
				{
					/* 最后半字节被忽略 */
					vbuf[len] = Obtainx((fldIn.val[j] & 0xF0 ) >> 4 );
					len++;
				}
				break;

			default:
				break;
			}
			
			pac.commit(fldOut, len);
			break;

		/* ============= BCD,BCD0,BCDF   >>>>>  INTERGER =========================*/
		case INTEGER:
			result1 = 0;
			for ( j = 0; j < fldIn.range-1; j++ )
			{
				/* 最后一字节暂不处理, 视长度而定 */
				result1 *= 10;
				result1 += (fldIn.val[j] & 0xF0) >> 4;
				result1 *= 10;
				result1 += fldIn.val[j] & 0x0F ;
			}

			/* j已经指向最后一字节,先把高4位处理掉 */
			result1 *= 10;
			result1 += (fldIn.val[j] & 0xF0) >> 4;
			if ( fldIn.len %2 == 0 || in == BCD  )
			{	/* 对于名义长度为奇数的BCDF、BCD0的情况, 最后半字节被忽略 */
				result1 *= 10;
				result1 += fldIn.val[j] & 0x0F ;
			}
			
			pac.input(fldOut, (unsigned char*)&result1, sizeof(result1));
			break;

		default:
			break;
		}

		break;
/* ============end of === BCD,BCD0,BCDF   >>>>>  ASCII, INTERGER ============================*/

	case ANY:	/* in */
		switch ( out )
		{
		case HEX:
		case HEXCAP:
			len = fldIn.range*2;
			pac.grant(len);

			vbuf = pac.buf.point;
			for (j = 0, i=0 ; j < fldIn.range ; j++)
			{
				vbuf[2*j] =  out == HEX ? ObtainX ((0xF0 & fldIn.val[j] ) >> 4):Obtainx((0xF0 & fldIn.val[j] ) >> 4) ;
				vbuf[2*j+1] = out == HEX ? ObtainX (0x0F & fldIn.val[j]):Obtainx(0x0F & fldIn.val[j]) ;
			}
			pac.commit(fldOut, len);

			break;

		case BASE64:
			len = fldIn.range*2 +8;
			pac.grant(len);

			vbuf = pac.buf.point;
			pac.commit(fldOut,
				BTool::base64_encode((char*) vbuf, (const unsigned char*)fldIn.val, fldIn.range));
			break;
		default:
			break;
		}
		break;

	case BASE64:
		switch ( out )
		{
		case ANY:
			len = fldIn.range;
			pac.grant(len);

			vbuf = pac.buf.point;
			pac.commit(fldOut,
				BTool::base64_decode((const char*)fldIn.val, vbuf, len));

			break;
		default:
			break;
		}
		break;

	case HEX:
	case HEXCAP:
		switch ( out )
		{
		case ANY:
			len = fldIn.range/2;
			pac.grant(len);

			vbuf = pac.buf.point;
			for (i=0 ; i < len ; i++)
			{
				vbuf[i] =  (0x0F & Obtainc(fldIn.val[2*i]) ) << 4;
				vbuf[i] |=  Obtainc(fldIn.val[2*i+1]) & 0x0f ;
			}
			pac.commit(fldOut, len);

			break;
		default:
			break;
		}
		break;

/* ========================== ASCII >>>>>  BCD, BCD0, BCDF, INTERGER ============================*/
	case ASCII:
		switch ( out )
		{
		/* ========== ASCII >>>>>  BCD, BCD0, BCDF ===================*/
		case BCD:
		case BCD0:
		case BCDF:
			len = fldIn.range/2;
			/* 多一字节, 以适应字节数为奇数的情况 */
			if ( fldIn.range %2 != 0 ) len++;
			pac.grant(len);		

			vbuf = pac.buf.point;
			pac.fld[fldOut].len = fldIn.range;		/* 给出名义长度 */

			i = 0;	
			j = 0;
			if ( fldIn.range %2 != 0 )
			{
				switch ( out )
				{
				case BCD:
					vbuf[0] = 0x0F & Obtainc(fldIn.val[0]);	
								/* 对于不为偶数的, 第一位补0,第一字符已处理 */
					i = 1;			/* 指向第二个输出字节 */
					j = 1;			/* 指向第二个输入字符 */
					len2 = len;
					break;

				case BCD0:
					vbuf[len-1] = (0x0F & Obtainc(fldIn.val[fldIn.range-1])) << 4;
								/* 对于不为偶数的, 最后位补0,最后一字符已处理 */
					len2 = len-1;		/* 后续处理不再有最后字符 */
					break;

				case BCDF:
					vbuf[len-1] = 0x0F | ((0x0F & Obtainc(fldIn.val[fldIn.range-1])) << 4);
								/* 对于不为偶数的, 最后位补F,最后一字符已处理 */
					len2 = len-1;		/* 后续处理不再有最后字符 */
					break;

				default:
					break;
				}
			} else
				len2 = len;

			for ( ; i < len2 ; i++)
			{
				vbuf[i] =  (0x0F & Obtainc(fldIn.val[j]) ) << 4;
				j++;
				vbuf[i] |=  Obtainc(fldIn.val[j]) & 0x0F ;
				j++;
			}
			pac.commit(fldOut, len);
			break;

		/* ========== ASCII >>>>>  INTEGER =================*/
		case INTEGER:
			result1 = 0;
			for ( j = 0; j < fldIn.range; j++ )
			{
				if ( fldIn.val[j] == ' ' ) continue;
				result1 *= 10;
				result1 += Obtainc(fldIn.val[j]) ;
			}
			pac.input(fldOut, (unsigned char*)&result1, sizeof(result1));
			break;

		default:
			break;
		}

		break;
/* ========end of ================ ASCII >>>>>  BCD, BCD0, BCDF, INTERGER ============================*/

/* ========================INTERGER >>>>>  BCD, BCD0, BCDF, ASCII ============================*/
	case INTEGER:
		WBUG("convert field %d to field %d from interger to type of %d.", fldIn.no, fldOut, out );
		switch ( fldIn.range ) 
		{
			case 8:
				if ( sizeof(result1) == 8 )
				memcpy(&result1, fldIn.val, 8);
				break;
			case 4:
				memcpy(&result32, fldIn.val, 4);
				result1 = result32;
				break;
			case 2:
				memcpy(&result16, fldIn.val, 2);
				result1 = result16;
				break;
			case 1:
				memcpy(&result8, fldIn.val, 1);
				result1 = result8;
			default:
				WLOG(ERR, "field %d range %d is invalid for interger.", fldIn.no, fldIn.range);
				goto HI_END;
		}
		switch ( out )
		{
		/* ========INTERGER >>>>>  BCD, BCD0, BCDF ============================*/
		case BCD:
		case BCD0:
		case BCDF:
			if (def.len <= 0 ) break;
			len = def.len /2;
			if ( def.len %2 != 0 )
				len++;
			pac.grant(len);
			vbuf = pac.buf.point;
			pac.fld[fldOut].len = def.len; 

			jj = len -1;	/* j指向最后一个输出字节 */
			switch ( out )
			{
			case BCD0:
			case BCDF:
				if ( def.len %2 != 0 )
				{
					/* yes,最后一个输出字节已经处理 */
					vbuf[jj] =((result1%10) & 0x0f) << 4;
					result1 /=10;
					if( out == BCDF )
						vbuf[jj] |= 0x0F;
					jj-- ;
				}
				break;
			default:
				break;
			}

			for ( ii = jj; ii >=0; ii--)
			{
				vbuf[ii] =((result1%10) & 0x0f) << 4;
				result1 /=10;
				vbuf[ii] |= (result1%10) & 0x0f;
				result1 /=10;
			}
			pac.commit(fldOut, len);
			break;

		/* ========INTERGER >>>>>  ASCII ============================*/
		case ASCII:
			if (def.len <= 0 ) break;
			len = (unsigned int) def.len + 1;
			pac.grant(len);
			vbuf = pac.buf.point;
			if ( def.format)
			{
				ii = TEXTUS_SNPRINTF((char*)vbuf, def.len, def.format, result1);
				if ( ii < 0 ) 
				{
					WLOG(ERR, "failed to convert field %d from interger to ascii.", fldIn.no );
				} else
					pac.commit(fldOut, ii);
			} else { 
				for ( ii = len-1; ii >=0; ii--)
				{
					vbuf[ii] = Obtainx(result1%10) ; //实际上是10进制
					result1 /=10;
				}
				for ( i = 0 ; i < len-1; i++)
					if ( vbuf[i] != '0' ) break;
				memmove(vbuf, &vbuf[i], len-i);
				pac.commit(fldOut, len-i);
			}
			break;

		default:
			WLOG(ERR, "failed to convert field %d from interger to %d.", fldIn.no, out );
			break;
		}
	HI_END:
		break;
/* ======end of ==================INTERGER >>>>>  BCD, BCD0, BCDF, ASCII ============================*/

	case ASN1_OBJ:
		switch ( out )
		{
		case ASCII:
			break;
		default :
			break;
		}
		break;

	case IP_ADDR_BIN:
		switch ( out )
		{
		case IP_ADDR_STR:
			len = fldIn.range*4;
			pac.grant(len);
			vbuf = pac.buf.point;
			ii = 0;
			for ( j = 0; j < fldIn.range-1; j++ )
			{
				jj = TEXTUS_SNPRINTF((char*)vbuf, len-ii, "%d.", fldIn.val[j]);
				ii += jj;
				vbuf += jj;
			}
			ii += TEXTUS_SNPRINTF((char*)vbuf, len-ii, "%d", fldIn.val[j]);
			pac.commit(fldOut, ii);
			
			break;
		default :
			break;
		}
		break;

	case IP_ADDR_STR:
		switch ( out )
		{
		case IP_ADDR_BIN:
			len = 1;
			for ( j = 0; j < fldIn.range; j++ )	/* 有多少个".", 即有多少个(+1)字节 */
				if ( fldIn.val[j] == '.' ) len++;
			pac.grant(len);
			vbuf = pac.buf.point;
			ii = 0; len = 0;
			for ( j = 0; j <= fldIn.range; j++ )
			{
				if ( fldIn.val[j] == '.' || j==fldIn.range ) 
				{
					char hbuf[5];
					if ( j - ii > 3 )
						goto END_TO_BIN;
					memcpy(hbuf,   &fldIn.val[ii], j-ii);
					hbuf[j-ii] = '\0';
					*vbuf = (char ) ((atoi(hbuf)) & 0xff );
					vbuf++; len++;
					ii = j+1;
				}
			}
			pac.commit(fldOut, len);
			
	END_TO_BIN:
			break;
		default :
			break;
		}
		break;
	default:
		break;
	}
}

void Unifom::domap(FieldObj &fldIn, PacketObj &pac, unsigned int out, bool &negative, Proc_Map &def)
{
	int num, i;
	struct With_Map *pmap;
	unsigned char *from, *to;
	unsigned int flen, tlen;

	num = def.mapNum;
	if ( num <= 0) 
		return ;
	
	for ( i = 0, pmap = &def.map[0]; i < num; i++, pmap++)
	{
		if ( !negative )
		{
			from = pmap->from;
			to = pmap->to;
			flen = pmap->len_from;
			tlen = pmap->len_to;
		} else {
			from = pmap->to;
			to = pmap->from;
			flen = pmap->len_to;
			tlen = pmap->len_from;
		}

		if ( from == (unsigned char*) 0 
			|| ( fldIn.range == flen && memcmp(fldIn.val, from, flen) == 0 ) ) 
		{
			pac.input(out, to, tlen);
			break;
		}
	}
	
}

void Unifom::do_obu_sn(FieldObj &fldIn, PacketObj &pac, unsigned int out, bool &negative,  Proc_ObuSN &def)
{
	int num, i;
	struct With_Map *pmap;
	unsigned char *from, *to;
	unsigned int flen;
	unsigned char o_sn[128], n_sn[128], fac_type[2];
	unsigned TEXTUS_LONG o_len, n_len;
	
	num = def.factory_map.mapNum;
	if ( num <= 0) 
		return ;

	o_len = fldIn.range;
	if ( o_len < 16 || o_len > 100 ) 
		return ;

	memcpy(o_sn, fldIn.val, fldIn.range);	//保存一下旧的表面编号及其它数据
	memcpy(&fac_type[0], &o_sn[6], 2);

	for ( i = 0, pmap = &(def.factory_map.map[0]); i < num; i++, pmap++)
	{
		if ( !negative )
		{
			from = pmap->from;
			to = pmap->to;
			flen = pmap->len_from;
		} else {
			from = pmap->to;
			to = pmap->from;
			flen = pmap->len_to;
		}

		if ( from == (unsigned char*) 0 
			|| ( 2 == flen && memcmp(&fac_type[0], from, flen) == 0 ) ) 
		{
			/* 厂商与类别对上了，下面作处理 */
			int obu_serial, j;
			char obu_hex[16];
	
			obu_serial = 0;
			for ( j = 8; j < 16; j++ )
			{
				obu_serial *= 10;
				obu_serial += Obtainc(o_sn[j]) ;
			}
			obu_serial = obu_serial%20000000;	//最高位对2取模, 如果这里有批次的话?			TEXTUS_SPRINTF(obu_hex, "%06X", obu_serial);
			memcpy(&n_sn[0], &o_sn[0], 8);
			memcpy(&n_sn[8], to, 2);		//16进制的厂商代码
			memcpy(&n_sn[10], &obu_hex[0], 6);	//16进制的序号
			memcpy(&n_sn[16], &o_sn[16], o_len-16);	//其它的补上
			n_len = o_len;
			pac.input(out, &n_sn[0], n_len);	//给到新域,也许就是原来的域
			break;
		}
	}
}

void Unifom::dotrim(FieldObj &fldIn, PacketObj &pac, unsigned int out, bool &negative, Proc_Trim &def)
{
	unsigned TEXTUS_LONG len,i;
	unsigned char *vbuf;
	if ( !negative )
	{	/* 去掉或前或后的字符 */
		len = fldIn.range;
		if ( def.method == 1 )	/* 前补 */
		{
			i = 0;
			while ( i < len && fldIn.val[i] == def.c) i++;
			pac.input(out, &fldIn.val[i], len-i);
		} else if ( def.method == 0 )	/* 后补 */
		{
			i = len;
			while ( i > 0 && fldIn.val[len-i-1] == def.c) i--;
			pac.input(out, fldIn.val, len-i);
		}
	} else {
		unsigned TEXTUS_LONG sp ;
		if ( (def.len <=  (int ) fldIn.range && def.len != 0 ) 
			|| (def.len == 0 && fldIn.range % 2 == 0) 
		)
		{	/* 如果实际数据已经超出预定长度, 
			或者def.len ==0 表示补成偶数, 而数据已经是偶数长
			那么, 实际数据输出即可.
			 */
			pac.input(out, fldIn.val, fldIn.range);
			return ;
		}
		
		if ( def.len == 0 )	/* 补成偶数位 */
		{	/* fldIn.range % 2 != 0 */
			len = fldIn.range +1;	/* 肯定是奇数长度, 要不前面就处理过了 */
		} else 
			len = def.len ;

		pac.grant(len);		/* 先保证有那么多空间 */
		vbuf = pac.buf.point;

		sp = len - fldIn.range;	/* 算出要补多少个字节 */
		if ( def.method == 1 )	/* 前补 */
		{
			memset(vbuf, def.c, sp);
			memcpy(&vbuf[sp], fldIn.val, fldIn.range);
		} else if ( def.method == 0 )	/* 后补 */
		{
			memset(&vbuf[fldIn.range], def.c, sp);
			memcpy(vbuf, fldIn.val, fldIn.range);
		} else
			return;	/* 如果没有定义, 最终数据并未输出 */
		pac.commit(out, def.len);
	}
}

void Unifom::dosubstr(FieldObj &fldIn, PacketObj &pac, unsigned int out, Proc_Substr &def)
{
	if ( fldIn.range > def.begin )
	{
		unsigned TEXTUS_LONG rest = fldIn.range - def.begin;
		unsigned TEXTUS_LONG r_len; 
		if ( def.len == 0 )
			r_len = rest;
		else
			r_len = def.len > rest ? rest : def.len;

		pac.input(out, &fldIn.val[def.begin], r_len);
	}
}

void Unifom::donow(FieldObj *fldIn, PacketObj &pac, unsigned int out, Proc_Now &def)
{
    	struct tm *tdatePtr;
	
	char timebuf[256];
	struct timeb now;

#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
	struct tm tdate;
#endif

	ftime(&now);
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
	tdatePtr = &tdate;
	localtime_s(&tdate, &now.time );
#else
	tdatePtr = localtime( &now.time );
#endif
	if (def.format )
	{
		char millstr[64];
    		(void) strftime( timebuf, sizeof(timebuf), def.format, tdatePtr );
		if ( def.use_milli)
		{
			TEXTUS_SPRINTF(millstr, ".%03d", now.millitm);
			TEXTUS_STRCAT(timebuf, millstr);
		}
		pac.input(out, (unsigned char*)&timebuf[0], strlen(timebuf));
	} else {
		pac.input(out, (unsigned char*)&(now.time),  sizeof(now.time));
	}
}

void Unifom::do_getLen(FieldObj *fldIn, PacketObj &pac, unsigned int out, Proc_GetLength &def)
{
	char buf[128];
	pac.grant(128);	//最终输出不过128字节
	TEXTUS_SPRINTF(buf, def.format, fldIn->range/def.unit + def.adjust);
	pac.input(out, (unsigned char*)buf, (unsigned TEXTUS_LONG)strlen(buf));
}

void Unifom::dolet(FieldObj *fldIn, PacketObj &pac, unsigned int out, Proc_Let &def)
{
	if ( !fldIn )
	{	/* 定义中没有设定source域号 */
		pac.input(out, def.src, def.src_len);
	} else if ( fldIn->no >= 0 ) {
		switch (def.method )
		{
		case GETRAW:
			pac.input(out, fldIn->raw, fldIn->_rlen);
			break;

		case GETVAL:
			pac.input(out, fldIn->val, fldIn->range);
			break;

		default:
			break;
		}
	} /* else; 定义某个域, 但这个域无值, 赋值就没有发生 */
}

void Unifom::duplicate(PacketObj &pac_in, PacketObj &pac_out)
{
	int i;
	pac_out.reset();
	for ( i = 0 ; i < pac_in.max; i++ )
	{
		if ( pac_in.fld[i].no >=0 ) 
			pac_out.input(pac_in.fld[i].no, pac_in.fld[i].val, pac_in.fld[i].range);
	}
}

/* 向接力者提交 */
PACINLINE void Unifom::deliver(Notitia::HERE_ORDO aordo)
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
