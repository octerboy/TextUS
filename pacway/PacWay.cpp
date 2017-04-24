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
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include "BTool.h"
#if defined(__APPLE__)
#define COMMON_DIGEST_FOR_OPENSSL
#include <CommonCrypto/CommonDigest.h>
#else
#include <openssl/md5.h>
#endif

int squeeze(const char *p, unsigned char *q)	//把空格等挤掉, 只留下16进制字符(大写), 返回实际的长度
{
	int i;
	i = 0;
	while ( *p )
	{ 
		if ( isxdigit(*p) ) 
		{
			if (q) q[i] = toupper(*p);
			i++;
		} else if ( !isspace(*p)) 
		{
			if (q) q[i] = *p;
			i++;
		}
		p++;
	}
	if (q) q[i] = '\0';
	return i;
};

#define ERROR_DEVICE_DOWN -100
#define ERROR_COMPLEX_TOO_DEEP -201
#define ERROR_RECV_PAC -202
#define ERROR_RESULT -210
#define ERROR_USER_ABORT -203
#define ERROR_INS_DEF -133

char err_global_str[128]={0};
/* 左边状态, 空闲, 等着新请求, 交易进行中 */
enum LEFT_STATUS { LT_Idle = 0, LT_Working = 3};
/* 右边状态, 空闲, 发出报文等响应 */
enum RIGHT_STATUS { RT_IDLE = 0, RT_OUT=7, RT_READY = 3};

enum Var_Type {VAR_ErrCode=1, VAR_FlowPrint=2, VAR_TotalIns = 3, VAR_CurOrder=4, VAR_CurCent=5, VAR_ErrStr=6, VAR_FlowID=7, VAR_Dynamic = 10, VAR_Me=12, VAR_Constant=98,  VAR_None=99};
/* 命令分几种，INS_Normal：标准，INS_Abort：终止 */
enum PacIns_Type { INS_None = 0, INS_Normal=1, INS_Abort=2, INS_Null};

