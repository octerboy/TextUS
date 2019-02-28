/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: serial communication
 Build: created by octerboy, 2019/01/25
 $Id$
*/
#define SCM_MODULE_ID	"$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "DPoll.h"
#include "Amor.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "textus_string.h"

#include "casecmp.h"
#include "BTool.h"

#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>

#ifndef inline
#define inline inline
#endif 

#define RCV_FRAME_SIZE 8192
#define ERRSTR_LEN 1024
#define ERROR_PRO(X) { \
	char *s; \
	char error_string[1024]; \
	DWORD dw = GetLastError(); \
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw, \
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) error_string, 1024, NULL );\
	s= strstr(error_string, "\r\n") ; \
	if (s )  *s = '\0';  \
	if ( errMsg ) \
		TEXTUS_SNPRINTF(errMsg, ERRSTR_LEN, "%s errno %d, %s", X,dw, error_string);\
	}

#define SLOG(Z) { Amor::Pius log_pius; \
		log_pius.ordo = Notitia::LOG_##Z; \
		log_pius.indic = &errMsg[0]; \
		aptus->sponte(&log_pius); }

int squeeze(const char *p, unsigned char *q)	//把空格等挤掉, 只留下16进制字符(大写), 返回实际的长度
{
	int i;
	i = 0;
	while ( *p ) { 
		if ( isxdigit(*p) ) 
		{
			if (q) q[i] = toupper(*p);
			i++;
		} else if ( !isspace(*p)) 
		{
			if (q) q[i] = *p;
			i++;
		}
		p++;
	}
	if (q) q[i] = '\0';
	return i;
};

class WinComm: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	WinComm();
	~WinComm();

	enum tagParityType {	
		uPARITY_NONE	= 0,
		uPARITY_ODD	= 1,
		uPARITY_EVEN	= 2,
		uPARITY_MARK	= 3,
		uPARITY_SPACE	= 4
	} ;

	enum tagByteSize {	
		uBYTE_SIZE_4	= 4,
		uBYTE_SIZE_5	= 5,
		uBYTE_SIZE_6	= 6,
		uBYTE_SIZE_7	= 7,
		uBYTE_SIZE_8	= 8
	} ;

	enum tagStopBits {
		uSTOP_BITS_1	= 0,
		uSTOP_BITS_15	= 1,
		uSTOP_BITS_2	= 2
	} ;

	enum tagBaudRate {	
		uBAUD_75	= 75,
		uBAUD_110	= 110,
		uBAUD_134	= 134,
		uBAUD_150	= 150,
		uBAUD_300	= 300,
		uBAUD_600	= 600,
		uBAUD_1200	= 1200,
		uBAUD_1800	= 1800,
		uBAUD_2400	= 2400,
		uBAUD_3600	= 3600,
		uBAUD_4800	= 4800,
		uBAUD_7200	= 7200,
		uBAUD_9600	= 9600,
		uBAUD_14K	= 14400,
		uBAUD_19K	= 19200,
		uBAUD_38K	= 38400,
		uBAUD_56K	= 56000,
		uBAUD_115K	= 115200,
		uBAUD_128K	= 128000,
		uBAUD_230K	= 230400,
		uBAUD_460K	= 460800,
		uBAUD_921K	= 921600
	} ;

