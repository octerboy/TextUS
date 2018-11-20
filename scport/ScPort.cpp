
#include "Amor.h"
#include "Notitia.h"
#include "casecmp.h"
#include "textus_string.h"
#include <winscard.h>
#include <winsmcrd.h>
#include <stdio.h>

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#define TYSAM_NAME "Tianyu SAM Card Reader"

class ScPort: public Amor
{
public:
	bool dev_ok;	//true:设备已经OK. 
	bool card_ok;	//true:用户卡片已经OK, 一旦发生通讯错误, 即将此值设为false

	int dev_init(void);
	int dev_close(void);
	int Pro_Open(char uid[17]);
	int Pro_Close();
	int Sam_Reset(char atr[64]);
	int Pro_Present();
	int CardCommand (const char* command, char* response, int *sw, bool sel_sam = false);
	char *m_error_buf;

	ScPort();
	~ScPort();

	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	char pmszReaders[8192];

	struct G_CFG {
		const char *uid_ins;	//取UID的专用指令, 读写器设定
		int uid_offset;		//用了UID的专用指令后, 返回的偏移量
		int uid_length;		//用了UID的专用指令后, 取uid的长度
		const char *rd_name;	//由外部设置的读写器名称
		char name[256];		//实际名称
		bool isTySam;		//是否天喻SAM
		unsigned char sam_face;	//
		unsigned char def_sam_face;	//
		bool found;		//有没有找到读写器
		SCARDCONTEXT    hSC;

		inline G_CFG() {
			hSC = 0;
			uid_offset = 0;
			uid_length = 4;
			rd_name = (const char*)0;
			uid_ins = (const char*)0;
			found = false;
			isTySam = false;
			sam_face = 0;
		};	
		inline ~G_CFG() {
		};
	};
	struct G_CFG *gCFG;     /* 全局共享参数 */
	bool has_config;

	SCARDHANDLE     hCardHandle;	//读写器句柄,初始为空。
	DWORD           activeProtocol;	//激活使用的协议,SCARD_PROTOCOL_T0/T1
	LPCSCARD_IO_REQUEST spci;   //SCARD_PCI_T0 SCARD_PCI_T1
#include "wlog.h"
};

/* 不同的读写器有完全不同的参数配置, ignite的函数内容完全不同.  */
void ScPort::ignite(TiXmlElement *prop)
{
	int i_sam_face;
	//MessageBox(NULL, "scport ignite", TEXT("Unigo "), MB_OK);	
	if (!prop) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
	}
	gCFG->rd_name = prop->Attribute("reader");
	if ( gCFG->rd_name)
	{
		if (strcasecmp(gCFG->rd_name, TYSAM_NAME) == 0)
		{
			gCFG->isTySam = true;
		}
	}
	gCFG->uid_ins = prop->Attribute("uid_command");
	prop->QueryIntAttribute("uid_offset", &gCFG->uid_offset);
	prop->QueryIntAttribute("uid_length", &gCFG->uid_length);

        prop->QueryIntAttribute("SIM", &i_sam_face);
        gCFG->def_sam_face = 0xFF & i_sam_face;

	return ;
}

static char m_sys_buf[256] = {0};
static void sys_err_desc() 
{ 
	DWORD dw;
	int mlen;
	dw = GetLastError(); 
    	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw,
        	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), m_sys_buf, 256, NULL );
	mlen = strlen(m_sys_buf);
	if ( mlen > 2 ) {
		if ( m_sys_buf[mlen-1] == 0x0a && m_sys_buf[mlen-2] == 0x0d)
			m_sys_buf[mlen-2] = 0;
	}
}

