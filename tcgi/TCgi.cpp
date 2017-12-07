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

#include "Amor.h"
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
	bool wr_blocked;

	TBuffer *rcv_buf, *snd_buf;
	PacketObj *rcv_pac;
#if defined(_WIN32)
	HANDLE pipe_in[2], pipe_out[2];
#else
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
		const char *exec_file;
		int argc;	/* 参数个数, 包括argv[0] 指向执行文件名 */
		TBuffer buf; 	/* 保存各个常量参数 */
		Argv *argv;
		DIRECTION direction;
		bool has_buffer;	/* 向后提供TBuffer, 这里成为数据源 */

		inline G_CFG () {
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
			aLen += strlen(v_ele->GetText());
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

	assert(pius);
	switch (pius->ordo)
	{
	case Notitia::PRO_TBUF :
		WBUG("facio PRO_TBUF");
		toExprog();
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
				&& rcv_pac->fld[no].val
			)
			{
				unsigned long &range = rcv_pac->fld[no].range;
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
		toExprog();
		break;

	case Notitia::CMD_CHANNEL_PAUSE :
		WBUG("sponte CMD_CHANNEL_PAUSE");
		deliver(Notitia::FD_CLRRD);
		break;

	case Notitia::CMD_CHANNEL_RESUME :
		WBUG("sponte CMD_CHANNEL_RESUME");
		deliver(Notitia::FD_SETRD);
		break;

	default:
		return false;
	}	
	return true;
}

TCgi::TCgi():work_buf(1024)
{
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
		if ( WriteFile( pipe_out[1], rcv_buf->base, rcv_buf->point - rcv_buf->base, (LPDWORD) &wBytes, NULL) == FALSE )
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
	unsigned char tmp[1024], *t_buf;
	bool has = false;
	int rBytes;
	if (snd_buf && ( gCFG->direction == BOTH || gCFG->direction == FROM_PROG ))
	{
		snd_buf->grant(1024);
		t_buf = snd_buf->point;
		has = true;	/* 将有数据传递 */
	} else 
		t_buf = tmp;

#if defined(_WIN32)
	if (ReadFile(pipe_in[0], t_buf, 1024, (LPDWORD)&rBytes, NULL) == FALSE)
#else
	if ( (rBytes = recv(sv[0], t_buf, 1024, MSG_NOSIGNAL)) < 0 )
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
	len = strlen(gCFG->exec_file);
	if ( len < CMDLENGTH-10  )
	TEXTUS_STRCAT ( cmdLine, gCFG->exec_file);
	TEXTUS_STRCAT ( cmdLine, "\"");
	len += 2;
	for ( i =1 ; cgi_argv[i]; i++)
	{
		len += strlen(cgi_argv[i]) + 1;
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
	if ( gCFG->direction == BOTH || gCFG->direction == FROM_PROG )
	{
		/* 以另一线程去读管道 */
		if ( _beginthread((my_thread_func)cycle_fromPro, 0, this) == -1 )
			WLOG_OSERR("_beginthread");
	}
#else
	if ( socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1 )	// 创建管道
	{
		WLOG_OSERR("create socketpair)");
		return false;
	}

	sessioning = true;
	/* 加入轮询 */
	mytor.scanfd = sv[0];
	deliver(Notitia::FD_SETEX);
	deliver(Notitia::FD_SETRD);
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

	case Notitia::DMD_SET_TIMER:
	case Notitia::DMD_CLR_TIMER:
		WBUG("deliver DMD_SET(CLR)_TIMER(%d)", aordo);
		tmp_pius.indic = this;
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
		aptus->sponte(&local_pius);	//向Sched
		return ;

	default:
		break;
	}
	aptus->sponte(&tmp_pius);
}

#include "hook.c"
