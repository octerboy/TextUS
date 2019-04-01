/* Copyright (c) 2005-2020 by Ju Haibo (octerboy@gmail.com)
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

#include "DPoll.h"
#include "Amor.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "BTool.h"
#include "PacData.h"
#include "textus_string.h"
#include "Describo.h"
#include "casecmp.h"

#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>

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
	Amor::Pius local_pius, pro_tbuf;
	time_t last_failed_time;	//���һ�ε�ʧ��ʱ��,��
	bool should_spo;
	PacketObj *fname_pac;
	char ttyname[1024];

	char errMsg[ERRSTR_LEN];
	struct G_CFG {
		unsigned int    parity, stop_bit, baud_rate, data_size;
		tcflag_t	c_iflag, un_iflag;        /* input modes */
		tcflag_t	c_oflag, un_oflag;        /* output modes */
		tcflag_t	c_cflag, un_cflag;        /* control modes */
		tcflag_t	c_lflag, un_lflag;        /* line discipline modes */
		cc_t	c_cc[NCCS];
		cc_t	c_line;
		int file_flg;
		int wmode;

		bool on_start;
		Amor *sch;
		struct DPoll::PollorBase lor; /* ̽ѯ */
		bool useAio;
		int pac_fld_num;
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
#if defined(B460800)
    			else if (baud <= 460800) 
				return (B460800);
#endif
#if defined(B500000)
    			else if (baud <= 500000) 
				return (B500000);
#endif
#if defined(B576000)
    			else if (baud <= 576000) 
				return (B576000);
#endif
#if defined(B921600)
			else if (baud <= 921600)
				return(B921600);
#endif
#if defined(B1000000)
			else if (baud <= 1000000)
				return(B1000000);
#endif
#if defined(B1152000)
			else if (baud <= 1152000)
				return(B1152000);
#endif
#if defined(B1500000)
			else if (baud <= 1500000)
				return(B1500000);
#endif
#if defined(B2000000)
			else if (baud <= 2000000)
				return(B2000000);
#endif
#if defined(B2500000)
			else if (baud <= 2500000)
				return(B2500000);
#endif
#if defined(B3000000)
			else if (baud <= 3000000)
				return(B3000000);
#endif
#if defined(B3500000)
			else if (baud <= 3500000)
				return(B3500000);
#endif
#if defined(B4000000)
			else if (baud <= 4000000)
				return(B4000000);
