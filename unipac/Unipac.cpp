/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title:Universal Pack Pro
 Build: created by octerboy, 2006/11/25, Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "casecmp.h"
#include "TBuffer.h"
#include "BTool.h"
#include "textus_string.h"
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>
#define PACINLINE inline

enum LocType {	/* ��λ���� */
	RIGID	= 0,	/* �̶����� */
	LVAR	= 1,	/* �䳤���ֽڵ�BCD���λΪ�򳤶� */
	LLVAR	= 2,	/* �䳤, ��λ��ǰ, ��λ�ں�, ��ͬ */
	LLLVAR	= 3,
	L4VAR	= 4,
	ALVAR	= 5,	 /* �䳤,һ���ֽڱ�ʾ����, ����Ϊ"1","33"�� */
	ALLVAR	= 6,	
	ALLLVAR	= 7,	
	AL4VAR	= 8,	
	HLVAR	= 9,	/* �䳤,һ���ֽڵ�ֵ��ʾ, 0x26��ʾ38�ֽڳ��ȵ�  */
	HLLVAR	= 10,	/* �䳤, ��λ��ǰ, ��λ�ں�, ��ͬ */
	HLLLVAR	= 11,	
	HL4VAR	= 12,	

	BLVAR	= 13,	/* �䳤,һ���ֽڵ�ֵ��ʾ, 0x26��ʾ38�ֽڳ��ȵ�  */
	BLLVAR	= 14,	/* �䳤, ��λ��ǰ, ��λ�ں�, ��ͬ */
	BLLLVAR	= 15,	
	BL4VAR	= 16,	
	AJP_STRING	= 21,	/* AJP��string��ʽ, ������ֳ���2�ֽ�Ϊ0xffff, ��Ϸ�, ���޴��� */
	AJP_HEAD	= 50,	/* AJP Head: num_headers      (integer)
					request_headers *(req_header_name req_header_value) */
	AJP_ATTRIBUTE	= 51,	/* AJP attribute: 
			   		attributes      *(attribut_name attribute_value)
			    		request_terminator (byte) OxFF */

	ASN1_BER= 52,	/* asn.1��BER��ʽ */
	ISOBITMAP=90,	/* λͼ, �̶�8�ֽ� */
	YLBITMAP= 97,	/* ����λͼ����, �����1bitΪ0, ��ȡ8�ֽ�, ����ȡ16�ֽ�, 
			   ���ٴ������(����ȡ24�ֽ�), oh! */
	LTERM	= 98,	/* ��ĳ�����ַ�Ϊ��β */
	WANTON	= 99,	/* ����, ���ڷ�����˵, ����һֱ��ĩβ; ���ںϳ���˵,����RIGID����, ���������ȼ�� */

	UNDEFINED= -1	/* δ���� */
};

enum Unit {	/* ���㳤�ȵĵ�λ */
	NIBBLE		=0,	/* ���ֽ� */
	UNI_BYTE 	=1,
	UNI_WORD	=2,
	UNI_DWORD	=3,
	ARRAY_ELE	=4	/*�Թ̶�����Ԫ�ֽ���Ϊ��λ */
};

enum MatchType {	/* ƥ������ */
	VAR_ANY	=0,	/* ���� */	
	CONSTANT=1,	/* ��ȫ��ͬ */
	BEGIN_W	=2,	/* ��ĳ����ͷ */
	END_W	=3
};

typedef struct _LenField {	/* ��ĳ�����������ָ����Χ */
	Unit unit; 		/* ���ȵ�λ */
	unsigned int fldno;	/* ��ָ�����ȵ���� */
	LocType type;		/* ��ȡ����ֵ�ķ�ʽ */
	inline _LenField* clone() {
		_LenField *n;
		n = new _LenField;
		n->unit = unit;
		n->fldno = fldno;
		n->type = type;
		return n;
	}
} LenFiled;

class Unipac: public Amor
{
public:
typedef struct _LenPara {
	Unit unit; 	/* ���ȵ�λ */
	int length;	/* �̶�����ֵ����󳤶�ֵ, -1��ʾ���� */
	int base;	/* �ַ���ת����ֵʱ���õĽ���, 2: ������, 8: 8����, 16:16����, 0:�ɱ� */
	int element_sz;	/* ����Ԫ��С */
	inline _LenPara* clone() {
		_LenPara *n;
		n = new _LenPara;
		n->unit = unit;
		n->length = length;
		n->base = base;
		n->element_sz = element_sz;
		return n;
	};
} LenPara;

typedef struct _ArrayPara {	/* ���鷽ʽ */
	int var_head_sz; 	/* �ɱ䳤�ĳ����ֽ��� */
	int many;	/* �̶�����ֵ��������ֵ, -1��ʾ���� */
	inline _ArrayPara* clone() {
		_ArrayPara *n;
		n = new _ArrayPara;
		n->var_head_sz = var_head_sz;
		n->many = many;
		return n;
	};
} ArrayPara ;

typedef struct _AjpHeadPara {	/* AJP head��ʽ */
	int head_num;	/* �̶�����ֵ��������ֵ, -1��ʾ���� */
	inline _AjpHeadPara* clone() {
		_AjpHeadPara *n;
		n = new _AjpHeadPara;
		n->head_num = head_num;
		return n;
	};
} AjpHeadPara ;

typedef struct _TermPara {
	unsigned int num; 	/* �ս��ַ������� */
	unsigned int *len;	/* �ս��ַ����������� */
	unsigned char **term;	/* �ս��ַ����������� */
	inline _TermPara* clone() 
	{
		_TermPara *n;
		unsigned int i;
		int total;
		n = new _TermPara;
		n->num = num;
		n->len = 0;
		n->term = 0;
		
		if (  num <= 0 ) 
			goto End;

		n->len = new unsigned int[num];
		n->term = new unsigned char* [ num ];
		total = 0;
		for ( i = 0 ; i < num ; i++ )
			total += len[i] +1;

		n->term[0] = new unsigned char[total];
		memset(n->term[0], 0, total);
		for ( i = 0 ; i < num ; i++ )
		{
			unsigned int &l = len[i];
			n->len[i] = l;
			memcpy(n->term[i], term[i], l);
			if ( i < num -1 )
				n->term[i+1] = n->term[i]+l+1;
		}
	End:
		return n;
	};
/*	
	inline _TermPara () 
	{
		num = 0;
		len = 0;
		term = 0;
	}
*/

	inline ~_TermPara () 
	{
		if ( num>0 )
			delete[] term[0];

		if (len ) delete[] len;
		if (term) delete[] (char*) term;
	};

} TermPara;

typedef struct _Locator {
	LocType type;
	void *para;		/* �綨����,
		��LenPara, TermPara, ArrayPara, AjpHeadPara  */
	inline void copy_to(_Locator &n) {
		n.type = type;
		n.para = para;
		if ( para )
		{	LenPara *np;
			TermPara *nt;
			//ArrayPara *na;
			AjpHeadPara *nj;

			switch (type) {
			case  LTERM:
				nt = ((TermPara *) para)->clone();
				n.para = nt;
				break;
			case WANTON:
			case UNDEFINED:	/* δ���� */
				n.para = 0;
				break;

			case AJP_HEAD:
			case AJP_ATTRIBUTE:
				nj = ((AjpHeadPara *) para)->clone();
				n.para = nj;
				break;
			default:	/* �������� */
				np = ((LenPara *) para)->clone();
				n.para = np;
				break;
			}
		}
	};
/*
	inline _Locator() {
		type = UNDEFINED;
		para = 0;
	}
*/
	inline ~_Locator() {
		if ( para )
		{
			switch (type) {
			case AJP_HEAD:
			case AJP_ATTRIBUTE:
				delete (AjpHeadPara *) para;
				break;
			case  LTERM:
				delete (TermPara *) para;
				break;
			case WANTON:
			case UNDEFINED:	/* δ���� */
				break;
			default:	/* �������� */
				delete (LenPara *) para;
				break;
			}
		}
	};
} Locator;

typedef struct _FldDef {
	int no;			/* ���, < 0 :δ����, >=0 : �ж���, �������±���ͬ */
	Locator locator;	/* ��Χ�綨����, ��LLVAR�� */
	unsigned int m_num; 	/* ������  */
	struct Match {
		MatchType type;  /* �����ݵ�ƥ������  */
		unsigned int len;	/* ���ݳ��� */
		unsigned char *val;	/*  ƥ���������  */
		bool NOT;	/* ���ȡ�� */
		Match () {
			NOT = false;
		};
	} *match;
	bool m_only;		/* ����ƥ�䵱ǰ��, �ڷֽⲻ��������, �ϳ�ʱ���������� */	

	inline void copy_to (_FldDef &n) {
		n.no = no;
		locator.copy_to(n.locator);
		n.m_num = m_num;
		n.m_only = m_only;
		if ( m_num > 0 )
		{	
			match = new Match[m_num];
		}
	}

	inline ~_FldDef () {
		if ( m_num > 0 )
		{	
			delete [] match[0].val;
			delete [] match;
		}
	}
} FldDef;

enum DIRECT { FACIO =1, SPONTE =2, BOTH= 0 };
typedef struct _PacDef {
	int max;   		/* ����������  */
	FldDef * fld;		/* ����ṹ  */
	DIRECT when;
	inline void copy_to (_PacDef &n) {
		int i;
		n.max = max;
		n.when = when;
		if ( max >= 0 ) 
		{
			n.fld = new FldDef[max+1];
			for ( i = 0 ; i <=max ; i++)
				fld[i].copy_to( n.fld[i] );
		}
	}

	inline ~_PacDef () {
		if ( fld )
			delete[] fld;
	}
} PacDef;
		
#include "PacData.h"
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	Unipac();
	~Unipac();
	enum BuffAct { COPY_BUFF, MOVE_BUFF, REF_BUFF };

private:
	Amor::Pius local_p;
	TBuffer *rcv_buf;	/* ������ڵ�����ݻ��� */
	TBuffer *snd_buf;

	PacketObj *rcv_pac;	/* ������ڵ��PacketObj */
	PacketObj *snd_pac;

	TBuffer un_buf;
	TBuffer do_buf;		/* �������ҽڵ�����ݻ��� */

	PacketObj un_pac;
	PacketObj do_pac;	/* �������ҽڵ��PacketObj */

	TBuffer *tb[3];
	PacketObj *pa[3];

	Amor::Pius tb_ps;
	Amor::Pius pa_ps;

