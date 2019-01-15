/* Copyright (c) 2018-2019 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 ID: Textus-DPoll.h
 Title: 与TPoll类交互的数据描述, 
 Build: 
	B01:created by octerboy, 2018/12/3
*/
#ifndef DPOLL__H
#define DPOLL__H
#include "Amor.h"

#if defined(__linux__)
#include <sys/epoll.h>
#endif

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
#include <sys/event.h>
#endif	//for bsd

#if defined(__sun)
#include <port.h>
#include <signal.h>
#include <poll.h>
#endif	//for sun

class DPoll
{
public:
	enum Poll_Type {
		NotUsed =  -1,
		Alarm = 7,
		Timer = 8,
		Aio = 0x10,
		File = 0x11,
		Sock = 0x12
	};

	struct PollorBase{
		Amor *pupa;	//被调用facio的对象
		Poll_Type type;	//7:alarm, 8:timer , 0x10: Aio, 0x11: aio file, 0x12: socket
	};

	struct PollorAio : PollorBase {
		Amor::Pius pro_ps;
#if defined(_WIN32)
		HANDLE file_hnd;
#endif

#if defined(__linux__)
#endif

#if defined(__sun)
#endif

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
#endif
		inline PollorAio() {
			pupa = 0;
			type = NotUsed ;
			pro_ps.indic = 0;
#if defined(_WIN32)
			file_hnd = INVALID_HANDLE_VALUE;
#endif
		};
	};
	struct Pollor : PollorBase {
		Amor::Pius pro_ps;
#if defined(_WIN32)
		union {
			HANDLE file;
			SOCKET sock;
		} hnd;
#endif
#if defined(__linux__)
		struct epoll_event ev;
		int op;
		int fd;		//描述符
#endif

#if defined(__sun)
		short   events;
		int fd;		//描述符
#endif
#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		struct kevent events[2];
#endif
		inline Pollor() {
			pupa = 0;
			type = NotUsed ;
			pro_ps.indic = 0;
#if defined(_WIN32)
			hnd.sock = INVALID_SOCKET;
			hnd.file = INVALID_HANDLE_VALUE;
#endif
		};
	};
};
#endif
