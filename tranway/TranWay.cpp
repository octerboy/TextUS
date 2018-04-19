/* Copyright (c) 2016-2018 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 Title:TranWay
 Build: created by octerboy, 2018/03/14 Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "BTool.h"
#include "TBuffer.h"
#include "casecmp.h"
#include "textus_string.h"
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

int squeeze(const char *p, unsigned char *q)	//把空格等挤掉, 只留下16进制字符(大写), 返回实际的长度
{
	int i;
	i = 0;
	while ( *p ) { 
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

char err_global_str[128]={0};
/* 右边状态, 空闲, 发出报文等响应 */
enum RIGHT_STATUS { RT_IDLE = 0, RT_OUT=7};
enum TRAN_STEP {Tran_Idle = 0, Tran_Working = 1, Tran_End=2};
enum SUB_RET {Sub_Working = 0, Sub_OK = 1, Sub_Rcv_Pac_Fail=-2, Sub_Soft_Fail=-1, Sub_Valid_Fail = -3};

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

enum TranIns_Type { INS_None = 0, INS_Normal=1, INS_Abort=2, INS_Respond =8, INS_LetVar=10, INS_Null=99};
enum Var_Type {VAR_ErrCode=1, VAR_FlowPrint=2, VAR_TotalIns = 3, VAR_CurOrder=4, VAR_CurCent=5, VAR_ErrStr=6, VAR_FlowID=7, VAR_Dynamic_Global = 8, VAR_Dynamic_Link = 9, VAR_Dynamic = 10, VAR_Me=12, VAR_Constant=98,  VAR_None=99};
struct PVar {
	Var_Type kind;
	const char *name;	//来自于doc文档
	int n_len;		//名称长度

	char me_name[64];	//Me变量名称，除去开头的 me. 三个字节, 不包括后缀. 从变量名name中复制，最大63字符
	unsigned int me_nm_len;
	const char *me_sub_name;  //Me变量后缀名， 从变量名name中定位。
	int me_sub_nm_len;
	bool me_had_var;	//主参考变量标记, true: 在合成请报文(hard_work_2)时, 不再从用户ele中取，因为在分析sub_serial时, 已经赋值
	TBuffer *con;		//外部可访问, 通常指向nal, Global时指向G_CFG中的变量
	TBuffer nal;		//内部数据, 

	bool keep_alive;	//true: 若是动态变量, 只在Notitia::START_SESSION、DMD_END_SESSION时清空, false: 对每个flow_id开始都清空,
	int dynamic_pos;	//动态变量位置, -1表示静态
	TiXmlElement *self_ele;	/* 自身, 其子元素包括两种可能: 1.函数变量表, 
				2.一个指令序列, 在指令子元素分析时, 如发现一个用到的变量中, 有子序列时, 把这些指令嵌入。 */
	PVar () {
		kind = VAR_None;	//起初认为非变量
		name = 0;
		n_len = 0;
		con = &nal;
		dynamic_pos = -1;	//非动态类
		keep_alive = false;

		self_ele = 0;
		memset(me_name, 0, sizeof(me_name));
		me_nm_len = 0;
		me_sub_name = 0;
		me_sub_nm_len = 0;
		me_had_var = false;
	};

	void put_still(const unsigned char *val, size_t len=0)
	{
		size_t c_len;
		if ( !val)
			return ;
		if ( len ==0 ) 
			c_len = strlen((char*)val);
		else
			c_len = len;
			
		nal.reset();
		nal.grant(c_len+1);
		nal.input((unsigned char*)val, c_len);
		nal.point[0] = 0;	//保证null结尾
		kind = VAR_Constant;	//认为是常数
		dynamic_pos = -1;
		con = &nal;	
	};

	void put_var(struct PVar* att_var) {
		kind = att_var->kind;
		nal.reset();
		nal.input(att_var->nal.base, att_var->nal.point-att_var->nal.base);
		dynamic_pos = att_var->dynamic_pos;
	};

	struct PVar* prepare(TiXmlElement *var_ele, int &dy_at) //变量准备
	{
		const char *p, *nm, *aval;
		size_t c_len = 0;
		kind = VAR_None;
		self_ele = var_ele;

		nm = var_ele->Attribute("name");
		if ( !nm ) return 0;

		name = nm;
		n_len = strlen(name);

		p = var_ele->GetText();
		if ( p) {
			nal.reset();
			nal.grant(strlen(p)+1);
			aval = var_ele->Attribute("escape");
			if ( !aval ) 
				aval = var_ele->GetDocument()->RootElement()->Attribute("escape");
			if ( aval && (aval[0] == 'Y' || aval[0] == 'y') )
				c_len = BTool::unescape(p, nal.base) ;
			else 
				c_len = squeeze(p, nal.base);
			nal.commit(c_len);
			nal.point[0] = 0;	//保证null结尾
		}

		if ( strcasecmp(nm, "$ink" ) == 0 ) //当前用户命令集脚印
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

		if ( strcasecmp(nm, "$FlowID" ) == 0 ) //业务流标识
		{
			dynamic_pos = Pos_FlowID;
			kind = VAR_FlowID;
		}
		if ( kind != VAR_None) goto P_RET; //已有定义，不再看这个Dynamic, 以上定义都与Dynamic相同处理
		if ( (p = var_ele->Attribute("dynamic")) )
		{
			if ( strcasecmp(p, "global" ) == 0 || *p == 'G' || *p== 'g'  )
				kind = VAR_Dynamic_Global;

			if ( strcasecmp(p, "link" ) == 0 || *p == 'L' || *p== 'l' ) 
				kind = VAR_Dynamic_Link;

			if ( strcasecmp(p, "copy" ) == 0 || *p == 'Y' || *p == 'y' || *p == 'C' || *p== 'c'  ) 
				kind = VAR_Dynamic;

			dynamic_pos = dy_at;	//动态变量位置
			dy_at++;
			if ( (p = var_ele->Attribute("alive")) && (*p == 'Y' || *p == 'y') )
				keep_alive = true;
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
			} else 		//如果不存在后缀
				me_nm_len = strlen(&nm[sizeof(ME_VARIABLE_HEAD)-1]);

			if ( me_nm_len >= sizeof ( me_name))	//Me变量名空间有限, 64字节最大。
				me_nm_len = sizeof ( me_name)-1;
			memcpy(me_name, &nm[sizeof(ME_VARIABLE_HEAD)-1], me_nm_len);
			me_name[me_nm_len] = 0 ;
		}

		if ( kind == VAR_None && c_len > 0) //这里有内容但还没有类型, 那就定为常数, 其它的也可以有内容, 就要别处定义了
			kind = VAR_Constant;	//认为是常数

		P_RET:
		return this;
	};

	void put_herev(TiXmlElement *h_ele) //分析一下本地变量
	{
		const char *p, *esc;
		size_t c_len = 0;

		if( (p = h_ele->GetText()) ) {
			nal.reset();
			nal.grant(strlen(p)+1);
			esc = h_ele->Attribute("escape");
			if ( !esc ) 
				esc = h_ele->GetDocument()->RootElement()->Attribute("escape");
			if ( esc && (esc[0] == 'Y' || esc[0] == 'y') )
				c_len = BTool::unescape(p, nal.base) ;
			else 
				c_len = squeeze(p, nal.base);
			nal.commit(c_len);
			nal.point[0] = 0;	//保证null结尾
			if (kind == VAR_None)	//只有原来没有定义类型的, 这里才定成常数. 
				kind = VAR_Constant;
		}
	};
};

struct DyVar:public DyVarBase { /* 动态变量， 包括来自报文的 */
	Var_Type kind;  //动态类型,
	struct PVar *def_var;	//文件中定义的变量

	DyVar () {
		kind = VAR_None;
		index = - 1;
		to_blank();
	};

	void input(int iv) {
		if ( !def_var ) return;
		con->reset();
		con->grant(64);
		val_p = con->base;
		TEXTUS_SNPRINTF((char*)val_p, 64, "%d", iv);
		c_len = strlen((char*)val_p);
		con->commit(c_len);
		con->point[0] = 0;
	};

