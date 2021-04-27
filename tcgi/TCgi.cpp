/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Textus CGI
 Build: created by octerboy, 2007/03/07, Panyu
 $Id$
*/
#define SCM_MODULE_ID	"$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "DPoll.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "BTool.h"
#include "Describo.h"
#include "PacData.h"

#if defined(_WIN32)
#include <process.h>
#include <string.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#define INLINE inline
#define RCV_FRAME_SIZE 2048

#if defined( _MSC_VER ) && (_MSC_VER < 1400 )
typedef unsigned int* ULONG_PTR;
typedef struct _OVERLAPPED_ENTRY {
	ULONG_PTR lpCompletionKey;
	LPOVERLAPPED lpOverlapped;
	ULONG_PTR Internal;
	DWORD dwNumberOfBytesTransferred;
} OVERLAPPED_ENTRY, *LPOVERLAPPED_ENTRY;
#endif	//for WIN32

class TCgi: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	bool sessioning;
	INLINE void fromExprog();

	TCgi();
	~TCgi();

private:
	Amor::Pius local_pius;
	Describo::Criptor mytor; //保存管道描述符, 各实例不同
	Amor::Pius epl_set_ps, epl_clr_ps, pro_tbuf_ps;
	DPoll::Pollor pollor; /* 保存事件句柄, 各子实例不同 */
	bool wr_blocked;

	TBuffer *rcv_buf, *snd_buf;
	PacketObj *rcv_pac;
#if defined(_WIN32)
	HANDLE pipe_in[2], pipe_out[2];
	OVERLAPPED snd_ovp, rcv_ovp;
	void transmitto_ex();
	void recito_ex();
	DPoll::Pollor pollor_out; /* 保存事件句柄, 各子实例不同 */
	Amor::Pius epl_set_out, epl_clr_out;
#else
	void toExprog_ex();
	TEXTUS_LONG fromExprog_ex();
	int sv[2];
#endif
	TBuffer work_buf;	/* 保存临时参数*/
	char **cgi_argv;	/* 在PRO_UNIPAC时, 这里表明某个cgi_argv置为某个field的内容, 数据在work_buf中 */

	INLINE void toExprog();
	INLINE void deliver(Notitia::HERE_ORDO aordo);
	INLINE void end();
	INLINE bool init();
	INLINE bool parent_exec();
	INLINE bool child_exec();

	typedef struct _Argv {
		char *val;
		int field_no;	/* -1, 表示不授受PacketObj中的域值 */
		inline _Argv () {
			val = 0;
			field_no = -1;
		};
	} Argv;
	
	enum DIRECTION { BOTH=0, TO_PROG=1, FROM_PROG=3, PROG_ONCE=4 } ;

	struct G_CFG {
		bool use_epoll;
		Amor *sch;
		struct DPoll::PollorBase lor; /* 探询 */
		const char *exec_file;
		int argc;	/* 参数个数, 包括argv[0] 指向执行文件名 */
		TBuffer buf; 	/* 保存各个常量参数 */
		Argv *argv;
		DIRECTION direction;
		bool has_buffer;	/* 向后提供TBuffer, 这里成为数据源 */

		inline G_CFG () {
			lor.type = DPoll::NotUsed;
			sch = 0;
			exec_file = 0;
			argc = 0;
			argv = 0 ;
			has_buffer = false;
		};

		inline ~G_CFG() {
			if ( argv)	
				delete[] argv;
		};
	};
	struct G_CFG *gCFG;
	bool has_config;
#include "wlog.h"
};

#include <assert.h>
#include <stdio.h>
#include "textus_string.h"
#include "casecmp.h"

