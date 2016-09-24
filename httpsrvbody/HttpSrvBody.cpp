/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: HTTP Service Body Pro
 Build: created by octerboy, 2006/04/27, Guangzhou
 $Header: /textus/httpsrvbody/HttpSrvBody.cpp 25    08-01-10 1:12 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: HttpSrvBody.cpp $"
#define TEXTUS_MODTIME  "$Date: 08-01-10 1:12 $"
#define TEXTUS_BUILDNO  "$Revision: 25 $"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "BTool.h"
#include "casecmp.h"
#include "textus_string.h"
#include <time.h>
#include <stdarg.h>
#if defined(__APPLE__)
#define COMMON_DIGEST_FOR_OPENSSL
#include <CommonCrypto/CommonDigest.h>
#else
#include <openssl/sha.h>
#endif

#define HTTPSRVINLINE inline
#define Sock_Framing_Start	1 
#define Sock_Framing_Head	2 
#define Sock_Framing_Data	4 
#define Sock_Framing_Close	6 
#define Sock_Status_OK		1000 

#define OPCODE_CONTINUATION 0x0 
#define OPCODE_TEXT         0x1 
#define OPCODE_BINARY       0x2 
#define OPCODE_CLOSE        0x8 
#define OPCODE_PING         0x9 
#define OPCODE_PONG         0xA 

class HttpSrvBody: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	HttpSrvBody();
	~HttpSrvBody();

private:
	long content_length;
	long proxy_out_bytes;

	Amor::Pius local_pius;		//仅用于向左边传回数据
	Amor::Pius set_buf_pius;	//仅用于向右边设置socket中的BUFFER
	Amor::Pius set_pre_buf_pius;	//仅用于向右边重设BUFFER
	
	bool session;
	bool isProxy;
	bool head_outed;
	
	TBuffer *rcv_buf;	/* 在http头完成后，这将是http体的内容 */
	TBuffer *snd_buf;
	TBuffer *left_tb[3];

	bool isSocket;
	bool lastSocket;	//如果前一次为websocket, 那么本次就不要再传SET-TUBFFER了. 通常, 配置一个特别的路径给websocket, 就传一次
	bool lookSocket();
	void rcvSocket();
	void sndSocket(unsigned char msg_type, unsigned char *c, unsigned long l);
	void sndSocket(unsigned char msg_type);

	struct  SockFrame 
	{
		unsigned char fin;
		unsigned char opcode;

		long int start_pos;
		long payload_length;
		int ext_len_head;
		int mask_bit;
		unsigned char mask[4];
		unsigned char data[128];
	};

	struct Sock_Pro_Def{
		const char *name;		//WebSocket协议的名称
		int sub;				//WebSocket各协议所对应的sub_ordo
	};

	struct G_CFG
	{
		long max_sock_len;

		int sock_num;	//WebSocket协议的数目
		struct Sock_Pro_Def *sock_pro_def;

		inline G_CFG() {
			max_sock_len = 8192;
			sock_num = 0;
			sock_pro_def = 0;
		};

		inline ~G_CFG() { 
			if ( sock_pro_def) delete[] sock_pro_def;
			sock_num = 0;
			sock_pro_def = 0;
		};

		inline void prop(TiXmlElement *cfg)
		{
			TiXmlElement *var_ele;
			const char *vn="WebSocket";
			int i;

			sock_num = 0;
			for (var_ele = cfg->FirstChildElement(vn); var_ele; var_ele = var_ele->NextSiblingElement(vn)) 
				sock_num++;
			if (sock_num == 0) return ;

			sock_pro_def =  new struct Sock_Pro_Def[sock_num];

			for (i = 0, var_ele = cfg->FirstChildElement(vn); var_ele; var_ele = var_ele->NextSiblingElement(vn),i++) 
			{
				sock_pro_def[i].name = var_ele->Attribute("name");
				var_ele->QueryIntAttribute("sub_ordo", &(sock_pro_def[i].sub));
			}
		};
	};

	struct G_CFG *gCFG;  
	bool has_config;

	typedef struct _Websock {
		int version;		/* websocket 版本, 目前仅支持13*/
		TBuffer buf_1st;	/* for websocket, recv from client */
		TBuffer buf_2nd;	/* for websocket, send to client */
		TBuffer *hitb[3];	/* for websocket, send to client */

		int framing;
		unsigned long should_len;	
		int pos;

		unsigned char opcode;
		struct SockFrame frm;
		unsigned short stat_code;
		bool continued;

		inline _Websock ()
		{
			hitb[0] = &buf_1st;
			hitb[1] = &buf_1st;
			hitb[2] = 0;
			reset();
		};

		inline void reset ()
		{
			framing = -1;
			pos = 0;
			should_len = 2;
			continued = false;	/* 假定不需要后续帧 */
			buf_1st.reset();
			buf_2nd.reset();
		};

		inline void continue_frame()
		{
			framing = Sock_Framing_Start;	//等着后续帧
			should_len +=2;
		};

		inline void neo_frame()
		{
			should_len = 2;
			pos = 0;
			continued = false;	/* 假定不需要后续帧 */
			framing = Sock_Framing_Start;	//等着新的开始
			stat_code = Sock_Status_OK;
			opcode = OPCODE_BINARY;
		}
	} Websock;
	Websock sock;
	int cur_sub_ordo;	//本实例ordo子类型，在TBUFFER中设定

	typedef struct _Chunko {
					/* 很多情况下, 一个chunk一次读完, 所以都处于初始值 */
		bool started ;		/* false: 还没有读完chunk头, true: chunk数据正在读 */
		long body_len;		/* 正在读chunk的长度, len_to_read 不包括CRLF这两个字 */
		long head_len;		/* head的长度， 包括CRLF */
		inline _Chunko ()
		{
			reset();
		};

		inline void reset ()
		{
			started = false;
			body_len = -1;
		};
	} Chunko;
	Chunko chunko;

	long chunk_offset;	/* 当前分析位置, 即rcv_buf中相对于base的偏移量 */