#endif
			return(B9600);
		}

		inline G_CFG(TiXmlElement *cfg) {
			const char *comm_str;
			int baud_f, size_f, stop_f;
			unsigned int i;
			TiXmlElement *ele;
			char n_str[256];

			parity = 0 ;		/* None=0,Odd=1,Even=2  */
			data_size = CS8;	/* Data Bit = 8     */
    			stop_bit = 1;		/* Stop Bit = 1     */
			on_start = true;
			useAio = false;

			if ( !cfg) return;
			pac_fld_num = 1;
			wmode = 0;
			comm_str = cfg->Attribute("mode");
			if ( strcasecmp(comm_str, "raw") ==0 )
			{
				wmode = 1;
			} else if ( strcasecmp(comm_str, "canon") ==0 ) {
				wmode = 2;
			}
			cfg->QueryIntAttribute("field", &(pac_fld_num));
			c_line = 0;
			if ( cfg->Attribute("line"))
			{
				cfg->QueryIntAttribute("line", &(file_flg));
				c_line = (cc_t) file_flg;
			}
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
                		on_start = false; /* ����һ��ʼ������ */
			sch = 0;
			lor.type = DPoll::NotUsed;

			ele =  cfg->FirstChildElement("input") ;
			c_iflag = 0;
			un_iflag =~c_iflag;
			if ( ele && (comm_str = ele->GetText() ) )
			{
				for ( i = 0 ; i < strlen(comm_str); i++)
					n_str[i]= toupper(comm_str[i]);
				n_str[i] = 0;
				#define Set_InputMode(X) if ( strstr(n_str, #X) ) c_iflag |= X;
				Set_InputMode(IGNBRK)
				Set_InputMode(BRKINT)
				Set_InputMode(IGNPAR)
				Set_InputMode(PARMRK)
				Set_InputMode(INPCK)
				Set_InputMode(ISTRIP)
				Set_InputMode(INLCR)
				Set_InputMode(IGNCR)
				Set_InputMode(ICRNL)
			#ifdef IUCLC
				Set_InputMode(IUCLC)
			#endif
				Set_InputMode(IXON)
				Set_InputMode(IXANY)
				Set_InputMode(IXOFF)
				Set_InputMode(IMAXBEL)
			#ifdef IUTF8
				Set_InputMode(IUTF8)
			#endif
				#undef Set_InputMode
				#define Set_InputMode(X) if ( strstr(n_str, "~"#X) ) un_iflag &= ~X ;
				Set_InputMode(IGNBRK)
				Set_InputMode(BRKINT)
				Set_InputMode(IGNPAR)
				Set_InputMode(PARMRK)
				Set_InputMode(INPCK)
				Set_InputMode(ISTRIP)
				Set_InputMode(INLCR)
				Set_InputMode(IGNCR)
				Set_InputMode(ICRNL)
			#ifdef IUCLC
				Set_InputMode(IUCLC)
			#endif
				Set_InputMode(IXON)
				Set_InputMode(IXANY)
				Set_InputMode(IXOFF)
				Set_InputMode(IMAXBEL)
			#ifdef IUTF8
				Set_InputMode(IUTF8)
			#endif
			}

			ele =  cfg->FirstChildElement("output") ;
			c_oflag = 0;
			un_oflag = ~c_oflag;
			if ( ele && (comm_str = ele->GetText() ) )
			{
				for ( i = 0 ; i < strlen(comm_str); i++)
					n_str[i]= toupper(comm_str[i]);
				n_str[i] = 0;
				#define Set_OutputMode(X) if ( strstr(n_str, #X) ) c_oflag |= X;
				Set_OutputMode(OPOST)
			#ifdef OUCLC
				Set_OutputMode(OLCUC)
			#endif
				Set_OutputMode(ONLCR)
				Set_OutputMode(OCRNL)
				Set_OutputMode(ONOCR)
				Set_OutputMode(ONLRET)
				Set_OutputMode(OFILL)
				Set_OutputMode(OFDEL)
				Set_OutputMode(NLDLY)
				Set_OutputMode(NL0)
				Set_OutputMode(NL1)
				Set_OutputMode(CRDLY)
				Set_OutputMode(CR0)
				Set_OutputMode(CR1)
				Set_OutputMode(CR2)
				Set_OutputMode(CR3)
				Set_OutputMode(TABDLY)
				Set_OutputMode(TAB0)
				Set_OutputMode(TAB1)
				Set_OutputMode(TAB2)
				Set_OutputMode(TAB3)
				Set_OutputMode(BSDLY)
				Set_OutputMode(BS0)
				Set_OutputMode(BS1)
				Set_OutputMode(FFDLY)
				Set_OutputMode(FF0)
				Set_OutputMode(FF1)
				Set_OutputMode(VTDLY)
				Set_OutputMode(VT0)
				Set_OutputMode(VT1)
			#ifdef OXTABS
				Set_OutputMode(OXTABS)
			#endif
			#ifdef ONOEOT
				Set_OutputMode(ONOEOT)
			#endif
			#undef Set_OutputMode	
			#define Set_OutputMode(X) if ( strstr(n_str, "~"#X) ) un_oflag &= ~X;
				Set_OutputMode(OPOST)
			#ifdef OUCLC
				Set_OutputMode(OLCUC)
			#endif
				Set_OutputMode(ONLCR)
				Set_OutputMode(OCRNL)
				Set_OutputMode(ONOCR)
				Set_OutputMode(ONLRET)
				Set_OutputMode(OFILL)
				Set_OutputMode(OFDEL)
				Set_OutputMode(NLDLY)
				Set_OutputMode(NL0)
				Set_OutputMode(NL1)
				Set_OutputMode(CRDLY)
				Set_OutputMode(CR0)
				Set_OutputMode(CR1)
				Set_OutputMode(CR2)
				Set_OutputMode(CR3)
				Set_OutputMode(TABDLY)
				Set_OutputMode(TAB0)
				Set_OutputMode(TAB1)
				Set_OutputMode(TAB2)
				Set_OutputMode(TAB3)
				Set_OutputMode(BSDLY)
				Set_OutputMode(BS0)
				Set_OutputMode(BS1)
				Set_OutputMode(FFDLY)
				Set_OutputMode(FF0)
				Set_OutputMode(FF1)
				Set_OutputMode(VTDLY)
				Set_OutputMode(VT0)
				Set_OutputMode(VT1)
			#ifdef OXTABS
				Set_OutputMode(OXTABS)
			#endif
			#ifdef ONOEOT
				Set_OutputMode(ONOEOT)
			#endif
			}

			ele =  cfg->FirstChildElement("local") ;
			c_lflag = 0;
			un_lflag = ~c_lflag;
			if ( ele && (comm_str = ele->GetText() ) )
			{
				for ( i = 0 ; i < strlen(comm_str); i++)
					n_str[i]= toupper(comm_str[i]);
				n_str[i] = 0;
				#define Set_Local(X) if ( strstr(n_str, #X) ) c_lflag |= X;
				Set_Local(ISIG)
				Set_Local(ICANON)
			#ifdef XCASE
				Set_Local(XCASE)
			#endif
				Set_Local(ECHO)
				Set_Local(ECHOE)
				Set_Local(ECHOK)
				Set_Local(ECHONL)
				Set_Local(NOFLSH)
				Set_Local(TOSTOP)
				Set_Local(ECHOCTL)
				Set_Local(ECHOPRT)
				Set_Local(ECHOKE)
				Set_Local(FLUSHO)
				Set_Local(PENDIN)
				Set_Local(IEXTEN)
			#ifdef EXTPROC
				Set_Local(EXTPROC)
			#endif
			#ifdef DEFECHO 
				Set_Local(DEFECHO)
			#endif
			#ifdef ALTWERASE
				Set_Local(ALTWERASE)
			#endif
			#ifdef NOKERNINFO
				Set_Local(NOKERNINFO)
			#endif
			#undef Set_Local
			#define Set_Local(X) if ( strstr(n_str, "~"#X) ) un_lflag &= ~X;
				Set_Local(ISIG)
				Set_Local(ICANON)
			#ifdef XCASE
				Set_Local(XCASE)
			#endif
				Set_Local(ECHO)
				Set_Local(ECHOE)
				Set_Local(ECHOK)
				Set_Local(ECHONL)
				Set_Local(NOFLSH)
				Set_Local(TOSTOP)
				Set_Local(ECHOCTL)
				Set_Local(ECHOPRT)
				Set_Local(ECHOKE)
				Set_Local(FLUSHO)
				Set_Local(PENDIN)
				Set_Local(IEXTEN)
			#ifdef EXTPROC
				Set_Local(EXTPROC)
			#endif
			#ifdef DEFECHO 
				Set_Local(DEFECHO)
			#endif
			#ifdef ALTWERASE
				Set_Local(ALTWERASE)
			#endif
			#ifdef NOKERNINFO
				Set_Local(NOKERNINFO)
			#endif
			}
			ele =  cfg->FirstChildElement("control") ;
			c_cflag = 0;
			un_cflag = ~c_cflag;
			if ( ele && (comm_str = ele->GetText() ) )
			{
				for ( i = 0 ; i < strlen(comm_str); i++)
					n_str[i]= toupper(comm_str[i]);
				n_str[i] = 0;
			#define Set_Ctrl(X) if ( strstr(n_str, #X) ) c_cflag |= X;
				Set_Ctrl(HUPCL)
				Set_Ctrl(CLOCAL)
				Set_Ctrl(CRTSCTS)
			#ifdef CIBAUD
				Set_Ctrl(CIBAUD)
			#endif
			#ifdef CBAUD
				Set_Ctrl(CBAUD)
			#endif
				Set_Ctrl(CREAD)
			#ifdef RCV1EN
				Set_Ctrl(RCV1EN)
			#endif
			#ifdef LOBLK
				Set_Ctrl(LOBLK)
			#endif
			#ifdef XCLUDE
				Set_Ctrl(XCLUDE)
			#endif
			#ifdef CMSPAR
				Set_Ctrl(CMSPAR)
			#endif
			#ifdef XMT1EN
				Set_Ctrl(XMT1EN)
			#endif
			#ifdef CRTSXOFF
				Set_Ctrl(CRTSXOFF)
			#endif
			#ifdef CRTS_IFLOW
				Set_Ctrl(CRTS_IFLOW)
			#endif
			#ifdef CDTR_IFLOW
				Set_Ctrl(CDTR_IFLOW)
			#endif
			#ifdef CCTS_OFLOW
				Set_Ctrl(CCTS_OFLOW)
			#endif
			#ifdef CDSR_OFLOW
				Set_Ctrl(CDSR_OFLOW)
			#endif
			#ifdef CCAR_OFLOW
				Set_Ctrl(CCAR_OFLOW)
			#endif
			#ifdef MDMBUF
				Set_Ctrl(MDMBUF)
			#endif
			#ifdef PAREXT
				Set_Ctrl(PAREXT)
			#endif
			#ifdef CBAUDEXT
				Set_Ctrl(CBAUDEXT)
			#endif
			#ifdef CIBAUDEXT
				Set_Ctrl(CIBAUDEXT)
			#endif
			#undef Set_Ctrl
			#define Set_Ctrl(X) if ( strstr(n_str, "~"#X) ) un_cflag &= ~X;
				Set_Ctrl(HUPCL)
				Set_Ctrl(CLOCAL)
				Set_Ctrl(CRTSCTS)
			#ifdef CIBAUD
				Set_Ctrl(CIBAUD)
			#endif
			#ifdef CBAUD
				Set_Ctrl(CBAUD)
			#endif
				Set_Ctrl(CREAD)
			#ifdef RCV1EN
				Set_Ctrl(RCV1EN)
			#endif
			#ifdef LOBLK
				Set_Ctrl(LOBLK)
			#endif
			#ifdef XCLUDE
				Set_Ctrl(XCLUDE)
			#endif
			#ifdef CMSPAR
				Set_Ctrl(CMSPAR)
			#endif
			#ifdef XMT1EN
				Set_Ctrl(XMT1EN)
			#endif
			#ifdef CRTSXOFF
				Set_Ctrl(CRTSXOFF)
			#endif
			#ifdef CRTS_IFLOW
				Set_Ctrl(CRTS_IFLOW)
			#endif
			#ifdef CDTR_IFLOW
				Set_Ctrl(CDTR_IFLOW)
			#endif
			#ifdef CCTS_OFLOW
				Set_Ctrl(CCTS_OFLOW)
			#endif
			#ifdef CDSR_OFLOW
				Set_Ctrl(CDSR_OFLOW)
			#endif
			#ifdef CCAR_OFLOW
				Set_Ctrl(CCAR_OFLOW)
			#endif
			#ifdef MDMBUF
				Set_Ctrl(MDMBUF)
			#endif
			#ifdef PAREXT
				Set_Ctrl(PAREXT)
			#endif
			#ifdef CBAUDEXT
				Set_Ctrl(CBAUDEXT)
			#endif
			#ifdef CIBAUDEXT
				Set_Ctrl(CIBAUDEXT)
			#endif
			}
			file_flg = 0;
			for ( ele= cfg->FirstChildElement("flag"); ele; ele = ele->NextSiblingElement("flag"))
			{
				comm_str = ele->GetText();
				if ( !comm_str || strlen(comm_str) == 0 ) continue;
				if ( strcasecmp(comm_str, "read_only" ) == 0 )	
					file_flg |= O_RDONLY;
				if ( strcasecmp(comm_str, "write_only" ) == 0 )	
					file_flg |= O_WRONLY;
				if ( strcasecmp(comm_str, "read_write" ) == 0 || strcasecmp(comm_str, "write/read") == 0 
					|| strcasecmp(comm_str, "write_read") == 0 || strcasecmp(comm_str, "read/write") == 0 )	
					file_flg |= O_RDWR;
#if defined(__sun) || defined(__linux__) || defined(__FreeBSD__) 
				if ( strcasecmp(comm_str, "no_delay" ) == 0 )	
					file_flg |= O_NDELAY;
#endif
				if ( strcasecmp(comm_str, "noc_tty" ) == 0 )	
					file_flg |= O_NOCTTY;
				if ( strcasecmp(comm_str, "no_block" ) == 0 )	
					file_flg |= O_NONBLOCK;
			}
			memset(c_cc, 0, sizeof(c_cc));
			ele =  cfg->FirstChildElement("character") ;
			#define SET_CC(X) \
			comm_str = ele->Attribute(#X);	\
			if ( comm_str) {		\
				BTool::unescape(comm_str, (unsigned char*)&n_str[0]) ;	\
				c_cc[X] = n_str[0];		\
			}
			SET_CC(VINTR)
			SET_CC(VQUIT)
			SET_CC(VERASE)
			SET_CC(VKILL)
			SET_CC(VEOF)
			SET_CC(VTIME)
			SET_CC(VMIN)
		#ifdef VSWTC
			SET_CC(VSWTC)
		#endif
			SET_CC(VSTART)
			SET_CC(VSTOP)
			SET_CC(VSUSP)
			SET_CC(VEOL)
			SET_CC(VREPRINT)
			SET_CC(VDISCARD)
			SET_CC(VWERASE)
			SET_CC(VLNEXT)
			SET_CC(VEOL2)
		}
	};
	struct G_CFG *gCFG;
	bool has_config;
	TBuffer *tb_arr[3];

    	int ttyfd;	/* �ļ���� */
	Describo::Criptor mytor; /* �����׽���, ����ʵ����ͬ */
	
	//struct  termios  ttyold, ttynew;
	//TINLINE bool set();
	TBuffer *rcv_buf, *snd_buf, *m_rcv_buf, *m_snd_buf;
	bool wr_blocked ; 	/* ���һ��д������־ */

	TINLINE void transmit();
	TINLINE bool recito();
	TINLINE bool init();
	bool setup_com();
	TINLINE void end();
	TINLINE void deliver(Notitia::HERE_ORDO aordo);

