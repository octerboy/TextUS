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

int squeeze(const char *p, unsigned char *q)	//�ѿո�ȼ���, ֻ����16�����ַ�(��д), ����ʵ�ʵĳ���
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
/* �ұ�״̬, ����, �������ĵ���Ӧ */
enum RIGHT_STATUS { RT_IDLE = 0, RT_OUT=7};
enum TRAN_STEP {Tran_Idle = 0, Tran_Working = 1, Tran_End=2};
enum SUB_RET {Sub_Working = 0, Sub_OK = 1, Sub_Rcv_Pac_Fail=-2, Sub_Soft_Fail=-1, Sub_Valid_Fail = -3};

/* ����SysTime�����ı����������ⲿ�������㣬��������ֻ�����ű�ָ������ */	
#define Pos_ErrCode 1 
#define Pos_FlowPrint 2 
#define Pos_TotalIns 3 
#define Pos_CurOrder 4 
#define Pos_CurCent 5 
#define Pos_ErrStr 6 
#define Pos_FlowID 7 
#define Pos_Fixed_Next 8  //��һ����̬������λ��, Ҳ��Ϊ�ű��Զ��嶯̬�����ĵ�1��λ��
#define VARIABLE_TAG_NAME "Var"
#define ME_VARIABLE_HEAD "me."

enum TranIns_Type { INS_None = 0, INS_Normal=1, INS_Abort=2, INS_Respond =8, INS_LetVar=10, INS_Null=99};
enum Var_Type {VAR_ErrCode=1, VAR_FlowPrint=2, VAR_TotalIns = 3, VAR_CurOrder=4, VAR_CurCent=5, VAR_ErrStr=6, VAR_FlowID=7, VAR_Dynamic_Global = 8, VAR_Dynamic_Link = 9, VAR_Dynamic = 10, VAR_Me=12, VAR_Constant=98,  VAR_None=99};
struct PVar {
	Var_Type kind;
	const char *name;	//������doc�ĵ�
	int n_len;		//���Ƴ���

	char me_name[64];	//Me�������ƣ���ȥ��ͷ�� me. �����ֽ�, ��������׺. �ӱ�����name�и��ƣ����63�ַ�
	unsigned int me_nm_len;
	const char *me_sub_name;  //Me������׺���� �ӱ�����name�ж�λ��
	int me_sub_nm_len;
	bool me_had_var;	//���ο��������, true: �ںϳ��뱨��(hard_work_2)ʱ, ���ٴ��û�ele��ȡ����Ϊ�ڷ���sub_serialʱ, �Ѿ���ֵ
	TBuffer *con;		//�ⲿ�ɷ���, ͨ��ָ��nal, Globalʱָ��G_CFG�еı���
	TBuffer nal;		//�ڲ�����, 

	bool keep_alive;	//true: ���Ƕ�̬����, ֻ��Notitia::START_SESSION��DMD_END_SESSIONʱ���, false: ��ÿ��flow_id��ʼ�����,
	int dynamic_pos;	//��̬����λ��, -1��ʾ��̬
	TiXmlElement *self_ele;	/* ����, ����Ԫ�ذ������ֿ���: 1.����������, 
				2.һ��ָ������, ��ָ����Ԫ�ط���ʱ, �緢��һ���õ��ı�����, ��������ʱ, ����Щָ��Ƕ�롣 */
	PVar () {
		kind = VAR_None;	//�����Ϊ�Ǳ���
		name = 0;
		n_len = 0;
		con = &nal;
		dynamic_pos = -1;	//�Ƕ�̬��
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
		nal.point[0] = 0;	//��֤null��β
		kind = VAR_Constant;	//��Ϊ�ǳ���
		dynamic_pos = -1;
		con = &nal;	
	};

	void put_var(struct PVar* att_var) {
		kind = att_var->kind;
		nal.reset();
		nal.input(att_var->nal.base, att_var->nal.point-att_var->nal.base);
		dynamic_pos = att_var->dynamic_pos;
	};

	struct PVar* prepare(TiXmlElement *var_ele, int &dy_at) //����׼��
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
			nal.point[0] = 0;	//��֤null��β
		}

		if ( strcasecmp(nm, "$ink" ) == 0 ) //��ǰ�û������ӡ
		{
			dynamic_pos = Pos_FlowPrint;
			kind = VAR_FlowPrint;
		}
		if ( strcasecmp(nm, "$total" ) == 0 ) //���û�������
		{
			dynamic_pos = Pos_TotalIns;
			kind = VAR_TotalIns;
		}
		if ( strcasecmp(nm, "$ErrCode" ) == 0 ) //�������
		{
			dynamic_pos = Pos_ErrCode;
			kind = VAR_ErrCode;
		}

		if ( strcasecmp(nm, "$CurOrder" ) == 0 ) //��ǰ�û�������
		{
			dynamic_pos = Pos_CurOrder;
			kind = VAR_CurOrder;
		}

		if ( strcasecmp(nm, "$CurCent" ) == 0 ) //��ǰ�����ٷֱ�
		{
			dynamic_pos = Pos_CurCent;
			kind = VAR_CurCent;
		}

		if ( strcasecmp(nm, "$ErrStr" ) == 0 ) //��������
		{
			dynamic_pos = Pos_ErrStr;
			kind = VAR_ErrStr;
		}

		if ( strcasecmp(nm, "$FlowID" ) == 0 ) //ҵ������ʶ
		{
			dynamic_pos = Pos_FlowID;
			kind = VAR_FlowID;
		}
		if ( kind != VAR_None) goto P_RET; //���ж��壬���ٿ����Dynamic, ���϶��嶼��Dynamic��ͬ����
		if ( (p = var_ele->Attribute("dynamic")) )
		{
			if ( strcasecmp(p, "global" ) == 0 || *p == 'G' || *p== 'g'  )
				kind = VAR_Dynamic_Global;

			if ( strcasecmp(p, "link" ) == 0 || *p == 'L' || *p== 'l' ) 
				kind = VAR_Dynamic_Link;

			if ( strcasecmp(p, "copy" ) == 0 || *p == 'Y' || *p == 'y' || *p == 'C' || *p== 'c'  ) 
				kind = VAR_Dynamic;

			dynamic_pos = dy_at;	//��̬����λ��
			dy_at++;
			if ( (p = var_ele->Attribute("alive")) && (*p == 'Y' || *p == 'y') )
				keep_alive = true;
		}

		if ( kind != VAR_None) goto P_RET; //���ж��壬

		/* ���¶�Me�������д��� ����������, ������Ҫ����һ��me.��Щ������ Ҫ�����������鷳 */
		if ( strncasecmp(nm, ME_VARIABLE_HEAD, sizeof(ME_VARIABLE_HEAD)-1) == 0 ) 	//��1����, �����һ��null�ַ�
		{
			kind = VAR_Me;
			me_sub_name = strpbrk(&nm[sizeof(ME_VARIABLE_HEAD)-1], ".");	//��Me���������ҵ�һ���㣬�������Ϊ��׺��.
			if ( me_sub_name )	//������ں�׺
			{
				me_nm_len = me_sub_name - &nm[sizeof(ME_VARIABLE_HEAD)-1];
				me_sub_name++;	//��Ȼ������㱾���Ǻ�׺��, �Ӻ�һ����ʼ���Ǻ�׺��
				me_sub_nm_len = strlen(me_sub_name);
			} else 		//��������ں�׺
				me_nm_len = strlen(&nm[sizeof(ME_VARIABLE_HEAD)-1]);

			if ( me_nm_len >= sizeof ( me_name))	//Me�������ռ�����, 64�ֽ����
				me_nm_len = sizeof ( me_name)-1;
			memcpy(me_name, &nm[sizeof(ME_VARIABLE_HEAD)-1], me_nm_len);
			me_name[me_nm_len] = 0 ;
		}

		if ( kind == VAR_None && c_len > 0) //���������ݵ���û������, �ǾͶ�Ϊ����, ������Ҳ����������, ��Ҫ�𴦶�����
			kind = VAR_Constant;	//��Ϊ�ǳ���

		P_RET:
		return this;
	};

	void put_herev(TiXmlElement *h_ele) //����һ�±��ر���
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
			nal.point[0] = 0;	//��֤null��β
			if (kind == VAR_None)	//ֻ��ԭ��û�ж������͵�, ����Ŷ��ɳ���. 
				kind = VAR_Constant;
		}
	};
};