#include "get_chunk_size.c"
	bool chunk_all();

	HTTPSRVINLINE void end();
	HTTPSRVINLINE void reset();
	HTTPSRVINLINE void outjs(const char* );
	HTTPSRVINLINE void deliver(Notitia::HERE_ORDO aordo);

#include "httpsrv_obj.h"
#include "wlog.h"
};

#include <assert.h>

void HttpSrvBody::ignite(TiXmlElement *cfg)
{
	const char *iscopy_str = cfg->Attribute("proxy");
	if ( iscopy_str && strcasecmp(iscopy_str,"yes") == 0)
		isProxy = true;

	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
		gCFG->prop(cfg);
	}	
}

bool HttpSrvBody::facio( Amor::Pius *pius)
{
	int mth;
	TBuffer **tb = 0;
	switch ( pius->ordo )
	{
	case Notitia::PRO_HTTP_HEAD:	/* HTTP头已经OK */
		WBUG("facio PRO_HTTP_HEAD");
		reset();
		chunko.reset();
		session = true;

		mth =  getHeadInt("method");
		content_length = getContentSize();
		if ( mth ==  2 ) //GET
			content_length = 0;

		if ( content_length != 0  && isProxy)	//从头就可知HTTP请求已经完整了, 就不需要再发HEAD消息了, 而是REQUEST
			deliver(Notitia::PRO_HTTP_HEAD);	

		if ( content_length == 0 )
			goto BODYOK;

		if ( content_length < -1 )
		{	/* look for websocket  */
			if ( !lookSocket() )
			{
				deliver(Notitia::PRO_HTTP_HEAD); //可能是什么古怪协议,发个HEAD消息，让后续模块处理。
			 } else {
				WBUG("WebSocket begin....");
			}
		}

		if ( rcv_buf->point > rcv_buf->base ) 
			goto BODYPRO; /* 已经有body数据, 转向下一步 */
		break; 

	case Notitia::PRO_TBUF:	/* HTTP体数据 */
		WBUG("facio PRO_TBUF");
		if (!session ) break;
BODYPRO:	
 		if ( content_length < -1 ) 	/* 不能确定消息体长度, 实际上是: 没有Content-Length头, 也不是Encoding */
		{
			if ( isSocket )
			{
				rcvSocket();
			} else {
				deliver(Notitia::HTTP_ASKING);		//什么古怪
			}
			break;
		}

		if ( content_length == - 1 )
		{ 	/* Transfer-Encoding, 对于这个编码传输, 只根据解码来判断消息体是否完成 */
			if ( chunk_all() )
				goto BODYOK;
		} else { /* 在这里, 属于正常的 */
			if ( isProxy )	/* 对于proxy, 数据每次提交后就清空了, 这里依靠计数来判断是否请求结束 */
			{
				proxy_out_bytes += (rcv_buf->point - rcv_buf->base);
				if ( content_length <= proxy_out_bytes )
					goto BODYOK;
			} else {	/* 数据仍旧保留在本缓冲区 */
				if ( content_length > 0 && content_length <= (rcv_buf->point - rcv_buf->base) )
					goto BODYOK;
			}
		}

		if ( isProxy )
			deliver(Notitia::HTTP_ASKING);	
		break;
BODYOK:
		local_pius.ordo = Notitia::HTTP_Request_Complete;
		local_pius.indic = &content_length;
		aptus->sponte(&local_pius);
		deliver(Notitia::PRO_HTTP_REQUEST); /* 整个HTTP请求已经OK */
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
			left_tb[0] = rcv_buf;	/* 把先前的保存下来 */
			left_tb[1] = snd_buf;
			left_tb[2] = 0;
			aptus->facio(pius);	/* 续传BUFFER */
		} else 
			WLOG(WARNING, "facio SET_TBUF null");
		break;

	case Notitia::DMD_END_SESSION:	/* 强制关闭 */
		WBUG("facio DMD_END_SESSION");
		end();
		break;

	default:
		return false;
	}

	return true;
}

