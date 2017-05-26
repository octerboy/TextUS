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
 Build: created by octerboy, 2007/10/24
 $Id$
*/
#define SCM_MODULE_ID	"$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "textus_string.h"
#include "Describo.h"
#include "casecmp.h"

#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#if !defined (_WIN32)
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#else
#include <io.h>
#endif

#ifndef TINLINE
#define TINLINE inline
#endif 

#define ERRSTR_LEN 1024
class TTY: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	TTY();
	~TTY();

private:
	Amor::Pius local_pius;
	Amor::Pius pro_tbuf;
	time_t last_failed_time;	//最近一次的失败时间,秒

	char errMsg[ERRSTR_LEN];

	struct G_CFG {
		unsigned int    parity, stop_bit, baud_rate, data_size;
		const char *ttyname;
		bool on_start;
		inline int checkBaud( int baud )
		{
			if (baud<=50)
				return(B50);

			else if (baud<=75)
				return(B75);

			else if (baud<=110)
				return(B110);

			else if (baud<=134)
				return(B134);

			else if (baud<=150)
				return(B150);

			else if (baud<=200)
				return(B200);

			else if (baud<=300)
				return(B300);

			else if (baud<=600)
				return(B600);

			else if (baud<=1200)
				return(B1200);

			else if (baud<=1800)
				return(B1800);

			else if (baud<=2400)
				return(B2400);

			else if (baud<=4800)
			 	return(B4800);

			else if (baud<=9600)
				return(B9600);

			else if (baud<=19200)
				return(B19200);

			else if (baud<=38400)
				return(B38400);

			else if (baud<=57600)
				return(B57600);

			else if (baud<=115200)
				return(B115200);

			else if (baud<=230400)
				return(B230400);

    			else if (baud <= 460800) 
				return (B460800);

			else if (baud <= 921600)
				return(B921600);

			return(B9600);
		}

		inline G_CFG(TiXmlElement *cfg) {
			const char *comm_str;
			int baud_f, size_f, stop_f;

			ttyname = 0;
			parity = 0 ;		/* None=0,Odd=1,Even=2  */
			data_size = CS8;	/* Data Bit = 8     */
    			stop_bit = 1;		/* Stop Bit = 1     */
			on_start = true;

			if ( !cfg) return;
			ttyname = cfg->Attribute("tty");
			comm_str = cfg->Attribute("parity");
			if ( strcasecmp(comm_str, "odd" ) == 0 )
				parity = 1 ;
			else if ( strcasecmp(comm_str, "even" ) == 0 )
				parity = 2 ;
			else if ( strcasecmp(comm_str, "none" ) == 0 )
				parity = 0;

			cfg->QueryIntAttribute("baud", &(baud_f));	
			baud_rate = checkBaud(baud_f);

			cfg->QueryIntAttribute("size", &(size_f));	
			if ( size_f == 8 )
				data_size = CS8;

			else if ( size_f == 7 )
				data_size = CS7;

			else if ( size_f == 6 )
				data_size = CS6;

			else if ( size_f == 5 )
				data_size = CS5;
				
			cfg->QueryIntAttribute("stop_bit", &(stop_f));	
			if ( stop_f == 2 || stop_f == 1 )
				stop_bit = stop_f;

			if ( (comm_str = cfg->Attribute("start") ) && strcasecmp(comm_str, "no") ==0 )
                		on_start = false; /* 并非一开始就启动 */
		};
	};
	struct G_CFG *gCFG;
	bool has_config;

    	int ttyfd;	/* 文件句柄 */
	Describo::Criptor mytor; /* 保存套接字, 各子实例不同 */
	
	struct  termio  ttyold, ttynew;
	TBuffer *rcv_buf;
	TBuffer *snd_buf;
	bool wr_blocked ; 	/* 最近一次写阻塞标志 */

	TINLINE void transmit();
	TINLINE bool recito();
	TINLINE void init();
	TINLINE bool set();
	bool setup_com();
	TINLINE void end();
	TINLINE void deliver(Notitia::HERE_ORDO aordo);

#include "wlog.h"
};

void TTY::ignite(TiXmlElement *cfg)
{
	if ( !gCFG) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
}

