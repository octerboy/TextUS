/* Copyright (c) 2016-2018 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 Title:PacWay
 Build: created by octerboy, 2016/04/01 Guangzhou
 $Header: /textus/PacWay.cpp 48    16-08-04 9:01 Test $
*/

#define SCM_MODULE_ID  "$Workfile: PacWay.cpp $"
#define TEXTUS_MODTIME  "$Date: 16-08-04 9:01 $"
#define TEXTUS_BUILDNO  "$Revision: 48 $"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "PacData.h"
#include "BTool.h"
#include "casecmp.h"
#include "textus_string.h"
#include "textus_load_mod.h"
#include "insext.h"

#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <openssl/md5.h>
#define MINLINE inline
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

int squeeze(const char *p, char q[])	//把空格等挤掉, 只留下16进制字符(大写), 返回实际的长度
{
	int i;
	i = 0;
	while ( *p )
	{ 
		if ( isxdigit(*p) ) 
		{
			q[i] = toupper(*p);
			i++;
		} else if ( !isspace(*p))  {
			q[i] = *p;
			i++;
		}
		p++;
	}
	q[i] = '\0';
	return i;
};

#define ERR_RESULT_INVALID -98
#define ERR_HSM_AGAIN -99
#define ERROR_MK_PAUSE -100
#define ERROR_MK_DEV -101
#define ERROR_IC_INS -102
#define ERROR_MK_TCP -103
#define ERROR_MK_RPC -104
#define ERROR_HSM_RPC -111
#define ERROR_HSM_FUN -112
#define ERROR_HSM_TCP -114
#define ERROR_INS_DEF -133
#define ERROR_INS_NOT_SW -135
#define ERROR_OTHER -200

unsigned short SW_OK=0x9000;	//SW1 SW2 = 9000, 这么做, 可能在其它机器上, 顺序不一样
char err_global_str[128]={0};
/* 左边状态, 空闲, 等着新请求, 初始化中, 制卡中 */
enum LEFT_STATUS { LT_IDLE = 0, LT_INITING = 2, LT_MKING = 3};

/* 右边状态, 空闲, IC指令发出, HSM指令发出, 终端测试中 */
enum RIGHT_STATUS { RT_IDLE = 0, RT_TERM_TEST = 1, RT_HSM_ASK = 2, RT_IC_COM=3, RT_TERM_FEED=4, 
		RT_TERM_PROMPT=5, RT_IC_RESET=6, RT_TERM_OUT=8, RT_INS_READY = 12};

	struct TermOPBase {
		const char *give;
		const char *take;
		const char *fun;
		int tlen;
		int glen;
		RIGHT_STATUS rt_stat;
		void me()	
		{
			glen = strlen(give);
			tlen = strlen(take);
		};
	};

	struct FeedCardOp:public TermOPBase {
		FeedCardOp() {
			give="F";
			take="f";
			fun = "pro_feed";
			rt_stat = RT_TERM_FEED;
			me();
		};
		void set () { };
	};

	struct OutCardOp :public TermOPBase {
		OutCardOp () {
			give="O";
			take="o";
			fun = "pro_out";
			rt_stat = RT_TERM_OUT;
			me();
		}
		void set () {	};
	};

	struct ProRstOp :public TermOPBase {
		ProRstOp ()	{
			give="R";
			take="r";
			fun = "pro_reset";
			rt_stat = RT_IC_RESET;
			me();
		}
		void set () {	};
	};

	struct PromptOp :public TermOPBase {		//进度提示操作
		char cent[16];
		int len;
		PromptOp () {
			give="P";
			take="p";
			fun = "pro_prompt";
			rt_stat = RT_TERM_PROMPT;
			me();			
		}
		void set (int icent) 
		{
			TEXTUS_SPRINTF(cent, "%d", icent);
			len = strlen(cent);
		};
	};

	struct ProComOp :public TermOPBase {
		ProComOp () {
			give="C";
			take="c";
			fun = "pro_com";
			rt_stat = RT_IC_COM;
			me();
		}
		void set () {	};
	};

/* 包括SysTime这样的变量，都由外部函数计算，所以这里只保留脚本指纹数据 */	
#define Pos_FlowPrint 1 
#define Pos_TotalIns 2 
#define Pos_Fixed_Next 3  //下一个动态变量的位置, 也是为脚本自定义动态变量的第1个位置

	struct PVar: public PVarBase 
	{
		int start_pos;		//从输入报文中, 什么位置开始
		int get_length;		//取多少长度的值
		int source_fld_no;	//来源域号。
		int dest_fld_no;	//目的域号, 

		TiXmlElement *me_ele;	/* 自身, 其子元素包括两种可能: 1.函数变量表, 
					2.一个指令序列, 在指令子元素分析时, 如发现一个用到的变量中, 有子序列时, 把这些指令嵌入。
				*/

		PVar () {
			kind = VAR_None;	//起初认为非变量
			name = 0;
			n_len = 0;
			c_len = 0;
			memset(content,0,sizeof(content));

			start_pos = 1;
			get_length = 999 ;
			source_fld_no = -1;
			dest_fld_no = -1;
			dynamic_pos = -1;	//非动态类
			me_ele = 0;
		};

		void put_still(const char *val, unsigned int len=0)
		{
			if ( !val)
				return ;
			if ( len ==0 ) 
				c_len = strlen(val);
			else
				c_len = len;
				
			if ( (c_len+1) > (int)sizeof(content) ) c_len =  sizeof(content)-1;
			memcpy(content, val, c_len);
			content[c_len] = 0;
			kind = VAR_Constant;	//认为是常数
		};

		struct PVar* prepare(TiXmlElement *var_ele, int &dy_at) //变量准备
		{
			const char *p, *dy, *nm;
			int i;
			kind = VAR_None;
			me_ele = var_ele;

			nm = var_ele->Attribute("name");
			if ( !nm ) return 0;

			name = nm;
			n_len = strlen(name);

			p = var_ele->GetText();
			if ( p) 
			{	
				c_len = squeeze(p, content);
			} 
			/* 变量无内容, 才认为这是特殊变量 */
			var_ele->QueryIntAttribute("from", &(source_fld_no));
			var_ele->QueryIntAttribute("to", &(dest_fld_no));
			var_ele->QueryIntAttribute("start", &(start_pos));
			var_ele->QueryIntAttribute("length", &(get_length));

			if ( strcasecmp(nm, "$ink" ) == 0 ) 
			{
				dynamic_pos = Pos_FlowPrint;
				kind = VAR_FlowPrint;
			}
			if ( strcasecmp(nm, "$total" ) == 0 ) 
			{
				dynamic_pos = Pos_TotalIns;
				kind = VAR_TotalIns;
			}

			if ( kind != VAR_None) goto P_RET; //已有定义，不再看这个Dynamic, 以上定义都与Dynamic相同处理

			dy = var_ele->Attribute("dynamic");
			if ( dy )
			{
				if ( strcasecmp(dy, "yes") == 0 ) 
				{
					dynamic_pos = dy_at;	//动态变量位置
					kind = VAR_Dynamic;
					dy_at++;
				} else if ( strcasecmp(dy, "refer") == 0 ) 
				{
					kind = VAR_Refer;
				}
			}

			if ( kind == VAR_None && p) 
			{	//这里有内容但还没有类型, 那就定为常数, 其它的也可以有内容, 就要别处定义了
				kind = VAR_Constant;	//认为是常数
			}

			P_RET:
			return this;
		};

		void put_herev(TiXmlElement *h_ele) //分析一下本地变量
		{
			const char *p;

			if( (p = h_ele->GetText()) )
			{
				c_len = squeeze(p, content);
				if (kind == VAR_None)	//只有原来没有定义类型的, 这里才定成常数. 
					kind = VAR_Constant;
			}

			h_ele->QueryIntAttribute("from", &(source_fld_no));
			h_ele->QueryIntAttribute("to", &(dest_fld_no));
			h_ele->QueryIntAttribute("start", &(start_pos));
			h_ele->QueryIntAttribute("length", &(get_length));
		};
	};

	struct DyVar: public DyVarBase {	//动态变量
		int dest_fld_no;	//目的域号, 
		struct PVar *def_var;

		DyVar () {
			kind = VAR_None;
			index = - 1;
			c_len = 0;
			dest_fld_no = -1;
			memset(val, 0, sizeof(val));
		};

		void input(const char *p, int len)
		{
			if ( (len+1) < (int)sizeof(val) )
			{
				memcpy(val, p, len);
				c_len = len;
				val[len] = 0;
			}
		};
		void input(int iv)
		{
			TEXTUS_SPRINTF(val, "%d", iv);
			c_len = sizeof(iv);
			val[c_len] = 0;
		};
	};

	struct MK_Session {		//记录一个制卡过程中的各种临时数据
		struct DyVar *snap;//随时的变量, 包括
		int snap_num;
		char err_str[1024];	//错误信息
		char station_str[1024];	//工作站信息

		char flow_id[64];
		int pro_order;		//当前处理的操作序号
		char bad_sw[32];

		LEFT_STATUS left_status;
		RIGHT_STATUS right_status;
		int ins_which;		//已经工作在哪个指令, 即为定义中数组的下标值
		int iRet;		//制卡工作结果

		inline MK_Session ()
		{
			snap=0;
		};

		inline void init(int m_snap_num) //这个m_snap_num来自各XML定义的最大动态变量数
		{
			if ( snap )
				return ;
			if ( m_snap_num <=0 ) 
				return ;
			snap_num = m_snap_num;
			snap = new struct DyVar[snap_num];
			for ( int i = 0 ; i < snap_num; i++)
			{
				snap[i].index = i;
			}
			snap[Pos_FlowPrint].kind = VAR_FlowPrint;
			snap[Pos_TotalIns].kind = VAR_TotalIns;
			reset();
		};

		~MK_Session ()
		{
			if ( snap ) delete[] snap;
			snap = 0;
		}

		inline void  reset() 
		{
			int i;
			for ( i = 0; i < snap_num; i++)
			{
				snap[i].c_len = 0;
				snap[i].val[0] = 0;
			}
			for ( i = Pos_Fixed_Next ; i < snap_num; i++)
			{
				snap[i].kind = VAR_None;/* 这个Pos_Fixed_Next很重要, 要不然, 那些UID等固有的动态变量会没有的！  */
				snap[i].def_var = 0;
			}
			left_status = LT_IDLE;
			right_status = RT_IDLE;
			ins_which = -1;
			bad_sw[0] = 0;
			err_str[0] = 0;	
			flow_id[0] = 0;
		};
	};

/* 变量集合*/
struct PVar_Set {	
	struct PVar *vars;
	int many;
	int dynamic_at;
	char var_nm[16];
	PVar_Set () 
	{
		vars = 0;
		many = 0;
		dynamic_at = Pos_Fixed_Next; //0,等 已经给$FlowPrint等占了
		memcpy(var_nm, "Variable", 8);
		var_nm[8] = 0;
	};

	~PVar_Set () 
	{
		if (vars ) delete []vars;
		vars = 0;
		many = 0;
	};
	bool is_var(const char *nm)
	{
		if (nm  && strlen(nm) == 8 && memcmp(nm, var_nm, 8) == 0 )
			return true;
		return false;
	}

	void defer_vars(TiXmlElement *map_root, TiXmlElement *icc_root=0) //分析一下变量定义
	{
		TiXmlElement *var_ele, *i_ele;
		const char *vn = &var_nm[0], *nm;
		bool had_nm;
		int vmany ;

		many = 0;
		for (var_ele = map_root->FirstChildElement(vn); var_ele; var_ele = var_ele->NextSiblingElement(vn) ) 
			many++;

		if ( icc_root)
		for (i_ele = icc_root->FirstChildElement(vn); i_ele; i_ele = i_ele->NextSiblingElement(vn) ) 
		{
			nm = i_ele->Attribute("name");
			had_nm = false;
			for (var_ele = map_root->FirstChildElement(vn); var_ele; var_ele = var_ele->NextSiblingElement(vn) ) 
			{
				if ( strcmp(nm, var_ele->Attribute("name") ) == 0 )
				{
					had_nm = true;
					break;
				}
			}
			if ( !had_nm )	//如果没有已定义的名, 才增加
				many++;
		}

		vars = new struct PVar[many];
		vmany = 0;
		for (var_ele = map_root->FirstChildElement(vn); var_ele; var_ele = var_ele->NextSiblingElement(vn) ) 
		{
			if ( vars[vmany].prepare(var_ele, dynamic_at) )
				vmany++;
		}

		if ( icc_root)
		for (i_ele = icc_root->FirstChildElement(vn); i_ele; i_ele = i_ele->NextSiblingElement(vn) ) 
		{
			nm = i_ele->Attribute("name");
			had_nm = false;
			for (var_ele = map_root->FirstChildElement(vn); var_ele; var_ele = var_ele->NextSiblingElement(vn) ) 
			{
				if ( strcmp(nm, var_ele->Attribute("name") ) == 0 )
				{
					had_nm = true;
					break;
				}
			}
			if ( !had_nm )	//如果没有已定义的名, 才增加
			{
				if ( vars[vmany].prepare(i_ele, dynamic_at) )
					vmany++;
			}
		}

		if ( vmany > many ) printf("bug!!!!!!! var_many prior %d less than post %d\n", many, vmany);
		many = vmany;		//最后再更新一次变量数
	};

	struct PVar *look( const char *n, struct PVar_Set *loc_v=0)	//看看是否为一个定义的变量名, 先查loc_v这个局部变量集
	{
		int d_len;
		struct PVar *rvar =0;

		if ( !n)  return 0;
		if ( loc_v ) 
		{
			rvar = loc_v->look(n);
			if ( rvar )
				return rvar;
		}

		d_len = strlen(n);
		for ( int i = 0 ; i < many; i++)
		{
			if ( d_len == vars[i].n_len)
			{
				if (memcmp(vars[i].name, n, d_len) == 0 ) 
					return &(vars[i]);
			}
		}
		return 0;
	};

	void put_still(const char *nm, const char *val, unsigned int len=0)
	{
		struct PVar *av = look(nm,0);
		if ( av) av->put_still(val, len);
	};

	char *get_value(const char *nm)
	{
		struct PVar *av = look(nm);
		if ( av )
			return &(av->content[0]);
		else 
			return 0;
	};

	/* 找静态的变量, 获得实际内容 */
	struct PVar *one_still( const char *nm, char buf[], int &len, struct PVar_Set *loc_v=0)
	{
		struct PVar  *vt;
		/* 在这时两种情况处理, 一个是有静态常数定义的, 另一个静态常数变量 */
		vt = look(nm, loc_v);	//看看是否为一个定义的变量名
		if ( !vt) 
		{
			len = squeeze(nm, buf); //非定义变量名, 这里直接处理了, 本身就是一个常数
			goto VARET;
		}

		len = 0;
		switch (vt->kind)
		{
		case VAR_Constant:	//静态常数变量
			len = vt->c_len;
			memcpy(buf, vt->content, len);
			break;

		default:
			len = 0;
			break;
		}
		VARET:
		buf[len] = 0;	//结束NULL
		return vt;
	};

	/* nxt 下一个变量, 对于多个tag元素，将之静态内容合成到 一个变量command中。对于非静态的，返回该tag元素是个动态变量 */
	struct PVar *all_still( TiXmlElement *ele, const char*tag, char command[], int &ac_len, TiXmlElement *&nxt, struct PVar_Set *loc_v=0)
	{
		TiXmlElement *comp = ele;
		int l;
		struct PVar  *rt;
				
		rt = 0;
		/* ac_len从参数传进, 累计的, command就是原来的好了, 不用重设指针 */
		while(comp)
        	{
			rt = one_still( comp->GetText(), &command[ac_len], l, loc_v);
			ac_len += l;
			comp = comp->NextSiblingElement(tag);
			if ( rt && rt->kind < VAR_Constant )		//如果有非静态的, 这里先中断
				break;
		}
		command[ac_len] = 0;	//结束NULL
		nxt = comp;	//指示下一个变量
		return rt;
	};

	/* 从一行（有动静态变量的，或数据），获得实时的内容　*/
	struct PVar *get_var_one(const char *nm, char buf[], int &len, struct MK_Session *sess, struct PVar_Set *loc_v=0)
	{
		struct PVar  *vt;
		struct DyVar *dvr;
				
		vt = look(nm, loc_v);	//看看是否为一个定义的变量名
		if ( !vt) 
		{
			len = squeeze(nm, buf); //非定义变量名, 这里直接处理了
			goto VARET;
		}

		len = 0;
		if (vt->kind <= VAR_Dynamic )
		{
			dvr = &(sess->snap[vt->dynamic_pos]);
			if ( dvr->kind == vt->kind )
			{
				len = dvr->c_len;
				memcpy(buf, dvr->val, len);
			}
		} else 	if ( vt->kind == VAR_Constant )
		{
			len = vt->c_len;
			memcpy(buf, vt->content, len);
		}
	VARET:
		buf[len] = 0;	//结束NULL
		return vt;
	};

	/* 对于多个tag元素，将实时的内容合成到 一个变量command中 */
	struct PVar *get_var_all( TiXmlElement *ele, const char*tag, char command[], int &tlen, struct MK_Session *sess, struct PVar_Set *loc_v=0)
	{
		TiXmlElement *comp = ele;
		int cl;
		struct PVar  *rt;
				
		rt = 0; tlen = 0;
		while(comp)
        	{
			rt = get_var_one( comp->GetText(), &command[tlen], cl, sess, loc_v);
			tlen += cl;
			comp = comp->NextSiblingElement(tag);
		}
		command[tlen] = 0;	//结束NULL
		return rt;
	};

	int get_neo_dynamic_pos ()
	{
		dynamic_at++;
		return (dynamic_at-1);
	};
};
/* 指令分两种，一种是从报文定义而来，即INS_Ori，还有一种是从INS_Ori的组合而来，即INS_User */
enum Command_Type { INS_None = 0, INS_Ori=1, INS_User=2};
	 