/* 包括SysTime这样的变量，都由外部函数计算，所以这里只保留脚本指纹数据 */	
#define Pos_ErrCode 1 
#define Pos_FlowPrint 2 
#define Pos_TotalIns 3 
#define Pos_CurOrder 4 
#define Pos_CurCent 5 
#define Pos_ErrStr 6 
#define Pos_FlowID 7 
#define Pos_Fixed_Next 8  //下一个动态变量的位置, 也是为脚本自定义动态变量的第1个位置
#define VARIABLE_TAG_NAME "Var"
#define ME_VARIABLE_HEAD "me."

	struct PVar
	{
		Var_Type kind;
		const char *name;	//来自于doc文档
		int n_len;		//名称长度

		unsigned char content[512];	//变量内容. 这个内容与下面的内容一般不会同时有
		int c_len;			//内容长度
		int dynamic_pos;	//动态变量位置, -1表示静态

		int source_fld_no;	//来自请求报文的哪个域号。		
		int start_pos;		//从请求报文中, 什么位置开始
		int get_length;		//取多少长度的值
		
		int dest_fld_no;	//响应报文的目的域号, 

		char me_name[64];	//Me变量名称，除去开头的 me. 三个字节, 不包括后缀. 从变量名name中复制，最大63字符
		unsigned int me_nm_len;
		const char *me_sub_name;  //Me变量后缀名， 从变量名name中定位。
		int me_sub_nm_len;

		bool dy_link;	//动态变量的赋值方式，true:取地址方式，不复制; false:复制方式
		TiXmlElement *self_ele;	/* 自身, 其子元素包括两种可能: 1.函数变量表, 
					2.一个指令序列, 在指令子元素分析时, 如发现一个用到的变量中, 有子序列时, 把这些指令嵌入。
					*/
		PVar () {
			kind = VAR_None;	//起初认为非变量
			name = 0;
			n_len = 0;

			c_len = 0;
			memset(content,0,sizeof(content));
			dynamic_pos = -1;	//非动态类

			source_fld_no = -1;
			start_pos = 1;
			get_length = -1;
			
			dest_fld_no = -1;
		
			self_ele = 0;

			memset(me_name, 0, sizeof(me_name));
			me_nm_len = 0;
			me_sub_name = 0;
			me_sub_nm_len = 0;

			dy_link = false;	//动态变量的赋值方式为复制方式。
		};

		void put_still(const unsigned char *val, unsigned int len=0)
		{
			if ( !val)
				return ;
			if ( len ==0 ) 
				c_len = strlen((char*)val);
			else
				c_len = len;
				
			if ( (c_len+1) > (int)sizeof(content) ) c_len =  sizeof(content)-1;
			memcpy(content, val, c_len);
			content[c_len] = 0;
			kind = VAR_Constant;	//认为是常数
		};

		void put_var(struct PVar* att_var)
		{
			kind = att_var->kind;
			c_len = att_var->c_len;
			if ( c_len > 0 ) 
				memcpy(content, att_var->content, c_len );
			att_var->content[c_len] = 0;	
			dynamic_pos = att_var->dynamic_pos;
		};

		struct PVar* prepare(TiXmlElement *var_ele, int &dy_at) //变量准备
		{
			const char *p, *nm;
			kind = VAR_None;
			self_ele = var_ele;

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

			if ( strcasecmp(nm, "$ink" ) == 0 ) //当前用户命令集指纹
			{
				dynamic_pos = Pos_FlowPrint;
				kind = VAR_FlowPrint;
			}
			if ( strcasecmp(nm, "$total" ) == 0 ) //总用户命令数
			{
				dynamic_pos = Pos_TotalIns;
				kind = VAR_TotalIns;
			}

			if ( strcasecmp(nm, "$ErrCode" ) == 0 ) //错误代码
			{
				dynamic_pos = Pos_ErrCode;
				kind = VAR_ErrCode;
			}

			if ( strcasecmp(nm, "$CurOrder" ) == 0 ) //当前用户命令编号
			{
				dynamic_pos = Pos_CurOrder;
				kind = VAR_CurOrder;
			}

			if ( strcasecmp(nm, "$CurCent" ) == 0 ) //当前工作百分比
			{
				dynamic_pos = Pos_CurCent;
				kind = VAR_CurCent;
			}

			if ( strcasecmp(nm, "$ErrStr" ) == 0 ) //错误描述
			{
				dynamic_pos = Pos_ErrStr;
				kind = VAR_ErrStr;
			}

			if ( strcasecmp(nm, "$FlowID" ) == 0 ) //错误描述
			{
				dynamic_pos = Pos_FlowID;
				kind = VAR_FlowID;
			}

			if ( var_ele->Attribute("link") )
			{
				dy_link = true;
			}

			if ( kind != VAR_None) goto P_RET; //已有定义，不再看这个Dynamic, 以上定义都与Dynamic相同处理

			if ( (p = var_ele->Attribute("dynamic")) && (*p == 'Y' || *p == 'y') )
			{
				dynamic_pos = dy_at;	//动态变量位置
				kind = VAR_Dynamic;
				dy_at++;
			}

			if ( kind != VAR_None) goto P_RET; //已有定义，

			/* 以下对Me变量进行处理， 在子序列中, 还是先要定义一下me.这些变量。 要不，还真是麻烦 */
			if ( strncasecmp(nm, ME_VARIABLE_HEAD, sizeof(ME_VARIABLE_HEAD)-1) == 0 ) 	//减1，啊, 最后有一个null字符
			{
				kind = VAR_Me;
				me_sub_name = strpbrk(&nm[sizeof(ME_VARIABLE_HEAD)-1], ".");	//从Me变量名后找第一个点，后面就作为后缀名.
				if ( me_sub_name )	//如果存在后缀
				{
					me_nm_len = me_sub_name - &nm[sizeof(ME_VARIABLE_HEAD)-1];
					me_sub_name++;	//当然，这个点本身不是后缀名, 从后一个开始才是后缀名
					me_sub_nm_len = strlen(me_sub_name);
				} else {			//如果不存在后缀
					me_nm_len = strlen(&nm[sizeof(ME_VARIABLE_HEAD)-1]);
				}

				if ( me_nm_len >= sizeof ( me_name))	//Me变量名空间有限, 64字节最大。
					me_nm_len = sizeof ( me_name)-1;
				memcpy(me_name, &nm[sizeof(ME_VARIABLE_HEAD)-1], me_nm_len);
				me_name[me_nm_len] = 0 ;
			}

			if ( kind == VAR_None && c_len > 0) 
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

	struct DyVar	/* 动态变量， 包括来自报文的 */
	{
		Var_Type kind;	//动态类型,
		int index;	//索引, 也就是下标值
		unsigned char *val_p;	//p有时指向这里的val， 但也可能指向输入、输出的域.c_len就是长度。
		char val[512];	//变量内容. 随时更新, 足够空间啦
		unsigned long c_len;

		struct PVar *def_var;	//文件中定义的变量

		DyVar () {
			kind = VAR_None;
			index = - 1;
			c_len = 0;
			val_p = 0;

			memset(val, 0, sizeof(val));
		};

		void input(unsigned char *p, unsigned long len, bool link=false)
		{
			if ( def_var->dy_link || link)
			{
				val_p = p;
				c_len = len;
			} else {
				val_p = (unsigned char*)&val[0];
				if ( (len+1) < sizeof(val) )
				{
					memcpy(val, p, len);
					c_len = len;
					val[len] = 0;
				}
			}
		};

		void input(int iv)
		{
			TEXTUS_SPRINTF(val, "%d", iv);
			c_len = strlen((char*)&val[0]);
			val[c_len] = 0;
			val_p = (unsigned char*)&val[0];
		};

		void input(const char *p, bool link=false)
		{
			c_len = strlen(p);
			if ( def_var->dy_link || link)
			{
				val_p = (unsigned char*)p;
			} else {
				val[c_len] = 0;
				val_p = (unsigned char*)&val[0];
				memcpy(val, p, c_len);
			}
		};

		void input(const char p)
		{
			c_len = 1;
			val[1] = 0;
			val_p = (unsigned char*)&val[0];
			val[0] = p;
		};
	};

	struct MK_Session {		//记录一个事务过程中的各种临时数据
		struct DyVar *snap;	//随时的变量, 包括
		int snap_num;
		bool willLast;		//最后一次试错. 通常，一个用户指令只尝试一次错, 此值为true。 有时有多次尝试, 此值先为false，最后一次为true.

		char err_str[1024];	//错误信息
		char flow_id[64];
		int pro_order;		//当前处理的操作序号

		LEFT_STATUS left_status;
		RIGHT_STATUS right_status;
		int ins_which;	//已经工作在哪个命令, 即为定义中数组的下标值
		int iRet;	//事务最终结果

		inline MK_Session ()
		{
			snap=0;
		};

		inline void  reset() 
		{
			int i;
			for ( i = 0; i < snap_num; i++)
			{
				snap[i].c_len = 0;
				snap[i].val[0] = 0;
			}
			for ( i = Pos_Fixed_Next ; i < snap_num; i++)
			{	/* 这个Pos_Fixed_Next很重要, 要不然, 那些固有的动态变量会没有的！  */
				snap[i].kind = VAR_None;
				snap[i].def_var = 0;
			}
			left_status = LT_Idle;
			right_status = RT_IDLE;
			ins_which = -1;
			err_str[0] = 0;	
			flow_id[0] = 0;
			willLast = true;
		};

		inline void init(int m_snap_num) //这个m_snap_num来自各XML定义的最大动态变量数
		{
			if ( snap )	return ;
			if ( m_snap_num <=0 ) return ;

			snap_num = m_snap_num;
			snap = new struct DyVar[snap_num];
			for ( int i = 0 ; i < snap_num; i++)
				snap[i].index = i;

			snap[Pos_ErrCode].kind = VAR_ErrCode;
			snap[Pos_FlowPrint].kind = VAR_FlowPrint;
			snap[Pos_TotalIns].kind = VAR_TotalIns;
			snap[Pos_CurOrder].kind = VAR_CurOrder;
			snap[Pos_CurCent].kind = VAR_CurCent;
			snap[Pos_ErrStr].kind = VAR_ErrStr; 
			snap[Pos_FlowID].kind = VAR_FlowID; 
			reset();
		};

		~MK_Session ()
		{
			if ( snap ) delete[] snap;
			snap = 0;
		};
	};

/* 变量集合*/
struct PVar_Set {	
	struct PVar *vars;
	int many;
	int dynamic_at;
	PVar_Set () 
	{
		vars = 0;
		many = 0;
		dynamic_at = Pos_Fixed_Next; //0,等 已经给$FlowPrint等占了
	};

	~PVar_Set () 
	{
		if (vars ) delete []vars;
		vars = 0;
		many = 0;
	};

	bool is_var(const char *nm)
	{
		if (nm  && strcasecmp(nm, VARIABLE_TAG_NAME) == 0 ) return true;
		return false;
	}

	void defer_vars(TiXmlElement *map_root, TiXmlElement *icc_root=0) //分析一下变量定义
	{
		TiXmlElement *var_ele, *i_ele;
		const char *vn = VARIABLE_TAG_NAME, *nm;
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

	void put_still(const char *nm, const unsigned char *val, unsigned int len=0)
	{
		struct PVar *av = look(nm,0);
		if ( av) av->put_still(val, len);
	};

	void put_var(const char *nm, struct PVar *att_var)
	{
		struct PVar *av = look(nm,0);
		if ( av) av->put_var(att_var);
	};

	/* 找静态的变量, 获得实际内容 */
	struct PVar *one_still( const char *nm, unsigned char *buf, unsigned long &len, struct PVar_Set *loc_v=0)
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
			if ( buf && len > 0 ) memcpy(buf, vt->content, len);
			break;

		default:
			len = 0;
			break;
		}
		VARET:
		if ( buf ) buf[len] = 0;	//结束NULL
		return vt;
	};

	/* nxt 下一个变量, 对于多个tag元素，将之静态内容合成到 一个变量command中。对于非静态的，返回该tag元素是个动态变量 */
	struct PVar *all_still( TiXmlElement *ele, const char*tag, unsigned char *command, unsigned long &ac_len, TiXmlElement *&nxt, struct PVar_Set *loc_v=0)
	{
		TiXmlElement *comp;
		unsigned long l;
		struct PVar  *rt;
		bool will_break= false;
				
		rt = 0;
		/* ac_len从参数传进, 累计的, command就是原来的好了, 不用重设指针 */
		for ( comp = ele; comp && !will_break ; comp = comp->NextSiblingElement(tag) )
        	{
			if ( !comp->GetText() ) continue; //没有内容, 略过
			//printf("tag %s  Text %s\n",tag, comp->GetText());
			if ( command ) 
				rt = one_still( comp->GetText(), &command[ac_len], l, loc_v);
			else 
				rt = one_still( comp->GetText(), 0, l, loc_v);
			ac_len += l;
			if ( rt && rt->kind < VAR_Constant )		//如果有非静态的, 这里需要中断, comp指向下一个
				will_break = true;
		}
		//printf("+++++++++++++++ %s \n",tag);
		if (command) command[ac_len] = 0;	//结束NULL
		nxt = comp;	//指示下一个变量
		return rt;
	};
};

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
		const unsigned char *src_con, *dst_con;
		int src_len, dst_len;

		if ( src->dynamic_pos >= 0 )
		{
			dvr = &sess->snap[src->dynamic_pos];
			src_con = dvr->val_p;
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
				dst_con = dvr->val_p;
				dst_len = dvr->c_len;
			} else {
				dst_con = &dst->content[0];
				dst_len = dst->c_len;
			}
		} else {
			dst_con = (unsigned char*)con_dst;
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
		bool ret=false;	//假定没有一个是符合的
		for(i = 0; i < dst_num; i++)
		{
			ret = dst_arr[i].valid_val(sess, src);
			if ( ret ) break;	//有多个值，有一个相同，就不再比较了。
		}
		if ( !ys_no ) ret = !ret; 	//如果没有一个相同, 再取反, 这就返回ok。也就是not所有的值。
		return ret;
	};
};

struct Condition {	//一个指令的匹配列表, 包括条件与结果的匹配
	bool hasLastWill;	/* 通常此值为false,即执行某指令不需要判断Mess中的willLast。 当有子系列被尝试多次, 就需要判断willLast为true时才执行.
				此值供外部脚本使用，指示是否可以调用错误处理过程（向终端发出卡指令等)
				*/
	int con_num;
	int res_num;
	struct Match *conie_list;
	struct Match *result_list;

