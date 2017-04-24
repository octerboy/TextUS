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
#define ERROR_RESULT -210
#define ERROR_USER_ABORT -203
#define ERROR_INS_DEF -133

char err_global_str[128]={0};
/* ���״̬, ����, ����������, ���׽����� */
enum LEFT_STATUS { LT_Idle = 0, LT_Working = 3};
/* �ұ�״̬, ����, �������ĵ���Ӧ */
enum RIGHT_STATUS { RT_IDLE = 0, RT_OUT=7, RT_READY = 3};

enum Var_Type {VAR_ErrCode=1, VAR_FlowPrint=2, VAR_TotalIns = 3, VAR_CurOrder=4, VAR_CurCent=5, VAR_ErrStr=6, VAR_FlowID=7, VAR_Dynamic = 10, VAR_Me=12, VAR_Constant=98,  VAR_None=99};
/* ����ּ��֣�INS_Normal����׼��INS_Abort����ֹ */
enum PacIns_Type { INS_None = 0, INS_Normal=1, INS_Abort=2, INS_Null};

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
		unsigned int me_nm_len;
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

		void put_var(struct PVar* att_var)
		{
			kind = att_var->kind;
			c_len = att_var->c_len;
			if ( c_len > 0 ) 
				memcpy(content, att_var->content, c_len );
			att_var->content[c_len] = 0;	
			dynamic_pos = att_var->dynamic_pos;
		};

		struct PVar* prepare(TiXmlElement *var_ele, int &dy_at) //����׼��
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

			if ( strcasecmp(nm, "$FlowID" ) == 0 ) //��������
			{
				dynamic_pos = Pos_FlowID;
				kind = VAR_FlowID;
			}

			if ( var_ele->Attribute("link") )
			{
				dy_link = true;
			}

			if ( kind != VAR_None) goto P_RET; //���ж��壬���ٿ����Dynamic, ���϶��嶼��Dynamic��ͬ����

			if ( (p = var_ele->Attribute("dynamic")) && (*p == 'Y' || *p == 'y') )
			{
				dynamic_pos = dy_at;	//��̬����λ��
				kind = VAR_Dynamic;
				dy_at++;
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
				} else {			//��������ں�׺
					me_nm_len = strlen(&nm[sizeof(ME_VARIABLE_HEAD)-1]);
				}

				if ( me_nm_len >= sizeof ( me_name))	//Me�������ռ�����, 64�ֽ����
					me_nm_len = sizeof ( me_name)-1;
				memcpy(me_name, &nm[sizeof(ME_VARIABLE_HEAD)-1], me_nm_len);
				me_name[me_nm_len] = 0 ;
			}

			if ( kind == VAR_None && c_len > 0) 
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
		char val[512];	//��������. ��ʱ����, �㹻�ռ���
		unsigned long c_len;

		struct PVar *def_var;	//�ļ��ж���ı���

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

	struct MK_Session {		//��¼һ����������еĸ�����ʱ����
		struct DyVar *snap;	//��ʱ�ı���, ����
		int snap_num;
		bool willLast;		//���һ���Դ�. ͨ����һ���û�ָ��ֻ����һ�δ�, ��ֵΪtrue�� ��ʱ�ж�γ���, ��ֵ��Ϊfalse�����һ��Ϊtrue.

		char err_str[1024];	//������Ϣ
		char flow_id[64];
		int pro_order;		//��ǰ����Ĳ������

		LEFT_STATUS left_status;
		RIGHT_STATUS right_status;
		int ins_which;	//�Ѿ��������ĸ�����, ��Ϊ������������±�ֵ
		int iRet;	//�������ս��

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
			{	/* ���Pos_Fixed_Next����Ҫ, Ҫ��Ȼ, ��Щ���еĶ�̬������û�еģ�  */
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

		inline void init(int m_snap_num) //���m_snap_num���Ը�XML��������̬������
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
		if (nm  && strcasecmp(nm, VARIABLE_TAG_NAME) == 0 ) return true;
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

	void put_var(const char *nm, struct PVar *att_var)
	{
		struct PVar *av = look(nm,0);
		if ( av) av->put_var(att_var);
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
			if ( buf && len > 0 ) memcpy(buf, vt->content, len);
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
		TiXmlElement *comp;
		unsigned long l;
		struct PVar  *rt;
		bool will_break= false;
				
		rt = 0;
		/* ac_len�Ӳ�������, �ۼƵ�, command����ԭ���ĺ���, ��������ָ�� */
		for ( comp = ele; comp && !will_break ; comp = comp->NextSiblingElement(tag) )
        	{
			if ( !comp->GetText() ) continue; //û������, �Թ�
			//printf("tag %s  Text %s\n",tag, comp->GetText());
			if ( command ) 
				rt = one_still( comp->GetText(), &command[ac_len], l, loc_v);
			else 
				rt = one_still( comp->GetText(), 0, l, loc_v);
			ac_len += l;
			if ( rt && rt->kind < VAR_Constant )		//����зǾ�̬��, ������Ҫ�ж�, compָ����һ��
				will_break = true;
		}
		//printf("+++++++++++++++ %s \n",tag);
		if (command) command[ac_len] = 0;	//����NULL
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
		return ret;	//���з�����ϣ����������������
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
			if ( !vr_tmp ) 		//���ǳ���, ����Ӧ�ý�����
			{
				if (e_tmp) printf("plain !!!!!!!!!!\n");	//�ⲻӦ��
				continue;
			}

			if ( vr_tmp->kind <= VAR_Dynamic )	//Me������, ��������̬
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
				g_ln = 0 ;
				if ( usr_ele->Attribute(vr_tmp->me_name) ) 	//�û������У���������
				{
					vr2_tmp = g_vars->one_still( usr_ele->Attribute(vr_tmp->me_name), 0, g_ln);
					if ( g_ln > 0 ) cmd_len += g_ln;
					if ( vr2_tmp && vr2_tmp->kind <= VAR_Dynamic )
					{
						dy_num++;
					}
					continue;	//�������ԣ�������Σ����ٿ���Ԫ����
				}

				e2_tmp = usr_ele->FirstChildElement(vr_tmp->me_name);
				while (e2_tmp)
				{
					g_ln = 0 ;
					vr2_tmp = g_vars->all_still(e2_tmp, vr_tmp->me_name, 0, g_ln, n2_ele, 0);
					e2_tmp = n2_ele;
					if ( g_ln > 0 ) cmd_len += g_ln;
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
		dy_cur = -1;	//����ĺ��ȼ�1�����Կ�ʼΪ0

		DY_NEXT_STILL
		now_still = true;

		e_tmp = pac_ele->FirstChildElement(tag);
		while ( e_tmp ) 
		{
ALL_STILL:
			g_ln = 0;
			vr_tmp= g_vars->all_still( e_tmp, tag, cp, g_ln, n_ele, me_vars);
			e_tmp = n_ele;

			if ( g_ln > 0 )	/* �մ�����Ǿ�̬���� */
			{
				DY_STILL(g_ln) //cpָ�����, �������ӣ� �����α겻�䣬��Ϊ��һ��������Me�����ľ�̬, ��Ҫ�ϲ���һ��
			}

			if ( !vr_tmp ) 		//���ǳ���, ����Ӧ�ý�����
			{
				if (e_tmp) printf("plain !!!!!!!!!!\n");	//�ⲻӦ��
				continue;
			}

			if ( vr_tmp->kind <= VAR_Dynamic )	//�ο�������, ��������̬��
			{
				DY_DYNAMIC(vr_tmp->dynamic_pos)	//���о�̬���ݵģ� ��ָ����һ��, �Դ�Ŷ�̬��
				DY_NEXT_STILL
				continue;
			}

			if ( vr_tmp->kind == VAR_Me && vr_tmp->c_len > 0)	//Me������������,1���޺�׺�ģ��������ж����; 2�����ߴ���׺�ģ�ǰ��Ĵ������Ѿ��趨������
			{
				memcpy(cp, vr_tmp->content, vr_tmp->c_len);
				DY_STILL(vr_tmp->c_len) //cpָ�����,�������ӣ� �����α겻�䣬��Ϊ��һ��������Me�����ľ�̬, ��Ҫ�ϲ���һ��
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
						DY_STILL(g_ln) //cpָ�����, �������ӣ� �����α겻�䣬��Ϊ��һ��������Me�����ľ�̬, ��Ҫ�ϲ���һ��
					}

					if ( vr2_tmp && vr2_tmp->kind <= VAR_Dynamic )
					{
						DY_DYNAMIC(vr2_tmp->dynamic_pos)	//���о�̬���ݵģ� ��ָ����һ��, �Դ�Ŷ�̬��
						DY_NEXT_STILL
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
		}
		if ( now_still ) 
			dy_num = dy_cur + 1;	//���һ���Ǿ�̬, dy_cur��δ����
		else
			dy_num = dy_cur;
	};
};

struct CmdRcv {
	int fld_no;	//���յ����, �ж�����ն��壬ÿ��ֻ����һ������.���ԣ��ж�����壬ָ��ͬһ����
	int dyna_pos;	//��̬����λ��, -1��ʾ��̬
	unsigned int start;
	int length;
	unsigned char *must_con;
	unsigned long must_len;
	const char *err_code; //����ֱ�Ӵ��ⲿ�����ļ��õ������ݣ������κδ��������򲻷���Ҫ�����ô˴����롣

	const char *tag;//���磺 reply, sw
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
	int subor;	//ָʾָ����͸���һ���¼�ģ��

	struct CmdSnd *snd_lst;
	int snd_num;

	struct CmdRcv *rcv_lst;
	int rcv_num;

	const char *err_code; //����ֱ�Ӵ��ⲿ�����ļ��õ������ݣ������κδ��� �������ĳ���ͨ�Ŵ��󣬰������Ĳ��ܽ����ġ�ͨ���رա�
	bool counted;		//�Ƿ����
	bool isFunction;	//�Ƿ�Ϊ����

	PacIns() 
	{
		subor = 0;
		snd_lst = 0;
		snd_num = 0;
		rcv_lst = 0;
		rcv_num = 0;

		err_code = 0;
		counted = false;	/* ������ָ�������� */
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
			if ( snd_lst[i].dy_num ==0 ) t_len += snd_lst[i].cmd_len;	//������pacdef��sendԪ�ض����
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
			{	/* ������pacdef��sendԪ�ض����һ��������� */
				memcpy(snd_pac->buf.point, snd_lst[i].cmd_buf, snd_lst[i].cmd_len);
				t_len = snd_lst[i].cmd_len;	
			} else {
				for ( j = 0; j < snd_lst[i].dy_num; j++ )
				{
					/* ��һ���б�������һ��������� */
					if (  snd_lst[i].dy_list[j].dy_pos < 0 ) 
					{
						/* ��̬����*/
						memcpy(&snd_pac->buf.point[t_len], snd_lst[i].dy_list[j].con, snd_lst[i].dy_list[j].len);
						t_len += snd_lst[i].dy_list[j].len;
					} else {
						/* ��̬����*/
						memcpy(&snd_pac->buf.point[t_len], sess->snap[snd_lst[i].dy_list[j].dy_pos].val_p, sess->snap[snd_lst[i].dy_list[j].dy_pos].c_len);
						t_len += sess->snap[snd_lst[i].dy_list[j].dy_pos].c_len;
					}
				}
			}
			snd_pac->commit(snd_lst[i].fld_no, t_len);	//���ȷ��
		}
		return ;
	};

	/* ��ָ�����Ӧ���ģ�ƥ����������,����ʱ�ó��������� */
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
					rlen -= (rply->start-1); //start�Ǵ�1��ʼ
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
			mess->snap[Pos_ErrCode].input(err_code);	//���ܶ����򲻷��ϵ������δ��������룬��ȡ�������Ļ�map�еĶ���
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

		if ((p = def_ele->Attribute("function")) && ( *p == 'y' || *p == 'Y') )	//���Ǻ�����չ�����ֻص�
			isFunction = true;
		else
			isFunction = false;

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
				} else if ( pac_ele->FirstChildElement(tag)) {	// pacdef���ж�����, ��mapx��������, ��Ͳ�Ҫ��
					p_ele->QueryIntAttribute("to", &(snd_lst[i].fld_no));
					snd_lst[i].hard_work_2(pac_ele, usr_ele, g_vars, me_vars);
					i++;
				}
			}
		}
		snd_num = i;	//����ٸ���һ�η��������Ŀ

		/* Ԥ�ý��յ�ÿ�����趨���*/
		rcv_num = 0;
		for (p_ele= def_ele->FirstChildElement(); p_ele; p_ele = p_ele->NextSiblingElement())
		{
			tag = p_ele->Value();
			if ( !tag ) continue;
			if ( strcasecmp(tag, "recv") == 0 || p_ele->Attribute("from"))
			{
				rcv_num++;
				if ( strcasecmp(tag, "recv") == 0 ) continue; /*recv ���ڻ������Ķ����г���.�ڻ��������У�from��Ҳ��һ���ͬrecv */
			} else continue;
			/*�������еķ���Ԫ��Ҳ���� */
			for (e_tmp = pac_ele->FirstChildElement(tag); e_tmp; e_tmp = e_tmp->NextSiblingElement(tag) ) 
				rcv_num++;
			/* �û�����ķ���Ԫ��Ҳ���� */
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
				p_ele->QueryIntAttribute("from", &(rcv_lst[i].fld_no));	/* ���recv����from ....?? */
				p = p_ele->GetText();
				if ( p )
				{
					lnn = strlen(p);
					rcv_lst[i].must_con = new unsigned char[lnn+1];
					rcv_lst[i].must_len = BTool::unescape(p, rcv_lst[i].must_con) ;
					rcv_lst[i].err_code = p_ele->Attribute("error");	//���������в����ϣ���˴�����
				}
				i++;
				if ( strcasecmp(tag, "recv") == 0 )
					continue;	/* recv ���ڻ������Ķ����г���, ����from�ģ����滹Ҫ���� */
			} else continue;
			/* �û�������������еķ���Ԫ��Ҳ����, ������߶�û�У������ﲻ��Ҫ������ */
			some_ele = pac_ele;