void TCgi::ignite(TiXmlElement *cfg)
{
	const char *comm_str;
	TiXmlElement *v_ele;
	int aLen, iLen;
	unsigned int i;
	unsigned char *p;

	if ( !cfg ) return;
	
	if ( gCFG ) 
		delete gCFG;

	gCFG = new struct G_CFG();
	has_config = true;

	gCFG->exec_file = cfg->Attribute("command");
	if ( !gCFG->exec_file ) 
		goto End;

	gCFG->argc = 1;
	aLen =  0;
	v_ele = cfg->FirstChildElement("argv");

	while ( v_ele)
	{
		if (v_ele->GetText())
		{
			aLen += static_cast<int>(strlen(v_ele->GetText()));
			aLen++; /* for null char */
		}
		gCFG->argc++;
		v_ele = v_ele->NextSiblingElement("argv");
	}

	gCFG->buf.grant(aLen);

	gCFG->argv = new Argv [gCFG->argc];
	gCFG->argv[0].val = (char*) gCFG->exec_file;
	cgi_argv = new char* [gCFG->argc+1];
	memset(cgi_argv, 0, sizeof(char*) * ( gCFG->argc+1) );
	
	aLen = 0; p = gCFG->buf.point;
	for (	i = 1, v_ele = cfg->FirstChildElement("argv"); 
		v_ele; 
		v_ele = v_ele->NextSiblingElement("argv"), i++ )
	{
		if (v_ele->GetText())
		{
			iLen = BTool::unescape(v_ele->GetText(), p);
			p[iLen] = '\0';
			gCFG->argv[i].val = (char*) p;
			p += iLen; ++p;
			aLen += iLen; aLen++;
		} 

		if ( (comm_str = v_ele->Attribute("field")) )
		{
			gCFG->argv[i].field_no = atoi(comm_str);
		}
	}
	gCFG->buf.commit(aLen);
	
	gCFG->direction = BOTH;
	comm_str = cfg->Attribute("only");
	if (comm_str ) 
	{
		if (strcasecmp(comm_str, "once") == 0 )
			gCFG->direction = PROG_ONCE;

		if (strcasecmp(comm_str, "write") == 0 )
			gCFG->direction = TO_PROG;

		if ( strcasecmp(comm_str, "read") == 0 )
			gCFG->direction = FROM_PROG;

		if (strcasecmp(comm_str, "read/write") == 0 
			||strcasecmp(comm_str, "write/read") == 0 )
			gCFG->direction = BOTH;
	}

	comm_str = cfg->Attribute("provide");
	if (comm_str ) 
	{
		if ( strcasecmp(comm_str, "buffer") == 0 )
			gCFG->has_buffer = true;
	}
	
	if ( gCFG->has_buffer && !rcv_buf && !snd_buf )
	{
		rcv_buf =new TBuffer(2048);
		snd_buf =new TBuffer(2048);
	}
End:
	return;	
}