#include "wlog.h"
};

#define DELI(X)	\
	if ( should_spo ) {	\
		aptus->sponte(&X);	\
	} else {			\
		aptus->facio(&X); }

void TTY::ignite(TiXmlElement *cfg)
{
	const char *comm_str;
	if ( !gCFG) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
	comm_str = cfg->Attribute("tty");
	if ( comm_str ) 
		TEXTUS_STRCPY(ttyname, comm_str);
}

#define SLOG(Z) { Amor::Pius log_pius; \
		log_pius.ordo = Notitia::LOG_##Z; \
		log_pius.indic = &errMsg[0]; \
		aptus->sponte(&log_pius); \
		}

#define ERROR_PRO(X)  if ( errMsg ) \
		TEXTUS_SNPRINTF(errMsg, ERRSTR_LEN, "%s errno %d, %s.", X, errno, strerror(errno));

bool TTY::facio( Amor::Pius *pius)
{
	Amor::Pius tmp_pius;
	assert(pius);
	switch (pius->ordo)
	{
	case Notitia::SET_UNIPAC:
		WBUG("facio SET_UNIPAC");
		{
			PacketObj **tmp;
			if ( (tmp = (PacketObj **)(pius->indic)))
			{
				if ( tmp[0] ) 
					fname_pac = tmp[0]; 
				else {
					WLOG(WARNING, "sponte SET_UNIPAC rcv_pac null");
				}
			} else {
				WLOG(WARNING, "facio SET_UNIPAC null");
			}
		}
		break;

	case Notitia::PRO_FILE_Pac :
		WBUG("facio PRO_FILE_Pac");
		TEXTUS_STRCPY(ttyname, (char*)(fname_pac->getfld(gCFG->pac_fld_num)));
		goto OPEN_PRO;
		break;

	case Notitia::PRO_FILE :
		WBUG("facio PRO_FILE");
		TEXTUS_STRCPY(ttyname, (char*)pius->indic);
OPEN_PRO:
		if ( init() )
		{
			local_pius.ordo = Notitia::Pro_File_Open;
		} else {
			local_pius.ordo = Notitia::Pro_File_Err;
		}
		local_pius.indic = 0;
		DELI(local_pius)
		break;

	case Notitia::FD_PRORD:
		WBUG("facio FD_PRORD");
		if ( recito() )
			aptus->facio(&pro_tbuf);
		break;

	case Notitia::FD_PROWR:
		WBUG("facio FD_PROWR");
		//д, ���ټ�, ����ϵͳ��æ
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
		tmp_pius.ordo = Notitia::CMD_GET_SCHED;
		aptus->sponte(&tmp_pius);	//��tpoll, ȡ��sched
		gCFG->sch = (Amor*)tmp_pius.indic;
		if ( !gCFG->sch ) 
		{
			WLOG(ERR, "no sched or tpoll");
			break;
		}
		tmp_pius.ordo = Notitia::POST_EPOLL;
		tmp_pius.indic = &gCFG->lor;
		gCFG->lor.pupa = this;
		
		gCFG->sch->sponte(&tmp_pius);	//��tpoll, ȡ��TPOLL
		if ( tmp_pius.indic == gCFG->sch )
		{
			gCFG->useAio = true;
			tmp_pius.ordo = Notitia::PRO_FILE_FD;
			tmp_pius.indic = &ttyfd;
			aptus->facio(&tmp_pius);
		} else {
			gCFG->useAio = false;
			deliver(Notitia::SET_TBUF);
		}
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
		if (!gCFG->useAio )
		{
			deliver(Notitia::SET_TBUF);
		}
		if ( gCFG->on_start )
			init();
		break;

	case Notitia::DMD_START_SESSION:
		WBUG("facio DMD_START_SESSION");
		init();		//��ʼ��������
		break;

	case Notitia::CMD_GET_FD:	//�����׽���������
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

	case Notitia::SET_TBUF:	/* ȡ������TBuffer��ַ */
		WBUG("facio SET_TBUF");
		should_spo = true;
		{
		TBuffer **tb;
		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//��Ȼtb����Ϊ��
			if ( *tb) 
			{	//�µ������TBuffer
				snd_buf = *tb;
			}
			tb++;
			if ( *tb) rcv_buf = *tb;
		} else 
			WLOG(NOTICE,"facio PRO_TBUF null.");
		}
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
	case Notitia::PRO_TBUF :	//����һ֡���ݶ���
		WBUG("sponte PRO_TBUF");	
		if ( ttyfd <= 0 )
			init();

		transmit();
		break;
		
	case Notitia::CMD_GET_FD:	//�����׽���������
		WBUG("sponte CMD_GET_FD");		
		pius->indic = &(ttyfd);
		break;

	case Notitia::DMD_END_SESSION:	//ǿ�ƹرգ���ͬ�����رգ�Ҫ֪ͨ����
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
		init();		//��ͨ��
		break;

	case Notitia::SET_TBUF:	/* ȡ������TBuffer��ַ */
		WBUG("sponte SET_TBUF");
		should_spo = false;
		{
		TBuffer **tbp;
		tbp = (TBuffer **)(pius->indic);
		if (tbp) 
		{	//��Ȼtb����Ϊ��
			if ( *tbp) 
			{	//�µ������TBuffer
				rcv_buf = *tbp;
			}
			tbp++;
			if ( *tbp) snd_buf = *tbp;
		} else 
			WLOG(NOTICE,"facio PRO_TBUF null.");
		}
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
	pro_tbuf.indic = &tb_arr[0];

	last_failed_time = 0;
	memset(errMsg, 0, sizeof(errMsg));

	gCFG = 0;
	has_config = false;
	ttyfd = -1;
	wr_blocked = false; 	//�տ�ʼ, ���һ��д��Ȼ������
	m_rcv_buf = new TBuffer(1024);
	m_snd_buf = new TBuffer(1024);
	rcv_buf = m_rcv_buf;
	snd_buf = m_snd_buf;
}