	Condition () { 
		con_num = 0; 
		conie_list= 0;
		res_num = 0; 
		result_list= 0;
		hasLastWill = false;
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
		const char *c;
		hasLastWill = false;
		c = ele->Attribute("has_last_will");
		if ( c && ( c[0] == 'Y' || c[0] == 'y' ) ) 
			hasLastWill = true;
			
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
		return ret;	//所有分项都符合，才算这个条件符合
	};

	bool valid_condition (MK_Session *sess)
	{
		if (hasLastWill && !sess->willLast ) return false;
		return valid_list(sess, conie_list, con_num);
	};
	bool valid_result (MK_Session *sess)
	{
		return valid_list(sess, result_list, res_num);
	};
};

struct DyList {
	unsigned char *con;  /* 指向 cmd_buf中的某个点 */
	unsigned long len;
	int dy_pos;
	DyList ()
	{
		con  =0;
		len = 0;
		dy_pos = -1;
	};
};

struct CmdSnd {
	int fld_no;	//发送的域号
	int dy_num;
	struct DyList *dy_list;
	unsigned char *cmd_buf;
	unsigned long cmd_len;
	const char *tag;	/*  比如"component"这样的内容，指明子序列中的元素 */

	CmdSnd () {
		cmd_buf = 0;
		cmd_len = 0;
		fld_no = -1;
		dy_num = 0;
		dy_list = 0;
	};


	void hard_work_2 ( TiXmlElement *pac_ele, TiXmlElement *usr_ele, struct PVar_Set *g_vars, struct PVar_Set *me_vars)
	{
		struct PVar *vr_tmp, *vr2_tmp=0;
		TiXmlElement *e_tmp, *n_ele;
		TiXmlElement *e2_tmp, *n2_ele;
		unsigned long g_ln;
		unsigned char *cp;
		int dy_cur;
		bool now_still;


		cmd_len = 0;
		cmd_buf = 0;
		dy_num = 0;

		e_tmp = pac_ele->FirstChildElement(tag); 
		g_ln = 0 ;
		while ( e_tmp ) 
		{
			g_ln = 0 ;
			vr_tmp = g_vars->all_still( e_tmp, tag, 0, g_ln, n_ele, me_vars);
			e_tmp = n_ele;
			if ( g_ln > 0 ) cmd_len += g_ln;
			if ( !vr_tmp ) 		//还是常数, 这里应该结束了
			{
				if (e_tmp) printf("plain !!!!!!!!!!\n");	//这不应该
				continue;
			}

			if ( vr_tmp->kind <= VAR_Dynamic )	//Me变量的, 不算作动态
			{
				dy_num++;
				continue;
			}

			if ( vr_tmp->kind == VAR_Me && vr_tmp->c_len > 0)	//Me变量已有内容,无后缀的，可能已有定义的，或者带后缀的。
			{
				cmd_len += vr_tmp->c_len;
				continue;
			}

			if ( vr_tmp->kind == VAR_Me && vr_tmp->me_sub_nm_len == 0)	//Me变量,且无后缀名, 则从用户命令中取，当然还没有内容
			{
				vr2_tmp = 0;
				g_ln = 0 ;
				if ( usr_ele->Attribute(vr_tmp->me_name) ) 	//用户命令中，属性优先
				{
					vr2_tmp = g_vars->one_still( usr_ele->Attribute(vr_tmp->me_name), 0, g_ln);
					if ( g_ln > 0 ) cmd_len += g_ln;
					if ( vr2_tmp && vr2_tmp->kind <= VAR_Dynamic )
					{
						dy_num++;
					}
					continue;	//有了属性，不管如何，不再看子元素了
				}

				e2_tmp = usr_ele->FirstChildElement(vr_tmp->me_name);
				while (e2_tmp)
				{
					g_ln = 0 ;
					vr2_tmp = g_vars->all_still(e2_tmp, vr_tmp->me_name, 0, g_ln, n2_ele, 0);
					e2_tmp = n2_ele;
					if ( g_ln > 0 ) cmd_len += g_ln;
					if ( !vr2_tmp ) 		//还是常数, 这里应该结束了
					{
						if (e2_tmp) printf("plain !!!!!!!!!!\n");	//这不应该
						continue;
					}

					if ( vr2_tmp->kind <= VAR_Dynamic )	//参考变量的, 不算作动态
					{
						dy_num++;
					}
				}
			}
		}

		cmd_buf = new unsigned char[cmd_len+1];	//对于非动态的, cmd_buf与cmd_len刚好是其全部的内容
		dy_num = dy_num *2+1;	/* dy_num表示多少个动态变量, 实际分段数最多是其2倍再多1 */
		dy_list = new struct DyList [dy_num];

	#define DY_NEXT_STILL	\
		dy_cur++;	\
		dy_list[dy_cur].con = cp;	\
		dy_list[dy_cur].len = 0;	\
		dy_list[dy_cur].dy_pos = -1;

	#define DY_DYNAMIC(X)	\
		now_still = false;		\
		if ( dy_list[dy_cur].len > 0 )	\
			dy_cur++;		\
		dy_list[dy_cur].con = 0;	\
		dy_list[dy_cur].len = 0;	\
		dy_list[dy_cur].dy_pos = X;

	#define DY_STILL(LEN)		\
		cp = &cp[LEN];		\
		now_still = true;	\
		dy_list[dy_cur].dy_pos = -1;	\
		dy_list[dy_cur].len += LEN;

		memset(&cmd_buf[0], 0, cmd_len+1);
		cp = &cmd_buf[0]; 
		dy_cur = -1;	//下面的宏先加1，所以开始为0

		DY_NEXT_STILL
		now_still = true;

		e_tmp = pac_ele->FirstChildElement(tag);
		while ( e_tmp ) 
		{
ALL_STILL:
			g_ln = 0;
			vr_tmp= g_vars->all_still( e_tmp, tag, cp, g_ln, n_ele, me_vars);
			e_tmp = n_ele;

			if ( g_ln > 0 )	/* 刚处理的是静态内容 */
			{
				DY_STILL(g_ln) //cp指针后移, 内容增加， 这里游标不变，因为下一个可能是Me变量的静态, 这要合并在一起
			}

			if ( !vr_tmp ) 		//还是常数, 这里应该结束了
			{
				if (e_tmp) printf("plain !!!!!!!!!!\n");	//这不应该
				continue;
			}

			if ( vr_tmp->kind <= VAR_Dynamic )	//参考变量的, 不算作动态。
			{
				DY_DYNAMIC(vr_tmp->dynamic_pos)	//已有静态内容的， 先指向下一个, 以存放动态的
				DY_NEXT_STILL
				continue;
			}

			if ( vr_tmp->kind == VAR_Me && vr_tmp->c_len > 0)	//Me变量已有内容,1、无后缀的，本身已有定义的; 2、或者带后缀的，前面的处理中已经设定了内容
			{
				memcpy(cp, vr_tmp->content, vr_tmp->c_len);
				DY_STILL(vr_tmp->c_len) //cp指针后移,内容增加， 这里游标不变，因为下一个可能是Me变量的静态, 这要合并在一起
				goto ALL_STILL;	//这里处理的是静态，所以从该处继续
			}

			if ( vr_tmp->kind == VAR_Me && vr_tmp->me_sub_nm_len == 0)	//Me变量,且无后缀名, 则从用户命令中取，当然还没有内容
			{
				vr2_tmp = 0;
				g_ln = 0;
				if (usr_ele->Attribute(vr_tmp->me_name)) //先看属性内容
				{
					vr2_tmp = g_vars->one_still( usr_ele->Attribute(vr_tmp->me_name), cp, g_ln);
					if ( g_ln > 0 )	/* 刚处理的是静态内容 */
					{
						DY_STILL(g_ln) //cp指针后移, 内容增加， 这里游标不变，因为下一个可能是Me变量的静态, 这要合并在一起
					}

					if ( vr2_tmp && vr2_tmp->kind <= VAR_Dynamic )
					{
						DY_DYNAMIC(vr2_tmp->dynamic_pos)	//已有静态内容的， 先指向下一个, 以存放动态的
						DY_NEXT_STILL
					}
					continue;	//有了属性，不管如何，不再看子元素了
				}
			
				e2_tmp = usr_ele->FirstChildElement(vr_tmp->me_name);
				while (e2_tmp)
				{
					g_ln = 0;
					vr2_tmp = g_vars->all_still(e2_tmp, vr_tmp->me_name, cp, g_ln, n2_ele, 0);
					e2_tmp = n2_ele;
					if ( g_ln > 0 )	/* 刚处理的是静态内容 */
					{
						DY_STILL(g_ln) //cp指针后移, 内容增加， 这里游标不变，因为下一个可能是Me变量的静态, 这要合并在一起
					}

					if ( !vr2_tmp ) 		//还是常数, 这里应该结束了
					{
						if (e2_tmp) printf("plain !!!!!!!!!!\n");	//这不应该
						continue;
					}

					if ( vr2_tmp->kind <= VAR_Dynamic )	//Me变量的, 不算作动态
					{
						DY_DYNAMIC(vr2_tmp->dynamic_pos)	//已有静态内容的， 先指向下一个, 以存放动态的
						DY_NEXT_STILL
						now_still = false;
					}
				}
			}
		}
		if ( now_still ) 
			dy_num = dy_cur + 1;	//最后一个是静态, dy_cur并未增加
		else
			dy_num = dy_cur;
	};
};

