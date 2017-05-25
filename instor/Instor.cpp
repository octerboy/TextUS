#include "Notitia.h"
#include "Amor.h"
#include <stdio.h>

#define SCM_MODULE_ID  "$Workfile: Instor5.cpp $"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "PacData.h"
#include "BTool.h"
#include "casecmp.h"
#include "textus_string.h"
//#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include <ctype.h>
#include <stdarg.h>

#define MINLINE inline
#define ObtainHex(s, X)   ( (s) > 9 ? (s)-10+X :(s)+'0')
#define Obtainx(s)   ObtainHex(s, 'a')
#define ObtainX(s)   ObtainHex(s, 'A')
#define Obtainc(s)   (s >= 'A' && s <='F' ? s-'A'+10 :(s >= 'a' && s <='f' ? s-'a'+10 : s-'0' ) )

class Instor: public Amor
{
public:
	Amor::Pius local_pius;

	PacketObj *rcv_pac;	/* 来自左节点的PacketObj */
	PacketObj *snd_pac;

	Instor();
	~Instor();

	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
	
	char m_error_buf[512];
	void handle_pac();	
	int dev_init(void);
	int OpenCard(char uid[17]);
	int Command(const char *command, char* reply, int *psw);
#include "wlog.h"
};

void Instor::ignite(TiXmlElement *cfg)
{
	int i=0;

	return ;
}

bool Instor::facio( Amor::Pius *pius)
{
	PacketObj **tmp;
	assert(pius);
	switch(pius->ordo )
	{
		case 33:
			/* .... */
			break;
		case Notitia::IGNITE_ALL_READY:
			break;
		
		case Notitia::CLONE_ALL_READY:
			break;

	case Notitia::PRO_UNIPAC:
		WBUG("facio PRO_UNIPAC");
		handle_pac();

		break;

	case Notitia::START_SESSION:
		WBUG("facio START_SESSION");
		break;

	case Notitia::SET_UNIPAC:
		WBUG("facio SET_UNIPAC");
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) rcv_pac = *tmp; 
			else
				WLOG(WARNING, "facio SET_UNIPAC rcv_pac null");
			tmp++;
			if ( *tmp) snd_pac = *tmp;
			else
				WLOG(WARNING, "facio SET_UNIPAC snd_pac null");
		} else 
			WLOG(WARNING, "facio SET_UNIPAC null");
		
		break;


	default:
		return false;
	}
	return true;
}

int Instor::Command(const char *command, char* reply, int *psw)
{
	// TODO: 在此添加实现代码
	int ret;
	int which = 1;
	void *ps[6];
	Amor::Pius para;

	ret = -1;
	m_error_buf[0] = 0;
	ps[0] = &ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) 0;	//不指定slot
	ps[3] = (void*)command;
	ps[4] = reply;
	ps[5] = psw;
	para.indic = &ps[0];

	switch (which)
	{
	case 0:	//Sam
		para.ordo = Notitia::IC_SAM_COMMAND;
		break;
	case 1:	//Pro
		para.ordo = Notitia::IC_PRO_COMMAND;
		break;
	default:
		//para.ordo = Notitia::TEXTUS_RESERVED;
		break;
	}
	
	aptus->facio(&para);
	if ( ret == 0 && *psw != 0x9000 )
	{
		TEXTUS_SPRINTF(m_error_buf, "%s(\"%s\")\n返回 %04x", which==0 ? "SamCommand":"ProCommand", command, *psw);
	}
	return ret;
}

int Instor::OpenCard(char uid[17])
{
	int ret;
	void *ps[4];
	Amor::Pius para;

	ret = -1;
	m_error_buf[0] = 0;
	ps[0] = &ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) 0;	//不指定slot
	ps[3] = &uid[0];
	para.indic = ps;
	para.ordo = Notitia::IC_OPEN_PRO;
	
	facio(&para);
	return ret;
}

int Instor::dev_init(void)
{
	int ret = -1;	//假定所有读写器失败
	void *ps[4];
	Amor::Pius para;

	m_error_buf[0] = 0;
	ps[0] = &ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) 0;	//不指定参数
	para.indic = ps;
	para.ordo = Notitia::IC_DEV_INIT; 
	
	facio(&para);
	return ret;
}

