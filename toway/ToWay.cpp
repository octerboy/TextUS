/* Copyright (c) 2016-2018 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title:route instruction from insway
 Build: created by octerboy, 2016/04/09, Panyu
 $Header: /textus/toway/ToWay.cpp     14-03-10 8:13 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: ToWay.cpp $"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "md5.h"
#include <stdarg.h>
#include "TBuffer.h"
#include "BTool.h"
#include "casecmp.h"
#include "textus_string.h"
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>

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
//	hex[2*i] = '\0';
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

#define TOKEN_LEN 21
class ToWay: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	ToWay();
	~ToWay();

	enum Work_Mode {NoWhere = 0, ToReader = 1, FromWay = 2};
	struct TokenList {
		char token[32];
		ToWay *reader;	//指向ToReader
		ToWay *control;	//控制者, 最初为0, WebSock_Start时设置. 如果还在pools，没有连到指令服务器, 终端中断就以此为据从队列中取出.
		struct TokenList *prev, *next;

		inline TokenList () {
			prev = 0;
			next = 0;
			reader = 0;
			
			memset(token, 0, sizeof(token));
		};

		inline void put ( struct TokenList *neo ) 
		{
			if( !neo ) return;
			neo->next = next;
			neo->prev = this;
			if ( next != 0 )
				next->prev = neo;
			next = neo;
		};

		inline struct TokenList * fetch(const char* mtoken) 
		{
			struct TokenList *obj = 0;
	
			for ( obj = next; obj; obj = obj->next )
			{
				if ( memcmp(obj->token, mtoken, TOKEN_LEN) == 0 )
					break;
			}
			if ( !obj ) return 0;	/* 没有一个有这样的 */
			/* 至此, 一个obj, 该符合条件, 这个obj要去掉  */
			obj->prev->next = obj->next; 
			if ( obj->next )
				obj->next->prev  =  obj->prev;
			return obj;
		};

		inline struct TokenList * fetch(ToWay* me) 
		{
			struct TokenList *obj = 0;
	
			for ( obj = next; obj; obj = obj->next )
			{
				if ( control == me )
					break;
			}
			if ( !obj ) return 0;	/* 没有一个有这样的 */
			/* 至此, 一个obj, 该符合条件, 这个obj要去掉  */
			obj->prev->next = obj->next; 
			if ( obj->next )
				obj->next->prev  =  obj->prev;
			return obj;
		};

		inline struct TokenList *fetch() 
		{
			struct TokenList *obj = 0;
	
			obj = next;

			if ( !obj ) return 0;	/* 没有一个有这样的 */
			/* 至此, 一个obj, 该符合条件, 这个obj要去掉  */
			obj->prev->next = obj->next; 
			if ( obj->next )
				obj->next->prev  =  obj->prev;
			return obj;
		};
	} *aone;


	ToWay *to_reader;
	ToWay *from_way;

	TBuffer *rcv_buf;	/* 在http头完成后，这将是http体的内容 */
	TBuffer *snd_buf;

private:
	Amor::Pius local_pius;

	struct G_CFG {
		struct TokenList pools;
		struct TokenList idle;
		long token_num ;

		enum Work_Mode work_mode;
		struct TokenList *token_db;
		int token_many;

		inline G_CFG() {
			work_mode = NoWhere;
			token_db = 0;
			token_many = 0;
		};

		inline void prop(TiXmlElement *cfg)
		{
			const char *wk_str;
			wk_str = cfg->Attribute("mode");
			if ( strcasecmp(wk_str, "reader") == 0)
			{
				work_mode = ToReader; 
			} else if ( strcasecmp(wk_str, "way") == 0)
			{
				work_mode = FromWay;
			} 

			token_many = 1000;
			cfg->QueryIntAttribute("token_max", &(token_many));
			token_db = new struct TokenList[token_many];
		};
		

		inline ~G_CFG() {
			if ( token_db )
				delete[] token_db;
		};
	};
	struct G_CFG *gCFG;     /* 全局共享参数 */
	bool has_config;
	inline void ins_ans(TBuffer *ans);
	inline void ins_req(TBuffer *req);
	inline void way_down();

	#include "wlog.h"
	#include "httpsrv_obj.h"
};


#include <assert.h>

void ToWay::ignite(TiXmlElement *prop)
{
	int i;
	if (!prop) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
		gCFG->prop(prop);
		for ( i = 0; i < gCFG->token_many; i++)
		{
			gCFG->idle.put(&(gCFG->token_db[i]));
		}
	}
}

ToWay::ToWay()
{
	rcv_buf = 0;
	snd_buf = 0;

	to_reader = 0;
	from_way = 0;

	local_pius.ordo = Notitia::PRO_TBUF;
	local_pius.indic = 0;

	gCFG = 0;
	has_config = false;
	aone = 0;
}

ToWay::~ToWay() 
{
	if ( has_config  )
	{	
		if(gCFG) delete gCFG;
	}
}