TTY::~TTY()
{	
	end();
	if (has_config )
		delete gCFG;
	delete m_rcv_buf;
	delete m_snd_buf;
}

TINLINE bool TTY::recito()
{
	int len;

	rcv_buf->grant(512);	//��֤���㹻�ռ�
ReadAgain:
	if( (len = read(ttyfd, (char *)rcv_buf->point, 512)) == 0) /* (char*) for WIN32 */
	{	//�Է��ر��׽���
		TEXTUS_SNPRINTF(errMsg, ERRSTR_LEN, "recv 0, disconnected");
		SLOG(INFO)
		end();
		return false;
	} else if ( len == -1 )
	{ 
		int error = errno;
		if (error == EINTR)
		{	 //���źŶ���,����
			goto ReadAgain;
		} else if ( error == EAGAIN || error == EWOULDBLOCK )
		{	//���ڽ�����, ��ȥ�ٵ�.
			ERROR_PRO("read encounter EAGAIN")
			SLOG(NOTICE)
			return false;
		} else	
		{	//��ȷ�д���
			ERROR_PRO("read")
			SLOG(ERR)
			return false;
		}
	} 

	rcv_buf->commit(len);	/* ָ������� */
	return true;
}

TINLINE void TTY::transmit()
{
	int len;
	int snd_len = snd_buf->point - snd_buf->base;	//���ͳ���

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
		int error = errno;
		if (error == EINTR)	
		{	//���źŶ���,����
			goto SendAgain;

		} else if (error == EWOULDBLOCK || error == EAGAIN)
		{	//��ȥ����, ��select, Ҫ��wrSet
			if ( wr_blocked ) 	//���һ������
			{
				return ;
			} else {	//�շ���������
				ERROR_PRO("write encounter EAGAIN")
				SLOG(NOTICE)
				wr_blocked = true;
				//��Sched, ������wrSet.
				local_pius.ordo =Notitia::FD_SETWR;
				aptus->sponte(&local_pius);
			}
		} else {
			ERROR_PRO("write")
			SLOG(ERR);
			end();
		}
	}

	snd_buf->commit(-len);	//�ύ������������
	if (snd_len > len )
	{	
		if ( wr_blocked ) 	//���һ�λ�������
		{
			return ;
		} else {	//�շ���������
			TEXTUS_SNPRINTF(errMsg, ERRSTR_LEN, "sending not completed.");
			SLOG(NOTICE)
			wr_blocked = true;
			//��Sched, ������wrSet.
			local_pius.ordo =Notitia::FD_SETWR;
			aptus->sponte(&local_pius);
		}
	} else 
	{	//������������
		if ( wr_blocked ) 	//���һ������
		{
			wr_blocked = false;
			local_pius.ordo =Notitia::FD_CLRWR;
			//��Sched, ������wrSet.
			aptus->sponte(&local_pius);	
		} else
		{	//һֱû������
			return ; //�������
		}
	}

	return ;
}