bool HttpSrvBody::sponte( Amor::Pius *pius)
{
	long len;
	assert(pius);
	struct SetResponseCmd *res_cmd = 0;
	
	switch ( pius->ordo )
	{
	case Notitia::CMD_HTTP_SET :	/* 置HTTP响应数据 */
		if ( !snd_buf)	
			WLOG(WARNING, "snd_buf is null!");
		if (!session ) break;
		res_cmd = (struct SetResponseCmd *)pius->indic;
		assert(res_cmd);
		switch ( res_cmd->fun)
		{
		case SetResponseCmd::SendError :
			aptus->sponte(pius);	/* 转向httpsrvhead,以后想办法扩展 */
			break;
					
		case SetResponseCmd::OutPutLen:
			len = res_cmd->len;
			goto REAL_OUTPUT;

		case SetResponseCmd::OutPut:
			len = strlen(res_cmd->valStr);
REAL_OUTPUT:		snd_buf->input((unsigned char*)res_cmd->valStr, len);
			WBUG("sponte CMD_HTTP_SET OutPut(\"%s\")", res_cmd->valStr);
			break;

		case SetResponseCmd::OutPutJS:
			this->outjs(res_cmd->valStr);
			WBUG("sponte CMD_HTTP_SET OutPutJS(\"%s\")", res_cmd->valStr);
			break;

		default:
			aptus->sponte(pius);	/* Not supported.  Httpsrvhead do it */
			break;
		}
		break;

	case Notitia::PRO_HTTP_HEAD :	/* 响应的HTTP头已经准备好 */
		WBUG("sponte PRO_HTTP_HEAD");
		if (!session ) break;
		head_outed = true;	/* 置一下标志, 表示已经HEAD处理, 在处理RESPONSE时不再处理 */
		aptus->sponte(pius);	/* 转向httpsrvhead */
		break;

	case Notitia::PRO_WEBSock_HEAD :	/* 响应的WebSocket头已经准备好 */
		WBUG("sponte PRO_WEBSock_HEAD");
		if (!session ) break;
		head_outed = true;	/* 置一下标志, 表示已经HEAD处理, */
		local_pius.ordo = Notitia::PRO_HTTP_HEAD;
		aptus->sponte(&local_pius);	/* 转向httpsrvhead */
		break;

	case Notitia::PRO_HTTP_RESPONSE:	/* HTTP响应已备 */
		WBUG("sponte PRO_HTTP_RESPONSE");
		if (!session ) break;
		if ( !head_outed )	/* 未处理HEAD，这里处理HEAD */
		{
			head_outed = true;
			local_pius.ordo = Notitia::PRO_HTTP_HEAD;
			aptus->sponte(&local_pius);
		}

		local_pius.ordo = Notitia::PRO_TBUF;
		aptus->sponte(&local_pius);
		reset();
		break;

	case Notitia::PRO_TBUF:	
		WBUG("sponte PRO_TBUF");
		if ( isSocket )
		{
			sndSocket(sock.opcode);
		} else {
			aptus->sponte(pius);	/* 转向httpsrvhead */
		}
		break;

	case Notitia::DMD_END_SESSION:	/* 强制关闭 */
		end();
		break;
		
	default:
		return false;
	}
	return true;
}

HttpSrvBody::HttpSrvBody()
{
	left_tb[0] = 0;
	left_tb[1] = 0;
	left_tb[2] = 0;
	rcv_buf = 0;
	snd_buf = 0;
	local_pius.indic = 0;
	local_pius.subor = 0;
	isProxy = false;
	set_buf_pius.indic = &sock.hitb[0];
	set_buf_pius.ordo = Notitia::SET_TBUF;
	set_buf_pius.subor = 0;

	set_pre_buf_pius.indic = &left_tb[0];
	set_pre_buf_pius.ordo = Notitia::SET_TBUF;
	set_pre_buf_pius.subor = 0;
	gCFG = 0;

	reset();
}