/* 返回ret=0正确, 其它错误 */
/* 不同的读写器有完全不同的参数配置, Pro_Open的函数内容完全不同.  */
int ScPort::Pro_Open(char uid[17])
{
	int ret, exists;
	long lReturn;
	char resp[256];
	int sw;
	const char *t0="SCARD_PROTOCOL_T0";
	const char *t1="SCARD_PCI_T1";
	const char *to="SCARD_PCI_OTHER";
	const char *t;
	bool isRe = false;
	int m_try = 3;

	ret = -1;
	Pro_Close();
	exists = Pro_Present();
	if ( exists == 1 )
	{
		TEXTUS_STRCPY(m_error_buf, "无卡"); 
		return -100;
	} if ( exists == -1 )  //函数故障
	{
		TEXTUS_STRCPY(m_error_buf, "读写器操作错误."); 
		goto END;
	}

Again:
	m_try--;
	if ( m_try <= 0 ) 
		goto END;
	if ( hCardHandle == -1 ) 
	{
		lReturn = SCardConnect(	gCFG->hSC, gCFG->name, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0|SCARD_PROTOCOL_T1,&hCardHandle, &activeProtocol);
		isRe=false;
	} 	else  {
		lReturn = SCardReconnect(hCardHandle, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0|SCARD_PROTOCOL_T1,SCARD_RESET_CARD, &activeProtocol);
		isRe=true;
	}

	t= to;
	if (activeProtocol == SCARD_PROTOCOL_T0 )
		t= t0;
		
	if (activeProtocol == SCARD_PROTOCOL_T1 )
		t= t1;
	spci = (activeProtocol == SCARD_PROTOCOL_T0 ? SCARD_PCI_T0:SCARD_PCI_T1);
	if ( SCARD_S_SUCCESS != lReturn )
	{
		hCardHandle = -1;
		switch (lReturn)
		{
		case SCARD_E_NOT_READY:
			WLOG(INFO, "OpenCard Error (SCARD_E_NOT_READY)");	//无卡
			break;
		case SCARD_W_REMOVED_CARD:
			WLOG(INFO, "OpenCard Error (SCARD_W_REMOVED_CARD)");	//无卡
			break;
		default:
			WLOG(ERR, "SCardConnect(hSC=%p) ret %08X", gCFG->hSC, lReturn );
			break;
		}
		goto TRY_NEXT;
	} else {
		WBUG("%s  %s, at %s, hCard=%p", isRe?"SCardReConnect":"SCardConnect", t, gCFG->name, hCardHandle);
	}

	if (gCFG->uid_ins /*&& activeProtocol == SCARD_PROTOCOL_T1 */ )
	{
		ret = CardCommand (gCFG->uid_ins, resp, &sw);
		memcpy(&uid[0], &resp[(gCFG->uid_offset)*2], (gCFG->uid_length)*2);
		uid[(gCFG->uid_length)*2] = '\0';
		WLOG(INFO, "Open Card ret %d,  uid= %s, sw=%04x, %s", ret, uid, sw, t == t0 ? "T0":"T1");
	} else {
		ret = 0;
	}

TRY_NEXT:
	if ( ret != 0 )
	{
		Sleep(100);	//有卡片，打开未成功, 再来.
		goto Again;
	}
END:
	return ret;	
}

/* 探卡, 检测卡片是否在位(包括非接), 返回true:存在, false:不存在 */
/* 不同的读写器, Pro_Present的函数内容完全不同.  */
int ScPort::Pro_Present()
{
	long lReturn;
	int ret;
	int my_try = 3;
	SCARD_READERSTATE ReaderState = {0};

	ReaderState.szReader = gCFG->name;
	ReaderState.dwCurrentState = SCARD_STATE_UNAWARE;	

	ret = -1;	
	/* 这里是各种读写器的具体做法 */
Again:
	my_try--;
	if ( my_try <= 0 ) 
		goto END;
	lReturn = SCardGetStatusChange(gCFG->hSC, 100, &ReaderState,1);
	if ( lReturn == SCARD_S_SUCCESS )
	{	
		if(ReaderState.dwEventState & SCARD_STATE_PRESENT)
			ret = 0;	
		else {
			ret = 1;
			card_ok =false;
		}
	} else {
		card_ok =false;
		WLOG(ERR, "SCardGetStatusChange(hSC=%p) ret %08X", gCFG->hSC, lReturn);
		/* 这个不应该出错的 */
		lReturn = SCardReleaseContext(gCFG->hSC);
		if ( SCARD_S_SUCCESS != lReturn )
			WLOG(ERR, "SCardReleaseContext(hSC=%p) ret %08X", gCFG->hSC, lReturn);
		Sleep(100);	//暂停一下, 再用新的句柄
		lReturn = SCardEstablishContext(SCARD_SCOPE_SYSTEM,  NULL, NULL, &(gCFG->hSC));
		if ( SCARD_S_SUCCESS != lReturn )
		{
			gCFG->hSC = -1;
			WLOG(ERR, "Established Context Failed error=%08X", lReturn);
			goto END;
		} else {
			WBUG("Established Context OK, hSC=%p", gCFG->hSC);
		}
		goto Again;
	}
END:
	return ret;
}

