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

class WinComm: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	WinComm();
	~WinComm();

private:
	bool isCli;
	DPoll::Pollor pollor; /* 保存事件句柄, 各子实例不同 */
	Amor::Pius epl_set_ps, epl_clr_ps, pro_tbuf_ps;
	Amor::Pius tmp_pius;

	OVERLAPPED ovlpW, ovlpR, ovlpE;
	HANDLE hdev, evt_hnd;	/* 串口访问文件句柄 */
	char comm_name[128];
	DWORD baud_rate;       /* Baudrate at which running       */
	BYTE data_size;        /* Number of bits/byte, 4-8        */
	BYTE parity;          /* 0-4=None,Odd,Even,Mark,Space    */
	BYTE stop_bits;        /* 0,1,2 = 1, 1.5, 2               */
	DWORD evtMask;
	bool has_evt;

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
		DWORD dwEvtMask, dwPurgeFlag, dwInQueue, dwOutQueue ;
		char action[256];
		DCB dcb;
		COMMTIMEOUTS tmo;
		bool has_dtr, has_rts, has_xon_xoff, has_out_cts, has_out_dsr, has_in_dsr;
		bool has_err_char, has_eof_char, has_evt_char, has_timeout, has_buffer ;
		inline G_CFG(TiXmlElement *cfg) {
			const char *comm_str;
			char n_str[256];
			TiXmlElement *ele;
			size_t i;
			int val;

			memset(&dcb, 0, sizeof(DCB));
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
			dwEvtMask = 0;
			ele =  cfg->FirstChildElement("event_mask") ;
			if ( ele && (comm_str = ele->GetText() ) )
			{
				for ( i = 0 ; i < strlen(comm_str); i++)
					n_str[i]= toupper(comm_str[i]);
				n_str[i] = 0;
				if ( strstr(n_str, "BREAK") ) dwEvtMask |= EV_BREAK;
				if ( strstr(n_str, "CTS") ) dwEvtMask |= EV_CTS;
				if ( strstr(n_str, "DSR") ) dwEvtMask |= EV_DSR;
				if ( strstr(n_str, "ERR") ) dwEvtMask |= EV_ERR;
				if ( strstr(n_str, "RING") ) dwEvtMask |= EV_RING;
				if ( strstr(n_str, "RLSD") ) dwEvtMask |= EV_RLSD;
				if ( strstr(n_str, "RXCHAR") ) dwEvtMask |= EV_RXCHAR;
				if ( strstr(n_str, "RXFLAG") ) dwEvtMask |= EV_RXFLAG;
				if ( strstr(n_str, "TXEMPTY") ) dwEvtMask |= EV_TXEMPTY;
			}
			dwPurgeFlag = 0;
			ele =  cfg->FirstChildElement("purge") ;
			if ( ele && (comm_str = ele->GetText() ) )
			{
				for ( i = 0 ; i < strlen(comm_str); i++)
					n_str[i]= tolower(comm_str[i]);
				n_str[i] = 0;
				if ( strstr(n_str, "rx_abort") ) dwPurgeFlag |= PURGE_RXABORT;
				if ( strstr(n_str, "rx_clear") ) dwPurgeFlag |= PURGE_RXCLEAR;
				if ( strstr(n_str, "tx_abort") ) dwPurgeFlag |= PURGE_TXABORT;
				if ( strstr(n_str, "tx_clear") ) dwPurgeFlag |= PURGE_TXCLEAR;
			}
			
			action[0] = 0;
			ele =  cfg->FirstChildElement("action") ;
			if ( ele && (comm_str = ele->GetText() ) )
			{
				for ( i = 0 ; i < strlen(comm_str); i++)
					action[i]= tolower(comm_str[i]);
				action[i] = 0;
			}
			has_buffer = false;
			ele =  cfg->FirstChildElement("buffer") ;
			if ( ele ) 
			{
				int val;
				dwInQueue =  dwOutQueue = 1;
				has_buffer = true;
				if ( TIXML_SUCCESS ==ele->QueryIntAttribute("in", &val))
					dwInQueue = (DWORD)val;
				if ( TIXML_SUCCESS == ele->QueryIntAttribute("out",&val))
					dwOutQueue = (DWORD)val;
			}
			has_timeout = false;
			ele =  cfg->FirstChildElement("timeout") ;
			if ( ele ) 
			{
				int val;
				has_timeout = true;
				tmo.ReadIntervalTimeout=-1;
				tmo.ReadTotalTimeoutConstant=-1;
				tmo.ReadTotalTimeoutMultiplier=-1;
				tmo.WriteTotalTimeoutConstant=-1;
				tmo.WriteTotalTimeoutMultiplier=-1;
				if ( TIXML_SUCCESS == ele->QueryIntAttribute("ReadInterval", &val))
					tmo.ReadIntervalTimeout = (DWORD)val;
				if ( TIXML_SUCCESS == ele->QueryIntAttribute("ReadConstant", &val))
					tmo.ReadTotalTimeoutConstant= (DWORD)val;
				if ( TIXML_SUCCESS == ele->QueryIntAttribute("ReadMultiplier", &val))
					tmo.ReadTotalTimeoutMultiplier = (DWORD)val;
				if ( TIXML_SUCCESS == ele->QueryIntAttribute("WriteMultiplier", &val))
					tmo.WriteTotalTimeoutMultiplier = (DWORD)val;
				if ( TIXML_SUCCESS == ele->QueryIntAttribute("WriteConstant", &val))
					tmo.WriteTotalTimeoutConstant= (DWORD)val;
			}
			has_xon_xoff = false;
			ele =  cfg->FirstChildElement("Xon_Xoff") ;
			if ( ele )
			{
				has_xon_xoff = true;
				comm_str = ele->GetText();
				for ( i = 0 ; i < strlen(comm_str); i++)
					n_str[i]= tolower(comm_str[i]);
				n_str[i] = 0;
				if ( strstr(n_str, "tx_continue_on_xoff"))
					dcb.fTXContinueOnXoff = true;
				else
					dcb.fTXContinueOnXoff = false;
				if ( strstr(n_str, "out_xon_xoff"))
					dcb.fOutX = true;
				else
					dcb.fOutX = false;
				if ( strstr(n_str, "in_xon_xoff"))
					dcb.fInX = true;
				else
					dcb.fInX = false;
				BTool::unescape(ele->Attribute("xon"), (unsigned char*)n_str);
				dcb.XonChar = n_str[0];
				BTool::unescape(ele->Attribute("xoff"), (unsigned char*)n_str);
				dcb.XoffChar = n_str[0];
				ele->QueryIntAttribute("xon_lim", &val);
				dcb.XonLim = (WORD)val;
				ele->QueryIntAttribute("xoff_lim", &val);
				dcb.XoffLim = (WORD)val;
			}
			has_rts = false;
			ele =  cfg->FirstChildElement("RTS") ;
			if ( ele )
			{
				has_rts = true;
				comm_str = ele->GetText();
				for ( i = 0 ; i < strlen(comm_str); i++)
					n_str[i]= tolower(comm_str[i]);
				n_str[i] = 0;
				if ( strstr(n_str, "disable"))
					dcb.fRtsControl = RTS_CONTROL_DISABLE;
				if ( strstr(n_str, "enable"))
					dcb.fRtsControl = RTS_CONTROL_ENABLE;
				if ( strstr(n_str, "handshake"))
					dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
				if ( strstr(n_str, "toggle"))
					dcb.fRtsControl = RTS_CONTROL_TOGGLE;
			}
			has_dtr = false;
			ele =  cfg->FirstChildElement("DTR") ;
			if ( ele )
			{
				has_dtr = true;
				comm_str = ele->GetText();
				for ( i = 0 ; i < strlen(comm_str); i++)
					n_str[i]= tolower(comm_str[i]);
				n_str[i] = 0;
				if ( strstr(n_str, "disable"))
					dcb.fDtrControl = DTR_CONTROL_DISABLE;
				if ( strstr(n_str, "enable"))
					dcb.fDtrControl = DTR_CONTROL_ENABLE;
				if ( strstr(n_str, "handshake"))
					dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
			}
			has_out_dsr = false;
			has_out_cts = false;
			has_in_dsr = false;
			ele =  cfg->FirstChildElement("control") ;
			if ( ele )
			{
				has_dtr = true;
				comm_str = ele->GetText();
				for ( i = 0 ; i < strlen(comm_str); i++)
					n_str[i]= tolower(comm_str[i]);
				n_str[i] = 0;
				if ( strstr(n_str, "cts_enable"))
				{
					has_out_cts = true;
					dcb.fOutxCtsFlow = TRUE;
				}
				if ( strstr(n_str, "cts_disable"))
				{
					has_out_cts = true;
					dcb.fOutxCtsFlow = FALSE;
				}
				if ( strstr(n_str, "out_dsr_disable"))
				{
					has_out_dsr = true;
					dcb.fOutxDsrFlow = FALSE;
				}
				if ( strstr(n_str, "out_dsr_enable"))
				{
					has_out_dsr = true;
					dcb.fOutxDsrFlow = TRUE;
				}
				if ( strstr(n_str, "in_dsr_disable"))
				{
					has_in_dsr = true;
					dcb.fDsrSensitivity = FALSE; 
				}
				if ( strstr(n_str, "in_dsr_enable"))
				{
					has_in_dsr = true;
					dcb.fDsrSensitivity = TRUE; 
				}
				comm_str = ele->Attribute("parity_error_char");
				if ( comm_str ) 
				{
					has_err_char = true;
					BTool::unescape(comm_str, (unsigned char*)n_str);
					dcb.fErrorChar = TRUE;
					dcb.ErrorChar = n_str[0];
				}
				comm_str = ele->Attribute("event_char");
				if ( comm_str ) 
				{
					has_evt_char = true;
					BTool::unescape(comm_str, (unsigned char*)n_str);
					dcb.EvtChar = n_str[0];
				}
				comm_str = ele->Attribute("EOF_char");
				if ( comm_str ) 
				{
					has_eof_char = true;
					BTool::unescape(comm_str, (unsigned char*)n_str);
					dcb.fErrorChar = TRUE;
					dcb.EofChar = n_str[0];
				}
			}
		};
	};
	struct G_CFG *gCFG;
	bool has_config;

	TBuffer *rcv_buf, *snd_buf;
	TBuffer *m_rcv_buf, *m_snd_buf;
	TBuffer *tb_arr[3];

	inline void transmitto_ex();
	inline void recito_ex();
	void wait_evt();
	void pro_evt();
	void evt_deliver(Pius *);
	inline void deliver(Notitia::HERE_ORDO aordo);