void HttpSrvBody::reset()
{
	session = false;
	head_outed = false;
	content_length = -2;
	chunk_offset = 0;
	proxy_out_bytes = 0;

	isSocket = false;
	lastSocket = false;
	cur_sub_ordo = 0;
	sock.reset();
}

HttpSrvBody::~HttpSrvBody() 
{
	if ( has_config  )
	{	
		if(gCFG) delete gCFG;
	}
}

Amor* HttpSrvBody::clone()
{
	HttpSrvBody *child = new HttpSrvBody();
	child->gCFG = gCFG;
	child->isProxy = isProxy;
	return (Amor*)child;
}

HTTPSRVINLINE void HttpSrvBody::end()
{
	if (session )
	{
		reset();
	}
}

HTTPSRVINLINE void HttpSrvBody::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	tmp_pius.ordo = aordo;
	WBUG("deliver Notitia::%d", aordo);
	if ( aordo == Notitia::PRO_TBUF ) 
		tmp_pius.subor = cur_sub_ordo;
	else
		tmp_pius.subor = 0;
	aptus->facio(&tmp_pius);
	return ;
}

bool HttpSrvBody::chunk_all()
{
	unsigned char *ptr, *c_base;	/* c_base指示在一个完整chunk之后, 或就是cli_rcv.base */
	WBUG("Transfer-Encoding...");
	bool ret = false;

	c_base = (unsigned char*) 0;	/* 0: 表示刚从PRO_TBUF来,  */
HERE:
	if ( c_base == (unsigned char*) 0 )
		c_base = rcv_buf->base + chunk_offset ; 

	if ( !chunko.started )
	{	/* 这一行应该是指示chunk长度 */
		for (ptr = c_base ; ptr < rcv_buf->point - 1; ptr++ )
		{	/* 判断一行的CRLF */
			if ( *ptr == '\r' && *(ptr+1) == '\n' )
			{
				chunko.body_len = get_chunk_size(c_base);
				WBUG("A Chunk size %ld", chunko.body_len);
				chunko.head_len = (ptr - c_base) + 2;	/* 包括CRLF */
				break;
			}
		}
		chunko.started =  ( chunko.body_len >= 0 );
	}

	if ( chunko.started )
	{	/* 这里是数据了, 从ptr开始 */
		if ( chunko.body_len == 0 )
		{	/* 下面应该是footers, 扫描到CRLFCRLF为止 */
			/* 一种情况是 0CRLFCRLF
			   另一种是: 0.....CRLFCRLF 或 0....CRLF....CRLFCRLF
			   所以, ptr倒回两个字节, 把chunk头的CRLF也算进去
			*/
			for (ptr = &c_base[chunko.head_len-2] ; ptr < rcv_buf->point - 3; ptr++ )
			{	/* 判断CRLFCRLF */
				if ( *ptr == '\r' && *(ptr+1) == '\n' &&
					*(ptr+2) == '\r' && *(ptr+3) == '\n' )
				{
					/* chunk传完了*/
					WBUG("Chunk completed!");
					ptr +=4 ;		/* ptr指向后面的数据, 另一个HTTP报文 */
					memmove(c_base, ptr, rcv_buf->point - ptr);	/* 数据前移,footers数据被盖 */
					rcv_buf->point -= (ptr - c_base);	
					chunko.reset();
					ret = true;
				}
			}
				
		} else if ( chunko.body_len > 0) {
				/* 具体数据 */
			long bz = chunko.body_len + chunko.head_len +2;	/* 整个Chunk长度, 2是因为包括CRLF */
			if ( rcv_buf->point - c_base >= bz ) 
			{
				/* 一个Chunk完整了 */
				WBUG("A Chunk cut out");
				/* 只留下data */
				ptr = &(c_base[chunko.head_len]); /* 指向数据区 */
				memmove(c_base, ptr, rcv_buf->point - ptr);	/* body数据前移, head数据被盖 */
				rcv_buf->point -= chunko.head_len;	
				c_base += chunko.body_len;	/* c_base指向了body结尾的CRLF */
				ptr = c_base +2;	/* ptr指向了本chunk后面的数据 */
				memmove(c_base, ptr, rcv_buf->point - ptr);	/* 后数据前移, CRLF被挤掉 */
				rcv_buf->point -= 2;	

				chunk_offset += chunko.body_len;
				chunko.reset();

				if ( c_base < rcv_buf->point) /* 下面还有,再找下一个 */
				goto HERE;
			}
		}
	}

	/* end of Transfer-Encoding */
	return ret;
}