/* 返回ret=0正确, 其它错误 */
/* 不同的读写器Pro_Close的函数内容完全不同.  */
int ScPort::Pro_Close()
{
	long lReturn;
	if ( hCardHandle == -1)
		return 0;

	lReturn = SCardDisconnect(hCardHandle,  SCARD_UNPOWER_CARD);
	card_ok = false;
	if ( SCARD_S_SUCCESS != lReturn )
	{
		WLOG(ERR, "SCardDisconnect(hSC=%p, hCard=%p) ret %08X", gCFG->hSC, hCardHandle, lReturn);
	}
	hCardHandle = -1;
	return 0;	
}

/* 返回ret=0正确, 其它错误 */
/* 不同的读写器有完全不同的参数配置, Sam_Reset的函数内容完全不同.  */
int ScPort::Sam_Reset(char atr[64])
{
	int ret;
	long lReturn;
	char resp[256], tmp[128];
	int sw;
	DWORD dwATRBufferLen = 60; 
	const char *t0="SCARD_PROTOCOL_T0";
	const char *t1="SCARD_PCI_T1";
	const char *to="SCARD_PCI_OTHER";
	const char *t;
	bool isRe;

	ret = -1;
	if ( hCardHandle == -1 ) 
	{
		lReturn = SCardConnect(	gCFG->hSC, gCFG->name, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0|SCARD_PROTOCOL_T1,&hCardHandle, &activeProtocol);
		isRe=false;
	} 	else  {
		lReturn = SCardReconnect(hCardHandle, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0|SCARD_PROTOCOL_T1,SCARD_RESET_CARD, &activeProtocol);
		isRe=true;
	}

	t= to;
	if (activeProtocol == SCARD_PROTOCOL_T0 )
		t= t0;
		
	if (activeProtocol == SCARD_PROTOCOL_T1 )
		t= t1;
	spci = (activeProtocol == SCARD_PROTOCOL_T0 ? SCARD_PCI_T0:SCARD_PCI_T1);
	if ( SCARD_S_SUCCESS != lReturn )
	{
		hCardHandle = -1;
		switch (lReturn)
		{
		case SCARD_E_NOT_READY:
			WLOG(INFO, "SamReset Error (SCARD_E_NOT_READY)");	//无卡
			break;
		case SCARD_W_REMOVED_CARD:
			WLOG(INFO, "SamReset Error (SCARD_W_REMOVED_CARD)");	//无卡
			break;

		default:
			//sys_err_desc();
			//WLOG(ERR, "SCardConnect(hSC=%p) ret %08X, error=%08X, %s", gCFG->hSC, lReturn, GetLastError(), m_sys_buf);
			WLOG(ERR, "SCardConnect(hSC=%p) ret %08X", gCFG->hSC, lReturn);
			break;
		}
		goto END;
	} else {
		WBUG("%s  %s, at %s, hCard=%p", isRe?"SCardReConnect":"SCardConnect", t, gCFG->name, hCardHandle);
	}

	if ( gCFG->isTySam ) 
	{
		switch ( gCFG->sam_face )
		{
		case 0x01:
			CardCommand ("FF14000100", &resp[0], &sw);
			break;
		case 0x02:
			CardCommand ("FF14000200", &resp[0], &sw);
			break;
		default:
			break;
		} 
	}
	
	SCardGetAttrib(hCardHandle, SCARD_ATTR_ATR_STRING, (BYTE*)&resp[0], &dwATRBufferLen);
	byte2hex((unsigned char*)&resp[0], dwATRBufferLen, tmp);
	tmp[2*dwATRBufferLen] = 0;
	TEXTUS_STRCPY(atr, tmp);
	WLOG(INFO, "SamReset atr= %s, %s", tmp, t == t0 ? "T0":"T1");
	ret = 0;
END:
	if ( ret)
		TEXTUS_STRCPY(m_error_buf, "SAM复位失败"); 
	return ret;	
}

