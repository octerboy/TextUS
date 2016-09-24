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

	Amor::Pius local_pius;		//����������ߴ�������
	Amor::Pius set_buf_pius;	//���������ұ�����socket�е�BUFFER
	Amor::Pius set_pre_buf_pius;	//���������ұ�����BUFFER
	
	bool session;
	bool isProxy;
	bool head_outed;
	
	TBuffer *rcv_buf;	/* ��httpͷ��ɺ��⽫��http������� */
	TBuffer *snd_buf;
	TBuffer *left_tb[3];

	bool isSocket;
	bool lastSocket;	//���ǰһ��Ϊwebsocket, ��ô���ξͲ�Ҫ�ٴ�SET-TUBFFER��. ͨ��, ����һ���ر��·����websocket, �ʹ�һ��
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
		const char *name;		//WebSocketЭ�������
		int sub;				//WebSocket��Э������Ӧ��sub_ordo
	};

	struct G_CFG
	{
		long max_sock_len;

		int sock_num;	//WebSocketЭ�����Ŀ
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
		int version;		/* websocket �汾, Ŀǰ��֧��13*/
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
			continued = false;	/* �ٶ�����Ҫ����֡ */
			buf_1st.reset();
			buf_2nd.reset();
		};

		inline void continue_frame()
		{
			framing = Sock_Framing_Start;	//���ź���֡
			should_len +=2;
		};

		inline void neo_frame()
		{
			should_len = 2;
			pos = 0;
			continued = false;	/* �ٶ�����Ҫ����֡ */
			framing = Sock_Framing_Start;	//�����µĿ�ʼ
			stat_code = Sock_Status_OK;
			opcode = OPCODE_BINARY;
		}
	} Websock;
	Websock sock;
	int cur_sub_ordo;	//��ʵ��ordo�����ͣ���TBUFFER���趨

	typedef struct _Chunko {
					/* �ܶ������, һ��chunkһ�ζ���, ���Զ����ڳ�ʼֵ */
		bool started ;		/* false: ��û�ж���chunkͷ, true: chunk�������ڶ� */
		long body_len;		/* ���ڶ�chunk�ĳ���, len_to_read ������CRLF�������� */
		long head_len;		/* head�ĳ��ȣ� ����CRLF */
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

	long chunk_offset;	/* ��ǰ����λ��, ��rcv_buf�������base��ƫ���� */
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
	case Notitia::PRO_HTTP_HEAD:	/* HTTPͷ�Ѿ�OK */
		WBUG("facio PRO_HTTP_HEAD");
		reset();
		chunko.reset();
		session = true;

		mth =  getHeadInt("method");
		content_length = getContentSize();
		if ( mth ==  2 ) //GET
			content_length = 0;

		if ( content_length != 0  && isProxy)	//��ͷ�Ϳ�֪HTTP�����Ѿ�������, �Ͳ���Ҫ�ٷ�HEAD��Ϣ��, ����REQUEST
			deliver(Notitia::PRO_HTTP_HEAD);	

		if ( content_length == 0 )
			goto BODYOK;

		if ( content_length < -1 )
		{	/* look for websocket  */
			if ( !lookSocket() )
			{
				deliver(Notitia::PRO_HTTP_HEAD); //������ʲô�Ź�Э��,����HEAD��Ϣ���ú���ģ�鴦��
			 } else {
				WBUG("WebSocket begin....");
			}
		}

		if ( rcv_buf->point > rcv_buf->base ) 
			goto BODYPRO; /* �Ѿ���body����, ת����һ�� */
		break; 

	case Notitia::PRO_TBUF:	/* HTTP������ */
		WBUG("facio PRO_TBUF");
		if (!session ) break;
BODYPRO:	
 		if ( content_length < -1 ) 	/* ����ȷ����Ϣ�峤��, ʵ������: û��Content-Lengthͷ, Ҳ����Encoding */
		{
			if ( isSocket )
			{
				rcvSocket();
			} else {
				deliver(Notitia::HTTP_ASKING);		//ʲô�Ź�
			}
			break;
		}

		if ( content_length == - 1 )
		{ 	/* Transfer-Encoding, ����������봫��, ֻ���ݽ������ж���Ϣ���Ƿ���� */
			if ( chunk_all() )
				goto BODYOK;
		} else { /* ������, ���������� */
			if ( isProxy )	/* ����proxy, ����ÿ���ύ��������, ���������������ж��Ƿ�������� */
			{
				proxy_out_bytes += (rcv_buf->point - rcv_buf->base);
				if ( content_length <= proxy_out_bytes )
					goto BODYOK;
			} else {	/* �����Ծɱ����ڱ������� */
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
		deliver(Notitia::PRO_HTTP_REQUEST); /* ����HTTP�����Ѿ�OK */
		break;

	case Notitia::SET_TBUF:	/* ȡ������TBuffer��ַ */
		WBUG("facio SET_TBUF");
		if ( (tb = (TBuffer **)(pius->indic)))
		{	//tbӦ����ΪNULL��*tb��rcv_buf
			if ( *tb) rcv_buf = *tb; 
			else
				WLOG(WARNING, "facio SET_TBUF rcv_buf null");
			tb++;
			if ( *tb) snd_buf = *tb;
			else
				WLOG(WARNING, "facio SET_TBUF snd_buf null");
			left_tb[0] = rcv_buf;	/* ����ǰ�ı������� */
			left_tb[1] = snd_buf;
			left_tb[2] = 0;
			aptus->facio(pius);	/* ����BUFFER */
		} else 
			WLOG(WARNING, "facio SET_TBUF null");
		break;

	case Notitia::DMD_END_SESSION:	/* ǿ�ƹر� */
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
	case Notitia::CMD_HTTP_SET :	/* ��HTTP��Ӧ���� */
		if ( !snd_buf)	
			WLOG(WARNING, "snd_buf is null!");
		if (!session ) break;
		res_cmd = (struct SetResponseCmd *)pius->indic;
		assert(res_cmd);
		switch ( res_cmd->fun)
		{
		case SetResponseCmd::SendError :
			aptus->sponte(pius);	/* ת��httpsrvhead,�Ժ���취��չ */
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

	case Notitia::PRO_HTTP_HEAD :	/* ��Ӧ��HTTPͷ�Ѿ�׼���� */
		WBUG("sponte PRO_HTTP_HEAD");
		if (!session ) break;
		head_outed = true;	/* ��һ�±�־, ��ʾ�Ѿ�HEAD����, �ڴ���RESPONSEʱ���ٴ��� */
		aptus->sponte(pius);	/* ת��httpsrvhead */
		break;

	case Notitia::PRO_WEBSock_HEAD :	/* ��Ӧ��WebSocketͷ�Ѿ�׼���� */
		WBUG("sponte PRO_WEBSock_HEAD");
		if (!session ) break;
		head_outed = true;	/* ��һ�±�־, ��ʾ�Ѿ�HEAD����, */
		local_pius.ordo = Notitia::PRO_HTTP_HEAD;
		aptus->sponte(&local_pius);	/* ת��httpsrvhead */
		break;

	case Notitia::PRO_HTTP_RESPONSE:	/* HTTP��Ӧ�ѱ� */
		WBUG("sponte PRO_HTTP_RESPONSE");
		if (!session ) break;
		if ( !head_outed )	/* δ����HEAD�����ﴦ��HEAD */
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
			aptus->sponte(pius);	/* ת��httpsrvhead */
		}
		break;

	case Notitia::DMD_END_SESSION:	/* ǿ�ƹر� */
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
	unsigned char *ptr, *c_base;	/* c_baseָʾ��һ������chunk֮��, �����cli_rcv.base */
	WBUG("Transfer-Encoding...");
	bool ret = false;

	c_base = (unsigned char*) 0;	/* 0: ��ʾ�մ�PRO_TBUF��,  */
HERE:
	if ( c_base == (unsigned char*) 0 )
		c_base = rcv_buf->base + chunk_offset ; 

	if ( !chunko.started )
	{	/* ��һ��Ӧ����ָʾchunk���� */
		for (ptr = c_base ; ptr < rcv_buf->point - 1; ptr++ )
		{	/* �ж�һ�е�CRLF */
			if ( *ptr == '\r' && *(ptr+1) == '\n' )
			{
				chunko.body_len = get_chunk_size(c_base);
				WBUG("A Chunk size %ld", chunko.body_len);
				chunko.head_len = (ptr - c_base) + 2;	/* ����CRLF */
				break;
			}
		}
		chunko.started =  ( chunko.body_len >= 0 );
	}

	if ( chunko.started )
	{	/* ������������, ��ptr��ʼ */
		if ( chunko.body_len == 0 )
		{	/* ����Ӧ����footers, ɨ�赽CRLFCRLFΪֹ */
			/* һ������� 0CRLFCRLF
			   ��һ����: 0.....CRLFCRLF �� 0....CRLF....CRLFCRLF
			   ����, ptr���������ֽ�, ��chunkͷ��CRLFҲ���ȥ
			*/
			for (ptr = &c_base[chunko.head_len-2] ; ptr < rcv_buf->point - 3; ptr++ )
			{	/* �ж�CRLFCRLF */
				if ( *ptr == '\r' && *(ptr+1) == '\n' &&
					*(ptr+2) == '\r' && *(ptr+3) == '\n' )
				{
					/* chunk������*/
					WBUG("Chunk completed!");
					ptr +=4 ;		/* ptrָ����������, ��һ��HTTP���� */
					memmove(c_base, ptr, rcv_buf->point - ptr);	/* ����ǰ��,footers���ݱ��� */
					rcv_buf->point -= (ptr - c_base);	
					chunko.reset();
					ret = true;
				}
			}
				
		} else if ( chunko.body_len > 0) {
				/* �������� */
			long bz = chunko.body_len + chunko.head_len +2;	/* ����Chunk����, 2����Ϊ����CRLF */
			if ( rcv_buf->point - c_base >= bz ) 
			{
				/* һ��Chunk������ */
				WBUG("A Chunk cut out");
				/* ֻ����data */
				ptr = &(c_base[chunko.head_len]); /* ָ�������� */
				memmove(c_base, ptr, rcv_buf->point - ptr);	/* body����ǰ��, head���ݱ��� */
				rcv_buf->point -= chunko.head_len;	
				c_base += chunko.body_len;	/* c_baseָ����body��β��CRLF */
				ptr = c_base +2;	/* ptrָ���˱�chunk��������� */
				memmove(c_base, ptr, rcv_buf->point - ptr);	/* ������ǰ��, CRLF������ */
				rcv_buf->point -= 2;	

				chunk_offset += chunko.body_len;
				chunko.reset();

				if ( c_base < rcv_buf->point) /* ���滹��,������һ�� */
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
	isSocket = false;	//�ٶ���ʼ����socket
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
				aptus->facio(&set_buf_pius);	//�ϴβ���Websocket, �������, �����軺����
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
					if (strcmp(protocol, gCFG->sock_pro_def[i].name) ==0 ) //�����򵥲���, protocol���ö��Ÿ����ģ�Ҫ�γ�һ�����顣
					{
						cur_sub_ordo = gCFG->sock_pro_def[i].sub;
						break;
					}
				}
				if ( i == gCFG->sock_num )	//δ�ҵ���ӦЭ��
				{
					setStatus(400);
					has_pro = false;
				} else {		//�Ѷ�������Э��
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

	if ( lastSocket && !isSocket )	//�ϴ���Websocket, ����β���, �Ͱѻ����������
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
				if ( !sock.continued) 	/* �������Ҫ����, ���ǺϷ���. Ҳ����˵ǰһ�������Ѿ�����, ��տ�ʼ */
				{ 
					sock.opcode = sock.frm.opcode; 
					//frame->utf8_state = UTF8_VALID; 
				} else {	/* ��Ҫ����(ǰһ֡��Ҫ��), ������ȴ���¿�ͷ(op��Ϊ0), �ⲻ�Ϸ� */
					sock.framing = Sock_Framing_Close;
					sock.stat_code = STATUS_CODE_PROTOCOL_ERROR; 
					break; 
				} 
			} else if ( !sock.continued ) 	//�������֡, ȴ��Ҫ�����
			{ 
				sock.framing = Sock_Framing_Close; 
				sock.stat_code = STATUS_CODE_PROTOCOL_ERROR; 
				break; 
			} 

			sock.continued = (sock.frm.fin == 0);	//Ҫ��Ҫ����, ����ͼ�����, ����֡�Ͳ���
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
		if (sock.frm.opcode >= 0x8) /* ��ǰ����һ������֡, ���ڱ��� */ 
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
				/* Ҫ��, */
				/* ֻ����һ֡��Ӱ������������ */
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

			if ( !sock.continued)		/* �������������֡�в������, ��ȥ������֡�ڽ��ջ����е����� 
							   ����, ������������֡����������. �������һ��Ч�ʡ�
							*/
			{
				rcv_buf->commit(sock.frm.start_pos - sock.pos);
				sock.neo_frame();
			} else {
				sock.continue_frame();
			}

		} else {	
			/* ����֡����BUFF�� */
			sock.buf_1st.grant(sock.frm.payload_length);
			q = sock.buf_1st.point;
			for (i = 0, j=0; i < sock.frm.payload_length; i++) 
			{ 
				*q++ = p[sock.pos++] ^ sock.frm.mask[j++ & 3]; 
			}
			sock.buf_1st.commit(sock.frm.payload_length);

			/* ����, һ֡�����Ѿ����� */
			if ( sock.continued)
			{	/* ������к���֡ */
				sock.continue_frame();
				goto Pay_End;
			}
	
			/* ������, ֡�Ѿ�ȫ������ */
			switch (sock.opcode) 
			{ 
			case OPCODE_TEXT: 
				/* ����Ҫ��UTF8ת��, �Ժ����� */
				goto DELIVER;
				break;
	
			case OPCODE_BINARY: 
				sock.stat_code = Sock_Status_OK; 
		DELIVER:
				rcv_buf->commit(-sock.pos);
				sock.neo_frame();
				deliver(Notitia::PRO_TBUF);	//�������Ҵ�
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

	/* ������Ҫ��һ���Ƿ�Ҫ�ر����� */
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

	/* ���, �ύ���� */
	local_pius.ordo = Notitia::PRO_TBUF;
	aptus->sponte(&local_pius);
}

HTTPSRVINLINE void HttpSrvBody::sndSocket(unsigned char op_code)
{
/* ���ݶ���sock.buf_2nd�� */
	unsigned char*p = sock.buf_2nd.point;
	unsigned long len = sock.buf_2nd.point - sock.buf_2nd.base;
	
	sndSocket(op_code, p, len);

	/* ���, �ύ���� */
	local_pius.ordo = Notitia::PRO_TBUF;
	aptus->sponte(&local_pius);
}

#include "hook.c"