	struct G_CFG {
		int segNum;	/* ������� */
		PacDef	*defSegs;	/* ���Ķ������ */

		int pacWhat;	/* ������PRO_UNIPAC��DO_UNIPACʱ, Ŀ���������� */
		int fldOffset;	/* ����PacketObjʱ, �����е���ż��ϴ�ֵ(ƫ����)��ʵ�ʴ������, ��ʼΪ0 */

		bool hasBuff;	/* �������Ƿ�ӵ��TBuffer. true: ��clone_readyʱ, ���Ҵ���TBuffer
				   ��Ҳ��ζ������PacketObj������� rcv_pac, snd_pac�����á�
				 */
		bool hasPack;	/* �������Ƿ�ӵ��Packet. true: ��clone_readyʱ, ���Ҵ���PacketObj
					����ζ���������������ߵ�TBuffer����, 
				 */
				/* �����߲���ͬʱΪtrue, ������ͬʱΪfalse, ����ʱpacWhat�����о���ֵ */

		BuffAct buffAct;	/* ���ʱ������Ĵ�����. */
		bool inversed ;		/* �Ƿ����� */
		int buf_extra_sz;	/* ����������ʱ, ��Ҫ�����ݻ������������Ŵ��� */
		inline G_CFG () {
			buf_extra_sz = 0;
			segNum = 0;	/* ������ */
			defSegs = 0;
			pacWhat = -1;
			fldOffset = 0;
			inversed = false;	/* Ĭ�������� */
			hasBuff = false;	/* �Ƿ�Ϊ������֮Դ, true: ��clone_readyʱ, ���Ҵ��ݵ�ַ */
			hasPack = false;	/* �Ƿ�ΪPacket֮Դ, true: ��clone_readyʱ, ���Ҵ��ݵ�ַ */
			buffAct = MOVE_BUFF;	/* Ĭ���ǽ��ʱ��buffer�Ƶ�packet�ڲ� */
		};
		inline ~G_CFG() {
			if ( defSegs)	
				delete[] defSegs;
		}
	};
	struct G_CFG *gCFG;
	bool has_config;

	PACINLINE void deliver(Notitia::HERE_ORDO aordo, bool inver=false);
	PACINLINE void reset();

	PACINLINE bool unpack(unsigned char*, long int, PacketObj &, DIRECT direct );
	PACINLINE int unfield(unsigned char*, long int, PacketObj &, FieldObj &, FldDef &, bool &);
	PACINLINE bool dopack(TBuffer *, PacketObj &, DIRECT direct );
	PACINLINE void dobitmap(PacketObj &, FldDef &);
	PACINLINE bool dofield(TBuffer &, FieldObj &, FldDef &);
	PACINLINE bool domatch(FieldObj &, FldDef &);

	/* ����Ϊ�������õ��ı���, ���ں�����, �Ƿ�Ϊ����stack�Ŀ�������ʡʱ��? */
	int *map_index;	/* λͼ��������, ÿ��ֵ��ʾ���, �Ӷ��ܹ��ҵ���Ӧ�Ķ��������� */
	int mapi_num;	/* ���� */
	int mapi_sub;	/* λͼ�����±���ʼָʾ, �Ӹ��±꿪ʼ������, ��ʼΪ0 */
	TBuffer do_tmp;		/* �ϳɱ�����ʱ�洢 */
	unsigned char yaBuf[16];
#include "wlog.h"
};

#include <assert.h>
#include "const_para.h"
#include "cupsDef.h"
#include "iso8583-87.h"
static Unipac::PacDef *defBase = 0;