bool TCgi::facio( Amor::Pius *pius)
{
	TBuffer **tb;
	PacketObj **tmp;
	int i;
	unsigned char *p;
	Amor::Pius tmp_p;
#if defined (_WIN32 )	
	OVERLAPPED_ENTRY *aget;
#else
	TEXTUS_LONG len;
#endif	//for WIN32


	assert(pius);
	switch (pius->ordo)
	{
	case Notitia::PRO_TBUF :
		WBUG("facio PRO_TBUF");
		if ( gCFG->use_epoll)
		{
#if defined (_WIN32 )
			transmitto_ex();
#else
			toExprog_ex();
#endif
		} else {
			toExprog();
		}
		break;

	case Notitia::PRO_UNIPAC :
		WBUG("facio PRO_UNIPAC");
		work_buf.reset();
		memset(cgi_argv, 0, sizeof(char*) * ( gCFG->argc+1) );	/* cgi_argv清空, 每次init再赋值 */

		p = work_buf.point;
		for ( i = 1; i < gCFG->argc; i++ )
		{
			int &no = gCFG->argv[i].field_no;

			if ( no >= 0 && rcv_pac->max >= no
				&& rcv_pac->fld[no].val	)
			{
				unsigned TEXTUS_LONG &range = rcv_pac->fld[no].range;
				if ( p + range +1 >= work_buf.limit )	/* 一个程序接受的参数超过1024? */
					break;
				memcpy(p, rcv_pac->fld[no].val, range);
				p[range] = '\0';
				cgi_argv[i] = (char*) p;
				p += range; p++;
			}
		}
		work_buf.commit(p - work_buf.point );
		break;

	case Notitia::ERR_EPOLL:
		WBUG("facio ERR_EPOLL");
		WLOG(WARNING, (char*)pius->indic);	
		end();	//直接关闭就可.
		break;

	case Notitia::PRO_EPOLL:
		WBUG("facio PRO_EPOLL");
#if defined (_WIN32 )
		aget = (OVERLAPPED_ENTRY *)pius->indic;
		if ( aget->lpOverlapped == &(rcv_ovp) )
		{	//已读数据,  不失败并有数据才向接力者传递
			if ( aget->dwNumberOfBytesTransferred ==0 ) 
			{
				WLOG(INFO, "IOCP recv 0 disconnected");
				end();
			} else {
				WBUG("child PRO_EPOLL recv %d bytes", aget->dwNumberOfBytesTransferred);
				snd_buf->commit(aget->dwNumberOfBytesTransferred);
				aptus->facio(&pro_tbuf_ps);
				recito_ex();
			}
		} else if ( aget->lpOverlapped == &(snd_ovp) ) {
			WBUG("client PRO_EPOLL sent %d bytes", aget->dwNumberOfBytesTransferred); //写数据完成
		} else {
			WLOG(ALERT, "not my overlap");
		}
#endif
		break;

	case Notitia::RD_EPOLL:
		WBUG("facio RD_EPOLL");
#if !defined (_WIN32 )
LOOP:
		switch ( (len = fromExprog_ex()) ) 
		{
		case 0:	//Pending
			/* action flags and filter for event remain unchanged */
			gCFG->sch->sponte(&epl_set_ps);	//向tpoll,  再一次注册
			break;

		case -1://Close
			end();	//失败即关闭
			break;

		case -2://Error
			end();	//失败即关闭
			break;

		default:	
			WBUG("recv %ld bytes", len);
			if ( len < RCV_FRAME_SIZE ) { 
				/* action flags and filter for event remain unchanged */
				gCFG->sch->sponte(&epl_set_ps);	//向tpoll,  再一次注册
				aptus->sponte(&pro_tbuf_ps);
			} else {
				aptus->sponte(&pro_tbuf_ps);
				goto LOOP;
			}
			break;
		}
#endif
		break;

	case Notitia::WR_EPOLL:
		WBUG("facio WR_EPOLL");
#if !defined (_WIN32 )
		toExprog_ex();
#endif
		break;

	case Notitia::EOF_EPOLL:
		WBUG("facio EOF_EPOLL");
		WLOG(INFO, "pipe closed.");
		end();
		break;

	case Notitia::FD_PRORD:
		WBUG("facio FD_PRORD");
		/* 读数据, 这是最常的, 只有不失败并有数据才向左节点传递 */
		fromExprog();
		break;

	case Notitia::FD_PROWR:
		WBUG("facio FD_PROWR");
		toExprog();
		//写, 很少见, 除非系统很忙
		break;

	case Notitia::FD_PROEX:
		WBUG("facio FD_PROEX");
		end();
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		end();
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		if ( gCFG->has_buffer)
			deliver(Notitia::SET_TBUF);

		tmp_p.ordo = Notitia::CMD_GET_SCHED;
		aptus->sponte(&tmp_p);	//向tpoll, 取得sched
		gCFG->sch = (Amor*)tmp_p.indic;
		if ( !gCFG->sch ) 
		{
			WLOG(ERR, "no sched or tpoll");
			break;
		}
		tmp_p.ordo = Notitia::POST_EPOLL;
		tmp_p.indic = &gCFG->lor;
		gCFG->lor.pupa = this;
		
		gCFG->sch->sponte(&tmp_p);	//向tpoll, 取得TPOLL
		if ( tmp_p.indic == gCFG->sch )
			gCFG->use_epoll = true;
		else
			gCFG->use_epoll = false;

		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		if ( gCFG->has_buffer)
			deliver(Notitia::SET_TBUF);
		break;

	case Notitia::SET_TBUF:	/* 取得输入TBuffer地址 */
		WBUG("facio SET_TBUF");
		if ( gCFG->has_buffer)	/* 这里有buffer, 不用别的 */
			break;

		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//当然tb不能为空
			if ( *tb) 
			{	//新到请求的TBuffer
				rcv_buf = *tb;
			}
			tb++;
			if ( *tb) snd_buf = *tb;
		} else 
			WLOG(NOTICE,"facio PRO_TBUF null.");
		break;

	case Notitia::SET_UNIPAC:
		WBUG("facio SET_UNIPAC");
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) rcv_pac = *tmp; 
			else
				WLOG(WARNING, "facio SET_UNIPAC rcv_pac null");

		} else 
			WLOG(WARNING, "facio SET_UNIPAC null");
		break;

	case Notitia::DMD_START_SESSION:
		WBUG("facio DMD_START_SESSION");
		init();		/* 启动外部进程 */
		break;

	case Notitia::FORKED_PARENT:
		WBUG("facio FORKED_PARENT");
		if ( sessioning )
			parent_exec();
		break;

	case Notitia::FORKED_CHILD:
		WBUG("facio FORKED_CHILD");
		if ( sessioning )
			child_exec();
		break;
	default:
		return false;
	}	
	return true;
}