struct CmdRcv {
	int fld_no;	//接收的域号, 有多个接收定义，每个只定义一个变量.所以，有多个定义，指向同一个域。
	int dyna_pos;	//动态变量位置, -1表示静态
	unsigned int start;
	int length;
	unsigned char *must_con;
	unsigned long must_len;
	const char *err_code; //这是直接从外部定义文件得到的内容，不作任何处理。当本域不符合要求，设置此错误码。

	const char *tag;//比如： reply, sw
	CmdRcv() {
		dyna_pos = -1;
		start =1;
		length = -1;
		must_con = 0;
		must_len = 0;
		err_code = 0;
	};
};

struct ComplexSubSerial;
struct PacIns:public Condition  {
	PacIns_Type type;
	int subor;	//指示指令报文送给哪一个下级模块

	struct CmdSnd *snd_lst;
	int snd_num;

	struct CmdRcv *rcv_lst;
	int rcv_num;

	const char *err_code; //这是直接从外部定义文件得到的内容，不作任何处理。 当本报文出现通信错误，包括报文不能解析的、通道关闭。
	bool counted;		//是否计数
	bool isFunction;	//是否为函数

	PacIns() 
	{
		subor = 0;
		snd_lst = 0;
		snd_num = 0;
		rcv_lst = 0;
		rcv_num = 0;

		err_code = 0;
		counted = false;	/* 不计入指令数计算 */
		isFunction = false;
		type = INS_None;
	};

	void get_snd_pac( PacketObj *snd_pac, int &bor, MK_Session *sess)
	{
		int i,j;
		unsigned long t_len;

		bor = subor;
		t_len = 0;
		for ( i = 0 ; i < snd_num; i++ )
		{
			if ( snd_lst[i].dy_num ==0 ) t_len += snd_lst[i].cmd_len;	//这是在pacdef中send元素定义的
			else {
				for ( j = 0; j < snd_lst[i].dy_num; j++ )
					t_len += snd_lst[i].dy_list[j].len;
			}
		}
		snd_pac->grant(t_len);
		for ( i = 0 ; i < snd_num; i++ )
		{
			t_len = 0;
			if ( snd_lst[i].dy_num ==0 )
			{	/* 这是在pacdef中send元素定义的一个域的内容 */
				memcpy(snd_pac->buf.point, snd_lst[i].cmd_buf, snd_lst[i].cmd_len);
				t_len = snd_lst[i].cmd_len;	
			} else {
				for ( j = 0; j < snd_lst[i].dy_num; j++ )
				{
					/* 由一个列表来构成一个域的内容 */
					if (  snd_lst[i].dy_list[j].dy_pos < 0 ) 
					{
						/* 静态内容*/
						memcpy(&snd_pac->buf.point[t_len], snd_lst[i].dy_list[j].con, snd_lst[i].dy_list[j].len);
						t_len += snd_lst[i].dy_list[j].len;
					} else {
						/* 动态内容*/
						memcpy(&snd_pac->buf.point[t_len], sess->snap[snd_lst[i].dy_list[j].dy_pos].val_p, sess->snap[snd_lst[i].dy_list[j].dy_pos].c_len);
						t_len += sess->snap[snd_lst[i].dy_list[j].dy_pos].c_len;
					}
				}
			}
			snd_pac->commit(snd_lst[i].fld_no, t_len);	//域的确认
		}
		return ;
	};

	/* 本指令处理响应报文，匹配必须的内容,出错时置出错代码变量 */
	bool pro_rcv_pac(PacketObj *rcv_pac,  struct MK_Session *mess)
	{
		int ii;
		unsigned char *fc;
		unsigned long rlen;
		struct CmdRcv *rply;

		for (ii = 0; ii < rcv_num; ii++)
		{
			rply = &rcv_lst[ii];
			fc = rcv_pac->getfld(rply->fld_no, &rlen);
			if ( !fc ) goto ErrRet;
			if (rply->must_con ) 
			{
				if ( !(rply->must_len == rlen && memcmp(rply->must_con, fc, rlen) == 0 ) ) goto ErrRet;
			}

			if ( rply->dyna_pos > 0)
			{
				if ( rlen >= (rply->start ) )	
				{
					rlen -= (rply->start-1); //start是从1开始
					if ( rply->length > 0 && (unsigned int)rply->length < rlen)
						rlen = rply->length;
					mess->snap[rply->dyna_pos].input(&fc[rply->start-1], rlen);
				}
			}
		}

		return true;
ErrRet:
		if ( rply->err_code )
			mess->snap[Pos_ErrCode].input(rply->err_code);
		else
			mess->snap[Pos_ErrCode].input(err_code);	//可能对于域不符合的情况，未定义错误码，就取基础报文或map中的定义
		return false;
	};