void Instor::handle_pac()
{
	unsigned char *p;
	unsigned long len;
	int ret;

	snd_pac->reset();
	snd_pac->input(0, (unsigned char*)"99",2);
	p =rcv_pac->getfld(1, &len);	//1, 功能字
	if ( p ) { p[len] = 0; WBUG("act %s", p); }
	if ( len == 4 && memcmp(p, "CTst",4) == 0)	//卡片复位
	{
		/* 打开机具进行初始化 */
		snd_pac->input(1, (unsigned char*)"CTstA",5);
		ret = dev_init();
		if ( ret ) 
		{
			WLOG(ERR, "%s", m_error_buf);
			snd_pac->input(2, (unsigned char*)"B",1);
			snd_pac->input(3, (unsigned char*)m_error_buf,strlen(m_error_buf));
		} else {
			snd_pac->input(2, (unsigned char*)"0",1);
			snd_pac->input(3, (unsigned char*)"Yes",3);
		}
		goto BACK;
	}
	if ( len == 4 && memcmp(p, "CRst",4) == 0)	//卡片复位
	{
		char uid[17];
		ret = OpenCard(uid);
		snd_pac->input(1, (unsigned char*)"CRstA",5);
		if ( ret ) 
		{
			char msg[32];
			WLOG(ERR, "%s", m_error_buf);
			TEXTUS_SPRINTF(msg, "%08x", ret);
			snd_pac->input(2, (unsigned char*)"A",1);
			snd_pac->input(3, (unsigned char*)msg, strlen(msg));
		} else {
			snd_pac->input(2, (unsigned char*)"0",1);
			snd_pac->input(3, (unsigned char*)"0",1);
		}
		snd_pac->input(4, (unsigned char*)"C",1);
		snd_pac->input(5, (unsigned char*)"info",1);
		snd_pac->input(6, (unsigned char*)uid,8);
		snd_pac->input(7, (unsigned char*)"",0);
		goto BACK;
	}

	if ( len == 3 && memcmp(p, "CAU",3) == 0)	//卡指令
	{
		char req[256], res[256], sw[5];
		int isw;
		p =rcv_pac->getfld(2, &len);	//2, 
		memcpy(req, p, len); req[len] = 0;

		ret = Command(req, res, &isw);
		WBUG("REQ %s, ANS %s, sw %4X", req, res, isw);
		TEXTUS_SPRINTF(sw, "%4X", (isw & 0x0000FFFF));
		if ( ret < 0  ) 
		{
			snd_pac->input(2, (unsigned char*)"A",1);
			snd_pac->input(3, (unsigned char*)"0", 0);
			WLOG(ERR, "%s", m_error_buf);
		} else {
			snd_pac->input(2, (unsigned char*)"0",1);
			snd_pac->input(3, (unsigned char*)res, strlen(res));
		}
		snd_pac->input(1, (unsigned char*)"CAUA",4);
		snd_pac->input(4, (unsigned char*)sw,4);
		goto BACK;
	}

	if ( len == 3 && memcmp(p, "CFd",3) == 0)	//进卡
	{
		char card_no[32];
		p =rcv_pac->getfld(2, &len);	//2, 
		memcpy(card_no, p, len); card_no[len] = 0;
		printf("-------feed card %s-----------\n", card_no);
		snd_pac->input(1, (unsigned char*)"CFdA",4);
		snd_pac->input(2, (unsigned char*)"0",1);
		snd_pac->input(3, (unsigned char*)"0",1);
		goto BACK;
	}

	if ( len == 4 && memcmp(p, "COut",4) == 0)	//出卡
	{
		char card_no[32];
		p =rcv_pac->getfld(3, &len);	//2, 
		memcpy(card_no, p, len); card_no[len] = 0;
		p =rcv_pac->getfld(2, &len);	//2, 
		printf("-------out card(%s) %s -----------\n", *p=='0' ? "OK":"Bad", card_no);
		snd_pac->input(1, (unsigned char*)"COutA",5);
		snd_pac->input(2, (unsigned char*)"0",1);
		snd_pac->input(3, (unsigned char*)"0",1);
		goto BACK;
	}
	if ( len == 4 && memcmp(p, "CPrm",4) == 0)	//进度提示
	{
		char cent[32];
		p =rcv_pac->getfld(2, &len);	//2, 
		memcpy(cent, p, len); cent[len] = 0;
		printf("-------%s%%-----------\n", cent);
		snd_pac->input(1, (unsigned char*)"CPrmA",5);
		snd_pac->input(2, (unsigned char*)"A",1);
		goto BACK;
	}

	if ( len == 3 && memcmp(p, "Fxt",3) == 0)	//准备下一张提示
	{
		char card_no[32], desc[32];
		p =rcv_pac->getfld(2, &len);	//2, 
		memcpy(card_no, p, len); card_no[len] = 0;
		p =rcv_pac->getfld(3, &len);	//2, 
		memcpy(desc, p, len); desc[len] = 0;
		printf("-------prepair %s %s-----------\n", desc, card_no);
		snd_pac->input(1, (unsigned char*)"FxtA",4);
		snd_pac->input(2, (unsigned char*)"A",1);
		goto BACK;
	}

BACK:
	aptus->sponte(&local_pius);
}

bool Instor::sponte( Amor::Pius *pius)
{
	return true;
}

Instor::Instor()
{
	local_pius.ordo = Notitia::PRO_UNIPAC;
	local_pius.indic = 0;
}

Instor::~Instor()
{
}

Amor* Instor::clone()
{
	return (Amor*)new Instor();
}
#define AMOR_CLS_TYPE Instor
#include "hook.c"