bool TCgi::sponte( Amor::Pius *pius) 
{ 
	switch (pius->ordo)
	{
	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		end();
		break;

	case Notitia::PRO_TBUF :
		WBUG("sponte PRO_TBUF");
		if ( gCFG->use_epoll)
		{
#if defined (_WIN32 )
			transmitto_ex();
#else
			toExprog_ex();
#endif
		} else {
			toExprog();
		}
		break;

	case Notitia::CMD_CHANNEL_PAUSE :
		WBUG("sponte CMD_CHANNEL_PAUSE");
		if ( gCFG->use_epoll)
		{
			gCFG->sch->sponte(&epl_clr_ps); //向tpoll,  注销
		} else {
			 deliver(Notitia::FD_CLRRD);
		}

		break;

	case Notitia::CMD_CHANNEL_RESUME :
		WBUG("sponte CMD_CHANNEL_RESUME");
		if ( gCFG->use_epoll)
		{
			gCFG->sch->sponte(&epl_set_ps); //向tpoll,  注册
		} else {
			deliver(Notitia::FD_SETRD);
		}

		break;

	default:
		return false;
	}	
	return true;
}

TCgi::TCgi():work_buf(1024)
{
	pollor.pupa = this;
#if defined(_WIN32)
	pollor.type = DPoll::IOCPFile;
	pollor_out.type = DPoll::IOCPFile;
	epl_clr_out.ordo = Notitia::CLR_EPOLL;
	epl_clr_out.indic = &pollor_out;
	epl_set_out.ordo = Notitia::SET_EPOLL;
	epl_set_out.indic = &pollor_out;
#endif
	epl_clr_ps.ordo = Notitia::CLR_EPOLL;
	epl_clr_ps.indic = &pollor;
	epl_set_ps.ordo = Notitia::SET_EPOLL;
	epl_set_ps.indic = &pollor;
	pro_tbuf_ps.ordo = Notitia::PRO_TBUF;
	pro_tbuf_ps.indic = 0;

	mytor.pupa = this;
	local_pius.ordo = Notitia::TEXTUS_RESERVED;
	local_pius.indic = &mytor;

	gCFG = 0;
	has_config = false;

	cgi_argv = (char**) 0;
	sessioning = false;
	wr_blocked = false;

	rcv_buf = 0;
	snd_buf = 0;
}

TCgi::~TCgi()
{	
	if ( has_config &&  gCFG ) 
		delete gCFG;

	if ( cgi_argv )
		delete[] cgi_argv;

	if ( gCFG->has_buffer )	/* 不判断rcv_buf是否为NULL, 如有问题则在此出现core dump */
	{
		delete rcv_buf;
		delete snd_buf;
	}
}

INLINE void TCgi::toExprog()
{
	int wBytes;
	if ( !sessioning )
	{
		if ( !init() )
			return;
	}

	if ( rcv_buf )
	{
		if ( gCFG->direction == BOTH || gCFG->direction == TO_PROG || gCFG->direction == PROG_ONCE )
		{
#if defined(_WIN32)
		WAgain:
		if ( WriteFile( pipe_out[1], rcv_buf->base, static_cast<DWORD>(rcv_buf->point - rcv_buf->base), (LPDWORD) &wBytes, NULL) == FALSE )
#else
		if ( rcv_buf->point ==  rcv_buf->base  ) 
		{
			shutdown(sv[0], SHUT_WR);	// 通知对端数据发送完毕
			return ;
		}

		if ( (wBytes = send(sv[0], rcv_buf->base, rcv_buf->point - rcv_buf->base, MSG_NOSIGNAL)) < 0 )
#endif
		{
			WLOG_OSERR("write pipe")
			end();
		} else {
			rcv_buf->commit(-wBytes);
			if ( rcv_buf->point - rcv_buf->base > 0 )
#if defined(_WIN32)
				goto WAgain;
#else
			{
				local_pius.ordo =Notitia::FD_SETWR;
				mytor.scanfd = sv[0];
				aptus->sponte(&local_pius);
				wr_blocked = true;
			}  else if ( wr_blocked ) {
				local_pius.ordo =Notitia::FD_CLRWR;
				mytor.scanfd = sv[0];
				aptus->sponte(&local_pius);
				wr_blocked = false;
			}
#endif
		}
		}
	}

	if ( gCFG->direction == PROG_ONCE )
#if defined(_WIN32)
		CloseHandle(pipe_out[1]);	/*  关闭写管道 */
#else
		shutdown(sv[0], SHUT_WR);	// 通知对端数据发送完毕
#endif
	/* 非持久的, 一次性的, 则写完后即 关闭写管道 */

	return ;
}