struct DyVar:public DyVarBase { /* ��̬������ �������Ա��ĵ� */
	Var_Type kind;  //��̬����,
	struct PVar *def_var;	//�ļ��ж���ı���

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
		con = &nal;	//con�п���ָ��һ��ȫ�ֵ�,�����Ǳ��ص�nal
		def_link = false;
	}
};

struct MK_Session {		//��¼һ����������еĸ�����ʱ����
	struct DyVar *snap;	//��ʱ�ı���, ����
	struct DyVarBase **psnap;	//ָ��snap
	int snap_num;
	bool willLast;		//���һ���Դ�. ͨ����һ���û�ָ��ֻ����һ�δ�, ��ֵΪtrue�� ��ʱ�ж�γ���, ��ֵ��Ϊfalse�����һ��Ϊtrue.

	char err_str[1024];	//������Ϣ
	char flow_id[64];
	int pro_order;		//��ǰ����Ĳ������

	RIGHT_STATUS right_status;
	int right_subor;	//ָʾ���ҷ���ʱ��subor, ����ʱ�˶ԡ�
	int ins_which;	//�Ѿ��������ĸ�����, ��Ϊ������������±�ֵ

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
				if ( i >= Pos_Fixed_Next )   //���Pos_Fixed_Next����Ҫ, Ҫ��Ȼ, ��Щ���еĶ�̬������û�е�
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

	inline void init(int m_snap_num) //���m_snap_num���Ը�XML��������̬������
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
		reset(false);	//��̬��Ӳ��λ
	};

	~MK_Session () {
		if ( snap ) delete[] snap;
		if ( psnap ) delete[] psnap;
		snap = 0;
		psnap = 0;
	};
};

struct PVar_Set {	/* ��������*/
	struct PVar *vars;
	int many;
	int dynamic_at;
	TiXmlElement *command_ele;	/* ���ھ��������, ָ����ϵ������Ӧ���û�����(Command)  */
	PVar_Set () {
		vars = 0;
		many = 0;
		dynamic_at = Pos_Fixed_Next; //0,�� �Ѿ���$FlowPrint��ռ��
		command_ele = 0;
	};
	
	~PVar_Set () {
		if (vars ) delete []vars;
		vars = 0;
		many = 0;
	};
	
#define IS_VAR(X) (X != 0 && strcasecmp(X, VARIABLE_TAG_NAME) == 0 ) 
	
	void defer_vars(TiXmlElement *map_root, TiXmlElement *icc_root=0) //����һ�±�������
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
			if ( !had_nm )	//���û���Ѷ������, ������
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
			if ( !had_nm )	{//���û���Ѷ������, ������
				if ( vars[vmany].prepare(i_ele, dynamic_at) )
					vmany++;
			}
		}
	
		if ( vmany > many ) printf("bug!!!!!!! var_many prior %d less than post %d\n", many, vmany);
		many = vmany;		//����ٸ���һ�α�����
	};
	
	struct PVar *look( const char *n, struct PVar_Set *loc_v=0)	//�����Ƿ�Ϊһ������ı�����, �Ȳ�loc_v����ֲ�������
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
	
	/* �Ҿ�̬�ı���, ���ʵ������ */
	struct PVar *one_still(TiXmlElement *comp, const char *nm, unsigned char *buf, size_t &len, struct PVar_Set *loc_v=0)
	{
		struct PVar  *vt;
		const char *esc = 0;
		/* ����ʱ�����������, һ�����о�̬���������, ��һ����̬�������� */
		vt = look(nm, loc_v);	//�����Ƿ�Ϊһ������ı�����
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
				len = squeeze(nm, buf); //�Ƕ��������, ����ֱ�Ӵ�����, �������һ������
			goto VARET;
		}

		len = 0;
		if ( vt->kind ==  VAR_Constant ) {	//��̬��������
			len = vt->con->point - vt->con->base;
			if ( buf && len > 0 ) memcpy(buf, vt->con->base, len);
		} else 
			len = 0;
		VARET:
		if ( buf ) buf[len] = 0;	//����NULL
		return vt;
	};

	/* nxt ��һ������, ���ڶ��tagԪ�أ���֮��̬���ݺϳɵ� һ������command�С����ڷǾ�̬�ģ����ظ�tagԪ���Ǹ���̬���� */
	struct PVar *all_still( TiXmlElement *ele, const char*tag, unsigned char *command, size_t &ac_len, TiXmlElement *&nxt, struct PVar_Set *loc_v=0)
	{
		TiXmlElement *comp;
		size_t l;
		struct PVar  *rt;
		bool will_break= false;
				
		rt = 0;
		/* ac_len�Ӳ�������, �ۼƵ�, command����ԭ���ĺ���, ��������ָ�� */
		for ( comp = ele; comp && !will_break ; comp = comp->NextSiblingElement(tag) )
		{
			if ( !comp->GetText() ) continue; //û������, �Թ�
			//printf("tag %s  Text %s\n",tag, comp->GetText());
			if ( command ) 
				rt = one_still(comp,  comp->GetText(), &command[ac_len], l, loc_v);
			else 
				rt = one_still(comp, comp->GetText(), 0, l, loc_v);
			ac_len += l;
			if ( rt && rt->kind < VAR_Constant )		//����зǾ�̬��, ������Ҫ�ж�, compָ����һ��
				will_break = true;
		}
		if (command) command[ac_len] = 0;	//����NULL
		nxt = comp;	//ָʾ��һ������
		return rt;
	};
};