void Unipac::ignite(TiXmlElement *cfg)
{
	const char *comm_str;
	TiXmlElement *seg_ele, *fld_ele;
	
	int  b, i, j;
	int maxium_fldno; /* ����������,���û���ⲿ����, ��Ϊ���Ķ����1.5�� */
	b = 0;	/* Ĭ��ʹ��ISO8583:1987 */

	if (!cfg) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
	}

	if ( gCFG->defSegs)	
		delete[] gCFG->defSegs;
	gCFG->segNum = 0;

	gCFG->inversed =  (comm_str = cfg->Attribute("inverse"))  && (strcasecmp(comm_str, "yes") == 0 );
	if ( (comm_str = cfg->Attribute("base")) )
	{
		if (strcasecmp(comm_str, "ISO8583:1987") == 0 )
			b = 0;
		else if (strcasecmp(comm_str, "cups2.0") == 0 )
			b = 1;
		else 
			b = -1;
	}
	if ( !defBase )
		defBase = new PacDef[2];
	defBase[0].max= 128;
	defBase[0].fld = iso8583_87;
	defBase[0].when = BOTH;
	defBase[1].max= 192;
	defBase[1].fld = cups2_0;
	defBase[1].when = BOTH;

	seg_ele= cfg->FirstChildElement("packet"); gCFG->segNum = 0;
	while(seg_ele)
	{
		gCFG->segNum++;
		seg_ele = seg_ele->NextSiblingElement("packet");
	}
	if ( gCFG->segNum == 0 )
	{
		if ( b >= 0 ) /* û���ⲿ����, ��ָ�����ڲ����� */
			gCFG->segNum = 1;
		else	/* û��ָ���ڲ����Ķ���, Ҳû���ⲿ����, ������ */
			return;
	}
	gCFG->defSegs = new PacDef[gCFG->segNum];
	for ( i = 0 ; i < gCFG->segNum ; i++ )
	{
		if ( b >= 0 )
		{
			defBase[b].copy_to(gCFG->defSegs[i]);
		} else {
			gCFG->defSegs[i].max = -1;
			gCFG->defSegs[i].fld = 0;
			gCFG->defSegs[i].when = BOTH;
		}
	}

	/* ÿһ�ζ��崦�� */
	maxium_fldno = -1;
	gCFG->buf_extra_sz  = 0 ;
	for(	seg_ele= cfg->FirstChildElement("packet"), i = 0;
		seg_ele;
		seg_ele = seg_ele->NextSiblingElement("packet"),i++)
	{
		int extra_size = 0;	/* �������������Ҫ�Ŀռ䣬Ϊ��ASN1, AJP_HEAD, AJP_ATTRIBUTE */
		int &fldMax = gCFG->defSegs[i].max;

		gCFG->defSegs[i].when = BOTH;
		
		comm_str = seg_ele->Attribute("only");
		if ( comm_str )
		{
			if ( strcmp(comm_str, "facio") == 0 )
				gCFG->defSegs[i].when = FACIO;
				
			if ( strcmp(comm_str, "sponte") == 0 )
				gCFG->defSegs[i].when = SPONTE;
		}

		if ( gCFG->defSegs[i].fld == (FldDef*)0 )	/* û�вο��������� */
		{
			for( 	fld_ele = seg_ele->FirstChildElement("field");
				fld_ele;
				fld_ele = fld_ele->NextSiblingElement("field") )
			{	
				comm_str = fld_ele->Attribute("no");
				if ( comm_str && fldMax < atoi(comm_str) )
						fldMax = atoi(comm_str);
			}

			if ( fldMax >= 0 )
			{
				gCFG->defSegs[i].fld = new FldDef[fldMax + 1];
				memset(gCFG->defSegs[i].fld, 0 , sizeof(FldDef) * (fldMax+1));
				for ( j = 0 ; j <= fldMax; j++ )
					gCFG->defSegs[i].fld[j].no = -1;  /* each field is not defined yet */
			}
		}

		if ( fldMax > maxium_fldno ) 
			maxium_fldno = fldMax;

		/* ����ÿ����Ķ��� */
		for( 	fld_ele = seg_ele->FirstChildElement("field");
			fld_ele;
			fld_ele = fld_ele->NextSiblingElement("field") )
		{	
			FldDef *fdef;
			LenPara *pra;
			TermPara *tra;
			//ArrayPara *ara;
			AjpHeadPara *jra;

			TiXmlElement *t_ele, *m_ele;
			int no;	/* field no of definition */

			unsigned int total_len, k;
			no = -1;
			comm_str = fld_ele->Attribute("no");
			if ( comm_str )
				no = atoi(comm_str);

			if ( no > fldMax || no < 0 ) 
				continue;	/* ���������� �� δָ�����, ��Ч */

			comm_str = fld_ele->Attribute("locate");
			if ( !comm_str ) 
				continue;	/* �޶�λ��ʽ, ��Ч */

			fdef = &(gCFG->defSegs[i].fld[no]);
			fdef->locator.type = UNDEFINED;
			#define LOC(X) if ( comm_str && strcasecmp(comm_str, #X) == 0 )	\
					fdef->locator.type =X;
			LOC(RIGID)
			LOC(LVAR)
			LOC(LLVAR)
			LOC(LLLVAR)
			LOC(L4VAR)
			LOC(ALVAR)
			LOC(ALLVAR)
			LOC(ALLLVAR)
			LOC(AL4VAR)
			LOC(HLVAR)	
			LOC(HLLVAR)	
			LOC(HLLLVAR)	
			LOC(HL4VAR)	
			LOC(BLVAR)	
			LOC(BLLVAR)	
			LOC(AJP_STRING)	
			LOC(BLLLVAR)	
			LOC(BL4VAR)	
			LOC(ASN1_BER)	
			LOC(ISOBITMAP)	
			LOC(YLBITMAP)	
			LOC(LTERM)      
			LOC(WANTON)	
			LOC(AJP_HEAD)
			LOC(AJP_ATTRIBUTE)
			if ( comm_str && strcasecmp(comm_str, "toend") == 0 )
					fdef->locator.type =WANTON;
			#undef LOC

			if ( fdef->locator.type == UNDEFINED )
				continue;	/* δ֪��λ��ʽ, ��Ч */

			fdef->no = no;	/* �����϶�������Ϸ� */

			switch(fdef->locator.type)
			{
			case RIGID:
			case LVAR:	
			case LLVAR:	
			case LLLVAR:	
			case L4VAR:	
			case ALVAR:	
			case ALLVAR:	
			case ALLLVAR:	
			case AL4VAR:	
			case HLVAR:	
			case HLLVAR:	
			case HLLLVAR:	
			case HL4VAR:	       
			case BLVAR:	
			case BLLVAR:	
			case AJP_STRING:	
			case BLLLVAR:	
			case BL4VAR:	        
				goto NORM_LEN_PRO;
			case ASN1_BER:
				extra_size += (sizeof(struct ComplexType)+sizeof(struct Asn1Type));
			NORM_LEN_PRO:
				fdef->locator.para = pra = new LenPara;
				pra->length = -1;	/* no limit if not RIGID */
				comm_str = fld_ele->Attribute("length");
				if ( comm_str )
					pra->length = atoi(comm_str);
				if ( pra->length < 0 && fdef->locator.type == RIGID )
					pra->length = 0;
					
				pra->base = 10;		/* Ĭ��Ϊ10���� */
				comm_str = fld_ele->Attribute("base");
				if ( comm_str )
					pra->base= atoi(comm_str);
				if ( pra->base < 2 || pra->base > 36 ) 
					pra->base= 0;

				pra->unit= UNI_BYTE;	/* default, I think as byte */
				comm_str = fld_ele->Attribute("unit");
				if ( comm_str ) 
				{	
					if ( strcasecmp(comm_str, "nibble") == 0 )
						pra->unit= NIBBLE;
					else if (  strcasecmp(comm_str, "byte") == 0 )
						pra->unit= UNI_BYTE;
					else if (strcasecmp(comm_str, "word") == 0 )
						pra->unit= UNI_WORD;
					else if (strcasecmp(comm_str, "double_word") == 0 )
						pra->unit= UNI_DWORD;
					else if (strcasecmp(comm_str, "element") == 0 )
					{
						pra->unit= ARRAY_ELE;
						pra->element_sz = 0;
						fld_ele->QueryIntAttribute("element", &pra->element_sz);
						if ( pra->element_sz <= 0 )
							pra->element_sz = 1;
					}
				}

				break;

			case LTERM:
				fdef->locator.para = tra = new TermPara;
				tra->num = 0;
				tra->len = 0;
				tra->term = 0;
				/* �ȼ����ж��ٸ�term */
				total_len = 0;
				for ( t_ele = fld_ele->FirstChildElement("term");
					t_ele;
					t_ele = t_ele->NextSiblingElement("term"))
				{
					tra->num++;
					total_len += 1;
					if ( t_ele->GetText())
						total_len += strlen(t_ele->GetText());
				}
				if ( tra->num == 0 ) 
					goto LTERM_END;

				tra->len = new unsigned int[tra->num];
				tra->term = new unsigned char*[tra->num];
				tra->term[0] = new unsigned char[total_len];
				memset(tra->term[0], 0, total_len);
				for ( t_ele = fld_ele->FirstChildElement("term"), k= 0;
					t_ele;
					t_ele = t_ele->NextSiblingElement("term"), k++)
				{
					tra->len[k] = BTool::unescape(t_ele->GetText(), tra->term[k]) ;
					if ( k+1 < tra->num )
						tra->term[k+1]= tra->term[k]+tra->len[k] +1;
				}
		LTERM_END:
				break;

			case WANTON:
			case YLBITMAP:
			case ISOBITMAP:
				fdef->locator.para = 0;
				break;

			case AJP_HEAD:
			case AJP_ATTRIBUTE:
				fdef->locator.para = jra = new AjpHeadPara;
				jra->head_num = 15;		/* Ĭ�����15 */
				fld_ele->QueryIntAttribute("max", &jra->head_num);
				fld_ele->QueryIntAttribute("maxium", &jra->head_num);
				extra_size += (sizeof(struct ComplexType)+jra->head_num * sizeof(struct AjpHeadAttrType));
				break;

			default:
				fdef->locator.para = 0;
				break;
			}

			/* ����ƥ����� */
			fdef->m_only = false;	/* Ĭ�ϲ��ǽ�ƥ�� */
			if ( (comm_str = fld_ele->Attribute("vain")) && strcasecmp(comm_str, "yes") == 0 )
				fdef->m_only = true;
			
			fdef->m_num = 0; total_len = 0;
			for ( m_ele = fld_ele->FirstChildElement();
				m_ele;
				m_ele = m_ele->NextSiblingElement())
			{
				if ( strcasecmp ( m_ele->Value(), "match" ) == 0 
					|| strcasecmp ( m_ele->Value(), "dismatch" ) == 0 )
				{
					fdef->m_num++;
					total_len += 1;
					if ( m_ele->GetText())
						total_len += strlen(m_ele->GetText());
				}
			}
			
			if ( fdef->m_num == 0 )	/* ��������������, Ҫ������֮ǰ, �м�! */
				continue;

			fdef->match = new FldDef::Match [fdef->m_num];
			fdef->match[0].val = new unsigned char[total_len];
			memset(fdef->match[0].val, 0 , total_len);
			k = 0;
			for ( m_ele = fld_ele->FirstChildElement();
				m_ele;
				m_ele = m_ele->NextSiblingElement())
			{
				if ( strcasecmp ( m_ele->Value(), "match" ) == 0 
					|| strcasecmp ( m_ele->Value(), "dismatch" ) == 0 )
				{
					FldDef::Match &match = fdef->match[k];

					if ( strcasecmp ( m_ele->Value(), "dismatch" ) == 0 )
						match.NOT = true;
					else
						match.NOT = false;

					match.type = CONSTANT;	/* Ĭ��Ϊ����� */
					match.len = BTool::unescape(m_ele->GetText(), match.val) ;
					comm_str = m_ele->Attribute("type");
					if ( !comm_str ) 
						goto GETNEXT;	

					if ( strcasecmp(comm_str, "any") == 0 )
						match.type = VAR_ANY;
					if ( strcasecmp(comm_str, "constant") == 0 )
						match.type = CONSTANT;
					if ( strcasecmp(comm_str, "begin") == 0 )
						match.type = BEGIN_W;
					if ( strcasecmp(comm_str, "end") == 0 )
						match.type = END_W;
				GETNEXT:
					if ( match.len == 0 )
						match.type = VAR_ANY;	/* û�����ݱ���Ϊ������ */

					if ( k+1 < fdef->m_num )
						fdef->match[k+1].val  = match.val + match.len + 1;
					k++;	/* ��һ��*/
				}
			}
		}
		/* һ�ζ������ */
		if ( extra_size > gCFG->buf_extra_sz ) 	/* ���⻺������Сȡ���ֵ */
			gCFG->buf_extra_sz  = extra_size  ;
		
	}

	if ( (comm_str = cfg->Attribute("maxium")) || (comm_str = cfg->Attribute("max")) )
		maxium_fldno = atoi(comm_str);
	else
		maxium_fldno += maxium_fldno/2;

	if ( (comm_str = cfg->Attribute("provide")) )
	{
		if ( strcasecmp(comm_str, "buffer") == 0 )
			gCFG->hasBuff = true;
		else if ( strcasecmp(comm_str, "packet") == 0 )
			gCFG->hasPack = true;
		else
			gCFG->pacWhat = atoi(comm_str);
	}

	if ( (comm_str = cfg->Attribute("buffer")) )
	{
		if ( strcasecmp(comm_str, "move") == 0 )
			gCFG->buffAct = MOVE_BUFF;
		else if ( strcasecmp(comm_str, "copy") == 0 )
			gCFG->buffAct = COPY_BUFF;
		else if ( strcasecmp(comm_str, "refer") == 0 )
			gCFG->buffAct = REF_BUFF;
	}

	if ( (comm_str = cfg->Attribute("offset")) )
	{
		gCFG->fldOffset = atoi(comm_str);
		if ( gCFG->fldOffset < 0 ) 
			gCFG->fldOffset = 0;
	}

	if ( gCFG->hasPack )
	{
		do_pac.produce(maxium_fldno) ;
		un_pac.produce(maxium_fldno) ;
	}
}

Unipac::Unipac():do_tmp(1024)
{
	tb_ps.ordo = Notitia::PRO_TBUF;
	tb_ps.indic = 0;

	pa_ps.ordo = Notitia::PRO_TBUF;
	pa_ps.indic = 0;

	local_p.ordo = Notitia::PRO_UNIPAC;
	local_p.indic = 0;

	rcv_buf = 0;
	snd_buf = 0;
	rcv_pac = 0;
	snd_pac = 0;
	tb[0] = &do_buf;
	tb[1] = &un_buf;
	tb[2] = 0;

	pa[0] = &un_pac;
	pa[1] = &do_pac;
	pa[2] = 0;
	
	map_index = (int*)0;
	mapi_num = 0;
	mapi_sub = 0;

	has_config = false;	/* ��ʼ��Ϊ�Լ���ӵ��ȫ�ֲ����� */
	gCFG = 0;
}

Unipac::~Unipac() 
{
	if ( has_config && gCFG)
		delete gCFG;

	if ( map_index )
		delete[] map_index;
}

Amor* Unipac::clone()
{
	Unipac *child = new Unipac();

	child->gCFG = gCFG;

	if ( gCFG && gCFG->hasPack )
	{
		child->do_pac.produce(do_pac.max);
		child->un_pac.produce(un_pac.max);
	}
	return (Amor*) child;
}