INLINE void TCgi::fromExprog()
{
	/* 读取pipe_in[0]中的数据 */
	unsigned char tmp[RCV_FRAME_SIZE], *t_buf;
	bool has = false;
	int rBytes;
	if (snd_buf && ( gCFG->direction == BOTH || gCFG->direction == FROM_PROG ))
	{
		snd_buf->grant(RCV_FRAME_SIZE);
		t_buf = snd_buf->point;
		has = true;	/* 将有数据传递 */
	} else 
		t_buf = tmp;

#if defined(_WIN32)
	if (ReadFile(pipe_in[0], t_buf, RCV_FRAME_SIZE, (LPDWORD)&rBytes, NULL) == FALSE)
#else
	if ( (rBytes = recv(sv[0], t_buf, RCV_FRAME_SIZE, MSG_NOSIGNAL)) < 0 )
#endif
	{
		WLOG_OSERR("read pipe");
		end();
	} else if ( rBytes == 0 ) {
		WLOG(INFO, "pipe closed");
		end();
	} else {
		if ( has )
		{
			snd_buf->commit(rBytes);
			deliver(Notitia::PRO_TBUF);
		} 
	}
}

#if defined(_WIN32)
typedef void (_cdecl *my_thread_func)(void*);
static void  cycle_fromPro (TCgi *arg)
{
	while ( arg->sessioning )
	{
		arg->fromExprog();
	}
}
#endif