/* �������ƥ��Ӧ���ǲ���Ҫ��� */
struct MatchDst {	//ƥ��Ŀ��
	struct PVar *dst;
	const char *con_dst;
	size_t len_dst;
	bool c_case;	/* �Ƿ����ִ�Сд, Ĭ��Ϊ�� */
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
			if ( p) { //����������û������, �����������ǳ�����, ������Ҳ����������, ��Ҫ�𴦶�����
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

struct Match {		//һ��ƥ����
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
		src = var_set->look(nm, loc_v);	//����������
		if (!src ) 
			return 0;
		
		if (strstr(mch_ele->Value(), "not"))
			ys_no = false;
		else
			ys_no = true;

		dst_num = 0;
		for (con_ele = mch_ele->FirstChildElement(vn); con_ele; con_ele = con_ele->NextSiblingElement(vn) ) 
			dst_num++;

		if ( dst_num == 0 ) {//һ����valueԪ��Ҳû��
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
		bool ret=false;	//�ٶ�û��һ���Ƿ��ϵ�
		for(i = 0; i < dst_num; i++) {
			ret = dst_arr[i].valid_val(sess, src);
			if ( ret ) break;	//�ж��ֵ����һ����ͬ���Ͳ��ٱȽ��ˡ�
		}
		if ( !ys_no ) ret = !ret; 	//���û��һ����ͬ, ��ȡ��, ��ͷ���ok��Ҳ����not���е�ֵ��
		return ret;
	};
};

struct Condition {	//һ��ָ���ƥ���б�, ��������������ƥ��
	bool hasLastWill;	/* ͨ����ֵΪfalse,��ִ��ĳָ���Ҫ�ж�Mess�е�willLast�� ������ϵ�б����Զ��, ����Ҫ�ж�willLastΪtrueʱ��ִ��.
				��ֵ���ⲿ�ű�ʹ�ã�ָʾ�Ƿ���Ե��ô�������̣����ն˷�����ָ���)
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
		return ret;	//���з�����ϣ����������������
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
			if ( !vr_tmp ) 	{	//���ǳ���, ����Ӧ�ý�����
				if (e_tmp) printf("plain !!!!!!!!!!\n");	//�ⲻӦ��
				continue;
			}

			if ( vr_tmp->kind <= VAR_Dynamic ) {	//Me������, ��������̬
				aSnd->dy_num++;
				continue;
			}

			if ( vr_tmp->kind == VAR_Me && vr_tmp->me_sub_nm_len == 0)	//Me����,���޺�׺��, ����û�������ȡ, ��me.body
			{
				vr2_tmp = 0;
				g_ln = 0 ;
				me_has_usr = false;
				if ( vr_tmp->me_had_var) //�������ο�, �Ͳ��ٴ��û�������ȡ
					goto HAD_LOOK;

				if ( usr_ele->Attribute(vr_tmp->me_name) ) 	//�û������У���������
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
						if ( !vr2_tmp ) 		//���ǳ���, ����Ӧ�ý�����
						{
							if (e2_tmp) printf("plain !!!!!!!!!!\n");	//�ⲻӦ��
							continue;
						}

						if ( vr2_tmp->kind <= VAR_Dynamic )	//�ο�������, ��������̬
							aSnd->dy_num++;
						me_has_usr = true;
					}
				}
				HAD_LOOK:
				if ( !me_has_usr && vr_tmp->con->point > vr_tmp->con->base) //�û�������δ�ҵ�
					aSnd->cmd_len += (vr_tmp->con->point - vr_tmp->con->base);
			}

			if (vr_tmp->kind == VAR_Me && vr_tmp->con->point > vr_tmp->con->base && vr_tmp->me_sub_nm_len > 0)	//Me������������,�������ж���,������׺
				aSnd->cmd_len += (vr_tmp->con->point - vr_tmp->con->base);
		}

		aSnd->cmd_buf = new unsigned char[aSnd->cmd_len+1];	//���ڷǶ�̬��, cmd_buf��cmd_len�պ�����ȫ��������
		aSnd->dy_num = aSnd->dy_num *2+1;	/* dy_num��ʾ���ٸ���̬����, ʵ�ʷֶ����������2���ٶ�1 */
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
		dy_cur = -1;	//����ĺ��ȼ�1�����Կ�ʼΪ0

		DY_NEXT_STILL
		now_still = true;

		e_tmp = pac_ele->FirstChildElement(aSnd->tag);
		while ( e_tmp ) {
ALL_STILL:
			g_ln = 0;
			vr_tmp= g_vars->all_still( e_tmp, aSnd->tag, cp, g_ln, n_ele, me_vars);
			e_tmp = n_ele;

			if ( g_ln > 0 )	{/* �մ�����Ǿ�̬���� */
				DY_STILL(g_ln) //cpָ�����, �������ӣ� �����α겻�䣬��Ϊ��һ��������Me�����ľ�̬, ��Ҫ�ϲ���һ��
			}

			if ( !vr_tmp ) 	{	//���ǳ���, ����Ӧ�ý�����
				if (e_tmp) printf("plain !!!!!!!!!!\n");	//�ⲻӦ��
				continue;
			}

			if ( vr_tmp->kind <= VAR_Dynamic )	//�ο�������, ��������̬��
			{
				DY_DYNAMIC(vr_tmp->dynamic_pos)	//���о�̬���ݵģ� ��ָ����һ��, �Դ�Ŷ�̬��
				DY_NEXT_STILL
				continue;
			}

			if ( vr_tmp->kind == VAR_Me && vr_tmp->me_sub_nm_len == 0)	//Me����,���޺�׺��, ����û�������ȡ����Ȼ��û������
			{
				vr2_tmp = 0;
				g_ln = 0;
				me_has_usr = false;
				if ( vr_tmp->me_had_var) { //�������ο�, �Ͳ��ٴ��û�������ȡ
					goto HAD_LOOK_VAR;
				}
				if (usr_ele->Attribute(vr_tmp->me_name)) //�ȿ���������,�������ԣ����ٿ���Ԫ����
				{
					vr2_tmp = g_vars->one_still(usr_ele, usr_ele->Attribute(vr_tmp->me_name), cp, g_ln);
					if ( g_ln > 0 )	/* �մ�����Ǿ�̬���� */
					{
						DY_STILL(g_ln) //cpָ�����, �������ӣ� �����α겻�䣬��Ϊ��һ��������Me�����ľ�̬, ��Ҫ�ϲ���һ��
					}

					if ( vr2_tmp && vr2_tmp->kind <= VAR_Dynamic )
					{
						DY_DYNAMIC(vr2_tmp->dynamic_pos)	//���о�̬���ݵģ� ��ָ����һ��, �Դ�Ŷ�̬��
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
						if ( g_ln > 0 )	/* �մ�����Ǿ�̬���� */
						{
							DY_STILL(g_ln) //cpָ�����, �������ӣ� �����α겻�䣬��Ϊ��һ��������Me�����ľ�̬, ��Ҫ�ϲ���һ��
						}

						if ( !vr2_tmp ) 		//���ǳ���, ����Ӧ�ý�����
						{
							if (e2_tmp) printf("plain !!!!!!!!!!\n");	//�ⲻӦ��
							continue;
						}

						if ( vr2_tmp->kind <= VAR_Dynamic )	//Me������, ��������̬
						{
							DY_DYNAMIC(vr2_tmp->dynamic_pos)	//���о�̬���ݵģ� ��ָ����һ��, �Դ�Ŷ�̬��
							DY_NEXT_STILL
							now_still = false;
						}
					}
				}
				HAD_LOOK_VAR:
				if ( !me_has_usr && vr_tmp->con->point > vr_tmp->con->base ) //�û�������δ�ҵ�
				{
					memcpy(cp, vr_tmp->con->base, vr_tmp->con->point - vr_tmp->con->base);
					DY_STILL(vr_tmp->con->point - vr_tmp->con->base) //cpָ�����,��������,�����α겻��,��Ϊ��һ��������Me�����ľ�̬, ��Ҫ�ϲ���һ��
					goto ALL_STILL;	//���ﴦ����Ǿ�̬�����ԴӸô�����
				}
			}
			if ( vr_tmp->kind == VAR_Me && vr_tmp->con->point > vr_tmp->con->base && vr_tmp->me_sub_nm_len > 0)	//Me������������,�������ж���ģ�������׺�ġ�
			{
				memcpy(cp, vr_tmp->con->base, vr_tmp->con->point - vr_tmp->con->base);
				DY_STILL(vr_tmp->con->point - vr_tmp->con->base) //cpָ�����,�������ӣ� �����α겻�䣬��Ϊ��һ��������Me�����ľ�̬, ��Ҫ�ϲ���һ��
				goto ALL_STILL;	//���ﴦ����Ǿ�̬�����ԴӸô�����
			}
		}
		if ( now_still ) 
			aSnd->dy_num = dy_cur + 1;	//���һ���Ǿ�̬, dy_cur��δ����
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
		my_obj->aptus->facio(&aps);	//����һ���趨���з����������
		if ( !this->ext_ins && this->type == INS_None) 
		{
			TEXTUS_SPRINTF(err_global_str, "map element of %s  is not defined!", this->ins_tag);
			return 0;
		}
		for ( i = 0 ; i < snd_num; i++ )
		{
			if ( snd_lst[i].cmd_buf == 0 && pac_ele->FirstChildElement( snd_lst[i].tag)) //��δ�趨����
				hard_work_2(&snd_lst[i], pac_ele, usr_ele, g_vars, me_vars);
		}

	RCV_PRO: /* Ԥ�ý��յ�ÿ�����趨���*/
		a_num  = 0;
		for ( i = 0 ; i < rcv_num; i++ )
		{
			const char *tag;
			tag = rcv_lst[i].tag;
			/*�������еķ���Ԫ��Ҳ���� */
			for (e_tmp = pac_ele->FirstChildElement(tag); e_tmp; e_tmp = e_tmp->NextSiblingElement(tag) ) a_num++;
			/* �û�����ķ���Ԫ��Ҳ���� */
			for (e_tmp = usr_ele->FirstChildElement(tag); e_tmp; e_tmp = e_tmp->NextSiblingElement(tag) ) a_num++;
		}
		if ( a_num == 0 ) goto LAST_CON;
		a_rcv_lst = rcv_lst;
		rcv_lst = new struct CmdRcv[rcv_num+a_num];
		memcpy(rcv_lst, a_rcv_lst, sizeof(struct CmdRcv)*rcv_num);
		j = rcv_num; //ָ���һ���յ�
		for ( i = 0 ; i < rcv_num; i++ )
		{
			const char *tag;
			tag = rcv_lst[i].tag;
			/* �û�������������еķ���Ԫ��Ҳ����, ��Ȼfrom�Ѿ���һ���� */
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
					vr_tmp = g_vars->look(p, me_vars);	//��Ӧ����, ��̬����, ����������
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
					vr_tmp = g_vars->look(p, me_vars);	//��Ӧ����, ��̬����, ����������
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
		err_code = pac_ele->Attribute("error"); //ÿһ�����Ķ���һ�������룬����INS_Abort�����á�
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
				rlen -= (rply->start-1); //start�Ǵ�1��ʼ
				if ( rply->length > 0 && (unsigned int)rply->length < rlen)
					rlen = rply->length;
				mess->snap[rply->dyna_pos].DyVarBase::input(&rply->src_con[rply->start-1], rlen);
			}
		}
	};
};