private:
	bool isCli;
	DPoll::Pollor pollor; /* 保存事件句柄, 各子实例不同 */
	Amor::Pius epl_set_ps, epl_clr_ps, pro_tbuf_ps;

	char errMsg[ERRSTR_LEN];

	OVERLAPPED ovlpW, ovlpR;
	HANDLE hdev;		/* 串口访问文件句柄 */
	char comm_name[128];
	DWORD baud_rate;       /* Baudrate at which running       */
	BYTE data_size;        /* Number of bits/byte, 4-8        */
	BYTE parity;          /* 0-4=None,Odd,Even,Mark,Space    */
	BYTE stop_bits;        /* 0,1,2 = 1, 1.5, 2               */

	void open_comm();
	void close_comm();

	DWORD checkBaud( int baud )
	{
		if (baud<=75)
			return(uBAUD_75);

		else if (baud<=110)
			return(uBAUD_110);

		else if (baud<=134)
			return(uBAUD_134);

		else if (baud<=150)
			return(uBAUD_150);

		else if (baud<=300)
			return(uBAUD_300);

		else if (baud<=600)
			return(uBAUD_600);

		else if (baud<=1200)
			return(uBAUD_1200);

		else if (baud<=1800)
			return(uBAUD_1800);

		else if (baud<=2400)
			return(uBAUD_2400);

		else if (baud<=3600)
		 	return(uBAUD_3600);

		else if (baud<=4800)
		 	return(uBAUD_4800);

		else if (baud<=7200)
		 	return(uBAUD_7200);

		else if (baud<=9600)
			return(uBAUD_9600);

		else if (baud<=14400)
			return(uBAUD_14K);
		
		else if (baud<=19200)
			return(uBAUD_19K);

		else if (baud<=38400)
			return(uBAUD_38K);

		else if (baud<=57600)
			return(uBAUD_56K);

		else if (baud<=115200)
			return(uBAUD_115K);

		else if (baud<=128000)
			return(uBAUD_128K);

		else if (baud <= 230400) 
			return (uBAUD_230K);

		else if (baud <= 460800) 
			return (uBAUD_460K);

		else if (baud <= 921600)
			return(uBAUD_921K);

		return(baud);
	};

	void get_prop(TiXmlElement *cfg) 
	{
		const char *comm_str;
		int baud_f, size_f;

		comm_str = cfg->Attribute("tty");
		if ( comm_str ) 
			TEXTUS_SPRINTF(comm_name, comm_str);

		comm_str = cfg->Attribute("parity");
		if ( strcasecmp(comm_str, "odd" ) == 0 )
			parity = uPARITY_ODD ;
		else if ( strcasecmp(comm_str, "even" ) == 0 )
			parity = uPARITY_EVEN ;
		else if ( strcasecmp(comm_str, "none" ) == 0 )
			parity = uPARITY_NONE;
		else if ( strcasecmp(comm_str, "space" ) == 0 )
			parity = uPARITY_SPACE;
		else if ( strcasecmp(comm_str, "mark" ) == 0 )
			parity = uPARITY_MARK;

		cfg->QueryIntAttribute("baud", &(baud_f));	
		baud_rate = checkBaud(baud_f);

		cfg->QueryIntAttribute("size", &(size_f));	
		if ( size_f == 8 )
			data_size = uBYTE_SIZE_8;
		else if ( size_f == 7 )
			data_size = uBYTE_SIZE_7;
		else if ( size_f == 6 )
			data_size = uBYTE_SIZE_6;
		else if ( size_f == 5 )
			data_size = uBYTE_SIZE_5;
			
		comm_str = cfg->Attribute("stop_bit");
		if ( strcasecmp(comm_str, "1" ) == 0 )
			stop_bits = uSTOP_BITS_1;
		else if ( strcasecmp(comm_str, "1.5" ) == 0 )
			stop_bits = uSTOP_BITS_15;
		else if ( strcasecmp(comm_str, "2" ) == 0 )
			stop_bits = uSTOP_BITS_2;
	};

	struct G_CFG {
		bool on_start;
		Amor *sch;
		struct DPoll::PollorBase lor; /* 探询 */
		unsigned char start_seq[1024];
		unsigned int seq_len;
		inline G_CFG(TiXmlElement *cfg) {
			const char *comm_str;
			sch = 0;
			lor.type = DPoll::NotUsed;
			on_start = true;
			if ( (comm_str = cfg->Attribute("start") ) && strcasecmp(comm_str, "no") ==0 )
                		on_start = false; /* 并非一开始就启动 */
			seq_len = 0;
			if ( (comm_str = cfg->Attribute("start_seq") ) )
			{
				seq_len = BTool::unescape(comm_str, start_seq) ;
			}
		};
	};
	struct G_CFG *gCFG;
	bool has_config;

	TBuffer *rcv_buf, *snd_buf;
	TBuffer *m_rcv_buf, *m_snd_buf;

	inline void transmitto_ex();
	inline void recito_ex();
	inline void deliver(Notitia::HERE_ORDO aordo);

#include "wlog.h"
};