/* 返回ret=0正确, 其它错误. */
/* 不同的读写器有完全不同的参数配置, CardCommand的函数内容完全不同.  */
int ScPort::CardCommand (const char* command, char* response, int *sw, bool sel_sam)
{
	int ret, exists;
	long lReturn;
	unsigned char sendblock[256], recvblock[256];
	DWORD sendlen, recvlen = 256;
	unsigned char get_resp[]={0x00, 0xC0, 0x00, 0x00, 0x00};
	size_t slen;
	int m_try = 3;

	slen = strlen(command)/2;
	if ( slen > 250 ) slen =250;
	hex2byte(&sendblock[0],  slen, command);
	sendlen = (unsigned char) (slen & 0xFF);

	*sw = 0;
	card_ok = false;
	ret = -1;

	if (!sel_sam )	//非SAM卡就是PRO卡
	{
		exists = Pro_Present();
		if ( exists == 1 )
		{
			TEXTUS_STRCPY(m_error_buf, "无卡"); 
			return -100;
		} if ( exists == -1 )  //函数故障
			goto END;
	} else {
		if ( gCFG->isTySam ) 
		{
			switch ( gCFG->sam_face )
			{
			case 0x01:
				CardCommand ("FF14000100", response, sw, true);
				break;
			case 0x02:
				CardCommand ("FF14000200", response, sw, true);
				break;
			default:
				break;
			} 
		}
	}

Again:
	m_try--;
	if ( m_try <= 0 ) 
		goto END;
JA:
	lReturn = SCardTransmit(hCardHandle, spci, sendblock, sendlen, NULL, recvblock, &recvlen);
	if ( SCARD_S_SUCCESS != lReturn )
	{
		WLOG(ERR, "SCardTransmit(%s)(hSC=%p, hCard=%p) ret %08X", command, gCFG->hSC, hCardHandle, lReturn);
		TEXTUS_STRCPY(m_error_buf,  "读卡器指令错误");
		if ( lReturn == ERROR_INVALID_FUNCTION )
		{
			slen = strlen(command)/2;
			if ( slen > 250 ) slen =250;
			hex2byte(&sendblock[0],  slen, command);
			sendlen = (unsigned char) (slen & 0xFF);
			recvlen=sizeof(recvblock);
			Sleep(100);	//稍息
			goto Again;
		} else
			goto END;
	} else if ( recvlen < 2 )  {
		WLOG(ERR, "SCardTransmit(%s), recvlen=%d", command,  recvlen);
		TEXTUS_STRCPY(m_error_buf, "IC卡接收数据错误");
		goto END;
	} else {
		if (recvblock[recvlen-2] == 0x61 ) 			
		{							
			WBUG("get_resp %02x %02x ", recvblock[recvlen-2], recvblock[recvlen-1]);
			get_resp[4] = recvblock[recvlen-1];
			recvlen=sizeof(recvblock);
			memcpy(sendblock, get_resp, 5);
			sendlen = 5;
			m_try = 0;	//接触界面, 不重试了
			goto JA;
		}

		*sw = recvblock[recvlen-2]*256 +  recvblock[recvlen-1];	
		recvlen -=2;	//最后两字节为SW,这里去掉
		byte2hex (&recvblock[0], recvlen, response); 
		response[recvlen*2] = 0;
		card_ok = true;
		ret = 0;
	}
END:
	if ( ret!= 0 )
		Pro_Close();
	return ret;
}