struct ComplexSubSerial {
	TiXmlElement *usr_def_entry;//MAP�ĵ��У����û�ָ��Ķ���
	TiXmlElement *sub_pro;		//ʵ��ʹ�õ�ָ�����У�����ͬ�û�ָ�����£������в�ͬ������,����usr_def_entry��ͬ, ��sub_pro��ͬ��
	struct PVar_Set *g_var_set;	//ȫ�ֱ�����
	struct PVar_Set sv_set;		//���������, �������������͵��û�����

	struct TranIns *tran_inses;
	int tr_many;

	TiXmlElement *map_root;
	TiXmlElement *usr_ele;	//�û�����
	const char *pri_key ;	//��ϵ�е�primary����ֵ, ����Ϊnull
	const char *pri_var_nm;	//��ϵ�е�primary��ָ��ı��������������, ����Ϊnull

	long loop_n;	/* ��������ѭ������: 0:����, ֱ��ĳ��ʧ��, >0: һ������, ��ʧ������ֹ  */
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

	/* �������������protect��ָ���Ԫ������������, ������Ȼ, ����para1,para2֮��ġ� 
		vnm: ��һ���������������ȫ�ֱ��в鲻����������������
		mid_num����Me��ָ����,����Ԫ�ػ�primay������ָ���ģ����������Ϊ: me.mid_num.xx
	*/
	struct PVar *set_loc_ref_var(const char *vnm, const char *mid_nm, bool me_pri=false)
	{
		unsigned char buf[512];		//ʵ������, ��������
		char loc_v_nm[128];
		TiXmlAttribute *att; 
		size_t len;
		struct PVar *ref_var = 0, *att_var=0;
		const char *att_val;
		