void WinComm::close_comm()
{
	WBUG("close_comm(%s).....", comm_name);
	if ( !CloseHandle(hdev) )
	{
		ERROR_PRO("CloseHandle")
		SLOG(ERR)
	}
	hdev = INVALID_HANDLE_VALUE;
	deliver(Notitia::END_SESSION);/* 向左、右传递本类的会话关闭信号 */
}

void WinComm::open_comm()
{
	DCB dcb;
	COMMPROP prop;
	char msg[128];

	if ( this->hdev  != INVALID_HANDLE_VALUE ) return;
	memset(&dcb,0,sizeof(dcb));

	this->hdev = CreateFile(this->comm_name, GENERIC_READ|GENERIC_WRITE, 0, NULL, 
		OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH | FILE_FLAG_OVERLAPPED, NULL);

	if (hdev == INVALID_HANDLE_VALUE)
	{
		sprintf_s(msg, "CreateFile(%s)",this->comm_name);
		ERROR_PRO(msg)
		SLOG(EMERG);
		return ;
	}

	//the following get commport properties into lpprop
	if (GetCommProperties (hdev, &prop) == 0)
	{
		ERROR_PRO("GetCommProperties")
		SLOG(EMERG);
		return ;	
	}

	//the following get and set DCB
	if (GetCommState(hdev, &dcb) == 0)
	{
		ERROR_PRO("GetCommState")
		SLOG(EMERG);
		return ;	
	}

	if(prop.dwSettableParams & SP_BAUD)
	{
		dcb.BaudRate = this->baud_rate ;
	} else {
		ERROR_PRO("不能设置波特率")
		SLOG(EMERG);
		return ;	
	}

	if(prop.dwSettableParams & SP_PARITY)
	{
		dcb.Parity = this->parity;
	} else {
		ERROR_PRO("不能设置校验方式")
		SLOG(EMERG);
		return ;	
	}

	if(prop.dwSettableParams & SP_STOPBITS)
	{
		dcb.StopBits = this->stop_bits ;
	} else {
		ERROR_PRO("不能设置停止位")
		SLOG(EMERG);
		return ;	
	}

	if(prop.dwSettableParams & SP_DATABITS)
	{
		dcb.ByteSize = this->data_size ;
	} else {
		ERROR_PRO("不能设置字节尺寸")
		SLOG(EMERG);
		return ;	
	}

	if (SetCommState(hdev,&dcb) == 0)
	{
		ERROR_PRO("SetCommState")
		SLOG(EMERG);
		return ;	
	}

	if ( !SetCommMask(hdev,EV_RXCHAR | EV_ERR ) )
//	if ( !SetCommMask(hdev,EV_RXCHAR | EV_BREAK |EV_ERR |EV_RING |EV_TXEMPTY) )
	{
		ERROR_PRO("SetCommMask")
		SLOG(EMERG);
		return ;	
	}

	if (ClearCommBreak(hdev) == 0)
	{
		ERROR_PRO("ClearCommBreak")
		SLOG(EMERG);
		return ;		
	}
	pollor.hnd.file = hdev;
	gCFG->sch->sponte(&epl_set_ps);	//向tpoll
	/* 接收(发送)缓冲区清空 */
	if ( rcv_buf) rcv_buf->reset();	
	if ( snd_buf) snd_buf->reset();
	deliver(Notitia::START_SESSION); //向接力者发出通知, 本对象开始
	recito_ex();
}

void WinComm::ignite(TiXmlElement *cfg)
{
	if ( !gCFG) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
	get_prop(cfg);
}