	void input(const char *p, bool link=false) {
		if ( !def_var || !p) return;
		DyVarBase::input((unsigned char*)p, strlen(p), link);
	};

	void input(const char p) {
		if ( !def_var ) return;
		con->reset();
		con->base[0] = p;
		con->base[1] = 0;
		con->commit(1);
		val_p = con->base;
		c_len = 1;
	};

	void to_blank() {
		c_len = 0;
		val_p = 0;
		def_var = 0;
		con = &nal;	//con有可能指向一个全局的,而不是本地的nal
		def_link = false;
	}
};

struct MK_Session {		//记录一个事务过程中的各种临时数据
	struct DyVar *snap;	//随时的变量, 包括
	struct DyVarBase **psnap;	//指向snap
	int snap_num;
	bool willLast;		//最后一次试错. 通常，一个用户指令只尝试一次错, 此值为true。 有时有多次尝试, 此值先为false，最后一次为true.

	char err_str[1024];	//错误信息
	char flow_id[64];
	int pro_order;		//当前处理的操作序号

	RIGHT_STATUS right_status;
	int right_subor;	//指示向右发出时的subor, 返回时核对。
	int ins_which;	//已经工作在哪个命令, 即为定义中数组的下标值

	inline MK_Session () {
		snap=0;
		snap_num = 0;
		psnap = 0;
	};

	inline void  reset(bool soft=true) {
		int i;
		for ( i = 0; i < snap_num; i++) {
			if ( !soft || !snap[i].def_var || (snap[i].def_var && !snap[i].def_var->keep_alive) )
			{
				snap[i].to_blank();
				if ( i >= Pos_Fixed_Next )   //这个Pos_Fixed_Next很重要, 要不然, 那些固有的动态变量会没有的
					snap[i].kind = VAR_None;
			}
		}
		right_status = RT_IDLE;
		right_subor = Amor::CAN_ALL;
		ins_which = -1;
		err_str[0] = 0;	
		flow_id[0] = 0;
		willLast = true;
	};

	inline void init(int m_snap_num) //这个m_snap_num来自各XML定义的最大动态变量数
	{
		int i;
		if ( snap ) return ;
		if ( m_snap_num <=0 ) return ;

		snap_num = m_snap_num;
		snap = new struct DyVar[snap_num];
		psnap = new struct DyVarBase*[snap_num];
		for ( i = 0 ; i < snap_num; i++)
			psnap[i] = &snap[i];
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
		if ( psnap ) delete[] psnap;
		snap = 0;
		psnap = 0;
	};
};

struct PVar_Set {	/* 变量集合*/
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
	
#define IS_VAR(X) (X != 0 && strcasecmp(X, VARIABLE_TAG_NAME) == 0 ) 
	
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
			if ( !had_nm )	{//如果没有已定义的名, 才增加
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
		if ( loc_v ) {
			rvar = loc_v->look(n);
			if ( rvar )
				return rvar;
		}
	
		d_len = strlen(n);
		for ( int i = 0 ; i < many; i++) {
			if ( d_len == vars[i].n_len) {
				if (memcmp(vars[i].name, n, d_len) == 0 ) 
					return &(vars[i]);
			}
		}
		return 0;
	};
	
	void put_still(const char *nm, const unsigned char *val, size_t len=0)
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
	struct PVar *one_still(TiXmlElement *comp, const char *nm, unsigned char *buf, size_t &len, struct PVar_Set *loc_v=0)
	{
		struct PVar  *vt;
		const char *esc = 0;
		/* 在这时两种情况处理, 一个是有静态常数定义的, 另一个静态常数变量 */
		vt = look(nm, loc_v);	//看看是否为一个定义的变量名
		if ( !vt) {
			if ( comp ) {
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
		if ( vt->kind ==  VAR_Constant ) {	//静态常数变量
			len = vt->con->point - vt->con->base;
			if ( buf && len > 0 ) memcpy(buf, vt->con->base, len);
		} else 
			len = 0;
		VARET:
		if ( buf ) buf[len] = 0;	//结束NULL
		return vt;
	};

	/* nxt 下一个变量, 对于多个tag元素，将之静态内容合成到 一个变量command中。对于非静态的，返回该tag元素是个动态变量 */
	struct PVar *all_still( TiXmlElement *ele, const char*tag, unsigned char *command, size_t &ac_len, TiXmlElement *&nxt, struct PVar_Set *loc_v=0)
	{
		TiXmlElement *comp;
		size_t l;
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
		if (command) command[ac_len] = 0;	//结束NULL
		nxt = comp;	//指示下一个变量
		return rt;
	};
};

/* 下面这段匹配应该是不需要变的 */
struct MatchDst {	//匹配目标
	struct PVar *dst;
	const char *con_dst;
	size_t len_dst;
	bool c_case;	/* 是否区分大小写, 默认为是 */
	MatchDst () {
		dst = 0;
		con_dst = 0;
		len_dst = 0;
		c_case = true;
	};

	bool set_val(struct PVar_Set *var_set, struct PVar_Set *loc_v, const char *p, const char *case_str) {
		bool ret = false;

		if ( case_str && ( case_str[0] == 'N' ||  case_str[0] == 'n') )
			c_case= false;
		else
			c_case = true;
		dst = var_set->look(p, loc_v);
		if (!dst ) {
			if ( p) { //变量名可能没有内容, 这里有内容是常数了, 其它的也可以有内容, 就要别处定义了
				con_dst = p;
				len_dst = strlen(con_dst);
				ret = true;
			}
		} 
		return ret;
	};