		len = 0;
		ref_var = g_var_set->one_still(0,vnm, buf, len);	//�ҵ��Ѷ���ο�������
		if ( len > 0 && me_pri)	{ //�ҵ���ȫ�ֱ������������ݣ��ӵ������С����ο���������
			struct PVar *av ;
			TEXTUS_SPRINTF(loc_v_nm, "%s%s", ME_VARIABLE_HEAD, mid_nm); 
			av = sv_set.look(loc_v_nm, 0);
			if ( av) {
				av->put_still(buf, len);
			}
		}
		if ( ref_var) for ( att = ref_var->self_ele->FirstAttribute(); att; att = att->Next())
		{
			//�����Լӵ����ر����� sv_set
			TEXTUS_SPRINTF(loc_v_nm, "%s%s.%s", ME_VARIABLE_HEAD, mid_nm, att->Name()); 
			att_val = att->Value();
			if ( !att_val ) continue;
			len = 0;
			/* ���� att->Value() ����ָ����һ�������� */
			att_var = g_var_set->one_still(ref_var->self_ele, att_val, buf, len);	//�ҵ��Ѷ���ı���
			if ( att_var ) 
				sv_set.put_var(loc_v_nm, att_var);
			else if ( len > 0 ) 
				sv_set.put_still(loc_v_nm, buf, len);
		}
		return ref_var;
	};

	struct PVar *set_loc_var(const char *vnm, const char *mid_nm)
	{
		unsigned char buf[512];		//ʵ������, ��������
		char loc_v_nm[128];
		size_t len;
		struct PVar *avar = 0;
		TEXTUS_SPRINTF(loc_v_nm, "%s%s", ME_VARIABLE_HEAD, mid_nm); 
		
		len = 0;
		avar = g_var_set->one_still(0,vnm, buf, len);	//�ҵ��Ѷ���ο�������
		if ( avar ) 
			sv_set.put_var(loc_v_nm, avar);
		else if ( len > 0 ) 
			sv_set.put_still(loc_v_nm, buf, len);

		return avar;
	};

