/* Copyright (c) 2016-2018 by Ju Haibo (octerboy@gmail.com)
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
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include <stdarg.h>
#include "TBuffer.h"
#include "BTool.h"
#include "casecmp.h"
#include "textus_string.h"
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include <assert.h>
#define TOKEN_SIZE 32
class ToWay;
	struct TokenList {
		char token[TOKEN_SIZE];
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
				if ( strcmp(obj->token, mtoken) == 0 )
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
	} ;
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


	ToWay *to_reader;
	ToWay *from_way;

	TBuffer *rcv_buf;	/* 在http头完成后，这将是http体的内容 */
	TBuffer *snd_buf;
	char reader_token[TOKEN_SIZE];

private:
	Amor::Pius local_pius;
	struct TokenList *aone;

	struct G_CFG {
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
	inline void ins_cross(TBuffer *ans);
	inline void way_down();
	void reader_to_home( ToWay *rdr);

	#include "wlog.h"
};

static struct TokenList *pools = 0;
void ToWay::ignite(TiXmlElement *prop)
{
	int i;
	if (!prop) return;
	if ( !pools ) 
		pools = new struct TokenList;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
		gCFG->prop(prop);
		if ( gCFG->work_mode == ToReader )
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
	char msg[32];
	struct TokenList *found;
	TBuffer **tb;

	TiXmlElement *peer = 0;
	Amor::Pius g_peer;
	TiXmlElement *cfg;

	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::CMD_SET_PEER:
		if ( gCFG->work_mode == FromWay )	/* 指令服务器要求 */
		{
			cfg = (TiXmlElement *)(pius->indic);
			if( !cfg)
			{
				WLOG(WARNING, "CMD_SET_PEER cfg is null");
				break;
			}
			TEXTUS_SPRINTF(msg, "%s:%s", cfg->Attribute("ip"),  cfg->Attribute("port"));
			WLOG(INFO, "facio CMD_SET_PEER, %s", msg);
			if ( to_reader ) {
				if (strcmp(to_reader->reader_token, msg) == 0 ) {	//已有, 相同, 可直接用
					break;
				} else {	//回收	
					reader_to_home( to_reader);
				}
			}

			found = pools->fetch(msg);
			//{int *a =0 ; *a = 0; };	
			if ( found )
			{
				WBUG("CMD_SET_PEER found %s", msg);
				to_reader = found->reader ;
				if ( to_reader ) 
				{
					to_reader->from_way = this;
					/* 建立了通路, found就用不着了 */
					found->reader = 0;
					found->control = 0;
					gCFG->idle.put(found);
				}
			} else {
				to_reader = (ToWay *) 0;
			}
		}
		break;

	case Notitia::WebSock_Start:	//在这里, indic是一个子协议名, 原来insway中的终端 tcp port。
		WBUG("facio WeBSock_Start %s", pius->indic == 0 ? "null": (const char*)pius->indic);
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

		g_peer.ordo = Notitia::CMD_GET_PEER;
		g_peer.subor = 0;
		g_peer.indic = 0;
		aptus->sponte(&g_peer);
		peer = (TiXmlElement *) g_peer.indic;
		if ( peer )
		{
			TEXTUS_SPRINTF(aone->token, "%s:%s", peer->Attribute("cliip"),  pius->indic == 0 ? "null":(const char*)pius->indic );
			WBUG("get token %s", aone->token);
		} else {
			WLOG(ERR,"get_peer return null");
			break;
		}
		pools->put(aone);	//放进pools，等着指令服务器连接时从中找出来, 按token
		aone->reader = this; //这就是reader了
		aone->control = this;	//如果还在pools，insway没有来连接, 而这个reader断了, 就以此从pools中取出来, 还到idle中。
		memcpy(this->reader_token, aone->token, TOKEN_SIZE);

		break;

	case Notitia::PRO_TBUF:	
		WBUG("facio PRO_TBUF");
		if ( gCFG->work_mode == ToReader )	/* HTTP体数据, 来自读卡器的响应 */
		{
			if ( from_way )
				from_way->ins_cross(rcv_buf);
			else {	
				WLOG(ERR, "This reader is not mapped to a way.");
				rcv_buf->reset();
			}
		} else if ( gCFG->work_mode == FromWay )	/* 来自指令服务器的指令 */
		{
			if ( to_reader)
				to_reader->ins_cross(rcv_buf);
			else {
				WLOG(NOTICE, "This reader is down.");
				way_down();
				return false;
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
				aone = pools->fetch(this);
				if ( !aone) 
					break;
				aone->reader = 0;
				aone->control = 0;
				gCFG->idle.put(aone);
			}
		
			if ( from_way )
			{
				from_way->way_down();
				from_way->to_reader = 0;
			}
			from_way = 0;
		} else  if ( gCFG->work_mode == FromWay )
		{
			if (to_reader )
			{
				reader_to_home( to_reader);
				//to_reader->way_down();
			}
			//to_reader = 0;
		}
		break;

	default:
		return false;
	}
	return true;
}

void ToWay::reader_to_home( ToWay *rdr)
{
	aone = gCFG->idle.fetch(); //取一个空的,
	pools->put(aone);	//放进pools，等着指令服务器连接时从中找出来, 按token
	aone->reader = rdr; //这就是reader了
	aone->control = rdr;//如果还在pools，insway没有来连接, 而这个reader断了, 就以此从pools中取出来, 还到idle中。
	memcpy(aone->token, rdr->reader_token, TOKEN_SIZE);
	rdr->from_way = 0;
}

bool ToWay::sponte( Amor::Pius *pius)
{
	return false;
}

/* 处理IC指令响应, FromWay模式下, rbuf是从ToReader那边传过来, 所以, 这里将rbuf的内容copy到snd_buf就可以了 */
/* 处理IC指令请求, ToReader模式下, rbuf是从FromWay那边传过来, 所以, 这里将rbuf的内容copy到snd_buf就可以了 */
inline void ToWay::ins_cross(TBuffer *rbuf)
{
	TBuffer::exchange(*rbuf, *snd_buf);
	local_pius.ordo = Notitia::PRO_TBUF;
	local_pius.indic = 0;
	local_pius.subor = 0;
	aptus->sponte(&local_pius);
	return ;
}
inline void ToWay::way_down()
{
	local_pius.ordo = Notitia::END_SESSION;
	local_pius.indic = 0;
	local_pius.subor = 0;
	aptus->sponte(&local_pius);
	return ;
}
#include "hook.c"