bool WinComm::facio( Amor::Pius *pius)
{
	OVERLAPPED_ENTRY *aget;
	TBuffer **tb;
	Amor::Pius tmp_pius;
	assert(pius);
	switch (pius->ordo)
	{
	case Notitia::PRO_TBUF :
		WBUG("facio PRO_TBUF");
		transmitto_ex();
/*
		if ( hdev == INVALID_HANDLE_VALUE )
		{
			Amor::Pius info_pius;
			info_pius.ordo = Notitia::CHANNEL_NOT_ALIVE;
			info_pius.indic = 0;
			aptus->sponte(&info_pius);
		} else {
			transmitto_ex();
		}
*/
		break;

	case Notitia::SET_TBUF:	/* 取得输入TBuffer地址 */
		WBUG("facio SET_TBUF");
		isCli = true;
		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//当然tb不能为空
			if ( *tb) 
			{	//新到请求的TBuffer
				snd_buf = *tb;
			}
			tb++;
			if ( *tb) rcv_buf = *tb;
		} else 
			WLOG(NOTICE,"facio PRO_TBUF null.");
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		close_comm();
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		deliver(Notitia::SET_TBUF);
		tmp_pius.ordo = Notitia::CMD_GET_SCHED;
		aptus->sponte(&tmp_pius);	//向tpoll, 取得sched
		gCFG->sch = (Amor*)tmp_pius.indic;
		if ( !gCFG->sch ) 
		{
			WLOG(ERR, "no sched or tpoll");
			break;
		}
		tmp_pius.ordo = Notitia::POST_EPOLL;
		tmp_pius.indic = &gCFG->lor;
		gCFG->lor.pupa = this;
		
		gCFG->sch->sponte(&tmp_pius);	//向tpoll, 取得TPOLL
		if ( tmp_pius.indic != gCFG->sch ) break;

		if ( gCFG->on_start )
			open_comm();
		if ( gCFG->seq_len > 0 ) {
/*
		 unsigned char snd1[30] = { 
			0x02, 0x00, 0x23 ,0x34 ,0x77 ,0x03 ,0x99 ,0x56 ,0x02 ,0x00 ,0x40 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
			0x14 ,0x99 ,0x77 ,0x88 ,0x77 ,0x35 ,0x40 ,0x32 ,0x03 ,0xaf
			};
		snd_buf->input(snd1, 28);
*/
			snd_buf->input(gCFG->start_seq, gCFG->seq_len);
			transmitto_ex();
		}
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		deliver(Notitia::SET_TBUF);
		if ( gCFG->on_start )
			open_comm();
		break;

	case Notitia::DMD_START_SESSION:
		WBUG("facio DMD_START_SESSION");
		open_comm();		//开始建立连接
		break;

	case Notitia::CMD_CHANNEL_PAUSE :
		WBUG("facio CMD_CHANNEL_PAUSE");
		gCFG->sch->sponte(&epl_clr_ps); //向tpoll,  注销
		break;

	case Notitia::CMD_CHANNEL_RESUME :
		WBUG("sponte CMD_CHANNEL_RESUME");
		gCFG->sch->sponte(&epl_set_ps); //向tpoll,  注册
		break;

	case Notitia::PRO_EPOLL:
		WBUG("facio PRO_EPOLL");
		aget = (OVERLAPPED_ENTRY *)pius->indic;
		if ( aget->lpOverlapped == &ovlpR )
		{	//已读数据,  不失败并有数据才向接力者传递
			if ( aget->dwNumberOfBytesTransferred ==0 ) 
			{
				WLOG(INFO, "IOCP recv 0 disconnected");
				close_comm();
			} else {
				WBUG("PRO_EPOLL recv %d bytes", aget->dwNumberOfBytesTransferred);
				rcv_buf->commit(aget->dwNumberOfBytesTransferred);
				if ( isCli ) 
					aptus->sponte(&pro_tbuf_ps);
				else
					aptus->facio(&pro_tbuf_ps);
			}
		} else if ( aget->lpOverlapped != &ovlpW ) {
			WLOG(EMERG, "not my overlap");
			break;
		}
		recito_ex();
		break;

	default:
		return false;
	}	
	return true;
}