struct DyList {
	char *con;
	int len;
	int dy_pos;
	DyList ()
	{
		con  =0;
		len = 0;
		dy_pos = -1;
	};
};

struct SwBase {
	unsigned short another_sw;	//除了9000之外， 另一个可允许的sw1sw2
	unsigned short another_sw2;	//除了9000之外， 另一个可允许的sw1sw2
	unsigned short not_sw;		//不允许的SW, 包括9000

	const char *another_sw_str;	//除了9000之外， 另一个可允许的sw1sw2
	const char *another_sw2_str;	//除了9000之外， 另一个可允许的sw1sw2
	const char *not_sw_str;		//不允许的SW, 包括9000
	SwBase () {};

	const char *get_sw(const char *p, unsigned short &sw)
	{
		const char *q=p;
		if ( p )
		{
			unsigned char hi,lo;
			hex2byte((unsigned char*)&hi, sizeof(hi), p); 
			p++; p++;
			hex2byte((unsigned char*)&lo, sizeof(lo), p);
			sw = hi << 8 | lo;
		}
		return q;
	};

	void allow_sw ( TiXmlElement *ele,  struct PVar_Set *loc_v=0)
	{
		another_sw = SW_OK;
		another_sw2 = SW_OK;
		not_sw = 0;
		another_sw_str = 0;
		another_sw2_str = 0;
		not_sw_str = 0;
		if ( loc_v ) 
		{
			another_sw_str = get_sw(loc_v->get_value(ele->Attribute("sw")), another_sw);
			another_sw2_str = get_sw(loc_v->get_value(ele->Attribute("sw2")), another_sw2);
			not_sw_str = get_sw(loc_v->get_value(ele->Attribute("not_sw")), not_sw);
		} 
		if (!another_sw_str) another_sw_str = get_sw(ele->Attribute("sw"), another_sw);
		if (!another_sw2_str) another_sw2_str = get_sw(ele->Attribute("sw2"), another_sw2);
		if (!not_sw_str) not_sw_str = get_sw(ele->Attribute("not_sw"), not_sw);
	};
		
	bool valid_sw ( unsigned short sw)
	{
		if ( sw == not_sw || (sw != SW_OK && sw != another_sw && sw != another_sw2 ) ) 
			return false;

		return true;
	};
};

	struct PlainReply {
		int dynamic_pos;	//动态变量位置, -1表示静态
		int start;
		int length;
		PlainReply () {
			dynamic_pos = -1;
			start =1;
			length = 500;
		};
	} ;

/* 下面这段匹配应该是不需要变的 */
struct MatchDst {	//匹配目标
	struct PVar *dst;
	const char *con_dst;
	int len_dst;
	void set_val(struct PVar_Set *var_set, struct PVar_Set *loc_v, const char *p)
	{
		dst = var_set->look(p, loc_v);
		if (!dst )
		{
			if ( p) 
			{	//变量名可能没有内容, 这里有内容是常数了, 其它的也可以有内容, 就要别处定义了
				con_dst = p;
				len_dst = strlen(con_dst);
			}
		} 
	};

	bool valid_val (MK_Session *sess, struct PVar *src)
	{
		bool ret=true;

		struct DyVar *dvr;
		char *src_con, *dst_con;
		int src_len, dst_len;

		if ( src->dynamic_pos >= 0 )
		{
			dvr = &sess->snap[src->dynamic_pos];
			src_con = &dvr->val[0];
			src_len = dvr->c_len;
		} else {
			src_con = &src->content[0];
			src_len = src->c_len;
		}
		if ( dst )
		{
			if ( dst->dynamic_pos >= 0 ) 
			{
				dvr = &sess->snap[dst->dynamic_pos];
				dst_con = &dvr->val[0];
				dst_len = dvr->c_len;
			} else {
				dst_con = &dst->content[0];
				dst_len = dst->c_len;
			}
		} else {
			dst_con = (char*)con_dst;
			dst_len = len_dst;
		}
		if ( dst_len == src_len && memcmp(dst_con, src_con, src_len) == 0 ) 
			ret = true;
		else 
			ret = false;

		return ret;
	};
};

struct Match {		//一个匹配项
	struct PVar *src;
	struct MatchDst *dst_arr;
	int dst_num;
	bool ys_no;
	Match () {
		src = 0;
		dst_arr = 0;
		ys_no = true;
	};

	struct Match* set_ma(TiXmlElement *mch_ele, struct PVar_Set *var_set, struct PVar_Set *loc_v=0)
	{
		int i;
		const char *nm, *p;
		const char *vn="value";
		TiXmlElement *con_ele;

		nm= mch_ele->Attribute("name");
		src = var_set->look(nm, loc_v);	//两个变量集
		if (!src ) 
			return 0;
		
		if (strstr(mch_ele->Value(), "not"))
			ys_no = false;
		else
			ys_no = true;

		dst_num = 0;
		for (con_ele = mch_ele->FirstChildElement(vn); con_ele; con_ele = con_ele->NextSiblingElement(vn) ) 
			dst_num++;

		if ( dst_num == 0 )	//一个子value元素也没有
		{
			dst_num = 1;
			dst_arr = new struct MatchDst;
			p = mch_ele->GetText();
			dst_arr->set_val(var_set, loc_v, p);
		} else {
			dst_arr = new struct MatchDst[dst_num];
			for (	i = 0, con_ele = mch_ele->FirstChildElement(vn); 
				con_ele; 
				con_ele = con_ele->NextSiblingElement(vn),i++ ) 
			{
				p = con_ele->GetText();
				dst_arr[i].set_val(var_set, loc_v, p);
			}
		}
		return this;
	};

	bool will_match(MK_Session *sess)
	{
		int i;
		bool ret=true;
		for(i = 0; i < dst_num; i++)
		{
			ret = dst_arr[i].valid_val(sess, src);
			if ( !ys_no ) ret = !ret; 
			if ( !ret ) break;
		}
		return ret;
	};
};

struct Condition {	//一个指令的匹配列表, 包括条件与结果的匹配
	int con_num;
	int res_num;
	struct Match *conie_list;
	struct Match *result_list;
	
	Condition () { 
		con_num = 0; 
		conie_list= 0;
		res_num = 0; 
		result_list= 0;
	};

	void set_list( TiXmlElement *ele, struct PVar_Set *var_set, struct PVar_Set *loc_v, const char *vn, const char *vn_not, int &m_num, struct Match *&list)
	{
		TiXmlElement *con_ele;
		m_num = 0;
		for (con_ele = ele->FirstChildElement(vn); con_ele; con_ele = con_ele->NextSiblingElement(vn) ) 
			m_num++;

		for (con_ele = ele->FirstChildElement(vn_not); con_ele; con_ele = con_ele->NextSiblingElement(vn_not) ) 
			m_num++;

		if ( m_num == 0 ) return;
		list = new struct Match[m_num];
		m_num=0;
		for (con_ele = ele->FirstChildElement(); con_ele; con_ele = con_ele->NextSiblingElement() ) 
		{
			if ( strcmp( con_ele->Value(), vn ) ==0 
				|| strcmp( con_ele->Value(), vn_not ) ==0 )
			{
				if ( list[m_num].set_ma(con_ele, var_set, loc_v) )
					m_num++;
			}
		}
	};

	void set_condition ( TiXmlElement *ele, struct PVar_Set *var_set, struct PVar_Set *loc_v)
	{
		set_list(ele, var_set, loc_v, "if", "if_not", con_num, conie_list);
		set_list(ele, var_set, loc_v, "must", "must_not", res_num, result_list);
	};
		
	bool valid_list(MK_Session *sess, struct Match *list, int l_num)
	{
		int i;
		bool ret=true;
		for(i = 0;i < l_num; i++)
		{
			ret = list[i].will_match(sess);
			if ( !ret ) 
				break;
		}
		return ret;
	};

	bool valid_condition (MK_Session *sess)
	{
		return valid_list(sess, conie_list, con_num);
	};
	bool valid_result (MK_Session *sess)
	{
		return valid_list(sess, result_list, res_num);
	};
};

struct CmdBase:public Condition  {
	int dy_num;
	struct DyList *dy_list;
	bool dynamic ;		//是否动态
	
	const char *tag;
	TiXmlElement *component;	//指令文档中的第一个component元素

	char cmd_buf[2048];
	int cmd_len;
	struct PlainReply *vres;	//响应所存放的变量组
	int reply_num;

	CmdBase () {
		dy_num = 0;
		dy_list = 0;
		vres = 0;
		reply_num = 0;
		dynamic = false;
	};

	void  get_current( char *buf, int &len, MK_Session *sess)
	{	/* 取实时的指令内容 */
		int i;
		struct DyVar *dvr;
		char *p=buf;
		len = 0;
		if ( !dynamic ) 
		{
			memcpy(buf, cmd_buf, cmd_len);
			len = cmd_len;
			goto G_RET;
		}
		for ( i = 0 ; i < dy_num; i++ )
		{
			struct DyList *dl = &dy_list[i];
			if ( dl->dy_pos >=0 ) 
			{
				dvr = &(sess->snap[dl->dy_pos]);
				memcpy(p, dvr->val, dvr->c_len);
				p += dvr->c_len;
				len += dvr->c_len;
			} else {
				memcpy(p, dl->con, dl->len);
				p += dl->len;
				len += dl->len;
			}
		}
	 G_RET:
		buf[len] = 0;
	};

	void set_cmd ( TiXmlElement *ele, struct PVar_Set *var_set, struct PVar_Set *o_set=0)
	{
		struct PVar *vr_tmp=0;
		TiXmlElement *e_tmp, *n_ele;
		const char *p;
		char *cp;

		TiXmlElement *rep_ele;
		const char *vn="reply";
		int r_i = 0;

		reply_num = 0; vres = 0;
		for (rep_ele = ele->FirstChildElement(vn); rep_ele; rep_ele = rep_ele->NextSiblingElement(vn) ) 
			reply_num++;

		vres = new struct PlainReply[reply_num]; r_i = 0;
		for (rep_ele = ele->FirstChildElement(vn); rep_ele; rep_ele = rep_ele->NextSiblingElement(vn) ) 
		{
			if ( (p = rep_ele->Attribute("name")) )
			{
				vr_tmp = var_set->look(p, o_set);	//响应变量, 动态变量, 两个变量集
				if (vr_tmp) 
				{
					vres[r_i].dynamic_pos = vr_tmp->dynamic_pos;
					rep_ele->QueryIntAttribute("start", &(vres[r_i].start));
					rep_ele->QueryIntAttribute("length", &(vres[r_i].length));
				}
			}
			r_i++;
		}

		tag = "component";
		component = ele->FirstChildElement(tag); 
		cmd_len = 0;
		cmd_buf[0] = 0;
		dy_num = 0;
		n_ele = e_tmp = component;
		while ( e_tmp ) 
		{
			vr_tmp= var_set->all_still( e_tmp, tag, cmd_buf, cmd_len, n_ele, o_set);
			e_tmp = n_ele;
			if ( !vr_tmp ) 		//还是常数, 这里应该结束了
			{
				if (e_tmp) printf("plain !!!!!!!!!!\n");	//这不应该
				continue;
			}

			if ( vr_tmp->kind <= VAR_Dynamic )	//参考变量的, 不算作动态
			{
				dynamic = true;		//动态啦
				dy_num++;
			}
		}

		if ( !dynamic )	//对于非动态的, cmd_buf与cmd_len刚好是其全部的内容
			goto NextPro;

		dy_num = dy_num *2+1;	/* dy_num表示多少个动态变量, 实际分段数最多是其2倍再多1 */
		dy_list = new struct DyList [dy_num];
		cp = &cmd_buf[0]; dy_num = 0;
		n_ele = e_tmp = component;
		while ( e_tmp ) 
		{
			dy_list[dy_num].con = cp;
			dy_list[dy_num].len = 0;
			vr_tmp= var_set->all_still( e_tmp, tag, dy_list[dy_num].con, dy_list[dy_num].len, n_ele, o_set);
			e_tmp = n_ele;

			if ( dy_list[dy_num].len > 0 )	/* 前面是一个静态常量 */
			{
				dy_list[dy_num].dy_pos = -1;
				cp = &cp[dy_list[dy_num].len];	//指针后移
				dy_num++;
			}

			if ( vr_tmp && vr_tmp->kind <= VAR_Dynamic )	//参考变量的, 不算作动态
			{
				dy_list[dy_num].con = 0;
				dy_list[dy_num].len = 0;
				dy_list[dy_num].dy_pos = vr_tmp->dynamic_pos;
				dy_num++;
			}
			if ( !vr_tmp ) 		//还是常数, 这里应该结束了
			{
				if (e_tmp) printf("plain !!!!!!!!!!\n");	//这不应该
				continue;
			}
		}
	NextPro:
		set_condition ( ele, var_set, o_set);
		return ;
	};

	void pro_response(char *res_buf, int rlen,  struct MK_Session *mess)
	{
		int ii;
		
		if ( !vres || reply_num <=0 ) 	//发现有动态定义的, 输出就保存在这里
			return ;
		for (ii = 0; ii < reply_num; ii++)
		{
			if ( vres[ii].dynamic_pos > 0)
			{
				struct PlainReply *rply = &vres[ii];
				struct DyVar *dv= &mess->snap[ rply->dynamic_pos];
				int min_len;
				if ( rlen >= (rply->start ) )	//start是从1开始
				{
					if ( (rply->length + rply->start-1) > rlen  )
						min_len = (rlen - rply->start+1) ;	//要取的长度大于返回的长度
					else
						min_len = rply->length;
					if ( min_len > 0 )
						dv->input(&res_buf[rply->start-1], min_len);
				}
			}
		}
	};
};

	struct PlainIns: public SwBase, public CmdBase 
	{	/* 基本IC指令 */
		unsigned char slot;	//卡槽, 默认'0',用户卡

		/* set, 这是指令集的调用. 如果是DesMac的子序列, 有局域变量集, 就是o_set了;
		   如果在大指令集的Command, 则没有o_set,子序列是变量要求的. 
		 */
		void set ( TiXmlElement *ele, struct PVar_Set *var_set, struct PVar_Set *o_set=0)
		{
			struct PVar *vr_tmp;
			const char *p;
			slot = '0';

			set_cmd (ele, var_set, o_set);
			p = ele->Attribute("slot");
			if ( p )
			{
				slot = (unsigned char) p[0];
			} else if (o_set) { //看局域变量有没有
				vr_tmp = o_set->look("me.slot", 0);
				if ( vr_tmp )	//应该是有的
				{
					if ( vr_tmp->c_len > 0 )
					{
						slot = atoi(vr_tmp->content);
					}
				}
			}
			allow_sw(ele, o_set);	//看可允许的SW
		};
	};

	struct HsmIns: public CmdBase
	{
		int fail_retry_num;	//失败重试次数，默认为０
		const char *location;	//HSM代码, 哪一台?
		const char *ok_ans_head;
		int ok_ans_head_len;

		const char *err_head;
		int err_head_len;

		void set ( TiXmlElement *ele, struct PVar_Set *var_set, struct PVar_Set *loc_v=0)
		{
			const char *p, *q;
			location = 0;

			set_cmd (ele, var_set, loc_v);
			if ( (p = ele->Attribute("location")) )	//p可能指向一个局域变量名: me.location, me.key.para9等
			{
				if ( loc_v)
				{
					if ( (q=loc_v->get_value(p)) )
						location = q;	
				} else 
					location = p;
			}

			ok_ans_head = ele->Attribute("response");
			ok_ans_head_len = 0;
			if (ok_ans_head ) ok_ans_head_len = strlen(ok_ans_head);

			err_head = ele->Attribute("error");
			err_head_len = 0;
			if (err_head ) err_head_len = strlen(err_head);

			fail_retry_num = 0;
			ele->QueryIntAttribute("retry", &fail_retry_num);
		};
	};

/* 外部函数调用， 应该不变*/
	struct CallFun: public Condition {
		const char *lib_nm, *fun_nm;
		TiXmlElement *component;	//指令文档中的第一个component元素

		TMODULE ext_mod;
		IWayCallType call;

		int pv_many;	//
		struct PVarBase **paras; //变量指针数组
		int max_snap;
		struct DyVarBase **snaps;
		CallFun () {
			lib_nm = 0;
			fun_nm = 0;
			ext_mod = 0;
			call = 0;
			pv_many = 0;
			paras = 0;
			max_snap = 0;
			snaps = 0;
		};

		void set ( TiXmlElement *ele, struct PVar_Set *var_set, struct PVar_Set *loc_v=0)
		{
			TiXmlElement *var_ele;
			const char *vn="parameter";
			struct PVar *vr_tmp;
			const char *p;
			int i;

			ext_mod = 0;
			call = 0;
			lib_nm = ele->Attribute("library");
			fun_nm = ele->Attribute("function");
			if ( !lib_nm || !fun_nm ) 
			{
				TEXTUS_SPRINTF(err_global_str, "function (%s) or libray (%s) is empty!", fun_nm, lib_nm);
				return ;
			}
			ext_mod =TEXTUS_LOAD_MOD(lib_nm, 0);
			if ( ext_mod ) 
			{
				TEXTUS_GET_ADDR(ext_mod, fun_nm, call, IWayCallType);
			} else {
				TEXTUS_SPRINTF(err_global_str, "Load libray (%s) failed!", lib_nm);
				return ;
			}
			if ( !call )
			{
				TEXTUS_SPRINTF(err_global_str, "Load function (%s) of libray (%s) failed!", fun_nm, lib_nm);
				return ;
			}

			i = 0;
			for (var_ele = ele->FirstChildElement(vn); var_ele; var_ele = var_ele->NextSiblingElement(vn) ) 
				i++;

			paras = new struct PVarBase*[i];
			i = 0;
			for (var_ele = ele->FirstChildElement(vn); var_ele; var_ele = var_ele->NextSiblingElement(vn) ) 
			{
				p = var_ele->GetText();
				if (p )
				{
					vr_tmp = var_set->look(p, loc_v);
					if (vr_tmp )
					{
						paras[i] = vr_tmp;
					} else {
						paras[i] = new struct PVar;
						paras[i]->kind = VAR_Constant;	//认为是无名常数
						paras[i]->c_len = squeeze(p, paras[i]->content);
					}
					i++;
				}
			}
			pv_many = i;	//最后再更新一次变量数
			set_condition ( ele, var_set, loc_v);
		};

		int callfun(struct MK_Session *mess)
		{
			int i, ret = 1;
			if ( !snaps ) {	
				/* 将mess中的动态变量, 传给函数。 
				mess的动态变量数, 是所有配置中所能定义的最大数. 所以, 在定义阶段, 无法知道最终的数.
				因此, 在运行时, 分配空间.
				为什么不将mess直接传给函数?
				为了只将动态量的基本信息传给函数,
				*/
				max_snap = mess->snap_num;	
				snaps = new struct DyVarBase*[max_snap];
			}
			if ( valid_condition(mess) )
			{
				for ( i = 0; i < mess->snap_num; i++ )
					snaps[i] = &mess->snap[i];
				ret = this->call(mess->snap_num, snaps, pv_many, paras);
				if ( ret == 0 ) 
					ret = 1;
				else
					ret = -1;
			}

			return ret;
		};
	};