TINLINE void TTY::end()
{
	WBUG("end().....");
	if ( ttyfd == -1 ) 
		return;	/* ���ظ��ر� */
	if ( !gCFG->useAio )
	{
		deliver(Notitia::FD_CLRWR);
		deliver(Notitia::FD_CLREX);
		deliver(Notitia::FD_CLRRD);
	}
	
	//ioctl(ttyfd,TCSETAF,&ttyold);	/* ��ԭ���� */
	close(ttyfd);
	ttyfd = -1;

	deliver(Notitia::END_SESSION);/* �����Ҵ��ݱ���ĻỰ�ر��ź� */
}

Amor* TTY::clone()
{
	TTY *child;
	child = new TTY();
	child->gCFG = gCFG;
	memcpy(child->ttyname, ttyname, strlen(ttyname)+1);
	return (Amor*)child;
}

/* ��������ύ */
TINLINE void TTY::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
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
		gCFG->sch->sponte(&local_pius);	//��Sched
		return ;

	default:
		break;
	}
	aptus->facio(&tmp_pius);
}

bool TTY::init()
{
	if ( !gCFG->file_flg )
		ttyfd=open(ttyname,O_RDWR);
	else
		ttyfd=open(ttyname, gCFG->file_flg);
	if ( ttyfd < 0 )
	{
		WLOG_OSERR("open tty");
		return false;
	}

    	if( !setup_com() )
	{
		WLOG_OSERR("set tty");
        	close(ttyfd);
		ttyfd = -1;
		return false;
	}
	if ( !gCFG->useAio )
	{
		mytor.scanfd = ttyfd;
		deliver(Notitia::FD_SETRD);
		deliver(Notitia::FD_CLRWR);
		deliver(Notitia::FD_CLREX);

		/* ����(����)��������� */
		if ( rcv_buf) rcv_buf->reset();	
		if ( snd_buf) snd_buf->reset();
	}
	deliver(Notitia::START_SESSION); //������߷���֪ͨ, ������ʼ
	return true;
}
#if 0
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