bool Unipac::facio( Amor::Pius *pius)
{
	TBuffer **tmt;
	PacketObj **tmp;
	assert(pius);

	if (!gCFG )
		return false;
	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF:
		WBUG("facio PRO_TBUF");
		if ( rcv_buf && gCFG->hasPack )
		{	/* ��һ�α��Ľ��� */
			un_pac.reset();
			switch ( gCFG->buffAct )
			{
			case COPY_BUFF:
				un_pac.buf.input(rcv_buf->base, rcv_buf->point - rcv_buf->base);	/* ����rcv_buf������ */
				goto  NOW_UNPACK;
			
			case MOVE_BUFF :
				TBuffer::exchange(un_pac.buf, *rcv_buf);	/* ��rcv_buf��un_pac.buf������ַ����,�����޿���*/
			NOW_UNPACK:
				if (unpack(un_pac.buf.base, un_pac.buf.point - un_pac.buf.base, un_pac,FACIO))
					aptus->facio(&local_p);
				break;

			case REF_BUFF :	/* �����������ǲ���packet.adjust(), �����ڴ���� */
				if (unpack(rcv_buf->base, rcv_buf->point - rcv_buf->base, un_pac,FACIO))
					aptus->facio(&local_p);
				break;

			default :
				break;
			}
		}
		break;

	case Notitia::PRO_UNIPAC:
		WBUG("facio PRO_UNIPAC");

		if ( gCFG->inversed ) 
			goto DO_PAC;

		if ( gCFG->pacWhat >= 0 && rcv_pac && rcv_pac->max >= gCFG->pacWhat )
		{
			FieldObj *obj;
			obj = &rcv_pac->fld[gCFG->pacWhat];
			if (unpack(obj->val, obj->range, *rcv_pac, FACIO))
				aptus->facio(&local_p);
		}
		break;
DO_PAC:
		if ( rcv_pac && gCFG->hasBuff )
		{	/* �����������ĺϳ�, ���е������ݶ���rcv_pac��, ���ɵ�������do_buf��, Ȼ�����Ҵ� */
			if ( dopack(&do_buf, *rcv_pac, FACIO) )
				aptus->facio(&pa_ps);

		} else if ( gCFG->pacWhat >=0 && rcv_pac && rcv_pac->max >= gCFG->pacWhat)
		{
			if ( dopack(0, *rcv_pac, FACIO) )
				aptus->facio(&local_p);
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

			if ( rcv_pac && snd_pac )
			{
				WBUG("facio SET_UNIPAC rcv(%p) snd(%p)", rcv_pac, snd_pac);
			}

			if (!gCFG->hasBuff && !gCFG->hasPack)	/* hasBuff��hasPack�������Ҵ���, �ܿ�֮ */
				aptus->facio(pius);	/* ���unipac�ɴ�����, �������������OK */
		} else 
			WLOG(WARNING, "facio SET_UNIPAC null");
		break;

	case Notitia::SET_TBUF:
		WBUG("facio SET_TBUF");
		if ( (tmt = (TBuffer **)(pius->indic)))
		{
			if ( *tmt) rcv_buf = *tmt; 
			else
				WLOG(WARNING, "facio SET_TBUF rcv_buf null");
			tmt++;
			if ( *tmt) snd_buf = *tmt;
			else
				WLOG(WARNING, "facio SET_TBUF snd_buf null");
		} else 
			WLOG(WARNING, "facio SET_TBUF null");
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		reset();
		break;

	case Notitia::IGNITE_ALL_READY:	
		WBUG("facio IGNITE_ALL_READY" );			
		goto ALL_READY;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE(IGNITE)_ALL_READY" );			
ALL_READY:
		if ( gCFG->hasBuff)
			deliver(Notitia::SET_TBUF);
		if ( gCFG->hasPack )
			deliver(Notitia::SET_UNIPAC);
		break;

	default:
		return false;
	}
	return true;
}

bool Unipac::sponte( Amor::Pius *pius)
{
	assert(pius);
	if (!gCFG )
		return false;

	switch ( pius->ordo )
	{
	case Notitia::PRO_TBUF :
		WBUG("sponte PRO_TBUF");
		if ( snd_pac && gCFG->hasBuff )
		{	/* ��һ�α��Ľ��� */
			snd_pac->reset();
			switch ( gCFG->buffAct )
			{
			case COPY_BUFF:
				snd_pac->buf.input(un_buf.base, un_buf.point - un_buf.base);	/* ����un_buf������ */
				goto  NOW_UNPACK;
			
			case MOVE_BUFF :
				TBuffer::exchange(snd_pac->buf, un_buf);	/* ��un_buf��snd_pac.buf������ַ����,�����޿���*/
			NOW_UNPACK:
				if (unpack(snd_pac->buf.base, snd_pac->buf.point - snd_pac->buf.base, *snd_pac,SPONTE))
					aptus->sponte(&local_p);
				break;

			case REF_BUFF :	/* �����������ǲ���packet.adjust(), �����ڴ���� */
				if (unpack(un_buf.base, un_buf.point - un_buf.base, *snd_pac,SPONTE))
					aptus->sponte(&local_p);
				break;

			default :
				break;
			}
		}
		break;

	case Notitia::PRO_UNIPAC:
		WBUG("sponte PRO_UNIPAC");
	
		/* ��������, 
		if ( gCFG->hasPack )
		{
			int *aa = 0; *aa =0 ;		
		}
		*/

		if ( gCFG->inversed ) 
			goto UN_PAC;

		if ( snd_buf && gCFG->hasPack )
		{	/* �����������ĺϳ�, ���е������ݶ���do_pac��, ���ɵ�������snd_buf��, ������ */
			if ( dopack(snd_buf, do_pac, SPONTE) )
				aptus->sponte(&tb_ps);

		} else if ( gCFG->pacWhat >=0 && snd_pac && snd_pac->max >= gCFG->pacWhat)
		{
			if( dopack(0, *snd_pac, SPONTE) )
				aptus->sponte(&local_p);
		}
		break;
UN_PAC:
		if ( gCFG->pacWhat >= 0 && snd_pac && snd_pac->max >= gCFG->pacWhat )
		{
			FieldObj *obj;
			obj = &snd_pac->fld[gCFG->pacWhat];
			if (unpack(obj->val, obj->range, *snd_pac, SPONTE))
				aptus->sponte(&local_p);
		}
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("sponte DMD_END_SESSION");
		reset();
		break;

	default:
		return false;
	}
	return true;
}

/* ��������ύ */
PACINLINE void Unipac::deliver(Notitia::HERE_ORDO aordo, bool inver)
{
	Amor::Pius tmp_pius;
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	
	switch (aordo)
	{
	case Notitia::SET_TBUF:
		WBUG("deliver SET_TBUF");
		tmp_pius.indic = &tb[0];
		break;

	case Notitia::SET_UNIPAC:
		WBUG("deliver SET_UNIPAC");
		tmp_pius.indic = &pa[0];
		break;

	default:
		WBUG("deliver Notitia::%d",aordo);
		break;
	}

	if ( inver )
		aptus->sponte(&tmp_pius);
	else
		aptus->facio(&tmp_pius);
	return ;
}

PACINLINE void Unipac::reset()
{
	do_buf.reset();
	un_buf.reset();
	do_pac.reset();
	un_pac.reset();
}

PACINLINE bool Unipac::unpack(unsigned char* raw, long length, PacketObj &packet, DIRECT direct)
{
	int i,j, ret;
	bool mapped ;
	int mi, sub;	/* mi: ָʾmap_index����һ�����
			   sub: ָʾpac_def����һ��fld���� */
	PacDef *pac_def;
	FldDef *fld_def, *pri_fldDef = 0, *last_fldDef = 0;
	unsigned char* base;
	long limit;
	
	int perhap_seg = 0, max_last = 0;
	assert(packet.max >=0 );

	if ( length <= 0 ) 	/* ���κ����ݶ���Ϊ���ɹ� */
		return false;
	packet.buf.grant(gCFG->buf_extra_sz);	/* ��֤����Ŀռ�, ������ASN1��AJP_HEAD�ȶ���, һ�㶼Ϊ0 */
	/* λͼ������ʼ�� */
	if ( mapi_num <= packet.max+1 )
	{
		mapi_num = packet.max+2; /* ��һ��, map_index��-1��ʾ�������޶����� */
		if ( map_index )
			delete[] map_index;
		map_index = new int [ mapi_num ];
		memset(map_index, -1, sizeof(int)* mapi_num);
	}

	for ( i = 0 ; i < gCFG->segNum; i++ )
	{
		int last;	
		pac_def  = &gCFG->defSegs[i];
		mapped = false;
		mi = 0;
		sub = 0;
		base = raw;
		limit = length;

		mapi_sub = 0;
		if ( pac_def->when != BOTH && pac_def->when != direct ) continue; /* ���򲻶� */

	NEXTFLD:
		if ( mapped ) 
		{
			/* ��λͼ, ����λͼָʾ */
			if ( map_index[mi] < 0 ) 
			{	/* ��λͼ�������, �Ѵ��������ж������ */
				if ( limit == 0  ) 	/*�����Ѹպ÷�����, OK */
					break; 
				else
					goto RESET_NEXTPAC;
			} else if ( limit < 0 )		/* ������δ����, �����ݳ���Χ */
				goto RESET_NEXTPAC;
				
			fld_def = &pac_def->fld[map_index[mi]];
			mi++;
		} else {
			/* ��λλͼ�������, ��ǰ���������һ��λͼ, �����������Ҳ����� */
			if ( limit == 0 )	/* �պ÷��������ݵ�β, OK. ����δ�����һ����Ч���� */
				break;
		GETSUB_DEF:
			if ( sub > pac_def->max ) /* ���ж������Ѵ���, û��ǡ�÷�����β */
				goto RESET_NEXTPAC;

			fld_def = &pac_def->fld[sub];
			sub++;
			if ( fld_def->no < 0 )
				goto  GETSUB_DEF;	/* ����һ������ */
		}

		if (fld_def->no < 0 )
		{
			WLOG(ERR, "field %d not defined!", map_index[mi]);
			goto RESET_NEXTPAC;	/* ����һ������� */
		}
		/* fld_def Ϊ���ڿ��õ����� */

		if ( fld_def->no + gCFG->fldOffset > packet.max) /* ��������±� */
		{
			WLOG(ERR, "fld_def.no=[%d] exceeds packet.max=[%d]", fld_def->no + gCFG->fldOffset, packet.max);
			goto RESET_NEXTPAC;	/* ����һ������� */
		}

		ret = unfield(base, limit, packet, packet.fld[fld_def->no + gCFG->fldOffset], *fld_def, mapped);
		WBUG("unfield packet.fld[%d] fld_def.no=%d base=%p mapped=%d ret=%d", \
			fld_def->no + gCFG->fldOffset, fld_def->no, base, mapped, ret);

		if ( ret < 0 ) 		/* �������ʧ�� */
			goto RESET_NEXTPAC;	/* ����һ������� */

		pri_fldDef = fld_def;	/* ����һ����������ɹ����� */

		base += ret;
		limit -=ret;
		goto NEXTFLD;

	RESET_NEXTPAC:	/* �������ʧ��, �������������Ϊ������ */
		WBUG("unpack failed in seg %d, buffer left %ld bytes", i, limit);
		last = mapped ? mi : sub;
		if ( last >  max_last ) 
		{
			max_last = last;
			perhap_seg = i;
			last_fldDef = pri_fldDef;
		}
		for ( j  =0 ; j < last ; j++ )
		{
			fld_def = mapped ? &pac_def->fld[map_index[j]] :
						&pac_def->fld[j];
			if ( fld_def->no >= 0  && !fld_def->m_only	/* �鶨���򲻸�ԭ */
				&& fld_def->no + gCFG->fldOffset < packet.max )
				packet.fld[fld_def->no + gCFG->fldOffset].no = -1;
		}
	}

	if ( i == gCFG->segNum )	/* �޷��������ݰ� */
	{
		deliver(Notitia::ERR_UNIPAC_RESOLVE, gCFG->inversed);
		WLOG(WARNING, "Can not resolve the packet, perhaps seg %d, last mathced field definition %d", perhap_seg, last_fldDef ? last_fldDef->no : -1);
		return false;
	}  else
	{
		WBUG("Resolv the packet successfully in seg# %d!", i);
	}
	return true;
}