#include "wlog.h"
};

void WinComm::close_comm()
{
	WBUG("close_comm(%s).....", comm_name);
	if ( !CloseHandle(hdev) )
	{
		WLOG_OSERR("CloseHandle");
	}
	hdev = INVALID_HANDLE_VALUE;
	deliver(Notitia::END_SESSION);/* 向左、右传递本类的会话关闭信号 */
}

void WinComm::open_comm()
{
	DCB dcb;
	COMMPROP prop;
	COMMTIMEOUTS tmo;
	char msg[128];

	if ( this->hdev  != INVALID_HANDLE_VALUE ) return;
	memset(&dcb,0,sizeof(dcb));

	this->hdev = CreateFile(this->comm_name, GENERIC_READ|GENERIC_WRITE, 0, NULL, 
//				OPEN_EXISTING, 0, NULL);
//		OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH, NULL);
		OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
//		OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH | FILE_FLAG_OVERLAPPED, NULL);

	if (hdev == INVALID_HANDLE_VALUE)
	{
		sprintf_s(msg, "CreateFile(%s)",this->comm_name);
		WLOG_OSERR(msg);
		return ;
	}
	if ( gCFG->dwEvtMask )
	{
		evt_hnd = CreateEvent(	NULL, TRUE, FALSE, NULL);
		if (evt_hnd == INVALID_HANDLE_VALUE)
		{
			WLOG_OSERR("CreateEvent");
			return ;
		}
	}
	//the following get commport properties into lpprop
	if (GetCommProperties (hdev, &prop) == 0)
	{
		WLOG_OSERR("GetCommProperties");
		return ;	
	}

	//the following get and set DCB
	if (GetCommState(hdev, &dcb) == 0)
	{
		WLOG_OSERR("GetCommState");
		return ;	
	}

	if(prop.dwSettableParams & SP_BAUD)
	{
		dcb.BaudRate = this->baud_rate ;
	} else {
		WLOG(ERR,"不能设置波特率");
		return ;	
	}

	if(prop.dwSettableParams & SP_PARITY)
	{
		dcb.Parity = this->parity;
	} else {
		WLOG(ERR,"不能设置校验方式");
		return ;	
	}

	if(prop.dwSettableParams & SP_STOPBITS)
	{
		dcb.StopBits = this->stop_bits ;
	} else {
		WLOG(ERR,"不能设置停止位");
		return ;	
	}

	if(prop.dwSettableParams & SP_DATABITS)
	{
		dcb.ByteSize = this->data_size ;
	} else {
		WLOG(ERR,"不能设置字节尺寸");
		return ;	
	}
	dcb.fBinary = TRUE;
	if ( gCFG->has_dtr)
	{
		dcb.fDtrControl = gCFG->dcb.fDtrControl;
	}
	if ( gCFG->has_out_dsr)
	{
		dcb.fOutxDsrFlow = gCFG->dcb.fOutxDsrFlow;
	}
	if ( gCFG->has_out_cts)
	{
		dcb.fOutxCtsFlow = gCFG->dcb.fOutxCtsFlow;
	}
	if ( gCFG->has_in_dsr)
	{
		dcb.fDsrSensitivity = gCFG->dcb.fDsrSensitivity;
	}
	if ( gCFG->has_err_char)
	{
		dcb.fErrorChar = gCFG->dcb.fErrorChar;
		dcb.ErrorChar = gCFG->dcb.ErrorChar;
	}
	if ( gCFG->has_evt_char)
	{
		dcb.EvtChar = gCFG->dcb.EvtChar;
	}
	if ( gCFG->has_eof_char)
	{
		dcb.EofChar = gCFG->dcb.EofChar;
	}

	if ( gCFG->has_rts)
	{
		dcb.fRtsControl = gCFG->dcb.fRtsControl;
	}

	if ( !strstr(gCFG->action, "no_set_state"))
	if ( !SetCommState(hdev,&dcb) )
	{
		WLOG_OSERR("SetCommState");
		return ;	
	}

	if ( gCFG->dwEvtMask )
	if ( !SetCommMask(hdev, gCFG->dwEvtMask ) )
	{
		WLOG_OSERR("SetCommMask");
		return ;	
	}

	if ( strstr(gCFG->action, "clear_break"))
	if ( !ClearCommBreak(hdev) )
	{
		WLOG_OSERR("SetCommBreak");
		return ;		
	}

	if ( strstr(gCFG->action, "set_break"))
	if ( !SetCommBreak(hdev) )
	{
		WLOG_OSERR("ClearCommBreak");
		return ;		
	}

	if ( gCFG->has_buffer)
	if ( !SetupComm(hdev, gCFG->dwInQueue, gCFG->dwOutQueue) )
	{
		WLOG_OSERR("SetupComm");
		return ;		
	}

	if ( gCFG->has_timeout)
	{
		if ( !GetCommTimeouts(hdev, &tmo) )
		{
			WLOG_OSERR("GetCommTimeouts");
			return ;		
		}
		if ( gCFG->tmo.ReadIntervalTimeout !=  -1 )
			tmo.ReadIntervalTimeout =  gCFG->tmo.ReadIntervalTimeout;
		if ( gCFG->tmo.ReadTotalTimeoutConstant!=  -1 )
			tmo.ReadTotalTimeoutConstant =  gCFG->tmo.ReadTotalTimeoutConstant ;
		if ( gCFG->tmo.ReadTotalTimeoutMultiplier!=  -1 )
			tmo.ReadTotalTimeoutMultiplier =  gCFG->tmo.ReadTotalTimeoutMultiplier ;
		if ( gCFG->tmo.WriteTotalTimeoutConstant!=  -1 )
			tmo.WriteTotalTimeoutConstant =  gCFG->tmo.WriteTotalTimeoutConstant ;
		if ( gCFG->tmo.WriteTotalTimeoutMultiplier!=  -1 )
			tmo.WriteTotalTimeoutMultiplier =  gCFG->tmo.WriteTotalTimeoutMultiplier ;
		if ( !SetCommTimeouts(hdev, &tmo) )
		{
			WLOG_OSERR("SetCommTimeouts");
			return ;		
		}
	}

	if ( gCFG->dwPurgeFlag)
	if ( !PurgeComm(hdev, gCFG->dwPurgeFlag))
	{
		WLOG_OSERR("PurgeComm");
		return ;
	}

	pollor.hnd.file = hdev;
	gCFG->sch->sponte(&epl_set_ps);	//向tpoll
	/* 接收(发送)缓冲区清空 */
	if ( rcv_buf) rcv_buf->reset();	
	if ( snd_buf) snd_buf->reset();
	deliver(Notitia::START_SESSION); //向接力者发出通知, 本对象开始
	recito_ex();
	wait_evt();
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
	assert(pius);
	switch (pius->ordo)
	{
	case Notitia::PRO_TBUF :
		WBUG("facio PRO_TBUF");
		transmitto_ex();
		break;

	case Notitia::SET_TBUF:	/* 取得输入TBuffer地址 */
		WBUG("facio SET_TBUF");
		isCli = true;
		{
		TBuffer **tbp;
		tbp = (TBuffer **)(pius->indic);
		if (tbp) 
		{	//当然tb不能为空
			if ( *tbp) 
			{	//新到请求的TBuffer
				snd_buf = *tbp;
			}
			tbp++;
			if ( *tbp) rcv_buf = *tbp;
		} else 
			WLOG(NOTICE,"facio PRO_TBUF null.");
		}
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

	case Notitia::ERR_EPOLL:
		WBUG("facio ERR_EPOLL");
		WLOG(WARNING, (char*)pius->indic);	
		close_comm();	//直接关闭就可.
		break;

	case Notitia::PRO_EPOLL:
		aget = (OVERLAPPED_ENTRY *)pius->indic;
		WBUG("facio PRO_EPOLL ovlp=%p (W=%p, R=%p)", aget->lpOverlapped, &ovlpW, &ovlpR);
		if ( aget->lpOverlapped == &ovlpR )
		{	//已读数据,  不失败并有数据才向接力者传递
			if ( aget->dwNumberOfBytesTransferred ==0 ) 
			{
				WLOG(INFO, "%s recv timeout", comm_name);
				recito_ex();
				tmp_pius.indic = 0;
				tmp_pius.ordo = Notitia::Comm_Recv_Timeout;
				evt_deliver(&tmp_pius);
			} else {
				WBUG("PRO_EPOLL recv %d bytes", aget->dwNumberOfBytesTransferred);
				rcv_buf->commit(aget->dwNumberOfBytesTransferred);
				if ( isCli ) 
					aptus->sponte(&pro_tbuf_ps);
				else
					aptus->facio(&pro_tbuf_ps);
				recito_ex();
			}
		} else if ( aget->lpOverlapped == &ovlpW ) {
			WBUG("epoll for write ok");	
		} else if ( aget->lpOverlapped == &ovlpE ) {
			WBUG("epoll for event ok");
			pro_evt();
			wait_evt();
		} else {
			WLOG(EMERG, "not my overlap");
		}
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
	pro_tbuf_ps.indic = &tb_arr[0];

	m_rcv_buf = new TBuffer(1024);
	m_snd_buf = new TBuffer(1024);
	rcv_buf = m_rcv_buf;
	snd_buf = m_snd_buf;
	gCFG = 0;
	has_config = false;
	memset(&ovlpR, 0, sizeof(OVERLAPPED));
	memset(&ovlpW, 0, sizeof(OVERLAPPED));
	memset(&ovlpE, 0, sizeof(OVERLAPPED));
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
//	printf("write ovlpW %p\n", &ovlpW);
	if ( !WriteFile(hdev, snd_buf->base, snd_len, NULL, &ovlpW) )
	{
		if ( ERROR_IO_PENDING != GetLastError() ) {
			WLOG_OSERR("WriteFile");
			close_comm();
			return ;
		}
	}
	snd_buf->commit(-(long)snd_len);	//已经到了系统
}

void WinComm::recito_ex()
{
	rcv_buf->grant(512);
	memset(&ovlpR, 0, sizeof(OVERLAPPED));
	//if (!ReadFile(hdev, rcv_buf->point, 1, &rd, NULL) )
	if (!ReadFile(hdev, rcv_buf->point, 1, NULL, &ovlpR) )
	{
		if ( ERROR_IO_PENDING != GetLastError() ) {
			WLOG_OSERR("ReadFile");
			close_comm();
		}
	} 
}

void WinComm::pro_evt()
{
	if ( evtMask && EV_BREAK)
	{
		tmp_pius.ordo = Notitia::Comm_Event_Break;
		tmp_pius.indic = 0;
		evt_deliver(&tmp_pius);
	} 
	if ( evtMask && EV_CTS)
	{
		tmp_pius.ordo = Notitia::Comm_Event_CTS;
		tmp_pius.indic = 0;
		evt_deliver(&tmp_pius);
	}
	if ( evtMask && EV_DSR)
	{
		tmp_pius.ordo = Notitia::Comm_Event_DSR;
		tmp_pius.indic = 0;
		evt_deliver(&tmp_pius);
	}
	if ( evtMask && EV_ERR)
	{
		DWORD err;
		COMSTAT st;
		if ( !ClearCommError(hdev, &err, &st))
		{
			WLOG_OSERR("ClearCommError");
			return;
		}
		tmp_pius.indic = (void*)&st;
		if ( err && CE_BREAK)
		{
			tmp_pius.ordo = Notitia::Comm_Err_Break;
			evt_deliver(&tmp_pius);
		}
		if ( err && CE_FRAME)
		{
			tmp_pius.ordo = Notitia::Comm_Err_Frame;
			evt_deliver(&tmp_pius);
		}
		if ( err && CE_OVERRUN)
		{
			tmp_pius.ordo = Notitia::Comm_Err_OverRun;
			evt_deliver(&tmp_pius);
		}
		if ( err && CE_RXOVER)
		{
			tmp_pius.ordo = Notitia::Comm_Err_RxOver;
			evt_deliver(&tmp_pius);
		}
		if ( err && CE_RXPARITY)
		{
			tmp_pius.ordo = Notitia::Comm_Err_RxParity;
			evt_deliver(&tmp_pius);
		}
	}
	if ( evtMask && EV_RING)
	{
		tmp_pius.ordo = Notitia::Comm_Event_Ring;
		tmp_pius.indic = 0;
		evt_deliver(&tmp_pius);
	}
	if ( evtMask && EV_RLSD)
	{
		tmp_pius.ordo = Notitia::Comm_Event_RLSD;
		tmp_pius.indic = 0;
		evt_deliver(&tmp_pius);
	}
	if ( evtMask && EV_RXCHAR)
	{
		WBUG("event RXCHAR");
		tmp_pius.ordo = Notitia::Comm_Event_RxChar;
		tmp_pius.indic = 0;
		evt_deliver(&tmp_pius);
	}
	if ( evtMask && EV_RXFLAG)
	{
		tmp_pius.ordo = Notitia::Comm_Event_RxFlag;
		tmp_pius.indic = 0;
		evt_deliver(&tmp_pius);
	}
	if ( evtMask && EV_TXEMPTY)
	{
		tmp_pius.ordo = Notitia::Comm_Event_TxEmpty;
		tmp_pius.indic = 0;
		evt_deliver(&tmp_pius);
	}
}

void WinComm::evt_deliver(Pius *ps)
{
	if ( isCli ) 
		aptus->sponte(ps);
	else
		aptus->facio(ps);
}

void WinComm::wait_evt()
{
	if ( !gCFG->dwEvtMask ) return;
	memset(&ovlpE, 0, sizeof(OVERLAPPED));
	ovlpE.hEvent = evt_hnd;
	ResetEvent(ovlpE.hEvent );
	evtMask = 0;
	if ( !WaitCommEvent(hdev, &evtMask, &ovlpE) )
	{
		if ( ERROR_IO_PENDING != GetLastError() ) {
			WLOG_OSERR("WaitCommEvent");
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
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;

	switch (aordo )
	{
	case Notitia::SET_TBUF:
		WBUG("deliver SET_TBUF");
		tb_arr[0] = rcv_buf;
		tb_arr[1] = snd_buf;
		tb_arr[2] = 0;
		tmp_pius.indic = &tb_arr[0];
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