/* 返回ret=0正确, 其它错误 */
/* 不同的读写器, dev_close的函数内容完全不同.  */
int ScPort::dev_close(void)
{
	long lReturn;
	dev_ok = false;
	if (gCFG->hSC == -1)  return 0;

	lReturn = SCardReleaseContext(gCFG->hSC);
	if ( SCARD_S_SUCCESS != lReturn )
	{
		WLOG(ERR, "SCardReleaseContext hSC=%p Failed error=%08X", gCFG->hSC, lReturn);
		return -1;
	} else {
		WBUG("SCardReleaseContext OK, hSC=%p", gCFG->hSC);
	}
	return 0;
}

/* 返回ret=0正确, 其它错误 */
/* 不同的读写器, dev_init的函数内容完全不同.  */
int ScPort::dev_init(void)
{
	int a_len;
	long lReturn;
	DWORD cch;
	dev_ok = false;
	char *tmp;

	lReturn = SCardEstablishContext(SCARD_SCOPE_SYSTEM,  NULL, NULL, &(gCFG->hSC));
	if ( SCARD_S_SUCCESS != lReturn )
	{
		gCFG->hSC = -1;
		WLOG(ERR, "Established Context Failed error=%08X", lReturn);
		goto END;
	} else {
		WBUG("Established Context OK, hSC=%p", gCFG->hSC);
	}

	if ( gCFG->hSC == -1 ) 
		goto END;
	cch = sizeof(pmszReaders);
	tmp = &pmszReaders[0];

	hCardHandle = -1;
	lReturn = SCardListReaders(gCFG->hSC, NULL, (LPTSTR)&pmszReaders, &cch);
	if ( SCARD_S_SUCCESS != lReturn )
	{
		WLOG(ERR, "SCardListReaders(hSC=%p) ret %08X", gCFG->hSC, lReturn);
		goto END;
	} else {
		WBUG("%s", "SCardListReaders OK");
	}
	tmp = pmszReaders;
	while( (a_len = lstrlen(tmp)) >0)
	{
		WBUG("has reader %s", tmp);
		if (strncasecmp(gCFG->rd_name, tmp, strlen(gCFG->rd_name)) == 0 )	
		{	
			TEXTUS_STRCPY(gCFG->name, tmp);
			gCFG->found = true;
			//WLOG(CRIT, "found reader %s", tmp);
			break;
		}
		tmp = &tmp[a_len+1];
 	}
	
	if (!gCFG->found ) {
		WLOG(ERR, "Can not find %s", gCFG->rd_name);
		goto END;
	} else {
		WLOG(INFO, "find %s", gCFG->rd_name);
	}
	dev_ok = true;
	return 0;
END:
	return -1;
}