/* ����Ϊʵ�ʳԵ��ĳ���, < 0 �������ʧ�� */
PACINLINE int Unipac::unfield(unsigned char* base, long range, PacketObj &packet, FieldObj &field, 
	FldDef &fld_def, bool &mapped)
{
	unsigned int i, j, array_num  /* AJP_HEAD������Ԫ֮���, Ԫ�ظ��� */ ;
	int k;
	unsigned char *left = 0, *wp, twp;	/* �������ݵ����, ��С��base */
	long int len, 	/* �������ݳ��� */
		post_len;	/* ��׺���� */
	char l_str[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	LenPara *lpra = (LenPara *) fld_def.locator.para;
	TermPara *tpra = (TermPara *) fld_def.locator.para;
	AjpHeadPara *jpra = (AjpHeadPara *) fld_def.locator.para;

	struct AjpHeadAttrType *ajp_dat = 0;
	unsigned char bmc, tbit;
	int mib;

	len = 0;
	post_len = 0;

	if ( fld_def.m_only )	/* �������, �������� */
	{
		if ( field.no < 0 ) /* Ҫ�����Ѿ����� */
			return -1;

		if ( !domatch(field, fld_def))
			return -1;
		else
			return 0;
	}

	field.no = -1;

	switch ( fld_def.locator.type )
	{
	case RIGID:
		WBUG("fld[%d] RIGID", fld_def.no);
		len = lpra->length;
		left = base;
		break;

	case ISOBITMAP:
		WBUG("fld[%d] ISOBITMAP", fld_def.no);
		if ( range < 8 ) 
			return -1;
		len = 8;
		left = base;
		break;

	case YLBITMAP:
		WBUG("fld[%d] YLBITMAP", fld_def.no);
		if ( range < 8 ) 
			return -1;
		if ( base[0] & 0x80 ) 
		{	/* ��16�ֽ� */
			if ( range < 16 ) 
				return -1;
			len = 16;
		} else
			len = 8;

		left = base;
		break;

	case LVAR:	
		len = base[0] & 0x0F;	
		if ( len > 9 )
			return -1;
		left = base+1;
		break;

	case LLVAR:	
		len = base[0] & 0x0F;	
		if ( len > 9 ) 
			return -1;
		len += ((base[0] & 0xF0 ) >> 4 ) *10;
		if ( len > 99 )
			return -1;
		left = base+1;
		break;

	case LLLVAR:	
		if ( range < 2 )
			return -1;

		len = base[1] & 0x0F;	
		if ( len > 9 ) 
			return -1;
		len += ((base[1] & 0xF0 ) >> 4 ) *10;
		if ( len > 99 ) 
			return -1;

		len += (base[0] & 0x0F ) *100;
		if ( len > 999 )
			return -1;

		left = base+2;
		break;

	case L4VAR:	
		if ( range < 2 )
			return -1;

		len = base[1] & 0x0F;	
		if ( len > 9 ) 
			return -1;
		len += ((base[1] & 0xF0 ) >> 4 ) *10;
		if ( len > 99 ) 
			return -1;

		len += (base[0] & 0x0F ) *100;
		if ( len > 999 ) 
			return -1;

		len += ((base[0] & 0xF0 ) >> 4 ) *1000;
		if ( len > 9999 )
			return -1;

		left = base+2;
		break;

	case ALVAR:	
		WBUG("fld[%d] ALVAR", fld_def.no);
		memcpy(l_str, base, 1);
#define AVAR_LEN len = strtol(l_str, 0, lpra->base); \
		if ( len == LONG_MIN || len == LONG_MAX ) \
			return -1;
		AVAR_LEN;

		left = base+1;
		break;

	case ALLVAR:	
		WBUG("fld[%d] ALLVAR", fld_def.no);
		if ( range < 2 )
			return -1;
		memcpy(l_str, base, 2);
		AVAR_LEN;

		left = base+2;
		break;

	case ALLLVAR:	
		WBUG("fld[%d] ALLLVAR", fld_def.no);
		if ( range < 3 )
			return -1;
		memcpy(l_str, base, 3);
		AVAR_LEN;

		left = base+3;
		break;

	case AL4VAR:	
		if ( range < 4 )
			return -1;
		memcpy(l_str, base, 4);
		AVAR_LEN;

		left = base+4;
		break;

	case HLVAR:	
		len = base[0];
		left = base+1;
		break;

	case HLLVAR:	
		if ( range < 2 )
			return -1;

		len = base[0];
		len *=256;
		len += base[1];

		left = base+2;
		break;

	case HLLLVAR:	
		if ( range < 3 )
			return -1;
		len = base[0];
		len *=256;
		len += base[1];
		len *=256;
		len += base[2];

		left = base+3;
		break;

	case HL4VAR:	       
		if ( range < 4 )
			return -1;
		len = base[0];
		len *=256;
		len += base[1];
		len *=256;
		len += base[2];
		len *=256;
		len += base[3];

		left = base+4;
		break;

	case BLVAR:	
		len = base[0];
		left = base+1;
		break;

	case BLLVAR:	
		if ( range < 2 )
			return -1;

		len = base[1];
		len *=256;
		len += base[0];

		left = base+2;
		break;

	case AJP_STRING:	
		if ( range < 2 )
			return -1;
		
		left = base+2;
		if ( base[0] == 0xff && base[1] == 0xff)
		{
			len = 0;
			post_len = 0;	/* ��βû��0 */
		} else {
			len = (base[0] << 8) + base[1];
			post_len = 1;	/* ��β����һ��0 */
			if ( left[len] != 0x0)
				return -1;
		}
		break;

	case BLLLVAR:	
		if ( range < 3 )
			return -1;

		len = base[2];
		len *=256;
		len += base[1];
		len *=256;
		len += base[0];

		left = base+3;
		break;

	case BL4VAR:	        
		if ( range < 4 )
			return -1;

		len = base[3];
		len *=256;
		len += base[2];
		len *=256;
		len += base[1];
		len *=256;
		len += base[0];

		left = base+4;
		break;

	case ASN1_BER:
		if ( range < 2 )
			return -1;
		
		field.other = (struct ComplexType *)(packet.alloc_buf(sizeof(struct ComplexType)));
		field.other->type = PacketObj::ASN1;
		field.other->value = (packet.alloc_buf(sizeof(struct Asn1Type)));
		((struct Asn1Type *)field.other->value)->kind = (char)base[0]; 	/* ��һ���ֽ�ΪASN1���������� */

		if (base[1] & 0x80) 
		{
			int i, sz;

			sz = base[1] & 0x7F;
			if (sz == 0 || sz > (int) (sizeof(int)) || range < sz+2 ) 
				return -1;

			len = 0;
			for (i = 0; i < sz; i++) 
				len = len << 8 | (base[i+2] & 0xff);

			left = base+i+2;
		} else {
			len    = (int) base[1];
			left = base+2;
		}
		((struct Asn1Type *)field.other->value)->val = left;
		((struct Asn1Type *)field.other->value)->len = len;

		break;

	case AJP_HEAD:
		if ( range < 2 )
			return -1;

		array_num = (base[0] << 8) + base[1];	/* ȡ��head���� */
		left = base;	/*����RIGID��ʽ */
		if ( jpra->head_num >0 && (long)array_num > jpra->head_num)
		{	
			WBUG("head_num=%u too large for limit is %d", array_num, jpra->head_num);
			return -1;
		}
		field.other = (struct ComplexType *)(packet.alloc_buf(sizeof(struct ComplexType)));
		field.other->type = PacketObj::AJP_HEAD_ATTR;
		if ( array_num > 0 ) 
		{
			field.other->value = (packet.alloc_buf(array_num*sizeof(struct AjpHeadAttrType)));
			ajp_dat = (struct AjpHeadAttrType*)field.other->value;
		} else {
			field.other->value = 0;
			goto END_AJP_PRO;
		}

		len = 2;
		wp = &left[2];	/* wp��������Ĺ���ָ����, left�Ͳ����� */
		for ( i = 0 ; i < array_num; i++)
		{
			len +=2;
			if ( range < len )	/* ԭ�еĳ���, ���������ֽڱ�ʾ�����ֽ�����sc_name, ����Χ�� */
				break;

			if ( wp[0] == 0xA0 ) /* sc_name */
			{
				ajp_dat->sc_name = (wp[0] << 8) + wp[1];
				wp +=2;
				ajp_dat->nm_len = 0;
				ajp_dat->name = 0;
			} else {		/* �����ֽڱ�ʾһ�����Ƶĳ��� */
				ajp_dat->sc_name = PacketObj::INVALID_SC_NAME;
				ajp_dat->nm_len = (wp[0] << 8) + wp[1] + 1;	/*���Ȱ�����β��0x0, ���Զ��1 */
				wp +=2;
				len += ajp_dat->nm_len;
				if ( range < len )	/* ����name�ĳ���, ����Χ�� */
					break;

				ajp_dat->name = (char*)wp;
				wp +=ajp_dat->nm_len;
			}

			/* ������, ���������� */
			len +=2;
			if ( range < len )	/* ԭ�еĳ���, ���������ֽڱ�ʾ�����ֽ���, ����Χ�� */
				break;

			ajp_dat->str_len = (wp[0] << 8) + wp[1] + 1;	/*���Ȱ�����β��0x0, ���Զ��1 */
			wp +=2;
			len += ajp_dat->str_len;
			if ( range < len )	/* �������ݵĳ���, ����Χ�� */
				break;

			ajp_dat->string = (char*)wp;
			wp += ajp_dat->str_len;
			
			if ( i < (array_num-1) )
				//ajp_dat->next = (struct AjpHeadAttrType*)&(field.other->value[(i+1)*sizeof(struct AjpHeadAttrType)]);
				ajp_dat->next = &ajp_dat[1];
			else
				ajp_dat->next = 0;

			ajp_dat = ajp_dat->next;
		}

		if ( i != array_num ) /* �����û�з�������, ����ʧ�� */
		{
			packet.buf.point -= (array_num*sizeof(struct AjpHeadAttrType) + sizeof(struct ComplexType));	/* �ճ�ԭ��ռ�еĿռ� */
			return -1;
		}
	END_AJP_PRO:
		lpra = 0;	/*���Ȳ������岻������ */
		break;

	case AJP_ATTRIBUTE:
		if ( range < 1 )
			return -1;

		left = base;	/*����RIGID��ʽ */
		wp = left;
		len = 1; 
		array_num = 0;	/* attribute���� */
		field.other = (struct ComplexType *)(packet.alloc_buf(sizeof(struct ComplexType)));
		field.other->type = PacketObj::AJP_HEAD_ATTR;
		field.other->value = 0;
		twp = wp[0];

		while ( twp != 0xFF) 
		{
			array_num++;
			if ( jpra->head_num >0 && (long)array_num > jpra->head_num)
			{	
				WBUG("attribu_num=%u too large for limit is %d", array_num, jpra->head_num);
				break;
			}
			if ( array_num == 1 )
			{
				field.other->value = (packet.alloc_buf(array_num*sizeof(struct AjpHeadAttrType)));
				ajp_dat = (struct AjpHeadAttrType*)field.other->value;
				ajp_dat->next = 0x0;
			} else {
				ajp_dat->next = (struct AjpHeadAttrType*)&(field.other->value[sizeof(struct AjpHeadAttrType)]);
				ajp_dat = ajp_dat->next;
			}

			if ( wp[0] == 0x0A )	
			{	/* ���һ�Ե�name��string */
				len += 2;	/* �����������ֽڱ�ʾ���� */
				if ( range < len )	/* ԭ�еĳ���, ���������ֽڱ�ʾ�����ֽ���, ����Χ�� */
					break;

				ajp_dat->sc_name = wp[0];
				ajp_dat->nm_len = (wp[1] << 8) + wp[2] + 1;	/*���Ȱ�����β��0x0, ���Զ��1 */
				wp +=3;
				len += ajp_dat->nm_len;
				if ( range < len )	/* ����name�ĳ���, ����Χ�� */
					break;

				ajp_dat->name = (char*)wp;
				wp +=ajp_dat->nm_len;

			} else {	/* ��ֱ������ */
				ajp_dat->sc_name = wp[0];
				ajp_dat->nm_len = 0;
				ajp_dat->name = 0;
				wp++;
			}
			
			/* ������, ���������� */
			len += 2;
			if ( range < len )	/* ԭ�еĳ���, ���������ֽڱ�ʾ�����ֽ���, ����Χ�� */
				break;

			ajp_dat->str_len = (wp[0] << 8) + wp[1] + 1;	/*���Ȱ�����β��0x0, ���Զ��1 */
			wp +=2;
			len += ajp_dat->str_len;
			if ( range < len )	/* �������ݵĳ���, ����Χ�� */
				break;

			ajp_dat->string = (char*)wp;
			wp += ajp_dat->str_len;	/* ָ����һ������ */
		
			len++;
			if ( range < len )	/* ԭ�еĳ���, ����1�ֽڵ�β0xff����, ����Χ�� */
			{
				twp = 0x0;	/* ��־һ��, ���ʧ��,���ܺ���һ������Χ���ֽھ���0xFF */
				break;
			} else 
				twp = wp[0];
		}

		if ( twp != 0xFF ) /* �����û�з�������, ����ʧ�� */
		{
			packet.buf.point -= (array_num*sizeof(struct AjpHeadAttrType) + sizeof(struct ComplexType));	/* �ճ�ԭ��ռ�еĿռ� */
			return -1;
		}
		lpra = 0;	/*���Ȳ������岻������ */
		break;

	case LTERM:
		WBUG("fld[%d] LTERM num %d", fld_def.no, tpra->num);
		lpra = 0; 
		for ( i = 0 ; i < tpra->num ; i++ )
		{
			register unsigned char *p, *q;
			long int cmp;
			cmp = tpra->len[i];
			if ( cmp ==0 || range < cmp) continue;
			p = base;
			q = base + range -cmp;
			while ( p <= q )
			{
				if (memcmp ( p, tpra->term[i],  cmp ) == 0 )
					break;
				p++;
			}
			if ( p <= q ) /* �ҵ�����ս�� */
			{
				post_len = cmp;
				len = p-base;
				left = base;
				break;
			}
		}
		if ( i == tpra->num ) 
		{
			WBUG("fld[%d] LTERM no terminator", fld_def.no);
			return -1;
		}
		
		break;

	case WANTON:
		lpra = 0;
		left = base;
		len = range;
		break;

	default:
		return -1;
	}

	/* lpra ��LTERM, WANTON ������ֱ���Ϊ null */
	if ( lpra && lpra->length > 0 && len > (long) lpra->length )
	{
		WBUG("len=%lu too large for limit is %d", len, lpra->length);
		return - 1;
	}

	WBUG("fld[%d].len=%lu", fld_def.no, len);
	field.len = len;	/* ���峤�� */
	if ( lpra )
	{
		switch ( lpra->unit )
		{
		case NIBBLE:
			field.range = len /2;
			if ( len %2 == 1 ) 
				field.range++;
			break;

		case UNI_BYTE:
			field.range = len ;
			break;

		case UNI_WORD:
			field.range = len*2 ;
			break;

		case UNI_DWORD:
			field.range = len*4 ;
			break;

		case ARRAY_ELE:
			field.range = len*lpra->element_sz ;
			break;

		default:
			field.range = len ;
			break;
		}
	} else
	{
		field.range = len ;
	}
	
	field.raw = base;
	field.val = left;
	field._rlen = field.range + ( left-base) + post_len; 
	if ( (long ) field._rlen > range )	/* ʵ����û����ô���ֽ��� */
		return -1;

	if ( !domatch(field, fld_def))
		return -1;	

	/* λͼ���� */
	switch ( fld_def.locator.type )
	{
	case ISOBITMAP:
	case YLBITMAP:
		mib = fld_def.no == 0 ? 1 : fld_def.no + 64  ;
		/* mib: ��ָʾ��ŵĿ�ʼ, �뵱ǰ�����й�: �統ǰλͼΪ0��(��λͼ), ���1��ʼ;
			�統ǰΪ1��, ���65��ʼ; �統ǰΪ65��, ���129��ʼ�� */
		for ( k = 0 ; k < len; k++)
		{
			bmc = base[k];
			for ( j = 0, tbit = 0x80 ; j < 8; j++, tbit >>= 1 )
			{
				if ( bmc & tbit )
				{
					map_index[mapi_sub] = k*8 + j + mib;
					/* ����YLBITMAP, 65�� �� 129�� �ǲ����ڵ�, ǿ���Թ�. oh? */
					if ( fld_def.locator.type == YLBITMAP 
						&& k ==0 && j == 0
						&& ( mib == 65 || mib == 129 ) )
						continue;
					mapi_sub++;
				}
			}
		}

		map_index[mapi_sub] = -1; /* ��β*/
		mapped = true;
		break;

	default:
		break;
	}

	field.no = fld_def.no + gCFG->fldOffset;	/* ������Ч */
	return field._rlen;
}

PACINLINE bool Unipac::dopack(TBuffer *totb, PacketObj &packet, DIRECT direct )
{
	int i,j;
	PacDef *pac_def;
	FldDef *fld_def;
	FldDef *pri_fldDef = 0, *last_fldDef = 0;
	unsigned int oldOff;
	TBuffer *tbuf;
	unsigned char *old_base;

	bool ret = true;
	bool hasMap = false;
	int fldMax = packet.max;

	int perhap_seg = 0, max_last = 0;
	if ( totb )
		tbuf = totb;
	else 
		tbuf = &do_tmp;

	oldOff = packet.buf.point - packet.buf.base;
	for ( i = 0, pac_def = gCFG->defSegs ; i < gCFG->segNum; i++, pac_def++ )
	{
		bool success=true;
		if ( pac_def->when != BOTH && pac_def->when != direct ) continue; /* ���򲻶�, ����һ������ */
		tbuf->reset();	/* tbuf������ʱ��, ����rcv/snd_buf */
		/* �ȴ���λͼ, ����, ��Ϊǰ���λͼָ������λͼ�Ĵ������ */
		for ( j = pac_def->max; j >=0; j--)
		{
			fld_def = &pac_def->fld[j];
			if ( fld_def->no >= 0 && ( fld_def->locator.type == ISOBITMAP || 
				fld_def->locator.type == YLBITMAP ) )
			{
				dobitmap(packet, *fld_def);
				hasMap = true;
			}
		}

		for ( j = 0, fld_def = pac_def->fld; 
			j <= pac_def->max && fldMax >=gCFG->fldOffset+j;  /* j ������ʵ��packet�������� */
			j++, fld_def++)
		{
			if ( (hasMap && packet.fld[gCFG->fldOffset+j].no < 0) || fld_def->no < 0 ) 
				continue;

			if ( packet.fld[gCFG->fldOffset+j].no < 0	/* ��λͼ��,�ж�������ֵ,�ⲻ�� */
			     ||	!dofield(*tbuf, packet.fld[ gCFG->fldOffset+j ], *fld_def) )
			{
				success = false;
			#ifndef NDEBUG 
				if ( packet.buf.point < packet.buf.limit )
					*packet.buf.point = '\0';
			#else
				if ( packet.fld[gCFG->fldOffset+j].val) {
					WBUG("dofield failed while fld_def.no=%d, packet[%d]={no=%d, val=\"%s\", range=%ld}", \
					fld_def->no, gCFG->fldOffset+j, packet.fld[gCFG->fldOffset+j].no, \
					packet.fld[gCFG->fldOffset+j].val, \
					packet.fld[gCFG->fldOffset+j].range);
				}
			#endif
				break;
			}
			pri_fldDef = fld_def;	/* ��������ϳɳɹ������� */
		}

		/* ������λͼָʾ��, ����Ҫ�����������; ��λͼ��, �����ж���������ֵ */
		if ( !success || (!hasMap && j<= pac_def->max) )
		{	/* ���ĺϳ�ʧ�� */
			if ( j >  max_last ) 	/* ��¼һ��ƥ����õ���� */
			{
				max_last = j;
				perhap_seg = i;
				last_fldDef = pri_fldDef;
			}

			for ( j = 0, fld_def = pac_def->fld; j <= pac_def->max; j++, fld_def++)
			{	/* ȡ��λͼ */
				if ( fld_def->no >= 0 && ( fld_def->locator.type == ISOBITMAP || 
				fld_def->locator.type == YLBITMAP ) )
					packet.fld[gCFG->fldOffset + j ].no = -1;
			}

			packet.buf.point = packet.buf.base + oldOff; /* buffer���ݻָ���ԭ���ĵ� */
			WBUG("Compose failed in seg# %d", i);
		} else /* ���ĺϳɳɹ� */
			break;
	}

	if ( i == gCFG->segNum )	/* �������ж����, ���޷��ϳ����ݰ� */
	{
		WLOG(WARNING, "Can not compose the packet, perhaps seg %d, last mathced field definition %d", perhap_seg, last_fldDef ? last_fldDef->no : -1);
		deliver(Notitia::ERR_UNIPAC_COMPOSE, gCFG->inversed);
		ret =false;
	}  else
	{
		WBUG("Compose the packet successfully in seg# %d!", i);
	}

	if ( !totb && ret)/* �����totb, ����ζ���������ϳɰ�����, �������ݲ���packet��, ������� */
	{ 		/* �����м�ϳɳɹ�, ���ɵ�������tbuf��, pacWhat��¼���������� */
			FieldObj *f = & (packet.fld[gCFG->pacWhat]);
			TBuffer &pbuf = packet.buf;
			/* ��¼ƫ����, �µ����ݽ�����֮�� */
			unsigned int o = packet.buf.point - packet.buf.base;

			old_base = packet.buf.base;
			f->no = -1;
			TBuffer::pour(pbuf, *tbuf);	/* ����ʱ��tbuf���ݵ��뵽packet��, pacWhatָ�������packet.buf�� */
			if ( old_base != tbuf->base )	/* packet.buf�ռ䷢���˱仯, ����Ҫ���� */
				packet.adjust(old_base);

			/* ��������� */
			f->val = pbuf.base + o;
			f->range = pbuf.point - f->val;
			f->no = gCFG->pacWhat;
			f->raw = 0;
			f->len = 0;
			f->_rlen = 0;
	}
	if ( totb ) do_pac.reset();	/* �ϳ����, ���۳ɹ����, ���packetobj��Ҳ��Ҫ�� */
	return ret;
}

PACINLINE void Unipac::dobitmap(PacketObj &packet, FldDef &fld_def)
{
	int start,end, i;
	unsigned int maplen ,current ;
	unsigned char map[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	bool hasValue = false;

	current = fld_def.no + gCFG->fldOffset;
	start = current +  (fld_def.no == 0 ? 1 : 64)  ;
	end = start + ( fld_def.locator.type == YLBITMAP ? 128 : 64)-1;
	if ( end > packet.max )
		end = packet.max;

	for ( i = start; i <= end; i++ )
	{
		int index = i - gCFG->fldOffset;
		if (packet.fld[i].no >= 0 )
		{
			index = index %64;
			if ( index == 0 ) index = 64;
			map[(index-1)/8] |= (0x80 >> (index+7)%8 );
			hasValue = true;
		}
	}
	
	if ( fld_def.locator.type == YLBITMAP )
	{	/* �ж��Ƿ���129��192�� */
		for ( i = 7; i < 16; i++)
		{
			if ( map[i] != 0 ) 
			{	/* ��129��192�� */
				map[0] |= 0x80;	
				break;
			}
		}
		if ( i == 16 ) /* û�к�64�� */
			maplen = 8;
		else
			maplen = 16;
	} else if (!hasValue ) {
		return;
	} else 
		maplen = 8;

	packet.input(current, map, maplen);
	return;
}

PACINLINE bool Unipac::dofield(TBuffer &buf, FieldObj &field, FldDef &fld_def)
{
	unsigned int yaLen = 0;
	unsigned int midLen = 0;
	unsigned char* midPtr;
	unsigned int postLen = 0;
	unsigned char* postPtr = 0;
	
	struct Asn1Type *asn_dat = 0;
	struct AjpHeadAttrType *ajp_dat = 0;

	LenPara *lpra = (LenPara *) fld_def.locator.para;
	TermPara *tpra = (TermPara *) fld_def.locator.para;
	AjpHeadPara *jpra = (AjpHeadPara *) fld_def.locator.para;
	
	long nlen;
	short head_num;	/* AJP head */
	int nbase = 0;
	unsigned char zero = 0x0;

	bool hasLpra;
	if ( field.raw )
	{	/* ָ��ԭʼ���ݵķ�ʽ, ������ƥ���鶼�Թ��� */
		midPtr = field.raw;
		midLen = field._rlen;
		goto LASTINPUT;
	}
	if ( !field.other )
		goto NORM_LEN_PRO;

	/* ����Ǹ�����������, ���ﴦ�� */
	switch ( fld_def.locator.type )
	{
	case ASN1_BER:
		if ( field.other->type != PacketObj::ASN1  )
			return false;

		asn_dat =  (struct Asn1Type *)field.other->value;
		if ( asn_dat->len > lpra->length)
			return false;

		yaBuf[0] = asn_dat->kind ;
	
		if (asn_dat->len <= 0x7F) 
		{
			yaBuf[1] = asn_dat->len & 0xff;
			yaLen = 2;
		} else if (asn_dat->len <= 0xff) {

			yaBuf[1] = 0x81;
			yaBuf[2] = (unsigned char)(asn_dat->len &0xff);
			yaLen = 3;

		} else if (asn_dat->len <= 0xffff) {

			yaBuf[1] = 0x82;
			yaBuf[2] = (unsigned char)( ( asn_dat->len >> 8 ) & 0xff );
			yaBuf[3] = (unsigned char)( asn_dat->len & 0xff );
			yaLen = 4;
		} else 
			return false;

		buf.input(yaBuf, yaLen);
		buf.input(asn_dat->val, asn_dat->len);
		break;

	case AJP_HEAD:
		if ( field.other->type != PacketObj::AJP_HEAD_ATTR )
			return false;
		ajp_dat = (struct AjpHeadAttrType*)field.other->value;
		head_num = 0;	/* �ȼ��� */
		while ( ajp_dat ) 
		{
			head_num ++;
			ajp_dat = ajp_dat->next;
		}
		if ( head_num > jpra->head_num) 
			return false;
		yaBuf[1] = head_num & 0xff;
		yaBuf[0] = (head_num >> 8) & 0xff;
		buf.input(yaBuf, 2);
		ajp_dat = (struct AjpHeadAttrType*)field.other->value;
		while ( ajp_dat ) 
		{
			if ( ajp_dat->sc_name == PacketObj::INVALID_SC_NAME)	/* �����Ч, ��ȡ���� */
			{
				yaBuf[1] = ajp_dat->nm_len & 0xff;
				yaBuf[0] = (ajp_dat->nm_len >> 8) & 0xff;
				buf.input(yaBuf, 2);
				buf.input((unsigned char*)ajp_dat->name, ajp_dat->nm_len);
			} else {
				yaBuf[1] = ajp_dat->sc_name & 0xff;
				yaBuf[0] = (ajp_dat->sc_name >> 8) & 0xff;
				buf.input(yaBuf, 2);
			}
			yaBuf[1] = ajp_dat->str_len & 0xff;
			yaBuf[0] = (ajp_dat->str_len >> 8) & 0xff;
			buf.input(yaBuf, 2);
			buf.input((unsigned char*)ajp_dat->string, ajp_dat->str_len);
			ajp_dat = ajp_dat->next;
		}
		break;

	case AJP_ATTRIBUTE:
		if ( field.other->type != PacketObj::AJP_HEAD_ATTR )
			return false;
		ajp_dat = (struct AjpHeadAttrType*)field.other->value;
		while ( ajp_dat ) 
		{
			if ( ajp_dat->sc_name == 0xa0)	/* ȡ���� */
			{
				yaBuf[0] = 0xa0;
				yaBuf[2] = ajp_dat->nm_len & 0xff;
				yaBuf[1] = (ajp_dat->nm_len >> 8) & 0xff;
				buf.input(yaBuf, 3);
				buf.input((unsigned char*)ajp_dat->name, ajp_dat->nm_len);
			} else {
				yaBuf[0] = ajp_dat->sc_name & 0xff;
				buf.input(yaBuf, 1);
			}
			yaBuf[1] = ajp_dat->str_len & 0xff;
			yaBuf[0] = (ajp_dat->str_len >> 8) & 0xff;
			buf.input(yaBuf, 2);
			buf.input((unsigned char*)ajp_dat->string, ajp_dat->str_len);
			ajp_dat = ajp_dat->next;
		}
		yaBuf[0] = 0xff;
		buf.input(yaBuf, 1);
		break;
	default:
		break;
	}

	/* �����������ͽ���, ����*/
	goto END_PRO;

NORM_LEN_PRO:	/* ���������� */
	/* ƥ���� */
	if ( !domatch(field, fld_def))
		return false;	
	else if ( fld_def.m_only )	/* �������, ����� */
		return true;

	midPtr = field.val;
	midLen = field.range;

	hasLpra = tpra && fld_def.locator.type != LTERM ;
	if ( field.len < 0 )
	{
		nlen = field.range;
		if ( hasLpra )	/* ���峤�Ȼ��� */
		switch ( lpra->unit)
		{
		case NIBBLE:
			nlen *=2;
			break;

		case UNI_BYTE:
			break;

		case UNI_WORD:
			nlen /=2;
			field.range =2*nlen;
			break;

		case UNI_DWORD:
			nlen /=4;
			field.range =4*nlen;
			break;

		case ARRAY_ELE:
			nlen /=lpra->element_sz;
			field.range = nlen*lpra->element_sz ;
			break;

		default:
			break;
		}
	} else
		nlen = field.len;

	if ( hasLpra)
	{	/* ���Ȳ��ܳ��� */
		if ( lpra->length  >= 0 && nlen > lpra->length)
			return false;
		nbase = lpra->base;
		if ( nbase == 0 )
			nbase = 10;
	}

	switch ( fld_def.locator.type )
	{
	case RIGID:
		if ( nlen != lpra->length)
			return false;
		break;
	case ISOBITMAP:
	case YLBITMAP:
		break;

	case LVAR:	
		yaLen = 1;
		if ( nlen > 9 ) 
			return false;
		yaBuf[0] =(unsigned char)nlen;	
		break;

	case LLVAR:	
		yaLen = 1;
		if ( nlen > 99 ) 
			return false;
		yaBuf[0] = (unsigned char) ((nlen%10) | (nlen/10) << 4);	
		break;

	case LLLVAR:	
		yaLen = 2;
		if ( nlen > 999 ) 
			return false;
		yaBuf[1] = (unsigned char) ((nlen%10) | ((nlen%100)/10) << 4 );
		yaBuf[0] = (unsigned char) (nlen/100);	
		break;

	case L4VAR:	
		yaLen = 2;
		if ( nlen > 9999 ) 
			return false;
		yaBuf[1] = (unsigned char) ((nlen%10) | ((nlen%100)/10) << 4);
		nlen /= 100;
		yaBuf[0] = (unsigned char) ((nlen%10) | (nlen/10) << 4);	
		break;

	case ALVAR:	
		yaLen = 1;
		switch (nbase)
		{
		case 10:
		case 8:
		case 2:
			if ( nlen > nbase)
				return false;
			yaBuf[0] = (unsigned char)('0' + nlen);
			break;

		case 16:
			if ( nlen > 16) 
				return false;
			yaBuf[0] =  (unsigned char)Obtainx(nlen);
			break;

		default:
			return false;
		}
		
		break;

	case ALLVAR:	
		yaLen = 2;
		switch (nbase)
		{
		case 2:
			if ( nlen > 3) 
				return false;
		case 8:
			if ( nlen > 63) 
				return false;
		case 10:
			if ( nlen > 99) 
				return false;

			yaBuf[1] = (unsigned char)('0' + nlen%nbase);
			nlen /= nbase;
			yaBuf[0] = (unsigned char)('0' + nlen);
			break;

		case 16:
			if ( nlen > 255) 
				return false;
				
			yaBuf[1] = (unsigned char)Obtainx(nlen%16);
			nlen /= 16;
			yaBuf[0] = (unsigned char)Obtainx(nlen);
			break;
		default:
			return false;
		}
/* ���ǲ��ÿո�
		if ( yaBuf[0] == '0' ) 
			yaBuf[0] = ' ';	
*/
		break;

	case ALLLVAR:	
		yaLen = 3;
		switch (nbase)
		{
		case 2:
			if ( nlen > 7) 
				return false;
		case 8:
			if ( nlen > 511) 
				return false;
		case 10:
			if ( nlen > 999) 
				return false;
			yaBuf[2] = (unsigned char)('0' + nlen%nbase);
			nlen /= nbase;
			yaBuf[1] = (unsigned char)('0' + nlen%nbase);
			nlen /= nbase;
			yaBuf[0] = (unsigned char)('0' + nlen);
			break;

		case 16:
			if ( nlen > 4095) 
				return false;
			yaBuf[2] =  (unsigned char)Obtainx(nlen%16);
			nlen /=16;
			yaBuf[1] =  (unsigned char)Obtainx(nlen%16);
			nlen /=16;
			yaBuf[0] =  (unsigned char)Obtainx(nlen);
			break;
		default:
			return false;
		}
/* ���ǲ��ÿո�
		if ( yaBuf[0] == '0' ) 
		{
			yaBuf[0] = ' ';	
			if ( yaBuf[1] == '0' ) 
				yaBuf[1] = ' ';	
		}
*/
		break;

	case AL4VAR:	
		yaLen = 4;
		switch (nbase)
		{
		case 2:
			if ( nlen > 15) 
				return false;
		case 8:
			if ( nlen > 4095) 
				return false;
		case 10:
			if ( nlen > 9999) 
				return false;
			yaBuf[3] = (unsigned char)('0' + nlen%nbase);
			nlen /= nbase;
			yaBuf[2] = (unsigned char)('0' + nlen%nbase);
			nlen /= nbase;
			yaBuf[1] = (unsigned char)('0' + nlen%nbase);
			nlen /= nbase;
			yaBuf[0] = (unsigned char)('0' + nlen);
			break;

		case 16:
			if ( nlen > 65535) 
				return false;

			yaBuf[3] =  (unsigned char)Obtainx(nlen%16);
			nlen /= 16;
			yaBuf[2] =  (unsigned char)Obtainx(nlen%16);
			nlen /= 16;
			yaBuf[1] =  (unsigned char)Obtainx(nlen%16);
			nlen /= 16;
			yaBuf[0] =  (unsigned char)Obtainx(nlen);
			break;
		default:
			return false;
		}
/* ���ǲ��ÿո�
		if ( yaBuf[0] == '0' ) 
		{
			yaBuf[0] = ' ';	
			if ( yaBuf[1] == '0' ) 
			{
				yaBuf[1] = ' ';	
				if ( yaBuf[2] == '0' ) 
					yaBuf[2] = ' ';	
			}
		}
*/
		break;

	case HLVAR:	
		yaLen = 1;
		if ( nlen > 255) 
			return false;
		yaBuf[0] =(unsigned char)nlen;
		break;

	case HLLVAR:	
		yaLen = 2;
		if ( nlen > 65535) 
			return false;
		yaBuf[1] = (unsigned char)(nlen%256);
		nlen /=256;
		yaBuf[0] =(unsigned char)nlen;
		break;

	case HLLLVAR:	
		yaLen = 3;
		if ( nlen > 16777215 ) 
			return false;
		yaBuf[2] = (unsigned char)(nlen%256);
		nlen /=256;
		yaBuf[1] = (unsigned char)(nlen%256);
		nlen /=256;
		yaBuf[0] =(unsigned char)nlen;
		break;

	case HL4VAR:	       
		yaLen = 4;
		yaBuf[3] = (unsigned char)(nlen%256);
		nlen /=256;
		yaBuf[2] = (unsigned char)(nlen%256);
		nlen /=256;
		yaBuf[1] = (unsigned char)(nlen%256);
		nlen /=256;
		yaBuf[0] =(unsigned char)nlen;
		break;

	case BLVAR:	
		yaLen = 1;
		if ( nlen > 255) 
			return false;
		yaBuf[0] =(unsigned char)nlen;
		break;

	case AJP_STRING:	
		yaLen = 2;
		if ( nlen == 0 ) 
		{
			yaBuf[0] = 0xff;
			yaBuf[1] = 0xff;
			postLen = 0;
			postPtr= 0;
			midPtr = 0;
			midLen = 0;
		} else {
			if ( nlen > 65535) 
				return false;
			yaBuf[1] = (unsigned char)(nlen & 0xff);
			yaBuf[0] =(unsigned char)((nlen>>8) & 0xff);
			postLen = 1;
			postPtr= &zero;
		}
		break;

	case BLLVAR:	
		yaLen = 2;
		if ( nlen > 65535) 
			return false;
		yaBuf[0] = (unsigned char)(nlen%256);
		nlen /=256;
		yaBuf[1] =(unsigned char)nlen;
		break;

	case BLLLVAR:	
		yaLen = 3;
		if ( nlen > 16777215 ) 
			return false;
		yaBuf[0] = (unsigned char)(nlen%256);
		nlen /=256;
		yaBuf[1] = (unsigned char)(nlen%256);
		nlen /=256;
		yaBuf[2] =(unsigned char)nlen;
		break;

	case BL4VAR:	       
		yaLen = 4;
		yaBuf[0] = (unsigned char)(nlen%256);
		nlen /=256;
		yaBuf[1] = (unsigned char)(nlen%256);
		nlen /=256;
		yaBuf[2] = (unsigned char)(nlen%256);
		nlen /=256;
		yaBuf[3] =(unsigned char)nlen;
		break;

	case LTERM:
		lpra = 0; 
		if ( tpra->num > 0 )
		{
			postPtr = tpra->term[0];
			postLen = tpra->len[0];
		}
		break;

	case WANTON:
		break;

	default:
		return false;
	}

LASTINPUT:
	if ( yaLen > 0 )
		buf.input(yaBuf, yaLen);
	
	if ( midLen > 0 )
		buf.input(midPtr, midLen);

	if ( postLen > 0 )
		buf.input(postPtr, postLen);

END_PRO:
	return true;
}

PACINLINE bool Unipac::domatch(FieldObj &field, FldDef &fld_def)
{
	unsigned int i;
	FldDef::Match *scan;
	bool matched;

	if ( fld_def.m_num == 0 )	/* û������ */
		return true;

	matched = false;
	for ( i = 0 , scan = &fld_def.match[0]; 
		i < fld_def.m_num; 
		i++, scan++ )
	{
		switch ( scan->type)
		{
		case VAR_ANY:
			matched = (field.no >= 0 ); /* ���û�д���, ��ƥ��ʧ�� */
			WBUG("VAR_ANY field.no=%d, fld_def.no=%d, matched=%d", field.no, fld_def.no, matched);
			break;

		case CONSTANT:
			if ( scan->len == field.range 
				&& memcmp(field.val, scan->val, scan->len) == 0 )
				matched = true;
#ifndef NDEBUG
			{
			char fmsg[1024], smsg[1024];
			char tmp[64];
			int mlen,i;

			if ( 333 > field.range ) 
				mlen = field.range ;
			else 
				mlen = 333;
			for ( i = 0 ; i < mlen ; i++)
			{
				TEXTUS_SPRINTF(tmp, "%02x ", field.val[i]);
				memcpy(&fmsg[i*3], tmp, 3);
			}
			fmsg[i*3] = 0;

			if ( 333 > scan->len ) 
				mlen = scan->len;
			else 
				mlen = 333;
			for ( i = 0 ; i < mlen ; i++)
			{
				TEXTUS_SPRINTF(tmp, "%02x ", scan->val[i]);
				memcpy(&smsg[i*3], tmp, 3);
			}
			smsg[i*3] = 0;
			
			WBUG("CONSTANT field.no=%d, field.val(%ld)=%s, scan.val(%u)=%s, matched=%d", field.no, field.range, fmsg, scan->len, smsg, matched);
			}
#endif 
			break;

		case BEGIN_W:
			if ( scan->len <= field.range 
				&& memcmp(field.val, scan->val, scan->len) == 0 )
				matched = true;
			break;

		case END_W:
			if ( scan->len <= field.range 
				&& memcmp(&field.val[field.range - scan->len], scan->val, scan->len) == 0 )
				matched = true;
			break;

		default:
			break;
		}

		if ( scan->NOT )
			matched = !matched;
		if ( matched ) 
			break;
	}
	
	return matched;
}
#include "hook.c"