/* 外部函数定义结束*/

	struct Base_Command {		//基础指令定义，只包括两种。　用于子序列，指令数少，所以不需要order了。
		enum Command_Type type;	//类型, 不用union类型, 
		struct PlainIns *plain_p;
		struct HsmIns *hsm_p;
		struct CallFun *fun_p;

		int  set ( TiXmlElement *ele, struct PVar_Set *vrset,  struct PVar_Set *o_set) //返回IC指令数
		{
			int ret = 0;
			plain_p = 0;
			hsm_p = 0;
			fun_p = 0;
			type = INS_None;
			

			if ( strcasecmp(ele->Value(), "icc") == 0 ) 
			{
				type = INS_Plain;
				plain_p = new struct PlainIns;
				plain_p->set(ele ,vrset, o_set);
				ret = 1;
			};

			if ( strcasecmp(ele->Value(), "hsm") == 0 ) 
			{
				type = INS_HSM;
				hsm_p = new struct HsmIns;
				hsm_p->set(ele ,vrset, o_set);
				ret = 0;
			};

			if ( strcasecmp(ele->Value(), "call") == 0 ) 
			{
				type = INS_Call;
				fun_p = new struct CallFun;
				fun_p->set(ele ,vrset, o_set);
				ret = 0;
			};
			return ret;
		};
	};

	struct INS_SubSet {
		struct Base_Command *instructions;
		int many;
		INS_SubSet () 
		{
			instructions= 0;
			many = 0;
		};

		~INS_SubSet () 
		{
			if (instructions ) delete []instructions;
			instructions = 0;
			many = 0;
		};

		int put_inses(TiXmlElement *root, struct PVar_Set *var_set, struct PVar_Set *o_set) //返回子序列IC指令数
		{
			TiXmlElement *b_ele;
			int which, refny=0, icc_num=0 ;

			/* 分析一下２种操作 */

			#define IS_SUBINS(x) strcasecmp(x, "icc") ==0  \
					|| strcasecmp(x, "call") ==0  \
					|| strcasecmp(x, "hsm") ==0

			if ( !root ) return 0;
			b_ele= root->FirstChildElement(); refny = 0;
			while(b_ele)
			{
				if ( b_ele->Value() )
				{
					if ( IS_SUBINS(b_ele->Value()) )
						refny++;
				}
				b_ele = b_ele->NextSiblingElement();
			}
			//确定变量数
			if ( refny ==0 )
				goto S_End;

			many = refny;
			instructions = new struct Base_Command[many];
			which = 0;

			b_ele= root->FirstChildElement(); 
			icc_num = 0;
			while(b_ele)
			{
				if ( b_ele->Value() )
				{
					if ( IS_SUBINS(b_ele->Value()) )
					{
						icc_num += instructions[which].set(b_ele, var_set, o_set);
						which++;
					}
				}
				b_ele = b_ele->NextSiblingElement();
			}
		S_End:
			return icc_num;
		};
	};

	struct ComplexSubSerial: public SwBase, public Condition {
		struct INS_SubSet *si_set;	//子序列
		TiXmlElement *sub_ins_entry;	//MAP文档的子序列元素

		TiXmlDocument var_doc;
		TiXmlElement *var_root;
		struct PVar_Set sv_set;	//局域变量集, 只有对DesMac之类的才有

		ComplexSubSerial()
		{
			var_root = 0;
			si_set = 0;
		};
		
		virtual ~ComplexSubSerial()
		{
			if ( si_set )  {
				delete si_set;
				si_set = 0;
			}
		};

		int defer_sub_serial(TiXmlElement *ele, TiXmlElement *root, struct PVar_Set *all_set, struct PVar_Set *sub_set=0)
		{
			set_condition ( ele, all_set, sub_set);
			if ( !root ) 	//没有子序列入口, 
				return 0;
			if (!si_set)
				si_set = new struct INS_SubSet();
			return si_set->put_inses(root, all_set, sub_set); //返回子序列指令数
		};
	
		void def_sub_vars(const char *xml) //分析一下变量定义
		{
			var_doc.Parse(xml);
			var_root = var_doc.RootElement();
			sv_set.defer_vars(var_root);
		};

		void sub_allow_sw( TiXmlElement *ele)
		{
			allow_sw(ele);	//看可允许的SW
			sv_set.put_still("me.sw", another_sw_str);
			sv_set.put_still("me.sw2", another_sw2_str);
			sv_set.put_still("me.not_sw", not_sw_str);
			sv_set.put_still("me.slot",  ele->Attribute("slot")); 
		};

		/* 参考型变量在子序列中的准备, 从全局变量表中发现它是一个参考变量, 再赋值到局域变量表中
		   loc_rf_nm 是局域变量表中的名, 如"protect", "refer", "key"等
		*/
		void ref_prepare_sub(const char *ref_nm,  struct PVar *&ref_var, struct PVar_Set *var_set, const char *loc_rf_nm)
		{
			char buf[512];		//实际内容, 常数内容
			int len;
			struct PVar *vr_tmp;
			struct PVar *loc_var;
			int i;

			char att_name[32];
			const char *at_val;

			if ( ref_nm )
			{
				ref_var = var_set->one_still(ref_nm, buf, len);	//找到已定义变量的
				if ( ref_var )
				{
					for ( i = 1; i <= VAR_SUB_NUM; i++)		//各种子变量值给赋上，都当作静态
					{
						TEXTUS_SPRINTF(att_name, "para%d", i);
						at_val = ref_var->me_ele->Attribute(att_name);
						if ( !at_val ) continue;		//没有子变量，看下一个
								
						TEXTUS_SPRINTF(att_name, "me.%s.para%d", loc_rf_nm, i); //局域变量
						vr_tmp = var_set->one_still(at_val, buf, len);	//at_val是个变量, 可能是动态
						if ( vr_tmp && vr_tmp->kind <= VAR_Dynamic ) 
						{	//变量若是动态, 如卡号, 则在这里设为动态, 并指向该位置
							loc_var = sv_set.look(att_name);
							loc_var->dynamic_pos = vr_tmp->dynamic_pos;
							loc_var->kind = vr_tmp->kind;
						} else 
							sv_set.put_still(att_name, buf, len);
					}
					at_val = ref_var->me_ele->Attribute("location");	//参考变量中有位置属性
					TEXTUS_SPRINTF(att_name, "me.%s.location", loc_rf_nm);
					sv_set.put_still(att_name, at_val);
				}
			}
		};

		/* 获得子序列入口, 与主参考变量有关 */
		void get_entry(struct PVar *ref_var, TiXmlElement *map_root, const char *entry_nm)
		{
			char pro_nm[128];
			if ( ref_var && ref_var->me_ele->Attribute("pro") ) //pro指示子序列的变体名
			{
				TEXTUS_SNPRINTF(pro_nm, sizeof(pro_nm), "%s%s", entry_nm, ref_var->me_ele->Attribute("pro"));
				sub_ins_entry = map_root->FirstChildElement(pro_nm);
			} else 
				sub_ins_entry = map_root->FirstChildElement(entry_nm);
		};
		/* 输入输出变量的全局本地处理 */
		void get_loc_var ( TiXmlElement *ele, const char *att_nm,  struct PVar_Set *var_set, const char *loc_var_nm, const char *default_val)
		{
			const char *att_val;
			struct PVar *vr_tmp, *h_var;
			char h_buf[512];
			int h_len;
			att_val = ele->Attribute(att_nm);
			if (att_val && strlen(att_val) > 0 ) 
			{
				h_var = var_set->one_still(att_val, h_buf, h_len);	//如果属性值为变量名, 要找到这个变量
				if ( h_var ) 
				{
					if ( h_var->kind <= VAR_Dynamic ) 
					{
						vr_tmp = sv_set.look(loc_var_nm);	//loc_var_nm是子序列本地变量名
						vr_tmp->dynamic_pos = h_var->dynamic_pos;	//本地变量对映到全部动态变量表
						vr_tmp->kind = h_var->kind;
					} else
						 sv_set.put_still(loc_var_nm, h_var->content);	//就把全局静态变量内容取过来
				} else 
					sv_set.put_still(loc_var_nm, att_val);	//本地变量内容就取属性值了
			} else 
				sv_set.put_still(loc_var_nm, default_val);	//本变量默认值
		};

		/* 仅用于输出变量的全局本地处理 */
		void get_loc_var ( TiXmlElement *ele, const char *att_nm,  struct PVar_Set *var_set, const char *loc_var_nm)
		{
			const char *att_val;
			struct PVar *vr_tmp, *h_var;
			char h_buf[512];
			int h_len;
			att_val = ele->Attribute(att_nm);
			if (att_val) 
			{
				h_var = var_set->one_still(att_val, h_buf, h_len);	//如果属性值为变量名, 要找到这个变量
				if ( h_var ) 
				{
					if ( h_var->kind <= VAR_Dynamic ) 
					{
						vr_tmp = sv_set.look(loc_var_nm);	//loc_var_nm是子序列本地变量名
						vr_tmp->dynamic_pos = h_var->dynamic_pos;	//本地变量对映到全部动态变量表
						vr_tmp->kind = h_var->kind;
					}
				}
			}
		};

		//virtual int set ( TiXmlElement *ele, struct PVar_Set *var_set, TiXmlElement *map_root, const char *entry_nm)
		//{
		//	return 0;
		//};

		//virtual void  get_current(MK_Session *sess, struct PVar_Set *var_set) { };
		int pro_analyze( TiXmlElement *app_ele, struct PVar_Set *var_set, TiXmlElement *map_root, const char *pkey_nm)
		{
		};
	};

	struct ChargeIns: public ComplexSubSerial {			//充值综合指令
		const char *key;	//充值key，指向文档
		struct PVar *key_lod;
		int set ( TiXmlElement *ele, struct PVar_Set *var_set, TiXmlElement *map_root, const char *entry_nm)
		{
			const char *xml="<?xml version=\"1.0\" encoding=\"gb2312\" ?>"
				"<Loc>"
					"<Variable name=\"me.key.location\" />"
					"<Variable name=\"me.slot\" />"
					"<Variable name=\"me.key.para1\" />"
					"<Variable name=\"me.key.para2\" />"
					"<Variable name=\"me.key.para3\" />"
					"<Variable name=\"me.key.para4\" />"
					"<Variable name=\"me.key.para5\" />"
					"<Variable name=\"me.key.para6\" />"
					"<Variable name=\"me.key.para7\" />"
					"<Variable name=\"me.key.para8\" />"
					"<Variable name=\"me.key.para9\" />"
					"<Variable name=\"me.key.para10\" />"
					"<Variable name=\"me.key.para11\" />"
					"<Variable name=\"me.key.para12\" />"
					"<Variable name=\"me.key.para13\" />"
					"<Variable name=\"me.key.para14\" />"
					"<Variable name=\"me.key.para15\" />"
					"<Variable name=\"me.key.para16\" />"
					"<Variable name=\"me.term\" />"
					"<Variable name=\"me.amount\" />"
					"<Variable name=\"me.datetime\" />"
					"<Variable name=\"me.not_sw\" />"
					"<Variable name=\"me.sw\" />"
					"<Variable name=\"me.sw2\" />"
				"</Loc>";

			def_sub_vars(xml);		//局域变量定义
			sub_allow_sw(ele);	//看可允许的SW, 包括slot

			get_loc_var (ele, "term",  var_set, "me.term", "123456789012");
			get_loc_var (ele, "amount",  var_set, "me.amount", "00000000");
			get_loc_var (ele, "datetime",  var_set, "me.datetime", "20131231083011");

			ref_prepare_sub(ele->Attribute("key"),  key_lod, var_set, "key");	//密钥变量预处理
			get_entry(key_lod, map_root, entry_nm);
			return defer_sub_serial(ele, sub_ins_entry, var_set, &sv_set);
		};
	};

	struct LoadInitIns: public ComplexSubSerial {			//圈存初始化指令
		struct PVar *key_lod;

		int set ( TiXmlElement *ele, struct PVar_Set *var_set, TiXmlElement *map_root, const char *entry_nm)
		{
			const char *xml="<?xml version=\"1.0\" encoding=\"gb2312\" ?>"
				"<Loc>"
					"<Variable name=\"me.key.location\" />"
					"<Variable name=\"me.slot\" />"
					"<Variable name=\"me.key.para1\" />"
					"<Variable name=\"me.key.para2\" />"
					"<Variable name=\"me.key.para3\" />"
					"<Variable name=\"me.key.para4\" />"
					"<Variable name=\"me.key.para5\" />"
					"<Variable name=\"me.key.para6\" />"
					"<Variable name=\"me.key.para7\" />"
					"<Variable name=\"me.key.para8\" />"
					"<Variable name=\"me.key.para9\" />"
					"<Variable name=\"me.key.para10\" />"
					"<Variable name=\"me.key.para11\" />"
					"<Variable name=\"me.key.para12\" />"
					"<Variable name=\"me.key.para13\" />"
					"<Variable name=\"me.key.para14\" />"
					"<Variable name=\"me.key.para15\" />"
					"<Variable name=\"me.key.para16\" />"
					"<Variable name=\"me.typeid\" />"
					"<Variable name=\"me.term\" />"
					"<Variable name=\"me.amount\" />"
					"<Variable name=\"me.datetime\" />"
					"<Variable name=\"me.online\" />"
					"<Variable name=\"me.balance\" />"
					"<Variable name=\"me.mac2\" />"
					"<Variable name=\"me.not_sw\" />"
					"<Variable name=\"me.sw\" />"
					"<Variable name=\"me.sw2\" />"
				"</Loc>";

			sub_ins_entry = map_root->FirstChildElement(entry_nm);
			def_sub_vars(xml);		//局域变量定义
			sub_allow_sw(ele);	//看可允许的SW

			get_loc_var (ele, "typeid",  var_set, "me.typeid", "02");
			get_loc_var (ele, "term",  var_set, "me.term", "123456789012");
			get_loc_var (ele, "amount",  var_set, "me.amount", "00000000");
			get_loc_var (ele, "datetime",  var_set, "me.datetime", "20131231083011");
			get_loc_var (ele, "online",  var_set, "me.online"); //仅输出的, 不会有默认值
			get_loc_var (ele, "balance",  var_set, "me.balance"); //仅输出的, 不会有默认值
			get_loc_var (ele, "mac2",  var_set, "me.mac2"); //仅输出的, 不会有默认值

			ref_prepare_sub(ele->Attribute("key"),  key_lod, var_set, "key");
			get_entry(key_lod, map_root, entry_nm);
			return defer_sub_serial(ele, sub_ins_entry, var_set, &sv_set);
		};
	};

	struct DebitIns: public ComplexSubSerial {			//完整的消费过程，包括初始化，扣款
		struct PVar *key_debit;

		int set ( TiXmlElement *ele, struct PVar_Set *var_set, TiXmlElement *map_root, const char *entry_nm)
		{
			const char *xml="<?xml version=\"1.0\" encoding=\"gb2312\" ?>"
				"<Loc>"
					"<Variable name=\"me.key.location\" />"
					"<Variable name=\"me.slot\" />"
					"<Variable name=\"me.key.para1\" />"
					"<Variable name=\"me.key.para2\" />"
					"<Variable name=\"me.key.para3\" />"
					"<Variable name=\"me.key.para4\" />"
					"<Variable name=\"me.key.para5\" />"
					"<Variable name=\"me.key.para6\" />"
					"<Variable name=\"me.key.para7\" />"
					"<Variable name=\"me.key.para8\" />"
					"<Variable name=\"me.key.para9\" />"
					"<Variable name=\"me.key.para10\" />"
					"<Variable name=\"me.key.para11\" />"
					"<Variable name=\"me.key.para12\" />"
					"<Variable name=\"me.key.para13\" />"
					"<Variable name=\"me.key.para14\" />"
					"<Variable name=\"me.key.para15\" />"
					"<Variable name=\"me.key.para16\" />"
					"<Variable name=\"me.typeid\" />"
					"<Variable name=\"me.term\" />"
					"<Variable name=\"me.transno\" />"
					"<Variable name=\"me.amount\" />"
					"<Variable name=\"me.datetime\" />"
					"<Variable name=\"me.offline\" />"
					"<Variable name=\"me.balance\" />"
					"<Variable name=\"me.tac\" />"
					"<Variable name=\"me.not_sw\" />"
					"<Variable name=\"me.sw\" />"
					"<Variable name=\"me.sw2\" />"
				"</Loc>";

			def_sub_vars(xml);		//局域变量定义
			sub_allow_sw(ele);	//允许的SW

			get_loc_var (ele, "typeid",  var_set, "me.typeid", "06");
			get_loc_var (ele, "term",  var_set, "me.term", "000000000012");
			get_loc_var (ele, "amount",  var_set, "me.amount", "00000000");
			get_loc_var (ele, "datetime",  var_set, "me.datetime", "00000000000000");
			get_loc_var (ele, "transno",  var_set, "me.transno", "0000");
			get_loc_var (ele, "offline",  var_set, "me.offline"); //仅输出的, 不会有默认值
			get_loc_var (ele, "balance",  var_set, "me.balance"); //仅输出的, 不会有默认值
			get_loc_var (ele, "tac",  var_set, "me.tac"); //仅输出的, 不会有默认值

			ref_prepare_sub(ele->Attribute("key"),  key_debit, var_set, "key");
			get_entry(key_debit, map_root, entry_nm);
			return defer_sub_serial(ele, sub_ins_entry, var_set, &sv_set);
		};
	};

	struct ExtAuthIns: public ComplexSubSerial {
		char auth[256];
		struct PVar *key_auth;	//认证key
		int set ( TiXmlElement *ele, struct PVar_Set *var_set, TiXmlElement *map_root, const char *entry_nm, TiXmlElement *key_e)
		{
			const char *au;
			const char *xml="<?xml version=\"1.0\" encoding=\"gb2312\" ?>"
				"<Loc>"
					"<Variable name=\"me.key.location\" />"
					"<Variable name=\"me.slot\" />"
					"<Variable name=\"me.key.para1\" />"
					"<Variable name=\"me.key.para2\" />"
					"<Variable name=\"me.key.para3\" />"
					"<Variable name=\"me.key.para4\" />"
					"<Variable name=\"me.key.para5\" />"
					"<Variable name=\"me.key.para6\" />"
					"<Variable name=\"me.key.para7\" />"
					"<Variable name=\"me.key.para8\" />"
					"<Variable name=\"me.key.para9\" />"
					"<Variable name=\"me.key.para10\" />"
					"<Variable name=\"me.key.para11\" />"
					"<Variable name=\"me.key.para12\" />"
					"<Variable name=\"me.key.para13\" />"
					"<Variable name=\"me.key.para14\" />"
					"<Variable name=\"me.key.para15\" />"
					"<Variable name=\"me.key.para16\" />"
					"<Variable name=\"me.auth\" />"
					"<Variable name=\"me.not_sw\" />"
					"<Variable name=\"me.sw\" />"
					"<Variable name=\"me.sw2\" />"
				"</Loc>";
			def_sub_vars(xml);
			sub_allow_sw(ele);	//允许的SW

			memset(auth, 0, sizeof(auth));
			au = key_e->Attribute("auth");	//<key auth=""></key>中的auth优先
			if ( !au)
				au = ele->Attribute("auth");
			if ( au && strlen(au) > 0 )
			{
				squeeze(au, auth);
				sv_set.put_still("me.auth", auth);
			} else
				sv_set.put_still("me.auth", "0082000008");

			ref_prepare_sub(key_e->GetText(),  key_auth, var_set, "key");	//"key"与me.key.para相对应
			get_entry(key_auth, map_root, entry_nm);
			return defer_sub_serial(ele, sub_ins_entry, var_set, &sv_set);
		};
	};
	
	struct DesMacIns: public ComplexSubSerial {
		struct PVar *pk_var;	//保护key，指向分析好的一个变量定义
		struct PVar *ek_var;	//导出key，指向分析好的一个变量定义

		bool head_dynamic;
		bool body_dynamic;

		TiXmlElement *head;	//指令文档中的第一个head元素
		TiXmlElement *body;	//指令文档中的第一个body元素
		TiXmlElement *tail;	//指令文档中的第一个tail元素
		const char *tag_head;
		const char *tag_body;
		const char *tag_tail;
		char head_buf[512]; 
		int head_len;
		char body_buf[512]; 
		int body_len;

		const char* len_format;

		int head_dy_pos;	
		int head_len_dy_pos;	

		int body_dy_pos;	
		int body_len_dy_pos;	

		int mac_len_dy_pos;	
		/* 取得实时的指令头、指令体的内容 */
		void  get_current(MK_Session *sess, struct PVar_Set *var_set)
		{
			struct DyVar *dvr, *dvr2;
			int blen, hlen;
			hlen = blen = 0;
			if ( head_dynamic )
			{
				dvr = &(sess->snap[head_dy_pos]);
				dvr2 = &(sess->snap[head_len_dy_pos]);
				var_set->get_var_all(head, tag_head, dvr->val, dvr->c_len, sess);
				if ( len_format ) 
				{
					TEXTUS_SPRINTF(dvr2->val, len_format, dvr->c_len/2);
				} else  {
					TEXTUS_SPRINTF(dvr2->val, "%02d", dvr->c_len/2);
				}
				dvr2->c_len = strlen(dvr2->val);
				hlen = dvr->c_len/2;
			} else 
				hlen = strlen(head_buf)/2;

			if ( body_dynamic ) 
			{
				dvr = &(sess->snap[body_dy_pos]);
				dvr2 = &(sess->snap[body_len_dy_pos]);
				var_set->get_var_all(body, tag_body, dvr->val, dvr->c_len, sess);
				if ( len_format ) 
				{
					TEXTUS_SPRINTF(dvr2->val, len_format, dvr->c_len/2);
				} else  {
					TEXTUS_SPRINTF(dvr2->val, "%02d", dvr->c_len/2);
				}
				dvr2->c_len = strlen(dvr2->val);
				blen = dvr->c_len/2; 
			} else
				blen = strlen(body_buf)/2;
		
			if ( body_dynamic || head_dynamic )
			{
				if ( blen%8 == 0 ) 
					blen += 8;
				else
					blen += (8-blen%8);	//加密数据补位后的长度
				blen += ( 8 + hlen );	//加上8字节随机数和头长度

				dvr2 = &(sess->snap[mac_len_dy_pos]);
				if ( len_format ) 
				{
					TEXTUS_SPRINTF(dvr2->val, len_format, blen);
				} else  {
					TEXTUS_SPRINTF(dvr2->val, "%03d", blen);
				}
				dvr2->c_len = strlen(dvr2->val);
			}
		};

		int set(TiXmlElement *ele, struct PVar_Set *var_set, TiXmlElement *map_root, const char *entry_nm)
		{
			const char *xml="<?xml version=\"1.0\" encoding=\"gb2312\" ?>"
				"<Loc>"
					"<Variable name=\"me.protect.location\" />"
					"<Variable name=\"me.slot\" />"
					"<Variable name=\"me.protect.para1\" />"
					"<Variable name=\"me.protect.para2\" />"
					"<Variable name=\"me.protect.para3\" />"
					"<Variable name=\"me.protect.para4\" />"
					"<Variable name=\"me.protect.para5\" />"
					"<Variable name=\"me.protect.para6\" />"
					"<Variable name=\"me.protect.para7\" />"
					"<Variable name=\"me.protect.para8\" />"
					"<Variable name=\"me.protect.para9\" />"
					"<Variable name=\"me.protect.para10\" />"
					"<Variable name=\"me.protect.para11\" />"
					"<Variable name=\"me.protect.para12\" />"
					"<Variable name=\"me.protect.para13\" />"
					"<Variable name=\"me.protect.para14\" />"
					"<Variable name=\"me.protect.para15\" />"
					"<Variable name=\"me.protect.para16\" />"
					"<Variable name=\"me.refer.para1\" />"
					"<Variable name=\"me.refer.para2\" />"
					"<Variable name=\"me.refer.para3\" />"
					"<Variable name=\"me.refer.para4\" />"
					"<Variable name=\"me.refer.para5\" />"
					"<Variable name=\"me.refer.para6\" />"
					"<Variable name=\"me.refer.para7\" />"
					"<Variable name=\"me.refer.para8\" />"
					"<Variable name=\"me.refer.para9\" />"
					"<Variable name=\"me.refer.para10\" />"
					"<Variable name=\"me.refer.para11\" />"
					"<Variable name=\"me.refer.para12\" />"
					"<Variable name=\"me.refer.para13\" />"
					"<Variable name=\"me.refer.para14\" />"
					"<Variable name=\"me.refer.para15\" />"
					"<Variable name=\"me.refer.para16\" />"
					"<Variable name=\"me.head\" />"
					"<Variable name=\"me.body\" />"
					"<Variable name=\"me.head_length\" />"
					"<Variable name=\"me.body_length\" />"
					"<Variable name=\"me.mac_length\" />"
					"<Variable name=\"me.not_sw\" />"
					"<Variable name=\"me.sw\" />"
					"<Variable name=\"me.sw2\" />"
				"</Loc>";

			struct PVar *vr_tmp;
			TiXmlElement *e_tmp, *n_ele;
			int j;
			char tmp[32];

			tag_head = "head";
			tag_body = "body";
			head =  ele->FirstChildElement(tag_head); 
			body =  ele->FirstChildElement(tag_body); 

			def_sub_vars(xml);		//局域变量定义
			sub_allow_sw(ele);	//看可允许的SW
			ref_prepare_sub(ele->Attribute("protect_key"),  pk_var, var_set, "protect");	//保护Key定义
			get_entry(pk_var, map_root, entry_nm);	//sub_ins_entry才确定
			if ( !sub_ins_entry) return 0;	//没有子序列定义, 直接返
			len_format = sub_ins_entry->Attribute("length_format");

			body_dynamic = false;
			head_dynamic = false;

			head_len = 0;
			head_buf[0] = 0;
			n_ele = e_tmp = head;
			while ( e_tmp ) 
			{
				vr_tmp= var_set->all_still( e_tmp, tag_head, head_buf, head_len, n_ele);
				e_tmp = n_ele;
				if ( !vr_tmp ) 		//还是常数, 这里应该结束了
				{
					if (e_tmp) printf("desmac head !!!!!!!!!!\n");	//这不应该
						continue;
				}

				if ( vr_tmp->kind <= VAR_Dynamic )
				{
					head_dynamic = true;		//动态啦
				} 
			}

			ek_var = 0;		//如果没有导出?
			body_len = 0;
			body_buf[0] = 0;
			n_ele = e_tmp = body;
			j = 0;
			while ( e_tmp ) 
			{
				vr_tmp= var_set->all_still( e_tmp, tag_body, body_buf, body_len, n_ele);
				e_tmp = n_ele;
				if ( !vr_tmp ) 		//还是常数, 这里应该结束了
				{
					if (e_tmp) printf("desmac body !!!!!!!!!!\n");	//这不应该
					continue;
				}
					
				if ( vr_tmp->kind == VAR_Refer )
				{
					ek_var = vr_tmp;		//参考变量, 多个也可, 以后从1开始
					if ( j == 0 ) 	//ek_var已经找到, 再找一次, 函数就这样, 有点重复
					{
						ref_prepare_sub(ek_var->name,  ek_var, var_set, "refer");
					} else {
						char ref_nm[16];
						TEXTUS_SPRINTF(ref_nm, "refer%d", j);
						ref_prepare_sub(ek_var->name,  ek_var, var_set, ref_nm);
					}
					j++;
		
				} else if ( vr_tmp->kind <= VAR_Dynamic )
				{
					body_dynamic = true;		//动态啦
				} 
			}

			if ( head_dynamic )		//sv_set: 局域变量集
			{
				vr_tmp = sv_set.look("me.head");
				vr_tmp->dynamic_pos = var_set->get_neo_dynamic_pos();	//动态变量位置
				vr_tmp->kind = VAR_Dynamic;
				head_dy_pos = vr_tmp->dynamic_pos;

				vr_tmp = sv_set.look("me.head_length");
				vr_tmp->dynamic_pos = var_set->get_neo_dynamic_pos();	//动态变量位置
				vr_tmp->kind = VAR_Dynamic;
				head_len_dy_pos	= vr_tmp->dynamic_pos;

			} else {
				sv_set.put_still("me.head",head_buf);
				if ( len_format ) 
					TEXTUS_SPRINTF(tmp, len_format, strlen(head_buf)/2);
				else 
					TEXTUS_SPRINTF(tmp, "%02lu", strlen(head_buf)/2);
				sv_set.put_still("me.head_length",tmp);
			}

			if ( body_dynamic ) 
			{
				vr_tmp = sv_set.look("me.body");
				vr_tmp->dynamic_pos = var_set->get_neo_dynamic_pos();	//动态变量位置
				vr_tmp->kind = VAR_Dynamic;
				body_dy_pos = vr_tmp->dynamic_pos;

				vr_tmp = sv_set.look("me.body_length");
				vr_tmp->dynamic_pos = var_set->get_neo_dynamic_pos();	//动态变量位置
				vr_tmp->kind = VAR_Dynamic;
				body_len_dy_pos = vr_tmp->dynamic_pos;

			} else {
				sv_set.put_still("me.body",body_buf);
				if ( len_format ) 
					TEXTUS_SPRINTF(tmp, len_format, strlen(body_buf)/2);
				else 
					TEXTUS_SPRINTF(tmp, "%02lu", strlen(body_buf)/2);
				sv_set.put_still("me.body_length",tmp);
			}

			if ( body_dynamic || head_dynamic )
			{
				vr_tmp = sv_set.look("me.mac_length");
				vr_tmp->dynamic_pos = var_set->get_neo_dynamic_pos();	//动态变量位置
				vr_tmp->kind = VAR_Dynamic;
				mac_len_dy_pos = vr_tmp->dynamic_pos;
			} else {
				int mlen;
				mlen = strlen(body_buf)/2;
				if ( mlen%8 == 0 ) 
					mlen += 8;
				else
					mlen += (8-mlen%8);
				mlen += ( 8 + strlen(head_buf)/2 );

				if ( len_format ) 
					TEXTUS_SPRINTF(tmp, len_format, mlen);
				else 
					TEXTUS_SPRINTF(tmp, "%03d", mlen);
				sv_set.put_still("me.mac_length",tmp);
			}

			return defer_sub_serial(ele, sub_ins_entry, var_set, &sv_set);
		};
	};
	
	struct MacIns: public ComplexSubSerial {
		const char *protect;	//保护key，指向文档
		struct PVar *pk_var;	//保护key，指向分析好的一个变量定义

		const char *tag;
		TiXmlElement *component;	//指令文档中的第一个component元素

		bool dynamic;
		char cmd_buf[512];
		int cmd_len;

		int cmd_dy_pos;	
		int cmd_len_dy_pos;	
		int mac_len_dy_pos;	

		const char *t_tag, *len_format;

		void  get_current( MK_Session *sess, struct PVar_Set *var_set)
		{
			struct DyVar *dvr, *dvr2;
			if ( dynamic ) 
			{
				dvr = &(sess->snap[cmd_dy_pos]);
				dvr2 = &(sess->snap[cmd_len_dy_pos]);
				var_set->get_var_all(component, tag, dvr->val, dvr->c_len, sess);
				if ( len_format ) 
				{
					TEXTUS_SPRINTF(dvr2->val, len_format, dvr->c_len/2);
				} else  {
					TEXTUS_SPRINTF(dvr2->val, "%03d", dvr->c_len/2);
				}
				dvr2->c_len = strlen(dvr2->val);

				dvr2 = &(sess->snap[mac_len_dy_pos]);
				if ( len_format ) 
				{
					TEXTUS_SPRINTF(dvr2->val, len_format, dvr->c_len/2+8);
				} else  {
					TEXTUS_SPRINTF(dvr2->val, "%03d", dvr->c_len/2+8);
				}
				dvr2->c_len = strlen(dvr2->val);
			}
		};

		int set(TiXmlElement *ele, struct PVar_Set *var_set, TiXmlElement *map_root, const char *entry_nm)
		{
			const char *xml="<?xml version=\"1.0\" encoding=\"gb2312\" ?>"
				"<Loc>"
					"<Variable name=\"me.protect.location\" />"
					"<Variable name=\"me.slot\" />"
					"<Variable name=\"me.protect.para1\" />"
					"<Variable name=\"me.protect.para2\" />"
					"<Variable name=\"me.protect.para3\" />"
					"<Variable name=\"me.protect.para4\" />"
					"<Variable name=\"me.protect.para5\" />"
					"<Variable name=\"me.protect.para6\" />"
					"<Variable name=\"me.protect.para7\" />"
					"<Variable name=\"me.protect.para8\" />"
					"<Variable name=\"me.protect.para9\" />"
					"<Variable name=\"me.protect.para10\" />"
					"<Variable name=\"me.protect.para11\" />"
					"<Variable name=\"me.protect.para12\" />"
					"<Variable name=\"me.protect.para13\" />"
					"<Variable name=\"me.protect.para14\" />"
					"<Variable name=\"me.protect.para15\" />"
					"<Variable name=\"me.protect.para16\" />"
					"<Variable name=\"me.command\" />"
					"<Variable name=\"me.command_length\" />"
					"<Variable name=\"me.mac_length\" />"
					"<Variable name=\"me.not_sw\" />"
					"<Variable name=\"me.sw\" />"
					"<Variable name=\"me.sw2\" />"
				"</Loc>";

			struct PVar *vr_tmp;
			TiXmlElement *e_tmp, *n_ele;

			def_sub_vars(xml);		//局域变量定义
			sub_allow_sw(ele);	//看可允许的SW
			ref_prepare_sub(ele->Attribute("protect_key"),  pk_var, var_set, "protect");	//保护密钥
			get_entry(pk_var, map_root, entry_nm);
			len_format = sub_ins_entry->Attribute("length_format");	//取得长度格式字符串

			tag = "component";
			component = ele->FirstChildElement(tag); 

			dynamic = false;
			cmd_len = 0;
			cmd_buf[0] = 0;
			n_ele = e_tmp = component;
			while ( e_tmp ) 
			{
				vr_tmp= var_set->all_still( e_tmp, tag, cmd_buf, cmd_len, n_ele);
				e_tmp = n_ele;
				if ( !vr_tmp ) 		//还是常数, 这里应该结束了
				{
					if (e_tmp) printf("macins !!!!!!!!!!\n");	//这不应该
					continue;
				}
					
				if ( vr_tmp->kind <= VAR_Dynamic )
				{
					dynamic = true;		//动态啦
				} 
			}

			if ( dynamic ) 
			{
				vr_tmp = sv_set.look("me.command");
				vr_tmp->dynamic_pos = var_set->get_neo_dynamic_pos();	//动态变量位置
				vr_tmp->kind = VAR_Dynamic;
				cmd_dy_pos = vr_tmp->dynamic_pos;

				vr_tmp = sv_set.look("me.command_length");
				vr_tmp->dynamic_pos = var_set->get_neo_dynamic_pos();	//动态变量位置
				vr_tmp->kind = VAR_Dynamic;
				cmd_len_dy_pos = vr_tmp->dynamic_pos;

				vr_tmp = sv_set.look("me.mac_length");
				vr_tmp->dynamic_pos = var_set->get_neo_dynamic_pos();	//动态变量位置
				vr_tmp->kind = VAR_Dynamic;
				mac_len_dy_pos = vr_tmp->dynamic_pos;
			} else {
				char tmp[32];
				sv_set.put_still("me.command", cmd_buf);
				if ( len_format ) 
					TEXTUS_SPRINTF(tmp, len_format, strlen(cmd_buf)/2);
				else 
					TEXTUS_SPRINTF(tmp, "%03lu", strlen(cmd_buf)/2);
				sv_set.put_still("me.command_length",tmp);

				if ( len_format ) 
					TEXTUS_SPRINTF(tmp, len_format, strlen(cmd_buf)/2+8);
				else 
					TEXTUS_SPRINTF(tmp, "%03lu", strlen(cmd_buf)/2+8);
				sv_set.put_still("me.mac_length",tmp);
			}

			return defer_sub_serial(ele, sub_ins_entry, var_set, &sv_set);
		};
	};
	
	struct GPKMCIns: public ComplexSubSerial {
		const char *ori_kmc;	//原KMC，指向文档, 可能是一个变量名, 或实际的值
		struct PVar *ori_kvar;	//原指向分析好的一个变量定义

		const char *neo_kmc;	//新KMC，指向文档, 可能是一个变量名, 或实际的值
		struct PVar *neo_kvar;	//新指向分析好的一个变量定义
		char ori_ver[8];		//原密钥版本
		char neo_ver[8];		//新密钥版本

		char put_type[8];	//密钥类型: 80或81, 对于put key有用.
		char put_ins[16];	//PUT KEY Command

		int set ( TiXmlElement *ele, struct PVar_Set *var_set, TiXmlElement *map_root, const char *entry_nm)
		{
			const char *tmp;

			const char *xml="<?xml version=\"1.0\" encoding=\"gb2312\" ?>"
				"<Loc>"
					"<Variable name=\"me.neokey.location\" />"
					"<Variable name=\"me.neokey.para1\" />"
					"<Variable name=\"me.neokey.para2\" />"
					"<Variable name=\"me.neokey.para3\" />"
					"<Variable name=\"me.neokey.para4\" />"
					"<Variable name=\"me.neokey.para5\" />"
					"<Variable name=\"me.neokey.para6\" />"
					"<Variable name=\"me.neokey.para7\" />"
					"<Variable name=\"me.neokey.para8\" />"
					"<Variable name=\"me.neokey.para9\" />"
					"<Variable name=\"me.neokey.para10\" />"
					"<Variable name=\"me.neokey.para11\" />"
					"<Variable name=\"me.neokey.para12\" />"
					"<Variable name=\"me.neokey.para13\" />"
					"<Variable name=\"me.neokey.para14\" />"
					"<Variable name=\"me.neokey.para15\" />"
					"<Variable name=\"me.neokey.para16\" />"
					"<Variable name=\"me.orikey.location\" />"
					"<Variable name=\"me.orikey.para1\" />"
					"<Variable name=\"me.orikey.para2\" />"
					"<Variable name=\"me.orikey.para3\" />"
					"<Variable name=\"me.orikey.para4\" />"
					"<Variable name=\"me.orikey.para5\" />"
					"<Variable name=\"me.orikey.para6\" />"
					"<Variable name=\"me.orikey.para7\" />"
					"<Variable name=\"me.orikey.para8\" />"
					"<Variable name=\"me.orikey.para9\" />"
					"<Variable name=\"me.orikey.para10\" />"
					"<Variable name=\"me.orikey.para11\" />"
					"<Variable name=\"me.orikey.para12\" />"
					"<Variable name=\"me.orikey.para13\" />"
					"<Variable name=\"me.orikey.para14\" />"
					"<Variable name=\"me.orikey.para15\" />"
					"<Variable name=\"me.orikey.para16\" />"
					"<Variable name=\"me.slot\" />"
					"<Variable name=\"me.put\" />"
					"<Variable name=\"me.type\" />"
					"<Variable name=\"me.not_sw\" />"
					"<Variable name=\"me.sw\" />"
					"<Variable name=\"me.sw2\" />"
				"</Loc>";

			def_sub_vars(xml);		//局域变量定义
			sub_allow_sw(ele);	//看可允许的SW

			memset(put_ins, 0, sizeof(put_ins));
			tmp = ele->Attribute("put");
			if ( tmp && strlen(tmp) > 0 ) 
			{
				squeeze(tmp, put_ins);
				sv_set.put_still("me.put", put_ins);
			} else {
				sv_set.put_still("me.put", "80D8018143");
			}

			memset(put_type, 0, sizeof(put_type));
			tmp = ele->Attribute("type");
			if ( tmp && strlen(tmp) > 0 ) 
			{
				squeeze(tmp, put_type);
				sv_set.put_still("me.type", put_type);
			} else {
				sv_set.put_still("me.type", "80");
			}

			ref_prepare_sub(ele->Attribute("ori_key"),  ori_kvar, var_set, "orikey");
			ref_prepare_sub(ele->Attribute("neo_key"),  neo_kvar, var_set, "neokey");
			get_entry(ori_kvar, map_root, entry_nm);	//以旧密钥的pro作子序列名
			return defer_sub_serial(ele, sub_ins_entry, var_set, &sv_set);
		};
	};
	
	struct ResetIns: public ComplexSubSerial {
		const char *pro_name;	//复位时使用的一个子序列名
		int set ( TiXmlElement *ele, struct PVar_Set *var_set, TiXmlElement *map_root, const char *entry_nm)
		{
			char pro_nm[128];
			pro_name = ele->Attribute("pro");
			if ( pro_name ) 
			{
				TEXTUS_SNPRINTF(pro_nm, sizeof(pro_nm), "%s%s", entry_nm, pro_name);
				sub_ins_entry = map_root->FirstChildElement(pro_nm);
			} else {
				sub_ins_entry = map_root->FirstChildElement(entry_nm);
			}
			return defer_sub_serial(ele, sub_ins_entry, var_set, 0);
		};
	};

	struct User_Command {		//INS_User指令定义
		int order;
		struct ComplexSubSerial *complex;
		int comp_num;

		//enum Command_Type type;	//类型, 不用union类型, 真不知道如何调用这个构造函数
		//	struct PlainIns plain;
		//	struct HsmIns hsm;
		//	struct CallFun fun;

		//	int auth_num;
		//	struct ExtAuthIns *auth;
		//	struct ComplexSubSerial *complex;

		//	/* -------- 对终端基本操作 ----------*/
		//	struct PromptOp prompt;
		//	struct FeedCardOp fcard;
		//	struct OutCardOp ocard;
		//	struct ProRstOp pro_rst;
		//	/* -------- 对终端基本操作 ----------*/

		//void set (Command_Type mtype, int cent)
		//{
		//	order = -1;
		//	type = mtype;
		//	if ( type == OP_Prompt )
		//	{
		//		prompt.set(cent);
		//	}
		//};
		//void set (Command_Type mtype)
		//{
		//	order = -1;
		//	type = mtype;
		//	switch (mtype)
		//	{
		//		/* -------- 对终端基本操作 ----------*/
		//		case OP_FeedCard:
		//			fcard.set();
		//			break;

		//		case OP_OutCard:
		//			ocard.set();
		//			break;

		//		default:
		//			break;
		//	}
		//};

		int  set ( TiXmlElement *app_ele, struct PVar_Set *vrset, TiXmlElement *map_root, struct ComplexSubSerial *pool) //返回对IC的指令数
		{
			TiXmlElement *sub_serial, *spro, *me, *pri;
			const char *pri_nm;
			const char *nm = app_ele->Value();
			int comp_num , ret_ic;

			complex = pool;
			app_ele->QueryIntAttribute("order", &order);

			sub_serial = map_root->FirstChildElement(nm); //前面已经分析过了, 这里肯定不为NULL,nm就是Command之类的。
			me = sub_serial->FirstChildElement("Me");
			pri_nm = me->Attribute("primary");
			if ( pri_nm)
			{
				if ( app_ele->Attribute(pri_nm))
				{
					/* pro_analyze 根据变量名, 也就是属性名或元素内容, 去找到实际真正的变量内容(必须是参考变量)。变量内容中指定了Pro等 */
					comp_num = 1;
					ret_ic = complex[0].pro_analyze(app_ele->Attribute(pri_nm));
				} else 
				{
					for( pri = app_ele->FirstChildElement(pri_nm), comp_num = 0; 
						pri_ele; 
						pri = pri->NextSiblingElement(pri_nm) )
					{
						ret_ic = complex[0].pro_analyze(key->GetText());
						comp_num++;
					}
				}
			} else {
				c_num = 1;
				ret_ic = complex[0].pro_analyze(0);
			}

			return ret_ic;


			//
			//if ( strcasecmp(ele->Value(), "Command") == 0 ) 
			//{
			//	type = INS_Plain;
			//	plain.set(ele, vrset);
			//	return 1;
			//};

			//if ( strcasecmp(ele->Value(), "HSM") == 0 ) 
			//{
			//	type = INS_HSM;
			//	hsm.set(ele ,vrset);
			//	return 0;
			//};

			//if ( strcasecmp(ele->Value(), "Call") == 0 ) 
			//{
			//	type = INS_Call;
			//	fun.set(ele ,vrset);
			//	return 0;
			//};

			//if ( strcasecmp(ele->Value(), "ExtAuth") == 0 ) 
			//{
			//	TiXmlElement *key;
			//	int i,j;
			//	const char *tag="key";

			//	i = 0;
			//	key =  ele->FirstChildElement(tag);	
			//	while ( key )
			//	{
			//		if ( key->GetText() )
			//		{
			//			i++;
			//		}
			//		key = key->NextSiblingElement(tag);
			//	}
			//	auth_num = i;
			//	auth = new struct ExtAuthIns[auth_num];

			//	i = 0; j = 0;
			//	key =  ele->FirstChildElement(tag);	
			//	while ( key )
			//	{
			//		if ( key->GetText() )
			//		{
			//			j += auth[i].set(ele, vrset, map_root, "ExtAuth", key);
			//			i++;
			//		}
			//		key = key->NextSiblingElement(tag);
			//	}

			//	type = INS_ExtAuth;
			//	return j;
			//};

			//if ( strcasecmp(ele->Value(), "DesMac") == 0 ) 
			//{
			//	type = INS_DesMac;
			//	complex = new struct DesMacIns;
			//	return complex->set(ele, vrset, map_root, "DesMac");
			//};

			//if ( strcasecmp(ele->Value(), "DesFile") == 0 ) 
			//{
			//	type = INS_DesMac;
			//	complex = new struct DesMacIns;
			//	return complex->set(ele, vrset, map_root, "DesMac");
			//};

			//if ( strcasecmp(ele->Value(), "PMac") == 0 ) 
			//{
			//	type = INS_Mac;
			//	complex = new struct  MacIns;
			//	return complex->set(ele, vrset, map_root, "PMac");
			//};

			//if ( strcasecmp(ele->Value(), "Load") == 0 ) 
			//{
			//	type = INS_Charge;
			//	complex = new struct  ChargeIns;
			//	return complex->set(ele, vrset, map_root, "Load");
			//};

			//if ( strcasecmp(ele->Value(), "Scp02Kmc") == 0 ) 
			//{
			//	type = INS_GpKmc;
			//	complex = new struct  ChargeIns;
			//	return complex->set(ele, vrset, map_root, "Scp02Kmc");
			//};

			//if ( strcasecmp(ele->Value(), "LoadInit") == 0 ) 
			//{
			//	type = INS_LoadInit;
			//	complex = new struct LoadInitIns;
			//	return complex->set(ele, vrset, map_root, "LoadInit");
			//};

			//if ( strcasecmp(ele->Value(), "Debit") == 0 ) 
			//{
			//	type = INS_Debit;
			//	complex = new struct DebitIns;
			//	return complex->set(ele, vrset, map_root, "Debit");
			//};

			//if ( strcasecmp(ele->Value(), "Reset") == 0 ) 
			//{
			//	type = INS_ProRst;
			//	complex = new struct ResetIns;
			//	pro_rst.set();
			//	return complex->set(ele, vrset, map_root, "Reset");
			//};

			//type = INS_None;
			//return 0;
		};
	};

	struct INS_Set {	
		struct User_Command *instructions;
		int many;
		struct ComplexSubSerial *comp_pool;
		INS_Set () 
		{
			instructions= 0;
			many = 0;
			comp_pool = 0;
		};

		~INS_Set () 
		{
			if (instructions ) delete []instructions;
			if (comp_pool ) delete []comp_pool;
			instructions = 0;
			many = 0;
		};

		int is_ins(TiXmlElement *app_ele, TiXmlElement *map_root, struct PVar_Set *var_set)
		{
			TiXmlElement *sub_serial, *spro, *me, *pri;
			const char *pri_nm;
			const char *nm = app_ele->Value();
			int c_num  =1;

			if ( var_set->is_var(nm)) return 0;

			sub_serial = map_root->FirstChildElement(nm); 
			if ( !sub_serial ) 
				return 0;
			
			me = sub_serial->FirstChildElement("Me");			
			pri_nm = me->Attribute("primary");
			if ( pri_nm)
			{
				if ( app_ele->Attribute(pri_nm))
					c_num = 1;
				else 
				{
					for( 	pri = app_ele->FirstChildElement(pri_nm); 
						pri_ele; 
						pri = pri->NextSiblingElement(pri_nm) )
					{
						c_num++;
					}
				}
			} else 
				c_num = 1;

			spro = sub_serial->FirstChildElement("Pro");
			if ( !spro ) 
				return 0;

			return c_num;
			
		};

		void put_inses(TiXmlElement *root, struct PVar_Set *var_set, TiXmlElement *map_root)
		{
			TiXmlElement *icc_ele;
			int mor, cor, vmany, refny, i, comp_num, i_num ;

			icc_ele= root->FirstChildElement(); refny = 0;
			comp_num = 0;
			while(icc_ele)
			{
				if ( icc_ele->Value() )
				{
					i = is_ins(icc_ele, map_root, var_set) )
					if ( i > 0 )
					{
						refny++;
						comp_num += i;
						
					}
				}
				icc_ele = icc_ele->NextSiblingElement();
			}
			//初步确定变量数
			many = refny ;
			instructions = new struct User_Command[many];
			comp_pool = new struct ComplexSubSerial[comp_num];
			vmany = 0;
			
			icc_ele= root->FirstChildElement(); mor = 0;i = 0;
			comp_num = 0;
			i_num = 0;
			while(icc_ele)
			{
				if ( icc_ele->Value() )
				{
					i = is_ins(icc_ele, map_root, var_set) )
					comp_num += i;
					if ( i > 0 )
					{
						cor = 0;
						icc_ele->QueryIntAttribute("order", &(cor)); 
						if ( cor <= mor ) goto ICC_NEXT;	//order不符合顺序的，略过
						i_num += instructions[vmany].set(icc_ele, var_set, map_root, &comp_pool[comp_num-i]);
						mor = cor;
						vmany++;
					}
				}
				ICC_NEXT:
				icc_ele = icc_ele->NextSiblingElement();
			}
			many = vmany; //最后再更新一次指令数
		};
	};	
	/* User_Command指令集定义结束 */
	
	struct  Personal_Def	//个人化定义
	{
		TiXmlDocument doc_c;	//User_Command指令定义；
		TiXmlElement *c_root;	
		TiXmlDocument doc_k;	//map：Variable定义，子序列定义
		TiXmlElement *k_root;

		TiXmlDocument doc_v;	//其它Variable定义
		TiXmlElement *v_root;
		
		struct PVar_Set person_vars;
		struct INS_Set ins_all;

		char flow_md[64];	//指令流指纹数据
		const char *flow_id;	//指令流标志
		const char *description;	//具体描述

		inline Personal_Def () {
			c_root = k_root = v_root = 0;
		};
		inline ~Personal_Def () {};

		int load_xml(const char *f_name, TiXmlDocument &doc,  TiXmlElement *&root, const char *md5_content)
		{
			doc.SetTabSize( 8 );
			if ( !doc.LoadFile (f_name) || doc.Error()) 
			{
				TEXTUS_SPRINTF(err_global_str, "Loading %s file failed in row %d and column %d, %s", f_name, doc.ErrorRow(), doc.ErrorCol(), doc.ErrorDesc());
				return -1;
			} 
			if ( md5_content)
			{
  				FILE *inFile;
  				MD5_CTX mdContext;
  				int bytes;
  				unsigned char data[1024];
				char md_str[64];
				unsigned char md[16];
				
				if ( strlen(md5_content) < 10 )
					return -3;

				TEXTUS_FOPEN(inFile, f_name, "rb");
  				if ( !inFile ) 
    					return -2;

  				MD5_Init (&mdContext);
  				while ((bytes = fread (data, 1, 1024, inFile)) != 0)
    					MD5_Update (&mdContext, data, bytes);

  				MD5_Final (&md[0], &mdContext);
				byte2hex(md, 16, md_str);
  				fclose (inFile);
				md_str[32] = 0;
				if ( strncasecmp(md_str, md5_content, 10) != 0) 
				{
					TEXTUS_SPRINTF(err_global_str, "Loading %s file failed in md5 error", f_name);
					return -3;
				}
  			}

			root = doc.RootElement();
			return 0;
		};

		void set_here(TiXmlElement *root)
		{
			TiXmlElement *var_ele;
			const char *vn="Variable";
			struct PVar *cv;

			if ( !root ) return;
			for (var_ele = root->FirstChildElement(vn); var_ele; var_ele = var_ele->NextSiblingElement(vn) ) 
			{	
				cv = person_vars.look(var_ele->Attribute("name")); //在已有的变量定义中寻找对应的
				if ( !cv ) continue; 	//无此变量, 略过
				cv->put_herev(var_ele);
			}
		};

		bool put_def( TiXmlElement *per_ele, TiXmlElement *key_ele_default)
		{
			const char *ic_nm, *map_nm, *v_nm;

			if ( (ic_nm = per_ele->Attribute("icc")))
				load_xml(ic_nm, doc_c,  c_root, per_ele->Attribute("md5"));
			else
				c_root = per_ele->FirstChildElement("IC_Personalize");

			if ( !c_root)
				return false;

			if ( (map_nm = per_ele->Attribute("key")))
				load_xml(map_nm, doc_k,  k_root, per_ele->Attribute("key_md5"));
			else
				k_root = per_ele->FirstChildElement("Key");

			if( !k_root ) k_root = key_ele_default;//prop中的key元素(密钥索引表), 则当本地无内容提供缺省。

			if ( (v_nm = per_ele->Attribute("var")))
				load_xml(v_nm, doc_v,  v_root, per_ele->Attribute("var_md5"));
			else
				v_root = per_ele->FirstChildElement("Var");

			if ( c_root)
			{
				if ( k_root ) 
					person_vars.defer_vars(k_root, c_root);	//变量定义, map文件优先
				else
					person_vars.defer_vars(c_root);	//变量定义, map文件优先
				flow_id = c_root->Attribute("flow");
				description = c_root->Attribute("desc");
				squeeze(per_ele->Attribute("md5"), flow_md);
			}
			set_here(c_root);	//再看本定义
			set_here(v_root);	//看看其它变量定义,key.xml等

			ins_all.put_inses(c_root, &person_vars, k_root);//指令集定义, 把变量定义输进去, 很多指令就是现成的了.
			return true;
		}
	};
	
	struct PersonDef_Set {	//User_Command集合之集合
		int num_icp;
		struct Personal_Def *icp_def;
		int max_snap_num;

		PersonDef_Set () 
		{
			icp_def = 0;
			num_icp = 0;
			max_snap_num = 0;
		};

		~PersonDef_Set () 
		{
			if (icp_def ) delete []icp_def;
			icp_def = 0;
			num_icp = 0;
		};

		void put_def(TiXmlElement *prop)	//个人化集合输入定义PersonDef_Set
		{
			TiXmlElement *key_ele, *per_ele;
			const char *vn = "personalize";
			int kk;
			int dy_at;
			key_ele = prop->FirstChildElement("key"); //如有一个key元素(指明密码机), 则为以下personalize提供缺省
			num_icp = 0; 
			for (per_ele = prop->FirstChildElement(vn); per_ele; per_ele = per_ele->NextSiblingElement(vn) ) 
				num_icp ++; 

			icp_def = new struct Personal_Def [num_icp];
			per_ele = prop->FirstChildElement(vn); kk = 0; 
			for (; per_ele; per_ele = per_ele->NextSiblingElement(vn), kk++ ) 
			{
				if ( !(icp_def[kk].put_def(per_ele, key_ele)))
					kk--;
				else { 
					dy_at = icp_def[kk].person_vars.dynamic_at;
					if ( dy_at > max_snap_num ) max_snap_num = dy_at;
				}
			}
			num_icp = kk; //实际再更新一下
			//if( kk > 0 ) {int *a =0 ; *a = 0; };
		};

		/* 根据flowid, 找一个合适的个人化配置 */
		struct Personal_Def *look(const char *fid) 
		{
			struct Personal_Def *def=0;
			for ( int i = 0 ; i < num_icp; i++ )
			{
				if ( !icp_def[i].flow_id ) continue;
				if (strcasecmp(fid, icp_def[i].flow_id) == 0 )
				{
					def = &icp_def[i];
					break;
				}
			}
			return def;
		};
	};