	int hard_work ( TiXmlElement *def_ele, TiXmlElement *pac_ele, TiXmlElement *usr_ele, struct PVar_Set *g_vars, struct PVar_Set *me_vars)
	{
		struct PVar *vr_tmp=0;
		TiXmlElement *e_tmp, *p_ele, *some_ele;
		const char *p=0;

		int i = 0;
		int lnn;
		const char *tag;

		if ( strcasecmp( pac_ele->Value(), "abort") ==0 )
		{
			type = INS_Abort;
			goto LAST_CON;
		}

		if ( strcasecmp( pac_ele->Value(), "null") ==0 )
		{
			type = INS_Null;
			goto LAST_CON;
		}

		subor=0; def_ele->QueryIntAttribute("subor", &subor);
		if ( (p = def_ele->Attribute("counted")) && ( *p == 'y' || *p == 'Y') )
			counted = true;
		else 
			counted = false;

		if ((p = def_ele->Attribute("function")) && ( *p == 'y' || *p == 'Y') )	//这是函数扩展，出现回调
			isFunction = true;
		else
			isFunction = false;

		/* 先预置发送的每个域，设定域号*/
		snd_num = 0;
		for (p_ele= def_ele->FirstChildElement(); p_ele; p_ele = p_ele->NextSiblingElement())
		{
			p = p_ele->Value();
			if ( !p ) continue;
			if ( strcasecmp(p, "send") == 0 || p_ele->Attribute("to")) 
				snd_num++;
		}
		snd_lst = new struct CmdSnd[snd_num];
		for (p_ele= def_ele->FirstChildElement(),i = 0; p_ele; p_ele = p_ele->NextSiblingElement())
		{
			p = p_ele->Value();
			if ( !p ) continue;
			if ( strcasecmp(p, "send") == 0 || p_ele->Attribute("to")) 
			{
				tag = p_ele->Value();
				snd_lst[i].tag =tag;
				if ( strcasecmp(tag, "send") == 0 ) 
				{
					p_ele->QueryIntAttribute("field", &(snd_lst[i].fld_no));
					p = p_ele->GetText();
					if ( p )
					{
						lnn = strlen(p);
						snd_lst[i].cmd_buf = new unsigned char[lnn+1];
						snd_lst[i].cmd_len = BTool::unescape(p, snd_lst[i].cmd_buf) ;
					}
					i++;
				} else if ( pac_ele->FirstChildElement(tag)) {	// pacdef中有定义域, 但mapx不给内容, 这就不要了
					p_ele->QueryIntAttribute("to", &(snd_lst[i].fld_no));
					snd_lst[i].hard_work_2(pac_ele, usr_ele, g_vars, me_vars);
					i++;
				}
			}
		}
		snd_num = i;	//最后再更新一次发送域的数目

		/* 预置接收的每个域，设定域号*/
		rcv_num = 0;
		for (p_ele= def_ele->FirstChildElement(); p_ele; p_ele = p_ele->NextSiblingElement())
		{
			tag = p_ele->Value();
			if ( !tag ) continue;
			if ( strcasecmp(tag, "recv") == 0 || p_ele->Attribute("from"))
			{
				rcv_num++;
				if ( strcasecmp(tag, "recv") == 0 ) continue; /*recv 仅在基础报文定义中出现.在基础报文中，from项也算一项，如同recv */
			} else continue;
			/*子序列中的返回元素也算上 */
			for (e_tmp = pac_ele->FirstChildElement(tag); e_tmp; e_tmp = e_tmp->NextSiblingElement(tag) ) 
				rcv_num++;
			/* 用户命令的返回元素也算上 */
			for (e_tmp = usr_ele->FirstChildElement(tag); e_tmp; e_tmp = e_tmp->NextSiblingElement(tag) ) 
				rcv_num++;				
		}
		rcv_lst = new struct CmdRcv[rcv_num];
		for (p_ele= def_ele->FirstChildElement(),i = 0; p_ele; p_ele = p_ele->NextSiblingElement())
		{
			tag = p_ele->Value();
			if ( !tag ) continue;
			if ( strcasecmp(tag, "recv") == 0 || p_ele->Attribute("from"))
			{
				rcv_lst[i].tag = tag;
				p_ele->QueryIntAttribute("field", &(rcv_lst[i].fld_no));
				p_ele->QueryIntAttribute("from", &(rcv_lst[i].fld_no));	/* 如果recv中有from ....?? */
				p = p_ele->GetText();
				if ( p )
				{
					lnn = strlen(p);
					rcv_lst[i].must_con = new unsigned char[lnn+1];
					rcv_lst[i].must_len = BTool::unescape(p, rcv_lst[i].must_con) ;
					rcv_lst[i].err_code = p_ele->Attribute("error");	//接收域若有不符合，设此错误码
				}
				i++;
				if ( strcasecmp(tag, "recv") == 0 )
					continue;	/* recv 仅在基础报文定义中出现, 对于from的，下面还要处理 */
			} else continue;
			/* 用户命令和子序列中的返回元素也算上, 如果两者都没有，则这里不需要分配了 */
			some_ele = pac_ele;
ANOTHER:
			for (e_tmp = some_ele->FirstChildElement(tag); e_tmp; e_tmp = e_tmp->NextSiblingElement(tag) ) 
			{
				rcv_lst[i].tag = tag;
				p_ele->QueryIntAttribute("from", &(rcv_lst[i].fld_no));	//对第一个有点重复
				if ( (p = e_tmp->Attribute("name")) )
				{
					vr_tmp = g_vars->look(p, me_vars);	//响应变量, 动态变量, 两个变量集
					if (vr_tmp) 
					{
						rcv_lst[i].dyna_pos = vr_tmp->dynamic_pos;
						lnn = 0;
						e_tmp->QueryIntAttribute("start", &(lnn));
						if ( lnn >= 1) 
							rcv_lst[i].start = (unsigned int)lnn;
						e_tmp->QueryIntAttribute("length", &(rcv_lst[i].length));
					}
				}
				i++;
			}
			if ( some_ele != usr_ele ) 
			{
				some_ele = usr_ele;
				goto ANOTHER;
			}
		}	/* 结束返回元素的定义*/

		type = INS_Normal;
LAST_CON:
		return counted ? 1:0 ;
	};

	void prepare(TiXmlElement *def_ele, TiXmlElement *pac_ele, TiXmlElement *usr_ele, struct PVar_Set *g_vars, struct PVar_Set *me_vars)
	{
		struct PVar *err_var;
		
		err_code = pac_ele->Attribute("error"); //每一个报文定义一个错误码，对于INS_Abort很有用。
		if (!err_code && def_ele ) err_code = def_ele->Attribute("error"); //一般INS_Normal不定义，那么取基础报文的定义
		if ( err_code ) 
		{	
			err_var = g_vars->look(err_code, me_vars);
			if (err_var)
			{
				err_code = (const char*)&err_var->content[0];
			} 
		}
		set_condition ( pac_ele, g_vars, me_vars);
	};
};

struct ComplexSubSerial {
	TiXmlElement *usr_def_entry;//MAP文档中，对用户指令的定义
	TiXmlElement *sub_pro;		//实际使用的指令序列，在相同用户指令名下，可能有不同的序列,可能usr_def_entry相同, 而sub_pro不同。

	struct PVar_Set *g_var_set;	//全局变量集
	struct PVar_Set sv_set;		//局域变量集, 用于引进参数型的用户命令

	struct PacIns *pac_inses;
	int pac_many;

	TiXmlElement *def_root;	//基础报文定义
	TiXmlElement *map_root;
	TiXmlElement *usr_ele;	//用户命令

	ComplexSubSerial()
	{
		map_root = def_root = usr_ele = 0;

		usr_def_entry = 0;
		sub_pro = 0;

		pac_inses = 0;
		pac_many = 0;
		g_var_set = 0;
	};

	~ComplexSubSerial()
	{
		if (pac_inses) delete []pac_inses;
		pac_inses = 0;
		pac_many = 0;
	};

	/* 局域变量名根据protect所指向的元素属性名而变, 这样自然, 不用para1,para2之类的。 
		vnm: 是一个变量名，如果在全局表中查不到，则当作变量内容
		mid_num：是Me中指定的,是子元素或primay属性所指定的，局域变量名为: me.mid_num.xx
	*/
	struct PVar *set_loc_ref_var(const char *vnm, const char *mid_nm)
	{
		unsigned char buf[512];		//实际内容, 常数内容
		char loc_v_nm[128];
		TiXmlAttribute *att; 
		unsigned long len;
		struct PVar *ref_var = 0, *att_var=0;
		const char *att_val;
		
		len = 0;
		ref_var = g_var_set->one_still(vnm, buf, len);	//找到已定义参考变量的
		if ( ref_var)
		{
			for ( att = ref_var->self_ele->FirstAttribute(); att; att = att->Next())
			{
				//把属性加到本地变量集 sv_set
				TEXTUS_SPRINTF(loc_v_nm, "%s%s.%s", ME_VARIABLE_HEAD, mid_nm, att->Name()); 
				att_val = att->Value();
				if ( !att_val ) continue;
				len = 0;
				/* 这里 att->Value() 可能指向另一个变量名 */
				att_var = g_var_set->one_still(att_val, buf, len);	//找到已定义的变量
				if ( att_var ) 
				{
					sv_set.put_var(loc_v_nm, att_var);
				} else {
					if ( len > 0 ) 
						sv_set.put_still(loc_v_nm, buf, len);
				}
			}
		}

		if ( len > 0 )	//找到的全局变量可能有内容，加到本地中。
		{
			TEXTUS_SPRINTF(loc_v_nm, "%s%s", ME_VARIABLE_HEAD, mid_nm); 
			sv_set.put_still(loc_v_nm, buf, len);
		}
		return ref_var;
	};

