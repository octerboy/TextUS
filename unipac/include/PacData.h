/* Copyright (c) 2005-2018 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "TBuffer.h"   
#include <string.h>
   struct ComplexType {
	char type;
	unsigned char *value;
   };

   struct AjpHeadAttrType {
	unsigned short sc_name;
	unsigned short nm_len;
	char	*name;
	unsigned short str_len;
	char *string;
	struct AjpHeadAttrType *next;
   };

   struct Asn1Type {
	char kind;
	int len;
	unsigned char *val;
   };

   struct PacketObj;
   struct FieldObj {
 	int no;			/* 域号, < 0 :无此数据, >=0 : 有数据, 与数组下标相同 */
  	unsigned char *raw;	/* 报文解析后, 指向原始数据buffer中的位置, 包括前缀、后缀 */
  	unsigned long _rlen;	/* raw所指的范围的字节数 */
  	unsigned char *val;	/* 报文解析后, 指向原始数据buffer中的位置, 不包括前缀、后缀 */
  	unsigned long range;	/* val所指的范围的字节数 */
  	long len;		/* 名义(定义)长度,  -1:此值无效 */
	ComplexType *other;		/* 其它种种情况 */
	/* 报文合成的规则:
		1：如果存在raw, 则取之, 不理会定义, 这为产生'非法报文'提供可能
		2: 如果存在len, 则将之用于变长类型, 或是RIGID的类型判断
		3: 一般是取val, 变长值根据range计算(对于半字节等, 对于双字节则可能会去掉最后一字节)
		4: 对于RIGID, 要求长度相同, 对于变长, 长度不能超限
		5: 仍然尝试多个定义的报文合成, 其条件除了长度的限制, 还有匹配规则的限制
	*/
	inline FieldObj() { 
		reset();
	};

	inline void reset() { 
		no = -1;
		raw = val = 0;
		_rlen =  range = 0;
		len = -1;
		other=0;
	};
   };

   struct PacketObj {
 	int max;	/* 最大下标 */
  	FieldObj  *fld;
	TBuffer buf;	/* 所域的内容缓冲 */
	enum ComplexKind {	/* 其他类型 */
		AJP_HEAD_ATTR	=0,	/* AJP中head 或 attribute */
		ASN1 	=1,
		INVALID_SC_NAME = 0xffff
	};

	void exchange( struct PacketObj *ano )
	{
		int mid;
		FieldObj  *f_mid;
		if ( !ano ) return;
		mid = ano->max;
		ano->max = this->max;
		this->max = mid;
		f_mid = ano->fld;
		ano->fld = this->fld;
		this->fld = f_mid;
		TBuffer::exchange(buf, ano->buf);
	};

	inline unsigned char *alloc_buf(long len)
	{
		unsigned char *p = buf.point;
		buf.commit(len);
		return p;
	};

#if defined(__LP64__) || defined(_M_X64) || defined(__amd64__)
#define M_SZ 8
#define M_SZ_MASK 7
#define NOT_M_SZ_MASK 0xfffffffffffffff8
#else
#define M_SZ 4
#define M_SZ_MASK 3
#define NOT_M_SZ_MASK 0xfffffffc
#endif
	inline unsigned char *alloc_align_buf(long len)
	{
		unsigned char *q,*p;
		size_t offset;
		p = buf.point;
		offset = reinterpret_cast<size_t>(p) & (M_SZ_MASK) ;
		if ( offset != 0)
		{
			q = (unsigned char *) ((size_t(p+ M_SZ_MASK)) & NOT_M_SZ_MASK );
			buf.commit(len + (M_SZ - offset));
			p = q;
		} 
		return p;
	};

	inline void produce( int _maxium) {
		if ( max < 0 && _maxium >=0 )
		{
			max = _maxium ;
			fld = new FieldObj[max+1];
		}
	};

	inline void reset() {
		int i;
		for ( i = 0 ; i  <= max; i++)
			fld[i].reset();
		buf.reset();
	};

	inline PacketObj():buf(128) { 
		max = -1;
		fld = 0;
	};

	inline ~PacketObj() { 
		if ( fld ) 
			delete[] fld;
	};

	inline void adjust( unsigned char *old_base) { 
		int i;
		for (i = 0 ; i <= max ; i++)
		{
			FieldObj *f = &fld[i];
			if ( f->no >= 0 )
			{
				if ( f->raw )
					f->raw = f->raw -old_base + buf.base;
				if ( f->val )
					f->val = f->val -old_base + buf.base;
				if ( f->other)
				{
					struct AjpHeadAttrType *head;
					struct Asn1Type *asn;

					f->other += (buf.base-old_base);
					f->other->value +=(buf.base-old_base);
					switch ( f->other->type )
					{
						case AJP_HEAD_ATTR:
							head = (struct AjpHeadAttrType *)f->other->value;
							while ( head ) 
							{
								if ( head->name )
									head->name +=(buf.base-old_base);
								if ( head->string)
									head->string +=(buf.base-old_base);
								if ( head->next)
									head->next +=(buf.base-old_base);
								head = head->next; /* 找到最末一个head */
							}

							break;
						case ASN1:
							asn = (struct Asn1Type *)f->other->value;
							if ( asn->val ) 
								asn +=(buf.base-old_base);
							break;
						default:
							break;
					}
				}
			}
		}
	};

	inline void grant( unsigned long n) { 
		if ( buf.point +n > buf.limit )
		{
			unsigned char *o = buf.base;
			buf.grant(n);
			adjust(o);
		}
	};

	inline void commit( int no, unsigned long len) { 
		if ( no < 0 ||  no > max ) return;
		fld[no].no = no;
		fld[no].val = buf.point;
		fld[no].range = len;
		buf.commit(len);
	};

	inline  unsigned char *getfld(int no, unsigned long *lenp) { 
		if ( no < 0 ||  no > max ) 
			return 0x0;
		if ( fld[no].no == no  )
		{	
			if (lenp)  
			{
				*lenp =   fld[no].range;
			}
			if ( fld[no].other)
				return (unsigned char*)fld[no].other;
			else
				return fld[no].val;
		}
		return 0x0;
	};

	inline  unsigned char *getfld(int no) { 
		if ( no < 0 ||  no > max ) 
			return 0x0;
		if ( fld[no].no == no  )
		{
			if ( fld[no].other)
				return (unsigned char*)fld[no].other;
			else
				return fld[no].val;
		}
		return 0x0;
	};

	inline void input( int no, unsigned char *val, unsigned long len) { 
		if ( no > max || no < 0) return;
		if ( !val ) return;
		if ( fld[no].no == no && fld[no].val && fld[no].range >= len )
		{	/* 此域已经存在, 空间足够, 原值覆盖即可 */
			fld[no].range = len;
			memcpy(fld[no].val, val, len);
		} else {
			/* 空间不够, 或者本没有 */
			fld[no].no = no;
			grant(len);
			fld[no].val = buf.point;
			fld[no].range = len;
			memcpy(buf.point, val, len);
			buf.commit(len);
		}
	};

	inline void input( int no, unsigned char val) { 
		if ( no > max || no < 0) return;
		if ( fld[no].no == no && fld[no].val && fld[no].range >= 1 )
		{	/* 此域已经存在, 空间足够, 原值覆盖即可 */
			fld[no].range = 1;
			*(fld[no].val) = val;
		} else {
			/* 空间不够, 或者本没有 */
			fld[no].no = no;
			grant(1);
			fld[no].val = buf.point;
			fld[no].range = 1;
			*(fld[no].val) = val;
			buf.commit(1);
		}
	};

	inline struct AjpHeadAttrType * alloc_ajp_head( int no, short nm_len, short val_len) { 
		FieldObj  *f;
		struct AjpHeadAttrType *head;
		ComplexType *complex;

		if ( no > max || no < 0) return 0;
		f = &fld[no];
		grant(nm_len+val_len+sizeof(struct ComplexType)+sizeof(struct AjpHeadAttrType));
		if ( !f->other ) /* 从buf中取空间 */
		{
			complex = (struct ComplexType *)alloc_align_buf(sizeof(struct ComplexType));
			f->other = complex ;
			complex->type = AJP_HEAD_ATTR;

			complex->value = alloc_align_buf(sizeof(struct AjpHeadAttrType));
			head = (struct AjpHeadAttrType *)complex->value; /* head 就是全新的了 */
		} else {
			complex = f->other;
			head = (struct AjpHeadAttrType *)complex->value;
			while ( head->next ) 
				head = head->next; /* 找到最末一个head */
			head->next = (struct AjpHeadAttrType *)alloc_align_buf(0);
			buf.commit(sizeof(struct AjpHeadAttrType));
			head = head->next;		/* head 就是全新的了 */
		}
		head->next = 0;
		return head;
	}

	inline char* inputAJP_string( unsigned short val_len, const char *val_str, unsigned short *neo_len) 
	{ 
		char *ret_str ;
		*neo_len = val_len+1;
		grant(val_len+1);

		ret_str = (char*)buf.point;
		buf.commit(val_len+1);		
		memcpy(ret_str, val_str, val_len);
		ret_str[val_len] = 0x0;
		return ret_str;
	}

	inline void inputAJP( int no, unsigned short sc_name, unsigned short val_len, const char *val_str) 
	{ 
		struct AjpHeadAttrType *head;
		if ( !val_str ) return;
		head = alloc_ajp_head(no, 0, val_len);
		if (!head) return;

		head->sc_name = sc_name; 
		head->nm_len = 0;
		head->name = 0;

		head->string = inputAJP_string(val_len, val_str, &head->str_len);
	};


	inline void inputAJP( int no, unsigned short nm_len, const char *nm_str, unsigned short val_len, const char *val_str) 
	{ 
		struct AjpHeadAttrType *head;
		if (  !nm_str || !val_str ) return;
		head = alloc_ajp_head(no, nm_len, val_len);
		if (!head) return;

		head->sc_name = INVALID_SC_NAME; /* 表示sc_name无效 */
		head->name = inputAJP_string(nm_len, nm_str, &head->nm_len);
		head->string = inputAJP_string(val_len, val_str, &head->str_len);
	}          

	/* 对于?req_attribute 0x0A Name (the name of the attribut follows) */
	inline void inputAJP( int no, unsigned short sc_name, unsigned short nm_len, const char *nm_str, unsigned short val_len, const char *val_str) 
	{
		struct AjpHeadAttrType *head;
		if (  !nm_str || !val_str ) return;
		head = alloc_ajp_head(no, nm_len, val_len);
		if (!head) return;

		head->sc_name = sc_name; 
		head->name = inputAJP_string(nm_len, nm_str, &head->nm_len);
		head->string = inputAJP_string(val_len, val_str, &head->str_len);
	}

	inline void inputASN1( int no, char kind, int val_len, unsigned char *val_str) { 
		struct Asn1Type *asn;
		FieldObj  *f;
		ComplexType *complex;

		if (  !val_str ) return;
		if ( no > max || no < 0) return ;
		f = &fld[no];
		grant(val_len+sizeof(struct ComplexType)+sizeof(struct Asn1Type)+sizeof(void*)*2);
		if ( !f->other ) /* 从buf中取空间 */
		{
			complex = (struct ComplexType *)alloc_align_buf(sizeof(struct ComplexType));
			f->other = complex ;
			complex->type = ASN1;

			complex->value = alloc_align_buf(sizeof(struct Asn1Type));
			asn = (struct Asn1Type *)complex->value; /* asn 就是全新的了 */
		} else {
			complex = f->other;
			asn = (struct Asn1Type *)complex->value; 
		}

		asn->kind = kind;
		asn->len = val_len;
		asn->val = alloc_buf(val_len);
		memcpy(asn->val, val_str, val_len);
	}          


	inline void input( int no, const char *val, int len) { 
		input(no, (unsigned char*)val, (unsigned long)len);
	};
/*
	inline void input( int no, const char *val, unsigned long len) { 
		input(no, (unsigned char*)val, len);
	};
	inline void input( int no, char val[], unsigned long len) { 
		input(no, (unsigned char*)(&val[0]), len);
	};
*/
	inline  unsigned char *getfld(int no, int *lenp) { 
		unsigned long ulen=0;
		unsigned char *p;
		p =  getfld(no, &ulen);
		*lenp = (int)ulen;
		return p;
	};
   };