bool ScPort::facio( Amor::Pius *pius)
{
	void **ps;
	int *ret;
	char *comm, *reply, *uid, *atr, uid2[32];
	int *psw;
	int *isPresent;
	int *pslot;
	int which;

	assert(pius);
	switch(pius->ordo )
	{
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		if ( !gCFG->rd_name )
		{
			WLOG(ERR, "Not defined reader name!");
		}	
		break;

	case Notitia::IC_DEV_INIT:
		WBUG("facio IC_DEV_INIT");
		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		m_error_buf = (char*)ps[1];
		*ret = dev_init();
		if ( *ret == 0 ) 
			dev_ok = true;
		else
			dev_ok = false;
		break;

	case Notitia::IC_DEV_QUIT:
		WBUG("facio IC_DEV_QUIT");
		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		m_error_buf = (char*)ps[1];
		dev_ok = false;
		*ret = dev_close();
		break;

	case Notitia::IC_OPEN_PRO:
		WBUG("facio IC_OPEN_PRO dev_ok %d", dev_ok);
		if ( !dev_ok ) 
			return false;		//如果设备没有准备好, 这里根本不处理
		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		m_error_buf = (char*)ps[1];
		uid = (char*)ps[3];
		*ret = Pro_Open(uid);
		if ( *ret == 0 ) 
			card_ok = true;
		else 
			card_ok = false;
		break;

	case Notitia::IC_CLOSE_PRO:
		WBUG("facio IC_CLOSE_PRO dev_ok %d", dev_ok);
		if ( !dev_ok ) 
			return false;		//如果设备没有准备好, 这里根本不处理
		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		m_error_buf = (char*)ps[1];
		card_ok = false;
		*ret = Pro_Close();
		break;

	case Notitia::IC_SAM_COMMAND:
		WBUG("facio IC_SAM_COMMAND sam_ok %d", card_ok);
		which = 0;
		goto COMM;

	case Notitia::IC_PRO_COMMAND:
		WBUG("facio IC_PRO_COMMAND pro_ok %d", card_ok);
		which = 1;
COMM:
		if ( !card_ok ) 
			return false;		//如果用户卡没有准备好, 这里根本不处理
		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		m_error_buf = (char*)ps[1];
		if ( which == 0 ) 
		{
			pslot = (int*)ps[2];
			if ( pslot)
			{
				gCFG->sam_face = 0xFF & (*pslot);
			} else {
				gCFG->sam_face = 0;
			}
		}
		comm = (char*)ps[3];
		reply = (char*)ps[4];
		psw = (int*)(ps[5]);

		*ret = CardCommand (comm, reply, psw);
		if ( *ret != 0 ) 
		{
			card_ok = false;
		} else {
			WBUG("%s req(%s) res(%s) sw(%04x)", which==0?"SamCom":"ProCom", comm, reply, *psw);
		}
		break;

	case Notitia::IC_RESET_SAM:
		WBUG("facio IC_RESET_SAM dev_ok %d", dev_ok);
		if ( !dev_ok ) 
			return false;		//如果设备没有准备好, 这里根本不处理
		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		m_error_buf = (char*)ps[1];
		pslot = (int*)ps[2];
		if ( pslot)
		{
			gCFG->sam_face = 0xFF & (*pslot);
		} else {
			gCFG->sam_face = gCFG->def_sam_face;
		}

		atr = (char*)ps[3];
		*ret = Sam_Reset(atr);
		if ( *ret == 0 ) 
		{
			card_ok = true;
		} else {
			card_ok = false;
		}
		break;

	case Notitia::IC_PRO_PRESENT:
		WLOG(NOTICE, "facio IC_PRO_PRESENT dev_ok %d", dev_ok);
		if ( !dev_ok ) 
			return false;		//如果设备没有准备好, 这里根本不处理
		ps = (void**)(pius->indic);
		isPresent = (int*)ps[0];
		m_error_buf = (char*)ps[1];
		if ( Pro_Present() == 0)
			*isPresent = (*isPresent)+1;
		break;

	case Notitia::ICC_CARD_open:
		WBUG("facio IC_OPEN_PRO dev_ok %d", dev_ok);
		if ( !dev_ok ) return false;		//如果设备没有准备好, 这里根本不处理
		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		*ret = Pro_Open(uid2);

		if ( *ret == 0 ) 
		{
			card_ok = true;
			WBUG("Pro_Open uid %s", uid2);
			memcpy((char*)ps[3], uid2, 8);
			*(int*)ps[5] = 1;
			memcpy((char*)ps[6], "02", 2);
		} else {
			card_ok = false;
		}

		break;
	default:
		return false;
	}
	return true;
}

bool ScPort::sponte( Amor::Pius *pius)
{
	return false;
}

ScPort::ScPort()
{
	gCFG = 0 ;
	m_error_buf = (char*) 0;
	dev_ok = false;
	card_ok = false;
}

ScPort::~ScPort()
{
	if ( has_config && gCFG )
		delete gCFG;
}

Amor* ScPort::clone()
{
	ScPort *child = new ScPort();
	child->gCFG =gCFG;
	return (Amor*)child;
}
#define AMOR_CLS_TYPE ScPort
#include "hook.c"