	//ref_vnm是一个参考变量名, 如$Main之类的. 根据这个$Main从全局变量集找到相应的定义。
	int pro_analyze( const char *pri_vnm)
	{
		struct PVar *ref_var, *me_var;
		const char *ref_nm;
		char pro_nm[128];
		TiXmlElement *pac_ele, *def_ele, *spac_ele, *t_ele;
		TiXmlElement *body;	//用户命令的第一个body元素
		int which, icc_num=0, i;
		const char *pri_key = usr_def_entry->Attribute("primary");

		sv_set.defer_vars(usr_def_entry); //局域变量定义完全还在那个自定义中
		for ( i = 0; i  < sv_set.many; i++ )
		{
			me_var = &(sv_set.vars[i]);
			if (me_var->kind != VAR_Me ) continue;		//只处理Me变量
			if ( pri_key && strcmp(me_var->me_name, pri_key) == 0 ) continue; //主参考变量下面处理

			if ( me_var->me_sub_name) //有后缀名, 这应该是参考变量
			{
				if ( me_var->c_len > 0 ) continue;	//有内容就不再处理了。
				ref_nm = usr_ele->Attribute(me_var->me_name);	//先看属性名为me.XX.yy中的XX名，ref_nm是$Main之的。
				if (!ref_nm )	//属性优先, 没有属性再看元素
				{
					body = usr_ele->FirstChildElement(me_var->me_name);	//再看元素为me.XX.yy中的XX名，ref_nm是$Main之的。
					if ( body ) ref_nm = body->GetText();
				}
				if (ref_nm )
					ref_var = set_loc_ref_var(ref_nm, me_var->me_name); /* primary属性指明protect之类的, 实际上就是me.protect.*这样的东西。这里更新局部变量集 */
			}
		}

		TEXTUS_SNPRINTF(pro_nm, sizeof(pro_nm), "%s", "Pro"); //先假定子序列是Pro element，如果有主参考变量，下面会更新。
		if ( pri_vnm ) //如果有主参考变量, 就即根据这个主参考变量中找到相应的sub_pro, pri_vnm就是$Main之类的。
		{
			ref_var = set_loc_ref_var(pri_vnm, pri_key); /* primary属性指明protect之类的, 实际上就是me.protect.*这样的东西。这里更新局部变量集 */
			if ( ref_var )
			{
				if (ref_var->self_ele->Attribute("pro") ) //参考变量的pro属性指示子序列
				{
					TEXTUS_SNPRINTF(pro_nm, sizeof(pro_nm), "%s%s", "Pro", ref_var->self_ele->Attribute("pro"));						
				}
			}
		}

		sub_pro = usr_def_entry->FirstChildElement(pro_nm);	//定位实际的子系列
		if ( !sub_pro ) return 0; //没有子序列 

		pac_many = 0;
		for ( pac_ele= sub_pro->FirstChildElement(); pac_ele; pac_ele = pac_ele->NextSiblingElement())
		{
			if ( !pac_ele->Value() ) continue;
			if ( def_root->FirstChildElement(pac_ele->Value()))	//如果在基础报文中有定义
			{
				pac_many++;
			} else if ( (t_ele = map_root->FirstChildElement(pac_ele->Value()) ) )	//如果在map中有定义, 也就是一个嵌套(类似于宏)
			{
				for ( spac_ele= t_ele->FirstChildElement(); spac_ele; spac_ele = spac_ele->NextSiblingElement())
				{
					if ( !spac_ele->Value() ) continue;
					if ( def_root->FirstChildElement(spac_ele->Value()) )	//如果在基础报文中有定义
						pac_many++;
				}
			}
		}
		//确定变量数
		if ( pac_many ==0 ) return 0;
		pac_inses = new struct PacIns[pac_many];

		which = 0; icc_num = 0;
		for ( pac_ele= sub_pro->FirstChildElement(); pac_ele; pac_ele = pac_ele->NextSiblingElement())
		{
			if ( !pac_ele->Value() ) continue;
			//printf("pac_ele->Value %s\n", pac_ele->Value());
			def_ele = def_root->FirstChildElement(pac_ele->Value());	//如果在基础报文中有定义
			if ( def_ele)	//如果在基础报文中有定义
			{
				pac_inses[which].prepare(def_ele, pac_ele, usr_ele, g_var_set, &sv_set);
				icc_num += pac_inses[which].hard_work(def_ele, pac_ele, usr_ele, g_var_set, &sv_set);
				which++;
			} else if ((t_ele = map_root->FirstChildElement(pac_ele->Value())))//如果在map中有定义, 也就是一个嵌套(类似于宏)
			{
				for ( spac_ele= t_ele->FirstChildElement(); spac_ele; spac_ele = spac_ele->NextSiblingElement())
				{
					if ( !spac_ele->Value() ) continue;
					def_ele = def_root->FirstChildElement(spac_ele->Value());	//如果在基础报文中有定义
					if (def_ele)
					{
						pac_inses[which].prepare(def_ele, spac_ele, usr_ele, g_var_set, &sv_set);
						icc_num += pac_inses[which].hard_work(def_ele, spac_ele, usr_ele, g_var_set, &sv_set);
						which++;
					}
				}
			}
		}
		return icc_num;
	};
};

struct User_Command : public Condition {
	int order;
	struct ComplexSubSerial *complex;
	int comp_num; //一般只有一个，有时需要重试几个

	int  set_sub( TiXmlElement *usr_ele, struct PVar_Set *vrset, TiXmlElement *sub_serial, TiXmlElement *def_root, TiXmlElement * map_root) //返回对IC的指令数
	{
		TiXmlElement *pri;
		const char *pri_nm;
		int ret_ic=0, i;

		//前面已经分析过了, 这里肯定不为NULL,nm就是Command之类的。 sub_serial 
		pri_nm = sub_serial->Attribute("primary");
		if ( pri_nm)
		{
			if ( usr_ele->Attribute(pri_nm))
			{
				/* pro_analyze 根据变量名, 去找到实际真正的变量内容(必须是参考变量)。变量内容中指定了Pro等 */
				comp_num = 1;
				complex = new struct ComplexSubSerial;
				#define PUT_COMPLEX(X) \
						complex[X].usr_def_entry = sub_serial;	\
						complex[X].g_var_set = vrset;			\
						complex[X].def_root = def_root;			\
						complex[X].usr_ele = usr_ele;			\
						complex[X].map_root = map_root;

				PUT_COMPLEX(0)
				ret_ic = complex->pro_analyze(usr_ele->Attribute(pri_nm));
			} else {	//一个用户操作，包括几个复合指令的尝试，有一个成功，就算OK
				for( pri = usr_ele->FirstChildElement(pri_nm), comp_num = 0; 
					pri; pri = pri->NextSiblingElement(pri_nm) )
				{
					comp_num++;
				}
				complex = new struct ComplexSubSerial[comp_num];

				for( pri = usr_ele->FirstChildElement(pri_nm), i = 0; 
					pri; pri = pri->NextSiblingElement(pri_nm) )
				{
					PUT_COMPLEX(i)
					ret_ic = complex[i].pro_analyze(pri->GetText()); //多个可选，指令数就计最后一个
					i++;
				}
			}
		} else {
			comp_num = 1;
			complex = new struct ComplexSubSerial;
			PUT_COMPLEX(0)
			ret_ic = complex->pro_analyze(0);
		}

		set_condition ( usr_ele, vrset, 0);
		return ret_ic;
	};
};

struct INS_Set {	
	struct User_Command *instructions;
	int many;
	INS_Set () 
	{
		instructions= 0;
		many = 0;
	};

	~INS_Set () 
	{
		if (instructions ) delete []instructions;
		instructions = 0;
		many = 0;
	};

	TiXmlElement *yes_ins(TiXmlElement *app_ele, TiXmlElement *map_root, struct PVar_Set *var_set)
	{
		TiXmlElement *sub_serial;
		const char *nm = app_ele->Value();

		if ( var_set->is_var(nm)) return 0;

		sub_serial = map_root->FirstChildElement(nm); //找到子系列		
		return sub_serial;
	};

	void put_inses(TiXmlElement *root, struct PVar_Set *var_set, TiXmlElement *map_root, TiXmlElement *pac_def_root)
	{
		TiXmlElement *usr_ele, *sub;
		int mor, cor, vmany, refny, i_num ;

		for ( usr_ele= root->FirstChildElement(), refny = 0; 
			usr_ele; usr_ele = usr_ele->NextSiblingElement())
		{
			if ( usr_ele->Value() )
			{
				if (yes_ins(usr_ele, map_root, var_set) )
					refny++;				
			}
		}

		//初步确定变量数
		many = refny ;
		instructions = new struct User_Command[many];
			
		mor = -999999;	//这样，顺序号可以从负数开始
		i_num = 0;
		vmany = 0;
		for ( usr_ele= root->FirstChildElement(); usr_ele; usr_ele = usr_ele->NextSiblingElement())
		{
			if ( usr_ele->Value() )
			{
				sub = yes_ins(usr_ele, map_root, var_set);
				if ( sub)
				{
					cor = 0;
					usr_ele->QueryIntAttribute("order", &(cor)); 
					if ( cor <= mor ) continue;	//order不符合顺序的，略过
					instructions[vmany].order = cor;
					i_num += instructions[vmany].set_sub(usr_ele, var_set, sub, pac_def_root, map_root);
					mor = cor;
					vmany++;
				}
			}
		}
		many = vmany; //最后再更新一次用户命令数
	};
};	