Amor* ToWay::clone()
{
	ToWay *child = new ToWay();
	child->gCFG = gCFG;
	return (Amor*) child;
}

bool ToWay::facio( Amor::Pius *pius)
{
	MD5_CTX Md5Ctx;
	unsigned char md[32];
	char msg[32];
	const char *qry;
	const char *protocol;
	struct TokenList *found;
	char *p;
	TBuffer **tb;

#if defined(_WIN32) && (_MSC_VER < 1400 )
	struct _timeb now;
#else
	struct timeb now;
#endif
	assert(pius);

	switch ( pius->ordo )
	{
#ifdef NOOOOOO
	case Notitia::PRO_HTTP_REQUEST:
		WBUG("facio PRO_HTTP_REQUEST");
		if ( gCFG->work_mode != ToReader )
		{
			WLOG(ERR,"Not ToReader for PRO_HTTP_REQUEST");
			break;
		}
		qry = getQuery();
		if ( !qry || strcmp(qry,"authcode") != 0 )
		{
			output(" ", 1);
			goto AUTH_BACK_END2;
		}
		aone = gCFG->idle.fetch(); //在ignite时，idle就预置了充分的数量
		if ( !aone )
		{
			output(" ", 1);
			goto AUTH_BACK_END2;
		}

		if ( gCFG->token_num == 0 ) 
		{
		#if defined(_WIN32) && (_MSC_VER < 1400 )
			_ftime(&now);
		#else
			ftime(&now);
		#endif
			gCFG->token_num = now.time & 0xFFFF;
		} else {
			gCFG->token_num++;
		}
		TEXTUS_SPRINTF(msg, "%ld", gCFG->token_num);
	
		MD5Init (&Md5Ctx);
		MD5Update (&Md5Ctx, msg, strlen(msg));
		MD5Update (&Md5Ctx, "9289Cd1L+!#", 11);
		MD5Final ((char*)&md[0], &Md5Ctx); 
		memcpy(aone->token, "iway-", 5);
		byte2hex(md, 8, &(aone->token[5]));
		aone->token[TOKEN_LEN] = 0;
		gCFG->pools.put(aone);
		aone->reader = 0;
		aone->control = this;
		output(aone->token, TOKEN_LEN);

AUTH_BACK_END2:
		local_pius.ordo = Notitia::PRO_HTTP_HEAD;
		aptus->sponte(&local_pius);
		break;
#endif

	case Notitia::WebSock_Start:
		WBUG("facio WeBSock_Start");
		if ( gCFG->work_mode != ToReader )
		{
			WLOG(ERR,"Not ToReader for PRO_WEBSock_HEAD");
			break;
		}
		aone = gCFG->idle.fetch(); //在ignite时，idle就预置了充分的数量
		if ( !aone )
		{
			way_down();
			break;
		}

		if ( gCFG->token_num == 0 ) 
		{
		#if defined(_WIN32) && (_MSC_VER < 1400 )
			_ftime(&now);
		#else
			ftime(&now);
		#endif
			gCFG->token_num = now.time & 0xFFFF;
		} else {
			gCFG->token_num++;
		}
		TEXTUS_SPRINTF(msg, "%ld", gCFG->token_num);
	
		MD5Init (&Md5Ctx);
		MD5Update (&Md5Ctx, msg, strlen(msg));
		MD5Update (&Md5Ctx, "9289Cd1L+!#", 11);
		MD5Final ((char*)&md[0], &Md5Ctx); 
		memcpy(aone->token, "iway-", 5);
		byte2hex(md, 8, &(aone->token[5]));
		aone->token[TOKEN_LEN] = 0;
		gCFG->pools.put(aone);	//放进pools，等着指令服务器连接时从中找出来, 按token
		aone->reader = this; //这就是reader了
		aone->control = this;	//如果还在pools，insway没有来连接, 而这个reader断了, 就以此从pools中取出来, 还到idle中。

#ifdef NOOOOOO	//这一段以前的，留纪念
		protocol = getHead("Sec-WebSocket-Protocol");
		if (protocol && memcmp(protocol, "iway-", 5) == 0 && strlen(protocol) == TOKEN_LEN ) 
		{
			struct TokenList *tk;
			tk = pools.fetch(protocol);
			if ( tk ) 
			{
				setHead("Sec-WebSocket-Protocol", tk->token);
				tk->reader = this;	//才是真正的reader
				tk->control = this;
				pools.put(tk);	//再入队, 等insway的调用
			} else  {
				setHead("Sec-WebSocket-Protocol", "noway");
			}
			local_pius.ordo = Notitia::PRO_HTTP_HEAD;
			aptus->sponte(&local_pius);
		} 
#endif
		snd_buf->input((unsigned char*)&(aone->token[0]), TOKEN_LEN);
		local_pius.ordo = Notitia::PRO_TBUF;
		local_pius.indic = 0;
		aptus->sponte(&local_pius);
		break;

	case Notitia::PRO_TBUF:	
		WBUG("facio PRO_TBUF");
		if ( gCFG->work_mode == ToReader )	/* HTTP体数据, 来自读卡器的响应 */
		{
			if ( from_way )
				from_way->ins_ans(rcv_buf);
			else {	/* 指示srvbody应该关闭*/
				WLOG(ERR, "This reader is not mapped to a way.");
				rcv_buf->reset();
			}
		} else if ( gCFG->work_mode == FromWay )	/* 来自指令服务器的指令 */
		{
			char msg[64];
			p = (char*)rcv_buf->base;
			if ( p[0] == 'T' )
			{
				/* 寻找相应的reader,这里就固定了这种格式协议  */
				p++;
				if ( (rcv_buf->point - rcv_buf->base - 1) ==  TOKEN_LEN )
				{
					found = gCFG->pools.fetch(p);
					if ( found )
					{
						to_reader = found->reader ;
						if ( to_reader ) 
						{
							to_reader->from_way = this;
							/* 建立了通路, found就用不着了 */
							found->reader = 0;
							found->control = 0;
							gCFG->idle.put(found);
							msg[1] = '0';
							sprintf(&msg[2],"%s\n", "OK");
						} else {
							msg[1] = 'A';
							sprintf(&msg[2], "%s\n", "Reader is still zero");
							WLOG(ERR, "%s", "Reader is still zero");
						}
					} else {
						msg[1] = 'B';
						sprintf(&msg[2], "%s\n", "Not found the token");
						p[TOKEN_LEN-2] = 0;
						WLOG(ERR, "Not found the token (%s)", p);
					}
				} else {
					msg[1] = 'C';
					sprintf(&msg[2], "%s\n", "The token length is invalid");
					WLOG(ERR, "The token length is invalid(%ld)", (rcv_buf->point - rcv_buf->base - 1));
				}

				msg[0] = 't';
				snd_buf->input((unsigned char*)&msg[0],strlen(msg));
				local_pius.ordo = Notitia::PRO_TBUF;
				local_pius.indic = 0;
				aptus->sponte(&local_pius);
			} else {
				if ( to_reader)
					to_reader->ins_req(rcv_buf);
			}
		}
		break;

	case Notitia::SET_TBUF:	/* 取得输入TBuffer地址 */
		WBUG("facio SET_TBUF");
		if ( (tb = (TBuffer **)(pius->indic)))
		{	//tb应当不为NULL，*tb是rcv_buf
			if ( *tb) rcv_buf = *tb; 
			else
				WLOG(WARNING, "facio SET_TBUF rcv_buf null");
			tb++;
			if ( *tb) snd_buf = *tb;
			else
				WLOG(WARNING, "facio SET_TBUF snd_buf null");
		} else 
			WLOG(WARNING, "facio SET_TBUF null");
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
		break;

	case Notitia::DMD_END_SESSION:  /* 强制关闭 */
		WBUG("facio DMD_END_SESSION");
		if ( gCFG->work_mode == ToReader )
		{
			while ( true)
			{
				aone = gCFG->pools.fetch(this);
				if ( !aone) 
					break;
				aone->reader = 0;
				aone->control = 0;
				gCFG->idle.put(aone);
			}
		
			if ( from_way )
				from_way->to_reader = 0;
			from_way = 0;
		} else  if ( gCFG->work_mode == FromWay )
		{
			if (to_reader )
			{
				to_reader->from_way = 0;
				to_reader->way_down();
			}
			to_reader = 0;
		}
		break;

	default:
		return false;
	}
	return true;
}

bool ToWay::sponte( Amor::Pius *pius)
{
	return false;
}

/* 处理IC指令响应, FromWay模式下, ans_buf是从ToReader那边传过来, 所以, 这里将ans_buf的内容copy到snd_buf就可以了 */
inline void ToWay::ins_ans(TBuffer *ans_buf)
{
	TBuffer::exchange(*ans_buf, *snd_buf);
	local_pius.ordo = Notitia::PRO_TBUF;
	local_pius.indic = 0;
	aptus->sponte(&local_pius);
	return ;
}

/* 处理IC指令请求, ToReader模式下, req_buf是从FromWay那边传过来, 所以, 这里将req_buf的内容copy到snd_buf就可以了 */
inline void ToWay::ins_req(TBuffer *req_buf)
{
	TBuffer::exchange(*req_buf, *snd_buf);
	local_pius.ordo = Notitia::PRO_TBUF;
	local_pius.indic = 0;
	aptus->sponte(&local_pius);
	return ;
}
inline void ToWay::way_down()
{
	local_pius.ordo = Notitia::END_SESSION;
	local_pius.indic = 0;
	aptus->sponte(&local_pius);
	return ;
}
#include "hook.c"