bool TCgi::init()
{
	int i;
	for ( i = 0 ; i < gCFG->argc; i++ )
	{
		if ( gCFG->argv[i].val && !cgi_argv[i] )
			cgi_argv[i] = gCFG->argv[i].val;
	}

#if defined(_WIN32)
	SECURITY_ATTRIBUTES sa;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

#define CMDLENGTH 4096
	char cmdLine[CMDLENGTH];
	int len;
	
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL; //使用系统默认的安全描述符
	sa.bInheritHandle = TRUE; //一定要为TRUE，不然句柄不能被继承。

	if ( !CreatePipe(&pipe_in[0], &pipe_in[1], &sa, 0)) 
	{
		WLOG_OSERR("CreatePipe pipe_in");
		return false;
	}

	if ( !CreatePipe(&pipe_out[0], &pipe_out[1], &sa, 0)) 
	{
		WLOG_OSERR("CreatePipe pipe_out");
		return false;
	}

	si.cb = sizeof(STARTUPINFO);
	GetStartupInfo(&si);
	si.hStdInput = pipe_out[0];
	//si.hStdError = pipe_in[1];
	si.hStdOutput = pipe_in[1];
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

	memset(cmdLine, 0, CMDLENGTH);
	cmdLine[0] = '\"';
	len = static_cast<int>(strlen(gCFG->exec_file));
	if ( len < CMDLENGTH-10  )
	TEXTUS_STRCAT ( cmdLine, gCFG->exec_file);
	TEXTUS_STRCAT ( cmdLine, "\"");
	len += 2;
	for ( i =1 ; cgi_argv[i]; i++)
	{
		len += static_cast<int>(strlen(cgi_argv[i])) + 1;
		if ( len > CMDLENGTH - 10 ) break;
		TEXTUS_STRCAT(cmdLine, " ");
		TEXTUS_STRCAT(cmdLine, cgi_argv[i]);
	}
	
	if (!CreateProcess(NULL, cmdLine, NULL,NULL,TRUE,NULL,NULL,NULL,&si,&pi)) 
	{
		WLOG_OSERR("CreateProcess");
		return false;
	}

	CloseHandle(pipe_in[1]); // 关闭读管道的写端
	CloseHandle(pipe_out[0]); // 关闭写管道的读端

	sessioning = true;
	if ( gCFG->use_epoll ) 
	{
		pollor.hnd.file = pipe_in[0];
		pollor.pro_ps.ordo = Notitia::PRO_EPOLL;
		gCFG->sch->sponte(&epl_set_ps);	//向tpoll
		pollor_out.hnd.file = pipe_out[1];
		pollor_out.pro_ps.ordo = Notitia::PRO_EPOLL;
		gCFG->sch->sponte(&epl_set_out);	//向tpoll
		if ( gCFG->direction == BOTH || gCFG->direction == FROM_PROG )
		{
			/* 主动去接收, 如果一开始有数据, 则先接收; 另外实现IOCP投递 */
			recito_ex();
		}
	} else {
		if ( gCFG->direction == BOTH || gCFG->direction == FROM_PROG )
		{
			/* 以另一线程去读管道 */
			if ( _beginthread((my_thread_func)cycle_fromPro, 0, this) == -1 )
				WLOG_OSERR("_beginthread");
		}
	}
#else
	if ( socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1 )	// 创建管道
	{
		WLOG_OSERR("create socketpair)");
		return false;
	}

	sessioning = true;
	/* 加入轮询 */
	if ( gCFG->use_epoll ) 
	{
		pollor.pro_ps.ordo = Notitia::RD_EPOLL;
		pollor.fd = sv[0];
#if  defined(__linux__)
		pollor.ev.events &= ~EPOLLOUT;
#endif	//for linux

#if  defined(__sun)
		pollor.events &= ~POLLOUT;
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		EV_SET(&(pollor.events[0]), sv[0], EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, &pollor);
		EV_SET(&(pollor.events[1]), sv[0], EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, &pollor);
#endif	//for bsd

		gCFG->sch->sponte(&epl_set_ps);	//向tpoll
	} else {
		mytor.scanfd = sv[0];
		deliver(Notitia::FD_SETEX);
		deliver(Notitia::FD_SETRD);
	}
	deliver(Notitia::CMD_FORK);
#endif
	return true;
}

INLINE bool TCgi::parent_exec()
{
#if !defined(_WIN32)
	WBUG("parent process ");
	close(sv[1]);	
	// 关闭管道的子进程端
	/* 现在可向sv[0]读写  */
	memset(cgi_argv, 0, sizeof(char*) * ( gCFG->argc+1) );	/* cgi_argv清空, 每次init再赋值 */
#endif
	return true;
}

INLINE bool TCgi::child_exec()
{
#if !defined(_WIN32)
	close(sv[0]);	// 关闭管道的父进程端
	WBUG("child process exec %s", gCFG->exec_file);
	if ( dup2(sv[1], STDOUT_FILENO) == -1 )	// 复制管道的子进程端到标准输出
	{
		WLOG_OSERR("dup2 socket to stdout");
	} else {
		WBUG("dup2 socket to stdout successfully!");
	}
	if ( dup2(sv[1], STDIN_FILENO) == -1 )	// 复制管道的子进程端到标准输入
	{
		WLOG_OSERR("dup2 socket to stdin");
	} else {
		WBUG("dup2 socket to stdin successfully!");
	}
	close(sv[1]);	// 关闭已复制的读管道
	/* exec执行命令或外部程序, 映像被替换 */
	execvp(gCFG->exec_file, cgi_argv);
#endif
	return true;
}

INLINE void TCgi::end()
{
	WBUG("end().....");
	if ( !sessioning ) 	/* 避免重复 */
		return;
	sessioning = false;
#if defined(_WIN32)
	CloseHandle(pipe_in[0]);	/* 关闭读管道 */
	CloseHandle(pipe_out[1]);	/*  关闭写管道 */
#else
	mytor.scanfd = sv[0];
	deliver(Notitia::FD_CLRWR);
	deliver(Notitia::FD_CLREX);
	deliver(Notitia::FD_CLRRD);

	//close(pipe_in[0]);	/* 关闭读管道会导致另一个子实例中的pipe_in[0]成为非法(尽管其具体值不同) 所以这里不用了 */
	//close(sv[0]);
#endif
	deliver(Notitia::END_SESSION);/* 向左、右传递本类的会话关闭信号 */
}