/* User_Command指令集定义结束 */
struct  Personal_Def	//个人化定义
{
	TiXmlDocument doc_c;	//User_Command指令定义；
	TiXmlElement *c_root;	
	TiXmlDocument doc_k;	//map：Variable定义，子序列定义
	TiXmlElement *k_root;

	TiXmlDocument doc_pac_def;	//pacdef：报文定义
	TiXmlElement *pac_def_root;

	TiXmlDocument doc_v;	//其它Variable定义
	TiXmlElement *v_root;
		
	struct PVar_Set person_vars;
	struct INS_Set ins_all;

	char flow_md[64];	//指令流指纹数据
	const char *flow_id;	//指令流标志

	inline Personal_Def () {
		c_root = k_root = v_root = pac_def_root = 0;
		flow_id = 0;
		memset(flow_md, 0, sizeof(flow_md));
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
		const char *vn=VARIABLE_TAG_NAME;
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
		const char *ic_nm, *map_nm, *v_nm, *df_nm;
		if ( !per_ele) return false;

		if ( (df_nm = per_ele->Attribute("pac")))
			load_xml(df_nm, doc_pac_def,  pac_def_root, per_ele->Attribute("pac_md5"));
		else
			pac_def_root = per_ele->FirstChildElement("pac");

		if ( (ic_nm = per_ele->Attribute("flow")))
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

		if ( !c_root || !k_root || !pac_def_root ) 
			return false;
		if ( c_root)
		{
			if ( k_root ) 
				person_vars.defer_vars(k_root, c_root);	//变量定义, map文件优先
			else
				person_vars.defer_vars(c_root);	//变量定义, map文件优先
			flow_id = c_root->Attribute("flow");
			if ( per_ele->Attribute("md5") )
				squeeze(per_ele->Attribute("md5"), (unsigned char*)&flow_md[0]);
		}
		set_here(c_root);	//再看本定义
		set_here(v_root);	//看看其它变量定义,key.xml等

		ins_all.put_inses(c_root, &person_vars, k_root, pac_def_root);//用户命令集定义.
		return true;
	};
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

