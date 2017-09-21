/* Copyright (c) 2016-2018 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 Title:PacWay
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
#define ERROR_RECV_PAC -202
#define ERROR_RESULT -210
#define ERROR_USER_ABORT -203
#define ERROR_DB_DEF -131
#define ERROR_DB_PRO -131
#define ERROR_INS_DEF -133

char err_global_str[128]={0};
/* 左边状态, 空闲, 等着新请求, 交易进行中 */
enum LEFT_STATUS { LT_Idle = 0, LT_Working = 3};
/* 右边状态, 空闲, 发出报文等响应 */
enum RIGHT_STATUS { RT_IDLE = 0, RT_OUT=7, RT_READY = 3};
/* 抱文交换方式 */
enum PAC_MODE { PAC_NONE = 0, PAC_FIRST =1, PAC_SECOND = 4, PAC_BOTH=5};

enum Var_Type {VAR_ErrCode=1, VAR_FlowPrint=2, VAR_TotalIns = 3, VAR_CurOrder=4, VAR_CurCent=5, VAR_ErrStr=6, VAR_FlowID=7, VAR_Dynamic = 10, VAR_Me=12, VAR_Constant=98,  VAR_None=99};
/* 命令分几种，INS_Normal：标准，INS_Abort：终止 */
enum PacIns_Type { INS_None = 0, INS_Normal=1, INS_Abort=2, INS_SetPeer=3, INS_GetPeer=4, INS_Get_CertNo=5, INS_Pro_DBFace=6, INS_Cmd_Ordo=7, INS_Respond =8, INS_ResultPacPro = 9, INS_Null=99};
enum ACT_DIR { FACIO=0, SPONTE=1 };
enum PAC_STEP {Pac_Idle = 0, Pac_Working = 1, Pac_End=2};

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

	struct PVar {
		Var_Type kind;
		const char *name;	//来自于doc文档
		int n_len;		//名称长度

		unsigned char content[512];	//变量内容. 这个内容与下面的内容一般不会同时有
		int c_len;			//内容长度
		int dynamic_pos;	//动态变量位置, -1表示静态

		char me_name[64];	//Me变量名称，除去开头的 me. 三个字节, 不包括后缀. 从变量名name中复制，最大63字符
		unsigned int me_nm_len;
		const char *me_sub_name;  //Me变量后缀名， 从变量名name中定位。
		int me_sub_nm_len;
		bool me_pri_refer;	//主参考变量标记, true: 在合成请报文(hard_work_2)时, 不再从用户ele中取，因为在分析sub_serial时, 已经赋值

		bool keep_alive;	//true: 若是动态变量, 只在Notitia::START_SESSION、DMD_END_SESSION时清空, false: 对每个flow_id开始都清空,
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

			self_ele = 0;

			memset(me_name, 0, sizeof(me_name));
			me_nm_len = 0;
			me_sub_name = 0;
			me_sub_nm_len = 0;
			me_pri_refer = false;

			dy_link = false;	//动态变量的赋值方式为复制方式。
			keep_alive = false;
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
			const char *p, *nm, *aval;
			kind = VAR_None;
			self_ele = var_ele;

			nm = var_ele->Attribute("name");
			if ( !nm ) return 0;

			name = nm;
			n_len = strlen(name);

			p = var_ele->GetText();
			if ( p)
			{
				aval = var_ele->Attribute("escape");
				if ( !aval ) 
					aval = var_ele->GetDocument()->RootElement()->Attribute("escape");
				if ( aval && (aval[0] == 'Y' || aval[0] == 'y') )
				{
					c_len = BTool::unescape(p, content) ;
				} else 
					c_len = squeeze(p, content);
			}
			/* 变量无内容, 才认为这是特殊变量 */

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

			aval =  var_ele->Attribute("link");
			if ( aval && (aval[0] == 'Y' || aval[0] == 'y') )
			{
				dy_link = true;
			}

			if ( kind != VAR_None) goto P_RET; //已有定义，不再看这个Dynamic, 以上定义都与Dynamic相同处理

			if ( (p = var_ele->Attribute("dynamic")) && (*p == 'Y' || *p == 'y') )
			{
				dynamic_pos = dy_at;	//动态变量位置
				kind = VAR_Dynamic;
				dy_at++;
				if ( (p = var_ele->Attribute("alive")) && (*p == 'Y' || *p == 'y') )
				{
					keep_alive = true;
				}
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
			const char *p, *esc;

			if( (p = h_ele->GetText()) )
			{
				esc = h_ele->Attribute("escape");
				if ( !esc ) 
					esc = h_ele->GetDocument()->RootElement()->Attribute("escape");
				if ( esc && (esc[0] == 'Y' || esc[0] == 'y') )
				{
					c_len = BTool::unescape(p, content) ;
				} else 
					c_len = squeeze(p, content);
				if (kind == VAR_None)	//只有原来没有定义类型的, 这里才定成常数. 
					kind = VAR_Constant;
			}
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
			def_var = 0;

			memset(val, 0, sizeof(val));
		};

		void input(unsigned char *p, unsigned long len, bool link=false)
		{
			if ( !def_var ) return;
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

		void input(int iv) {
			if ( !def_var ) return;
			TEXTUS_SPRINTF(val, "%d", iv);
			c_len = strlen((char*)&val[0]);
			val[c_len] = 0;
			val_p = (unsigned char*)&val[0];
		};

		void input(const char *p, bool link=false)
		{
			if ( !def_var ) return;
			c_len = strlen(p);
			if ( def_var->dy_link || link)
			{
				val_p = (unsigned char*)p;
			} else {
				if ( c_len > (sizeof(val)-1) )	
					c_len = sizeof(val)-1;
				val[c_len] = 0;
				val_p = (unsigned char*)&val[0];
				memcpy(val, p, c_len);
			}
		};

		void input(const char p) {
			if ( !def_var ) return;
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
		int right_subor;	//指示向右发出时的subor, 返回时核对。
		int ins_which;	//已经工作在哪个命令, 即为定义中数组的下标值
		int iRet;	//事务最终结果
		bool handle_last_pac;	//处理最后报文

		inline MK_Session () {
			snap=0;
			snap_num = 0;
		};

		inline void  reset(bool soft=true) {
			int i;
			for ( i = 0; i < snap_num; i++)
			{
				if ( !soft || !snap[i].def_var || (snap[i].def_var && !snap[i].def_var->keep_alive) )
				{
					snap[i].c_len = 0;
					snap[i].val[0] = 0;
					snap[i].val_p = 0;
					snap[i].def_var = 0;
					if ( i >= Pos_Fixed_Next )
					{
						/* 这个Pos_Fixed_Next很重要, 要不然, 那些固有的动态变量会没有的！  */
						snap[i].kind = VAR_None;
					}
				}
			}
			left_status = LT_Idle;
			right_status = RT_IDLE;
			right_subor = 0;
			ins_which = -1;
			err_str[0] = 0;	
			flow_id[0] = 0;
			willLast = true;
			handle_last_pac = false;
		};

		inline void init(int m_snap_num) //这个m_snap_num来自各XML定义的最大动态变量数
		{
			int i;
			if ( snap )	return ;
			if ( m_snap_num <=0 ) return ;

			snap_num = m_snap_num;
			snap = new struct DyVar[snap_num];
			for ( i = 0 ; i < snap_num; i++)
				snap[i].index = i;

			snap[Pos_ErrCode].kind = VAR_ErrCode;
			snap[Pos_FlowPrint].kind = VAR_FlowPrint;
			snap[Pos_TotalIns].kind = VAR_TotalIns;
			snap[Pos_CurOrder].kind = VAR_CurOrder;
			snap[Pos_CurCent].kind = VAR_CurCent;
			snap[Pos_ErrStr].kind = VAR_ErrStr; 
			snap[Pos_FlowID].kind = VAR_FlowID; 
			reset(false);	//动态量硬复位
		};

		~MK_Session () {
			if ( snap ) delete[] snap;
			snap = 0;
		};
	};

/* 变量集合*/
struct PVar_Set {	
	struct PVar *vars;
	int many;
	int dynamic_at;
	TiXmlElement *command_ele;	/* 对于局域变量集, 指向子系列所相应的用户命令(Command)  */
	PVar_Set () {
		vars = 0;
		many = 0;
		dynamic_at = Pos_Fixed_Next; //0,等 已经给$FlowPrint等占了
		command_ele = 0;
	};

	~PVar_Set () {
		if (vars ) delete []vars;
		vars = 0;
		many = 0;
	};

	bool is_var(const char *nm) {
		if (nm  && strcasecmp(nm, VARIABLE_TAG_NAME) == 0 ) return true;
		return false;
	};

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
	struct PVar *one_still(TiXmlElement *comp, const char *nm, unsigned char *buf, unsigned long &len, struct PVar_Set *loc_v=0)
	{
		struct PVar  *vt;
		const char *esc = 0;
		/* 在这时两种情况处理, 一个是有静态常数定义的, 另一个静态常数变量 */
		vt = look(nm, loc_v);	//看看是否为一个定义的变量名
		if ( !vt) 
		{
			if ( comp )
			{
				esc = comp->Attribute("escape");
				if ( !esc ) 
					esc = comp->GetDocument()->RootElement()->Attribute("escape");
			}
			if ( esc && (esc[0] == 'Y' || esc[0] == 'y') )
			{
				if ( buf)
					len = BTool::unescape(nm, buf) ;
				else 
					len = strlen(nm);
			} else 
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
				rt = one_still(comp,  comp->GetText(), &command[ac_len], l, loc_v);
			else 
				rt = one_still(comp, comp->GetText(), 0, l, loc_v);
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
	bool c_case;	/* 是否区分大小写, 默认为是 */
	MatchDst () {
		dst = 0;
		con_dst = 0;
		len_dst = 0;
		c_case = true;
	};

	bool set_val(struct PVar_Set *var_set, struct PVar_Set *loc_v, const char *p, const char *case_str)
	{
		bool ret = false;
		const char *val_nm;
		TiXmlElement *body;

		if ( case_str && ( case_str[0] == 'N' ||  case_str[0] == 'n') )
			c_case= false;
		else
			c_case = true;
		dst = var_set->look(p, loc_v);
		if (!dst )
		{
			if ( p) 
			{	//变量名可能没有内容, 这里有内容是常数了, 其它的也可以有内容, 就要别处定义了
				con_dst = p;
				len_dst = strlen(con_dst);
				ret = true;
			}
		}  else if (dst->kind == VAR_Me && loc_v ) //只处理Me变量
		{
			if ( dst->c_len > 0 ) 
			{
				con_dst = (const char*)&dst->content[0];
				len_dst = dst->c_len;
				dst = 0;
				ret = true;
			} else {
				val_nm = 0;
				val_nm = loc_v->command_ele->Attribute(dst->me_name);
				if (!val_nm )	//属性优先, 没有属性再看元素
				{
					body = loc_v->command_ele->FirstChildElement(dst->me_name);
					if ( body ) val_nm = body->GetText();
				}
				if ( val_nm)
				{
					dst = var_set->look(val_nm);
					if (!dst )
					{
						con_dst = val_nm;
						len_dst = strlen(con_dst);
					}
					ret = true;
				}
			}
		}
		return ret;
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
		if ( dst_len != src_len )
			ret = false;
		else {
			if ( c_case )
				ret = (memcmp(dst_con, src_con, src_len) == 0);
			else
				ret = (strncasecmp((const char*)dst_con, (const char*)src_con, src_len) == 0);
		}

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
			dst_arr->set_val(var_set, loc_v, p, mch_ele->Attribute("case"));
		} else {
			dst_arr = new struct MatchDst[dst_num];
			for (	i = 0, con_ele = mch_ele->FirstChildElement(vn); 
				con_ele; 
				con_ele = con_ele->NextSiblingElement(vn),i++ ) 
			{
				p = con_ele->GetText();
				dst_arr[i].set_val(var_set, loc_v, p, con_ele->Attribute("case"));
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
	DyList () {
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
		bool now_still, me_has_usr;

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

			if ( vr_tmp->kind == VAR_Me && vr_tmp->me_sub_nm_len == 0)	//Me变量,且无后缀名, 则从用户命令中取
			{
				vr2_tmp = 0;
				g_ln = 0 ;
				me_has_usr = false;
				if ( vr_tmp->me_pri_refer) { //对于主参考, 就不再从用户命令中取
					goto HAD_LOOK;
				}
				if ( usr_ele->Attribute(vr_tmp->me_name) ) 	//用户命令中，属性优先
				{
					vr2_tmp = g_vars->one_still(0, usr_ele->Attribute(vr_tmp->me_name), 0, g_ln);
					if ( g_ln > 0 ) cmd_len += g_ln;
					if ( vr2_tmp && vr2_tmp->kind <= VAR_Dynamic )
						dy_num++;
					me_has_usr = true;
				} else {
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
							dy_num++;
						me_has_usr = true;
					}
				}
				HAD_LOOK:
				if ( !me_has_usr && vr_tmp->c_len > 0) //用户命令中未找到
					cmd_len += vr_tmp->c_len;
			}

			if (vr_tmp->kind == VAR_Me && vr_tmp->c_len > 0 && vr_tmp->me_sub_nm_len > 0)	//Me变量已有内容,可能已有定义,并带后缀
				cmd_len += vr_tmp->c_len;
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

			if ( vr_tmp->kind == VAR_Me && vr_tmp->me_sub_nm_len == 0)	//Me变量,且无后缀名, 则从用户命令中取，当然还没有内容
			{
				vr2_tmp = 0;
				g_ln = 0;
				me_has_usr = false;
				if ( vr_tmp->me_pri_refer) { //对于主参考, 就不再从用户命令中取
					goto HAD_LOOK_VAR;
				}
				if (usr_ele->Attribute(vr_tmp->me_name)) //先看属性内容,有了属性，不再看子元素了
				{
					vr2_tmp = g_vars->one_still(usr_ele, usr_ele->Attribute(vr_tmp->me_name), cp, g_ln);
					if ( g_ln > 0 )	/* 刚处理的是静态内容 */
					{
						DY_STILL(g_ln) //cp指针后移, 内容增加， 这里游标不变，因为下一个可能是Me变量的静态, 这要合并在一起
					}

					if ( vr2_tmp && vr2_tmp->kind <= VAR_Dynamic )
					{
						DY_DYNAMIC(vr2_tmp->dynamic_pos)	//已有静态内容的， 先指向下一个, 以存放动态的
						DY_NEXT_STILL
					}
					me_has_usr = true;
				} else {
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
						me_has_usr = true;
					}
				}
				HAD_LOOK_VAR:
				if ( !me_has_usr && vr_tmp->c_len > 0) //用户命令中未找到
				{
					memcpy(cp, vr_tmp->content, vr_tmp->c_len);
					DY_STILL(vr_tmp->c_len) //cp指针后移,内容增加， 这里游标不变，因为下一个可能是Me变量的静态, 这要合并在一起
					goto ALL_STILL;	//这里处理的是静态，所以从该处继续
				}
			}
			if ( vr_tmp->kind == VAR_Me && vr_tmp->c_len > 0  && vr_tmp->me_sub_nm_len > 0)	//Me变量已有内容,可能已有定义的，并带后缀的。
			{
				memcpy(cp, vr_tmp->content, vr_tmp->c_len);
				DY_STILL(vr_tmp->c_len) //cp指针后移,内容增加， 这里游标不变，因为下一个可能是Me变量的静态, 这要合并在一起
				goto ALL_STILL;	//这里处理的是静态，所以从该处继续
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
	bool err_disp_hex;

	const char *tag;//比如： reply, sw
	CmdRcv() {
		dyna_pos = -1;
		start =1;
		length = -1;
		must_con = 0;
		must_len = 0;
		err_code = 0;
		err_disp_hex = false;
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
	enum ACT_DIR fac_spo;	//动作方向, to do..... no work yet.
	TEXTUS_ORDO ordo;	//其它动作
	const char *dbface_name;
	DBFace *dbface;

	enum PAC_MODE pac_mode;	/* 报文模式, 一般不交换 */
	bool pac_cross;	/* 是否交叉, 如第一个换到第二个 */

	PacIns() {
		type = INS_None;
		subor = 0;
		snd_lst = 0;
		snd_num = 0;
		rcv_lst = 0;
		rcv_num = 0;

		err_code = 0;
		counted = false;	/* 不计入指令数计算 */
		isFunction = false;
		fac_spo = SPONTE;
		dbface_name = 0;
		dbface = 0;
		pac_mode = PAC_NONE;
		pac_cross = false;
	};

	void pac_cross_before(PacketObj *req_pac, PacketObj *rply_pac, PacketObj *first_pac, PacketObj *second_pac)
	{
		if ( pac_cross)
		{
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

	PacketObj *pac_cross_after(PacketObj *req_pac, PacketObj *rply_pac, PacketObj *first_pac, PacketObj *second_pac)
	{
		PacketObj *n_pac = rply_pac;
		if ( pac_cross)
		{
			if ( pac_mode == PAC_SECOND || pac_mode == PAC_BOTH )
			{
				second_pac->exchange(req_pac);
			}
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
			{
				first_pac->exchange(req_pac);
			}
		}
		return n_pac;
	};

	void get_req_pac( PacketObj *req_pac, int &bor, MK_Session *sess)
	{
		int i,j;
		unsigned long t_len;


		bor = subor;
		t_len = 0;
		for ( i = 0 ; i < snd_num; i++ )
		{
			if ( snd_lst[i].dy_num ==0 ) 
			{ 
				t_len += snd_lst[i].cmd_len;	//这是在pacdef中send元素定义的
			} else {
				for ( j = 0; j < snd_lst[i].dy_num; j++ )
				{
					if (  snd_lst[i].dy_list[j].dy_pos < 0 ) 
					{
						t_len += snd_lst[i].dy_list[j].len;
					} else {
						t_len += sess->snap[snd_lst[i].dy_list[j].dy_pos].c_len;
					}
				}
			}
		}
		req_pac->grant(t_len);
		for ( i = 0 ; i < snd_num; i++ )
		{
			t_len = 0;
			if ( snd_lst[i].dy_num ==0 )
			{	/* 这是在pacdef中send元素定义的一个域的内容 */
				memcpy(req_pac->buf.point, snd_lst[i].cmd_buf, snd_lst[i].cmd_len);
				t_len = snd_lst[i].cmd_len;	
			} else {
				for ( j = 0; j < snd_lst[i].dy_num; j++ )
				{
					/* 由一个列表来构成一个域的内容 */
					if (  snd_lst[i].dy_list[j].dy_pos < 0 ) 
					{
						/* 静态内容*/
						memcpy(&req_pac->buf.point[t_len], snd_lst[i].dy_list[j].con, snd_lst[i].dy_list[j].len);
						t_len += snd_lst[i].dy_list[j].len;
					} else {
						/* 动态内容*/
						memcpy(&req_pac->buf.point[t_len], sess->snap[snd_lst[i].dy_list[j].dy_pos].val_p, sess->snap[snd_lst[i].dy_list[j].dy_pos].c_len);
						t_len += sess->snap[snd_lst[i].dy_list[j].dy_pos].c_len;
					}
				}
			}
			req_pac->commit(snd_lst[i].fld_no, t_len);	//域的确认
		}
		return ;
	};

	/* 本指令处理响应报文，匹配必须的内容,出错时置出错代码变量 */
	bool pro_rply_pac(PacketObj *rply_pac, struct MK_Session *mess)
	{
		int ii;
		unsigned char *fc;
		unsigned long rlen, mlen;
		struct CmdRcv *rply;
		char con[512];
		PacketObj *n_pac=rply_pac;
					
		for (ii = 0; ii < rcv_num; ii++)
		{
			rply = &rcv_lst[ii];
			fc = n_pac->getfld(rply->fld_no, &rlen);
			if (rply->must_con ) 
			{
				if ( !fc ) 
				{
					TEXTUS_SPRINTF(mess->err_str, "field %d does not exist", rply->fld_no);
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
					TEXTUS_SPRINTF(mess->err_str, "field %d contents %s", rply->fld_no, con);
					goto ErrRet;
				}
			}

			if ( rply->dyna_pos > 0 && fc)
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
		const char *p=0, *pp=0; 
		char *q;

		int i = 0;
		int lnn;
		const char *tag;

		p = def_ele->Attribute("dir");	
		if ( p )
		{
			if ( strcasecmp( p, "facio") ==0 )
			{
				fac_spo = FACIO;
			} else if ( strcasecmp( p, "sponte") ==0 )
			{
				fac_spo = SPONTE;
			}
		}

		if ( strcasecmp( pac_ele->Value(), "Abort") ==0 )
		{
			type = INS_Abort;
			goto LAST_CON;
		}

		if ( strcasecmp( pac_ele->Value(), "Null") ==0 )
		{
			type = INS_Null;
			goto LAST_CON;
		}
		
		if ( strcasecmp( pac_ele->Value(), "Respond") ==0 )
		{
			type = INS_Respond;
			goto LAST_CON;
		}
		
		if ( strcasecmp( pac_ele->Value(), "Terminator") ==0 )
		{
			type = INS_ResultPacPro;
			goto DefaultUnipac;
		}
		
		p = def_ele->Attribute("type");	
		if ( !p ) 
		{
			type = INS_Normal;
			goto DefaultUnipac;
		}
		if ( strcasecmp( p, "GetPeer") ==0 )
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
		} else if ( (pp = def_ele->Attribute("ordo")) )
		{
			type =  INS_Cmd_Ordo;
			fac_spo = FACIO;
			ordo = Notitia::get_ordo(pp);
		}

	DefaultUnipac:
		subor=0; def_ele->QueryIntAttribute("subor", &subor);
		if ( (p = def_ele->Attribute("counted")) && ( *p == 'y' || *p == 'Y') )
			counted = true;
		else 
			counted = false;

		if ((p = def_ele->Attribute("function")) && ( *p == 'y' || *p == 'Y') )	//这是函数扩展，出现回调
			isFunction = true;
		else
			isFunction = false;

		if ( (p = def_ele->Attribute("exchange")) )
		{
			q = (char*)strpbrk( p, ":" );
			if ( q ) {
				*q++ = '\0';
			}
			if (strcasecmp ( p, "first") == 0 ) 
			{
				pac_mode = PAC_FIRST;
			} else if ( strcasecmp (p, "second") == 0 )
			{
				pac_mode = PAC_SECOND;
			} else if ( strcasecmp (p, "both") ==0 ) 
			{
				pac_mode = PAC_BOTH;
			}
			if ( q ) {
				if (strcasecmp ( q, "X") == 0 ) 
					pac_cross = true;
			}
		}

		/* 先预置发送的每个域，设定域号*/
		snd_num = 0;
		for (p_ele= def_ele->FirstChildElement(); p_ele; p_ele = p_ele->NextSiblingElement())
		{
			p = p_ele->Value();
			if ( !p ) continue;
			if ( strcasecmp(p, "send") == 0 || p_ele->Attribute("to")) 
				snd_num++;
		}
		if ( snd_num ==0 ) goto RCV_PRO;	
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
	RCV_PRO:
		rcv_num = 0;
		for (p_ele= def_ele->FirstChildElement(); p_ele; p_ele = p_ele->NextSiblingElement())
		{
			tag = p_ele->Value();
			if ( !tag ) continue;
			if ( strcasecmp(tag, "recv") == 0 ||p_ele->Attribute("from") ) 
			{
				rcv_num++;
				if ( strcasecmp(tag, "recv") == 0 ) continue;	/* recv 仅在基础报文定义中出现 */

				/*子序列中的返回元素也算上 */
				for (e_tmp = pac_ele->FirstChildElement(tag); e_tmp; e_tmp = e_tmp->NextSiblingElement(tag) ) rcv_num++;
				/* 用户命令的返回元素也算上 */
				for (e_tmp = usr_ele->FirstChildElement(tag); e_tmp; e_tmp = e_tmp->NextSiblingElement(tag) ) rcv_num++;
			}
		}
		if ( rcv_num ==0 ) goto LAST_CON;	
		rcv_lst = new struct CmdRcv[rcv_num];
		for (p_ele= def_ele->FirstChildElement(),i = 0; p_ele; p_ele = p_ele->NextSiblingElement())
		{
			tag = p_ele->Value();
			if ( !tag ) continue;
			if ( strcasecmp(tag, "recv") == 0 || p_ele->Attribute("from")) 
			{
				rcv_lst[i].tag = tag;
				p_ele->QueryIntAttribute("field", &(rcv_lst[i].fld_no));
				p_ele->QueryIntAttribute("from", &(rcv_lst[i].fld_no));
				p = p_ele->GetText();
				if ( p ) {
					lnn = strlen(p);
					rcv_lst[i].must_con = new unsigned char[lnn+1];
					rcv_lst[i].must_len = BTool::unescape(p, rcv_lst[i].must_con) ;
					rcv_lst[i].err_code = p_ele->Attribute("error");	//接收域若有不符合，设此错误码
				}
				p = p_ele->Attribute("disp");
				if (!p) p = def_ele->Attribute("disp");
				if (!p) p = def_ele->GetDocument()->RootElement()->Attribute("disp");
				if ( p && strcasecmp(p, "hex") == 0 )
					rcv_lst[i].err_disp_hex = true;
					
				i++;
				if ( strcasecmp(tag, "recv") == 0 ) continue;	/* recv 仅在基础报文定义中出现 */

				/* 用户命令和子序列中的返回元素也算上, 当然from已经算一个了 */
				some_ele = pac_ele;
ANOTHER:
				for (e_tmp = some_ele->FirstChildElement(tag); e_tmp; e_tmp = e_tmp->NextSiblingElement(tag) ) 
				{
					p_ele->QueryIntAttribute("from", &(rcv_lst[i].fld_no));	//对第一个有点重复
					rcv_lst[i].tag =tag;
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
				if ( some_ele != usr_ele ) {
					some_ele = usr_ele;
					goto ANOTHER;
				}
			}
		}	/* 结束返回元素的定义*/

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
	const char *pri_key ;	//子系列的primary属性值, 可能为null
	const char *pri_var_nm;	//子系列的primary所指向的变量名或具体内容, 可能为null

	long loop_n;	/* 本子序列循环次数: 0:无限, 直到某种失败, >0: 一定次数, 若失败则中止  */

	ComplexSubSerial() {
		map_root = def_root = usr_ele = 0;

		usr_def_entry = 0;
		sub_pro = 0;

		pac_inses = 0;
		pac_many = 0;
		g_var_set = 0;
		loop_n = 1;
		pri_key = 0;
		pri_var_nm = 0;
	};

	~ComplexSubSerial() {
		if (pac_inses) delete []pac_inses;
		pac_inses = 0;
		pac_many = 0;
	};

	/* 局域变量名根据protect所指向的元素属性名而变, 这样自然, 不用para1,para2之类的。 
		vnm: 是一个变量名，如果在全局表中查不到，则当作变量内容
		mid_num：是Me中指定的,是子元素或primay属性所指定的，局域变量名为: me.mid_num.xx
	*/
	struct PVar *set_loc_ref_var(const char *vnm, const char *mid_nm, bool me_pri=false)
	{
		unsigned char buf[512];		//实际内容, 常数内容
		char loc_v_nm[128];
		TiXmlAttribute *att; 
		unsigned long len;
		struct PVar *ref_var = 0, *att_var=0;
		const char *att_val;
		
		len = 0;
		ref_var = g_var_set->one_still(0,vnm, buf, len);	//找到已定义参考变量的
		if ( len > 0 && me_pri)	{ //找到的全局变量可能有内容，加到本地中。主参考变量才有
			struct PVar *av ;
			TEXTUS_SPRINTF(loc_v_nm, "%s%s", ME_VARIABLE_HEAD, mid_nm); 
			av = sv_set.look(loc_v_nm, 0);
			if ( av) {
				av->put_still(buf, len);
			}
		}
		if ( ref_var) {
			for ( att = ref_var->self_ele->FirstAttribute(); att; att = att->Next())
			{
				//把属性加到本地变量集 sv_set
				TEXTUS_SPRINTF(loc_v_nm, "%s%s.%s", ME_VARIABLE_HEAD, mid_nm, att->Name()); 
				att_val = att->Value();
				if ( !att_val ) continue;
				len = 0;
				/* 这里 att->Value() 可能指向另一个变量名 */
				att_var = g_var_set->one_still(ref_var->self_ele, att_val, buf, len);	//找到已定义的变量
				if ( att_var ) 
				{
					sv_set.put_var(loc_v_nm, att_var);
				} else {
					if ( len > 0 ) 
						sv_set.put_still(loc_v_nm, buf, len);
				}
			}
		}

		return ref_var;
	};

	void ev_pro( TiXmlElement *sub, int &which, int &icc_num)	//为了无限制嵌套
	{
		TiXmlElement *pac_ele, *def_ele, *t_ele;
		for ( pac_ele= sub->FirstChildElement(); pac_ele; pac_ele = pac_ele->NextSiblingElement())
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
				ev_pro(t_ele, which, icc_num);
			}
		}
	};

	void ev_num( TiXmlElement *sub, int &many )	//为了无限制嵌套
	{
		TiXmlElement *pac_ele, *t_ele;
		for ( pac_ele= sub->FirstChildElement(); pac_ele; pac_ele = pac_ele->NextSiblingElement())
		{
			if ( !pac_ele->Value() ) continue;
			if ( def_root->FirstChildElement(pac_ele->Value()))	//如果在基础报文中有定义
				many++;
			else if ((t_ele = map_root->FirstChildElement(pac_ele->Value())))//如果在map中有定义, 也就是一个嵌套(类似于宏)
				ev_num(t_ele, many);
		}
	};

	const char *find_from_usr_ele(const char *me_name)
	{
		const char *nm=0;
		TiXmlElement *body;	//用户命令的第一个body元素
		nm = usr_ele->Attribute(me_name);	//先看属性名为me.XX.yy中的XX名，nm是$Main之的。
		if (!nm )	//属性优先, 没有属性再看元素
		{
			body = usr_ele->FirstChildElement(me_name);	//再看元素为me.XX.yy中的XX名，nm是$Main之的。
			if ( body ) nm = body->GetText();
		}
		if (!nm )	//还是没有, 那看map文件的入口元素
			nm = usr_def_entry->Attribute(me_name);	//nm是$Main之类的。
		return nm;
	}
	int pro_analyze(const char *loop_str)
	{
		struct PVar *ref_var, *me_var;
		const char *nm;
		char pro_nm[128];
		int which, icc_num=0, i;
		struct PVar_Set tmp_sv;		//临时局域变量集
		unsigned long tmplen;

		if ( loop_str ) loop_n = atoi(loop_str);
		if ( loop_n < 0 ) loop_n = 1;
			
		TEXTUS_SNPRINTF(pro_nm, sizeof(pro_nm), "%s", "Pro"); //先假定子序列是Pro element，如果有主参考变量，则找合成的
		if ( pri_var_nm ) //如果有主参考变量, 就即根据这个主参考变量中找到相应的sub_pro, pri_var_nm就是$Main之类的。
		{
			ref_var = g_var_set->one_still(0,pri_var_nm, 0, tmplen);	//找到已定义参考变量的
			if ( ref_var ) {
				if ( (nm = ref_var->self_ele->Attribute("SubPro")) )  { //参考变量的pro属性指示子序列
					TEXTUS_SNPRINTF(pro_nm, sizeof(pro_nm), "%s%s", "Pro", nm);
				}
			}
		}

		sub_pro = usr_def_entry->FirstChildElement(pro_nm);	//定位实际的子系列
		if ( !sub_pro ) return -1; //没有子序列 

		sv_set.dynamic_at = g_var_set->dynamic_at;
		//{int *a =0 ; *a = 0; };
		sv_set.defer_vars(usr_def_entry); 
		g_var_set->dynamic_at = sv_set.dynamic_at ;
		sv_set.command_ele = usr_ele;

		if ( pri_var_nm ) set_loc_ref_var(pri_var_nm, pri_key, true);	//先把主参考量赋值, pri_key是"protect"这样的内容
		for ( i = 0; i < sv_set.many; i++ ) {
			me_var = &(sv_set.vars[i]);
			if ( pri_key && strcmp(me_var->me_name, pri_key) == 0 ) 
			{	//主参考变量标记后，略过
				me_var->me_pri_refer = true;
				continue;	
			} 
			if ( me_var->kind != VAR_Me || !me_var->me_sub_name ) continue;	//只处理有后缀的Me变量
			nm = find_from_usr_ele(me_var->me_name); 	//从用户命令中找, nm是"$Main"之类的
			/* 如果参考量没有相应的属性, 原有带后缀变量内容得以保留, 否则被更新 */
			if (nm ) ref_var = set_loc_ref_var(nm, me_var->me_name); /* 处理me.export.*这样的东西。这里更新局部变量集 */
		}

		pac_many = 0;
		ev_num (sub_pro, pac_many);
		//确定变量数
		if ( pac_many ==0 ) return 0;
		pac_inses = new struct PacIns[pac_many];

		which = 0; icc_num = 0;
		ev_pro ( sub_pro, which, icc_num);
		return icc_num;
	};
};

struct User_Command : public Condition {
	int order;
	struct ComplexSubSerial *complex;
	int comp_num; //一般只有一个，有时需要重试几个

	User_Command () {
		complex =0;
		comp_num = 0;
		order = -9999999;
	};

	~User_Command () {
		if (complex && comp_num == 1 ) delete complex;
		else
		if (complex && comp_num > 1 ) delete[] complex;
		complex =0;
		comp_num = 0;
	};

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
						complex[X].pri_key = pri_nm;			\
						complex[X].map_root = map_root;

				PUT_COMPLEX(0)
				complex->pri_var_nm = usr_ele->Attribute(pri_nm);
				ret_ic = complex->pro_analyze(usr_ele->Attribute("loop"));
			} else {	//一个用户操作，包括几个复合指令的尝试，有一个成功，就算OK
				for( pri = usr_ele->FirstChildElement(pri_nm), comp_num = 0; pri; pri = pri->NextSiblingElement(pri_nm) )
					comp_num++;
				if ( comp_num ==0 ) return -1;
				complex = new struct ComplexSubSerial[comp_num];

				for( pri = usr_ele->FirstChildElement(pri_nm), i = 0; 
					pri; pri = pri->NextSiblingElement(pri_nm) )
				{
					PUT_COMPLEX(i)
					complex[i].pri_var_nm = pri->GetText();
					ret_ic = complex[i].pro_analyze(pri->Attribute("loop")); //多个可选，指令数就计最后一个
					i++;
				}
			}
		} else {
			comp_num = 1;
			complex = new struct ComplexSubSerial;
			PUT_COMPLEX(0)
			complex->pri_var_nm = 0;
			ret_ic = complex->pro_analyze(usr_ele->Attribute("loop"));
		}

		set_condition ( usr_ele, vrset, 0);
		return ret_ic;
	};
};

struct INS_Set {	
	struct User_Command *instructions;
	int many;
	int ic_num;
	struct PacIns *last_pac_ins;
	INS_Set () {
		instructions= 0;
		many = 0;
		ic_num = 0;
		last_pac_ins = 0;
	};

	~INS_Set () {
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

	#define MACRO_SUFFIX "_Macro"
	void ev_maro( TiXmlElement *m_root, struct PVar_Set *var_set, TiXmlElement *map_root, TiXmlElement *pac_def_root, int &vmany, int &mor,int base_cor)	//为了无限制嵌套宏定义
	{
		const char *com_nm;
		char macro_nm[128];
		TiXmlElement *macro_ele, *m_usr_ele, *sub;
		int cor, a_ic_num, nbase;

		for ( m_usr_ele= m_root->FirstChildElement(); m_usr_ele; m_usr_ele = m_usr_ele->NextSiblingElement())
		{
			if ( !m_usr_ele->Attribute("order")) continue;
			if ( (com_nm = m_usr_ele->Value()) ) {
				sub = yes_ins(m_usr_ele, map_root, var_set);
				if ( sub) {
					m_usr_ele->QueryIntAttribute("order", &(cor)); 
					if ( (cor = base_cor + cor ) <= mor ) continue; //order不合序,略过
					a_ic_num = instructions[vmany].set_sub(m_usr_ele, var_set, sub, pac_def_root, map_root);
					if ( a_ic_num < 0 ) continue;
					ic_num += a_ic_num;
					instructions[vmany].order = cor;
					mor = cor;
					vmany++;
				} else if ( !var_set->is_var(com_nm)) { 
					TEXTUS_SNPRINTF(macro_nm, sizeof(macro_nm)-1, "%s"MACRO_SUFFIX, com_nm);
					macro_ele= map_root->FirstChildElement(macro_nm); //map_root中找宏定义
					if ( !macro_ele) continue;
					m_usr_ele->QueryIntAttribute("order", &(cor)); 
					nbase = base_cor + cor;
					if ( nbase <= mor ) continue;	//order不符合顺序的，略过
					ev_maro( macro_ele, var_set, map_root, pac_def_root, vmany, mor, nbase);	//为了无限制嵌套宏定义
				}
			}
		}
	};

	void ev_num( TiXmlElement *m_root, struct PVar_Set *var_set, TiXmlElement *map_root, int &refny )	//为了无限制嵌套宏定义
	{
		const char *com_nm;
		char macro_nm[128];
		TiXmlElement *macro_ele, *m_usr_ele;

		for ( m_usr_ele= m_root->FirstChildElement(); m_usr_ele; m_usr_ele = m_usr_ele->NextSiblingElement()) {
			if ( !m_usr_ele->Attribute("order")) continue;
			if ( (com_nm = m_usr_ele->Value()) ) {
				if (yes_ins(m_usr_ele, map_root, var_set) )
					refny++;				
				else if ( !var_set->is_var(com_nm)) { 
					TEXTUS_SNPRINTF(macro_nm, sizeof(macro_nm)-1, "%s"MACRO_SUFFIX, com_nm);
					macro_ele= map_root->FirstChildElement(macro_nm); //map_root中找宏定义
					if ( !macro_ele) continue;
					ev_num(macro_ele, var_set, map_root, refny);
				}
			}
		}
	};

	void put_inses(TiXmlElement *root, struct PVar_Set *var_set, TiXmlElement *map_root, TiXmlElement *pac_def_root)
	{
		TiXmlElement *usr_ele, *sub, *macro_ele, *m_usr_ele;
		int mor, cor, vmany, refny,i, a_ic_num, base_cor;
		const char *com_nm;
		char macro_nm[128];
		refny = 0; 
		ev_num(root, var_set, map_root, refny);

		//初步确定变量数
		many = refny ;
		if ( many ==0 ) return;
		instructions = new struct User_Command[many];
			
		mor = -999999;	//这样，顺序号可以从负数开始
		ic_num = 0;
		vmany = 0;
		ev_maro(root, var_set, map_root, pac_def_root, vmany, mor, 0);	//为了无限制嵌套宏定义

		many = vmany; //最后再更新一次用户命令数
		//look for last_pac_ins
		for ( i = 0 ; i < many; i++)
		{
			int j;
			for ( j = 0; j < instructions[i].comp_num; j++ )
			{
				int k;
				for ( k = 0 ; k < instructions[i].complex[j].pac_many; k++ )
				{
					last_pac_ins = &(instructions[i].complex[j].pac_inses[k]);
					if ( last_pac_ins->type != INS_ResultPacPro)
						last_pac_ins = 0;
				}
			}
		}
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
		if ( !f_name || strlen(f_name) ==0 ) return 0;
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

	bool put_def( TiXmlElement *per_ele, TiXmlElement *prop)
	{
		const char *ic_nm, *nm;
		if ( !per_ele) return false;

#define GET_XML_DEF(ROOT, DEF_DOC, LOC_ELE, ATTR_NAME_FILE, MD5_ATTR) \
		if ( !(ROOT = per_ele->FirstChildElement(LOC_ELE)))	\
		{								\
			if ( (nm = per_ele->Attribute(ATTR_NAME_FILE)))		\
				load_xml(nm, DEF_DOC,  ROOT, per_ele->Attribute(MD5_ATTR));	\
			else if ( !(ROOT = prop->FirstChildElement(LOC_ELE)))				\
			{											\
				if ( (nm = prop->Attribute(ATTR_NAME_FILE)))						\
					load_xml(nm, DEF_DOC,  ROOT, prop->Attribute(MD5_ATTR));	\
			}												\
		}

		if ( !(c_root = per_ele->FirstChildElement("Flow")))
		{
			nm =  per_ele->Attribute("md5");
			if ( (ic_nm = per_ele->Attribute("flow")))
				load_xml(ic_nm, doc_c,  c_root, nm);
			if ( nm)
				squeeze(nm, (unsigned char*)&flow_md[0]);
		}
		GET_XML_DEF(pac_def_root, doc_pac_def, "Pac",  "pac", "pac_md5")
		GET_XML_DEF(k_root, doc_k, "Key",  "key", "key_md5")
		GET_XML_DEF(v_root, doc_v, "Var",  "var", "var_md5")
		if ( !c_root || !k_root || !pac_def_root ) return false;
		person_vars.defer_vars(k_root, c_root);	//变量定义, map文件优先
		flow_id = c_root->Attribute("flow");
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

	PersonDef_Set () {
		icp_def = 0;
		num_icp = 0;
		max_snap_num = 0;
	};

	~PersonDef_Set () {
		if (icp_def ) delete []icp_def;
		icp_def = 0;
		num_icp = 0;
	};

	void put_def(TiXmlElement *prop, const char *vn)	//个人化集合输入定义PersonDef_Set
	{
		TiXmlElement *per_ele;
		int kk;
		num_icp = 0; 
		for (per_ele = prop->FirstChildElement(vn); per_ele; per_ele = per_ele->NextSiblingElement(vn) ) 
			num_icp ++; 

		icp_def = new struct Personal_Def [num_icp];
		for (per_ele = prop->FirstChildElement(vn), kk = 0; per_ele;per_ele = per_ele->NextSiblingElement(vn))
		{
			if ( icp_def[kk].put_def(per_ele, prop))
			{
				if ( icp_def[kk].person_vars.dynamic_at > max_snap_num ) max_snap_num = icp_def[kk].person_vars.dynamic_at;
				kk++;
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
	void mk_hand(bool right_down=false);	//还有右边状态处理
	void mk_result(bool end_mess=true);

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
	PacketObj *hi_req_p, *hi_reply_p; /* 向右传递的, 可能是对HMS, 或是对IC终端 */
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
		PAC_STEP pac_step;
		long sub_loop;	//循环次数
	} command_wt;

	int sub_serial_pro(struct ComplexSubSerial *comp, bool &has_back,  Amor::Pius *&fac_ps, struct PacIns *last_paci=0);
	void set_peer(PacketObj *pac, int sub);
	void get_cert(PacketObj *pac, int sub);
	void get_peer(PacketObj *pac, int sub);
	Amor::Pius prodb_ps;
	#include "wlog.h"
};

void PacWay::ignite(TiXmlElement *prop)
{
	const char *comm_str;
	if (!prop) return;
	if ( !gCFG ) {
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

PacWay::PacWay() {
	hipa[0] = &hi_req;
	hipa[1] = &hi_reply;
	hipa[2] = 0;
	hi_req_p = &hi_req;
	hi_reply_p = &hi_reply;

	gCFG = 0;
	has_config = false;
	loc_pro_pac.ordo = Notitia::PRO_UNIPAC;
	loc_pro_pac.indic = 0;
	loc_pro_pac.subor = -1;

	prodb_ps.ordo =  Notitia::PRO_DBFACE;
	prodb_ps.indic = 0;
	prodb_ps.subor = -1;
}

PacWay::~PacWay() {
	if ( has_config  )
	{	
		if(gCFG) delete gCFG;
		gCFG = 0;
	}
}

Amor* PacWay::clone() {
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
		gCFG->null_icp_def.put_def(gCFG->prop->FirstChildElement("bike"), gCFG->prop);
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
		mess.reset(false);	//动态量硬复位
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION" );
		mess.reset(false);	//动态量硬复位
		break;

	default:
		return false;
	}
	return true;
}

bool PacWay::sponte( Amor::Pius *pius)
{
	PacketObj **tmp;
	assert(pius);
	if (!gCFG ) return false;

	switch ( pius->ordo )
	{
	case Notitia::SET_UNIPAC:
		WBUG("sponte SET_UNIPAC");
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) hi_req_p = *tmp; 
			else {
				WLOG(WARNING, "sponte SET_UNIPAC rcv_pac null");
			}
			tmp++;
			if ( *tmp) hi_reply_p = *tmp;
			else {
				WLOG(WARNING, "sponte SET_UNIPAC snd_pac null");
			}
		} else 
			WLOG(WARNING, "sponte SET_UNIPAC null");

		break;
	case Notitia::MULTI_UNIPAC_END:
		WBUG("sponte MULTI_UNIPAC_END");
		break;

	case Notitia::PRO_UNIPAC:
		WBUG("sponte PRO_UNIPAC");
		if ( mess.right_status != RT_OUT || mess.right_subor != pius->subor  )	//表明是右端返回
		{
			const char *r_str;
			switch (mess.right_status)
			{
			case RT_OUT:
				r_str = "RT_OUT";
				break;
			case RT_IDLE:
				r_str = "RT_IDLE";
				break;
			case RT_READY:
				r_str = "RT_READY";
				break;
			}
			WLOG(WARNING, "mess error right_status=%s right_subor=%d pius->subor=%d", r_str, mess.right_subor, pius->subor);
		} else 
			mk_hand();
		break;

	case Notitia::DMD_END_SESSION:	//右节点关闭, 要处理
		WBUG("sponte DMD_END_SESSION");
		if ( mess.left_status == LT_Working && mess.right_status == RT_OUT)	//表明是制卡工作
		{
			mess.iRet = ERROR_DEVICE_DOWN;
			TEXTUS_SPRINTF(mess.err_str, "device down at subor=%d", pius->subor);
			mk_hand(true);
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
		if (alen >= sizeof(mess.flow_id) ) alen = sizeof(mess.flow_id)-1 ;
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
				if (dvr->c_len > 0) continue; //如果动态量没有清空, 就不再赋新值.这个量跨flow, 直到Notitia:START_SESSION,END_SESSION
				dvr->kind = vt->kind;
				dvr->def_var = vt;
				if ( vt->c_len > 0 )	//先把定义的静态内容链接过来, 动态变量的默认值
					dvr->input(&(vt->content[0]), vt->c_len, true);	
			}
		}
		
		//{int *a =0 ; *a = 0; };
		if (mess.iRet == ERROR_INS_DEF)
		{
			TEXTUS_SPRINTF(mess.err_str, "not defined flow_id: %s ", mess.flow_id );
			mess.snap[Pos_ErrCode].input(mess.iRet);
			mk_result();	//工作结束
			goto HERE_END;
		}
		/* 任务开始  */
		mess.snap[Pos_TotalIns].input( cur_def->ins_all.ic_num);
		if ( cur_def->flow_md[0] )
			mess.snap[Pos_FlowPrint].input( cur_def->flow_md);
		mess.snap[Pos_FlowID].input(mess.flow_id);

		mess.ins_which = 0;
		mess.left_status = LT_Working;
		mess.right_status = RT_READY;	//指示终端准备开始工作,

		//if ( strcmp(mess.flow_id, "22TY") ==0 ) {int *a =0 ; *a = 0; };
		if ( !cur_def->ins_all.instructions )
		{
			mk_result();
			goto HERE_END;
		}
			
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

void PacWay::get_cert(PacketObj *pac, int sub)
{
	Amor::Pius peer_ps;

	/* 外部配置文件中, 必须设置第1域 */
	peer_ps.ordo = Notitia::CMD_GET_CERT_NO;
	peer_ps.subor = sub;
	peer_ps.indic = 0;
	aptus->facio(&peer_ps);
	if (peer_ps.indic)
	{
		pac->input(1, (unsigned char*)peer_ps.indic, strlen((const char*)peer_ps.indic));
	}
}

void PacWay::set_peer(PacketObj *pac, int sub)
{
	TiXmlElement peer_xml("peer");
	Amor::Pius peer_ps;
	char ip[64], port[32];
	unsigned char *p;
	unsigned long rlen;

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

void PacWay::get_peer(PacketObj *pac, int sub)
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
	if ( peer )
	{
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

/* 子序列入口 */
int PacWay::sub_serial_pro(struct ComplexSubSerial *comp, bool &has_back,  Amor::Pius *&fac_ps, struct PacIns *last_paci)
{
	struct PacIns *paci;
	char h_msg[1024];
	if ( last_paci ) 
	{
		paci=last_paci;
		goto PACI_PRO;
	}
SUB_INS_PRO:
	paci = &(comp->pac_inses[command_wt.pac_which]);
	if ( command_wt.pac_step == Pac_Idle )
	{
		if ( !paci->valid_condition(&mess) )		/* 不符合条件,就转下一条 */
		{
			command_wt.pac_which++;	//指向下一条报文处理
			if (  command_wt.pac_which == comp->pac_many )
				return 1;	//整个已经完成
			 else
				goto SUB_INS_PRO;
		}
	}

PACI_PRO:
	has_back = false;
	fac_ps = 0;
	switch ( paci->type)
	{
	case INS_Normal:
	case INS_Pro_DBFace:
		switch ( command_wt.pac_step )
		{
		case Pac_Idle:
			hi_req_p->reset();	//请求复位
			paci->pac_cross_before(hi_req_p, hi_reply_p, rcv_pac, snd_pac);
			paci->get_req_pac(hi_req_p, loc_pro_pac.subor, &mess);
			if (  paci->subor < 0 ) 	//仅仅是报文域赋值
			{
				command_wt.pac_step = Pac_End;
				goto END_PRO;
			}
			command_wt.pac_step = Pac_Working;

			if ( paci->isFunction || paci->rcv_num == 0 ) 
				has_back = true;
			else
				has_back = false;

			mess.right_subor = paci->subor;
			WBUG("will mess.pro_order=%d command_wt.pac_which=%d has_back(%s)", mess.pro_order, command_wt.pac_which, has_back ? "yes":"no");
			//if ( mess.pro_order == 2 &&  command_wt.pac_which ==1 ) {int *a=0; *a=0;}
			if ( paci->type == INS_Normal )
				fac_ps = &loc_pro_pac;
			else if ( paci->type == INS_Pro_DBFace) 
			{	//DB操作
				if (paci->dbface == 0 ) 
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
					if (paci->dbface == 0 ) 
					{
						mess.iRet = ERROR_DB_DEF;
						TEXTUS_SPRINTF(mess.err_str, "no dbface error at %d of %s", mess.pro_order, cur_def->flow_id);
						if ( paci->err_code) mess.snap[Pos_ErrCode].input(paci->err_code);
						return -2;
					}
				}
				prodb_ps.indic = paci->dbface;
				prodb_ps.subor = paci->subor;
				fac_ps = &prodb_ps;
			}
			return 0; 	/* 正在进行 */
			break;
		case Pac_Working:
			//if ( mess.pro_order == 49 &&  command_wt.pac_which == 1 ) {int *a=0; *a=0;}
			command_wt.pac_step = Pac_End;
			break;

		default:
			break;
		}
		break;

	case INS_Abort:
		if (paci->err_code ) mess.snap[Pos_ErrCode].input(paci->err_code);
		command_wt.pac_step = Pac_End;
		return  -1;	//脚本所控制的错误, 软失败
		break;

	case INS_Respond:
		mk_result(false);		//回应前端, 但整个业务不结束
		command_wt.pac_step = Pac_End;
		break;

	case INS_SetPeer:
		hi_req_p->reset();
		paci->pac_cross_before(hi_req_p, hi_reply_p, rcv_pac, snd_pac);
		paci->get_req_pac(hi_req_p, loc_pro_pac.subor, &mess);
		set_peer(hi_req_p, loc_pro_pac.subor);
		command_wt.pac_step = Pac_End;
		break;

	case INS_GetPeer:
		hi_reply_p->reset();
		get_peer(hi_reply_p, paci->subor);
		command_wt.pac_step = Pac_End;
		break;

	case INS_Cmd_Ordo:
		command_wt.pac_step = Pac_End;
		break;

	case INS_Get_CertNo:
		hi_reply_p->reset();
		get_cert(hi_reply_p, paci->subor);
		command_wt.pac_step = Pac_End;
		break;

	case INS_ResultPacPro:
		switch ( command_wt.pac_step )
		{
		case Pac_Idle:
			hi_req_p->reset();	//请求复位
			paci->pac_cross_before(hi_req_p, hi_reply_p, rcv_pac, snd_pac);
			paci->get_req_pac(hi_req_p, loc_pro_pac.subor, &mess);
			mess.handle_last_pac = true;
			if (  paci->subor < 0 ) 	//仅仅是报文域赋值
			{
				command_wt.pac_step = Pac_End;
				paci->pac_cross_after(hi_req_p, hi_reply_p, rcv_pac, snd_pac);
				return 1;	//完成
			}

			command_wt.pac_step = Pac_Working;
			has_back = true;
			mess.right_subor = paci->subor;
			WBUG("mk_result pac_pro has_back(%s)", has_back ? "yes":"no");
			fac_ps = &loc_pro_pac;
			return 0; 	/* 正在进行 */
			break;
		case Pac_Working:
			command_wt.pac_step = Pac_End;
			paci->pac_cross_after(hi_req_p, hi_reply_p, rcv_pac, snd_pac);
			return 1;	//完成
			break;

		default:
			break;
		}
		break;

	case INS_Null:
		command_wt.pac_step =Pac_End;
		break;

	default :
		break;
	}
END_PRO:
	if ( command_wt.pac_step == Pac_End )
	{
		PacketObj *n_pac = paci->pac_cross_after(hi_req_p, hi_reply_p, rcv_pac, snd_pac);
		if ( paci->rcv_num > 0 && !paci->pro_rply_pac(n_pac, &mess)) 
		{
			mess.iRet = ERROR_RECV_PAC;
			TEXTUS_SPRINTF(h_msg, "fault at order=%d pac_which=%d of %s (%s)", mess.pro_order, command_wt.pac_which, cur_def->flow_id, mess.err_str);
			memcpy(mess.err_str, h_msg, strlen(h_msg));
			mess.err_str[strlen(h_msg)] = 0;
			return -3;	//这是基本报文错误，非map所控制
		}  else if ( !paci->valid_result(&mess) )
		{
			mess.iRet = ERROR_RESULT;
			TEXTUS_SPRINTF(mess.err_str, "result error at order=%d pac_which=%d of %s", mess.pro_order, command_wt.pac_which, cur_def->flow_id);
			if ( paci->err_code) mess.snap[Pos_ErrCode].input(paci->err_code);
			return -2;	//这是map所控制
		} else {
			command_wt.pac_which++;	//指向下一条报文处理
			command_wt.pac_step = Pac_Idle;
			if (  command_wt.pac_which == comp->pac_many )
				return 1;	//整个已经完成
			else
				goto SUB_INS_PRO;
		}
	}
	return 0;
}

void PacWay::mk_hand(bool right_down)
{
	struct User_Command *usr_com;
	struct PacIns *paci;
	int i_ret;
	Amor::Pius *fac_ps;
	bool has_back;
	struct PVar  *vt;

#define NEXT_INS	\
		mess.ins_which++;	\
		if ( mess.ins_which < cur_def->ins_all.many)	\
			goto INS_PRO;		\
		else 				\
			mk_result();		

INS_PRO:
	usr_com = &(cur_def->ins_all.instructions[mess.ins_which]);
	mess.pro_order = usr_com->order;	
	if ( right_down )
	{
		paci = &(usr_com->complex[command_wt.cur].pac_inses[command_wt.pac_which]);
		if ( paci->err_code) mess.snap[Pos_ErrCode].input(paci->err_code);
		mk_result();	//结束
		return;
	}
	if ( mess.snap[Pos_CurCent].def_var) 
		mess.snap[Pos_CurCent].input((mess.ins_which*100)/cur_def->ins_all.many);

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
			mess.right_status = RT_READY;
			NEXT_INS
			break;
		}
		command_wt.cur = 0;
		if ( usr_com->comp_num == 1 )
			mess.willLast = true;
		else 
			mess.willLast = false; //一个用户操作，包括几个复合指令的尝试，有一个成功，就算OK
NEXT_PRI_TRY:
		command_wt.sub_loop = usr_com->complex[command_wt.cur].loop_n; //软失败的重试次数
LOOP_PRI_TRY:
		command_wt.pac_which = 0;	//新子系列, pac从第0个开始
		command_wt.pac_step = Pac_Idle;	//pac处理开始, 
		command_wt.step++;	//指向下一步
	case 1:
		i_ret = sub_serial_pro( &(usr_com->complex[command_wt.cur]), has_back, fac_ps );
		if ( i_ret == -1 ) 	//这是软失败
		{
			command_wt.sub_loop--;
			if ( command_wt.sub_loop != 0 ) 	//如果原为是0,则为负,几乎到不了0
			{
				command_wt.sub_loop--;
				command_wt.step--;
				goto LOOP_PRI_TRY;
			}
			if ( command_wt.cur < (usr_com->comp_num-1) )	//用户定义的Abort才试下一个
			{
				command_wt.cur++;
				command_wt.step--;
				if ( command_wt.cur == (usr_com->comp_num-1) ) //最后一条复合指令啦，如果出错就调用自定义的出错过程(响应报文设置一些数据，或者向终端发些指令)
					mess.willLast = true;
				vt =  mess.snap[Pos_ErrCode].def_var;
				if ( vt ) mess.snap[Pos_ErrCode].input(&(vt->content[0]), vt->c_len, true);	
				goto NEXT_PRI_TRY;		//试另一个
			} else {		//最后一条处理失败，定义出错值
				mess.iRet = ERROR_USER_ABORT;			
				TEXTUS_SPRINTF(mess.err_str, "user abort at %d of %s", mess.pro_order, cur_def->flow_id);
				mk_result();		/* 结束 */
			}
		} else if ( i_ret ==0  ) 
		{
			mess.right_status = RT_OUT;
			if ( has_back ) 
			{
				aptus->facio(fac_ps);     //向右发出指令, 右节点不再sponte
				if ( mess.right_status == RT_OUT ) 
					goto INS_PRO;		//本指令处理结果. 但是右节点dmd_end_session导致复位, 就不再处理
			} else {
				aptus->facio(fac_ps);     //向右发出指令, aptus.facio的处理放在最后, 很重要!!。因为可能在这个调用中就收到右节点的sponte. 注意!!. 以后的工作中一定要注意.
			}
		} else 	if ( i_ret > 0  ) 
		{
			mess.right_status = RT_READY;	//右端闲
			WBUG("has completed %d, order %d", mess.ins_which, mess.pro_order);
			NEXT_INS
		} else if ( i_ret < 0 )  //脚本控制或报文定义 的 错误
		{
			mk_result(); 
		}
		break;
	}
}

void PacWay::mk_result(bool end_mess)
{
	Amor::Pius *fac_ps;
	bool has_back;

	if ( mess.iRet != 0 ) 
	{
		WLOG(WARNING, "Error %s:  %s", mess.snap[Pos_ErrCode].val_p, mess.err_str);
		mess.snap[Pos_ErrStr].input(mess.err_str);
	}

	if ( !mess.handle_last_pac ) 
	{
		if ( cur_def->ins_all.last_pac_ins ) 
		{
			int ret;
			command_wt.pac_step = Pac_Idle;	//pac处理开始, 
			ret = sub_serial_pro( 0, has_back, fac_ps, cur_def->ins_all.last_pac_ins);
			mess.right_status = RT_OUT;
			if ( ret == 0  )
			{
				aptus->facio(fac_ps);     //向右发出指令, 右节点不再sponte
				ret = sub_serial_pro( 0, has_back, fac_ps, cur_def->ins_all.last_pac_ins);
			}
			if ( ret == 0  )
				WLOG(EMERG, "bug! last_pac_pro should finished!");
		}
	}
	//{int *a=0; *a=0;}
	aptus->sponte(&loc_pro_pac);    //制卡的结果回应给控制台
	if (end_mess) mess.reset();
}
#include "hook.c"