#define SLOG(Z) { Amor::Pius log_pius; \
		log_pius.ordo = Notitia::LOG_##Z; \
		log_pius.indic = &errMsg[0]; \
		aptus->sponte(&log_pius); \
		}

#if defined (_WIN32 )
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
#else
#define ERROR_PRO(X)  if ( errMsg ) \
		TEXTUS_SNPRINTF(errMsg, ERRSTR_LEN, "%s errno %d, %s.", X, errno, strerror(errno));
#endif

bool TTY::facio( Amor::Pius *pius)
{
	assert(pius);
	switch (pius->ordo)
	{
	case Notitia::FD_PRORD:
		WBUG("facio FD_PRORD");
		if ( recito() )
			aptus->facio(&pro_tbuf);
		break;

	case Notitia::FD_PROWR:
		WBUG("facio FD_PROWR");
		//写, 很少见, 除非系统很忙
		transmit();
		break;

	case Notitia::FD_PROEX:
		WBUG("facio FD_PROEX");
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		end();
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		deliver(Notitia::SET_TBUF);
		if ( gCFG->on_start )
			init();
/*
		{
		 unsigned char snd1[30] = { 
			0x02, 0x00, 0x23 ,0x34 ,0x77 ,0x03 ,0x99 ,0x56 ,0x02 ,0x00 ,0x40 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
			0x14 ,0x99 ,0x77 ,0x88 ,0x77 ,0x35 ,0x40 ,0x32 ,0x03 ,0xaf
			};
		 rcv_buf->input(snd1, 28);
		aptus->facio(&pro_tbuf);
		}
*/
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		deliver(Notitia::SET_TBUF);
		if ( gCFG->on_start )
			init();
		break;

	case Notitia::DMD_START_SESSION:
		WBUG("facio DMD_START_SESSION");
		init();		//开始建立连接
		break;

	case Notitia::CMD_GET_FD:	//给出套接字描述符
		WBUG("sponte CMD_GET_FD");		
		pius->indic = &(ttyfd);
		break;

	case Notitia::CMD_CHANNEL_PAUSE :
		WBUG("facio CMD_CHANNEL_PAUSE");
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

bool TTY::sponte( Amor::Pius *pius) 
{ 
	switch (pius->ordo)
	{
	case Notitia::PRO_TBUF :	//处理一帧数据而已
		WBUG("sponte PRO_TBUF");	
		if ( ttyfd <= 0 )
			init();

		transmit();
		break;
		
	case Notitia::CMD_GET_FD:	//给出套接字描述符
		WBUG("sponte CMD_GET_FD");		
		pius->indic = &(ttyfd);
		break;

	case Notitia::DMD_END_SESSION:	//强制关闭，等同主动关闭，要通知别人
		WLOG(INFO,"DMD_END_SESSION, close %d", ttyfd);
		end();
		break;

	case Notitia::CMD_CHANNEL_PAUSE :
		WBUG("sponte CMD_CHANNEL_PAUSE");
		deliver(Notitia::FD_CLRRD);
		break;

	case Notitia::CMD_CHANNEL_RESUME :
		WBUG("sponte CMD_CHANNEL_RESUME");
		deliver(Notitia::FD_SETRD);
		break;

	case Notitia::DMD_START_SESSION:
		WBUG("sponte DMD_START_SESSION");
		init();		//打开通道
		break;

	default:
		return false;
	}	
	return true;
}

TTY::TTY()
{
	mytor.pupa = this;
	local_pius.ordo = Notitia::TEXTUS_RESERVED;
	local_pius.indic = &mytor;

	pro_tbuf.ordo = Notitia::PRO_TBUF;
	pro_tbuf.indic = 0;

	last_failed_time = 0;
	memset(errMsg, 0, sizeof(errMsg));

	rcv_buf = new TBuffer(1024);
	snd_buf = new TBuffer(1024);
	gCFG = 0;
	has_config = false;
	ttyfd = -1;
	wr_blocked = false; 	//刚开始, 最近一次写当然不阻塞
}

TTY::~TTY()
{	
	end();
	if (has_config )
		delete gCFG;
	delete rcv_buf;
	delete snd_buf;
}

TINLINE bool TTY::recito()
{
	int len;

	rcv_buf->grant(512);	//保证有足够空间
ReadAgain:
	if( (len = read(ttyfd, (char *)rcv_buf->point, 512)) == 0) /* (char*) for WIN32 */
	{	//对方关闭套接字
		TEXTUS_SNPRINTF(errMsg, ERRSTR_LEN, "recv 0, disconnected");
		SLOG(INFO)
		end();
		return false;
	} else if ( len == -1 )
	{ 
#if defined (_WIN32 )
		DWORD error;	
		error = GetLastError();
#else
		int error = errno;
#endif 
		if (error == EINTR)
		{	 //有信号而已,再试
			goto ReadAgain;
		} else if ( error == EAGAIN || error == EWOULDBLOCK )
		{	//还在进行中, 回去再等.
			ERROR_PRO("read encounter EAGAIN")
			SLOG(NOTICE)
			return false;
		} else	
		{	//的确有错误
			ERROR_PRO("read")
			SLOG(ERR)
			return false;
		}
	} 

	rcv_buf->commit(len);	/* 指针向后移 */
	return true;
}

TINLINE void TTY::transmit()
{
	int len;
	int snd_len = snd_buf->point - snd_buf->base;	//发送长度

	if ( ttyfd <= 0 )
		return;
SendAgain:
	/*
	for ( int i = 0 ; i < snd_buf->point - snd_buf->base; i++ )
		printf("%02x ", snd_buf->base[i]);
	printf("\n");
	*/
	len = write(ttyfd, (char *)snd_buf->base, snd_len); /* (char*) for WIN32 */
	if( len == -1 )
	{ 
#if defined (_WIN32 )
		DWORD error;	
		error = GetLastError();
#else
		int error = errno;
#endif 
		if (error == EINTR)	
		{	//有信号而已,再试
			goto SendAgain;

		} else if (error == EWOULDBLOCK || error == EAGAIN)
		{	//回去再试, 用select, 要设wrSet
			if ( wr_blocked ) 	//最近一次阻塞
			{
				return ;
			} else {	//刚发生的阻塞
				ERROR_PRO("write encounter EAGAIN")
				SLOG(NOTICE)
				wr_blocked = true;
				//向Sched, 以设置wrSet.
				local_pius.ordo =Notitia::FD_SETWR;
				aptus->sponte(&local_pius);
			}
		} else {
			ERROR_PRO("write")
			SLOG(ERR);
			end();
		}
	}

	snd_buf->commit(-len);	//提交所读出的数据
	if (snd_len > len )
	{	
		if ( wr_blocked ) 	//最近一次还是阻塞
		{
			return ;
		} else {	//刚发生的阻塞
			TEXTUS_SNPRINTF(errMsg, ERRSTR_LEN, "sending not completed.");
			SLOG(NOTICE)
			wr_blocked = true;
			//向Sched, 以设置wrSet.
			local_pius.ordo =Notitia::FD_SETWR;
			aptus->sponte(&local_pius);
		}
	} else 
	{	//不发生阻塞了
		if ( wr_blocked ) 	//最近一次阻塞
		{
			wr_blocked = false;
			local_pius.ordo =Notitia::FD_CLRWR;
			//向Sched, 以设置wrSet.
			aptus->sponte(&local_pius);	
		} else
		{	//一直没有阻塞
			return ; //发送完成
		}
	}

	return ;
}

TINLINE void TTY::end()
{
	WBUG("end().....");
	if ( ttyfd == -1 ) 
		return;	/* 不重复关闭 */
	deliver(Notitia::FD_CLRWR);
	deliver(Notitia::FD_CLREX);
	deliver(Notitia::FD_CLRRD);
	
	ioctl(ttyfd,TCSETAF,&ttyold);	/* 复原设置 */
	close(ttyfd);
	ttyfd = -1;

	deliver(Notitia::END_SESSION);/* 向左、右传递本类的会话关闭信号 */
}

Amor* TTY::clone()
{
	TTY *child;
	child = new TTY();
	child->gCFG = gCFG;
	return (Amor*)child;
}

/* 向接力者提交 */
TINLINE void TTY::deliver(Notitia::HERE_ORDO aordo)
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
		break;

	case Notitia::START_SESSION:
		WBUG("deliver START_SESSION");
		break;

	case Notitia::FD_CLRRD:
	case Notitia::FD_CLRWR:
	case Notitia::FD_CLREX:
	case Notitia::FD_SETRD:
	case Notitia::FD_SETWR:
	case Notitia::FD_SETEX:
		local_pius.ordo =aordo;
		aptus->sponte(&local_pius);	//向Sched
		return ;

	default:
		break;
	}
	aptus->facio(&tmp_pius);
}