#if defined(__linux__)
	ttynew.c_line=0;    /* line discipline */
#endif

	if ( ioctl(ttyfd,TCSETAF,&ttynew) == -1 )
		return false;

	return true;
}
#endif

bool TTY:: setup_com(){
	struct termios options; 
	unsigned int i;
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
		options.c_cflag |= (PARODD | PARENB); /* ����Ϊ��Ч��*/  
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
	options.c_cflag |= gCFG->c_cflag;
	options.c_cflag &= gCFG->un_cflag;
	options.c_iflag |= gCFG->c_iflag;
	options.c_oflag |= gCFG->c_oflag;
	options.c_iflag &= gCFG->un_iflag;
	options.c_oflag &= gCFG->un_oflag;
	if ( gCFG->wmode  == 1 )  {
		/* Set c_iflag input options */
		options.c_iflag &=~(IXON | IXOFF | IXANY);
		options.c_iflag &=~(INLCR | IGNCR | ICRNL);
		options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
		/* Set c_oflag output options */
		options.c_oflag &= ~OPOST;   
	} else if ( gCFG->wmode  == 2 ) {
		options.c_lflag |= (ICANON | ECHO | ECHOE );
	}

#if defined(__linux__)
	options.c_line= gCFG->c_line;    /* line discipline */
#endif
	/* Set the timeout options */
	options.c_cc[VMIN]  = 0;
	options.c_cc[VTIME] = 10;
	for ( i = 0 ; i < NCCS; i++ ) 
	{
		if ( gCFG->c_cc[i] > 0 ) 
			options.c_cc[i] = gCFG->c_cc[i];
	}

	if ( tcsetattr(ttyfd, TCSANOW, &options) != 0 ) 
		return false;
	return true;
}
/* https://www.cmrr.umn.edu/~strupp/serial.html#3_1_1 */
#include "hook.c"