HTTPSRVINLINE void HttpSrvBody::outjs(const char *in)
{
	char *outPtr;
	const char *inPtr;
	char *out;
	int outLen;
	int usedLen;
	int i;
	int inlen = strlen(in);

	outLen = 2*inlen;
	out = new char [outLen+1];
	if (  out == (char*)0 )
	{
		return;
	}

	memset(out,0,outLen+1);
	usedLen = 0;

	outPtr = out;
	inPtr = in;

	for ( i = 0 ; i < inlen ; i++, inPtr++) 
	{
		if ( *inPtr > 0 && *inPtr < 32 ) 
		{
			usedLen += 4;
			TEXTUS_SNPRINTF(outPtr, outLen, "\\x%02X",(unsigned char)*inPtr);
			outPtr +=4;
		} else if (  *inPtr == '\\' ||  *inPtr == '\"')
		{
			usedLen += 2;
			outPtr[0] = '\\';
			outPtr[1] = *inPtr;
			outPtr +=2;
			
		} else if (  *inPtr == '/' )
		{
			usedLen += 4;
			TEXTUS_SNPRINTF(outPtr, outLen, "\\x%s","2F");
			outPtr +=4;

		} else if (  *inPtr == '<' )
		{
			usedLen += 4;
			TEXTUS_SNPRINTF(outPtr, outLen, "\\x%s","3C");
			outPtr +=4;

		} else if (  *inPtr == '>' )
		{
			usedLen += 4;
			TEXTUS_SNPRINTF(outPtr, outLen, "\\x%s","3E");
			outPtr +=4;
		} else {
			*outPtr = *inPtr;
			outPtr ++;
			usedLen++;
		}	 

		if ( usedLen+10 > outLen ) 
		{
			char *news;
			outLen += inlen;
			news = new char [outLen];
			memcpy ( news, out, usedLen);
			delete[] out;
			out = news;
		}
	}
	*outPtr = '\0';

	snd_buf->input((unsigned char*)out, usedLen);
	delete[] out;
	return;
}

#define WEBSOCKET_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" 
#define WEBSOCKET_GUID_LEN 36 


HTTPSRVINLINE bool HttpSrvBody::lookSocket()
{
	const char *conn, *upg, *socKey;
	const char *protocol;
	int i;
	bool has_pro;

	conn = getHead("Connection");	
	isSocket = false;	//假定开始不是socket
	has_pro= false;
	if ( conn && strcasecmp(conn, "Upgrade") == 0 )
	{
		upg = getHead("Upgrade");
		socKey = getHead("Sec-WebSocket-Key");
		if ( socKey != (const char*)0 && upg !=(const char*)0 && (strcasecmp(upg, "websocket") == 0) )
		{
			SHA_CTX c;
			unsigned char md[SHA_DIGEST_LENGTH];
			char md2[40];

			sock.version = getHeadInt("Sec-WebSocket-Version");
			if ( sock.version != 13 ) 
			{
				setStatus(426);
				addHead("Title", "Upgrade Required");
				setHead("Connection", "close");
				addHead("Sec-WebSocket-Versiont", "13");
				goto S_END;
			}
			
			if ( !lastSocket )
			{
				aptus->facio(&set_buf_pius);	//上次不是Websocket, 而这次是, 就重设缓冲区
			}

			SHA1_Init(&c);
			SHA1_Update(&c,socKey,strlen(socKey));
			SHA1_Update(&c,WEBSOCKET_GUID,WEBSOCKET_GUID_LEN);
			SHA1_Final(&(md[0]),&c);
			BTool::base64_encode(md2, md, SHA_DIGEST_LENGTH);
			sock.reset();
			sock.neo_frame();

			protocol = getHead("Sec-WebSocket-Protocol");
			if (protocol) 
			{
				for ( i =0 ; i < gCFG->sock_num; i++)
				{
					if (strcmp(protocol, gCFG->sock_pro_def[i].name) ==0 ) //这样简单不行, protocol是用逗号隔开的，要形成一个数组。
					{
						cur_sub_ordo = gCFG->sock_pro_def[i].sub;
						break;
					}
				}
				if ( i == gCFG->sock_num )	//未找到相应协议
				{
					setStatus(400);
					has_pro = false;
				} else {		//已定义已有协议
					setStatus(101);
					setHead("Title", "Switching Protocols");
					setHead("Connection", "Upgrade");
					addHead("Upgrade", "websocket");
					addHead("Sec-WebSocket-Accept", md2);
					addHead("Sec-WebSocket-Protocol", gCFG->sock_pro_def[i].name);
					has_pro = true;
				}
				/*
				sndSocket(OPCODE_TEXT, (unsigned char*)"Oway-123", 8);
				*/
			}
			isSocket = true;
S_END:
			local_pius.ordo = Notitia::PRO_HTTP_HEAD;
			aptus->sponte(&local_pius);
			if ( has_pro ) deliver(Notitia::PRO_WEBSock_HEAD);	
		}
	}

	if ( lastSocket && !isSocket )	//上次是Websocket, 而这次不是, 就把缓冲区设回来
	{
		aptus->facio(&set_pre_buf_pius);
	}
	lastSocket = isSocket;
	return isSocket;
}