class PacWay: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	void handle_pac();	//左边状态处理
	bool sponte( Pius *);
	Amor *clone();
		
	PacWay();
	~PacWay();
	/* 制卡处理进入方式 */
	enum MK_Mode { FROM_START = 0,  FROM_HSM = 0x11 , FROM_ICTERM = 0x22, FROM_OTHER =99};
	enum Work_Mode { TO_HSM = 0x11 , TO_ICTERM = 0x22, TO_NONE=0};

private:
	bool spo_term_hand();	//右边状态处理
	void h_fail(char tmp[], char desc[], int p_len, int q_len, const char *p, const char *q, const char *fun);
	bool mk_hand(MK_Mode mmode);	
	void mk_result();

	void pro_com_R(const char *request, const int rlen, unsigned char slot);
	void pro_com_S(char *response, size_t len, unsigned short &sw);

	void hsm_com_R(const char *com, int in_len, const char *kloc);
	int hsm_com_S(char *&res, int &out_len, const char *ref, int head_len);
	void hsm_fun_R(const char *com, int in_len, const char *kloc);
	void hsm_fun_S(char *&res, int &out_len);

	int ins_plain_pro(struct PlainIns *);
	int ins_hsm_pro( struct HsmIns * );

	struct G_CFG 	//全局定义
	{
		TiXmlElement *prop;
		Work_Mode wmod;
		struct PersonDef_Set person_defs;

		int hcmd_fldno;		/* 密码机指令域号 */
		int fldOffset;	/* 处理PacketObj时, 定义中的域号加上此值(偏移量)即实际处理的域, 初始为0 */
		int maxium_fldno;		/* 最大域号 */

		int flowID_fld_no;	//流标识域, 业务代码域, 
		int error_fld_no;	//错误码域,
		int errDesc_fld_no;	//错误消息域
		int station_fld_no;	//工作站标识域

		inline G_CFG() {
			wmod =  TO_NONE;
			fldOffset = 0;
			maxium_fldno = 64;
			flowID_fld_no = 3;
			error_fld_no = 39;
			errDesc_fld_no = 40;	
			station_fld_no = 59;
		};	
		inline ~G_CFG() {
		};
	};

	PacketObj hi_req, hi_reply; /* 向右传递的, 可能是对HMS, 或是对IC终端 */
	PacketObj *hipa[3];
	PacketObj *rcv_pac;	/* 来自左节点的PacketObj */
	PacketObj *snd_pac;

	struct MK_Session mess;	//记录一个制卡过程中的各种临时数据
	struct Personal_Def *cur_def;	//当前定义

	struct G_CFG *gCFG;     /* 全局共享参数 */
	bool has_config;
	PacWay *tohsm_pri;
	PacWay *toterm;
	PacWay *tohsm_slv;
	Amor::Pius loc_pro_pac;

	char res_buf[1024], hsm_cmd_buf[2048], icc_cmd_buf[1024];
	MINLINE void deliver(TEXTUS_ORDO aordo);

	enum Back_Status {BS_Requesting = 0, BS_Answered = 1, BS_Crashed = 2 };
	Back_Status back_st;		//右处理状态

	struct WKBase {
		int step;	//0: send, 1: recv 
	};

	struct CompWKBase:public WKBase {
		int sub_which;
	} ;

	struct CompWKBase comp_wt;	//子序列工作状态

	struct _WP:public WKBase {
		char com[16];
	} plain_wt;

	struct _WAU:public CompWKBase {
		int cur;
	} auth_wt;

	/* 以下对IC卡5种基本操作, 设这些操作的原因是因为异步操作, 需要来保存过程状态 */
	struct _WFC:public WKBase {
		struct FeedCardOp fcard;	//进卡
	} fcard_wt;

	struct _WOC:public WKBase {		//出卡
		struct OutCardOp ocard;
	} ocard_wt;

	struct _WPST:public WKBase {		//卡复位
		struct ProRstOp pro_rst;
		int try_num;
	} pro_rst_wt;

	struct _WPMT:public WKBase {
		struct PromptOp prompt;		//进度提示
	} prompt_wt;

	struct _WPCOM {
		char cmd_buf[1024];
		struct ProComOp procom;		//卡指令
	} procom_wt;
	
	struct _HMOP:public WKBase {		//HSM 指令
		char cmd[2048];
		int len;
		int ci;
		const char *loc;
	} hsmcom_wt;

	struct TermOPBase* get_ic_base();
	void me_sub_zero()
	{
		plain_wt.step = 0;
		hsmcom_wt.step = 0;
		hsmcom_wt.cmd[0] = 0;
		hsmcom_wt.len = 0;
		hsmcom_wt.ci = 1;
		hsmcom_wt.loc = (const char*)0;
		procom_wt.cmd_buf[0]=0;
	};
	void me_zero()
	{
		me_sub_zero();
		fcard_wt.step=0;
		ocard_wt.step=0;
		prompt_wt.step=0;
		pro_rst_wt.step=0;
		pro_rst_wt.try_num=1;

		auth_wt.step=0;
		auth_wt.cur = 0;
		comp_wt.step = 0;
	};
	int sub_serial_pro( struct INS_SubSet *si_set, struct CompWKBase *wk);
	#include "wlog.h"
};