Amor* TCgi::clone()
{
	TCgi *child;

	child = new TCgi();
	child->gCFG = gCFG;
	
	child->cgi_argv = new char* [gCFG->argc+1];
	memset(child->cgi_argv, 0, sizeof(char*) * ( gCFG->argc+1) );

	if ( gCFG->has_buffer && !child->rcv_buf && !child->snd_buf )
	{
		child->rcv_buf =new TBuffer(2048);
		child->snd_buf =new TBuffer(2048);
	}
	
	return (Amor*)child;
}

/* 向接力者提交 */
INLINE void TCgi::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	void *ps[3];

	switch (aordo )
	{
	case Notitia::PRO_TBUF:
		WBUG("deliver PRO_TBUF");
		if ( gCFG->has_buffer )
			aptus->facio(&tmp_pius);
		else
			aptus->sponte(&tmp_pius);
		return;

	case Notitia::SET_TBUF:
		WBUG("deliver SET_TBUF");
		ps[0]= snd_buf;
		ps[1]= rcv_buf;
		ps[2]= 0;
		tmp_pius.indic = ps;
		aptus->facio(&tmp_pius);
		return;

	case Notitia::END_SESSION:
		WBUG("deliver END_SESSION");
		aptus->facio(&tmp_pius);
		break;

	case Notitia::START_SESSION:
		WBUG("deliver START_SESSION");
		aptus->facio(&tmp_pius);
		break;

	case Notitia::CMD_FORK:
		WBUG("deliver CMD_FORK");
		break;

	case Notitia::FD_CLRRD:
	case Notitia::FD_CLRWR:
	case Notitia::FD_CLREX:
	case Notitia::FD_SETRD:
	case Notitia::FD_SETWR:
	case Notitia::FD_SETEX:
		WBUG("deliver FD_CLR/SET(%d)", aordo);
		local_pius.ordo =aordo;
		gCFG->sch->sponte(&local_pius);	//向Sched
		return ;

	default:
		break;
	}
	aptus->sponte(&tmp_pius);
}

#if defined (_WIN32 )
void TCgi::transmitto_ex()
{
	DWORD snd_len = static_cast<DWORD> (rcv_buf->point - rcv_buf->base);	//发送长度
	memset(&snd_ovp, 0, sizeof(OVERLAPPED));
	if ( !WriteFile(pipe_out[1], rcv_buf->base, snd_len, NULL, &snd_ovp) )
	{
		if ( ERROR_IO_PENDING != GetLastError() ) {
			WLOG_OSERR("write pipe")
			end();
			return ;
		}
	}
	snd_buf->commit(-(TEXTUS_LONG)snd_len);	//已经到了系统
}

void TCgi::recito_ex()
{
	snd_buf->grant(RCV_FRAME_SIZE);
	memset(&rcv_ovp, 0, sizeof(OVERLAPPED));
	if (!ReadFile(pipe_in[0], snd_buf->point, RCV_FRAME_SIZE, NULL, &rcv_ovp) )
	{
		if ( ERROR_IO_PENDING != GetLastError() ) {
			WLOG_OSERR("read pipe");
			end();
		}
	} 
}

#else	//for not windows

/* 接收发生错误时, 建议关闭这个套接字 */
TEXTUS_LONG TCgi::fromExprog_ex()
{	
	/* 读取pipe_in[0]中的数据 */
	unsigned char tmp[RCV_FRAME_SIZE], *t_buf;
	bool has = false;
	TEXTUS_LONG rBytes;
	if (snd_buf && ( gCFG->direction == BOTH || gCFG->direction == FROM_PROG ))
	{
		snd_buf->grant(RCV_FRAME_SIZE);
		t_buf = snd_buf->point;
		has = true;	/* 将有数据传递 */
	} else 
		t_buf = tmp;

ReadAgain:
	if ( (rBytes = recv(sv[0], t_buf, RCV_FRAME_SIZE, MSG_NOSIGNAL)) < 0 )
	{
		int error = errno;
		if (error == EINTR)
		{	 //有信号而已,再试
			goto ReadAgain;
		} else if ( error == EAGAIN || error == EWOULDBLOCK )
		{	//还在进行中, 回去再等.
			WLOG(INFO, "read pipe encounter EAGAIN");
			return 0;
		} else	
		{	//的确有错误
			WLOG_OSERR("read pipe");
			return -2;
		}
	} else if ( rBytes == 0 ) {
		WLOG(INFO, "pipe closed");
		return -1;
	}
	if ( has ) 
		snd_buf->commit(rBytes);
	return rBytes;
}