#define FRAME_GET_FIN(BYTE)         (((BYTE) >> 7) & 0x01) 
#define FRAME_GET_RSV1(BYTE)        (((BYTE) >> 6) & 0x01) 
#define FRAME_GET_RSV2(BYTE)        (((BYTE) >> 5) & 0x01) 
#define FRAME_GET_RSV3(BYTE)        (((BYTE) >> 4) & 0x01) 
#define FRAME_GET_OPCODE(BYTE)      ( (BYTE)       & 0x0F) 
#define FRAME_GET_MASK(BYTE)        (((BYTE) >> 7) & 0x01) 
#define FRAME_GET_PAYLOAD_LEN(BYTE) ( (BYTE)       & 0x7F) 
#define FRAME_SET_FIN(BYTE)         (((BYTE) & 0x01) << 7) 
#define FRAME_SET_OPCODE(BYTE)       ((BYTE) & 0x0F) 
#define FRAME_SET_MASK(BYTE)        (((BYTE) & 0x01) << 7) 
#define FRAME_SET_LENGTH(X64, IDX)  (unsigned char)(((X64) >> ((IDX)*8)) & 0xFF) 

#define STATUS_CODE_GOING_AWAY        1001 
#define STATUS_CODE_PROTOCOL_ERROR    1002 
#define STATUS_CODE_RESERVED          1004 /* Protocol 8: frame too large */ 
#define STATUS_CODE_INVALID_UTF8      1007 
#define STATUS_CODE_POLICY_VIOLATION  1008 
#define STATUS_CODE_MESSAGE_TOO_LARGE 1009 
#define STATUS_CODE_INTERNAL_ERROR    1011 