void PacWay::ignite(TiXmlElement *prop)
{
	const char *comm_str;
	if (!prop) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		gCFG->prop = prop;
		has_config = true;
	}

	if ( (comm_str = prop->Attribute("work_mode")) )
	{
		if ( strcmp(comm_str, "icterm") == 0 )
			gCFG->wmod = TO_ICTERM;
		if ( strcmp(comm_str, "hsm") == 0 )
			gCFG->wmod = TO_HSM;
	}

	if ( (comm_str = prop->Attribute("max_fld")) )
		gCFG->maxium_fldno = atoi(comm_str);

	if ( gCFG->maxium_fldno < 0 )
		gCFG->maxium_fldno = 0;
	hi_req.produce(gCFG->maxium_fldno) ;
	hi_reply.produce(gCFG->maxium_fldno) ;

	prop->QueryIntAttribute("hsm_cmd_fld", &gCFG->hcmd_fldno);

	prop->QueryIntAttribute("business_code", &gCFG->flowID_fld_no);
	prop->QueryIntAttribute("error_code", &gCFG->error_fld_no);
	prop->QueryIntAttribute("error_desc", &gCFG->errDesc_fld_no);
	prop->QueryIntAttribute("workstation", &gCFG->station_fld_no);

	return;
}

PacWay::PacWay()
{
	hipa[0] = &hi_req;
	hipa[1] = &hi_reply;
	hipa[2] = 0;

	gCFG = 0;
	has_config = false;
	tohsm_pri = (PacWay*) 0;
	tohsm_slv = (PacWay*) 0;
	loc_pro_pac.ordo = Notitia::PRO_UNIPAC;
	loc_pro_pac.indic = 0;
}