	bool valid_val (MK_Session *sess, struct PVar *src) {
		bool ret=true;

		struct DyVar *dvr;
		const unsigned char *src_con, *dst_con;
		size_t src_len, dst_len;

		if ( src->dynamic_pos >= 0 ) {
			dvr = &sess->snap[src->dynamic_pos];
			src_con = dvr->val_p;
			src_len = dvr->c_len;
		} else {
			src_con = src->con->base;
			src_len = src->con->point - src->con->base;
		}
		if ( dst ) {
			if ( dst->dynamic_pos >= 0 ) {
				dvr = &sess->snap[dst->dynamic_pos];
				dst_con = dvr->val_p;
				dst_len = dvr->c_len;
			} else {
				dst_con = dst->con->base;
				dst_len = dst->con->point - dst->con->base;
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

	struct Match* set_ma(TiXmlElement *mch_ele, struct PVar_Set *var_set, struct PVar_Set *loc_v=0) {
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

		if ( dst_num == 0 ) {//一个子value元素也没有
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

	bool will_match(MK_Session *sess) {
		int i;
		bool ret=false;	//假定没有一个是符合的
		for(i = 0; i < dst_num; i++) {
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

	void set_list( TiXmlElement *ele, struct PVar_Set *var_set, struct PVar_Set *loc_v, const char *vn, const char *vn_not, int &m_num, struct Match *&list) {
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

	void set_condition ( TiXmlElement *ele, struct PVar_Set *var_set, struct PVar_Set *loc_v) {
		const char *c;
		hasLastWill = false;
		c = ele->Attribute("has_last_will");
		if ( c && ( c[0] == 'Y' || c[0] == 'y' ) ) 
			hasLastWill = true;
			
		set_list(ele, var_set, loc_v, "if", "if_not", con_num, conie_list);
		set_list(ele, var_set, loc_v, "must", "must_not", res_num, result_list);
	};
		
	bool valid_list(MK_Session *sess, struct Match *list, int l_num) {
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

	bool valid_condition (MK_Session *sess) {
		if (hasLastWill && !sess->willLast ) return false;
		return valid_list(sess, conie_list, con_num);
	};

	bool valid_result (MK_Session *sess) {
		return valid_list(sess, result_list, res_num);
	};
};

struct ComplexSubSerial;
struct TranIns:public InsData  
{
	TranIns_Type type;
	struct Condition condition;
	void set_condition ( TiXmlElement *ele, struct PVar_Set *var_set, struct PVar_Set *loc_v) {
		return condition.set_condition(ele, var_set,loc_v);
	}
	bool valid_condition (MK_Session *sess) {
		return condition.valid_condition(sess);
	}
	bool valid_result (MK_Session *sess) {
		return condition.valid_result(sess);
	}
	TranIns()
	{
		type = INS_None;
	};
	void hard_work_2 ( struct CmdSnd *aSnd, TiXmlElement *pac_ele, TiXmlElement *usr_ele, struct PVar_Set *g_vars, struct PVar_Set *me_vars)
	{
		struct PVar *vr_tmp, *vr2_tmp=0;
		TiXmlElement *e_tmp, *n_ele;
		TiXmlElement *e2_tmp, *n2_ele;
		size_t g_ln;
		unsigned char *cp;
		int dy_cur;
		bool now_still, me_has_usr;

		aSnd->cmd_len = 0;
		aSnd->cmd_buf = 0;
		aSnd->dy_num = 0;

		e_tmp = pac_ele->FirstChildElement(aSnd->tag); 
		g_ln = 0 ;
		while ( e_tmp ) {
			g_ln = 0 ;
			vr_tmp = g_vars->all_still( e_tmp, aSnd->tag, 0, g_ln, n_ele, me_vars);
			e_tmp = n_ele;
			if ( g_ln > 0 ) aSnd->cmd_len += g_ln;
			if ( !vr_tmp ) 	{	//还是常数, 这里应该结束了
				if (e_tmp) printf("plain !!!!!!!!!!\n");	//这不应该
				continue;
			}

			if ( vr_tmp->kind <= VAR_Dynamic ) {	//Me变量的, 不算作动态
				aSnd->dy_num++;
				continue;
			}

			if ( vr_tmp->kind == VAR_Me && vr_tmp->me_sub_nm_len == 0)	//Me变量,且无后缀名, 则从用户命令中取, 如me.body
			{
				vr2_tmp = 0;
				g_ln = 0 ;
				me_has_usr = false;
				if ( vr_tmp->me_had_var) //对于主参考, 就不再从用户命令中取
					goto HAD_LOOK;

				if ( usr_ele->Attribute(vr_tmp->me_name) ) 	//用户命令中，属性优先
				{
					vr2_tmp = g_vars->one_still(0, usr_ele->Attribute(vr_tmp->me_name), 0, g_ln);
					if ( g_ln > 0 ) aSnd->cmd_len += g_ln;
					if ( vr2_tmp && vr2_tmp->kind <= VAR_Dynamic )
						aSnd->dy_num++;
					me_has_usr = true;
				} else {
					e2_tmp = usr_ele->FirstChildElement(vr_tmp->me_name);
					while (e2_tmp) {
						g_ln = 0 ;
						vr2_tmp = g_vars->all_still(e2_tmp, vr_tmp->me_name, 0, g_ln, n2_ele, 0);
						e2_tmp = n2_ele;
						if ( g_ln > 0 ) aSnd->cmd_len += g_ln;
						if ( !vr2_tmp ) 		//还是常数, 这里应该结束了
						{
							if (e2_tmp) printf("plain !!!!!!!!!!\n");	//这不应该
							continue;
						}

						if ( vr2_tmp->kind <= VAR_Dynamic )	//参考变量的, 不算作动态
							aSnd->dy_num++;
						me_has_usr = true;
					}
				}
				HAD_LOOK:
				if ( !me_has_usr && vr_tmp->con->point > vr_tmp->con->base) //用户命令中未找到
					aSnd->cmd_len += (vr_tmp->con->point - vr_tmp->con->base);
			}

			if (vr_tmp->kind == VAR_Me && vr_tmp->con->point > vr_tmp->con->base && vr_tmp->me_sub_nm_len > 0)	//Me变量已有内容,可能已有定义,并带后缀
				aSnd->cmd_len += (vr_tmp->con->point - vr_tmp->con->base);
		}

		aSnd->cmd_buf = new unsigned char[aSnd->cmd_len+1];	//对于非动态的, cmd_buf与cmd_len刚好是其全部的内容
		aSnd->dy_num = aSnd->dy_num *2+1;	/* dy_num表示多少个动态变量, 实际分段数最多是其2倍再多1 */
		aSnd->dy_list = new struct DyList [aSnd->dy_num];

	#define DY_NEXT_STILL	\
		dy_cur++;	\
		aSnd->dy_list[dy_cur].con = cp;	\
		aSnd->dy_list[dy_cur].len = 0;	\
		aSnd->dy_list[dy_cur].dy_pos = -1;

	#define DY_DYNAMIC(X)	\
		now_still = false;		\
		if ( aSnd->dy_list[dy_cur].len > 0 )	\
			dy_cur++;		\
		aSnd->dy_list[dy_cur].con = 0;	\
		aSnd->dy_list[dy_cur].len = 0;	\
		aSnd->dy_list[dy_cur].dy_pos = X;

	#define DY_STILL(LEN)		\
		cp = &cp[LEN];		\
		now_still = true;	\
		aSnd->dy_list[dy_cur].dy_pos = -1;	\
		aSnd->dy_list[dy_cur].len += LEN;

		memset(&aSnd->cmd_buf[0], 0, aSnd->cmd_len+1);
		cp = &aSnd->cmd_buf[0]; 
		dy_cur = -1;	//下面的宏先加1，所以开始为0

		DY_NEXT_STILL
		now_still = true;

		e_tmp = pac_ele->FirstChildElement(aSnd->tag);
		while ( e_tmp ) {
ALL_STILL:
			g_ln = 0;
			vr_tmp= g_vars->all_still( e_tmp, aSnd->tag, cp, g_ln, n_ele, me_vars);
			e_tmp = n_ele;

			if ( g_ln > 0 )	{/* 刚处理的是静态内容 */
				DY_STILL(g_ln) //cp指针后移, 内容增加， 这里游标不变，因为下一个可能是Me变量的静态, 这要合并在一起
			}

			if ( !vr_tmp ) 	{	//还是常数, 这里应该结束了
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
				if ( vr_tmp->me_had_var) { //对于主参考, 就不再从用户命令中取
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
					while (e2_tmp) {
						g_ln = 0;
						vr2_tmp = g_vars->all_still(e2_tmp, vr_tmp->me_name, cp, g_ln, n2_ele, 0);
						e2_tmp = n2_ele;
						me_has_usr = true;
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
				HAD_LOOK_VAR:
				if ( !me_has_usr && vr_tmp->con->point > vr_tmp->con->base ) //用户命令中未找到
				{
					memcpy(cp, vr_tmp->con->base, vr_tmp->con->point - vr_tmp->con->base);
					DY_STILL(vr_tmp->con->point - vr_tmp->con->base) //cp指针后移,内容增加,这里游标不变,因为下一个可能是Me变量的静态, 这要合并在一起
					goto ALL_STILL;	//这里处理的是静态，所以从该处继续
				}
			}
			if ( vr_tmp->kind == VAR_Me && vr_tmp->con->point > vr_tmp->con->base && vr_tmp->me_sub_nm_len > 0)	//Me变量已有内容,可能已有定义的，并带后缀的。
			{
				memcpy(cp, vr_tmp->con->base, vr_tmp->con->point - vr_tmp->con->base);
				DY_STILL(vr_tmp->con->point - vr_tmp->con->base) //cp指针后移,内容增加， 这里游标不变，因为下一个可能是Me变量的静态, 这要合并在一起
				goto ALL_STILL;	//这里处理的是静态，所以从该处继续
			}
		}
		if ( now_still ) 
			aSnd->dy_num = dy_cur + 1;	//最后一个是静态, dy_cur并未增加
		else
			aSnd->dy_num = dy_cur;
	};

	int hard_work (TiXmlElement *pac_ele, TiXmlElement *usr_ele, struct PVar_Set *g_vars, struct PVar_Set *me_vars, Amor *my_obj)
	{
		struct PVar *vr_tmp=0;
		TiXmlElement *e_tmp, *p_ele, *some_ele;
		const char *p=0, *pp=0; 
		char *q;

		int i = 0, a_num,j;
		size_t lnn; 
		int lnn2;
		struct CmdRcv *a_rcv_lst;
		Amor::Pius aps;

		log_str = pac_ele->Attribute("log");	
		ins_tag =  pac_ele->Value();
		if ( strcasecmp(ins_tag, "Abort") ==0 )
			type = INS_Abort;
		else if ( strcasecmp(ins_tag, "Null") ==0 )
			type = INS_Null;
		else if ( strcasecmp(ins_tag, "Respond") ==0 )
			type = INS_Respond;
		else if ( strcasecmp(ins_tag, "Let") ==0 )
			type = INS_LetVar;

		if ( type !=INS_None ) 
			goto LAST_CON;
		aps.ordo = Notitia::Set_InsWay;
		aps.indic = this;
		my_obj->aptus->facio(&aps);	//由下一级设定所有发送域的数组
		if ( !this->ext_ins && this->type == INS_None) 
		{
			TEXTUS_SPRINTF(err_global_str, "map element of %s  is not defined!", this->ins_tag);
			return 0;
		}
		for ( i = 0 ; i < snd_num; i++ )
		{
			if ( snd_lst[i].cmd_buf == 0 && pac_ele->FirstChildElement( snd_lst[i].tag)) //还未设定内容
				hard_work_2(&snd_lst[i], pac_ele, usr_ele, g_vars, me_vars);
		}

	RCV_PRO: /* 预置接收的每个域，设定域号*/
		a_num  = 0;
		for ( i = 0 ; i < rcv_num; i++ )
		{
			const char *tag;
			tag = rcv_lst[i].tag;
			/*子序列中的返回元素也算上 */
			for (e_tmp = pac_ele->FirstChildElement(tag); e_tmp; e_tmp = e_tmp->NextSiblingElement(tag) ) a_num++;
			/* 用户命令的返回元素也算上 */
			for (e_tmp = usr_ele->FirstChildElement(tag); e_tmp; e_tmp = e_tmp->NextSiblingElement(tag) ) a_num++;
		}
		if ( a_num == 0 ) goto LAST_CON;
		a_rcv_lst = rcv_lst;
		rcv_lst = new struct CmdRcv[rcv_num+a_num];
		memcpy(rcv_lst, a_rcv_lst, sizeof(struct CmdRcv)*rcv_num);
		j = rcv_num; //指向第一个空的
		for ( i = 0 ; i < rcv_num; i++ )
		{
			const char *tag;
			tag = rcv_lst[i].tag;
			/* 用户命令和子序列中的返回元素也算上, 当然from已经算一个了 */
			some_ele = pac_ele;
ANOTHER:
			for (e_tmp = some_ele->FirstChildElement(tag); e_tmp; e_tmp = e_tmp->NextSiblingElement(tag) ) 
			{
				memcpy(&rcv_lst[j], &rcv_lst[i], sizeof(struct CmdRcv));
				rcv_lst[j].must_con = 0;
				rcv_lst[j].must_len = 0;
				rcv_lst[j].err_code = 0;
				rcv_lst[j].err_disp_hex = 0;
				
				if ( (p = e_tmp->Attribute("name")) ) {
					vr_tmp = g_vars->look(p, me_vars);	//响应变量, 动态变量, 两个变量集
					if (vr_tmp) {
						rcv_lst[j].dyna_pos = vr_tmp->dynamic_pos;
						lnn2 = 0;
						e_tmp->QueryIntAttribute("start", &(lnn2));
						if ( lnn2 >= 1) 
							rcv_lst[j].start = (size_t)lnn2;
						lnn2 = 0;
						e_tmp->QueryIntAttribute("length", &(lnn2));
						if ( lnn2 > 0) 
							rcv_lst[j].length = (size_t)lnn2;
					}
				}

				if ( (p = e_tmp->GetText()) ){
					vr_tmp = g_vars->look(p, me_vars);	//响应变量, 动态变量, 两个变量集
					if (vr_tmp && vr_tmp->kind == VAR_Constant ) {
						rcv_lst[j].src_con = vr_tmp->con->base;
						rcv_lst[j].src_len = (size_t)(vr_tmp->con->point - vr_tmp->con->base);
					}
				}
				j++;
			}
			if ( some_ele != usr_ele ) {
				some_ele = usr_ele;
				goto ANOTHER;
			}
		}

		rcv_num += a_num ;
		delete [] a_rcv_lst;
LAST_CON:
		return counted ? 1:0 ;
	};

	void prepare(TiXmlElement *pac_ele, TiXmlElement *usr_ele, struct PVar_Set *g_vars, struct PVar_Set *me_vars) {
		struct PVar *err_var;
		err_code = pac_ele->Attribute("error"); //每一个报文定义一个错误码，对于INS_Abort很有用。
		if ( err_code ) 
		{	
			err_var = g_vars->look(err_code, me_vars);
			if (err_var)
				err_code = (const char*)err_var->con->base;
		}
		set_condition ( pac_ele, g_vars, me_vars);
	};

	void set_rcv(struct MK_Session *mess) {
		int ii;
		size_t rlen;
		struct CmdRcv *rply;
		for (ii = 0; ii < rcv_num; ii++)
		{
			rply = &rcv_lst[ii];
			rlen = rply->src_len;
			if ( rply->dyna_pos > 0 && rply->src_con && rlen >= rply->start ) {
				rlen -= (rply->start-1); //start是从1开始
				if ( rply->length > 0 && (unsigned int)rply->length < rlen)
					rlen = rply->length;
				mess->snap[rply->dyna_pos].DyVarBase::input(&rply->src_con[rply->start-1], rlen);
			}
		}
	};
};

struct ComplexSubSerial {
	TiXmlElement *usr_def_entry;//MAP文档中，对用户指令的定义
	TiXmlElement *sub_pro;		//实际使用的指令序列，在相同用户指令名下，可能有不同的序列,可能usr_def_entry相同, 而sub_pro不同。
	struct PVar_Set *g_var_set;	//全局变量集
	struct PVar_Set sv_set;		//局域变量集, 用于引进参数型的用户命令

	struct TranIns *tran_inses;
	int tr_many;

	TiXmlElement *map_root;
	TiXmlElement *usr_ele;	//用户命令
	const char *pri_key ;	//子系列的primary属性值, 可能为null
	const char *pri_var_nm;	//子系列的primary所指向的变量名或具体内容, 可能为null

	long loop_n;	/* 本子序列循环次数: 0:无限, 直到某种失败, >0: 一定次数, 若失败则中止  */
	Amor *my_obj;

	ComplexSubSerial() {
		map_root = usr_ele = 0;

		usr_def_entry = 0;
		sub_pro = 0;

		tran_inses = 0;
		tr_many = 0;
		g_var_set = 0;
		loop_n = 1;
		pri_key = 0;
		pri_var_nm = 0;
		my_obj = 0;
	};

	~ComplexSubSerial() {
		if (tran_inses) delete []tran_inses;
		tran_inses = 0;
		tr_many = 0;
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
		size_t len;
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
		if ( ref_var) for ( att = ref_var->self_ele->FirstAttribute(); att; att = att->Next())
		{
			//把属性加到本地变量集 sv_set
			TEXTUS_SPRINTF(loc_v_nm, "%s%s.%s", ME_VARIABLE_HEAD, mid_nm, att->Name()); 
			att_val = att->Value();
			if ( !att_val ) continue;
			len = 0;
			/* 这里 att->Value() 可能指向另一个变量名 */
			att_var = g_var_set->one_still(ref_var->self_ele, att_val, buf, len);	//找到已定义的变量
			if ( att_var ) 
				sv_set.put_var(loc_v_nm, att_var);
			else if ( len > 0 ) 
				sv_set.put_still(loc_v_nm, buf, len);
		}
		return ref_var;
	};

	struct PVar *set_loc_var(const char *vnm, const char *mid_nm)
	{
		unsigned char buf[512];		//实际内容, 常数内容
		char loc_v_nm[128];
		size_t len;
		struct PVar *avar = 0;
		TEXTUS_SPRINTF(loc_v_nm, "%s%s", ME_VARIABLE_HEAD, mid_nm); 
		
		len = 0;
		avar = g_var_set->one_still(0,vnm, buf, len);	//找到已定义参考变量的
		if ( avar ) 
			sv_set.put_var(loc_v_nm, avar);
		else if ( len > 0 ) 
			sv_set.put_still(loc_v_nm, buf, len);

		return avar;
	};

	bool ev_pro( TiXmlElement *sub, int &which, int &icc_num)	//为了无限制嵌套
	{
		TiXmlElement *pac_ele, *def_ele, *t_ele;
		for ( pac_ele= sub->FirstChildElement(); pac_ele; pac_ele = pac_ele->NextSiblingElement())
		{
			if ( !pac_ele->Value() ) continue;
			if ((t_ele = map_root->FirstChildElement(pac_ele->Value())))//如果在map中有定义, 也就是一个嵌套(类似于宏)
			{
				if ( !ev_pro(t_ele, which, icc_num) )
					return false;
			} else {
				tran_inses[which].prepare(pac_ele, usr_ele, g_var_set, &sv_set);
				icc_num += tran_inses[which].hard_work(pac_ele, usr_ele, g_var_set, &sv_set, my_obj);
				if ( !tran_inses[which].ext_ins && !tran_inses[which].type) return false;
				which++;
			}
		}
		return true;
	};

	void ev_num( TiXmlElement *sub, int &many )	//为了无限制嵌套
	{
		TiXmlElement *pac_ele, *t_ele;
		for ( pac_ele= sub->FirstChildElement(); pac_ele; pac_ele = pac_ele->NextSiblingElement())
		{
			if ( !pac_ele->Value() ) continue;
			if ((t_ele = map_root->FirstChildElement(pac_ele->Value())))//如果在map中有定义, 也就是一个嵌套(类似于宏)
				ev_num(t_ele, many);
			else	/* 在map中有定义的就算一个了 */
				many++;
		}
	};

	const char *find_from_usr_ele(const char *me_name) {
		const char *nm=0;
		TiXmlElement *body;	//用户命令的第一个body元素
		nm = usr_ele->Attribute(me_name);	//先看属性名为me.XX.yy中的XX名，nm是$Main之的。
		if (!nm ) { //属性优先, 没有属性再看元素
			body = usr_ele->FirstChildElement(me_name);	//再看元素为me.XX.yy中的XX名，nm是$Main之的。
			if ( body ) { 
				nm = body->GetText();
				if ( body->NextSiblingElement(me_name) ) //不只一个元素, 这里无法处理, 留给hard_work_2
					return 0;
			}
		}
		if (!nm )	//还是没有, 那看map文件的入口元素
			nm = usr_def_entry->Attribute(me_name);	//nm是$Main之类的。
		return nm;
	}

	int pro_analyze(const char *loop_str) {
		struct PVar *ref_var, *me_var;
		const char *nm;
		char pro_nm[128];
		int which, icc_num=0, i;
		struct PVar_Set tmp_sv;		//临时局域变量集
		size_t tmplen;

		if ( loop_str ) loop_n = atoi(loop_str);
		if ( loop_n < 0 ) loop_n = 1;
			
		TEXTUS_SNPRINTF(pro_nm, sizeof(pro_nm), "%s", "Pro"); //先假定子序列是Pro element，如果有主参考变量，则找合成的
		if ( pri_var_nm ) {//如果有主参考变量, 就即根据这个主参考变量中找到相应的sub_pro, pri_var_nm就是$Main之类的。
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
		g_var_set->dynamic_at = sv_set.dynamic_at ;	//sv_set(局部变量集, 不能动态的, 这行和上面的一行没用)
		sv_set.command_ele = usr_ele;

		if ( pri_var_nm ) set_loc_ref_var(pri_var_nm, pri_key, true);	//先把主参考量赋值, pri_key是"protect"这样的内容
		for ( i = 0; i < sv_set.many; i++ ) {
			me_var = &(sv_set.vars[i]);
			if ( pri_key && strcmp(me_var->me_name, pri_key) == 0 ) 
			{	//主参考变量标记后，略过
				me_var->me_had_var = true;
				continue;	
			} 
			if ( me_var->kind == VAR_Me ) {
				nm = find_from_usr_ele(me_var->me_name); 	//从用户命令中找, nm是"$Main"之类的
				if ( !nm ) continue;
				me_var->me_had_var = true;
				if ( me_var->me_sub_name )  //处理有后缀的Me变量
					/* 如果参考量没有相应的属性, 原有带后缀变量内容得以保留, 否则被更新 */
					set_loc_ref_var(nm, me_var->me_name); /* 处理me.export.*这样的东西。这里更新局部变量集 */
				else 
					set_loc_var(nm, me_var->me_name); /* 处理me.typeid之类的*/
			}
		}

		tr_many = 0;
		ev_num (sub_pro, tr_many);
		//确定变量数
		if ( tr_many ==0 ) return 0;
		tran_inses = new struct TranIns[tr_many];

		which = 0; icc_num = 0;
		if ( !ev_pro ( sub_pro, which, icc_num))  { tr_many = which; }
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

	int  set_sub( TiXmlElement *usr_ele, struct PVar_Set *vrset, TiXmlElement *sub_serial, TiXmlElement * map_root, Amor *my_obj) //返回对IC的指令数
	{
		TiXmlElement *pri;
		const char *pri_nm;
		int ret_ic=0, i;

		//前面已经分析过了, 这里肯定不为NULL,nm就是Command之类的。 sub_serial 
		pri_nm = sub_serial->Attribute("primary");
		if ( pri_nm) {
			if ( usr_ele->Attribute(pri_nm)) {
				/* pro_analyze 根据变量名, 去找到实际真正的变量内容(必须是参考变量)。变量内容中指定了Pro等 */
				comp_num = 1;
				complex = new struct ComplexSubSerial;
				#define PUT_COMPLEX(X) \
						complex[X].usr_def_entry = sub_serial;	\
						complex[X].g_var_set = vrset;			\
						complex[X].usr_ele = usr_ele;			\
						complex[X].pri_key = pri_nm;			\
						complex[X].my_obj = my_obj;			\
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
	INS_Set () {
		instructions= 0;
		many = 0;
		ic_num = 0;
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

		if ( IS_VAR(nm)) return 0;

		sub_serial = map_root->FirstChildElement(nm); //找到子系列		
		return sub_serial;
	};

	#define MACRO_SUFFIX "_Macro"
	void ev_maro( TiXmlElement *m_root, struct PVar_Set *var_set, TiXmlElement *map_root, int &vmany, int &mor,int base_cor, Amor *my_obj)	//为了无限制嵌套宏定义
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
					a_ic_num = instructions[vmany].set_sub(m_usr_ele, var_set, sub, map_root, my_obj);
					if ( a_ic_num < 0 ) continue;
					ic_num += a_ic_num;
					instructions[vmany].order = cor;
					mor = cor;
					vmany++;
				} else if ( !(IS_VAR(com_nm))) { 
					TEXTUS_SNPRINTF(macro_nm, sizeof(macro_nm)-1, "%s"MACRO_SUFFIX, com_nm);
					macro_ele= map_root->FirstChildElement(macro_nm); //map_root中找宏定义
					if ( !macro_ele) continue;
					m_usr_ele->QueryIntAttribute("order", &(cor)); 
					nbase = base_cor + cor;
					if ( nbase <= mor ) continue;	//order不符合顺序的，略过
					ev_maro( macro_ele, var_set, map_root, vmany, mor, nbase, my_obj);	//为了无限制嵌套宏定义
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
				else if ( !(IS_VAR(com_nm))) { 
					TEXTUS_SNPRINTF(macro_nm, sizeof(macro_nm)-1, "%s"MACRO_SUFFIX, com_nm);
					macro_ele= map_root->FirstChildElement(macro_nm); //map_root中找宏定义
					if ( !macro_ele) continue;
					ev_num(macro_ele, var_set, map_root, refny);
				}
			}
		}
	};

	void put_inses(TiXmlElement *root, struct PVar_Set *var_set, TiXmlElement *map_root, Amor *my_obj)
	{
		int mor, vmany, refny,i,j,k;
		refny = 0; 
		ev_num(root, var_set, map_root, refny);

		//初步确定变量数
		many = refny ;
		if ( many ==0 ) return;
		instructions = new struct User_Command[many];
			
		mor = -999999;	//这样，顺序号可以从负数开始
		ic_num = 0;
		vmany = 0;
		ev_maro(root, var_set, map_root, vmany, mor, 0, my_obj);	//为了无限制嵌套宏定义

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
	const char *k_name;
	TiXmlDocument doc_v;	//其它Variable定义
	TiXmlElement *v_root;
		
	struct PVar_Set person_vars;
	struct INS_Set ins_all;

	char flow_md[64];	//指令流指纹数据
	const char *flow_id;	//指令流标志

	inline Personal_Def () {
		c_root = k_root = v_root = 0;
		flow_id = 0;
		k_name = 0;
		memset(flow_md, 0, sizeof(flow_md));
	};
	inline ~Personal_Def () {};

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

	bool put_def( TiXmlElement *per_ele, TiXmlElement *prop, Amor *my_obj)
	{
		const char *ic_nm, *nm;
		if ( !per_ele) return false;

#define GET_XML_DEF(ROOT, DEF_DOC, LOC_ELE, ATTR_NAME_FILE, MD5_ATTR) \
		nm = 0;							\
		if ( !(ROOT = per_ele->FirstChildElement(LOC_ELE)))	\
		{								\
			if ( (nm = per_ele->Attribute(ATTR_NAME_FILE)))		\
				load_xml(nm, DEF_DOC,  ROOT, per_ele->Attribute(MD5_ATTR),err_global_str);	\
			else if ( !(ROOT = prop->FirstChildElement(LOC_ELE)))				\
			{											\
				if ( (nm = prop->Attribute(ATTR_NAME_FILE)))						\
					load_xml(nm, DEF_DOC,  ROOT, prop->Attribute(MD5_ATTR),err_global_str);	\
			}												\
		}

		if ( !(c_root = per_ele->FirstChildElement("Flow")))
		{
			nm =  per_ele->Attribute("md5");
			if ( (ic_nm = per_ele->Attribute("flow")))
				load_xml(ic_nm, doc_c,  c_root, nm,err_global_str);
			if ( nm)
				squeeze(nm, (unsigned char*)&flow_md[0]);
		}
		GET_XML_DEF(k_root, doc_k, "Key",  "key", "key_md5")
		k_name = nm;	//nm已经是key属性的内容了,即文件名
		GET_XML_DEF(v_root, doc_v, "Var",  "var", "var_md5")
		if ( !c_root || !k_root ) return false;
		person_vars.defer_vars(k_root, c_root);	//变量定义, map文件优先
		flow_id = c_root->Attribute("flow");
		set_here(c_root);	//再看本定义
		set_here(v_root);	//看看其它变量定义,key.xml等

		ins_all.put_inses(c_root, &person_vars, k_root, my_obj);//用户命令集定义.
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

	void put_def(TiXmlElement *prop, const char *vn, Amor *my_obj)	//个人化集合输入定义PersonDef_Set
	{
		TiXmlElement *per_ele;
		int kk;
		num_icp = 0; 
		for (per_ele = prop->FirstChildElement(vn); per_ele; per_ele = per_ele->NextSiblingElement(vn) ) 
			num_icp ++; 

		icp_def = new struct Personal_Def [num_icp];
		for (per_ele = prop->FirstChildElement(vn), kk = 0; per_ele;per_ele = per_ele->NextSiblingElement(vn))
		{
			if ( icp_def[kk].put_def(per_ele, prop, my_obj))
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

class TranWay: public Amor {
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Amor::Pius *);
	void handle_tran(struct FlowStr *fl);	//左边状态处理
	bool sponte( Amor::Pius *);
	Amor *clone();

	TranWay();
	~TranWay();

private:
	void h_fail(char tmp[], char desc[], int p_len, int q_len, const char *p, const char *q, const char *fun);
	void mk_hand();
	void mk_result(bool fend=true);
	void set_global_vars();

	struct G_CFG { 	//全局定义
		TBuffer *var_bufs;	//全局变量数据区
		size_t bufs_num;
		TiXmlElement *prop;
		struct PersonDef_Set person_defs;
		struct Personal_Def null_icp_def;

		inline G_CFG() {
			var_bufs = 0;
			bufs_num = 0;
		};	
		inline ~G_CFG() { 
			if ( var_bufs ) {
				 delete[] var_bufs;
				var_bufs = 0;
				bufs_num = 0;
			}
		};
	};

	Amor::Pius loc_ans_tran, prodb_ps, other_ps, loc_pro_ins, log_ps;

	struct MK_Session mess;	//记录一个过程中的各种临时数据
	struct Personal_Def *cur_def;	//当前定义
	struct InsReply cur_ins_reply;	//当前insway数据
	struct InsWay cur_insway;	//当前insway数据

	struct G_CFG *gCFG;     /* 全局共享参数 */
	bool has_config;
	struct WKBase {
		int step;	//0: just start, 1: doing 
		int cur;
		int pac_which;
		TRAN_STEP tran_step;
		long sub_loop;	//循环次数
	} command_wt;

	enum SUB_RET sub_serial_pro(struct ComplexSubSerial *comp);
	#include "wlog.h"
};

void TranWay::set_global_vars()
{
	int i,j,k;
	struct  Personal_Def *def=0, *def2=0;

	size_t num;
	num = 0;
	for ( i = 0 ; i < gCFG->person_defs.num_icp; i++ )
	{	
		def = &gCFG->person_defs.icp_def[i];
		for ( j = 0 ; j < i ; j++ ) {	//往前寻找
			//def2 = &gCFG->person_defs.icp_def[j].person_vars;
			def2 = &gCFG->person_defs.icp_def[j];
			if ( def->k_root == def2->k_root ) 
				break;
			if ( def->k_name != 0 && def2->k_name != 0 ) 
			{
				if (strcmp(def->k_name, def2->k_name) == 0 ) 
					break;
			}
		}
		if ( j == i ) {	//未找到以前, 这是新的, 计算global变量,
			for ( k = 0; k < def->person_vars.many; k++ )
				if ( def->person_vars.vars[k].kind == VAR_Dynamic_Global ) num++;	
		}
	}
	gCFG->var_bufs = new TBuffer[num];
	gCFG->bufs_num = num;
	num = 0;
	for ( i = 0 ; i < gCFG->person_defs.num_icp; i++ )
	{	
		def = &gCFG->person_defs.icp_def[i];
		for ( j = 0 ; j < i ; j++ ) {	//往前寻找
			def2 = &gCFG->person_defs.icp_def[j];
			if ( def->k_root == def2->k_root ) 	//可能是内部定义的, 而不是外部定义
				break;
			if ( def->k_name != 0 && def2->k_name != 0 ) 
			{
				if (strcmp(def->k_name, def2->k_name) == 0 ) 	//相同的map.xml文件
					break;
			}
		}
		if ( j == i ) {	//未找到以前, 这是新的, 赋予全局指针
			for ( k = 0; k < def->person_vars.many; k++ )
				if ( def->person_vars.vars[k].kind == VAR_Dynamic_Global ) {
					def->person_vars.vars[k].con = &gCFG->var_bufs[num];
					TBuffer::pour(gCFG->var_bufs[num], def->person_vars.vars[k].nal);
					num++;
				}
		} else { //找到以前的, 就复制过来, 指针而已
			for ( k = 0; k < def->person_vars.many; k++ )
				if ( def->person_vars.vars[k].kind == VAR_Dynamic_Global ) 
					def->person_vars.vars[k].con = def2->person_vars.vars[k].con;
		}
	}
	//{int *a =0 ; *a = 0; };
}

void TranWay::ignite(TiXmlElement *prop) {
	const char *comm_str;
	if (!prop) return;
	if ( !gCFG ) {
		gCFG = new struct G_CFG();
		gCFG->prop = prop;
		has_config = true;
	}
	return;
}

TranWay::TranWay() {
	gCFG = 0;
	has_config = false;
	loc_pro_ins.ordo = Notitia::Pro_InsWay;
	loc_pro_ins.indic = &cur_insway;
	loc_pro_ins.subor = Amor::CAN_ALL;
	loc_ans_tran.ordo = Notitia::Ans_TranWay;
	loc_ans_tran.indic = 0;
	loc_ans_tran.subor = Amor::CAN_ALL;
	log_ps.ordo = Notitia::Log_InsWay;
}

TranWay::~TranWay() {
	if ( has_config  ) {	
		if(gCFG) delete gCFG;
		gCFG = 0;
	}
}

Amor* TranWay::clone() {
	TranWay *child = new TranWay();
	child->gCFG = gCFG;
	return (Amor*) child;
}

bool TranWay::facio( Amor::Pius *pius) {
	Amor::Pius tmp_pius;
	switch ( pius->ordo ) {
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		gCFG->person_defs.put_def(gCFG->prop, "bus", this);
		gCFG->null_icp_def.put_def(gCFG->prop->FirstChildElement("bike"), gCFG->prop, this);
		set_global_vars();
		mess.init(gCFG->person_defs.max_snap_num);
		if ( err_global_str[0] != 0 ) {
			WLOG(ERR,"%s", err_global_str);
		}
		cur_insway.psnap = mess.psnap;
		cur_insway.snap_num = mess.snap_num;
		cur_insway.reply = &cur_ins_reply;
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
		mess.init(gCFG->person_defs.max_snap_num);
		cur_insway.psnap = mess.psnap;
		cur_insway.snap_num = mess.snap_num;
		cur_insway.reply = &cur_ins_reply;
		break;

	case Notitia::START_SESSION:
		WBUG("facio START_SESSION" );
		mess.reset(false);	//动态量硬复位
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION" );
		mess.reset(false);	//动态量硬复位
		break;

	case Notitia::Pro_TranWay:    /* 有来自控制台的请求 */
		WBUG("facio Pro_TranWay");
		handle_tran((struct FlowStr*)pius->indic);
		break;
	default:
		return false;
	}
	return true;
}

bool TranWay::sponte( Amor::Pius *pius) {
	assert(pius);
	if (!gCFG ) return false;

	switch ( pius->ordo ) {
	case Notitia::Ans_InsWay:
		WBUG("sponte Ans_InsWay indic=%p", pius->indic);
		memcpy(&cur_ins_reply, pius->indic, sizeof(struct InsReply));

		if ( mess.right_status != RT_OUT || mess.right_subor != pius->subor )	//表明是右端返回
		{
			if ( mess.right_status == RT_OUT )
			{
				WLOG(WARNING, "mess error right_status=RT_OUT right_subor=%d pius->subor=%d", mess.right_subor, pius->subor);
			} else {
				WLOG(WARNING, "mess error right_status=RT_IDLE right_subor=%d pius->subor=%d", mess.right_subor, pius->subor);
			}
		} else {
			mk_hand();
		}
		break;

	case Notitia::DMD_END_SESSION:	//右节点关闭, 要处理
		WBUG("sponte DMD_END_SESSION");
		if ( mess.right_status == RT_OUT)	//表明是事务处理中
		{
			struct User_Command *usr_com;
			struct TranIns *trani;
			TEXTUS_SPRINTF(mess.err_str, "device down at subor=%d", pius->subor);
			usr_com = &(cur_def->ins_all.instructions[mess.ins_which]);
			trani = &(usr_com->complex[command_wt.cur].tran_inses[command_wt.pac_which]);
			if ( trani->err_code) mess.snap[Pos_ErrCode].input(trani->err_code);
			mess.snap[Pos_ErrStr].input(mess.err_str);
			WLOG(WARNING, "Error %s:  %s", mess.snap[Pos_ErrCode].val_p, mess.err_str);
			mk_result(true);		
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

void TranWay::handle_tran(struct FlowStr *flp) {
	int i;
	size_t alen;
	struct PVar  *vt;
	struct DyVar *dvr;
	bool no_def_fail = false;

	/* 这里就是一般的业务啦 */
	mess.reset();	//会话复位
	if ( !flp ) {	//对于未指定flow_id的， 取那个默认的
		cur_def = &(gCFG->null_icp_def);
		goto CUR_DEF_PRO;
	}
	if (flp->len >= sizeof(mess.flow_id) ) 
		alen = sizeof(mess.flow_id)-1 ;
	else
		alen = flp->len;
	memcpy(mess.flow_id, flp->flow_str, alen);
	mess.flow_id[alen] = 0;
	cur_def = gCFG->person_defs.look(mess.flow_id); //找一个相应的指令流定义
CUR_DEF_PRO:
	TEXTUS_STRCPY(mess.err_str, " ");
	if ( !cur_def ) {
		no_def_fail = true;	
		cur_def = &(gCFG->null_icp_def);
	}
	/* 寻找变量集中所有动态的, 看看是否有start_pos和get_length的, 根据定义赋值到mess中 */
	for ( i = 0 ; i <  cur_def->person_vars.many; i++) 
	{
		vt = &cur_def->person_vars.vars[i];
		if ( vt->dynamic_pos >=0 ) {
			dvr = &mess.snap[vt->dynamic_pos];
			if (dvr->c_len > 0) continue; //如果动态量没有清空, 就不再赋新值.这个量跨flow, 直到Notitia:START_SESSION,END_SESSION
			dvr->kind = vt->kind;
			dvr->def_var = vt;
			dvr->def_link = (vt->kind == VAR_Dynamic_Link);
			if ( vt->kind == VAR_Dynamic_Global ) 
				dvr->con = vt->con;	//全局
			if ( vt->con->point > vt->con->base )	//先把定义的静态内容链接过来, 动态变量的默认值
				dvr->DyVarBase::input(vt->con->base, vt->con->point - vt->con->base, true); //只是link, 即使全局量也不矛盾。
		}
	}
	
	//{int *a =0 ; *a = 0; };
	if ( no_def_fail) {
		TEXTUS_SPRINTF(mess.err_str, "not defined flow_id: %s ", mess.flow_id );
		mess.snap[Pos_ErrCode].input("-99");
		WLOG(WARNING, "Error %s:  %s", mess.snap[Pos_ErrCode].val_p, mess.err_str);
		mess.snap[Pos_ErrStr].input(mess.err_str);
		mk_result(true);	//工作结束
		return;
	}
	/* 任务开始  */
	mess.snap[Pos_TotalIns].input( cur_def->ins_all.ic_num);
	if ( cur_def->flow_md[0] )
		mess.snap[Pos_FlowPrint].input( cur_def->flow_md);
	mess.snap[Pos_FlowID].input(mess.flow_id);

	mess.ins_which = 0;
	command_wt.step=0; //指示终端准备开始工作
	mk_hand();
}

/* 子序列入口 */
enum SUB_RET TranWay::sub_serial_pro(struct ComplexSubSerial *comp)
{
	struct TranIns *trani;
SUB_INS_PRO:
	trani = &(comp->tran_inses[command_wt.pac_which]);
	if ( command_wt.tran_step == Tran_Idle )
	{
		if ( !trani->valid_condition(&mess) )		/* 不符合条件,就转下一条 */
		{
			command_wt.pac_which++;	//指向下一条报文处理
			if (  command_wt.pac_which == comp->tr_many )
				return Sub_OK;	//整个已经完成
			 else
				goto SUB_INS_PRO;
		}
	}

	switch ( trani->type) {
	case INS_Abort:
		if (trani->err_code ) mess.snap[Pos_ErrCode].input(trani->err_code);
		TEXTUS_SPRINTF(mess.err_str,  "user abort at %d of %s", mess.pro_order, cur_def->flow_id);
		mess.snap[Pos_ErrStr].input(mess.err_str);
		WLOG(WARNING, "Error %s:  %s", mess.snap[Pos_ErrCode].val_p, mess.err_str);
		command_wt.tran_step = Tran_End;
		return  Sub_Soft_Fail;	//脚本所控制的错误, 软失败
		break;

	case INS_Null:
		command_wt.tran_step =Tran_End;
		break;

	case INS_Respond:
		aptus->sponte(&loc_ans_tran);    //暂回应前端, 但整个业务不结束
		command_wt.tran_step = Tran_End;
		break;

	case INS_LetVar:
		command_wt.tran_step = Tran_End;
		trani->set_rcv(&mess);
		break;

	default:
		switch ( command_wt.tran_step ) {
		case Tran_Idle:
			cur_insway.dat = trani;
			cur_ins_reply.err_code = 0;
			WBUG("will(%s) order=%d which=%d", trani->isFunction ? "sync":"async", mess.pro_order, command_wt.pac_which);
			if ( trani->isFunction )
			{
				aptus->facio(&loc_pro_ins); //向右发出指令, 右节点不再sponte. 正常情况下, right_status不变
				goto TRAN_END;
			}
			command_wt.tran_step = Tran_Working;
			mess.right_subor = trani->up_subor;
			return Sub_Working; 	/* 正在进行 */
			break;
		case Tran_Working:
			//if ( mess.pro_order == 49 &&  command_wt.pac_which == 1 ) {int *a=0; *a=0;}
			TRAN_END:
			command_wt.tran_step = Tran_End;
			if ( cur_ins_reply.err_code) 
			{
				TEXTUS_SPRINTF(mess.err_str, "fault at order=%d pac_which=%d of %s (%s)", mess.pro_order, command_wt.pac_which, cur_def->flow_id,  cur_ins_reply.err_str);
				mess.snap[Pos_ErrCode].input(cur_ins_reply.err_code);
				mess.snap[Pos_ErrStr].input(mess.err_str);
				WLOG(WARNING, "ERROR_RECV_PAC %s:  %s", mess.snap[Pos_ErrCode].val_p, mess.err_str);
				return Sub_Rcv_Pac_Fail;	//这是基本报文错误，非map所控制
			}
			break;
		default:
			break;
		}
		break;
	}

	assert( command_wt.tran_step == Tran_End );
	if ( !trani->valid_result(&mess) )
	{
		TEXTUS_SPRINTF(mess.err_str, "result error at order=%d pac_which=%d of %s", mess.pro_order, command_wt.pac_which, cur_def->flow_id);
		if ( trani->err_code) mess.snap[Pos_ErrCode].input(trani->err_code);
		mess.snap[Pos_ErrStr].input(mess.err_str);
		WLOG(WARNING, "Error %s:  %s", mess.snap[Pos_ErrCode].val_p, mess.err_str);
		return Sub_Valid_Fail;	//这是map所控制
	} else {
		command_wt.pac_which++;	//指向下一条报文处理
		command_wt.tran_step = Tran_Idle;
		if (  command_wt.pac_which == comp->tr_many )
			return Sub_OK;	//整个已经完成
		else
			goto SUB_INS_PRO;
	}
	return Sub_Working;
}

void TranWay::mk_hand()
{
	struct User_Command *usr_com;
	struct TranIns *trani;
	int i_ret;
	struct PVar  *vt;

#define ERR_TO_LAST_INS	\
		aptus->facio(&log_ps);     				\
		if ( mess.ins_which == (cur_def->ins_all.many-1)) {	\
			mk_result(true);				\
			return;						\
		} else {						\
			mess.ins_which = cur_def->ins_all.many - 1;	\
			command_wt.step=0;				\
			goto INS_PRO;					\
		}
#define NEXT_INS	\
		mess.ins_which++;				\
		command_wt.step=0;				\
		if ( mess.ins_which < cur_def->ins_all.many)	\
			goto INS_PRO;				\
		else 						\
			mk_result(false);		
INS_PRO:
	usr_com = &(cur_def->ins_all.instructions[mess.ins_which]);
	//{int *a =0 ; *a = 0; };
	switch ( command_wt.step) {
	case 0:	//开始
		if ( !usr_com->valid_condition(&mess) ) {
			NEXT_INS
			break;
		}
		mess.pro_order = usr_com->order;	
		if ( mess.snap[Pos_CurCent].def_var) 
			mess.snap[Pos_CurCent].input((mess.ins_which*100)/cur_def->ins_all.many);

		command_wt.cur = 0;
		mess.willLast = ( usr_com->comp_num == 1 ); //一个用户操作，包括几个复合指令的尝试，有一个成功，就算OK
NEXT_PRI_TRY:
		command_wt.sub_loop = usr_com->complex[command_wt.cur].loop_n; //软失败的重试次数
LOOP_PRI_TRY:
		command_wt.pac_which = 0;	//新子系列, pac从第0个开始
		command_wt.tran_step = Tran_Idle;	//pac处理开始, 
		command_wt.step++;	//指向下一步
	case 1:
		i_ret = sub_serial_pro( &(usr_com->complex[command_wt.cur]));
		switch (i_ret ) {
		case Sub_Soft_Fail: //这是软失败
			command_wt.sub_loop--;
			if ( command_wt.sub_loop != 0 ) 
			{ //如果原为是0,则为负,几乎到不了0
				command_wt.sub_loop--;
				command_wt.step--;
				goto LOOP_PRI_TRY;
			}
			if ( command_wt.cur < (usr_com->comp_num-1) ) 
			{ //用户定义的Abort才试下一个
				command_wt.cur++;
				command_wt.step--;
				mess.willLast = ( command_wt.cur == (usr_com->comp_num-1) ); //最后一条复合指令啦，如果出错就调用自定义的出错过程(响应报文设置一些数据，或者向终端发些指令)
				vt =  mess.snap[Pos_ErrCode].def_var;	//重试下一个之前, 结果码(错误码）置为初始值
				if ( vt ) mess.snap[Pos_ErrCode].DyVarBase::input(vt->con->base, vt->con->point - vt->con->base, true);	
				vt =  mess.snap[Pos_ErrStr].def_var;	//重试下一个之前, 结果提示置为初始值
				if (vt)	mess.snap[Pos_ErrStr].DyVarBase::input(vt->con->base, vt->con->point - vt->con->base, true);
				goto NEXT_PRI_TRY;		//试另一个
			} else {		//最后一条处理失败，定义出错值
				ERR_TO_LAST_INS
			}
			break;

		case Sub_OK: //完成
			mess.right_status = RT_IDLE;	//右端闲
			WBUG("has completed %d, order %d", mess.ins_which, mess.pro_order);
			NEXT_INS
			break;

		case Sub_Rcv_Pac_Fail: //接收响应失败
		case Sub_Valid_Fail: //校验响应失败, 脚本控制或报文定义 的 错误
			ERR_TO_LAST_INS
			break;

		case Sub_Working: //正进行中
			mess.right_status = RT_OUT;
			aptus->facio(&loc_pro_ins);     //向右发出指令,aptus.facio的处理放在最后,很重要!! 因为这个调用中可能收到右节点的sponte. 注意!!,一定要注意.
			break;
		}
		break;
	}
}

void TranWay::mk_result(bool fend)
{
	int ret;
	if ( fend )	//这是为了对付中途中断的情况 
	{
		mess.ins_which = cur_def->ins_all.many-1;	//最后一个用户指令, 多重试几个, 只算第一个,取最后一条报文处理
		mess.pro_order = cur_def->ins_all.instructions[mess.ins_which].order;
		command_wt.pac_which = cur_def->ins_all.instructions[mess.ins_which].complex[0].tr_many-1;
		command_wt.tran_step = Tran_Idle; //pac处理开始,
		ret = sub_serial_pro(&cur_def->ins_all.instructions[mess.ins_which].complex[0]);
		if ( ret <= 0  ) {
			WLOG(EMERG, "bug! last_pac_pro should finished!");
		}
	}
	aptus->sponte(&loc_ans_tran);    //制卡的结果回应给控制台
	mess.reset();
}
#include "hook.c"
