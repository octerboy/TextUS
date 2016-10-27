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

int squeeze(const char *p, char q[])	//�ѿո�ȼ���, ֻ����16�����ַ�(��д), ����ʵ�ʵĳ���
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

unsigned short SW_OK=0x9000;	//SW1 SW2 = 9000, ��ô��, ����������������, ˳��һ��
char err_global_str[128]={0};
/* ���״̬, ����, ����������, ��ʼ����, �ƿ��� */
enum LEFT_STATUS { LT_IDLE = 0, LT_INITING = 2, LT_MKING = 3};

/* �ұ�״̬, ����, ICָ���, HSMָ���, �ն˲����� */
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

	struct PromptOp :public TermOPBase {		//������ʾ����
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

/* ����SysTime�����ı����������ⲿ�������㣬��������ֻ�����ű�ָ������ */	
#define Pos_FlowPrint 1 
#define Pos_TotalIns 2 
#define Pos_Fixed_Next 3  //��һ����̬������λ��, Ҳ��Ϊ�ű��Զ��嶯̬�����ĵ�1��λ��

	struct PVar: public PVarBase 
	{
		int start_pos;		//�����뱨����, ʲôλ�ÿ�ʼ
		int get_length;		//ȡ���ٳ��ȵ�ֵ
		int source_fld_no;	//��Դ��š�
		int dest_fld_no;	//Ŀ�����, 

		TiXmlElement *me_ele;	/* ����, ����Ԫ�ذ������ֿ���: 1.����������, 
					2.һ��ָ������, ��ָ����Ԫ�ط���ʱ, �緢��һ���õ��ı�����, ��������ʱ, ����Щָ��Ƕ�롣
				*/

		PVar () {
			kind = VAR_None;	//�����Ϊ�Ǳ���
			name = 0;
			n_len = 0;
			c_len = 0;
			memset(content,0,sizeof(content));

			start_pos = 1;
			get_length = 999 ;
			source_fld_no = -1;
			dest_fld_no = -1;
			dynamic_pos = -1;	//�Ƕ�̬��
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
			kind = VAR_Constant;	//��Ϊ�ǳ���
		};

		struct PVar* prepare(TiXmlElement *var_ele, int &dy_at) //����׼��
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
			/* ����������, ����Ϊ����������� */
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

	struct DyVar: public DyVarBase {	//��̬����
		int dest_fld_no;	//Ŀ�����, 
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

	struct MK_Session {		//��¼һ���ƿ������еĸ�����ʱ����
		struct DyVar *snap;//��ʱ�ı���, ����
		int snap_num;
		char err_str[1024];	//������Ϣ
		char station_str[1024];	//����վ��Ϣ

		char flow_id[64];
		int pro_order;		//��ǰ����Ĳ������
		char bad_sw[32];

		LEFT_STATUS left_status;
		RIGHT_STATUS right_status;
		int ins_which;		//�Ѿ��������ĸ�ָ��, ��Ϊ������������±�ֵ
		int iRet;		//�ƿ��������

		inline MK_Session ()
		{
			snap=0;
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
				snap[i].kind = VAR_None;/* ���Pos_Fixed_Next����Ҫ, Ҫ��Ȼ, ��ЩUID�ȹ��еĶ�̬������û�еģ�  */
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

/* ��������*/
struct PVar_Set {	
	struct PVar *vars;
	int many;
	int dynamic_at;
	char var_nm[16];
	PVar_Set () 
	{
		vars = 0;
		many = 0;
		dynamic_at = Pos_Fixed_Next; //0,�� �Ѿ���$FlowPrint��ռ��
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

	void defer_vars(TiXmlElement *map_root, TiXmlElement *icc_root=0) //����һ�±�������
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

	/* �Ҿ�̬�ı���, ���ʵ������ */
	struct PVar *one_still( const char *nm, char buf[], int &len, struct PVar_Set *loc_v=0)
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
			memcpy(buf, vt->content, len);
			break;

		default:
			len = 0;
			break;
		}
		VARET:
		buf[len] = 0;	//����NULL
		return vt;
	};

	/* nxt ��һ������, ���ڶ��tagԪ�أ���֮��̬���ݺϳɵ� һ������command�С����ڷǾ�̬�ģ����ظ�tagԪ���Ǹ���̬���� */
	struct PVar *all_still( TiXmlElement *ele, const char*tag, char command[], int &ac_len, TiXmlElement *&nxt, struct PVar_Set *loc_v=0)
	{
		TiXmlElement *comp = ele;
		int l;
		struct PVar  *rt;
				
		rt = 0;
		/* ac_len�Ӳ�������, �ۼƵ�, command����ԭ���ĺ���, ��������ָ�� */
		while(comp)
        	{
			rt = one_still( comp->GetText(), &command[ac_len], l, loc_v);
			ac_len += l;
			comp = comp->NextSiblingElement(tag);
			if ( rt && rt->kind < VAR_Constant )		//����зǾ�̬��, �������ж�
				break;
		}
		command[ac_len] = 0;	//����NULL
		nxt = comp;	//ָʾ��һ������
		return rt;
	};

	/* ��һ�У��ж���̬�����ģ������ݣ������ʵʱ�����ݡ�*/
	struct PVar *get_var_one(const char *nm, char buf[], int &len, struct MK_Session *sess, struct PVar_Set *loc_v=0)
	{
		struct PVar  *vt;
		struct DyVar *dvr;
				
		vt = look(nm, loc_v);	//�����Ƿ�Ϊһ������ı�����
		if ( !vt) 
		{
			len = squeeze(nm, buf); //�Ƕ��������, ����ֱ�Ӵ�����
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
		buf[len] = 0;	//����NULL
		return vt;
	};

	/* ���ڶ��tagԪ�أ���ʵʱ�����ݺϳɵ� һ������command�� */
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
		command[tlen] = 0;	//����NULL
		return rt;
	};

	int get_neo_dynamic_pos ()
	{
		dynamic_at++;
		return (dynamic_at-1);
	};
};
/* ָ������֣�һ���Ǵӱ��Ķ����������INS_Ori������һ���Ǵ�INS_Ori����϶�������INS_User */
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
	unsigned short another_sw;	//����9000֮�⣬ ��һ���������sw1sw2
	unsigned short another_sw2;	//����9000֮�⣬ ��һ���������sw1sw2
	unsigned short not_sw;		//�������SW, ����9000

	const char *another_sw_str;	//����9000֮�⣬ ��һ���������sw1sw2
	const char *another_sw2_str;	//����9000֮�⣬ ��һ���������sw1sw2
	const char *not_sw_str;		//�������SW, ����9000
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
		int dynamic_pos;	//��̬����λ��, -1��ʾ��̬
		int start;
		int length;
		PlainReply () {
			dynamic_pos = -1;
			start =1;
			length = 500;
		};
	} ;

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
	bool dynamic ;		//�Ƿ�̬
	
	const char *tag;
	TiXmlElement *component;	//ָ���ĵ��еĵ�һ��componentԪ��

	char cmd_buf[2048];
	int cmd_len;
	struct PlainReply *vres;	//��Ӧ����ŵı�����
	int reply_num;

	CmdBase () {
		dy_num = 0;
		dy_list = 0;
		vres = 0;
		reply_num = 0;
		dynamic = false;
	};

	void  get_current( char *buf, int &len, MK_Session *sess)
	{	/* ȡʵʱ��ָ������ */
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
				vr_tmp = var_set->look(p, o_set);	//��Ӧ����, ��̬����, ����������
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
			if ( !vr_tmp ) 		//���ǳ���, ����Ӧ�ý�����
			{
				if (e_tmp) printf("plain !!!!!!!!!!\n");	//�ⲻӦ��
				continue;
			}

			if ( vr_tmp->kind <= VAR_Dynamic )	//�ο�������, ��������̬
			{
				dynamic = true;		//��̬��
				dy_num++;
			}
		}

		if ( !dynamic )	//���ڷǶ�̬��, cmd_buf��cmd_len�պ�����ȫ��������
			goto NextPro;

		dy_num = dy_num *2+1;	/* dy_num��ʾ���ٸ���̬����, ʵ�ʷֶ����������2���ٶ�1 */
		dy_list = new struct DyList [dy_num];
		cp = &cmd_buf[0]; dy_num = 0;
		n_ele = e_tmp = component;
		while ( e_tmp ) 
		{
			dy_list[dy_num].con = cp;
			dy_list[dy_num].len = 0;
			vr_tmp= var_set->all_still( e_tmp, tag, dy_list[dy_num].con, dy_list[dy_num].len, n_ele, o_set);
			e_tmp = n_ele;

			if ( dy_list[dy_num].len > 0 )	/* ǰ����һ����̬���� */
			{
				dy_list[dy_num].dy_pos = -1;
				cp = &cp[dy_list[dy_num].len];	//ָ�����
				dy_num++;
			}

			if ( vr_tmp && vr_tmp->kind <= VAR_Dynamic )	//�ο�������, ��������̬
			{
				dy_list[dy_num].con = 0;
				dy_list[dy_num].len = 0;
				dy_list[dy_num].dy_pos = vr_tmp->dynamic_pos;
				dy_num++;
			}
			if ( !vr_tmp ) 		//���ǳ���, ����Ӧ�ý�����
			{
				if (e_tmp) printf("plain !!!!!!!!!!\n");	//�ⲻӦ��
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
		
		if ( !vres || reply_num <=0 ) 	//�����ж�̬�����, ����ͱ���������
			return ;
		for (ii = 0; ii < reply_num; ii++)
		{
			if ( vres[ii].dynamic_pos > 0)
			{
				struct PlainReply *rply = &vres[ii];
				struct DyVar *dv= &mess->snap[ rply->dynamic_pos];
				int min_len;
				if ( rlen >= (rply->start ) )	//start�Ǵ�1��ʼ
				{
					if ( (rply->length + rply->start-1) > rlen  )
						min_len = (rlen - rply->start+1) ;	//Ҫȡ�ĳ��ȴ��ڷ��صĳ���
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
	{	/* ����ICָ�� */
		unsigned char slot;	//����, Ĭ��'0',�û���

		/* set, ����ָ��ĵ���. �����DesMac��������, �о��������, ����o_set��;
		   ����ڴ�ָ���Command, ��û��o_set,�������Ǳ���Ҫ���. 
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
			} else if (o_set) { //�����������û��
				vr_tmp = o_set->look("me.slot", 0);
				if ( vr_tmp )	//Ӧ�����е�
				{
					if ( vr_tmp->c_len > 0 )
					{
						slot = atoi(vr_tmp->content);
					}
				}
			}
			allow_sw(ele, o_set);	//���������SW
		};
	};

	struct HsmIns: public CmdBase
	{
		int fail_retry_num;	//ʧ�����Դ�����Ĭ��Ϊ��
		const char *location;	//HSM����, ��һ̨?
		const char *ok_ans_head;
		int ok_ans_head_len;

		const char *err_head;
		int err_head_len;

		void set ( TiXmlElement *ele, struct PVar_Set *var_set, struct PVar_Set *loc_v=0)
		{
			const char *p, *q;
			location = 0;

			set_cmd (ele, var_set, loc_v);
			if ( (p = ele->Attribute("location")) )	//p����ָ��һ�����������: me.location, me.key.para9��
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

/* �ⲿ�������ã� Ӧ�ò���*/
	struct CallFun: public Condition {
		const char *lib_nm, *fun_nm;
		TiXmlElement *component;	//ָ���ĵ��еĵ�һ��componentԪ��

		TMODULE ext_mod;
		IWayCallType call;

		int pv_many;	//
		struct PVarBase **paras; //����ָ������
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
						paras[i]->kind = VAR_Constant;	//��Ϊ����������
						paras[i]->c_len = squeeze(p, paras[i]->content);
					}
					i++;
				}
			}
			pv_many = i;	//����ٸ���һ�α�����
			set_condition ( ele, var_set, loc_v);
		};

		int callfun(struct MK_Session *mess)
		{
			int i, ret = 1;
			if ( !snaps ) {	
				/* ��mess�еĶ�̬����, ���������� 
				mess�Ķ�̬������, ���������������ܶ���������. ����, �ڶ���׶�, �޷�֪�����յ���.
				���, ������ʱ, ����ռ�.
				Ϊʲô����messֱ�Ӵ�������?
				Ϊ��ֻ����̬���Ļ�����Ϣ��������,
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
/* �ⲿ�����������*/

	struct Base_Command {		//����ָ��壬ֻ�������֡������������У�ָ�����٣����Բ���Ҫorder�ˡ�
		enum Command_Type type;	//����, ����union����, 
		struct PlainIns *plain_p;
		struct HsmIns *hsm_p;
		struct CallFun *fun_p;

		int  set ( TiXmlElement *ele, struct PVar_Set *vrset,  struct PVar_Set *o_set) //����ICָ����
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

		int put_inses(TiXmlElement *root, struct PVar_Set *var_set, struct PVar_Set *o_set) //����������ICָ����
		{
			TiXmlElement *b_ele;
			int which, refny=0, icc_num=0 ;

			/* ����һ�£��ֲ��� */

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
			//ȷ��������
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
		struct INS_SubSet *si_set;	//������
		TiXmlElement *sub_ins_entry;	//MAP�ĵ���������Ԫ��

		TiXmlDocument var_doc;
		TiXmlElement *var_root;
		struct PVar_Set sv_set;	//���������, ֻ�ж�DesMac֮��Ĳ���

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
			if ( !root ) 	//û�����������, 
				return 0;
			if (!si_set)
				si_set = new struct INS_SubSet();
			return si_set->put_inses(root, all_set, sub_set); //����������ָ����
		};
	
		void def_sub_vars(const char *xml) //����һ�±�������
		{
			var_doc.Parse(xml);
			var_root = var_doc.RootElement();
			sv_set.defer_vars(var_root);
		};

		void sub_allow_sw( TiXmlElement *ele)
		{
			allow_sw(ele);	//���������SW
			sv_set.put_still("me.sw", another_sw_str);
			sv_set.put_still("me.sw2", another_sw2_str);
			sv_set.put_still("me.not_sw", not_sw_str);
			sv_set.put_still("me.slot",  ele->Attribute("slot")); 
		};

		/* �ο��ͱ������������е�׼��, ��ȫ�ֱ������з�������һ���ο�����, �ٸ�ֵ�������������
		   loc_rf_nm �Ǿ���������е���, ��"protect", "refer", "key"��
		*/
		void ref_prepare_sub(const char *ref_nm,  struct PVar *&ref_var, struct PVar_Set *var_set, const char *loc_rf_nm)
		{
			char buf[512];		//ʵ������, ��������
			int len;
			struct PVar *vr_tmp;
			struct PVar *loc_var;
			int i;

			char att_name[32];
			const char *at_val;

			if ( ref_nm )
			{
				ref_var = var_set->one_still(ref_nm, buf, len);	//�ҵ��Ѷ��������
				if ( ref_var )
				{
					for ( i = 1; i <= VAR_SUB_NUM; i++)		//�����ӱ���ֵ�����ϣ���������̬
					{
						TEXTUS_SPRINTF(att_name, "para%d", i);
						at_val = ref_var->me_ele->Attribute(att_name);
						if ( !at_val ) continue;		//û���ӱ���������һ��
								
						TEXTUS_SPRINTF(att_name, "me.%s.para%d", loc_rf_nm, i); //�������
						vr_tmp = var_set->one_still(at_val, buf, len);	//at_val�Ǹ�����, �����Ƕ�̬
						if ( vr_tmp && vr_tmp->kind <= VAR_Dynamic ) 
						{	//�������Ƕ�̬, �翨��, ����������Ϊ��̬, ��ָ���λ��
							loc_var = sv_set.look(att_name);
							loc_var->dynamic_pos = vr_tmp->dynamic_pos;
							loc_var->kind = vr_tmp->kind;
						} else 
							sv_set.put_still(att_name, buf, len);
					}
					at_val = ref_var->me_ele->Attribute("location");	//�ο���������λ������
					TEXTUS_SPRINTF(att_name, "me.%s.location", loc_rf_nm);
					sv_set.put_still(att_name, at_val);
				}
			}
		};

		/* ������������, �����ο������й� */
		void get_entry(struct PVar *ref_var, TiXmlElement *map_root, const char *entry_nm)
		{
			char pro_nm[128];
			if ( ref_var && ref_var->me_ele->Attribute("pro") ) //proָʾ�����еı�����
			{
				TEXTUS_SNPRINTF(pro_nm, sizeof(pro_nm), "%s%s", entry_nm, ref_var->me_ele->Attribute("pro"));
				sub_ins_entry = map_root->FirstChildElement(pro_nm);
			} else 
				sub_ins_entry = map_root->FirstChildElement(entry_nm);
		};
		/* �������������ȫ�ֱ��ش��� */
		void get_loc_var ( TiXmlElement *ele, const char *att_nm,  struct PVar_Set *var_set, const char *loc_var_nm, const char *default_val)
		{
			const char *att_val;
			struct PVar *vr_tmp, *h_var;
			char h_buf[512];
			int h_len;
			att_val = ele->Attribute(att_nm);
			if (att_val && strlen(att_val) > 0 ) 
			{
				h_var = var_set->one_still(att_val, h_buf, h_len);	//�������ֵΪ������, Ҫ�ҵ��������
				if ( h_var ) 
				{
					if ( h_var->kind <= VAR_Dynamic ) 
					{
						vr_tmp = sv_set.look(loc_var_nm);	//loc_var_nm�������б��ر�����
						vr_tmp->dynamic_pos = h_var->dynamic_pos;	//���ر�����ӳ��ȫ����̬������
						vr_tmp->kind = h_var->kind;
					} else
						 sv_set.put_still(loc_var_nm, h_var->content);	//�Ͱ�ȫ�־�̬��������ȡ����
				} else 
					sv_set.put_still(loc_var_nm, att_val);	//���ر������ݾ�ȡ����ֵ��
			} else 
				sv_set.put_still(loc_var_nm, default_val);	//������Ĭ��ֵ
		};

		/* ���������������ȫ�ֱ��ش��� */
		void get_loc_var ( TiXmlElement *ele, const char *att_nm,  struct PVar_Set *var_set, const char *loc_var_nm)
		{
			const char *att_val;
			struct PVar *vr_tmp, *h_var;
			char h_buf[512];
			int h_len;
			att_val = ele->Attribute(att_nm);
			if (att_val) 
			{
				h_var = var_set->one_still(att_val, h_buf, h_len);	//�������ֵΪ������, Ҫ�ҵ��������
				if ( h_var ) 
				{
					if ( h_var->kind <= VAR_Dynamic ) 
					{
						vr_tmp = sv_set.look(loc_var_nm);	//loc_var_nm�������б��ر�����
						vr_tmp->dynamic_pos = h_var->dynamic_pos;	//���ر�����ӳ��ȫ����̬������
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

	struct ChargeIns: public ComplexSubSerial {			//��ֵ�ۺ�ָ��
		const char *key;	//��ֵkey��ָ���ĵ�
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

			def_sub_vars(xml);		//�����������
			sub_allow_sw(ele);	//���������SW, ����slot

			get_loc_var (ele, "term",  var_set, "me.term", "123456789012");
			get_loc_var (ele, "amount",  var_set, "me.amount", "00000000");
			get_loc_var (ele, "datetime",  var_set, "me.datetime", "20131231083011");

			ref_prepare_sub(ele->Attribute("key"),  key_lod, var_set, "key");	//��Կ����Ԥ����
			get_entry(key_lod, map_root, entry_nm);
			return defer_sub_serial(ele, sub_ins_entry, var_set, &sv_set);
		};
	};

	struct LoadInitIns: public ComplexSubSerial {			//Ȧ���ʼ��ָ��
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
			def_sub_vars(xml);		//�����������
			sub_allow_sw(ele);	//���������SW

			get_loc_var (ele, "typeid",  var_set, "me.typeid", "02");
			get_loc_var (ele, "term",  var_set, "me.term", "123456789012");
			get_loc_var (ele, "amount",  var_set, "me.amount", "00000000");
			get_loc_var (ele, "datetime",  var_set, "me.datetime", "20131231083011");
			get_loc_var (ele, "online",  var_set, "me.online"); //�������, ������Ĭ��ֵ
			get_loc_var (ele, "balance",  var_set, "me.balance"); //�������, ������Ĭ��ֵ
			get_loc_var (ele, "mac2",  var_set, "me.mac2"); //�������, ������Ĭ��ֵ

			ref_prepare_sub(ele->Attribute("key"),  key_lod, var_set, "key");
			get_entry(key_lod, map_root, entry_nm);
			return defer_sub_serial(ele, sub_ins_entry, var_set, &sv_set);
		};
	};

	struct DebitIns: public ComplexSubSerial {			//���������ѹ��̣�������ʼ�����ۿ�
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

			def_sub_vars(xml);		//�����������
			sub_allow_sw(ele);	//�����SW

			get_loc_var (ele, "typeid",  var_set, "me.typeid", "06");
			get_loc_var (ele, "term",  var_set, "me.term", "000000000012");
			get_loc_var (ele, "amount",  var_set, "me.amount", "00000000");
			get_loc_var (ele, "datetime",  var_set, "me.datetime", "00000000000000");
			get_loc_var (ele, "transno",  var_set, "me.transno", "0000");
			get_loc_var (ele, "offline",  var_set, "me.offline"); //�������, ������Ĭ��ֵ
			get_loc_var (ele, "balance",  var_set, "me.balance"); //�������, ������Ĭ��ֵ
			get_loc_var (ele, "tac",  var_set, "me.tac"); //�������, ������Ĭ��ֵ

			ref_prepare_sub(ele->Attribute("key"),  key_debit, var_set, "key");
			get_entry(key_debit, map_root, entry_nm);
			return defer_sub_serial(ele, sub_ins_entry, var_set, &sv_set);
		};
	};

	struct ExtAuthIns: public ComplexSubSerial {
		char auth[256];
		struct PVar *key_auth;	//��֤key
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
			sub_allow_sw(ele);	//�����SW

			memset(auth, 0, sizeof(auth));
			au = key_e->Attribute("auth");	//<key auth=""></key>�е�auth����
			if ( !au)
				au = ele->Attribute("auth");
			if ( au && strlen(au) > 0 )
			{
				squeeze(au, auth);
				sv_set.put_still("me.auth", auth);
			} else
				sv_set.put_still("me.auth", "0082000008");

			ref_prepare_sub(key_e->GetText(),  key_auth, var_set, "key");	//"key"��me.key.para���Ӧ
			get_entry(key_auth, map_root, entry_nm);
			return defer_sub_serial(ele, sub_ins_entry, var_set, &sv_set);
		};
	};
	
	struct DesMacIns: public ComplexSubSerial {
		struct PVar *pk_var;	//����key��ָ������õ�һ����������
		struct PVar *ek_var;	//����key��ָ������õ�һ����������

		bool head_dynamic;
		bool body_dynamic;

		TiXmlElement *head;	//ָ���ĵ��еĵ�һ��headԪ��
		TiXmlElement *body;	//ָ���ĵ��еĵ�һ��bodyԪ��
		TiXmlElement *tail;	//ָ���ĵ��еĵ�һ��tailԪ��
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
		/* ȡ��ʵʱ��ָ��ͷ��ָ��������� */
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
					blen += (8-blen%8);	//�������ݲ�λ��ĳ���
				blen += ( 8 + hlen );	//����8�ֽ��������ͷ����

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

			def_sub_vars(xml);		//�����������
			sub_allow_sw(ele);	//���������SW
			ref_prepare_sub(ele->Attribute("protect_key"),  pk_var, var_set, "protect");	//����Key����
			get_entry(pk_var, map_root, entry_nm);	//sub_ins_entry��ȷ��
			if ( !sub_ins_entry) return 0;	//û�������ж���, ֱ�ӷ�
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
				if ( !vr_tmp ) 		//���ǳ���, ����Ӧ�ý�����
				{
					if (e_tmp) printf("desmac head !!!!!!!!!!\n");	//�ⲻӦ��
						continue;
				}

				if ( vr_tmp->kind <= VAR_Dynamic )
				{
					head_dynamic = true;		//��̬��
				} 
			}

			ek_var = 0;		//���û�е���?
			body_len = 0;
			body_buf[0] = 0;
			n_ele = e_tmp = body;
			j = 0;
			while ( e_tmp ) 
			{
				vr_tmp= var_set->all_still( e_tmp, tag_body, body_buf, body_len, n_ele);
				e_tmp = n_ele;
				if ( !vr_tmp ) 		//���ǳ���, ����Ӧ�ý�����
				{
					if (e_tmp) printf("desmac body !!!!!!!!!!\n");	//�ⲻӦ��
					continue;
				}
					
				if ( vr_tmp->kind == VAR_Refer )
				{
					ek_var = vr_tmp;		//�ο�����, ���Ҳ��, �Ժ��1��ʼ
					if ( j == 0 ) 	//ek_var�Ѿ��ҵ�, ����һ��, ����������, �е��ظ�
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
					body_dynamic = true;		//��̬��
				} 
			}

			if ( head_dynamic )		//sv_set: ���������
			{
				vr_tmp = sv_set.look("me.head");
				vr_tmp->dynamic_pos = var_set->get_neo_dynamic_pos();	//��̬����λ��
				vr_tmp->kind = VAR_Dynamic;
				head_dy_pos = vr_tmp->dynamic_pos;

				vr_tmp = sv_set.look("me.head_length");
				vr_tmp->dynamic_pos = var_set->get_neo_dynamic_pos();	//��̬����λ��
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
				vr_tmp->dynamic_pos = var_set->get_neo_dynamic_pos();	//��̬����λ��
				vr_tmp->kind = VAR_Dynamic;
				body_dy_pos = vr_tmp->dynamic_pos;

				vr_tmp = sv_set.look("me.body_length");
				vr_tmp->dynamic_pos = var_set->get_neo_dynamic_pos();	//��̬����λ��
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
				vr_tmp->dynamic_pos = var_set->get_neo_dynamic_pos();	//��̬����λ��
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
		const char *protect;	//����key��ָ���ĵ�
		struct PVar *pk_var;	//����key��ָ������õ�һ����������

		const char *tag;
		TiXmlElement *component;	//ָ���ĵ��еĵ�һ��componentԪ��

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

			def_sub_vars(xml);		//�����������
			sub_allow_sw(ele);	//���������SW
			ref_prepare_sub(ele->Attribute("protect_key"),  pk_var, var_set, "protect");	//������Կ
			get_entry(pk_var, map_root, entry_nm);
			len_format = sub_ins_entry->Attribute("length_format");	//ȡ�ó��ȸ�ʽ�ַ���

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
				if ( !vr_tmp ) 		//���ǳ���, ����Ӧ�ý�����
				{
					if (e_tmp) printf("macins !!!!!!!!!!\n");	//�ⲻӦ��
					continue;
				}
					
				if ( vr_tmp->kind <= VAR_Dynamic )
				{
					dynamic = true;		//��̬��
				} 
			}

			if ( dynamic ) 
			{
				vr_tmp = sv_set.look("me.command");
				vr_tmp->dynamic_pos = var_set->get_neo_dynamic_pos();	//��̬����λ��
				vr_tmp->kind = VAR_Dynamic;
				cmd_dy_pos = vr_tmp->dynamic_pos;

				vr_tmp = sv_set.look("me.command_length");
				vr_tmp->dynamic_pos = var_set->get_neo_dynamic_pos();	//��̬����λ��
				vr_tmp->kind = VAR_Dynamic;
				cmd_len_dy_pos = vr_tmp->dynamic_pos;

				vr_tmp = sv_set.look("me.mac_length");
				vr_tmp->dynamic_pos = var_set->get_neo_dynamic_pos();	//��̬����λ��
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
		const char *ori_kmc;	//ԭKMC��ָ���ĵ�, ������һ��������, ��ʵ�ʵ�ֵ
		struct PVar *ori_kvar;	//ԭָ������õ�һ����������

		const char *neo_kmc;	//��KMC��ָ���ĵ�, ������һ��������, ��ʵ�ʵ�ֵ
		struct PVar *neo_kvar;	//��ָ������õ�һ����������
		char ori_ver[8];		//ԭ��Կ�汾
		char neo_ver[8];		//����Կ�汾

		char put_type[8];	//��Կ����: 80��81, ����put key����.
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

			def_sub_vars(xml);		//�����������
			sub_allow_sw(ele);	//���������SW

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
			get_entry(ori_kvar, map_root, entry_nm);	//�Ծ���Կ��pro����������
			return defer_sub_serial(ele, sub_ins_entry, var_set, &sv_set);
		};
	};
	
	struct ResetIns: public ComplexSubSerial {
		const char *pro_name;	//��λʱʹ�õ�һ����������
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

	struct User_Command {		//INS_Userָ���
		int order;
		struct ComplexSubSerial *complex;
		int comp_num;

		//enum Command_Type type;	//����, ����union����, �治֪����ε���������캯��
		//	struct PlainIns plain;
		//	struct HsmIns hsm;
		//	struct CallFun fun;

		//	int auth_num;
		//	struct ExtAuthIns *auth;
		//	struct ComplexSubSerial *complex;

		//	/* -------- ���ն˻������� ----------*/
		//	struct PromptOp prompt;
		//	struct FeedCardOp fcard;
		//	struct OutCardOp ocard;
		//	struct ProRstOp pro_rst;
		//	/* -------- ���ն˻������� ----------*/

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
		//		/* -------- ���ն˻������� ----------*/
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

		int  set ( TiXmlElement *app_ele, struct PVar_Set *vrset, TiXmlElement *map_root, struct ComplexSubSerial *pool) //���ض�IC��ָ����
		{
			TiXmlElement *sub_serial, *spro, *me, *pri;
			const char *pri_nm;
			const char *nm = app_ele->Value();
			int comp_num , ret_ic;

			complex = pool;
			app_ele->QueryIntAttribute("order", &order);

			sub_serial = map_root->FirstChildElement(nm); //ǰ���Ѿ���������, ����϶���ΪNULL,nm����Command֮��ġ�
			me = sub_serial->FirstChildElement("Me");
			pri_nm = me->Attribute("primary");
			if ( pri_nm)
			{
				if ( app_ele->Attribute(pri_nm))
				{
					/* pro_analyze ���ݱ�����, Ҳ������������Ԫ������, ȥ�ҵ�ʵ�������ı�������(�����ǲο�����)������������ָ����Pro�� */
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
			//����ȷ��������
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
						if ( cor <= mor ) goto ICC_NEXT;	//order������˳��ģ��Թ�
						i_num += instructions[vmany].set(icc_ele, var_set, map_root, &comp_pool[comp_num-i]);
						mor = cor;
						vmany++;
					}
				}
				ICC_NEXT:
				icc_ele = icc_ele->NextSiblingElement();
			}
			many = vmany; //����ٸ���һ��ָ����
		};
	};	
	/* User_Commandָ�������� */
	
	struct  Personal_Def	//���˻�����
	{
		TiXmlDocument doc_c;	//User_Commandָ��壻
		TiXmlElement *c_root;	
		TiXmlDocument doc_k;	//map��Variable���壬�����ж���
		TiXmlElement *k_root;

		TiXmlDocument doc_v;	//����Variable����
		TiXmlElement *v_root;
		
		struct PVar_Set person_vars;
		struct INS_Set ins_all;

		char flow_md[64];	//ָ����ָ������
		const char *flow_id;	//ָ������־
		const char *description;	//��������

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
				cv = person_vars.look(var_ele->Attribute("name")); //�����еı���������Ѱ�Ҷ�Ӧ��
				if ( !cv ) continue; 	//�޴˱���, �Թ�
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
				description = c_root->Attribute("desc");
				squeeze(per_ele->Attribute("md5"), flow_md);
			}
			set_here(c_root);	//�ٿ�������
			set_here(v_root);	//����������������,key.xml��

			ins_all.put_inses(c_root, &person_vars, k_root);//ָ�����, �ѱ����������ȥ, �ܶ�ָ������ֳɵ���.
			return true;
		}
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
	/* �ƿ�������뷽ʽ */
	enum MK_Mode { FROM_START = 0,  FROM_HSM = 0x11 , FROM_ICTERM = 0x22, FROM_OTHER =99};
	enum Work_Mode { TO_HSM = 0x11 , TO_ICTERM = 0x22, TO_NONE=0};

private:
	bool spo_term_hand();	//�ұ�״̬����
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

	struct G_CFG 	//ȫ�ֶ���
	{
		TiXmlElement *prop;
		Work_Mode wmod;
		struct PersonDef_Set person_defs;

		int hcmd_fldno;		/* �����ָ����� */
		int fldOffset;	/* ����PacketObjʱ, �����е���ż��ϴ�ֵ(ƫ����)��ʵ�ʴ������, ��ʼΪ0 */
		int maxium_fldno;		/* ������ */

		int flowID_fld_no;	//����ʶ��, ҵ�������, 
		int error_fld_no;	//��������,
		int errDesc_fld_no;	//������Ϣ��
		int station_fld_no;	//����վ��ʶ��

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

	PacketObj hi_req, hi_reply; /* ���Ҵ��ݵ�, �����Ƕ�HMS, ���Ƕ�IC�ն� */
	PacketObj *hipa[3];
	PacketObj *rcv_pac;	/* ������ڵ��PacketObj */
	PacketObj *snd_pac;

	struct MK_Session mess;	//��¼һ���ƿ������еĸ�����ʱ����
	struct Personal_Def *cur_def;	//��ǰ����

	struct G_CFG *gCFG;     /* ȫ�ֹ������ */
	bool has_config;
	PacWay *tohsm_pri;
	PacWay *toterm;
	PacWay *tohsm_slv;
	Amor::Pius loc_pro_pac;

	char res_buf[1024], hsm_cmd_buf[2048], icc_cmd_buf[1024];
	MINLINE void deliver(TEXTUS_ORDO aordo);

	enum Back_Status {BS_Requesting = 0, BS_Answered = 1, BS_Crashed = 2 };
	Back_Status back_st;		//�Ҵ���״̬

	struct WKBase {
		int step;	//0: send, 1: recv 
	};

	struct CompWKBase:public WKBase {
		int sub_which;
	} ;

	struct CompWKBase comp_wt;	//�����й���״̬

	struct _WP:public WKBase {
		char com[16];
	} plain_wt;

	struct _WAU:public CompWKBase {
		int cur;
	} auth_wt;

	/* ���¶�IC��5�ֻ�������, ����Щ������ԭ������Ϊ�첽����, ��Ҫ���������״̬ */
	struct _WFC:public WKBase {
		struct FeedCardOp fcard;	//����
	} fcard_wt;

	struct _WOC:public WKBase {		//����
		struct OutCardOp ocard;
	} ocard_wt;

	struct _WPST:public WKBase {		//����λ
		struct ProRstOp pro_rst;
		int try_num;
	} pro_rst_wt;

	struct _WPMT:public WKBase {
		struct PromptOp prompt;		//������ʾ
	} prompt_wt;

	struct _WPCOM {
		char cmd_buf[1024];
		struct ProComOp procom;		//��ָ��
	} procom_wt;
	
	struct _HMOP:public WKBase {		//HSM ָ��
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

	case Notitia::PRO_UNIPAC:    /* �����Կ���̨������ */
		WBUG("facio PRO_UNIPAC %s",  gCFG->wmod == TO_ICTERM ? "TERM" : "other" );
		if (gCFG->wmod != TO_ICTERM)  //������Ƕ��ƿ��ն˵ģ���ô��������ڵ������
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
		if (gCFG->wmod == TO_HSM)  //����Ƕ�HSM���͸����Ǹ���ICTERM��, ���ﴦ��HSM��. �ⲿ�������ļ�Ҫ֧��һ��Fly
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
				toterm->mk_hand(FROM_HSM); //�����ն˵��Ǹ��ڵ�????
			}
			goto H_END;
		}

		if (gCFG->wmod != TO_ICTERM)  //������Ƕ��ƿ��ն˵ģ���ô��������ڵ������
		{
			WBUG("bug!! not ic term!!");
			goto H_END;
		}

		if ( !mk_hand( FROM_ICTERM))	//���û�д���, �͵�����������.  
		{
			back_st = BS_Answered; 		//�����ݻ����ˡ�
			if ( !spo_term_hand() )	//�ն˵�������������
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

	case Notitia::DMD_END_SESSION:	//�ҽڵ�ر�, Ҫ����
		WBUG("sponte DMD_END_SESSION");
		if ( mess.left_status == LT_IDLE)	/* ��߽ڵ�û����������, ����Ҳ����Ҫ���� */
			break;
		if (gCFG->wmod == TO_HSM) 
		{
			if ( !toterm )
			{
				WBUG("bug!! hsm don't know how to ic term!!");
			} else {
				toterm->mess.iRet = ERROR_HSM_TCP;
				TEXTUS_SPRINTF(toterm->mess.err_str, "hsm down for %s",  mess.station_str);
				toterm->mk_result(); //�����ն˵��Ǹ��ڵ�????
			}
			goto D_END;
		}

		if (gCFG->wmod != TO_ICTERM)  //������Ƕ��ƿ��ն˵ģ���ô��������ڵ������
		{
			WBUG("bug!! not ic term!!");
			goto D_END;
		}

		if ( mess.left_status == LT_MKING  )	//�������ƿ�����
		{
			mess.iRet = ERROR_MK_TCP;
			TEXTUS_SPRINTF(mess.err_str, "term device down at %s",  mess.station_str);
			WLOG(WARNING, mess.err_str);
			mk_result();	//����
		} else {
			back_st = BS_Crashed; 		//�����ݻ����ˡ�
			spo_term_hand();	//���״̬����, Ҳ����
		}
D_END:
		break;

	case Notitia::START_SESSION:	//�ҽ�֪ͨ, �ѻ�
		break;

	default:
		return false;
		break;
	}
	return true;
}

/* ���ر����Ƿ��ն˲��� */
bool PacWay::spo_term_hand()
{
	unsigned char *p, *q, *r;
	unsigned long p_len, q_len, r_len;

	switch ( mess.right_status) 
	{
	case RT_TERM_TEST:	//�ƿ��ն˲���
		if ( back_st == BS_Crashed )           
		{
			TEXTUS_SPRINTF(mess.err_str,  "CTst Crashed at %s", mess.station_str); 
			WLOG(WARNING, "%s", mess.err_str);
			snd_pac->input(gCFG->error_fld_no, "A", 1);	//����3��Ϊ���
			snd_pac->input(4, mess.err_str, strlen(mess.err_str));
			goto H_END;
		}
		if ( back_st != BS_Answered )
		{
			WBUG("bug!! back_st != BS_Answered");
		}

		p_len = q_len =  r_len = 0;
		p = hi_reply.getfld(1, &p_len);	//��Ӧ������
		q = hi_reply.getfld(2, &q_len);	//�����
		r = hi_reply.getfld(3, &r_len);	//3��Ϊ���Խ������

		if ( p_len != 1 || memcmp(p, "t",1) != 0 || q_len != 1 || r_len < 1 )
		{
			TEXTUS_SPRINTF(mess.err_str,  "CTst RPC_ERROR at %s", mess.station_str); 
			WLOG(WARNING, "%s", mess.err_str);
			snd_pac->input(gCFG->error_fld_no, "C", 1);	//ͨѶ���ĵ�����
			snd_pac->input(gCFG->errDesc_fld_no, mess.err_str, strlen(mess.err_str));
			goto H_END;
		}

		if ( *q != '0' ) 
		{
			char tmp[128];
			memcpy(tmp, r, r_len > 120 ? 120 : r_len);
			tmp[r_len > 120 ? 120 : r_len] = 0;
			WLOG(WARNING, "CTst DEV_ERROR(%s) at  %s", tmp, mess.station_str);
			snd_pac->input(gCFG->error_fld_no, (unsigned char*)"B", 1);	//�豸����������
			snd_pac->input(gCFG->errDesc_fld_no, r, r_len);
			goto H_END;
		}
		/* �豸OK */
		snd_pac->input(gCFG->error_fld_no, (unsigned char*)"0", 1);	//����3��Ϊ���
H_END:
		mess.left_status = LT_IDLE;
		mess.right_status = RT_IDLE;
		aptus->sponte(&loc_pro_pac);    //��Ӧ������̨����
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

	actp=rcv_pac->getfld(gCFG->flowID_fld_no, &alen);		//ȡ��ҵ�����, ������ʶ
	if ( !actp)
	{
		WBUG("business code field null");
		goto END;
	}

	switch ( mess.left_status) 
	{
	case LT_IDLE:
		if ( alen==4 &&  memcmp(actp, "\x00\x00\x00\x00", alen) ==0 ) 
		{ /* ����������ͨ���Ķ�Ӧ��ϵ */
			snd_pac->reset();
			plen = 0;
			p = rcv_pac->getfld(gCFG->station_fld_no, &plen);		//�ն˱�ʶ
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
			aptus->facio(&loc_pro_pac);     //���ҷ���ָ��, Ȼ��ȴ�. �������������. ����������Ҫ
			goto HERE_END;
		}	
	
		/* �������һ���ҵ���� */
		mess.reset();	//�Ự��λ
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
		
		cur_def = gCFG->person_defs.look(mess.flow_id); //��һ����Ӧ��ָ��������
		if ( !cur_def ) 
		{
			TEXTUS_SPRINTF(mess.err_str, "not defined flow_id: %s ", mess.flow_id );
			mess.iRet = ERROR_INS_DEF;	
			mk_result();	//��������
			goto HERE_END;
		}
		/* �ƿ�����ʼ  */
		mess.snap[Pos_FlowTotal].input( cur_def->ins_all.many);
		if ( cur_def->flow_md[0] )
			mess.snap[Pos_FlowPrint].input( cur_def->flow_md, strlen(cur_def->flow_md));

		/* Ѱ�ұ����������ж�̬��, �����Ƿ���start_pos��get_length��, ���ݶ��帳ֵ��mess�� */
		for ( i = 0 ; i <  cur_def->person_vars.many; i++)
		{
			vt = &cur_def->person_vars.vars[i];
			if ( vt->dynamic_pos >=0 )
			{
				dvr = &mess.snap[vt->dynamic_pos];
				dvr->kind = vt->kind;
				dvr->dest_fld_no = vt->dest_fld_no;
				dvr->def_var = vt;
				if ( vt->c_len > 0 )	//�ȰѶ���ľ�̬���ݿ�һ��, ��̬������Ĭ��ֵ
				{
					dvr->input(&(vt->content[0]), vt->c_len);
				}
				if ( vt->source_fld_no >=0 )
					p = rcv_pac->getfld(vt->source_fld_no, &plen);
				else
					continue;
				if ( p && vt->get_length > 0 )	//���������Ż�ȡ�ĳ���, ��̬�����Ͳ�ȡ
				{
					if ( plen > vt->start_pos )
					{
						plen -= (vt->start_pos-1);	//plenΪʵ����ȡ�ĳ���, start_pos�Ǵ�1��ʼ
						dvr->input( (const char*)&(p[vt->start_pos-1]), plen < vt->get_length ? plen : vt->get_length);
					}
				}
				/* ���Դ���ȡֵΪ����, ���ʵ�ʱ���û�и���, ��ȡ����ľ�̬���� */
			}
		}
	//{int *a =0 ; *a = 0; };

		WLOG(NOTICE, "Current cardno is %s by %s at %s",  mess.snap[Pos_CardNo].val, cur_def->flow_id, mess.station_str);
		mk_hand(FROM_START);	//����!!������Ҫ, �����TRUE��ָ�տ�ʼ����
HERE_END:
		break;

	case LT_INITING: //������, ����Ӧ
		if ( alen > 20 ) alen =20;
		byte2hex(actp, alen, desc); desc[2*alen]= 0;
		WLOG(WARNING,"still testing mkdev! from console %s", desc);
		break;

	case LT_MKING:	//������, ����Ӧ����
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

/* hsm_fun_X ��, �Ƿ���HSM�Ľڵ㣬 û��mk_sess����!!!, ��Ҫ���������! */
void PacWay::hsm_fun_R(const char *com, int in_len, const char *kloc)
{
	hi_req.reset();
	hi_reply.reset();
	if ( kloc ) 
		hi_req.input(1, kloc, strlen(kloc)); //����C, ֱ�ӵ�ָ��	//���ʵ���ò���?
	else
		hi_req.input(1, "C", 1); //����C, ֱ�ӵ�ָ��	//���ʵ���ò���?
	hi_req.input(gCFG->hcmd_fldno, com, in_len);
}

void PacWay::hsm_fun_S(char *&res, int &out_len)
{
	res = (char*)hi_reply.getfld(gCFG->hcmd_fldno, &out_len);
}

/* hsm_com_X, ��TERM��, ��mk_sess�� */
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
	tohsm_pri->hsm_fun_S(res, out_len);	//�ǵ��Ǹ�HSM�ڵ�ȡ���ݵġ�
	if (out_len < head_len ) 
	{
		WLOG(WARNING, "hsm_com RPC_ERR for %s",  mess.station_str);
		return ERROR_HSM_RPC;
	}

	res[out_len] = 0;
	WLOG(CRIT, "hsm_com %s, return %s", hsmcom_wt.cmd, res);
	if ( memcmp(res, ref, head_len) != 0  ) 	//hsm_cmd������֤res_len >= 4
	{
		int m_len;
		if ( head_len+1 > (int) sizeof(mess.bad_sw))
			m_len = sizeof(mess.bad_sw) -1;
		else
			m_len = head_len;
		memcpy(mess.bad_sw, res, m_len);
		mess.bad_sw[m_len] = 0;
		WLOG(WARNING, "hsm_com return %s, for %s  when order= %d in %s",  mess.bad_sw, mess.station_str, mess.pro_order, cur_def->flow_id);
		if ( hsmcom_wt.ci > 0 )	//��������һ�������
		{
			WLOG(WARNING, "HSM again %d", hsmcom_wt.ci);
			hsmcom_wt.ci--;
			hsm_com_R(0,0,0);
			return ERR_HSM_AGAIN;
		}
		return ERROR_HSM_FUN;
	}
	res = &res[head_len];	//��ָ��ƫ��4, �ͱܿ����Ǹ�7100֮��Ķ���.
	out_len -=head_len ;
	return 0;
}

/* �Ƿ�Ϊ�ƿ������ķ��� */
struct TermOPBase* PacWay::get_ic_base()
{
	struct TermOPBase *wbase = 0;
	switch ( mess.right_status) 
	{
	case RT_IC_COM:		//COSָ���
		wbase = &procom_wt.procom;
		break;

	case RT_TERM_PROMPT:	//������ʾ
		wbase = &prompt_wt.prompt;
		break;

	case RT_TERM_FEED:
		wbase = &fcard_wt.fcard;
		break;

	case RT_TERM_OUT:
		wbase = &ocard_wt.ocard;
		break;

	case RT_IC_RESET:	//IC��λ
		wbase = &pro_rst_wt.pro_rst;
		break;

	default:
		break;
	}
	return wbase;
}

/* ������д�����, ����ƿ��ն˷��صĽ����������� */
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
		pro_com_S(res_buf,  sizeof(res_buf)-1, sw);	//����IC����ģ�������������
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
	plain_wt.step++; //ָʾ��һ������
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
		if ( mess.iRet == ERR_HSM_AGAIN )		//���ܻ�����
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

/* ��������� */
int PacWay::sub_serial_pro( struct INS_SubSet *si_set, struct CompWKBase *wk)
{
	int i_ret=0, s_ret=0;
	struct Base_Command *ins;

	if ( !si_set )	//���������������, �͵�ִ�����.
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
			s_ret = 1;	//ָ���Ѿ�ִ�����
		else {
			goto SUB_INS_PRO;
		}
	} else if ( i_ret <= 0 ) {	//���ڽ�����,ָ���ѱ�, �����Ѿ���������
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
			//��ʼ������Ϊ��������
			mess.ins_which = 0;
			mess.iRet = 0;	//�ٶ�һ��ʼ����OK��
			TEXTUS_STRCPY(mess.err_str, " ");
			mess.left_status = LT_MKING;
			mess.right_status = RT_INS_READY;	//ָʾ�ն�׼����ʼ����, 
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
			goto INS_PRO; //�տ�ʼ������ֱ�ӹ����� �����Ǵ�����ƿ��ն˻���ܻ��ķ��ء�

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
			if ( !(wbase = get_ic_base()) )  //�����ƿ���������Ӧ, ����(�������ն˲��ԡ���ʾ��һ�ſ���)
				return false;
			/* �ƿ���Ӧ����ͨѶ���ݼ�� */
			p = (char*)hi_reply.getfld(1, &p_len);
			if ( p_len != (unsigned long)wbase->tlen || memcmp(p, wbase->take, wbase->tlen) != 0)	//�������Ƿ��Ӧ
			{
				if ( p_len > 30 ) p_len = 30; 
				memcpy(tmp, p, p_len ); 
				tmp[p_len] = 0;
				TEXTUS_SPRINTF(mess.err_str, "%s, RPC_ERROR(%s), at %s", wbase->fun, tmp, mess.station_str);
				WLOG(WARNING, "%s", mess.err_str);
				mess.iRet= ERROR_MK_RPC;
				goto CardErrPro;
			}

			q = (char*)hi_reply.getfld(2, &q_len);	//����볤��������
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

	if ( mess.right_status  ==  RT_INS_READY )	//�ն˿���,���๤����Ԫ��ո�λ
		me_zero();
	
	hi_req.reset();	//����λ
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

			case 1: //������ʾ����Ӧ, ��������
				mess.right_status = RT_INS_READY ;	//�Ҷ���, ������һ��ָ��
				break;
			}
			prompt_wt.step++;	//ָʾ��һ������
		break;

	case OP_FeedCard:
			switch ( fcard_wt.step)
			{
			case 0:
				hi_req.input(2, mess.snap[Pos_CardNo].val, mess.snap[Pos_CardNo].c_len);
				hi_req.input(3, "0", 1);	//���̱�־�ò�����
				hi_req.input(4, cur_def->description, strlen(cur_def->description));
				mess.right_status = ins->fcard.rt_stat;
				break;

			case 1: //������Ӧ
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
				mess.right_status = RT_INS_READY ;	//�Ҷ���, ������һ��ָ��
				break;
			}
			fcard_wt.step++;	//ָʾ��һ������
		break;

	case OP_OutCard: //��������, �п���ǰ���ƿ�ʧ��
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
				mess.right_status = RT_INS_READY ;	//�Ҷ���, ������һ��ָ��
				break;
			}
			ocard_wt.step++;

		break;

	case INS_ProRst:		//��Ƭ��λ 
			switch ( pro_rst_wt.step)
			{
			case 0:	//��㽫����������, �������һ��״̬
				if ( !ins->complex->valid_condition(&mess) )
				{
					mess.right_status = RT_INS_READY ;	//�Ҷ���, ������һ��ָ��
					break;
				}

				mess.right_status = ins->pro_rst.rt_stat;
				pro_rst_wt.step++;
				break;
			case 1:
				p = (char*)hi_reply.getfld(3, &p_len);	//2����slot
				r = (char*)hi_reply.getfld(7, &r_len);	//UID
				if ( p_len != 1 || *p != '0' ||!r )
				{
					q = (char*)hi_reply.getfld(4, &q_len);
					h_fail(tmp, desc, p_len, q_len, p, q, "pro_reset");
					mess.iRet = ERROR_IC_INS;		//��λʧ��, ���ǿ�����, �����豸
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
					//ͨ���� ��һ��ָ��
					mess.right_status = RT_INS_READY ;	//�Ҷ���, ������һ��ָ��
					break;
				}
				/* ���ﲻ�ж�, ������������ */
				pro_rst_wt.step++;	//����ʹ��step=2
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
						goto CardErrPro; //����SW���嶼������

					if ( i_ret == 0 )  
						goto INS_OUT; //�����л�û��ִ�����, �����ж�
//{int *a =0 ; *a = 0; };
					if ( !ins->complex->valid_result(&mess) )
					{
						mess.iRet = ERR_RESULT_INVALID;
						goto CardErrPro; 
					}
					//ͨ���� ��һ��ָ��
					mess.right_status = RT_INS_READY ;	//�Ҷ���, ������һ��ָ��
				default:
					break;
				}
			}
		break;

	case INS_Call:
			if ( !ins->fun.valid_condition(&mess) )
			{
				mess.right_status = RT_INS_READY ;	//�Ҷ���, ������һ��ָ��
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
				mess.right_status = RT_INS_READY ;	//�Ҷ���, ������һ��ָ��
			} else if ( i_ret < 0 ) {
				goto CardErrPro; //����SW���嶼������
			}
		break;

	case INS_Plain:
			if ( plain_wt.step ==0 && !ins->plain.valid_condition(&mess) )
			{
				mess.right_status = RT_INS_READY ;	//�Ҷ���, ������һ��ָ��
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
				mess.right_status = RT_INS_READY ;	//�Ҷ���, ������һ��ָ��
			} else if ( i_ret < 0 ) {
				goto CardErrPro; //����SW���嶼������
			}
		break;

	case INS_HSM:
			if ( hsmcom_wt.step ==0 && !ins->hsm.valid_condition(&mess) )
			{
				mess.right_status = RT_INS_READY ;	//�Ҷ���, ������һ��ָ��
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
				mess.right_status = RT_INS_READY ;	//�Ҷ���, ������һ��ָ��
			} else if ( i_ret < 0 ) {
				goto HsmErrPro; 
			}
		break;

	case INS_ExtAuth:
			switch ( auth_wt.step)
			{
			case 0:	//��ʼ
				if ( !ins->auth[0].valid_condition(&mess) )
				{
					mess.right_status = RT_INS_READY ;	//�Ҷ���, ������һ��ָ��
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
					goto AUTH_AGAIN;		//��һ����֤��ʼ
				}
				if ( i_ret < 0 ) 
					goto CardErrPro; //����SW���嶼������

				if ( i_ret ==0  )
					goto INS_OUT; //�����л�û��ִ�����, �����ж�
				if ( !ins->auth[0].valid_result(&mess) )
				{
					mess.iRet = ERR_RESULT_INVALID;
					goto CardErrPro; 
				}
				//ͨ���� ��һ��ָ��
				mess.right_status = RT_INS_READY ;	//�Ҷ���, ������һ��ָ��
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
					mess.right_status = RT_INS_READY ;	//�Ҷ���, ������һ��ָ��
					break;
				}

				comp_wt.sub_which = 0;
				ins->complex->get_current(&mess, &(cur_def->person_vars));
				comp_wt.step++;	
				me_sub_zero();
			case 1:
				i_ret = sub_serial_pro(ins->complex->si_set, &comp_wt);
				if ( i_ret < 0 ) 
					goto CardErrPro; //����SW���嶼������

				if ( i_ret == 0 ) 
					goto INS_OUT; //�����л�û��ִ�����, �����ж�

				if ( !ins->complex->valid_result(&mess) )
				{
					mess.iRet = ERR_RESULT_INVALID;
					goto CardErrPro; 
				}
				//ͨ���� ��һ��ָ��
				mess.right_status = RT_INS_READY ;	//�Ҷ���, ������һ��ָ��
			default:
				break;
			}
		break;

	case INS_None:
		break;
	}

	if ( mess.iRet != 0 && mess.ins_which < (cur_def->ins_all.many-1) && mess.iRet != ERR_HSM_AGAIN ) //�д�����
	{
		WBUG("bug!! mk_hand has error! but not goto error!");
	}

	if (mess.right_status == RT_INS_READY )	//�½�һ��ָ������
	{
		WLOG(CRIT, "has completed %d, order %d", mess.ins_which, mess.pro_order);
		mess.ins_which++;
		if ( mess.ins_which < cur_def->ins_all.many)
			goto INS_PRO;		//һ��ָ�����, ��һ��
		else 
			mk_result();		//һ�ſ��Ѿ����
	} else  {
		goto INS_OUT;
	}
HAND_END:
	return true;
INS_OUT:
	wbase = get_ic_base();
	if (wbase)
	{	//��һ���ж�����IC������
		hi_req.input(1, wbase->give, wbase->glen);	//�����������������������Ҫ!!!
		aptus->facio(&loc_pro_pac);     //���ҷ���ָ��, Ȼ��ȴ�, ������ڱ�????
	} else {	//HSM����,
		tohsm_pri->aptus->facio(&loc_pro_pac);     //���ҷ���ָ��, Ȼ��ȴ�
	}
	/* aptus.facio�Ĵ���������, ����Ҫ!!����Ϊ�п�������������о��յ��ҽڵ��sponte. ע��!!. �Ժ�Ĺ�����һ��Ҫע����� */
	return true;

CardErrPro: //���ſ��д���֮
HsmErrPro: //���ſ��д���֮
	if ( mess.iRet != 0 ) //�д�����
	{
		mess.ins_which++;
		if ( mess.ins_which >= cur_def->ins_all.many)		//���һ��������������
		{
			mk_result();
		} else {				//�����������һ��
			mess.ins_which = cur_def->ins_all.many - 1;	//ָ�����һ����������
			mess.right_status = RT_INS_READY ;	//�Ҷ���, ������һ��ָ��
			goto INS_PRO;		//ʵ���������һ��
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
			snd_pac->input(gCFG->error_fld_no, "0A", 2);	//�����ͨѶ��, ��������Ӧֹͣ
			break;

		case ERROR_MK_TCP:
			snd_pac->input(gCFG->error_fld_no, "0B", 2);	//ͨѶ����, �����ٷ���
			break;

		case ERROR_IC_INS:
			snd_pac->input(gCFG->error_fld_no, "0C", 2);	//���󣬿ɷ���һ�ſ�
			break;	

		case ERROR_HSM_FUN:
			snd_pac->input(gCFG->error_fld_no, "0D", 2);	//���󣬿ɷ���һ�ſ�
			break;	

		case ERROR_MK_DEV:
			snd_pac->input(gCFG->error_fld_no, "0E", 2);	//���� ���ǲ��ر�ͨѶ, �����ٷ���
			break;

		case ERROR_MK_PAUSE:
			snd_pac->input(gCFG->error_fld_no, "0F", 2);	//�˹�������ͣ
			break;

		case ERROR_MK_RPC:
			snd_pac->input(gCFG->error_fld_no, "11", 2);	//���� �����ٷ���, �ر�ͨѶ
			break;	

		case ERROR_HSM_RPC:
			snd_pac->input(gCFG->error_fld_no, "12", 2);	//���� �����ٷ���
			break;

		case ERROR_INS_DEF:	//�Ҳ����������, ���߶����ļ�����
			snd_pac->input(gCFG->error_fld_no, "13", 2);	//���󣬿ɷ���һ�ſ�
			break;

		default:
			TEXTUS_SPRINTF(mess.err_str, "%s", "unkown error");
			snd_pac->input(gCFG->error_fld_no, "99", 2);
			break;
		}
		WLOG(WARNING, "The card of %s (uid= %s) personalized failed, iRet %d, %s", mess.snap[Pos_CardNo].val, mess.snap[Pos_UID].val, mess.iRet, mess.err_str);
		TEXTUS_SPRINTF(mess.err_str, "%s, %d, %s",  mess.bad_sw, mess.pro_order, cur_def == 0 ? " ": cur_def->flow_id);
			
		snd_pac->input(gCFG->errDesc_fld_no, mess.err_str, strlen(mess.err_str)); //�������ǷŴ�������
	} else {	//�ƿ��ɹ�
		snd_pac->input(gCFG->error_fld_no, "0", 1);
		for ( i = 0 ; i < mess.snap_num; i++)	//������ж�̬����, ����и�ֵ, �����õ����ر�����
		{
			dvr = &mess.snap[i];
			if ( dvr->dest_fld_no >=0 && dvr->kind != VAR_None && dvr->c_len > 0 )
			{
				snd_pac->input(dvr->dest_fld_no, dvr->val, dvr->c_len);
			}
		}
	}
	aptus->sponte(&loc_pro_pac);    //�ƿ��Ľ����Ӧ������̨
	mess.reset();
}

/* ��������ύ */
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