PacWay::~PacWay() 
{
	if ( has_config  )
	{	
		if(gCFG) delete gCFG;
	}
}

Amor* PacWay::clone()
{
	PacWay *child = new PacWay();
	child->gCFG = gCFG;
	child->hi_req.produce(hi_req.max);
	child->hi_reply.produce(hi_reply.max);
	return (Amor*) child;
}

bool PacWay::facio( Amor::Pius *pius)
{
	PacketObj **tmp;

	switch ( pius->ordo )
	{
	case Notitia::SET_UNIPAC:
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
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
		WBUG("facio PRO_UNIPAC %s",  gCFG->wmod == TO_ICTERM ? "TERM" : "other" );
		if (gCFG->wmod != TO_ICTERM)  //如果不是对制卡终端的，那么不接受左节点的数据
			break;

		handle_pac();
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		gCFG->person_defs.put_def(gCFG->prop);
		mess.init(gCFG->person_defs.max_snap_num);
		if ( err_global_str[0] != 0 )
		{
			WLOG(ERR,"%s", err_global_str);
		}
		goto DEL_PAC;
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
		mess.init(gCFG->person_defs.max_snap_num);
DEL_PAC:
		deliver(Notitia::SET_UNIPAC);
		if (gCFG->wmod == TO_HSM)  //如果是对HSM，就告诉那个对ICTERM的, 这里处理HSM的. 外部的配置文件要支持一下Fly
		{
			deliver(Notitia::WHO_AM_I);
		}
		break;

	case Notitia::START_SESSION:
		WBUG("facio START_SESSION" );
		mess.reset();
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION" );
		mess.reset();
		break;

	default:
		return false;
	}
	return true;
}

bool PacWay::sponte( Amor::Pius *pius)
{
	assert(pius);
	if (!gCFG )
		return false;

	switch ( pius->ordo )
	{
	case Notitia::PRO_UNIPAC:
		WBUG("sponte PRO_UNIPAC %s",  gCFG->wmod == TO_ICTERM ? "TERM" : "HSM" );
		if (gCFG->wmod == TO_HSM) 
		{
			if ( !toterm )
			{
				WBUG("bug!! hsm don't know how to ic term!!");
			} else {
				toterm->mk_hand(FROM_HSM); //访问终端的那个节点????
			}
			goto H_END;
		}

		if (gCFG->wmod != TO_ICTERM)  //如果不是对制卡终端的，那么不接受左节点的数据
		{
			WBUG("bug!! not ic term!!");
			goto H_END;
		}

		if ( !mk_hand( FROM_ICTERM))	//如果没有处理, 就当其它处理了.  
		{
			back_st = BS_Answered; 		//有数据回来了。
			if ( !spo_term_hand() )	//终端的其它处理数据
			{
				if ( mess.right_status == RT_IDLE )
				{
					WLOG(WARNING, "not expected from ic term");
				} else {
					WBUG("bug not for term!!!!!!");
				}
			}
		}
H_END:
		break;

	case Notitia::WHO_AM_I:
		WBUG("sponte WHO_AM_I(%p)", (PacWay*)(pius->indic));
		if( this->tohsm_pri)
		{
			this->tohsm_slv = (PacWay*)(pius->indic);
			this->tohsm_slv->toterm = this;
		} else {
			this->tohsm_pri = (PacWay*)(pius->indic);
			this->tohsm_pri->toterm = this;
		}
		break;

	case Notitia::DMD_END_SESSION:	//右节点关闭, 要处理
		WBUG("sponte DMD_END_SESSION");
		if ( mess.left_status == LT_IDLE)	/* 左边节点没有请求数据, 这里也不需要处理 */
			break;
		if (gCFG->wmod == TO_HSM) 
		{
			if ( !toterm )
			{
				WBUG("bug!! hsm don't know how to ic term!!");
			} else {
				toterm->mess.iRet = ERROR_HSM_TCP;
				TEXTUS_SPRINTF(toterm->mess.err_str, "hsm down for %s",  mess.station_str);
				toterm->mk_result(); //访问终端的那个节点????
			}
			goto D_END;
		}

		if (gCFG->wmod != TO_ICTERM)  //如果不是对制卡终端的，那么不接受左节点的数据
		{
			WBUG("bug!! not ic term!!");
			goto D_END;
		}

		if ( mess.left_status == LT_MKING  )	//表明是制卡工作
		{
			mess.iRet = ERROR_MK_TCP;
			TEXTUS_SPRINTF(mess.err_str, "term device down at %s",  mess.station_str);
			WLOG(WARNING, mess.err_str);
			mk_result();	//结束
		} else {
			back_st = BS_Crashed; 		//有数据回来了。
			spo_term_hand();	//如果状态不对, 也不回
		}
D_END:
		break;

	case Notitia::START_SESSION:	//右节通知, 已活
		break;

	default:
		return false;
		break;
	}
	return true;
}

/* 返回表明是否终端操作 */
bool PacWay::spo_term_hand()
{
	unsigned char *p, *q, *r;
	unsigned long p_len, q_len, r_len;

	switch ( mess.right_status) 
	{
	case RT_TERM_TEST:	//制卡终端测试
		if ( back_st == BS_Crashed )           
		{
			TEXTUS_SPRINTF(mess.err_str,  "CTst Crashed at %s", mess.station_str); 
			WLOG(WARNING, "%s", mess.err_str);
			snd_pac->input(gCFG->error_fld_no, "A", 1);	//返回3域为结果
			snd_pac->input(4, mess.err_str, strlen(mess.err_str));
			goto H_END;
		}
		if ( back_st != BS_Answered )
		{
			WBUG("bug!! back_st != BS_Answered");
		}

		p_len = q_len =  r_len = 0;
		p = hi_reply.getfld(1, &p_len);	//响应功能字
		q = hi_reply.getfld(2, &q_len);	//结果码
		r = hi_reply.getfld(3, &r_len);	//3域为测试结果返回

		if ( p_len != 1 || memcmp(p, "t",1) != 0 || q_len != 1 || r_len < 1 )
		{
			TEXTUS_SPRINTF(mess.err_str,  "CTst RPC_ERROR at %s", mess.station_str); 
			WLOG(WARNING, "%s", mess.err_str);
			snd_pac->input(gCFG->error_fld_no, "C", 1);	//通讯报文的问题
			snd_pac->input(gCFG->errDesc_fld_no, mess.err_str, strlen(mess.err_str));
			goto H_END;
		}

		if ( *q != '0' ) 
		{
			char tmp[128];
			memcpy(tmp, r, r_len > 120 ? 120 : r_len);
			tmp[r_len > 120 ? 120 : r_len] = 0;
			WLOG(WARNING, "CTst DEV_ERROR(%s) at  %s", tmp, mess.station_str);
			snd_pac->input(gCFG->error_fld_no, (unsigned char*)"B", 1);	//设备工作有问题
			snd_pac->input(gCFG->errDesc_fld_no, r, r_len);
			goto H_END;
		}
		/* 设备OK */
		snd_pac->input(gCFG->error_fld_no, (unsigned char*)"0", 1);	//返回3域为结果
H_END:
		mess.left_status = LT_IDLE;
		mess.right_status = RT_IDLE;
		aptus->sponte(&loc_pro_pac);    //回应给控制台数据
		break;

	default:
		return false;
		break;
	}
	return true;
}