/* 发送有错误时, 返回-1, 建议关闭这个套接字 */
void TCgi::toExprog_ex()
{
	TEXTUS_LONG wBytes, snd_len ;
	int ret;
	if ( !sessioning )
	{
		if ( !init() )
			return;
	}
#define RET_PRO(X) ret=X; goto EP_PRO;

	if ( rcv_buf  && (gCFG->direction == BOTH || gCFG->direction == TO_PROG || gCFG->direction == PROG_ONCE ))
	{
		if ( rcv_buf->point ==  rcv_buf->base  ) 
		{
			shutdown(sv[0], SHUT_WR);	// 通知对端数据发送完毕
			return ;
		}

SendAgain:
		snd_len = rcv_buf->point - rcv_buf->base;	//发送长度
		if ( (wBytes = send(sv[0], rcv_buf->base, rcv_buf->point - rcv_buf->base, MSG_NOSIGNAL)) < 0 )
		{
			int error = errno;
			if (error == EINTR)	
			{	//有信号而已,再试
				WLOG_OSERR("write pipe")
				goto SendAgain;
			} else if (error ==EWOULDBLOCK || error == EAGAIN)
			{	//回去再试, 用select, 要设wrSet
				WLOG(INFO, "writing pipe encounter EAGAIN");
				if ( wr_blocked ) 	//最近一次阻塞
				{
					RET_PRO(3)
				} else {	//刚发生的阻塞
					wr_blocked = true;
					RET_PRO(1)
				}
			} else {
				WLOG_OSERR("write pipe")
				RET_PRO(-1)
			}
		} else {
			rcv_buf->commit(-wBytes);
			if (snd_len > wBytes )
			{	
				WLOG(INFO, "write pipe not completed.");
				if ( wr_blocked ) 	//最近一次还是阻塞
				{
					RET_PRO(3)
				} else {	//刚发生的阻塞
					wr_blocked = true;
					RET_PRO(1) //回去再试, 
				}
			} else 
			{	//不发生阻塞了
				if ( wr_blocked ) 	//最近一次阻塞
				{
					wr_blocked = false;
					RET_PRO(2) //发送完成, 不用再设
				} else
				{	//一直没有阻塞
					RET_PRO(0) //发送完成
				}
			}
		}
EP_PRO:
		switch ( ret )
		{
		case 0: //没有阻塞, 不变
			break;
	
		case 2: //原有阻塞, 没有阻塞了, 清一下
#if  defined(__linux__)
			pollor.ev.events &= ~EPOLLOUT;	//等下一次设置POLLIN时不再设
#endif	//for linux

#if  defined(__sun)
			pollor.events &= ~POLLOUT;	//等下一次设置POLLIN时不再设
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
			pollor.events[1].flags = EV_ADD | EV_DISABLE;	//等下一次设置时不再设
#endif	//for bsd
			break;
		
		case 1:	//新写阻塞, 需要设一下了
		case 3:	//还是写阻塞, 再设
#if  defined(__linux__)
			pollor.ev.events |= EPOLLOUT;
#endif	//for linux

#if  defined(__sun)
			pollor.events |= POLLOUT;
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
			pollor.events[1].flags = EV_ADD | EV_ONESHOT;
#endif	//for bsd

			gCFG->sch->sponte(&epl_set_ps);	//向tpoll, 以设置kqueue等
			return;	//即返回, PROG_ONCE不起作用
	
		case -1://有严重错误, 关闭
			end();
			break;

		default:
			break;
		}
	}

	if ( gCFG->direction == PROG_ONCE )
		shutdown(sv[0], SHUT_WR);	// 通知对端数据发送完毕
	/* 非持久的, 一次性的, 则写完后即 关闭写管道 */
}

#endif

#include "hook.c"