	bool ev_pro( TiXmlElement *sub, int &which, int &icc_num)	//Ϊ��������Ƕ��
	{
		TiXmlElement *pac_ele, *def_ele, *t_ele;
		for ( pac_ele= sub->FirstChildElement(); pac_ele; pac_ele = pac_ele->NextSiblingElement())
		{
			if ( !pac_ele->Value() ) continue;
			if ((t_ele = map_root->FirstChildElement(pac_ele->Value())))//�����map���ж���, Ҳ����һ��Ƕ��(�����ں�)
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

	void ev_num( TiXmlElement *sub, int &many )	//Ϊ��������Ƕ��
	{
		TiXmlElement *pac_ele, *t_ele;
		for ( pac_ele= sub->FirstChildElement(); pac_ele; pac_ele = pac_ele->NextSiblingElement())
		{
			if ( !pac_ele->Value() ) continue;
			if ((t_ele = map_root->FirstChildElement(pac_ele->Value())))//�����map���ж���, Ҳ����һ��Ƕ��(�����ں�)
				ev_num(t_ele, many);
			else	/* ��map���ж���ľ���һ���� */
				many++;
		}
	};

	const char *find_from_usr_ele(const char *me_name) {
		const char *nm=0;
		TiXmlElement *body;	//�û�����ĵ�һ��bodyԪ��
		nm = usr_ele->Attribute(me_name);	//�ȿ�������Ϊme.XX.yy�е�XX����nm��$Main֮�ġ�
		if (!nm ) { //��������, û�������ٿ�Ԫ��
			body = usr_ele->FirstChildElement(me_name);	//�ٿ�Ԫ��Ϊme.XX.yy�е�XX����nm��$Main֮�ġ�
			if ( body ) { 
				nm = body->GetText();
				if ( body->NextSiblingElement(me_name) ) //��ֻһ��Ԫ��, �����޷�����, ����hard_work_2
					return 0;
			}
		}
		if (!nm )	//����û��, �ǿ�map�ļ������Ԫ��
			nm = usr_def_entry->Attribute(me_name);	//nm��$Main֮��ġ�
		return nm;
	}

	int pro_analyze(const char *loop_str) {
		struct PVar *ref_var, *me_var;
		const char *nm;
		char pro_nm[128];
		int which, icc_num=0, i;
		struct PVar_Set tmp_sv;		//��ʱ���������
		size_t tmplen;

		if ( loop_str ) loop_n = atoi(loop_str);
		if ( loop_n < 0 ) loop_n = 1;
			
		TEXTUS_SNPRINTF(pro_nm, sizeof(pro_nm), "%s", "Pro"); //�ȼٶ���������Pro element����������ο����������Һϳɵ�
		if ( pri_var_nm ) {//��������ο�����, �ͼ�����������ο��������ҵ���Ӧ��sub_pro, pri_var_nm����$Main֮��ġ�
			ref_var = g_var_set->one_still(0,pri_var_nm, 0, tmplen);	//�ҵ��Ѷ���ο�������
			if ( ref_var ) {
				if ( (nm = ref_var->self_ele->Attribute("SubPro")) )  { //�ο�������pro����ָʾ������
					TEXTUS_SNPRINTF(pro_nm, sizeof(pro_nm), "%s%s", "Pro", nm);
				}
			}
		}

		sub_pro = usr_def_entry->FirstChildElement(pro_nm);	//��λʵ�ʵ���ϵ��
		if ( !sub_pro ) return -1; //û�������� 

		sv_set.dynamic_at = g_var_set->dynamic_at;
		//{int *a =0 ; *a = 0; };
		sv_set.defer_vars(usr_def_entry); 
		g_var_set->dynamic_at = sv_set.dynamic_at ;	//sv_set(�ֲ�������, ���ܶ�̬��, ���к������һ��û��)
		sv_set.command_ele = usr_ele;

		if ( pri_var_nm ) set_loc_ref_var(pri_var_nm, pri_key, true);	//�Ȱ����ο�����ֵ, pri_key��"protect"����������
		for ( i = 0; i < sv_set.many; i++ ) {
			me_var = &(sv_set.vars[i]);
			if ( pri_key && strcmp(me_var->me_name, pri_key) == 0 ) 
			{	//���ο�������Ǻ��Թ�
				me_var->me_had_var = true;
				continue;	
			} 
			if ( me_var->kind == VAR_Me ) {
				nm = find_from_usr_ele(me_var->me_name); 	//���û���������, nm��"$Main"֮���
				if ( !nm ) continue;
				me_var->me_had_var = true;
				if ( me_var->me_sub_name )  //�����к�׺��Me����
					/* ����ο���û����Ӧ������, ԭ�д���׺�������ݵ��Ա���, ���򱻸��� */
					set_loc_ref_var(nm, me_var->me_name); /* ����me.export.*�����Ķ�����������¾ֲ������� */
				else 
					set_loc_var(nm, me_var->me_name); /* ����me.typeid֮���*/
			}
		}

		tr_many = 0;
		ev_num (sub_pro, tr_many);
		//ȷ��������
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
	int comp_num; //һ��ֻ��һ������ʱ��Ҫ���Լ���

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

	int  set_sub( TiXmlElement *usr_ele, struct PVar_Set *vrset, TiXmlElement *sub_serial, TiXmlElement * map_root, Amor *my_obj) //���ض�IC��ָ����
	{
		TiXmlElement *pri;
		const char *pri_nm;
		int ret_ic=0, i;

		//ǰ���Ѿ���������, ����϶���ΪNULL,nm����Command֮��ġ� sub_serial 
		pri_nm = sub_serial->Attribute("primary");
		if ( pri_nm) {
			if ( usr_ele->Attribute(pri_nm)) {
				/* pro_analyze ���ݱ�����, ȥ�ҵ�ʵ�������ı�������(�����ǲο�����)������������ָ����Pro�� */
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
			} else {	//һ���û�������������������ָ��ĳ��ԣ���һ���ɹ�������OK
				for( pri = usr_ele->FirstChildElement(pri_nm), comp_num = 0; pri; pri = pri->NextSiblingElement(pri_nm) )
					comp_num++;
				if ( comp_num ==0 ) return -1;
				complex = new struct ComplexSubSerial[comp_num];

				for( pri = usr_ele->FirstChildElement(pri_nm), i = 0; 
					pri; pri = pri->NextSiblingElement(pri_nm) )
				{
					PUT_COMPLEX(i)
					complex[i].pri_var_nm = pri->GetText();
					ret_ic = complex[i].pro_analyze(pri->Attribute("loop")); //�����ѡ��ָ�����ͼ����һ��
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

		sub_serial = map_root->FirstChildElement(nm); //�ҵ���ϵ��		
		return sub_serial;
	};

	#define MACRO_SUFFIX "_Macro"
	void ev_maro( TiXmlElement *m_root, struct PVar_Set *var_set, TiXmlElement *map_root, int &vmany, int &mor,int base_cor, Amor *my_obj)	//Ϊ��������Ƕ�׺궨��
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
					if ( (cor = base_cor + cor ) <= mor ) continue; //order������,�Թ�
					a_ic_num = instructions[vmany].set_sub(m_usr_ele, var_set, sub, map_root, my_obj);
					if ( a_ic_num < 0 ) continue;
					ic_num += a_ic_num;
					instructions[vmany].order = cor;
					mor = cor;
					vmany++;
				} else if ( !(IS_VAR(com_nm))) { 
					TEXTUS_SNPRINTF(macro_nm, sizeof(macro_nm)-1, "%s"MACRO_SUFFIX, com_nm);
					macro_ele= map_root->FirstChildElement(macro_nm); //map_root���Һ궨��
					if ( !macro_ele) continue;
					m_usr_ele->QueryIntAttribute("order", &(cor)); 
					nbase = base_cor + cor;
					if ( nbase <= mor ) continue;	//order������˳��ģ��Թ�
					ev_maro( macro_ele, var_set, map_root, vmany, mor, nbase, my_obj);	//Ϊ��������Ƕ�׺궨��
				}
			}
		}
	};

	void ev_num( TiXmlElement *m_root, struct PVar_Set *var_set, TiXmlElement *map_root, int &refny )	//Ϊ��������Ƕ�׺궨��
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
					macro_ele= map_root->FirstChildElement(macro_nm); //map_root���Һ궨��
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

		//����ȷ��������
		many = refny ;
		if ( many ==0 ) return;
		instructions = new struct User_Command[many];
			
		mor = -999999;	//������˳��ſ��ԴӸ�����ʼ
		ic_num = 0;
		vmany = 0;
		ev_maro(root, var_set, map_root, vmany, mor, 0, my_obj);	//Ϊ��������Ƕ�׺궨��

		many = vmany; //����ٸ���һ���û�������
	};
};	

/* User_Commandָ�������� */
struct  Personal_Def	//���˻�����
{
	TiXmlDocument doc_c;	//User_Commandָ��壻
	TiXmlElement *c_root;	
	TiXmlDocument doc_k;	//map��Variable���壬�����ж���
	TiXmlElement *k_root;
	const char *k_name;
	TiXmlDocument doc_v;	//����Variable����
	TiXmlElement *v_root;
		
	struct PVar_Set person_vars;
	struct INS_Set ins_all;