HTTPSRVINLINE void HttpSrvBody::rcvSocket()
{
	unsigned char*p = rcv_buf->base;
	unsigned char *q;
	unsigned long len = rcv_buf->point - rcv_buf->base;
	long i,j;

	switch (sock.framing) 
	{ 
	case Sock_Framing_Start:
		WBUG("Sock_Framing_Start buf_len(%ld), should_len(%ld), pos(%d)", len, sock.should_len, sock.pos);
		if ( len < sock.should_len) break;
		sock.frm.fin = (p[sock.pos]) & 0x80; 
		sock.frm.start_pos = sock.pos;
		if ((FRAME_GET_RSV1(p[sock.pos]) != 0) || 
			(FRAME_GET_RSV2(p[sock.pos]) != 0) || 
			(FRAME_GET_RSV3(p[sock.pos]) != 0)) 
		{
			sock.framing = Sock_Framing_Close; 
			sock.stat_code = STATUS_CODE_PROTOCOL_ERROR;
			break; 
		} 
		sock.frm.opcode = FRAME_GET_OPCODE(p[sock.pos++]); 
		if (sock.frm.opcode >= 0x8) /* Control frame */ 
		{
			if (!sock.frm.fin) 
			{
				sock.framing = Sock_Framing_Close; 
				sock.stat_code = STATUS_CODE_PROTOCOL_ERROR; 
				break; 
			} 
		}  else { /* Message frame */ 
			if (sock.frm.opcode)  /* !continuation frame */
			{ 
				if ( !sock.continued) 	/* 如果不需要后续, 这是合法的. 也就是说前一段数据已经结束, 或刚开始 */
				{ 
					sock.opcode = sock.frm.opcode; 
					//frame->utf8_state = UTF8_VALID; 
				} else {	/* 需要后续(前一帧所要求), 而这里却重新开头(op不为0), 这不合法 */
					sock.framing = Sock_Framing_Close;
					sock.stat_code = STATUS_CODE_PROTOCOL_ERROR; 
					break; 
				} 
			} else if ( !sock.continued ) 	//这里持续帧, 却不要求持续
			{ 
				sock.framing = Sock_Framing_Close; 
				sock.stat_code = STATUS_CODE_PROTOCOL_ERROR; 
				break; 
			} 

			sock.continued = (sock.frm.fin == 0);	//要不要后续, 这里就记下来, 控制帧就不记
		}

		sock.frm.payload_length = (long)FRAME_GET_PAYLOAD_LEN(p[sock.pos]); 
		sock.frm.mask_bit = FRAME_GET_MASK(p[sock.pos++]);

		if (sock.frm.payload_length == 126) 
		{ 
			sock.frm.payload_length = 0; 
			sock.frm.ext_len_head = 2;
			sock.should_len += 2;
		} else if (sock.frm.payload_length == 127) 
		{ 
			sock.frm.payload_length = 0; 
			sock.frm.ext_len_head = 8; 
			sock.should_len += 8;
		} else {
			sock.frm.ext_len_head = 0; 
		}

		if (sock.frm.mask_bit ) 
		{
			sock.should_len += 4;
		}

		if (( !sock.frm.mask_bit ) ||   /* Client mask required */ 
			((sock.frm.opcode >= 0x8) && /* Control frame's payload larger than 125 bytes */ 
			(sock.frm.ext_len_head != 0))) 
		{ 
			sock.framing = Sock_Framing_Close; 
			sock.stat_code = STATUS_CODE_PROTOCOL_ERROR; 
			break; 
		}  else {
			sock.framing = Sock_Framing_Head; 
		}

		if ( len >= sock.should_len)
			goto Pro_FRM_HEAD;

		break;

	case Sock_Framing_Head:
		WBUG("Sock_Framing_Head buf_len(%ld), should_len(%ld)", len, sock.should_len);
		if ( len < sock.should_len) break;
Pro_FRM_HEAD:
		for ( i = 0; i < sock.frm.ext_len_head; i++)
		{
			sock.frm.payload_length <<= 8; 
			sock.frm.payload_length += p[sock.pos++];
			if ((sock.frm.payload_length < 0) || (sock.frm.payload_length > gCFG->max_sock_len)) 
			{ 
				sock.framing = Sock_Framing_Close; 
				sock.stat_code =  STATUS_CODE_MESSAGE_TOO_LARGE;
				break; 
			} else {
				sock.framing = Sock_Framing_Data; 
			}
		} 
		sock.should_len += sock.frm.payload_length;

		if (sock.frm.mask_bit )
		{
			memcpy(sock.frm.mask, &p[sock.pos], 4);
			sock.pos +=4;
		} else {
			memset(sock.frm.mask, 0, 4);
		}
		if (len >= sock.should_len )
			goto Pro_FRM_DATA;

		break;

	case Sock_Framing_Data: 
		WBUG("Sock_Framing_Data buf_len(%ld), should_len(%ld)", len, sock.should_len);
		if ( len < sock.should_len) break;
Pro_FRM_DATA:
		if (sock.frm.opcode >= 0x8) /* 当前收了一个控制帧, 放在本地 */ 
		{
			q = &sock.frm.data[0];
			for (i = 0, j=0; i < sock.frm.payload_length; i++) 
			{ 
				*q++ = p[sock.pos++] ^ sock.frm.mask[j++ & 3]; 
			}

			switch (sock.frm.opcode) 
			{ 
			case OPCODE_CLOSE: 
				WBUG("WebSocket close frame.");
				sock.framing = Sock_Framing_Close; 
				sock.stat_code = Sock_Status_OK; 
				break; 

			case OPCODE_PING: 
				/* 要做, */
				/* 只有这一帧不影响整个数据流 */
				WBUG("WebSocket ping frame.");
				sndSocket(OPCODE_PONG, sock.frm.data, sock.frm.payload_length);
				break; 

			case OPCODE_PONG: 
				WBUG("WebSocket pong frame.");
				break; 

			default: 
				sock.framing = Sock_Framing_Close; 
				sock.stat_code = STATUS_CODE_PROTOCOL_ERROR; 
				break;
			}
			if ( sock.framing == Sock_Framing_Close ) 
				goto Pay_End;

			if ( !sock.continued)		/* 如果不是在数据帧中插进来的, 就去掉控制帧在接收缓冲中的数据 
							   否则, 等着所有数据帧到齐了再清. 这样提高一点效率。
							*/
			{
				rcv_buf->commit(sock.frm.start_pos - sock.pos);
				sock.neo_frame();
			} else {
				sock.continue_frame();
			}

		} else {	
			/* 数据帧就在BUFF里 */
			sock.buf_1st.grant(sock.frm.payload_length);
			q = sock.buf_1st.point;
			for (i = 0, j=0; i < sock.frm.payload_length; i++) 
			{ 
				*q++ = p[sock.pos++] ^ sock.frm.mask[j++ & 3]; 
			}
			sock.buf_1st.commit(sock.frm.payload_length);

			/* 至此, 一帧数据已经结束 */
			if ( sock.continued)
			{	/* 如果还有后续帧 */
				sock.continue_frame();
				goto Pay_End;
			}
	
			/* 到这里, 帧已经全部结束 */
			switch (sock.opcode) 
			{ 
			case OPCODE_TEXT: 
				/* 这里要做UTF8转换, 以后再做 */
				goto DELIVER;
				break;
	
			case OPCODE_BINARY: 
				sock.stat_code = Sock_Status_OK; 
		DELIVER:
				rcv_buf->commit(-sock.pos);
				sock.neo_frame();
				deliver(Notitia::PRO_TBUF);	//数据向右传
				break; 

			default: 
				sock.framing = Sock_Framing_Close; 
				sock.stat_code = STATUS_CODE_PROTOCOL_ERROR; 
				break; 
			} 
		}

	Pay_End:
		break; 

	case Sock_Framing_Close: 
		WBUG("Sock_Framing_Close");
		break; 

	default: 
		sock.framing = Sock_Framing_Close; 
		sock.stat_code = STATUS_CODE_PROTOCOL_ERROR; 
		break; 
	} 

	/* 在这里要看一下是否要关闭链接 */
	if ( sock.framing == Sock_Framing_Close )
	{
		Amor::Pius tmp_pius;
		unsigned char msg[128];
		msg[0] = (sock.stat_code >> 8) & 0xFF ;
		msg[1] = sock.stat_code & 0xFF;
		msg[2] = 0;

		switch (sock.stat_code)
		{
		case STATUS_CODE_PROTOCOL_ERROR:
			TEXTUS_SNPRINTF((char*)&msg[2], sizeof(msg)-3, "websocket protocol error");
			break;

		case STATUS_CODE_MESSAGE_TOO_LARGE:
			TEXTUS_SNPRINTF((char*)&msg[2], sizeof(msg)-3, "websocket message too large");
			break;

		default:
			TEXTUS_SNPRINTF((char*)&msg[2], sizeof(msg)-3, "websocket closed");
			break;
		}
		sock.reset();
		sndSocket(OPCODE_CLOSE, msg, (unsigned long)(2 + strlen((const char*)&msg[2])));
		WBUG("will terminate websocket");
		tmp_pius.ordo = Notitia::END_SESSION;
		tmp_pius.indic = 0;
		tmp_pius.subor = 0;
		aptus->sponte(&tmp_pius);
	}
}