ANOTHER:
			for (e_tmp = some_ele->FirstChildElement(tag); e_tmp; e_tmp = e_tmp->NextSiblingElement(tag) ) 
			{
				rcv_lst[i].tag = tag;
				p_ele->QueryIntAttribute("from", &(rcv_lst[i].fld_no));	//�Ե�һ���е��ظ�
				if ( (p = e_tmp->Attribute("name")) )
				{
					vr_tmp = g_vars->look(p, me_vars);	//��Ӧ����, ��̬����, ����������
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
		}	/* ��������Ԫ�صĶ���*/

		type = INS_Normal;
LAST_CON:
		return counted ? 1:0 ;
	};

	void prepare(TiXmlElement *def_ele, TiXmlElement *pac_ele, TiXmlElement *usr_ele, struct PVar_Set *g_vars, struct PVar_Set *me_vars)
	{
		struct PVar *err_var;
		
		err_code = pac_ele->Attribute("error"); //ÿһ�����Ķ���һ�������룬����INS_Abort�����á�
		if (!err_code && def_ele ) err_code = def_ele->Attribute("error"); //һ��INS_Normal�����壬��ôȡ�������ĵĶ���
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
	TiXmlElement *usr_def_entry;//MAP�ĵ��У����û�ָ��Ķ���
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
	struct PVar *set_loc_ref_var(const char *vnm, const char *mid_nm)
	{
		unsigned char buf[512];		//ʵ������, ��������
		char loc_v_nm[128];
		TiXmlAttribute *att; 
		unsigned long len;
		struct PVar *ref_var = 0, *att_var=0;
		const char *att_val;
		
		len = 0;
		ref_var = g_var_set->one_still(vnm, buf, len);	//�ҵ��Ѷ���ο�������
		if ( ref_var)
		{
			for ( att = ref_var->self_ele->FirstAttribute(); att; att = att->Next())
			{
				//�����Լӵ����ر����� sv_set
				TEXTUS_SPRINTF(loc_v_nm, "%s%s.%s", ME_VARIABLE_HEAD, mid_nm, att->Name()); 
				att_val = att->Value();
				if ( !att_val ) continue;
				len = 0;
				/* ���� att->Value() ����ָ����һ�������� */
				att_var = g_var_set->one_still(att_val, buf, len);	//�ҵ��Ѷ���ı���
				if ( att_var ) 
				{
					sv_set.put_var(loc_v_nm, att_var);
				} else {
					if ( len > 0 ) 
						sv_set.put_still(loc_v_nm, buf, len);
				}
			}
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
		TiXmlElement *pac_ele, *def_ele, *spac_ele, *t_ele;
		TiXmlElement *body;	//�û�����ĵ�һ��bodyԪ��
		int which, icc_num=0, i;
		const char *pri_key = usr_def_entry->Attribute("primary");

		sv_set.defer_vars(usr_def_entry); //�������������ȫ�����Ǹ��Զ�����
		for ( i = 0; i  < sv_set.many; i++ )
		{
			me_var = &(sv_set.vars[i]);
			if (me_var->kind != VAR_Me ) continue;		//ֻ����Me����
			if ( pri_key && strcmp(me_var->me_name, pri_key) == 0 ) continue; //���ο��������洦��

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
					ref_var = set_loc_ref_var(ref_nm, me_var->me_name); /* primary����ָ��protect֮���, ʵ���Ͼ���me.protect.*�����Ķ�����������¾ֲ������� */
			}
		}

		TEXTUS_SNPRINTF(pro_nm, sizeof(pro_nm), "%s", "Pro"); //�ȼٶ���������Pro element����������ο��������������¡�
		if ( pri_vnm ) //��������ο�����, �ͼ�����������ο��������ҵ���Ӧ��sub_pro, pri_vnm����$Main֮��ġ�
		{
			ref_var = set_loc_ref_var(pri_vnm, pri_key); /* primary����ָ��protect֮���, ʵ���Ͼ���me.protect.*�����Ķ�����������¾ֲ������� */
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
			if ( !pac_ele->Value() ) continue;
			if ( def_root->FirstChildElement(pac_ele->Value()))	//����ڻ����������ж���
			{
				pac_many++;
			} else if ( (t_ele = map_root->FirstChildElement(pac_ele->Value()) ) )	//�����map���ж���, Ҳ����һ��Ƕ��(�����ں�)
			{
				for ( spac_ele= t_ele->FirstChildElement(); spac_ele; spac_ele = spac_ele->NextSiblingElement())
				{
					if ( !spac_ele->Value() ) continue;
					if ( def_root->FirstChildElement(spac_ele->Value()) )	//����ڻ����������ж���
						pac_many++;
				}
			}
		}
		//ȷ��������
		if ( pac_many ==0 ) return 0;
		pac_inses = new struct PacIns[pac_many];

		which = 0; icc_num = 0;
		for ( pac_ele= sub_pro->FirstChildElement(); pac_ele; pac_ele = pac_ele->NextSiblingElement())
		{
			if ( !pac_ele->Value() ) continue;
			//printf("pac_ele->Value %s\n", pac_ele->Value());
			def_ele = def_root->FirstChildElement(pac_ele->Value());	//����ڻ����������ж���
			if ( def_ele)	//����ڻ����������ж���
			{
				pac_inses[which].prepare(def_ele, pac_ele, usr_ele, g_var_set, &sv_set);
				icc_num += pac_inses[which].hard_work(def_ele, pac_ele, usr_ele, g_var_set, &sv_set);
				which++;
			} else if ((t_ele = map_root->FirstChildElement(pac_ele->Value())))//�����map���ж���, Ҳ����һ��Ƕ��(�����ں�)
			{
				for ( spac_ele= t_ele->FirstChildElement(); spac_ele; spac_ele = spac_ele->NextSiblingElement())
				{
					if ( !spac_ele->Value() ) continue;
					def_ele = def_root->FirstChildElement(spac_ele->Value());	//����ڻ����������ж���
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
	int comp_num; //һ��ֻ��һ������ʱ��Ҫ���Լ���

	int  set_sub( TiXmlElement *usr_ele, struct PVar_Set *vrset, TiXmlElement *sub_serial, TiXmlElement *def_root, TiXmlElement * map_root) //���ض�IC��ָ����
	{
		TiXmlElement *pri;
		const char *pri_nm;
		int ret_ic=0, i;

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
			} else {	//һ���û�������������������ָ��ĳ��ԣ���һ���ɹ�������OK
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
			cv = person_vars.look(var_ele->Attribute("name")); //�����еı���������Ѱ�Ҷ�Ӧ��
			if ( !cv ) continue; 	//�޴˱���, �Թ�
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

		if( !k_root ) k_root = key_ele_default;//prop�е�keyԪ��(��Կ������), �򵱱����������ṩȱʡ��

		if ( (v_nm = per_ele->Attribute("var")))
			load_xml(v_nm, doc_v,  v_root, per_ele->Attribute("var_md5"));
		else
			v_root = per_ele->FirstChildElement("Var");

		if ( !c_root || !k_root || !pac_def_root ) 
			return false;
		if ( c_root)
		{
			if ( k_root ) 
				person_vars.defer_vars(k_root, c_root);	//��������, map�ļ�����
			else
				person_vars.defer_vars(c_root);	//��������, map�ļ�����
			flow_id = c_root->Attribute("flow");
			if ( per_ele->Attribute("md5") )
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

	void put_def(TiXmlElement *prop, const char *vn)	//���˻��������붨��PersonDef_Set
	{
		TiXmlElement *key_ele, *per_ele;
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
	void mk_hand();	//�����ұ�״̬����
	void mk_result();

	struct G_CFG 	//ȫ�ֶ���
	{
		TiXmlElement *prop;
		struct PersonDef_Set person_defs;
		struct Personal_Def null_icp_def;

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
	struct WKBase {
		int step;	//0: just start, 1: doing 
		int cur;
		int pac_which;
		int pac_step;	//0: send, 1: recv
	} command_wt;

	int sub_serial_pro(struct ComplexSubSerial *comp);
	bool call_back;	/* false: һ����ˣ����ҷ���������һ�����ٴ�����Ӧ; 
			true: ���ں����������չ���лص�������sponteʱ�ͼ����أ��ɷ����㴦����Ӧ���ݣ��Ӷ��������Ƕ��̫�
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

	case Notitia::PRO_UNIPAC:    /* �����Կ���̨������ */
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
		if (!call_back) //���ڻص������������ء�
			mk_hand();
		break;

	case Notitia::DMD_END_SESSION:	//�ҽڵ�ر�, Ҫ����
		WBUG("sponte DMD_END_SESSION");
		if ( mess.left_status == LT_Working  )	//�������ƿ�����
		{
			mess.iRet = ERROR_DEVICE_DOWN;
			TEXTUS_SPRINTF(mess.err_str, "device down at %d", pius->subor);
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
		mess.iRet = 0;	//�ٶ�һ��ʼ����OK��
		TEXTUS_STRCPY(mess.err_str, " ");

		cur_def = gCFG->person_defs.look(mess.flow_id); //��һ����Ӧ��ָ��������
		if ( !cur_def ) 
		{
			mess.iRet = ERROR_INS_DEF;	
			cur_def = &(gCFG->null_icp_def);
		}
		/* Ѱ�ұ����������ж�̬��, �����Ƿ���start_pos��get_length��, ���ݶ��帳ֵ��mess�� */
		for ( i = 0 ; i <  cur_def->person_vars.many; i++)
		{
			vt = &cur_def->person_vars.vars[i];
			if ( vt->dynamic_pos >=0 )
			{
				dvr = &mess.snap[vt->dynamic_pos];
				dvr->kind = vt->kind;
				dvr->def_var = vt;
				if ( vt->c_len > 0 )	//�ȰѶ���ľ�̬�������ӹ���, ��̬������Ĭ��ֵ
					dvr->input(&(vt->content[0]), vt->c_len, true);	

				if ( vt->source_fld_no >=0 )
					p = rcv_pac->getfld(vt->source_fld_no, &plen);
				else
					continue;
				if (!p) continue;
				if ( plen > vt->start_pos )	//ƫ�����������ȣ���Ȼ����ȡ��
				{
					plen -= (vt->start_pos-1);	//plenΪʵ����ȡ�ĳ���, start_pos�Ǵ�1��ʼ
					if ( vt->get_length > 0 && vt->get_length < plen) 
						plen = vt->get_length;
					dvr->input( &p[vt->start_pos-1], plen);
				}
				/* ���Դ���ȡֵΪ����, ���ʵ�ʱ���û�и���, ��ȡ����ľ�̬���� */
			}
		}
		if (mess.iRet == ERROR_INS_DEF)
		{
			TEXTUS_SPRINTF(mess.err_str, "not defined flow_id: %s ", mess.flow_id );
			mess.snap[Pos_ErrCode].input(mess.iRet);
			mk_result();	//��������
			goto HERE_END;
		}
		/* ����ʼ  */
		mess.snap[Pos_TotalIns].input( cur_def->ins_all.many);
		if ( cur_def->flow_md[0] )
			mess.snap[Pos_FlowPrint].input( cur_def->flow_md);

		mess.ins_which = 0;
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
			if ( !paci->valid_condition(&mess) )		/* ����������,��ת��һ�� */
			{
				i_ret = 1;
				break;
			}

			hi_req.reset();	//����λ
			paci->get_snd_pac(&hi_req, loc_pro_pac.subor, &mess);
			command_wt.pac_step++;
			i_ret = 0;	/* ������ */
			call_back = paci->isFunction; //���ں��������ص�����������call_back��sponteʱ���ж�
			mess.right_status = RT_OUT;
			aptus->facio(&loc_pro_pac);     //���ҷ���, Ȼ��ȴ�
			if ( call_back ) goto GO_ON_FUNC; //���÷���ʱ��������Ӧ�����ѱ�����������
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
					i_ret = -2;	//����map������
				}
			} else {
				mess.iRet = ERROR_RECV_PAC;
				TEXTUS_SPRINTF(mess.err_str, "fault at %d of %s", mess.pro_order, cur_def->flow_id);
				i_ret = -3;	//���ǻ������Ĵ��󣬷�map������
			}
			break;

		default:
			break;
		}
		break;

	case INS_Abort:
		i_ret = -1;	//�ű������ƵĴ���
		if (paci->err_code ) mess.snap[Pos_ErrCode].input(paci->err_code);
		break;

	case INS_Null:
		if ( paci->valid_condition(&mess) )		/* ����ǰ������, ���жϽ�� */
		{
			if ( paci->valid_result(&mess) )
				i_ret = 1;
			else {
				mess.iRet = ERROR_RESULT;
				TEXTUS_SPRINTF(mess.err_str, "result error at %d of %s", mess.pro_order, cur_def->flow_id);
				if ( !paci->err_code) mess.snap[Pos_ErrCode].input(paci->err_code);
				i_ret = -2;	//����map������
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
		command_wt.pac_which++;	//ָ����һ�����Ĵ���
		command_wt.pac_step = 0;
		if (  command_wt.pac_which == comp->pac_many )
		{
			return 1;	//�����Ѿ����
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

	if ( mess.right_status  ==  RT_READY )	//�ն˿���,���๤����Ԫ��ո�λ
	{
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
NEXT_PRI_TRY:
			command_wt.pac_which = 0;	//����ϵ��, pac�ӵ�0����ʼ
			command_wt.pac_step = 0;	//pac����ʼ, 
			if ( usr_com->comp_num == 1 )
				mess.willLast = true;
			else 
				mess.willLast = false; //һ���û�������������������ָ��ĳ��ԣ���һ���ɹ�������OK
			command_wt.step++;	//ָ����һ��

		case 1:
			i_ret = sub_serial_pro( &(usr_com->complex[command_wt.cur]) );
			if ( i_ret == -1 ) 
			{
				if ( command_wt.cur < (usr_com->comp_num-1) )	//�û������Abort������һ��
				{
					if ( command_wt.cur == (usr_com->comp_num-1) ) //���һ������ָ�������������͵����Զ���ĳ������(��Ӧ��������һЩ���ݣ��������ն˷�Щָ��)
						mess.willLast = true;
					command_wt.cur++;
					command_wt.step--;
					goto NEXT_PRI_TRY;		//����һ��
				} else {		//���һ������ʧ�ܣ��������ֵ
					mess.iRet = ERROR_USER_ABORT;			
					TEXTUS_SPRINTF(mess.err_str, "user abort at %d of %s", mess.pro_order, cur_def->flow_id);
				}
			} else if ( i_ret < 0 )  //�ű����ƻ��Ķ��� �� ����
			{
				mk_result(); 
			} else 	if ( i_ret > 0  ) 
			{
				mess.right_status = RT_READY;	//�Ҷ���
				WBUG("has completed %d, order %d", mess.ins_which, mess.pro_order);
			}
			/*if ( i_ret ==0  ) ���������ڽ���, ���ﲻ���κδ��� */
			break;
		default:
			break;
	} //end of switch command_wt.step

	if ( mess.right_status == RT_READY )	//������һ���û�����
	{
		mess.ins_which++;
		if ( mess.ins_which < cur_def->ins_all.many)
			goto INS_PRO;		// ��һ��
		else 
			mk_result();		//һ�������Ѿ����
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
#include "hook.c"