void PacWay::handle_pac()
{
	unsigned char *p;
	int plen=0;
	int i;

	unsigned char *actp;
	unsigned long alen;
	char desc[64];

	struct PVar  *vt;
	struct DyVar *dvr;

	actp=rcv_pac->getfld(gCFG->flowID_fld_no, &alen);		//取得业务代码, 即流标识
	if ( !actp)
	{
		WBUG("business code field null");
		goto END;
	}

	switch ( mess.left_status) 
	{
	case LT_IDLE:
		if ( alen==4 &&  memcmp(actp, "\x00\x00\x00\x00", alen) ==0 ) 
		{ /* 建立对两个通道的对应关系 */
			snd_pac->reset();
			plen = 0;
			p = rcv_pac->getfld(gCFG->station_fld_no, &plen);		//终端标识
			if ( p ) {
				if ( plen > (int)sizeof(mess.station_str) )
					plen =  (int)sizeof(mess.station_str);
				memcpy(mess.station_str, p, plen);
				mess.station_str[plen] = 0;
			}

			WLOG(INFO, "MKInit dev is %s", mess.station_str);
			hi_req.reset();
			hi_reply.reset();
			hi_req.input(1,(unsigned char*)"T", 1);
			hi_req.input(2, p, plen);

			back_st = BS_Requesting;
			mess.left_status = LT_INITING;
			mess.right_status = RT_TERM_TEST;
			aptus->facio(&loc_pro_pac);     //向右发出指令, 然后等待. 这里会启动连接. 放在最后很重要
			goto HERE_END;
		}	
	
		/* 这里就是一般的业务啦 */
		mess.reset();	//会话复位
		snd_pac->reset();
		actp = rcv_pac->getfld(gCFG->flowID_fld_no, &alen);
		snd_pac->input(gCFG->flowID_fld_no, actp, alen);
		if (alen > 63 ) alen = 63;
		memcpy(mess.flow_id, actp, alen);
		mess.flow_id[alen] = 0;

		p=rcv_pac->getfld(gCFG->station_fld_no, &plen);
		if ( p ) {
			if ( plen > (int)sizeof(mess.station_str) )
				plen =  (int)sizeof(mess.station_str);
			memcpy(mess.station_str, p, plen);
			mess.station_str[plen] = 0;
		}
		
		cur_def = gCFG->person_defs.look(mess.flow_id); //找一个相应的指令流定义
		if ( !cur_def ) 
		{
			TEXTUS_SPRINTF(mess.err_str, "not defined flow_id: %s ", mess.flow_id );
			mess.iRet = ERROR_INS_DEF;	
			mk_result();	//工作结束
			goto HERE_END;
		}
		/* 制卡任务开始  */
		mess.snap[Pos_FlowTotal].input( cur_def->ins_all.many);
		if ( cur_def->flow_md[0] )
			mess.snap[Pos_FlowPrint].input( cur_def->flow_md, strlen(cur_def->flow_md));

		/* 寻找变量集中所有动态的, 看看是否有start_pos和get_length的, 根据定义赋值到mess中 */
		for ( i = 0 ; i <  cur_def->person_vars.many; i++)
		{
			vt = &cur_def->person_vars.vars[i];
			if ( vt->dynamic_pos >=0 )
			{
				dvr = &mess.snap[vt->dynamic_pos];
				dvr->kind = vt->kind;
				dvr->dest_fld_no = vt->dest_fld_no;
				dvr->def_var = vt;
				if ( vt->c_len > 0 )	//先把定义的静态内容拷一遍, 动态变量的默认值
				{
					dvr->input(&(vt->content[0]), vt->c_len);
				}
				if ( vt->source_fld_no >=0 )
					p = rcv_pac->getfld(vt->source_fld_no, &plen);
				else
					continue;
				if ( p && vt->get_length > 0 )	//如果不设域号或取的长度, 动态变量就不取
				{
					if ( plen > vt->start_pos )
					{
						plen -= (vt->start_pos-1);	//plen为实际能取的长度, start_pos是从1开始
						dvr->input( (const char*)&(p[vt->start_pos-1]), plen < vt->get_length ? plen : vt->get_length);
					}
				}
				/* 所以从域取值为优先, 如果实际报文没有该域, 就取这里的静态定义 */
			}
		}
	//{int *a =0 ; *a = 0; };

		WLOG(NOTICE, "Current cardno is %s by %s at %s",  mess.snap[Pos_CardNo].val, cur_def->flow_id, mess.station_str);
		mk_hand(FROM_START);	//启动!!，很重要, 这里的TRUE是指刚开始启动
HERE_END:
		break;

	case LT_INITING: //不接受, 不响应
		if ( alen > 20 ) alen =20;
		byte2hex(actp, alen, desc); desc[2*alen]= 0;
		WLOG(WARNING,"still testing mkdev! from console %s", desc);
		break;

	case LT_MKING:	//不接受, 不响应即可
		if ( alen > 20 ) alen =20;
		byte2hex(actp, alen, desc); desc[2*alen]= 0;
		WLOG(WARNING,"still making card! from console %s", desc);
		break;

	default:
		break;
	}
END:
	return;
}


void PacWay::pro_com_R(const char *request, const int rlen, unsigned char slot)
{
	hi_req.input(2, slot);
	hi_req.input(3, request, rlen);
	memcpy(procom_wt.cmd_buf, request, rlen);
	procom_wt.cmd_buf[rlen] = 0;
	mess.right_status = procom_wt.procom.rt_stat;
}

void PacWay::pro_com_S(char *response, size_t size_len, unsigned short &sw)
{
	unsigned char *p;
	unsigned int lsw;
	unsigned long out_len = 0;
	char str_sw[5];

	sw = 0;
	str_sw[0]= '\0';
	p = hi_reply.getfld(3, &out_len);
	if ( *p != '0' )
	{
		str_sw[0] = *p;
		str_sw[1] = 0;
		WLOG(WARNING, "pro_com %s, return %s at %s when order= %d in %s",  procom_wt.cmd_buf, str_sw, mess.station_str, mess.pro_order, cur_def->flow_id);
		return ;
	}
	p = hi_reply.getfld(4, &out_len);
	if ( size_len < out_len ) 
		out_len = size_len;
	if ( p )memcpy(response, p, out_len); 
	response[out_len] = 0;

	out_len = 0;
	p = hi_reply.getfld(5, &out_len);
	if ( out_len == 4 ) 
	{
		memcpy(str_sw, p, 4); str_sw[4] = 0;
		TEXTUS_SSCANF(str_sw,"%4x",&lsw);
		sw = lsw & 0x0000FFFF;
	}
	hi_reply.reset();

	WLOG(CRIT, "pro_com %s, return %s, sw=%4x", procom_wt.cmd_buf, response, sw);
	if ( sw != SW_OK ) 
	{
		WLOG(WARNING, "pro_com %s, ans %s, sw=%s at %s when order= %d in %s",  procom_wt.cmd_buf, response, str_sw, mess.station_str, mess.pro_order, cur_def->flow_id);
		return ;
	}
	return ;
}

/* hsm_fun_X 的, 是访问HSM的节点， 没有mk_sess数据!!!, 不要在这里访问! */
void PacWay::hsm_fun_R(const char *com, int in_len, const char *kloc)
{
	hi_req.reset();
	hi_reply.reset();
	if ( kloc ) 
		hi_req.input(1, kloc, strlen(kloc)); //功能C, 直接的指令	//这个实际用不着?
	else
		hi_req.input(1, "C", 1); //功能C, 直接的指令	//这个实际用不着?
	hi_req.input(gCFG->hcmd_fldno, com, in_len);
}

void PacWay::hsm_fun_S(char *&res, int &out_len)
{
	res = (char*)hi_reply.getfld(gCFG->hcmd_fldno, &out_len);
}

/* hsm_com_X, 是TERM的, 有mk_sess啦 */
void PacWay::hsm_com_R(const char *com, int in_len, const char *kloc)
{
	if ( !tohsm_pri ) 
	{
		WLOG(EMERG, "not define HSM!");
		return ;
	}
	mess.right_status = RT_HSM_ASK;
	
	if ( com) 
	{
		memcpy(hsmcom_wt.cmd, com, in_len);
		hsmcom_wt.cmd[in_len] = 0;
		hsmcom_wt.len = in_len;
		hsmcom_wt.loc = kloc;
		tohsm_pri->hsm_fun_R(com, in_len, kloc);
	} else 
		tohsm_pri->hsm_fun_R(hsmcom_wt.cmd, hsmcom_wt.len, hsmcom_wt.loc);
}

int PacWay::hsm_com_S(char *&res, int &out_len, const char *ref, int head_len )
{
	if ( !tohsm_pri ) 
	{
		WLOG(EMERG, "hsm not defined for %s",  mess.station_str);
		return ERROR_HSM_TCP;
	}
	tohsm_pri->hsm_fun_S(res, out_len);	//是到那个HSM节点取数据的。
	if (out_len < head_len ) 
	{
		WLOG(WARNING, "hsm_com RPC_ERR for %s",  mess.station_str);
		return ERROR_HSM_RPC;
	}

	res[out_len] = 0;
	WLOG(CRIT, "hsm_com %s, return %s", hsmcom_wt.cmd, res);
	if ( memcmp(res, ref, head_len) != 0  ) 	//hsm_cmd函数保证res_len >= 4
	{
		int m_len;
		if ( head_len+1 > (int) sizeof(mess.bad_sw))
			m_len = sizeof(mess.bad_sw) -1;
		else
			m_len = head_len;
		memcpy(mess.bad_sw, res, m_len);
		mess.bad_sw[m_len] = 0;
		WLOG(WARNING, "hsm_com return %s, for %s  when order= %d in %s",  mess.bad_sw, mess.station_str, mess.pro_order, cur_def->flow_id);
		if ( hsmcom_wt.ci > 0 )	//还可重试一次密码机
		{
			WLOG(WARNING, "HSM again %d", hsmcom_wt.ci);
			hsmcom_wt.ci--;
			hsm_com_R(0,0,0);
			return ERR_HSM_AGAIN;
		}
		return ERROR_HSM_FUN;
	}
	res = &res[head_len];	//把指针偏移4, 就避开了那个7100之类的东西.
	out_len -=head_len ;
	return 0;
}

/* 是否为制卡动作的返回 */
struct TermOPBase* PacWay::get_ic_base()
{
	struct TermOPBase *wbase = 0;
	switch ( mess.right_status) 
	{
	case RT_IC_COM:		//COS指令发出
		wbase = &procom_wt.procom;
		break;

	case RT_TERM_PROMPT:	//进度提示
		wbase = &prompt_wt.prompt;
		break;

	case RT_TERM_FEED:
		wbase = &fcard_wt.fcard;
		break;

	case RT_TERM_OUT:
		wbase = &ocard_wt.ocard;
		break;

	case RT_IC_RESET:	//IC复位
		wbase = &pro_rst_wt.pro_rst;
		break;

	default:
		break;
	}
	return wbase;
}

/* 这个集中错误处理, 针对制卡终端返回的进卡、出卡等 */
void PacWay::h_fail(char tmp[], char desc[], int p_len, int q_len, const char *p, const char *q, const char *fun)
{
	tmp[0] = *p;
	tmp[1] = 0;
	if ( q_len > 30 ) q_len = 30;
	if (q ) memcpy(desc, q, q_len );
	desc[q_len] = 0;
	TEXTUS_SPRINTF(mess.err_str, "%s return %s(%s), at %s", fun, tmp, desc, mess.station_str);
	WLOG(WARNING, "%s", mess.err_str);
}

int PacWay::ins_plain_pro(struct PlainIns *plain)
{
	int t_len;
	unsigned short sw;
	switch ( plain_wt.step)
	{
	case 0:
		if ( plain->dynamic ) 
		{
			plain->get_current(icc_cmd_buf, t_len, &mess);
			pro_com_R(icc_cmd_buf, t_len, plain->slot);
		} else {
			pro_com_R(plain->cmd_buf, plain->cmd_len, plain->slot);
		}
		break;
	case 1:
		pro_com_S(res_buf,  sizeof(res_buf)-1, sw);	//不是IC卡错的，都进不到这里
		if (  !plain->valid_sw(sw) ) 
		{
			TEXTUS_SPRINTF(mess.bad_sw, "%04X", sw);
			mess.iRet = ERROR_IC_INS;
			return -1;
		}
		plain->pro_response(res_buf, strlen(res_buf),  &mess);
		break;
	default:
		break;
	}
	plain_wt.step++; //指示下一步操作
	return plain_wt.step-1;	//-1: err; 0: ing; 1: finished
}

int PacWay::ins_hsm_pro( struct HsmIns *hsm)
{
	int t_len, res_len;
	char *p;
	switch ( hsmcom_wt.step)
	{
	case 0:
		hsmcom_wt.ci = hsm->fail_retry_num;
		if ( hsm->dynamic ) 
		{
			hsm->get_current(hsm_cmd_buf, t_len, &mess);
			hsm_com_R(hsm_cmd_buf, t_len, hsm->location);
		} else {
			hsm_com_R(hsm->cmd_buf, hsm->cmd_len, hsm->location);
		}
		break;
	case 1:
		mess.iRet = hsm_com_S(p, res_len, hsm->ok_ans_head, hsm->ok_ans_head_len);
		if ( mess.iRet == ERR_HSM_AGAIN )		//加密机重试
		{
			hsmcom_wt.step--;	//
			goto Pro_Ret;
		} else if ( mess.iRet !=0 ) 
			return -1;

		hsm->pro_response(p, res_len,  &mess);
		break;
	default:
		break;
	}
Pro_Ret:
	hsmcom_wt.step++;
	return hsmcom_wt.step-1; //-1: err; 0: ing; 1: finished
}

/* 子序列入口 */
int PacWay::sub_serial_pro( struct INS_SubSet *si_set, struct CompWKBase *wk)
{
	int i_ret=0, s_ret=0;
	struct Base_Command *ins;

	if ( !si_set )	//如果不存在子序列, 就当执行完成.
		return 1;
SUB_INS_PRO:
	ins = &(si_set->instructions[wk->sub_which]);
	i_ret = 1;
	switch ( ins->type)
	{
	case INS_Plain:
			if (plain_wt.step ==0 ) 
			{
				if ( !ins->plain_p->valid_condition(&mess) )
					break;
			}
			i_ret = ins_plain_pro(ins->plain_p);
			if ( i_ret > 0 && !ins->plain_p->valid_result(&mess) )
			{
				mess.iRet = ERR_RESULT_INVALID;
				i_ret = -1;
			}
		break;

	case INS_HSM:
			if ( hsmcom_wt.step ==0 ) 
			{
				if ( !ins->hsm_p->valid_condition(&mess) )
					break;
			}
			i_ret = ins_hsm_pro(ins->hsm_p);
			if ( i_ret > 0 && !ins->hsm_p->valid_result(&mess) )
			{
				mess.iRet = ERR_RESULT_INVALID;
				i_ret = -1;
			}
		break;

	case INS_Call:
			i_ret = ins->fun_p->callfun(&mess);
			if ( i_ret > 0 && !ins->fun_p->valid_result(&mess) )
			{
				mess.iRet = ERR_RESULT_INVALID;
				i_ret = -1;
			}
			//if (wk->sub_which == 2 ) {int *a =0 ; *a = 0; };
		break;
	default :
		break;
	}
	if ( i_ret > 0 )
	{
		wk->sub_which++;
		me_sub_zero();
		if ( wk->sub_which == si_set->many )
			s_ret = 1;	//指令已经执行完成
		else {
			goto SUB_INS_PRO;
		}
	} else if ( i_ret <= 0 ) {	//正在进行中,指令已备, 或者已经发生错误
		s_ret = i_ret;
	}

	if ( s_ret == -1 ) {int *a =0 ; *a = 0; };
	return s_ret;
}