HTTPSRVINLINE void HttpSrvBody::sndSocket(unsigned char op_code, unsigned char *msg_data, unsigned long msg_length)
{
	unsigned char*p = snd_buf->point;
	snd_buf->grant(msg_length +16 );
	
	*p++ = 0x80 | (op_code & 0x0F);	//fin is  1, last frame 
	if (msg_length < 126) 
	{ 
		*p++ = msg_length & 0x0F;	//mask is 0, no mask
	} else { 
		if (msg_length < 65536) 
		{ 
			*p++ = 126; 		//mask is 0, no mask
		} else { 
			*p++ = 127; 		////mask is 0, no mask
#if defined(__LP64__) || defined(_M_X64) || defined(__amd64__)
			*p++ = (msg_length >> (7*8))  & 0xFF; 
			*p++ = (msg_length >> (6*8))  & 0xFF;
			*p++ = (msg_length >> (5*8))  & 0xFF;
			*p++ = (msg_length >> (4*8))  & 0xFF;
#endif
			*p++ = (msg_length >> (3*8))  & 0xFF;
			*p++ = (msg_length >> (2*8))  & 0xFF;
		} 

		*p++ = (msg_length >> 8)  & 0xFF;
		*p++ = msg_length & 0xFF;
	} 
	snd_buf->commit((unsigned long)(p - snd_buf->point));
	snd_buf->input(msg_data, (unsigned long)msg_length);

	/* 最后, 提交数据 */
	local_pius.ordo = Notitia::PRO_TBUF;
	aptus->sponte(&local_pius);
}

HTTPSRVINLINE void HttpSrvBody::sndSocket(unsigned char op_code)
{
/* 数据都在sock.buf_2nd中 */
	unsigned char*p = sock.buf_2nd.point;
	unsigned long len = sock.buf_2nd.point - sock.buf_2nd.base;
	
	sndSocket(op_code, p, len);

	/* 最后, 提交数据 */
	local_pius.ordo = Notitia::PRO_TBUF;
	aptus->sponte(&local_pius);
}

#include "hook.c"