void TTY::init()
{
	ttyfd=open(gCFG->ttyname,O_RDWR);
	if ( ttyfd < 0 )
	{
		ERROR_PRO("open tty");
		SLOG(ERR);
		return;
	}

    	if( !setup_com() )
	{
		ERROR_PRO("set tty");
		SLOG(ERR);
        	close(ttyfd);
		ttyfd = -1;
		return;
	}

	mytor.scanfd = ttyfd;
	deliver(Notitia::FD_SETRD);
	deliver(Notitia::FD_CLRWR);
	deliver(Notitia::FD_CLREX);

	/* 接收(发送)缓冲区清空 */
	if ( rcv_buf) rcv_buf->reset();	
	if ( snd_buf) snd_buf->reset();
	deliver(Notitia::START_SESSION); //向接力者发出通知, 本对象开始
}

bool TTY::set()
{
	if ( ioctl(ttyfd,TCGETA,&ttyold) == -1 )
        	return false;

	if ( ioctl(ttyfd,TCGETA,&ttynew) == -1 )
		return false;

	ttynew.c_lflag &= ~ICANON ;

	ttynew.c_cc[VMIN] = 1 ;
	ttynew.c_cc[VTIME] = 0 ;

	ttynew.c_iflag = 0 ;
	ttynew.c_oflag = 0 ;

	ttynew.c_cflag = (gCFG->baud_rate | gCFG->data_size | CREAD) ;

	if ( gCFG->parity == 1)
	{
		ttynew.c_cflag &= ~PARENB ;
		ttynew.c_cflag |= PARODD ;

	} else if ( gCFG->parity == 2) 
	{
		ttynew.c_cflag |= PARENB ;
		ttynew.c_cflag &= ~PARODD ;

        } else if ( gCFG->parity == 0)
	{
                ttynew.c_cflag &= ~PARENB ;
                ttynew.c_cflag &= ~PARODD ;
	}

	if (gCFG->stop_bit == 2) 
		ttynew.c_cflag |= CSTOPB ;
	else
		ttynew.c_cflag &= ~CSTOPB ;

	ttynew.c_line=0;    /* line discipline */

	if ( ioctl(ttyfd,TCSETAF,&ttynew) == -1 )
		return false;

	return true;
}