bool WinComm::sponte( Amor::Pius *pius) 
{ 
	TBuffer **tb;
	switch (pius->ordo)
	{
	case Notitia::PRO_TBUF :	//处理一帧数据而已
		WBUG("sponte PRO_TBUF");	
		if ( hdev == INVALID_HANDLE_VALUE )
		{
			Amor::Pius info_pius;
			info_pius.ordo = Notitia::CHANNEL_NOT_ALIVE;
			info_pius.indic = 0;
			aptus->facio(&info_pius);
		} else {
			transmitto_ex();
		}
		break;
		
	case Notitia::SET_TBUF:	/* 取得输入TBuffer地址 */
		WBUG("sponte SET_TBUF");
		isCli = false;
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

	case Notitia::DMD_END_SESSION:	//强制关闭，等同主动关闭，要通知别人
		WLOG(INFO,"DMD_END_SESSION, close %p", hdev);
		close_comm();
		break;

	case Notitia::CMD_CHANNEL_PAUSE :
		WBUG("sponte CMD_CHANNEL_PAUSE");
		gCFG->sch->sponte(&epl_clr_ps); //向tpoll,  注销
		break;

	case Notitia::CMD_CHANNEL_RESUME :
		WBUG("sponte CMD_CHANNEL_RESUME");
		gCFG->sch->sponte(&epl_set_ps); //向tpoll,  注册
		break;

	case Notitia::DMD_START_SESSION:
		WBUG("sponte DMD_START_SESSION");
		open_comm();		//打开通道
		break;

	default:
		return false;
	}	
	return true;
}

WinComm::WinComm()
{
	pollor.pupa = this;
	pollor.type = DPoll::File;
	pollor.pro_ps.ordo = Notitia::PRO_EPOLL;
	epl_set_ps.ordo = Notitia::SET_EPOLL;
	epl_set_ps.indic = &pollor;
	epl_clr_ps.ordo = Notitia::CLR_EPOLL;
	epl_clr_ps.indic = &pollor;
	pro_tbuf_ps.ordo = Notitia::PRO_TBUF;
	pro_tbuf_ps.indic = 0;

	memset(errMsg, 0, sizeof(errMsg));

	m_rcv_buf = new TBuffer(1024);
	m_snd_buf = new TBuffer(1024);
	rcv_buf = m_rcv_buf;
	snd_buf = m_snd_buf;
	gCFG = 0;
	has_config = false;
	memset(&ovlpR, 0, sizeof(OVERLAPPED));
	memset(&ovlpW, 0, sizeof(OVERLAPPED));
	isCli = false;
	hdev  = INVALID_HANDLE_VALUE;
}

WinComm::~WinComm()
{	
	close_comm();
	if (has_config )
		delete gCFG;
	delete m_rcv_buf;
	delete m_snd_buf;
}

void WinComm::transmitto_ex()
{
	DWORD snd_len = snd_buf->point - snd_buf->base;	//发送长度
	memset(&ovlpW, 0, sizeof(OVERLAPPED));
	if ( !WriteFile(hdev, snd_buf->base, snd_len, NULL, &ovlpW) )
	{
		if ( ERROR_IO_PENDING != GetLastError() ) {
			ERROR_PRO ("WriteFile");
			SLOG(EMERG)
			close_comm();
			return ;
		}
	}
	snd_buf->commit(-(long)snd_len);	//已经到了系统
}

void WinComm::recito_ex()
{
	rcv_buf->grant(RCV_FRAME_SIZE);
	memset(&ovlpR, 0, sizeof(OVERLAPPED));
	if (!ReadFile(hdev, rcv_buf->point, RCV_FRAME_SIZE, NULL, &ovlpR) )
	{
		if ( ERROR_IO_PENDING != GetLastError() ) {
			ERROR_PRO ("ReadFile");
			SLOG(EMERG)
			close_comm();
		}
	} 
}

Amor* WinComm::clone()
{
	WinComm *child;
	child = new WinComm();
	child->gCFG = gCFG;
	return (Amor*)child;
}

/* 向接力者提交 */
inline void WinComm::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	TBuffer *tb[3];
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;

	switch (aordo )
	{
	case Notitia::SET_TBUF:
		WBUG("deliver SET_TBUF");
		tb[0] = rcv_buf;
		tb[1] = snd_buf;
		tb[2] = 0;
		tmp_pius.indic = &tb[0];
		break;

	case Notitia::END_SESSION:
		WBUG("deliver END_SESSION");
		if ( isCli ) {
			aptus->sponte(&tmp_pius);
			return;
		}
		break;

	case Notitia::START_SESSION:
		WBUG("deliver START_SESSION");
		if ( isCli ) {
			aptus->sponte(&tmp_pius);
			return;
		}
		break;

	default:
		break;
	}
	aptus->facio(&tmp_pius);
}
#include "hook.c"