	char flow_md[64];	//ָ����ָ������
	const char *flow_id;	//ָ������־

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
			cv = person_vars.look(var_ele->Attribute("name")); //�����еı���������Ѱ�Ҷ�Ӧ��
			if ( !cv ) continue; 	//�޴˱���, �Թ�
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
		k_name = nm;	//nm�Ѿ���key���Ե�������,���ļ���
		GET_XML_DEF(v_root, doc_v, "Var",  "var", "var_md5")
		if ( !c_root || !k_root ) return false;
		person_vars.defer_vars(k_root, c_root);	//��������, map�ļ�����
		flow_id = c_root->Attribute("flow");
		set_here(c_root);	//�ٿ�������
		set_here(v_root);	//����������������,key.xml��

		ins_all.put_inses(c_root, &person_vars, k_root, my_obj);//�û��������.
		return true;
	};
};
	
struct PersonDef_Set {	//User_Command����֮����
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

	void put_def(TiXmlElement *prop, const char *vn, Amor *my_obj)	//���˻��������붨��PersonDef_Set
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
		num_icp = kk; //ʵ���ٸ���һ��
		//if( kk > 0 ) {int *a =0 ; *a = 0; };
	};

	/* ����flowid, ��һ�����ʵĸ��˻����� */
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
	void handle_tran(struct FlowStr *fl);	//���״̬����
	bool sponte( Amor::Pius *);
	Amor *clone();

	TranWay();
	~TranWay();

