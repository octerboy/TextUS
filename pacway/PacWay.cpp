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
#if defined(__APPLE__)
#define COMMON_DIGEST_FOR_OPENSSL
#include <CommonCrypto/CommonDigest.h>
#else
#include <openssl/sha.h>
#endif

int squeeze(const char *p, unsigned char *q)	//�ѿո�ȼ���, ֻ����16�����ַ�(��д), ����ʵ�ʵĳ���
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
#define ERROR_USER_ABORT -203
#define ERROR_INS_DEF -133

char err_global_str[128]={0};
/* ���״̬, ����, ����������, ���׽����� */
enum LEFT_STATUS { LT_Idle = 0, LT_Working = 3};
/* �ұ�״̬, ����, �������ĵ���Ӧ */
enum RIGHT_STATUS { RT_IDLE = 0, RT_OUT=7, RT_READY = 3};

enum Var_Type {VAR_ErrCode=1, VAR_FlowPrint=2, VAR_TotalIns = 3, VAR_CurOrder=4, VAR_CurCent=5, VAR_ErrStr=6, VAR_WillErrPro=7, VAR_Dynamic = 10, VAR_Refer=11, VAR_Me=12, VAR_Constant=98,  VAR_None=99};
/* ����ּ��֣�INS_Normal����׼��INS_Abort����ֹ */
enum PacIns_Type { INS_None = 0, INS_Normal=1, INS_Abort=2};

/* ����SysTime�����ı����������ⲿ�������㣬��������ֻ�����ű�ָ������ */	
#define Pos_ErrCode 1 
#define Pos_FlowPrint 2 
#define Pos_TotalIns 3 
#define Pos_CurOrder 4 
#define Pos_CurCent 5 
#define Pos_ErrStr 6 
#define Pos_WillErrPro 7 
#define Pos_Fixed_Next 8  //��һ����̬������λ��, Ҳ��Ϊ�ű��Զ��嶯̬�����ĵ�1��λ��
#define VARIABLE_TAG_NAME "Variable"
#define ME_VARIABLE_HEAD "me."
	struct PVar
	{
		Var_Type kind;
		const char *name;	//������doc�ĵ�
		int n_len;		//���Ƴ���

		unsigned char content[512];	//��������. ������������������һ�㲻��ͬʱ��
		int c_len;			//���ݳ���
		int dynamic_pos;	//��̬����λ��, -1��ʾ��̬

		int source_fld_no;	//���������ĵ��ĸ���š�		
		int start_pos;		//����������, ʲôλ�ÿ�ʼ
		int get_length;		//ȡ���ٳ��ȵ�ֵ
		
		int dest_fld_no;	//��Ӧ���ĵ�Ŀ�����, 

		char me_name[64];	//Me�������ƣ���ȥ��ͷ�� me. �����ֽ�, ��������׺. �ӱ�����name�и��ƣ����63�ַ�
		int me_nm_len;
		const char *me_sub_name;  //Me������׺���� �ӱ�����name�ж�λ��
		int me_sub_nm_len;

		bool dy_link;	//��̬�����ĸ�ֵ��ʽ��true:ȡ��ַ��ʽ��������; false:���Ʒ�ʽ
		TiXmlElement *self_ele;	/* ����, ����Ԫ�ذ������ֿ���: 1.����������, 
					2.һ��ָ������, ��ָ����Ԫ�ط���ʱ, �緢��һ���õ��ı�����, ��������ʱ, ����Щָ��Ƕ�롣
					*/
		PVar () {
			kind = VAR_None;	//�����Ϊ�Ǳ���
			name = 0;
			n_len = 0;

			c_len = 0;
			memset(content,0,sizeof(content));
			dynamic_pos = -1;	//�Ƕ�̬��

			source_fld_no = -1;
			start_pos = 1;
			get_length = -1;
			
			dest_fld_no = -1;
		
			self_ele = 0;

			memset(me_name, 0, sizeof(me_name));
			me_nm_len = 0;
			me_sub_name = 0;
			me_sub_nm_len = 0;

			dy_link = false;	//��̬�����ĸ�ֵ��ʽΪ���Ʒ�ʽ��
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
			kind = VAR_Constant;	//��Ϊ�ǳ���
		};

		struct PVar* prepare(TiXmlElement *var_ele, int &dy_at) //����׼��
		{
			const char *p, *dy, *nm;
			int i;
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
			/* ����������, ����Ϊ����������� */
			var_ele->QueryIntAttribute("from", &(source_fld_no));
			var_ele->QueryIntAttribute("to", &(dest_fld_no));
			var_ele->QueryIntAttribute("start", &(start_pos));
			var_ele->QueryIntAttribute("length", &(get_length));

			if ( strcasecmp(nm, "$ink" ) == 0 ) //��ǰ�û����ָ��
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

			if ( strcasecmp(nm, "$WillErrPro" ) == 0 ) //��������
			{
				dynamic_pos = Pos_WillErrPro;
				kind = VAR_ErrStr;
			}

			if ( var_ele->Attribute("link") )
			{
				dy_link = true;
			}

			if ( kind != VAR_None) goto P_RET; //���ж��壬���ٿ����Dynamic, ���϶��嶼��Dynamic��ͬ����

			dy = var_ele->Attribute("dynamic");
			if ( dy )
			{
				if ( strcasecmp(dy, "yes") == 0 ) 
				{
					dynamic_pos = dy_at;	//��̬����λ��
					kind = VAR_Dynamic;
					dy_at++;
				} else if ( strcasecmp(dy, "refer") == 0 ) 
				{
					kind = VAR_Refer;
				}
			}

			if ( kind != VAR_None) goto P_RET; //���ж��壬

			/* ���¶�Me�������д��� ����������, ������Ҫ����һ��me.��Щ������ Ҫ�����������鷳 */
			if ( strncasecmp(nm, ME_VARIABLE_HEAD, sizeof(ME_VARIABLE_HEAD)) == 0 ) 
			{
				kind = VAR_Me;
				me_sub_name = strpbrk(&nm[sizeof(ME_VARIABLE_HEAD)], ".");	//��Me���������ҵ�һ���㣬�������Ϊ��׺��.
				if ( me_sub_name )	//������ں�׺
				{
					me_nm_len = me_sub_name - &nm[sizeof(ME_VARIABLE_HEAD)];
					me_sub_name++;	//��Ȼ������㱾���Ǻ�׺��, �Ӻ�һ����ʼ���Ǻ�׺��
					me_sub_nm_len = strlen(me_sub_name);
				} else {			//��������ں�׺
					me_nm_len = strlen(&nm[sizeof(ME_VARIABLE_HEAD)]);
				}

				if ( me_nm_len >= sizeof ( me_name))	//Me�������ռ�����, 64�ֽ����
					me_nm_len = sizeof ( me_name)-1;
				memcpy(me_name, &nm[sizeof(ME_VARIABLE_HEAD)], me_nm_len);
				me_name[me_nm_len] = 0 ;
			}

			if ( kind == VAR_None && p) 
			{	//���������ݵ���û������, �ǾͶ�Ϊ����, ������Ҳ����������, ��Ҫ�𴦶�����
				kind = VAR_Constant;	//��Ϊ�ǳ���
			}

			P_RET:
			return this;
		};

		void put_herev(TiXmlElement *h_ele) //����һ�±��ر���
		{
			const char *p;

			if( (p = h_ele->GetText()) )
			{
				c_len = squeeze(p, content);
				if (kind == VAR_None)	//ֻ��ԭ��û�ж������͵�, ����Ŷ��ɳ���. 
					kind = VAR_Constant;
			}

			h_ele->QueryIntAttribute("from", &(source_fld_no));
			h_ele->QueryIntAttribute("to", &(dest_fld_no));
			h_ele->QueryIntAttribute("start", &(start_pos));
			h_ele->QueryIntAttribute("length", &(get_length));
		};
	};

	struct DyVar	/* ��̬������ �������Ա��ĵ� */
	{	
		Var_Type kind;	//��̬����, 
		int index;	//����, Ҳ�����±�ֵ
		unsigned char *val_p;	//p��ʱָ�������val�� ��Ҳ����ָ�����롢�������.c_len���ǳ��ȡ�
		unsigned char val[512];	//��������. ��ʱ����, �㹻�ռ���
		unsigned long c_len;

		struct PVar *def_var;	//�ļ��ж���ı���

		DyVar () {
			kind = VAR_None;
			index = - 1;
			c_len = 0;
			val_p = 0;

			memset(val, 0, sizeof(val));
		};

		void input(unsigned char *p, unsigned long len)
		{
			if ( def_var->dy_link)
			{
				val_p = p;
				c_len = len;
			} else {
				val_p = &val[0];
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
			TEXTUS_SPRINTF((char*)&val[0], "%d", iv);
			c_len = strlen((char*)&val[0]);
			val[c_len] = 0;
			val_p = &val[0];
		};

		void input(const char *p)
		{
			c_len = strlen(p);
			if ( def_var->dy_link)
			{
				val_p = (unsigned char*)p;
			} else {
				val[c_len] = 0;
				val_p = &val[0];
				memcpy(val, p, c_len);
			}
		};

		void input(const char p)
		{
			c_len = 1;
			val[1] = 0;
			val_p = &val[0];
			val[0] = p;
		};
	};

	struct MK_Session {		//��¼һ����������еĸ�����ʱ����
		struct DyVar *snap;		//��ʱ�ı���, ����
		int snap_num;
		char err_str[1024];		//������Ϣ
		char flow_id[64];
		int pro_order;		//��ǰ����Ĳ������

		LEFT_STATUS left_status;
		RIGHT_STATUS right_status;
		int ins_which;	//�Ѿ��������ĸ�����, ��Ϊ������������±�ֵ
		int iRet;		//�������ս��

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
			{	/* ���Pos_Fixed_Next����Ҫ, Ҫ��Ȼ, ��Щ���еĶ�̬������û�еģ�  *//* ���Pos_Fixed_Next����Ҫ, Ҫ��Ȼ, ��Щ���еĶ�̬������û�еģ�  */
				snap[i].kind = VAR_None;
				snap[i].def_var = 0;
			}
			left_status = LT_Idle;
			right_status = RT_IDLE;
			ins_which = -1;
			err_str[0] = 0;	
			flow_id[0] = 0;
		};

		inline void init(int m_snap_num) //���m_snap_num���Ը�XML��������̬������
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
			snap[Pos_ErrCode].kind = VAR_ErrCode;
			snap[Pos_CurOrder].kind = VAR_CurOrder;
			snap[Pos_CurCent].kind = VAR_CurCent;
			reset();
		};

		~MK_Session ()
		{
			if ( snap ) delete[] snap;
			snap = 0;
		};
	};