bool TTY:: setup_com(){
	struct termios options; 
	if ( tcgetattr(ttyfd, &options) != 0 ) 
		return false;

	/* Set the baud rates to gCFG->baud_rate ...*/
	if ( cfsetispeed(&options, gCFG->baud_rate) != 0 ) 
		return false;
	if ( cfsetospeed(&options, gCFG->baud_rate) != 0 ) 
		return false;

	/* Enable the receiver and set local mode...*/
	options.c_cflag |= (CLOCAL | CREAD);

	/* Set parity options.*/
	if ( gCFG->parity == 1)
	{
		options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/  
		options.c_iflag |= INPCK;

	} else if ( gCFG->parity == 2) 
	{
		options.c_cflag |= PARENB;
		options.c_cflag &= ~PARODD;
		options.c_iflag |= INPCK;

        } else if ( gCFG->parity == 0)
	{
		options.c_cflag &= ~PARENB;
		options.c_iflag &= ~INPCK; 
	}

	if (gCFG->stop_bit == 2) 
		options.c_cflag |= CSTOPB;
	else
		options.c_cflag &= ~CSTOPB;

	options.c_cflag &= ~CSIZE;
	options.c_cflag |= gCFG->data_size;    

	/* Set c_iflag input options */
	options.c_iflag &=~(IXON | IXOFF | IXANY);
	options.c_iflag &=~(INLCR | IGNCR | ICRNL);
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	/* Set c_oflag output options */
	options.c_oflag &= ~OPOST;   

	/* Set the timeout options */
	options.c_cc[VMIN]  = 0;
	options.c_cc[VTIME] = 10;

	if ( tcsetattr(ttyfd, TCSANOW, &options) != 0 ) 
		return false;
	return true;
}

#include "hook.c"
