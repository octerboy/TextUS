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
class DPoll
{
public:
	enum Poll_Type {
		NotUsed =  -1,
		Readable =  0,
		Writable  = 3,
		Alarm = 7,
		Timer = 8,
		WinFile = 0x11,
		WinSock = 0x12
	};

	struct PollorBase{
		Amor *pupa;	//被调用facio的对象
		Poll_Type type;	//0: whether Readable, 3: whether Writable, 7:alarm, 8:timer , 0x11: windows file, 0x12: windows socket
	};

	struct Pollor : PollorBase {
#if defined(_WIN32)
		long num_of_trans;	 //对于IOCP, 则为NumberOfBytesTransferred
		void *overlap;	//对于IOCP, 则为LPOVERLAPPED
		union {
			HANDLE file;
			SOCKET sock;
		} hnd;
#else
		int fd;		//描述符
#endif
		inline Pollor() {
			pupa = 0;
			type = NotUsed ;
#if defined(_WIN32)
			num_of_trans = 0;
			overlap = 0;
			hnd.sock = INVALID_SOCKET;
			hnd.file = INVALID_HANDLE_VALUE;
#else
			fd = -1;
#endif
		};
	};
};
#endif