	void put_def(TiXmlElement *prop, const char *vn)	//个人化集合输入定义PersonDef_Set
	{
		TiXmlElement *key_ele, *per_ele;
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

private:
	void h_fail(char tmp[], char desc[], int p_len, int q_len, const char *p, const char *q, const char *fun);
	void mk_hand();	//还有右边状态处理
	void mk_result();

	struct G_CFG 	//全局定义
	{
		TiXmlElement *prop;
		struct PersonDef_Set person_defs;
		struct Personal_Def null_icp_def;

		int maxium_fldno;		/* 最大域号 */
		int flowID_fld_no;	//流标识域, 业务代码域, 

		inline G_CFG() {
			maxium_fldno = 64;
			flowID_fld_no = 3;
		};	
		inline ~G_CFG() { };
	};

	PacketObj hi_req, hi_reply; /* 向右传递的, 可能是对HMS, 或是对IC终端 */
	PacketObj *hipa[3];
	PacketObj *rcv_pac;	/* 来自左节点的PacketObj */
	PacketObj *snd_pac;
	Amor::Pius loc_pro_pac;

	struct MK_Session mess;	//记录一个制卡过程中的各种临时数据
	struct Personal_Def *cur_def;	//当前定义

	struct G_CFG *gCFG;     /* 全局共享参数 */
	bool has_config;
	struct WKBase {
		int step;	//0: just start, 1: doing 
		int cur;
		int pac_which;
		int pac_step;	//0: send, 1: recv
	} command_wt;

	int sub_serial_pro(struct ComplexSubSerial *comp);
	bool call_back;	/* false: 一般如此，向右发出后，在下一轮中再处理响应; 
			true: 对于函数处理的扩展，有回调处理，在sponte时就即返回，由发出点处理响应数据，从而避免调用嵌套太深。
			*/
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

	if ( (comm_str = prop->Attribute("max_fld")) )
		gCFG->maxium_fldno = atoi(comm_str);

	if ( (comm_str = prop->Attribute("flow_fld")) )
		gCFG->flowID_fld_no = atoi(comm_str);

	if ( gCFG->maxium_fldno < 0 )
		gCFG->maxium_fldno = 0;
	hi_req.produce(gCFG->maxium_fldno) ;
	hi_reply.produce(gCFG->maxium_fldno) ;

	return;
}

PacWay::PacWay()
{
	hipa[0] = &hi_req;
	hipa[1] = &hi_reply;
	hipa[2] = 0;

	gCFG = 0;
	has_config = false;
	loc_pro_pac.ordo = Notitia::PRO_UNIPAC;
	loc_pro_pac.indic = 0;
	loc_pro_pac.subor = -1;

	call_back = false;
}

PacWay::~PacWay() 
{
	if ( has_config  )
	{	
		if(gCFG) delete gCFG;
		gCFG = 0;
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
	Amor::Pius tmp_pius;

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
		WBUG("facio PRO_UNIPAC");
		handle_pac();
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		gCFG->person_defs.put_def(gCFG->prop, "bus");
		gCFG->null_icp_def.put_def(gCFG->prop->FirstChildElement("bike"), 0);
		mess.init(gCFG->person_defs.max_snap_num);
		if ( err_global_str[0] != 0 )
		{
			WLOG(ERR,"%s", err_global_str);
		}
		tmp_pius.ordo = Notitia::SET_UNIPAC;
		tmp_pius.indic = &hipa[0];
		aptus->facio(&tmp_pius);
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
		mess.init(gCFG->person_defs.max_snap_num);
		tmp_pius.ordo = Notitia::SET_UNIPAC;
		tmp_pius.indic = &hipa[0];
		aptus->facio(&tmp_pius);
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
	if (!gCFG ) return false;

	switch ( pius->ordo )
	{
	case Notitia::PRO_UNIPAC:
		WBUG("sponte PRO_UNIPAC");
		if (!call_back) //对于回调函数，即返回。
			mk_hand();
		break;

	case Notitia::DMD_END_SESSION:	//右节点关闭, 要处理
		WBUG("sponte DMD_END_SESSION");
		if ( mess.left_status == LT_Working  )	//表明是制卡工作
		{
			mess.iRet = ERROR_DEVICE_DOWN;
			TEXTUS_SPRINTF(mess.err_str, "device down at %d", pius->subor);
			mk_result();	//结束
		}
		break;

	case Notitia::START_SESSION:	//右节通知, 已活
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

	struct PVar  *vt;
	struct DyVar *dvr;

	actp=rcv_pac->getfld(gCFG->flowID_fld_no, &alen);		//取得业务代码, 即流标识
	if ( !actp)
	{
		WBUG("business code field null");
		return;
	}

	switch ( mess.left_status) 
	{
	case LT_Idle:
		/* 这里就是一般的业务啦 */
		mess.reset();	//会话复位
		snd_pac->reset();
		actp = rcv_pac->getfld(gCFG->flowID_fld_no, &alen);
		if (alen > 63 ) alen = 63;
		memcpy(mess.flow_id, actp, alen);
		mess.flow_id[alen] = 0;
		mess.iRet = 0;	//假定一开始都是OK。
		TEXTUS_STRCPY(mess.err_str, " ");

		cur_def = gCFG->person_defs.look(mess.flow_id); //找一个相应的指令流定义
		if ( !cur_def ) 
		{
			mess.iRet = ERROR_INS_DEF;	
			cur_def = &(gCFG->null_icp_def);
		}
		/* 寻找变量集中所有动态的, 看看是否有start_pos和get_length的, 根据定义赋值到mess中 */
		for ( i = 0 ; i <  cur_def->person_vars.many; i++)
		{
			vt = &cur_def->person_vars.vars[i];
			if ( vt->dynamic_pos >=0 )
			{
				dvr = &mess.snap[vt->dynamic_pos];
				dvr->kind = vt->kind;
				dvr->def_var = vt;
				if ( vt->c_len > 0 )	//先把定义的静态内容链接过来, 动态变量的默认值
					dvr->input(&(vt->content[0]), vt->c_len, true);	

				if ( vt->source_fld_no >=0 )
					p = rcv_pac->getfld(vt->source_fld_no, &plen);
				else
					continue;
				if (!p) continue;
				if ( plen > vt->start_pos )	//偏移量超出长度，当然不用取了
				{
					plen -= (vt->start_pos-1);	//plen为实际能取的长度, start_pos是从1开始
					if ( vt->get_length > 0 && vt->get_length < plen) 
						plen = vt->get_length;
					dvr->input( &p[vt->start_pos-1], plen);
				}
				/* 所以从域取值为优先, 如果实际报文没有该域, 就取这里的静态定义 */
			}
		}
		if (mess.iRet == ERROR_INS_DEF)
		{
			TEXTUS_SPRINTF(mess.err_str, "not defined flow_id: %s ", mess.flow_id );
			mess.snap[Pos_ErrCode].input(mess.iRet);
			mk_result();	//工作结束
			goto HERE_END;
		}
		/* 任务开始  */
		mess.snap[Pos_TotalIns].input( cur_def->ins_all.many);
		if ( cur_def->flow_md[0] )
			mess.snap[Pos_FlowPrint].input( cur_def->flow_md);

		mess.ins_which = 0;
		mess.left_status = LT_Working;
		mess.right_status = RT_READY;	//指示终端准备开始工作,

	//{int *a =0 ; *a = 0; };
		mk_hand();
HERE_END:
		break;

	case LT_Working:	//不接受, 不响应即可
		WLOG(WARNING,"still working!");
		break;

	default:
		break;
	}
	return;
}

/* 子序列入口 */
int PacWay::sub_serial_pro(struct ComplexSubSerial *comp)
{
	int i_ret=1;
	struct PacIns *paci;

SUB_INS_PRO:
	paci = &(comp->pac_inses[command_wt.pac_which]);
	switch ( paci->type)
	{
	case INS_Normal:
		switch ( command_wt.pac_step )
		{
		case 0:
			if ( !paci->valid_condition(&mess) )		/* 不符合条件,就转下一条 */
			{
				i_ret = 1;
				break;
			}

			hi_req.reset();	//请求复位
			paci->get_snd_pac(&hi_req, loc_pro_pac.subor, &mess);
			command_wt.pac_step++;
			i_ret = 0;	/* 进行中 */
			call_back = paci->isFunction; //对于函数，属回调的情况。这个call_back在sponte时被判断
			mess.right_status = RT_OUT;
			aptus->facio(&loc_pro_pac);     //向右发出, 然后等待
			if ( call_back ) goto GO_ON_FUNC; //调用返回时，这里响应报文已备，继续处理。
			break;
		case 1:
GO_ON_FUNC:
			if ( paci->pro_rcv_pac(&hi_reply, &mess)) 
			{
				if ( paci->valid_result(&mess) )
					i_ret = 1;
				else {
					mess.iRet = ERROR_RESULT;
					TEXTUS_SPRINTF(mess.err_str, "result error at %d of %s", mess.pro_order, cur_def->flow_id);
					if ( !paci->err_code) mess.snap[Pos_ErrCode].input(paci->err_code);
					i_ret = -2;	//这是map所控制
				}
			} else {
				mess.iRet = ERROR_RECV_PAC;
				TEXTUS_SPRINTF(mess.err_str, "fault at %d of %s", mess.pro_order, cur_def->flow_id);
				i_ret = -3;	//这是基本报文错误，非map所控制
			}
			break;

		default:
			break;
		}
		break;

	case INS_Abort:
		i_ret = -1;	//脚本所控制的错误
		if (paci->err_code ) mess.snap[Pos_ErrCode].input(paci->err_code);
		break;

	case INS_Null:
		if ( paci->valid_condition(&mess) )		/* 符合前提条件, 再判断结果 */
		{
			if ( paci->valid_result(&mess) )
				i_ret = 1;
			else {
				mess.iRet = ERROR_RESULT;
				TEXTUS_SPRINTF(mess.err_str, "result error at %d of %s", mess.pro_order, cur_def->flow_id);
				if ( !paci->err_code) mess.snap[Pos_ErrCode].input(paci->err_code);
				i_ret = -2;	//这是map所控制
			}
		} else {
			i_ret = 1;
		}
		break;

	default :
		break;
	}

	if ( i_ret > 0 )
	{
		command_wt.pac_which++;	//指向下一条报文处理
		command_wt.pac_step = 0;
		if (  command_wt.pac_which == comp->pac_many )
		{
			return 1;	//整个已经完成
		} else
			goto SUB_INS_PRO;
	}
	return i_ret;
}

void PacWay::mk_hand()
{
	struct User_Command *usr_com;
	int i_ret;

INS_PRO:
	usr_com = &(cur_def->ins_all.instructions[mess.ins_which]);
	mess.pro_order = usr_com->order;	

	if ( mess.right_status  ==  RT_READY )	//终端空闲,各类工作单元清空复位
	{
		command_wt.step=0;
		command_wt.cur = 0;
	}
	//{int *a =0 ; *a = 0; };
	switch ( command_wt.step)
	{
		case 0:	//开始
			if ( !usr_com->valid_condition(&mess) )
			{
				mess.right_status = RT_READY ;	//右端闲, 可以下一条命令
				break;
			}
			command_wt.cur = 0;
NEXT_PRI_TRY:
			command_wt.pac_which = 0;	//新子系列, pac从第0个开始
			command_wt.pac_step = 0;	//pac处理开始, 
			if ( usr_com->comp_num == 1 )
				mess.willLast = true;
			else 
				mess.willLast = false; //一个用户操作，包括几个复合指令的尝试，有一个成功，就算OK
			command_wt.step++;	//指向下一步

		case 1:
			i_ret = sub_serial_pro( &(usr_com->complex[command_wt.cur]) );
			if ( i_ret == -1 ) 
			{
				if ( command_wt.cur < (usr_com->comp_num-1) )	//用户定义的Abort才试下一个
				{
					if ( command_wt.cur == (usr_com->comp_num-1) ) //最后一条复合指令啦，如果出错就调用自定义的出错过程(响应报文设置一些数据，或者向终端发些指令)
						mess.willLast = true;
					command_wt.cur++;
					command_wt.step--;
					goto NEXT_PRI_TRY;		//试另一个
				} else {		//最后一条处理失败，定义出错值
					mess.iRet = ERROR_USER_ABORT;			
					TEXTUS_SPRINTF(mess.err_str, "user abort at %d of %s", mess.pro_order, cur_def->flow_id);
				}
			} else if ( i_ret < 0 )  //脚本控制或报文定义 的 错误
			{
				mk_result(); 
			} else 	if ( i_ret > 0  ) 
			{
				mess.right_status = RT_READY;	//右端闲
				WBUG("has completed %d, order %d", mess.ins_which, mess.pro_order);
			}
			/*if ( i_ret ==0  ) 子序列正在进行, 这里不作任何处理 */
			break;
		default:
			break;
	} //end of switch command_wt.step

	if ( mess.right_status == RT_READY )	//可以下一条用户命令
	{
		mess.ins_which++;
		if ( mess.ins_which < cur_def->ins_all.many)
			goto INS_PRO;		// 下一条
		else 
			mk_result();		//一个交易已经完成
	}
//{int *a =0 ; *a = 0; };
}

void PacWay::mk_result()
{
	struct DyVar *dvr;
	struct PVar  *vt;
	int i;

	if ( mess.iRet != 0 ) 
	{
		WLOG(WARNING, "Error %s:  %s", mess.snap[Pos_ErrCode].val_p, mess.err_str);
		mess.snap[Pos_ErrStr].input(mess.err_str);
	}

	/* 从变量定义集中，包括静态的和动态的，都设置到响应报文中 */
	for ( i = 0 ; i <  cur_def->person_vars.many; i++)
	{
		vt = &cur_def->person_vars.vars[i];
		if ( vt->dest_fld_no < 0 ) continue;
		if ( vt->dynamic_pos >=0 )
		{
			dvr = &mess.snap[vt->dynamic_pos];
			if ( dvr->kind != VAR_None )
				snd_pac->input(vt->dest_fld_no, dvr->val_p, dvr->c_len);
		} else {
			snd_pac->input(vt->dest_fld_no, &vt->content[0], vt->c_len);
		}
	}
	aptus->sponte(&loc_pro_pac);    //制卡的结果回应给控制台
	mess.reset();
}
#include "hook.c"