/* ��������*/
struct PVar_Set {	
	struct PVar *vars;
	int many;
	int dynamic_at;
	PVar_Set () 
	{
		vars = 0;
		many = 0;
		dynamic_at = Pos_Fixed_Next; //0,�� �Ѿ���$FlowPrint��ռ��
	};

	~PVar_Set () 
	{
		if (vars ) delete []vars;
		vars = 0;
		many = 0;
	};

	bool is_var(const char *nm)
	{
		if (nm  && strlen(nm) == sizeof(VARIABLE_TAG_NAME) && memcmp(nm, VARIABLE_TAG_NAME, sizeof(VARIABLE_TAG_NAME)) == 0 )
			return true;
		return false;
	}

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
			if ( !had_nm )	//���û���Ѷ������, ������
			{
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


	/* �Ҿ�̬�ı���, ���ʵ������ */
	struct PVar *one_still( const char *nm, unsigned char *buf, unsigned long &len, struct PVar_Set *loc_v=0)
	{
		struct PVar  *vt;
		/* ����ʱ�����������, һ�����о�̬���������, ��һ����̬�������� */
		vt = look(nm, loc_v);	//�����Ƿ�Ϊһ������ı�����
		if ( !vt) 
		{
			len = squeeze(nm, buf); //�Ƕ��������, ����ֱ�Ӵ�����, �������һ������
			goto VARET;
		}

		len = 0;
		switch (vt->kind)
		{
		case VAR_Constant:	//��̬��������
			len = vt->c_len;
			if ( buf ) memcpy(buf, vt->content, len);
			break;

		default:
			len = 0;
			break;
		}
		VARET:
		if ( buf ) buf[len] = 0;	//����NULL
		return vt;
	};

	/* nxt ��һ������, ���ڶ��tagԪ�أ���֮��̬���ݺϳɵ� һ������command�С����ڷǾ�̬�ģ����ظ�tagԪ���Ǹ���̬���� */
	struct PVar *all_still( TiXmlElement *ele, const char*tag, unsigned char *command, unsigned long &ac_len, TiXmlElement *&nxt, struct PVar_Set *loc_v=0)
	{
		TiXmlElement *comp = ele;
		unsigned long l;
		struct PVar  *rt;
				
		rt = 0;
		/* ac_len�Ӳ�������, �ۼƵ�, command����ԭ���ĺ���, ��������ָ�� */
		while(comp)
        	{
			if ( command ) 
				rt = one_still( comp->GetText(), &command[ac_len], l, loc_v);
			else 
				rt = one_still( comp->GetText(), 0, l, loc_v);
			ac_len += l;
			comp = comp->NextSiblingElement(tag);
			if ( rt && rt->kind < VAR_Constant )		//����зǾ�̬��, �������ж�
				break;
		}
		command[ac_len] = 0;	//����NULL
		nxt = comp;	//ָʾ��һ������
		return rt;
	};
};

/* �������ƥ��Ӧ���ǲ���Ҫ��� */
struct MatchDst {	//ƥ��Ŀ��
	struct PVar *dst;
	const char *con_dst;
	int len_dst;
	void set_val(struct PVar_Set *var_set, struct PVar_Set *loc_v, const char *p)
	{
		dst = var_set->look(p, loc_v);
		if (!dst )
		{
			if ( p) 
			{	//����������û������, �����������ǳ�����, ������Ҳ����������, ��Ҫ�𴦶�����
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

	struct Match* set_ma(TiXmlElement *mch_ele, struct PVar_Set *var_set, struct PVar_Set *loc_v=0)
	{
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

		if ( dst_num == 0 )	//һ����valueԪ��Ҳû��
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
		bool ret=false;	//�ٶ�û��һ���Ƿ��ϵ�
		for(i = 0; i < dst_num; i++)
		{
			ret = dst_arr[i].valid_val(sess, src);
			if ( ret ) break;	//�ж��ֵ����һ����ͬ���Ͳ��ٱȽ��ˡ�
		}
		if ( !ys_no ) ret = !ret; 	//���û��һ����ͬ, ��ȡ��, ��ͷ���ok��Ҳ����not���е�ֵ��
		return ret;
	};
};

struct Condition {	//һ��ָ���ƥ���б�, ��������������ƥ��
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
		return ret;	//���з�����ϣ����������������
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

struct DyList {
	unsigned char *con;  /* ָ�� cmd_buf�е�ĳ���� */
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
	int fld_no;	//���͵����
	int dy_num;
	struct DyList *dy_list;
	unsigned char *cmd_buf;
	unsigned long cmd_len;
	const char *tag;	/*  ����"component"���������ݣ�ָ���������е�Ԫ�� */

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
		TiXmlElement *e_tmp, *n_ele, *p_ele;
		TiXmlElement *e2_tmp, *n2_ele;
		unsigned long g_ln;
		unsigned char *cp;

		cmd_len = 0;
		cmd_buf = 0;
		dy_num = 0;

		e_tmp = pac_ele->FirstChildElement(tag); 
		while ( e_tmp ) 
		{
			vr_tmp = g_vars->all_still( e_tmp, tag, 0, cmd_len, n_ele, me_vars);
			e_tmp = n_ele;
			if ( !vr_tmp ) 		//���ǳ���, ����Ӧ�ý�����
			{
				if (e_tmp) printf("plain !!!!!!!!!!\n");	//�ⲻӦ��
				continue;
			}

			if ( vr_tmp->kind <= VAR_Dynamic )	//�ο�������, ��������̬
			{
				dy_num++;
				continue;
			}

			if ( vr_tmp->kind == VAR_Me && vr_tmp->c_len > 0)	//Me������������,�޺�׺�ģ��������ж���ģ����ߴ���׺�ġ�
			{
				cmd_len += vr_tmp->c_len;
				continue;
			}

			if ( vr_tmp->kind == VAR_Me && vr_tmp->me_sub_nm_len == 0)	//Me����,���޺�׺��, ����û�������ȡ����Ȼ��û������
			{
				vr2_tmp = 0;
				if ( usr_ele->Attribute(vr_tmp->me_name) ) 	//�û������У���������
				{
					vr2_tmp = g_vars->one_still( usr_ele->Attribute(vr_tmp->me_name), 0, cmd_len);
					if ( vr2_tmp && vr2_tmp->kind <= VAR_Dynamic )
					{
						dy_num++;
					}
					continue;	//�������ԣ�������Σ����ٿ���Ԫ����
				}

				e2_tmp = usr_ele->FirstChildElement(vr_tmp->me_name);
				while (e2_tmp)
				{
					vr2_tmp = g_vars->all_still(e2_tmp, vr_tmp->me_name, 0, cmd_len, n2_ele, 0);
					e2_tmp = n2_ele;
					if ( !vr2_tmp ) 		//���ǳ���, ����Ӧ�ý�����
					{
						if (e2_tmp) printf("plain !!!!!!!!!!\n");	//�ⲻӦ��
						continue;
					}

					if ( vr2_tmp->kind <= VAR_Dynamic )	//�ο�������, ��������̬
					{
						dy_num++;
					}
				}
			}
		}

		cmd_buf = new unsigned char[cmd_len+1];	//���ڷǶ�̬��, cmd_buf��cmd_len�պ�����ȫ��������
		dy_num = dy_num *2+1;	/* dy_num��ʾ���ٸ���̬����, ʵ�ʷֶ����������2���ٶ�1 */
		dy_list = new struct DyList [dy_num];

		cp = &cmd_buf[0]; dy_num = 0;
		e_tmp = pac_ele->FirstChildElement(tag); 
		while ( e_tmp ) 
		{
			dy_list[dy_num].con = cp;
			dy_list[dy_num].len = 0;
ALL_STILL:
			g_ln = 0;
			vr_tmp= g_vars->all_still( e_tmp, tag, cp, g_ln, n_ele, me_vars);
			e_tmp = n_ele;

			if ( g_ln > 0 )	/* �մ�����Ǿ�̬���� */
			{
				dy_list[dy_num].dy_pos = -1;
				cp = &cp[g_ln];	//ָ�����
				dy_list[dy_num].len += g_ln; //��������
			}

			if ( vr_tmp && vr_tmp->kind <= VAR_Dynamic )	//�ο�������, ��������̬��
			{
				if ( g_ln > 0 )	/* �մ�����Ǿ�̬����, �Ѿ����ڸ�dy���� */
					dy_num++;	//ָ����һ��
				dy_list[dy_num].con = 0;
				dy_list[dy_num].len = 0;
				dy_list[dy_num].dy_pos = vr_tmp->dynamic_pos;
				dy_num++;
				continue;
			}

			if ( !vr_tmp ) 		//���ǳ���, ����Ӧ�ý�����
			{
				if (e_tmp) printf("plain !!!!!!!!!!\n");	//�ⲻӦ��
				continue;
			}

			if ( vr_tmp->kind == VAR_Me && vr_tmp->c_len > 0)	//Me������������,1���޺�׺�ģ��������ж����; 2�����ߴ���׺�ģ�ǰ��Ĵ������Ѿ��趨������
			{
				memcpy(cp, vr_tmp->content, vr_tmp->c_len);
				dy_list[dy_num].len += vr_tmp->c_len; //�����ۼ�
				cp = &cp[vr_tmp->c_len];	//ָ�����
				goto ALL_STILL;	//���ﴦ����Ǿ�̬�����ԴӸô�����
			}

			if ( vr_tmp->kind == VAR_Me && vr_tmp->me_sub_nm_len == 0)	//Me����,���޺�׺��, ����û�������ȡ����Ȼ��û������
			{
				vr2_tmp = 0;
				g_ln = 0;
				if (usr_ele->Attribute(vr_tmp->me_name)) //�ȿ���������
				{
					vr2_tmp = g_vars->one_still( usr_ele->Attribute(vr_tmp->me_name), cp, g_ln);
					if ( g_ln > 0 )	/* �մ�����Ǿ�̬���� */
					{
						dy_list[dy_num].dy_pos = -1;
						cp = &cp[g_ln];	//ָ�����
						dy_list[dy_num].len += g_ln; //��������
					}

					if ( vr2_tmp && vr2_tmp->kind <= VAR_Dynamic )
					{
						if ( g_ln > 0 )	/* �մ�����Ǿ�̬����, �Ѿ����ڸ�dy���� */
							dy_num++;	//ָ����һ��
						dy_list[dy_num].con = 0;
						dy_list[dy_num].len = 0;
						dy_list[dy_num].dy_pos = vr_tmp->dynamic_pos;
						dy_num++;
					}
					continue;	//�������ԣ�������Σ����ٿ���Ԫ����
				}
			
				e2_tmp = usr_ele->FirstChildElement(vr_tmp->me_name);
				while (e2_tmp)
				{
					g_ln = 0;
					vr2_tmp = g_vars->all_still(e2_tmp, vr_tmp->me_name, cp, g_ln, n2_ele, 0);
					e2_tmp = n2_ele;
					if ( g_ln > 0 )	/* �մ�����Ǿ�̬���� */
					{
						dy_list[dy_num].dy_pos = -1;
						cp = &cp[g_ln];	//ָ�����
						dy_list[dy_num].len += g_ln; //��������
					}

					if ( !vr2_tmp ) 		//���ǳ���, ����Ӧ�ý�����
					{
						if (e2_tmp) printf("plain !!!!!!!!!!\n");	//�ⲻӦ��
						continue;
					}

					if ( vr2_tmp->kind <= VAR_Dynamic )	//�ο�������, ��������̬
					{
						if ( g_ln > 0 )	/* �մ�����Ǿ�̬����, �Ѿ����ڸ�dy���� */
							dy_num++;	//ָ����һ��
						dy_list[dy_num].con = 0;
						dy_list[dy_num].len = 0;
						dy_list[dy_num].dy_pos = vr_tmp->dynamic_pos;
						dy_num++;
					}
				}
			}
		}
	};
};

struct CmdRcv {
	int fld_no;	//���յ����, �ж�����ն��壬ÿ��ֻ����һ������.���ԣ��ж�����壬ָ��ͬһ����
	int dyna_pos;	//��̬����λ��, -1��ʾ��̬
	int start;
	int length;
	unsigned char *must_con;
	unsigned long must_len;
	const char *err_code; //����ֱ�Ӵ��ⲿ�����ļ��õ������ݣ������κδ��������򲻷���Ҫ�����ô˴����롣

	const char *tag;//���磺 reply, sw
	CmdRcv () {
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
	int subor;	//ָʾָ����͸���һ���¼�ģ��

	struct CmdSnd *snd_lst;
	int snd_num;

	struct CmdRcv *rcv_lst;
	int rcv_num;

	const char *err_code; //����ֱ�Ӵ��ⲿ�����ļ��õ������ݣ������κδ��� �������ĳ���ͨ�Ŵ��󣬰������Ĳ��ܽ����ġ�ͨ���رա�
	struct ComplexSubSerial *complex;	//��������Ϊ0, �򷵻ش�ֵ��ָʾ����������һ���ӹ��̡�
	bool isIcc;

	PacIns() 
	{
		subor = 0;
		snd_lst = 0;
		snd_num = 0;
		rcv_lst = 0;
		rcv_num = 0;

		err_code = 0;
		complex = 0;
		isIcc = false;	/* ������icc_num���� */
		type = INS_None;
	};

	void prepair_snd_pac( PacketObj *snd_pac, int &bor, MK_Session *sess)
	{
		/* ..��get_current���... */
		int i,j;
		unsigned long t_len;

		bor = subor;
		t_len = 0;
		for ( i = 0 ; i < snd_num; i++ )
		{
			for ( j = 0; j < snd_lst[i].dy_num; j++ )
			{
				t_len += snd_lst[i].dy_list[j].len;
			}
		}
		snd_pac->buf.grant(t_len);

		for ( i = 0 ; i < snd_num; i++ )
		{
			t_len = 0;
			for ( j = 0; j < snd_lst[i].dy_num; j++ )
			{
				snd_pac->buf.input(snd_lst[i].dy_list[j].con, snd_lst[i].dy_list[j].len);	//һ���������
				t_len += snd_lst[i].dy_list[j].len;	//һ����ĳ���
			}
			snd_pac->commit(snd_lst[i].fld_no, t_len);	//���ȷ��
		}
		return ;
	};

	/* ��ָ�����Ӧ���ģ�ƥ����������,����ʱ�ó��������� */
	bool pro_rcv_pac(PacketObj *rcv_pac,  struct MK_Session *mess)
	{
		int ii,min_len;
		unsigned char *fc;
		unsigned long rlen;
		struct CmdRcv *rply;

		for (ii = 0; ii < rcv_num; ii++)
		{
			rply = &rcv_lst[ii];
			if ( rply->dyna_pos > 0)
			{
				fc = rcv_pac->getfld(rply->fld_no, &rlen);
				if ( !fc ) 
					goto ErrRet;
				if ( !(rply->must_len == rlen && memcmp(rply->must_con, fc, rlen) == 0 ) ) 
					goto ErrRet;

				if ( rlen >= (rply->start ) )	
				{
					rlen -= (rply->start-1); //start�Ǵ�1��ʼ
					if ( rply->length > 0 && rply->length < rlen) rlen = rply->length;
					mess->snap[rply->dyna_pos].input(&fc[rply->start-1], rlen);
				}
			}
		}
		return true;
ErrRet:
		if ( rply->err_code )
			mess->snap[Pos_ErrCode].input(rply->err_code);
		else
			mess->snap[Pos_ErrCode].input(err_code);	//���ܶ����򲻷��ϵ������δ��������룬��ȡ�������Ļ�map�еĶ���
		return false;
	};

	int hard_work ( TiXmlElement *def_ele, TiXmlElement *pac_ele, TiXmlElement *usr_ele, struct PVar_Set *g_vars, struct PVar_Set *me_vars)
	{
		struct PVar *vr_tmp=0;
		TiXmlElement *e_tmp, *n_ele, *p_ele;
		const char *p;

		int i = 0;
		int lnn;
		const char *tag;

		err_code = pac_ele->Attribute("error"); //ÿһ�����Ķ���һ�������룬����INS_Abort�����á�
		if (!err_code ) err_code = def_ele->Attribute("error"); //һ��INS_Normal�����壬��ôȡ�������ĵĶ���
		if ( strcasecmp( pac_ele->Value(), "abort") )
		{
			type = INS_Abort;
			goto LAST_CON;
		}

		subor=0; def_ele->QueryIntAttribute("subor", &subor);
		if ( def_ele->Attribute("is_icc") )
			isIcc = true;

		/* ��Ԥ�÷��͵�ÿ�����趨���*/
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
				p_ele->QueryIntAttribute("field", &(snd_lst[i].fld_no));
				tag = p_ele->Value();
				snd_lst[i].tag =tag;
				if ( strcasecmp(tag, "send") == 0 ) 
				{
					p = p_ele->GetText();
					if ( p )
					{
						lnn = strlen(p);
						snd_lst[i].cmd_buf = new unsigned char[lnn+1];
						snd_lst[i].cmd_len = squeeze(p, snd_lst[i].cmd_buf);	
					}
				} else {
					snd_lst[i].hard_work_2(pac_ele, usr_ele, g_vars, me_vars);
				}
				i++;
			}
		}

		/* Ԥ�ý��յ�ÿ�����趨���*/
		rcv_num = 0;
		for (p_ele= def_ele->FirstChildElement(); p_ele; p_ele = p_ele->NextSiblingElement())
		{
			p = p_ele->Value();
			if ( !p ) continue;
			if ( strcasecmp(p, "recv") == 0 || p_ele->Attribute("from")) 
			{
				rcv_num++;
				continue;	/* recv ���ڻ������Ķ����г��� */
			}
			/*�������еķ���Ԫ��Ҳ����, ������û����û�У������ﲻ��Ҫ������ */
			for (e_tmp = pac_ele->FirstChildElement(p); e_tmp; e_tmp = e_tmp->NextSiblingElement(p) ) 
				rcv_num++;
			/* �û�����ķ���Ԫ��Ҳ���� */
			for (e_tmp = usr_ele->FirstChildElement(p); e_tmp; e_tmp = e_tmp->NextSiblingElement(p) ) 
				rcv_num++;				
		}
		rcv_lst = new struct CmdRcv[rcv_num];
		for (p_ele= def_ele->FirstChildElement(),i = 0; p_ele; p_ele = p_ele->NextSiblingElement())
		{
			tag = p_ele->Value();
			if ( !tag ) continue;
			if ( strcasecmp(p, "recv") == 0 || p_ele->Attribute("from")) 
			{
				p_ele->QueryIntAttribute("field", &(rcv_lst[i].fld_no));
				rcv_lst[i].tag = tag;
				if ( strcasecmp(tag, "recv") == 0 ) 
				{
					p = p_ele->GetText();
					if ( p )
					{
						lnn = strlen(p);
						rcv_lst[i].must_con = new unsigned char[lnn+1];
						rcv_lst[i].must_len = squeeze(p, rcv_lst[i].must_con);
						rcv_lst[i].err_code = p_ele->Attribute("error");	//���������в����ϣ���˴�����
					}
					i++;
					continue;	/* recv ���ڻ������Ķ����г��� */
				}

				/*�������еķ���Ԫ��Ҳ����, ������û����û�У������ﲻ��Ҫ������ */
				for (e_tmp = pac_ele->FirstChildElement(tag); e_tmp; e_tmp = e_tmp->NextSiblingElement(tag) ) 
				{
					p_ele->QueryIntAttribute("field", &(rcv_lst[i].fld_no));	//�Ե�һ���е��ظ�
					rcv_lst[i].tag =tag;
					if ( (p = e_tmp->Attribute("name")) )
					{
						vr_tmp = g_vars->look(p, me_vars);	//��Ӧ����, ��̬����, ����������
						if (vr_tmp) 
						{
							rcv_lst[i].dyna_pos = vr_tmp->dynamic_pos;
							e_tmp->QueryIntAttribute("start", &(rcv_lst[i].start));
							e_tmp->QueryIntAttribute("length", &(rcv_lst[i].length));
						}
					}
					i++;
				}
				/* �û�����ķ���Ԫ��Ҳ���ϣ�������һ�μ�����ͬ */
				for (e_tmp = usr_ele->FirstChildElement(tag); e_tmp; e_tmp = e_tmp->NextSiblingElement(tag) ) 
				{
					p_ele->QueryIntAttribute("field", &(rcv_lst[i].fld_no));	//�Ե�һ���е��ظ�
					rcv_lst[i].tag =tag;
					if ( (p = e_tmp->Attribute("name")) )
					{
						vr_tmp = g_vars->look(p, me_vars);	//��Ӧ����, ��̬����, ����������
						if (vr_tmp) 
						{
							rcv_lst[i].dyna_pos = vr_tmp->dynamic_pos;
							e_tmp->QueryIntAttribute("start", &(rcv_lst[i].start));
							e_tmp->QueryIntAttribute("length", &(rcv_lst[i].length));
						}
					}
					i++;
				}
			}
		}	/* ��������Ԫ�صĶ���*/

		type = INS_Normal;
LAST_CON:
		set_condition ( pac_ele, g_vars, me_vars);
		return isIcc ? 1:0 ;
	};
};

struct ComplexSubSerial {
	TiXmlElement *usr_def_entry;	//MAP�ĵ��У����û�ָ��Ķ���
	TiXmlElement *sub_pro;		//ʵ��ʹ�õ�ָ�����У�����ͬ�û�ָ�����£������в�ͬ������,����usr_def_entry��ͬ, ��sub_pro��ͬ��

	struct PVar_Set *g_var_set;	//ȫ�ֱ�����
	struct PVar_Set sv_set;		//���������, �������������͵��û�����

	struct PacIns *pac_inses;
	int pac_many;

	TiXmlElement *def_root;	//�������Ķ���
	TiXmlElement *map_root;
	TiXmlElement *usr_ele;	//�û�����

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

	/* �������������protect��ָ���Ԫ������������, ������Ȼ, ����para1,para2֮��ġ� 
		vnm: ��һ���������������ȫ�ֱ��в鲻����������������
		mid_num����Me��ָ����,����Ԫ�ػ�primay������ָ���ģ����������Ϊ: me.mid_num.xx
	*/
	struct PVar *set_loc_ref_var(struct PVar *ref_var, const char *mid_nm)
	{
		TiXmlAttribute *att; 
		char loc_v_nm[128];

		for ( att = ref_var->self_ele->FirstAttribute(); att; att = att->Next())
		{
			//�����Լӵ����ر����� sv_set
			TEXTUS_SPRINTF(loc_v_nm, "%s%s.%s", ME_VARIABLE_HEAD, mid_nm, att->Name()); 
			sv_set.put_still(loc_v_nm, (unsigned char*)att->Value()); //ԭ��û�ж����, ���ﲻ�ḳֵ�ġ�����, ǰ�治���ж������ˡ�
		}
		return ref_var;
	};

	struct PVar *set_loc_ref_var(const char *vnm, const char *mid_nm)
	{
		unsigned char buf[512];		//ʵ������, ��������
		char loc_v_nm[128];
		unsigned long len;
		struct PVar *ref_var = 0;

		ref_var = g_var_set->one_still(vnm, buf, len);	//�ҵ��Ѷ���ο�������
		if ( ref_var)
		{
			set_loc_ref_var(ref_var, mid_nm);
		}

		if ( len > 0 )	//�ҵ���ȫ�ֱ������������ݣ��ӵ������С�
		{
			TEXTUS_SPRINTF(loc_v_nm, "%s%s", ME_VARIABLE_HEAD, mid_nm); 
			sv_set.put_still(loc_v_nm, buf, len);
		}
		return ref_var;
	};

	//ref_vnm��һ���ο�������, ��$Main֮���. �������$Main��ȫ�ֱ������ҵ���Ӧ�Ķ��塣
	int pro_analyze( const char *pri_vnm)
	{
		struct PVar *ref_var, *me_var;
		const char *ref_nm;

		char pro_nm[128];

		TiXmlElement *pac_ele, *def_ele, *e_tmp, *n_ele, *p_ele;
		TiXmlElement *body;	//�û�����ĵ�һ��bodyԪ��
		struct PVar *vr_tmp=0;
		int which, icc_num=0 ;
		int i;

		sv_set.defer_vars(usr_def_entry); //�������������ȫ�����Ǹ��Զ�����, "imply"֮��Ĳ�Ҫ�� 
		for ( i = 0 ; i  < sv_set.many; i++ )
		{
			me_var = &(sv_set.vars[i]);
			if (me_var->kind != VAR_Me ) continue;		//ֻ����Me����
			if ( usr_def_entry->Attribute("primary") && strcmp(me_var->me_name, usr_def_entry->Attribute("primary")) == 0 ) continue; //���ο��������洦��

			if ( me_var->me_sub_name) //�к�׺��, ��Ӧ���ǲο�����
			{
				if ( me_var->c_len > 0 ) continue;	//�����ݾͲ��ٴ����ˡ�
				ref_nm = usr_ele->Attribute(me_var->me_name);	//�ȿ�������Ϊme.XX.yy�е�XX����ref_nm��$Main֮�ġ�
				if (!ref_nm )	//��������, û�������ٿ�Ԫ��
				{
					body = usr_ele->FirstChildElement(me_var->me_name);	//�ٿ�Ԫ��Ϊme.XX.yy�е�XX����ref_nm��$Main֮�ġ�
					if ( body ) ref_nm = body->GetText();
				}
				if (ref_nm )
					ref_var = set_loc_ref_var(ref_nm, me_var->me_name); /* primary����ָ��protect֮���, ʵ���Ͼ���me.protect.*�����Ķ����������Ѿ����¾ֲ������� */
			}
		}

		TEXTUS_SNPRINTF(pro_nm, sizeof(pro_nm), "%s", "Pro"); //�ȼٶ���������Pro element����������ο��������������¡�
		if ( pri_vnm ) //��������ο�����, �ͼ�����������ο��������ҵ���Ӧ��sub_pro, pri_vnm����$Main֮��ġ�
		{
			ref_var = set_loc_ref_var(pri_vnm, usr_def_entry->Attribute("primary")); /* primary����ָ��protect֮���, ʵ���Ͼ���me.protect.*�����Ķ�����������¾ֲ������� */
			if ( ref_var )
			{
				if (ref_var->self_ele->Attribute("pro") ) //�ο�������pro����ָʾ������
				{
					TEXTUS_SNPRINTF(pro_nm, sizeof(pro_nm), "%s%s", "Pro", ref_var->self_ele->Attribute("pro"));						
				}
			}
		}

		sub_pro = usr_def_entry->FirstChildElement(pro_nm);	//��λʵ�ʵ���ϵ��
		if ( !sub_pro ) return 0; //û�������� 

		pac_many = 0;
		for ( pac_ele= sub_pro->FirstChildElement(); pac_ele; pac_ele = pac_ele->NextSiblingElement())
		{
			if ( pac_ele->Value() )	//ֱ����һ��Ԫ�ؾ͵�����ָ����
				pac_many++;
		}
		//ȷ��������
		if ( pac_many ==0 ) return 0;

		pac_inses = new struct PacIns[pac_many];

		which = 0; icc_num = 0;
		for ( pac_ele= sub_pro->FirstChildElement(); pac_ele; pac_ele = pac_ele->NextSiblingElement())
		{
			if ( pac_ele->Value() )
			{
				def_ele = def_root->FirstChildElement(pac_ele->Value());	//����ڻ����������ж���
				if ( def_ele)
				{
					icc_num += pac_inses[which].hard_work(def_ele, pac_ele, usr_ele, g_var_set, &sv_set);
					which++;
				} else if ( map_root->FirstChildElement(pac_ele->Value()) )	//�����map���ж���, Ҳ����һ��Ƕ�׵�������
				{
					pac_inses[which].complex = new ComplexSubSerial;
					pac_inses[which].complex->usr_def_entry = map_root->FirstChildElement(pac_ele->Value());
					pac_inses[which].complex->g_var_set = g_var_set;
					pac_inses[which].complex->def_root = def_root;
					pac_inses[which].complex->usr_ele = pac_ele; //�����Ҫ��ͬ�û�����
					pac_inses[which].complex->map_root = map_root;
					icc_num += pac_inses[which].complex->pro_analyze(0);
					which++;
				}
			}
			pac_ele = pac_ele->NextSiblingElement();
		}
		return icc_num;
	};
};

struct User_Command : public Condition {		//INS_Userָ���
	int order;
	struct ComplexSubSerial *complex;
	int comp_num; //һ��ֻ��һ������ʱ��Ҫ���Լ���

	int  set_sub( TiXmlElement *usr_ele, struct PVar_Set *vrset, TiXmlElement *sub_serial, TiXmlElement *def_root, TiXmlElement * map_root) //���ض�IC��ָ����
	{
		TiXmlElement *pri;
		const char *pri_nm;
		int ret_ic, i;

		//ǰ���Ѿ���������, ����϶���ΪNULL,nm����Command֮��ġ� sub_serial 
		pri_nm = sub_serial->Attribute("primary");
		if ( pri_nm)
		{
			if ( usr_ele->Attribute(pri_nm))
			{
				/* pro_analyze ���ݱ�����, ȥ�ҵ�ʵ�������ı�������(�����ǲο�����)������������ָ����Pro�� */
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
					
			} else {
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
					ret_ic = complex[i].pro_analyze(pri->GetText()); //�����ѡ��ָ�����ͼ����һ��
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

		sub_serial = map_root->FirstChildElement(nm); //�ҵ���ϵ��		
		return sub_serial;
	};

	void put_inses(TiXmlElement *root, struct PVar_Set *var_set, TiXmlElement *map_root, TiXmlElement *pac_def_root)
	{
		TiXmlElement *usr_ele, *sub;
		int mor, cor, vmany, refny, i, i_num ;

		for ( usr_ele= root->FirstChildElement(), refny = 0; 
			usr_ele; usr_ele = usr_ele->NextSiblingElement())
		{
			if ( usr_ele->Value() )
			{
				if (yes_ins(usr_ele, map_root, var_set) )
					refny++;				
			}
		}
		//����ȷ��������
		many = refny ;
		instructions = new struct User_Command[many];
			
		mor = -999999;	//������˳��ſ��ԴӸ�����ʼ
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
					if ( cor <= mor ) continue;	//order������˳��ģ��Թ�
					instructions[vmany].order = cor;
					i_num += instructions[vmany].set_sub(usr_ele, var_set, sub, pac_def_root, map_root);
					mor = cor;
					vmany++;
				}
			}
		}
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

	TiXmlDocument doc_pac_def;	//pacdef�����Ķ���
	TiXmlElement *pac_def_root;

	TiXmlDocument doc_v;	//����Variable����
	TiXmlElement *v_root;
		
	struct PVar_Set person_vars;
	struct INS_Set ins_all;

	char flow_md[64];	//ָ����ָ������
	const char *flow_id;	//ָ������־

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

	bool put_def( TiXmlElement *per_ele, TiXmlElement *key_ele_default)
	{
		const char *ic_nm, *map_nm, *v_nm, *df_nm;

		if ( (df_nm = per_ele->Attribute("pac")))
			load_xml(df_nm, doc_pac_def,  pac_def_root, per_ele->Attribute("pac_md5"));
		else
			pac_def_root = per_ele->FirstChildElement("pac");

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

		if( !k_root ) k_root = key_ele_default;//prop�е�keyԪ��(��Կ������), �򵱱����������ṩȱʡ��

		if ( (v_nm = per_ele->Attribute("var")))
			load_xml(v_nm, doc_v,  v_root, per_ele->Attribute("var_md5"));
		else
			v_root = per_ele->FirstChildElement("Var");

		if ( c_root)
		{
			if ( k_root ) 
				person_vars.defer_vars(k_root, c_root);	//��������, map�ļ�����
			else
				person_vars.defer_vars(c_root);	//��������, map�ļ�����
			flow_id = c_root->Attribute("flow");
			squeeze(per_ele->Attribute("md5"), (unsigned char*)&flow_md[0]);
		}
		set_here(c_root);	//�ٿ�������
		set_here(v_root);	//����������������,key.xml��

		ins_all.put_inses(c_root, &person_vars, k_root, pac_def_root);//�û��������.
		return true;
	};
};
	
struct PersonDef_Set {	//User_Command����֮����
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

	void put_def(TiXmlElement *prop)	//���˻��������붨��PersonDef_Set
	{
		TiXmlElement *key_ele, *per_ele;
		const char *vn = "personalize";
		int kk;
		int dy_at;
		key_ele = prop->FirstChildElement("key"); //����һ��keyԪ��(ָ�������), ��Ϊ����personalize�ṩȱʡ
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

class PacWay: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	void handle_pac();	//���״̬����
	bool sponte( Pius *);
	Amor *clone();

	PacWay();
	~PacWay();

private:
	void h_fail(char tmp[], char desc[], int p_len, int q_len, const char *p, const char *q, const char *fun);
	bool mk_hand();	//�����ұ�״̬����
	void mk_result();

	struct G_CFG 	//ȫ�ֶ���
	{
		TiXmlElement *prop;
		struct PersonDef_Set person_defs;

		int maxium_fldno;		/* ������ */
		int flowID_fld_no;	//����ʶ��, ҵ�������, 

		inline G_CFG() {
			maxium_fldno = 64;
			flowID_fld_no = 3;
		};	
		inline ~G_CFG() { };
	};

	PacketObj hi_req, hi_reply; /* ���Ҵ��ݵ�, �����Ƕ�HMS, ���Ƕ�IC�ն� */
	PacketObj *hipa[3];
	PacketObj *rcv_pac;	/* ������ڵ��PacketObj */
	PacketObj *snd_pac;
	Amor::Pius loc_pro_pac;

	struct MK_Session mess;	//��¼һ���ƿ������еĸ�����ʱ����
	struct Personal_Def *cur_def;	//��ǰ����

	struct G_CFG *gCFG;     /* ȫ�ֹ������ */
	bool has_config;

	inline void deliver(TEXTUS_ORDO aordo);

	struct WKBase {
		int step;	//0: just start, 1: doing 
		int cur;
	} command_wt;

#define SUB_DEPTH 10
	struct CompWKBase {
		struct ComplexSubSerial *comp;
		int which;
	};

	struct SubWKBase {
		struct CompWKBase comps[SUB_DEPTH];
		int depth;	//ָ��ǰ�ģ�
		void reset()
		{
			int i;
			depth = -1;	//һ��ʼû�У�
			for ( i = 0; i < SUB_DEPTH; i++)
			{
				comps[i].comp = 0;
				comps[i].which = 0;
			}
		};

		SubWKBase () 
		{
			reset();
		};
		
		bool push(struct ComplexSubSerial *c)
		{
			depth++; //����һ��λ��
			if (depth >= SUB_DEPTH )
				return false;

			comps[depth].comp = c;
			comps[depth].which = 0;
			return true;
		};

		struct CompWKBase *pop()
		{
			struct CompWKBase *ret;
			depth--;
			if ( depth < 0 )
				return 0;
			return  &comps[depth];
		};

		struct CompWKBase *peek()
		{
			return &comps[depth];
		}
	} sub_wt;

	struct PacWork {
		int step;	//0: send, 1: recv 
	} pac_wt;

	int sub_serial_pro();
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

	case Notitia::PRO_UNIPAC:    /* �����Կ���̨������ */
		WBUG("facio PRO_UNIPAC");
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
		deliver(Notitia::SET_UNIPAC);
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
		mess.init(gCFG->person_defs.max_snap_num);
		deliver(Notitia::SET_UNIPAC);
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
		mk_hand();
		break;

	case Notitia::DMD_END_SESSION:	//�ҽڵ�ر�, Ҫ����
		WBUG("sponte DMD_END_SESSION");
		if ( mess.left_status == LT_Working  )	//�������ƿ�����
		{
			mess.iRet = ERROR_DEVICE_DOWN;
			TEXTUS_SPRINTF(mess.err_str, "device down at %d", pius->subor);
			mess.snap[Pos_ErrCode].input(mess.iRet);
			mk_result();	//����
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

void PacWay::handle_pac()
{
	unsigned char *p;
	int plen=0;
	int i;

	unsigned char *actp;
	unsigned long alen;

	struct PVar  *vt;
	struct DyVar *dvr;

	actp=rcv_pac->getfld(gCFG->flowID_fld_no, &alen);		//ȡ��ҵ�����, ������ʶ
	if ( !actp)
	{
		WBUG("business code field null");
		return;
	}

	switch ( mess.left_status) 
	{
	case LT_Idle:
		/* �������һ���ҵ���� */
		mess.reset();	//�Ự��λ
		snd_pac->reset();
		actp = rcv_pac->getfld(gCFG->flowID_fld_no, &alen);
		if (alen > 63 ) alen = 63;
		memcpy(mess.flow_id, actp, alen);
		mess.flow_id[alen] = 0;

		cur_def = gCFG->person_defs.look(mess.flow_id); //��һ����Ӧ��ָ��������
		if ( !cur_def ) 
		{
			mess.iRet = ERROR_INS_DEF;	
			TEXTUS_SPRINTF(mess.err_str, "not defined flow_id: %s ", mess.flow_id );
			mk_result();	//��������
			goto HERE_END;
		}
		/* ����ʼ  */
		mess.snap[Pos_TotalIns].input( cur_def->ins_all.many);
		if ( cur_def->flow_md[0] )
			mess.snap[Pos_FlowPrint].input( cur_def->flow_md);

		/* Ѱ�ұ����������ж�̬��, �����Ƿ���start_pos��get_length��, ���ݶ��帳ֵ��mess�� */
		for ( i = 0 ; i <  cur_def->person_vars.many; i++)
		{
			vt = &cur_def->person_vars.vars[i];
			if ( vt->dynamic_pos >=0 )
			{
				dvr = &mess.snap[vt->dynamic_pos];
				dvr->kind = vt->kind;
				dvr->def_var = vt;
				if ( vt->c_len > 0 )	//�ȰѶ���ľ�̬���ݿ�һ��, ��̬������Ĭ��ֵ
				{
					dvr->input(&(vt->content[0]), vt->c_len);
				}
				if ( vt->source_fld_no >=0 )
					p = rcv_pac->getfld(vt->source_fld_no, &plen);
				else
					continue;
				if (!p) continue;
				if ( plen > vt->start_pos )	//ƫ�����������ȣ���Ȼ����ȡ��
				{
					plen -= (vt->start_pos-1);	//plenΪʵ����ȡ�ĳ���, start_pos�Ǵ�1��ʼ
					if ( vt->get_length > 0 && vt->get_length < plen) plen = vt->get_length;
					dvr->input( &p[vt->start_pos-1], plen);
				}
				/* ���Դ���ȡֵΪ����, ���ʵ�ʱ���û�и���, ��ȡ����ľ�̬���� */
			}
		}
		mess.ins_which = 0;
		mess.iRet = 0;	//�ٶ�һ��ʼ����OK��
		TEXTUS_STRCPY(mess.err_str, " ");
		mess.left_status = LT_Working;
		mess.right_status = RT_READY;	//ָʾ�ն�׼����ʼ����,

	//{int *a =0 ; *a = 0; };
		mk_hand();
HERE_END:
		break;

	case LT_Working:	//������, ����Ӧ����
		WLOG(WARNING,"still working!");
		break;

	default:
		break;
	}
	return;
}

/* ��������� */
int PacWay::sub_serial_pro()
{
	int i_ret=0, s_ret=0;
	struct PacIns *paci;
	struct CompWKBase *wk = sub_wt.peek();

SUB_INS_PRO:
	paci = &(wk->comp->pac_inses[wk->which]);
	i_ret = 1;
	switch ( paci->type)
	{
	case INS_Normal:
		switch ( pac_wt.step )
		{
		case 0:
			if ( !paci->valid_condition(&mess) )		/* ����������,��ת��һ�� */
			{
				i_ret = 1;
				break;
			}
			if ( paci->complex ) 	/* Ҫִ����һ������ */
			{
				sub_wt.push(paci->complex);
				return sub_serial_pro();
			}

			hi_req.reset();	//����λ
			paci->prepair_snd_pac(&hi_req, loc_pro_pac.subor, &mess);
			
			pac_wt.step++;
			i_ret = 0;	/* ������ */
			break;

		case 1:
			if ( paci->pro_rcv_pac(&hi_reply, &mess) ) 
				i_ret = 1;
			else {
				mess.iRet = ERROR_RECV_PAC;
				TEXTUS_SPRINTF(mess.err_str, "fault at %d of %s", mess.pro_order, cur_def->flow_id);
				i_ret = -2;	//���ǻ������Ĵ��󣬷ǽű�������
			}
			break;

		default:
			break;
		}
		break;

	case INS_Abort:
		i_ret = -1;	//�ű������ƵĴ���
		mess->snap[Pos_ErrCode].input(paci->err_code);
		break;

	default :
		break;
	}

	if ( i_ret > 0 )
	{
		wk->which++;	//ָ����һ�����Ĵ���
		pac_wt.step = 0;
		if ( wk->which == wk->comp->pac_many )
		{
			//�������Ѿ�ִ����ɣ�����Ƕ�׵ġ�
			wk = sub_wt.pop();
			if (wk)		//��Ƕ���г���
				goto SUB_INS_PRO;
			else 
				s_ret = 1;	//�����Ѿ����
		} else {
			goto SUB_INS_PRO;
		}
	} else if ( i_ret <= 0 ) {	//���ڽ�����,ָ���ѱ�, �����Ѿ���������
		s_ret = i_ret;
	}

	//if ( s_ret == -1 ) {int *a =0 ; *a = 0; };
	return s_ret;
}

bool PacWay::mk_hand()
{
	struct User_Command *usr_com;
	int i_ret;
INS_PRO:
	usr_com = &(cur_def->ins_all.instructions[mess.ins_which]);
	mess.pro_order = usr_com->order;	

	if ( mess.right_status  ==  RT_READY )	//�ն˿���,���๤����Ԫ��ո�λ
	{
		pac_wt.step = 0;
		command_wt.step=0;
		command_wt.cur = 0;
	}
	
	//{int *a =0 ; *a = 0; };
	switch ( command_wt.step)
	{
		case 0:	//��ʼ
			if ( !usr_com->valid_condition(&mess) )
			{
				mess.right_status = RT_READY ;	//�Ҷ���, ������һ������
				break;
			}
			command_wt.cur = 0;
NEXT_PRI:
			sub_wt.reset();
			if (!sub_wt.push(&(usr_com->complex[command_wt.cur])))
			{
				mess.iRet = ERROR_COMPLEX_TOO_DEEP;			
				TEXTUS_SPRINTF(mess.err_str, "too many nested complex at %d of %s", mess.pro_order, cur_def->flow_id);
				goto ErrPro;
			}
			pac_wt.step = 0;
			mess.snap[Pos_WillErrPro].input('0');	//ָ���ⲿ�ű�����ʹʧ��Ҳ��������Ӧ���̡���Ϊ����һ���м䲽��			
			command_wt.step++;

		case 1:	
			if ( command_wt.cur == (usr_com->comp_num-1) )
				mess.snap[Pos_WillErrPro].input('1');	//ָ���ⲿ�ű������ʧ�ܣ��͵�����Ӧ���̣����򲻵��á�

			i_ret = sub_serial_pro();
			if ( i_ret == -1 && command_wt.cur < (usr_com->comp_num-1) )	//�ű����ƵĴ��������һ��
			{	
				command_wt.cur++;
				command_wt.step--;
				goto NEXT_PRI;		//����һ��
			}

			if ( i_ret == -1 ) 
			{
				mess.iRet = ERROR_USER_ABORT;			
				TEXTUS_SPRINTF(mess.err_str, "user abort at %d of %s", mess.pro_order, cur_def->flow_id);
			}

			if ( i_ret < 0 ) goto ErrPro; //�ű����ƻ��Ķ��� �� ����
			if ( i_ret ==0  ) goto INS_OUT; //���������ڽ���, �����ж�
			mess.right_status = RT_READY;	//�Ҷ���, ������һ��ָ��
//{int *a =0 ; *a = 0; };
			break;
		default:
			break;
	}

	if (mess.right_status == RT_READY )	//�½�һ���û���������
	{
		WLOG(CRIT, "has completed %d, order %d", mess.ins_which, mess.pro_order);
		mess.ins_which++;
		if ( mess.ins_which < cur_def->ins_all.many)
			goto INS_PRO;		// ��һ��
		else 
			mk_result();		//һ�������Ѿ����
	}
HAND_END:
	return true;
INS_OUT:
	mess.right_status = RT_OUT;
	aptus->facio(&loc_pro_pac);     //���ҷ���, Ȼ��ȴ�, ������ڱ�????
	return true;

ErrPro: //�д���֮
	if ( mess.iRet != 0 ) //�д�����
	{
		mk_result();
	} else {
		WBUG("bug!! CardErrPro HsmErrPro no error\n");	//ha,   BUG....
	}
	goto HAND_END;
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

	/* �ӱ������弯�У�������̬�ĺͶ�̬�ģ������õ���Ӧ������ */
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
	aptus->sponte(&loc_pro_pac);    //�ƿ��Ľ����Ӧ������̨
	mess.reset();
}

/* ��������ύ */
inline void PacWay::deliver(TEXTUS_ORDO aordo)
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

	default:	
		break;
	}
	return ;
}

#include "hook.c"


