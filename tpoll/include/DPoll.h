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

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)  
#include <sys/event.h>
#endif	//for bsd

#if defined(__sun)
#include <port.h>
#include <signal.h>
#include <poll.h>
#endif	//for sun

#if defined(__linux__)
#include <sys/epoll.h>
#include <sys/syscall.h>
#include <linux/aio_abi.h>
#include <inttypes.h>
#elif !defined(_WIN32)
#include <aio.h>
#endif

class DPoll
{
public:
	enum Poll_Type {
		NotUsed =  -1,
		SysExit = 0,
		User = 6,
		Alarm = 7,
		Timer = 8,
		EventFD = 9, /* only for linux eventfd*/
		Aio = 0x10,
		FileD = 0x11,
		IOCPFile = 0x12,
		IOCPSock = 0x13
	};

	struct PollorBase{
		Amor *pupa;	//被调用facio的对象
		Poll_Type type;	//7:alarm, 8:timer , 0x10: Aio, 0x11: aio file, 0x12: socket
	};

#if !defined(_WIN32)
	struct PollorAio : PollorBase {
		Amor::Pius pro_ps;
#if defined(__linux__)
		struct iocb aiocb_W, aiocb_R;
		struct iocb *iocbpp[2];
		aio_context_t ctx;
#else
		struct aiocb aiocb_W, aiocb_R;
#endif

#if defined(__sun)
		port_notify_t pn;
#endif

		inline PollorAio() {
			pupa = 0;
			type = Aio;
			pro_ps.indic = 0;
			memset(&aiocb_R, 0, sizeof(aiocb_R));
			memset(&aiocb_W, 0, sizeof(aiocb_W));
#if defined(__sun)
			aiocb_R.aio_sigevent.sigev_value.sival_ptr = &pn;
			aiocb_W.aio_sigevent.sigev_value.sival_ptr = &pn;
			pn.portnfy_user = this;
#endif
#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
			aiocb_R.aio_sigevent.sigev_value.sival_ptr = this;
			aiocb_W.aio_sigevent.sigev_value.sival_ptr = this;
#endif
#if defined(__linux__)
			aiocb_R.aio_lio_opcode = IOCB_CMD_PREAD;
			aiocb_W.aio_lio_opcode = IOCB_CMD_PWRITE;
			iocbpp[0] = &aiocb_R;
			iocbpp[1] = &aiocb_W;
#endif
		};
	};
#endif

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
		int fd;		//描述符
#endif
		inline Pollor() {
			pupa = 0;
			pro_ps.indic = 0;
			type = FileD;
#if defined(_WIN32)
			hnd.file = INVALID_HANDLE_VALUE;
#else
			fd = -1;
#endif
		};
	};
};
#endif