private:
	void h_fail(char tmp[], char desc[], int p_len, int q_len, const char *p, const char *q, const char *fun);
	void mk_hand();
	void mk_result(bool fend=true);
	void set_global_vars();

	struct G_CFG { 	//ȫ�ֶ���
		TBuffer *var_bufs;	//ȫ�ֱ���������
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

	struct MK_Session mess;	//��¼һ�������еĸ�����ʱ����
	struct Personal_Def *cur_def;	//��ǰ����
	struct InsReply cur_ins_reply;	//��ǰinsway����
	struct InsWay cur_insway;	//��ǰinsway����

	struct G_CFG *gCFG;     /* ȫ�ֹ������ */
	bool has_config;
	struct WKBase {
		int step;	//0: just start, 1: doing 
		int cur;
		int pac_which;
		TRAN_STEP tran_step;
		long sub_loop;	//ѭ������
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
		for ( j = 0 ; j < i ; j++ ) {	//��ǰѰ��
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
		if ( j == i ) {	//δ�ҵ���ǰ, �����µ�, ����global����,
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
		for ( j = 0 ; j < i ; j++ ) {	//��ǰѰ��
			def2 = &gCFG->person_defs.icp_def[j];
			if ( def->k_root == def2->k_root ) 	//�������ڲ������, �������ⲿ����
				break;
			if ( def->k_name != 0 && def2->k_name != 0 ) 
			{
				if (strcmp(def->k_name, def2->k_name) == 0 ) 	//��ͬ��map.xml�ļ�
					break;
			}
		}
		if ( j == i ) {	//δ�ҵ���ǰ, �����µ�, ����ȫ��ָ��
			for ( k = 0; k < def->person_vars.many; k++ )
				if ( def->person_vars.vars[k].kind == VAR_Dynamic_Global ) {
					def->person_vars.vars[k].con = &gCFG->var_bufs[num];
					TBuffer::pour(gCFG->var_bufs[num], def->person_vars.vars[k].nal);
					num++;
				}
		} else { //�ҵ���ǰ��, �͸��ƹ���, ָ�����
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
		mess.reset(false);	//��̬��Ӳ��λ
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION" );
		mess.reset(false);	//��̬��Ӳ��λ
		break;

	case Notitia::Pro_TranWay:    /* �����Կ���̨������ */
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

		if ( mess.right_status != RT_OUT || mess.right_subor != pius->subor )	//�������Ҷ˷���
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

	case Notitia::DMD_END_SESSION:	//�ҽڵ�ر�, Ҫ����
		WBUG("sponte DMD_END_SESSION");
		if ( mess.right_status == RT_OUT)	//��������������
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

	case Notitia::START_SESSION:	//�ҽ�֪ͨ, �ѻ�
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

	/* �������һ���ҵ���� */
	mess.reset();	//�Ự��λ
	if ( !flp ) {	//����δָ��flow_id�ģ� ȡ�Ǹ�Ĭ�ϵ�
		cur_def = &(gCFG->null_icp_def);
		goto CUR_DEF_PRO;
	}
	if (flp->len >= sizeof(mess.flow_id) ) 
		alen = sizeof(mess.flow_id)-1 ;
	else
		alen = flp->len;
	memcpy(mess.flow_id, flp->flow_str, alen);
	mess.flow_id[alen] = 0;
	cur_def = gCFG->person_defs.look(mess.flow_id); //��һ����Ӧ��ָ��������
CUR_DEF_PRO:
	TEXTUS_STRCPY(mess.err_str, " ");
	if ( !cur_def ) {
		no_def_fail = true;	
		cur_def = &(gCFG->null_icp_def);
	}
	/* Ѱ�ұ����������ж�̬��, �����Ƿ���start_pos��get_length��, ���ݶ��帳ֵ��mess�� */
	for ( i = 0 ; i <  cur_def->person_vars.many; i++) 
	{
		vt = &cur_def->person_vars.vars[i];
		if ( vt->dynamic_pos >=0 ) {
			dvr = &mess.snap[vt->dynamic_pos];
			if (dvr->c_len > 0) continue; //�����̬��û�����, �Ͳ��ٸ���ֵ.�������flow, ֱ��Notitia:START_SESSION,END_SESSION
			dvr->kind = vt->kind;
			dvr->def_var = vt;
			dvr->def_link = (vt->kind == VAR_Dynamic_Link);
			if ( vt->kind == VAR_Dynamic_Global ) 
				dvr->con = vt->con;	//ȫ��
			if ( vt->con->point > vt->con->base )	//�ȰѶ���ľ�̬�������ӹ���, ��̬������Ĭ��ֵ
				dvr->DyVarBase::input(vt->con->base, vt->con->point - vt->con->base, true); //ֻ��link, ��ʹȫ����Ҳ��ì�ܡ�
		}
	}
	
	//{int *a =0 ; *a = 0; };
	if ( no_def_fail) {
		TEXTUS_SPRINTF(mess.err_str, "not defined flow_id: %s ", mess.flow_id );
		mess.snap[Pos_ErrCode].input("-99");
		WLOG(WARNING, "Error %s:  %s", mess.snap[Pos_ErrCode].val_p, mess.err_str);
		mess.snap[Pos_ErrStr].input(mess.err_str);
		mk_result(true);	//��������
		return;
	}
	/* ����ʼ  */
	mess.snap[Pos_TotalIns].input( cur_def->ins_all.ic_num);
	if ( cur_def->flow_md[0] )
		mess.snap[Pos_FlowPrint].input( cur_def->flow_md);
	mess.snap[Pos_FlowID].input(mess.flow_id);

	mess.ins_which = 0;
	command_wt.step=0; //ָʾ�ն�׼����ʼ����
	mk_hand();
}

/* ��������� */
enum SUB_RET TranWay::sub_serial_pro(struct ComplexSubSerial *comp)
{
	struct TranIns *trani;
SUB_INS_PRO:
	trani = &(comp->tran_inses[command_wt.pac_which]);
	if ( command_wt.tran_step == Tran_Idle )
	{
		if ( !trani->valid_condition(&mess) )		/* ����������,��ת��һ�� */
		{
			command_wt.pac_which++;	//ָ����һ�����Ĵ���
			if (  command_wt.pac_which == comp->tr_many )
				return Sub_OK;	//�����Ѿ����
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
		return  Sub_Soft_Fail;	//�ű������ƵĴ���, ��ʧ��
		break;

	case INS_Null:
		command_wt.tran_step =Tran_End;
		break;

	case INS_Respond:
		aptus->sponte(&loc_ans_tran);    //�ݻ�Ӧǰ��, ������ҵ�񲻽���
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
				aptus->facio(&loc_pro_ins); //���ҷ���ָ��, �ҽڵ㲻��sponte. ���������, right_status����
				goto TRAN_END;
			}
			command_wt.tran_step = Tran_Working;
			mess.right_subor = trani->up_subor;
			return Sub_Working; 	/* ���ڽ��� */
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
				return Sub_Rcv_Pac_Fail;	//���ǻ������Ĵ��󣬷�map������
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
		return Sub_Valid_Fail;	//����map������
	} else {
		command_wt.pac_which++;	//ָ����һ�����Ĵ���
		command_wt.tran_step = Tran_Idle;
		if (  command_wt.pac_which == comp->tr_many )
			return Sub_OK;	//�����Ѿ����
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
	case 0:	//��ʼ
		if ( !usr_com->valid_condition(&mess) ) {
			NEXT_INS
			break;
		}
		mess.pro_order = usr_com->order;	
		if ( mess.snap[Pos_CurCent].def_var) 
			mess.snap[Pos_CurCent].input((mess.ins_which*100)/cur_def->ins_all.many);

		command_wt.cur = 0;
		mess.willLast = ( usr_com->comp_num == 1 ); //һ���û�������������������ָ��ĳ��ԣ���һ���ɹ�������OK
NEXT_PRI_TRY:
		command_wt.sub_loop = usr_com->complex[command_wt.cur].loop_n; //��ʧ�ܵ����Դ���
LOOP_PRI_TRY:
		command_wt.pac_which = 0;	//����ϵ��, pac�ӵ�0����ʼ
		command_wt.tran_step = Tran_Idle;	//pac����ʼ, 
		command_wt.step++;	//ָ����һ��
	case 1:
		i_ret = sub_serial_pro( &(usr_com->complex[command_wt.cur]));
		switch (i_ret ) {
		case Sub_Soft_Fail: //������ʧ��
			command_wt.sub_loop--;
			if ( command_wt.sub_loop != 0 ) 
			{ //���ԭΪ��0,��Ϊ��,����������0
				command_wt.sub_loop--;
				command_wt.step--;
				goto LOOP_PRI_TRY;
			}
			if ( command_wt.cur < (usr_com->comp_num-1) ) 
			{ //�û������Abort������һ��
				command_wt.cur++;
				command_wt.step--;
				mess.willLast = ( command_wt.cur == (usr_com->comp_num-1) ); //���һ������ָ�������������͵����Զ���ĳ������(��Ӧ��������һЩ���ݣ��������ն˷�Щָ��)
				vt =  mess.snap[Pos_ErrCode].def_var;	//������һ��֮ǰ, �����(�����룩��Ϊ��ʼֵ
				if ( vt ) mess.snap[Pos_ErrCode].DyVarBase::input(vt->con->base, vt->con->point - vt->con->base, true);	
				vt =  mess.snap[Pos_ErrStr].def_var;	//������һ��֮ǰ, �����ʾ��Ϊ��ʼֵ
				if (vt)	mess.snap[Pos_ErrStr].DyVarBase::input(vt->con->base, vt->con->point - vt->con->base, true);
				goto NEXT_PRI_TRY;		//����һ��
			} else {		//���һ������ʧ�ܣ��������ֵ
				ERR_TO_LAST_INS
			}
			break;

		case Sub_OK: //���
			mess.right_status = RT_IDLE;	//�Ҷ���
			WBUG("has completed %d, order %d", mess.ins_which, mess.pro_order);
			NEXT_INS
			break;

		case Sub_Rcv_Pac_Fail: //������Ӧʧ��
		case Sub_Valid_Fail: //У����Ӧʧ��, �ű����ƻ��Ķ��� �� ����
			ERR_TO_LAST_INS
			break;

		case Sub_Working: //��������
			mess.right_status = RT_OUT;
			aptus->facio(&loc_pro_ins);     //���ҷ���ָ��,aptus.facio�Ĵ���������,����Ҫ!! ��Ϊ��������п����յ��ҽڵ��sponte. ע��!!,һ��Ҫע��.
			break;
		}
		break;
	}
}

void TranWay::mk_result(bool fend)
{
	int ret;
	if ( fend )	//����Ϊ�˶Ը���;�жϵ���� 
	{
		mess.ins_which = cur_def->ins_all.many-1;	//���һ���û�ָ��, �����Լ���, ֻ���һ��,ȡ���һ�����Ĵ���
		mess.pro_order = cur_def->ins_all.instructions[mess.ins_which].order;
		command_wt.pac_which = cur_def->ins_all.instructions[mess.ins_which].complex[0].tr_many-1;
		command_wt.tran_step = Tran_Idle; //pac����ʼ,
		ret = sub_serial_pro(&cur_def->ins_all.instructions[mess.ins_which].complex[0]);
		if ( ret <= 0  ) {
			WLOG(EMERG, "bug! last_pac_pro should finished!");
		}
	}
	aptus->sponte(&loc_ans_tran);    //�ƿ��Ľ����Ӧ������̨
	mess.reset();
}
#include "hook.c"