bool PacWay::mk_hand(MK_Mode mmode)
{
	char tmp[64], desc[64];
	
	struct DyVar *dvr=0;
	struct User_Command *ins;
	struct TermOPBase *wbase ;
	char *p, *q, *r;
	unsigned long p_len, q_len, r_len;

	int i_ret;

#if defined(_WIN32) && (_MSC_VER < 1400 )
	struct _timeb now;
#else
	struct timeb now;
#endif
	struct tm *tdatePtr;
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
	struct tm tdate;
#endif
	char time_str[32];

	switch ( mmode)
	{
		case FROM_START:
			//初始，都设为非正常。
			mess.ins_which = 0;
			mess.iRet = 0;	//假定一开始都是OK。
			TEXTUS_STRCPY(mess.err_str, " ");
			mess.left_status = LT_MKING;
			mess.right_status = RT_INS_READY;	//指示终端准备开始工作, 
			#if defined(_WIN32) && (_MSC_VER < 1400 )
				_ftime(&now);
			#else
				ftime(&now);
			#endif
			#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
				tdatePtr = &tdate;
				localtime_s(tdatePtr, &now.time);
			#else
				tdatePtr = localtime(&now.time);
			#endif

			(void) strftime( time_str, sizeof(time_str), "%Y%m%d%H%M%S", tdatePtr );
			mess.snap[Pos_SysTime].input( time_str, strlen(time_str));
			goto INS_PRO; //刚开始工作，直接工作。 以下是处理从制卡终端或加密机的返回。

			break;

		case FROM_HSM:
			if ( mess.right_status != RT_HSM_ASK || mess.left_status != LT_MKING )
			{
				TEXTUS_SPRINTF(mess.err_str, "%s", "bug!! from hsm , but not asking hsm or making card, bug!!");
				WLOG(EMERG, "%s", mess.err_str);
				mess.iRet= ERROR_HSM_RPC;
				goto HsmErrPro;
			}
			break;

		case FROM_ICTERM: 
			if ( !(wbase = get_ic_base()) )  //不是制卡工作的响应, 返回(可能是终端测试、提示下一张卡等)
				return false;
			/* 制卡响应基础通讯数据检查 */
			p = (char*)hi_reply.getfld(1, &p_len);
			if ( p_len != (unsigned long)wbase->tlen || memcmp(p, wbase->take, wbase->tlen) != 0)	//功能字是否对应
			{
				if ( p_len > 30 ) p_len = 30; 
				memcpy(tmp, p, p_len ); 
				tmp[p_len] = 0;
				TEXTUS_SPRINTF(mess.err_str, "%s, RPC_ERROR(%s), at %s", wbase->fun, tmp, mess.station_str);
				WLOG(WARNING, "%s", mess.err_str);
				mess.iRet= ERROR_MK_RPC;
				goto CardErrPro;
			}

			q = (char*)hi_reply.getfld(2, &q_len);	//结果码长度有问题
			if ( q_len != 1 )
			{
				if ( q_len > 30 ) q_len = 30; memcpy(tmp, q, q_len ); tmp[q_len] = 0;
				TEXTUS_SPRINTF(mess.err_str, "%s, RPC_ERROR(%ld, %s), at %s", wbase->fun, q_len, tmp, mess.station_str);
				WLOG(WARNING, "%s", mess.err_str);
				mess.iRet= ERROR_MK_RPC;
				goto CardErrPro;
			}
			break;
		default:
			mess.iRet= ERROR_OTHER;
			goto CardErrPro;
			break;
	}

INS_PRO:
	ins = &(cur_def->ins_all.instructions[mess.ins_which]);
	mess.pro_order = ins->order;	

	if ( mess.right_status  ==  RT_INS_READY )	//终端空闲,各类工作单元清空复位
		me_zero();
	
	hi_req.reset();	//请求复位
	//{int *a =0 ; *a = 0; };
	switch ( ins->type)
	{
	case OP_Prompt:
			switch ( prompt_wt.step)
			{
			case 0:
				hi_req.input(2, ins->prompt.cent, ins->prompt.len);
				mess.right_status = ins->prompt.rt_stat;
				break;

			case 1: //进度提示的响应, 从来不管
				mess.right_status = RT_INS_READY ;	//右端闲, 可以下一条指令
				break;
			}
			prompt_wt.step++;	//指示下一步操作
		break;

	case OP_FeedCard:
			switch ( fcard_wt.step)
			{
			case 0:
				hi_req.input(2, mess.snap[Pos_CardNo].val, mess.snap[Pos_CardNo].c_len);
				hi_req.input(3, "0", 1);	//厂商标志用不着了
				hi_req.input(4, cur_def->description, strlen(cur_def->description));
				mess.right_status = ins->fcard.rt_stat;
				break;

			case 1: //进卡响应
				p = (char*)hi_reply.getfld(2, &p_len);
				if ( p_len != 1 || *p != '0' )
				{
					q = (char*)hi_reply.getfld(3, &q_len);
					h_fail(tmp, desc, p_len, q_len, p, q, "pro_feed");
					if ( *p == 'B' )
						mess.iRet = ERROR_MK_PAUSE;
					else
						mess.iRet = ERROR_MK_DEV;
					goto CardErrPro;
				}
				mess.right_status = RT_INS_READY ;	//右端闲, 可以下一条指令
				break;
			}
			fcard_wt.step++;	//指示下一步操作
		break;

	case OP_OutCard: //出卡操作, 有可能前面制卡失败
			switch ( ocard_wt.step)
			{
			case 0:
				hi_req.input(2, mess.iRet==0 ? "0":"A", 1);
				hi_req.input(3, mess.snap[Pos_CardNo].val, mess.snap[Pos_CardNo].c_len);
				mess.right_status = ins->ocard.rt_stat;
				break;
			case 1:	
				p = (char*)hi_reply.getfld(2, &p_len);
				if ( p_len != 1 || *p != '0' )
				{
					q = (char*)hi_reply.getfld(3, &q_len);
					h_fail(tmp, desc, p_len, q_len, p, q, "pro_out");
					mess.iRet = ERROR_MK_DEV;
					goto CardErrPro;
				}
				mess.right_status = RT_INS_READY ;	//右端闲, 可以下一条指令
				break;
			}
			ocard_wt.step++;

		break;

	case INS_ProRst:		//卡片复位 
			switch ( pro_rst_wt.step)
			{
			case 0:	//外层将功能字设上, 这里就设一下状态
				if ( !ins->complex->valid_condition(&mess) )
				{
					mess.right_status = RT_INS_READY ;	//右端闲, 可以下一条指令
					break;
				}

				mess.right_status = ins->pro_rst.rt_stat;
				pro_rst_wt.step++;
				break;
			case 1:
				p = (char*)hi_reply.getfld(3, &p_len);	//2域是slot
				r = (char*)hi_reply.getfld(7, &r_len);	//UID
				if ( p_len != 1 || *p != '0' ||!r )
				{
					q = (char*)hi_reply.getfld(4, &q_len);
					h_fail(tmp, desc, p_len, q_len, p, q, "pro_reset");
					mess.iRet = ERROR_IC_INS;		//复位失败, 就是卡而已, 不关设备
					goto CardErrPro;
				}
				dvr = &(mess.snap[Pos_UID]);
				dvr->input(r, r_len);
				if ( !ins->complex->si_set ) 
				{
					if ( !ins->complex->valid_result(&mess) )
					{
						mess.iRet = ERR_RESULT_INVALID;
						goto CardErrPro; 
					}
					//通过， 下一条指令
					mess.right_status = RT_INS_READY ;	//右端闲, 可以下一条指令
					break;
				}
				/* 这里不中断, 继续走子序列 */
				pro_rst_wt.step++;	//这里使得step=2
			case 2:
				switch ( comp_wt.step)
				{
				case 0:
					comp_wt.sub_which = 0;
					ins->complex->get_current(&mess, &(cur_def->person_vars));
					comp_wt.step++;	
					me_sub_zero();
				case 1:
					i_ret = sub_serial_pro(ins->complex->si_set, &comp_wt);
					if ( i_ret < 0 ) 
						goto CardErrPro; //两种SW定义都不符合

					if ( i_ret == 0 )  
						goto INS_OUT; //子序列还没有执行完成, 这里中断
//{int *a =0 ; *a = 0; };
					if ( !ins->complex->valid_result(&mess) )
					{
						mess.iRet = ERR_RESULT_INVALID;
						goto CardErrPro; 
					}
					//通过， 下一条指令
					mess.right_status = RT_INS_READY ;	//右端闲, 可以下一条指令
				default:
					break;
				}
			}
		break;

	case INS_Call:
			if ( !ins->fun.valid_condition(&mess) )
			{
				mess.right_status = RT_INS_READY ;	//右端闲, 可以下一条指令
				break;
			}
				
			i_ret = ins->fun.callfun(&mess);
			if ( i_ret > 0 )
			{
				if ( !ins->fun.valid_result(&mess) )
				{
					mess.iRet = ERR_RESULT_INVALID;
					goto CardErrPro; 
				}
				mess.right_status = RT_INS_READY ;	//右端闲, 可以下一条指令
			} else if ( i_ret < 0 ) {
				goto CardErrPro; //两种SW定义都不符合
			}
		break;

	case INS_Plain:
			if ( plain_wt.step ==0 && !ins->plain.valid_condition(&mess) )
			{
				mess.right_status = RT_INS_READY ;	//右端闲, 可以下一条指令
				break;
			}
				
			i_ret = ins_plain_pro(&ins->plain);
			if ( i_ret > 0 )
			{
				if ( !ins->plain.valid_result(&mess) )
				{
					mess.iRet = ERR_RESULT_INVALID;
					goto CardErrPro; 
				}
				mess.right_status = RT_INS_READY ;	//右端闲, 可以下一条指令
			} else if ( i_ret < 0 ) {
				goto CardErrPro; //两种SW定义都不符合
			}
		break;

	case INS_HSM:
			if ( hsmcom_wt.step ==0 && !ins->hsm.valid_condition(&mess) )
			{
				mess.right_status = RT_INS_READY ;	//右端闲, 可以下一条指令
				break;
			}
				
			i_ret = ins_hsm_pro(&ins->hsm);
			if ( i_ret > 0 )
			{
				if ( !ins->hsm.valid_result(&mess) )
				{
					mess.iRet = ERR_RESULT_INVALID;
					goto HsmErrPro; 
				}
				mess.right_status = RT_INS_READY ;	//右端闲, 可以下一条指令
			} else if ( i_ret < 0 ) {
				goto HsmErrPro; 
			}
		break;

	case INS_ExtAuth:
			switch ( auth_wt.step)
			{
			case 0:	//开始
				if ( !ins->auth[0].valid_condition(&mess) )
				{
					mess.right_status = RT_INS_READY ;	//右端闲, 可以下一条指令
					break;
				}
				auth_wt.cur = 0;
AUTH_AGAIN:
				auth_wt.sub_which = 0;
				auth_wt.step++;	
				me_sub_zero();
			case 1:	
				i_ret = sub_serial_pro(ins->auth[auth_wt.cur].si_set, &auth_wt);
				if ( i_ret < 0 && auth_wt.cur < (ins->auth_num-1)) 
				{	
					auth_wt.cur++;
					auth_wt.step--;
					goto AUTH_AGAIN;		//再一次认证开始
				}
				if ( i_ret < 0 ) 
					goto CardErrPro; //两种SW定义都不符合

				if ( i_ret ==0  )
					goto INS_OUT; //子序列还没有执行完成, 这里中断
				if ( !ins->auth[0].valid_result(&mess) )
				{
					mess.iRet = ERR_RESULT_INVALID;
					goto CardErrPro; 
				}
				//通过， 下一条指令
				mess.right_status = RT_INS_READY ;	//右端闲, 可以下一条指令
//{int *a =0 ; *a = 0; };
				break;
			default:
				break;
			}
		break;

	case INS_Charge:
	case INS_LoadInit:
	case INS_Debit:
	case INS_DesMac:
	case INS_Mac:
	case INS_GpKmc:
			switch ( comp_wt.step)
			{
			case 0:
				if ( !ins->complex->valid_condition(&mess) )
				{
					mess.right_status = RT_INS_READY ;	//右端闲, 可以下一条指令
					break;
				}

				comp_wt.sub_which = 0;
				ins->complex->get_current(&mess, &(cur_def->person_vars));
				comp_wt.step++;	
				me_sub_zero();
			case 1:
				i_ret = sub_serial_pro(ins->complex->si_set, &comp_wt);
				if ( i_ret < 0 ) 
					goto CardErrPro; //两种SW定义都不符合

				if ( i_ret == 0 ) 
					goto INS_OUT; //子序列还没有执行完成, 这里中断

				if ( !ins->complex->valid_result(&mess) )
				{
					mess.iRet = ERR_RESULT_INVALID;
					goto CardErrPro; 
				}
				//通过， 下一条指令
				mess.right_status = RT_INS_READY ;	//右端闲, 可以下一条指令
			default:
				break;
			}
		break;

	case INS_None:
		break;
	}

	if ( mess.iRet != 0 && mess.ins_which < (cur_def->ins_all.many-1) && mess.iRet != ERR_HSM_AGAIN ) //有错误发生
	{
		WBUG("bug!! mk_hand has error! but not goto error!");
	}

	if (mess.right_status == RT_INS_READY )	//新近一条指令做完
	{
		WLOG(CRIT, "has completed %d, order %d", mess.ins_which, mess.pro_order);
		mess.ins_which++;
		if ( mess.ins_which < cur_def->ins_all.many)
			goto INS_PRO;		//一条指令结束, 下一条
		else 
			mk_result();		//一张卡已经完成
	} else  {
		goto INS_OUT;
	}
HAND_END:
	return true;
INS_OUT:
	wbase = get_ic_base();
	if (wbase)
	{	//再一次判定这是IC卡操作
		hi_req.input(1, wbase->give, wbase->glen);	//把这个功能字设上啦。很重要!!!
		aptus->facio(&loc_pro_pac);     //向右发出指令, 然后等待, 这个放在别处????
	} else {	//HSM操作,
		tohsm_pri->aptus->facio(&loc_pro_pac);     //向右发出指令, 然后等待
	}
	/* aptus.facio的处理放在最后, 很重要!!。因为有可能在这个调用中就收到右节点的sponte. 注意!!. 以后的工作中一定要注意这点 */
	return true;

CardErrPro: //这张卡有错，弃之
HsmErrPro: //这张卡有错，弃之
	if ( mess.iRet != 0 ) //有错误发生
	{
		mess.ins_which++;
		if ( mess.ins_which >= cur_def->ins_all.many)		//最后一条（出卡）做完
		{
			mk_result();
		} else {				//不是做完最后一条
			mess.ins_which = cur_def->ins_all.many - 1;	//指向最后一条（出卡）
			mess.right_status = RT_INS_READY ;	//右端闲, 可以下一条指令
			goto INS_PRO;		//实际上做最后一条
		}
	} else {
		WBUG("bug!! CardErrPro HsmErrPro no error\n");	//ha,   BUG....
	}
	goto HAND_END;
}

void PacWay::mk_result()
{
	struct DyVar *dvr;
	int i;

	if ( mess.iRet != 0 ) 
	{
		switch ( mess.iRet )
		{
		case ERROR_HSM_TCP:
			snd_pac->input(gCFG->error_fld_no, "0A", 2);	//密码机通讯错, 整个工作应停止
			break;

		case ERROR_MK_TCP:
			snd_pac->input(gCFG->error_fld_no, "0B", 2);	//通讯错误, 不能再发了
			break;

		case ERROR_IC_INS:
			snd_pac->input(gCFG->error_fld_no, "0C", 2);	//错误，可发下一张卡
			break;	

		case ERROR_HSM_FUN:
			snd_pac->input(gCFG->error_fld_no, "0D", 2);	//错误，可发下一张卡
			break;	

		case ERROR_MK_DEV:
			snd_pac->input(gCFG->error_fld_no, "0E", 2);	//错误， 但是不关闭通讯, 不能再发了
			break;

		case ERROR_MK_PAUSE:
			snd_pac->input(gCFG->error_fld_no, "0F", 2);	//人工发卡暂停
			break;

		case ERROR_MK_RPC:
			snd_pac->input(gCFG->error_fld_no, "11", 2);	//错误， 不能再发了, 关闭通讯
			break;	

		case ERROR_HSM_RPC:
			snd_pac->input(gCFG->error_fld_no, "12", 2);	//错误， 不能再发了
			break;

		case ERROR_INS_DEF:	//找不到这个定义, 或者定义文件错误
			snd_pac->input(gCFG->error_fld_no, "13", 2);	//错误，可发下一张卡
			break;

		default:
			TEXTUS_SPRINTF(mess.err_str, "%s", "unkown error");
			snd_pac->input(gCFG->error_fld_no, "99", 2);
			break;
		}
		WLOG(WARNING, "The card of %s (uid= %s) personalized failed, iRet %d, %s", mess.snap[Pos_CardNo].val, mess.snap[Pos_UID].val, mess.iRet, mess.err_str);
		TEXTUS_SPRINTF(mess.err_str, "%s, %d, %s",  mess.bad_sw, mess.pro_order, cur_def == 0 ? " ": cur_def->flow_id);
			
		snd_pac->input(gCFG->errDesc_fld_no, mess.err_str, strlen(mess.err_str)); //这个域就是放错误数据
	} else {	//制卡成功
		snd_pac->input(gCFG->error_fld_no, "0", 1);
		for ( i = 0 ; i < mess.snap_num; i++)	//检查所有动态变量, 如果有赋值, 就设置到返回报文中
		{
			dvr = &mess.snap[i];
			if ( dvr->dest_fld_no >=0 && dvr->kind != VAR_None && dvr->c_len > 0 )
			{
				snd_pac->input(dvr->dest_fld_no, dvr->val, dvr->c_len);
			}
		}
	}
	aptus->sponte(&loc_pro_pac);    //制卡的结果回应给控制台
	mess.reset();
}

/* 向接力者提交 */
MINLINE void PacWay::deliver(TEXTUS_ORDO aordo)
{
	Amor::Pius tmp_pius;
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	
	switch (aordo)
	{
	case Notitia::SET_UNIPAC:
		WBUG("deliver SET_UNIPAC req(%p) ans(%p)", hipa[0], hipa[1]);
		tmp_pius.indic = &hipa[0];
		aptus->facio(&tmp_pius);
		break;

	case Notitia::WHO_AM_I:
		WBUG("deliver(sponte) WHO_AM_I");
		tmp_pius.indic = (void*) this;
		aptus->sponte(&tmp_pius);
		break;
	}
	return ;
}

#include "hook.c"
