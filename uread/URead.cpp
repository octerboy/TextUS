#include "Amor.h"
#include "Notitia.h"
#include "casecmp.h"
#include "textus_string.h"
#include <stdio.h>

#define SCM_MODULE_ID  "$Workfile: URead.cpp $"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

typedef int (__stdcall *TP_open_dev)(char* Paras);
typedef int (__stdcall *TP_close_dev)(long DevHandle);
typedef int (__stdcall *TP_sam_reset)(long DevHandle,int iSockID,int* iReplylength,char* sReply);
typedef int (__stdcall *TP_card_open)(long DevHandle, int RequestMode,char* PhysicsCardno, char* ResetInformation, int* CardPlace, char* CardType);
typedef int (__stdcall *TP_card_close)(long DevHandle);
typedef int (__stdcall *TP_pro_command)(long DevHandle,int CardPlace, int iCommandLength,char* sCommand, int* iReplylength, char* sReply);
typedef int (__stdcall *TP_sam_command)(long DevHandle,int iSockID,int iCommandLength, char* sCommand ,int* iReplylength,char* sReply);
typedef int (__stdcall *TP_pro_detect)(long DevHandle);
typedef char* (__stdcall *TP_get_info)(int rc);

typedef bool (__stdcall *TP_GetCardNo_RFID)(char* CardNo);
typedef bool (__stdcall *TP_GetCPCID_RFID)(char* CPCID);
typedef bool (__stdcall *TP_GetFlagStationInfo_RFID)(char* CPCID,char *InitData,int *FlagStationCnt, char *FlagStationInfo);
typedef bool (__stdcall *TP_GetPowerInfo_RFID)(char* CPCID,int *PowerInfo);
typedef bool (__stdcall *TP_Set433CardMode_RFID)(char* CPCID,int iMode);
typedef bool (__stdcall *TP_Get433CardMode_RFID)(char* CPCID,int* iMode);

typedef int (__stdcall *TP_ICC_authenticate) (long DevHandle, int CardPlace,int sector,int keytype, char* key);
typedef int (__stdcall *TP_ICC_readsector) (long DevHandle, int CardPlace, int sector, int start, int len, char* data);
typedef int (__stdcall *TP_ICC_writesector)  (long DevHandle, int CardPlace, int sector, int start, int len, char* data);
typedef int (__stdcall *TP_GetReaderVersion) (long DevHandle,char* sReaderVersion, int iRVerMaxLength,char* sAPIVersion, int iAPIVerMaxlength);
typedef int (__stdcall *TP_Led_display) (long DevHandle,unsigned char cRed,unsigned char cGreen,unsigned char cBlue);
typedef int (__stdcall *TP_Audio_control) (long DevHandle,unsigned  char cBeep);


class URead: public Amor
{
public:
	
	TP_open_dev open_dev;
	TP_close_dev close_dev;

	TP_sam_reset sam_reset;
	TP_card_open card_open;
	TP_card_close card_close;
	TP_pro_command pro_command;
	TP_sam_command sam_command;
	TP_pro_detect pro_detect;
	TP_get_info get_info;

	TP_GetCardNo_RFID getcardno_rfid;
	TP_GetCPCID_RFID getcpcid_rfid;
	TP_GetFlagStationInfo_RFID getflagstationinfo_rfid;
	TP_GetPowerInfo_RFID getpowerinfo_rfid;
	TP_Set433CardMode_RFID set433cardmode_rfid;
	TP_Get433CardMode_RFID get433cardmode_rfid;

	TP_ICC_authenticate icc_authenticate;
	TP_ICC_readsector icc_readsector;
	TP_ICC_writesector icc_writesector;
	TP_GetReaderVersion getreaderversion;
	TP_Led_display led_display;
	TP_Audio_control audio_control;

	bool dev_ok;	//true:设备已经OK. 
	bool pro_ok;	//true:用户卡片已经OK, 一旦发生通讯错误, 即将此值设为false
	bool sam_ok;	//true:SAM已经OK, 一旦发生通讯错误, 即将此值设为false
	bool must_detect;	//是否一定要探卡

	int dev_init(void);
	int dev_close(void);
	int Pro_Open(char uid[17]);
	int Pro_Close();
	bool Pro_Present();
	int Sam_Reset(char atr[64]);
	int CardCommand (char* command, char* response, int which, int *sw);
	char *m_error_buf;

	URead();
	~URead();

	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
	
	int  hdev;
	int sam_face;
	char comm_para[128];

	int card_place;	//卡片位置

	char unireader_dll_real_path[1024];
	HINSTANCE unireader_dll_hd;
	int load_dll();
	int unload_dll();
	void error_sys_pro(const char *h_msg) ;

#include "wlog.h"
};
#include "md5.c"

void URead::error_sys_pro(const char *h_msg) 
{ 
    char errstr[1024];
    DWORD dw = GetLastError(); 

    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        errstr, 1024, NULL );

    wsprintf(&m_error_buf[0], "%s failed with error %d: %s", (char*)h_msg, dw, errstr); 
	WLOG(ERR, "%s", m_error_buf);
}

/* 不同的读写器有完全不同的参数配置, ignite的函数内容完全不同.  */
void URead::ignite(TiXmlElement *prop)
{
	const char *lib_file, *com;
	TiXmlElement *doc_root=0;

	const char *md5_content;
	
	doc_root = prop->GetDocument()->RootElement();

	if (!prop) return;		//这一行倒是都要的。

	com = prop->Attribute("com") ;
	if ( com ) 
	{
		TEXTUS_STRCPY(comm_para, com);
	} else {
		TEXTUS_STRCPY(comm_para, "1,COM1,115200");
	}
	prop->QueryIntAttribute("sam_slot", &sam_face);

	lib_file = prop->Attribute("dll") ;
	md5_content = prop->Attribute("dllsum") ;
	if ( lib_file )
	{
		TEXTUS_SPRINTF(unireader_dll_real_path, "%s%s", doc_root->Attribute("path"), lib_file);
		if ( md5_content)
		{
  			FILE *inFile;
  			MD5_CTX mdContext;
  			int bytes;
  			unsigned char data[1024];
			char md_str[64];
				
			if ( strlen(md5_content) < 10 )
    				goto MyEnd;

			TEXTUS_FOPEN(inFile, unireader_dll_real_path, "rb");
  			if ( !inFile ) 
    				goto MyEnd;

  			MD5Init (&mdContext);
    		MD5Update (&mdContext, (unsigned char*)"213423_13axcc3q  3qqaaa", 23);
  			while ((bytes = fread (data, 1, 1024, inFile)) != 0)
    				MD5Update (&mdContext, data, bytes);

  			MD5Final (&mdContext);
			byte2hex(mdContext.digest, 16, md_str);
  			fclose (inFile);
			md_str[32] = 0;
			if ( prop->Attribute("wantsum") )
			{
				printf("wantsum %s\n", md_str);
			}
			if ( memcmp(md_str, md5_content, 10) != 0) 
			{
    				goto MyEnd;
			}
			load_dll();
  		}
	}
MyEnd:
	return ;
}
int URead::unload_dll()
{
	int ret = 0;
	if( !FreeLibrary(unireader_dll_hd) )
	{
		error_sys_pro("unload unireader.dll") ;
		ret = -1;
	}
	unireader_dll_hd=NULL;
	return ret;
}

int URead::load_dll()
{
	int ret = 0;
	unireader_dll_hd=NULL;
	unireader_dll_hd =LoadLibrary(unireader_dll_real_path);
	if ( unireader_dll_hd ) 
	{
		open_dev = (TP_open_dev) GetProcAddress(unireader_dll_hd, "READER_open");
		close_dev = (TP_close_dev) GetProcAddress(unireader_dll_hd, "READER_close");

		sam_reset  = (TP_sam_reset) GetProcAddress(unireader_dll_hd, "SAM_reset");
		card_open =(TP_card_open)GetProcAddress(unireader_dll_hd, "CARD_open");
		card_close=(TP_card_close)GetProcAddress(unireader_dll_hd, "CARD_close");
		pro_command=(TP_pro_command)GetProcAddress(unireader_dll_hd, "PRO_command");
		sam_command=(TP_sam_command)GetProcAddress(unireader_dll_hd, "SAM_command");
		pro_detect=(TP_pro_detect)GetProcAddress(unireader_dll_hd, "PRO_detect");
		get_info =(TP_get_info)GetProcAddress(unireader_dll_hd, "GetOpInfo");
		getcardno_rfid = (TP_GetCardNo_RFID) GetProcAddress(unireader_dll_hd, "GetCardNo_RFID");
		getcpcid_rfid = (TP_GetCPCID_RFID)  GetProcAddress(unireader_dll_hd, "GetCPCID_RFID");
		getflagstationinfo_rfid = (TP_GetFlagStationInfo_RFID ) GetProcAddress(unireader_dll_hd, "GetFlagStationInfo_RFID");
		getpowerinfo_rfid = (TP_GetPowerInfo_RFID)  GetProcAddress(unireader_dll_hd, "GetPowerInfo_RFID") ;
		set433cardmode_rfid = (TP_Set433CardMode_RFID) GetProcAddress(unireader_dll_hd, "Set433CardMode_RFID") ;
		get433cardmode_rfid = (TP_Get433CardMode_RFID) GetProcAddress(unireader_dll_hd, "Get433CardMode_RFID") ;
		icc_authenticate = (TP_ICC_authenticate) GetProcAddress(unireader_dll_hd, "ICC_authenticate");
		icc_readsector = (TP_ICC_readsector) GetProcAddress(unireader_dll_hd, "ICC_readsector");
		icc_writesector = (TP_ICC_writesector) GetProcAddress(unireader_dll_hd, "ICC_writesector");
		getreaderversion = (TP_GetReaderVersion) GetProcAddress(unireader_dll_hd, "GetReaderVersion");
		led_display = (TP_Led_display) GetProcAddress(unireader_dll_hd, "Led_display") ;
		audio_control = (TP_Audio_control) GetProcAddress(unireader_dll_hd, "Audio_control");
	} else {
		error_sys_pro("load unireader.dll") ;
		ret = -1;
	}
	return ret;
}
/* 返回ret=0正确, 其它错误 */
/* 不同的读写器有完全不同的参数配置, Pro_Open的函数内容完全不同.  */
int URead::Pro_Open(char uid[17])
{
	int ret;
	char rst_info[255];
	char ctype[2];	//卡类类型
	
	ret=card_open(hdev, 0, uid, rst_info, &card_place, ctype);
	if ( ret == 0) 
	{
/*
		if ( ctype != 2 )	//必须是CPU卡
		{
			sprintf(m_error_buf, "%s() 返回类型 %0X", "CARD_open", ctype);
			ret = -100;
		}
*/
		WBUG("ProOpen uid %s ,rst_info %s , card_place %d ,ctype %s", uid, rst_info, card_place, ctype);
	} else {
		TEXTUS_SNPRINTF(m_error_buf, 128, "%s() 返回 %0X", "CARD_open" , ret);
	}
	return ret;	
}

/* 返回ret=0正确, 其它错误 */
/* 不同的读写器Pro_Close的函数内容完全不同.  */
int URead::Pro_Close()
{
	card_close(hdev);
	return 0;	
}

/* 探卡, 检测卡片是否在位(包括非接), 返回true:存在, false:不存在 */
/* 不同的读写器, Pro_Present的函数内容完全不同.  */
bool URead::Pro_Present()
{
	int  ret;
	bool has=false;
	if  ( !pro_detect)
		return has;
	ret = pro_detect(hdev);
	if ( ret == 0 ) 
		has = true;
	else
		has = false;
	return has;
}

/* 返回ret=0正确, 其它错误 */
/* 不同的读写器有完全不同的参数配置, Sam_Reset的函数内容完全不同.  */
int URead::Sam_Reset(char atr[64])
{
	int ret;
	int len;
	char resp[255];
	
	atr[0] = 0;
	ret=sam_reset(hdev, sam_face, &len, resp);
	if ( ret ==0) 
	{
		if ( len > 60 ) len = 60;
		memcpy(atr, resp, len);
		atr[len]=0;
	} else {
		TEXTUS_SNPRINTF(m_error_buf, 256, "%s(face %d) 返回 %d %s", "Sam_Reset" ,sam_face,  ret,  get_info? get_info(ret): " ");
		WLOG(ERR,"%s", m_error_buf);
	}
	return ret;	
}

/* 返回ret=0正确, 其它错误. */
/* 卡片只要有返回, 即使sw不为0x9000, 这里仍返回ret = 0 */
/* 不同的读写器有完全不同的参数配置, CardCommand的函数内容完全不同.  */
int URead::CardCommand (char* command, char* response, int which, int *sw)
{
	char tmpstr[64];
	int ret;
	int slen;
	char *fun_str;
	int slot;

	slen = 0;
	if ( which == 0 )
	{		
		slot = sam_face; //SAM卡操作
		ret = sam_command(hdev, slot, strlen(command), command, &slen, response);
		fun_str = "SAM_command";
	} else {
		ret = pro_command(hdev, card_place, strlen(command), command, &slen, response);
		fun_str = "PRO_command";
	}

	if ( slen > 510) slen = 510;
	if (ret!=0) 
	{
		TEXTUS_SPRINTF(tmpstr, "%08X",ret);
		TEXTUS_SNPRINTF(m_error_buf, 128, "%s(\"%s\")\n返回 %s", fun_str, command, tmpstr);
		*sw = 0;
	} else if ( slen < 4) {
		TEXTUS_SNPRINTF(m_error_buf, 128, "%s(\"%s\")\n返回长度 %d", fun_str, command, slen);
		*sw = 0;
		ret = -100;
	} else {
		TEXTUS_SSCANF(&response[slen-4], "%4X", sw);
		response[slen-4] = 0;
		ret = 0;
	}

	return ret;
}

/* 返回ret=0正确, 其它错误 */
/* 不同的读写器, dev_close的函数内容完全不同.  */
int URead::dev_close(void)
{
	int ret;
	if (hdev == 0 ) 
		return 0;
	ret = close_dev(hdev);
	if ( ret != 0 ) 
	{
		TEXTUS_SNPRINTF(m_error_buf, 128, "设备函数错误, READER_close返回 %d", ret);
		WLOG(ERR, "%s", m_error_buf);
	}

	hdev = 0;
	return ret;
}

/* 返回ret=0正确, 其它错误 */
/* 不同的读写器, dev_init的函数内容完全不同.  */
int URead::dev_init(void)
{
	int ret;
	if ( !open_dev || !close_dev || !sam_reset || !card_open || !card_close || !pro_command ||!sam_command )
	{
		TEXTUS_SNPRINTF(m_error_buf, 128, "动态库错误:无函数");
		ret = -1;
		WLOG(ERR, "%s", m_error_buf);
		goto END;
	}
	if ( must_detect && !pro_detect )
	{
		TEXTUS_SNPRINTF(m_error_buf, 128, "动态库错误:无探卡函数");
		ret = -1;
		WLOG(ERR, "%s", m_error_buf);
		goto END;
	}
	if (hdev != 0 )
	{
		ret = close_dev(hdev);
		hdev = 0;		
		Sleep(300);
	}

	hdev = open_dev(comm_para);		//先按设置来设
	if ( hdev > 0)
	{
		WBUG("READER_open ret handle %d", hdev);
		ret = 0;
	} else {
		TEXTUS_SNPRINTF(m_error_buf, 128, "设备打开错误, READER_open返回 %08x", hdev);
		WLOG(ERR, "%s", m_error_buf);
		ret = -1;
		hdev = 0;
	}
END:
	return ret;
}

bool URead::facio( Amor::Pius *pius)
{
	void **ps;
	int *ret;
	char *comm, *reply, *uid, *atr;
	int *psw;
	int which;
	int *isPresent;
	int *piSockID;
	char rst_info[255], uid2[32];
	char ctype[2];	//卡类类型

	assert(pius);

	switch(pius->ordo )
	{
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
		WBUG("facio IC_DEV_INIT %d %s", dev_ok, dev_ok?"":m_error_buf);
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
		if ( !dev_ok ) return false;		//如果设备没有准备好, 这里根本不处理

		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		m_error_buf = (char*)ps[1];
		uid = (char*)ps[3];
		*ret = Pro_Open(uid);
		if ( *ret == 0 ) 
			pro_ok = true;
		else
			pro_ok = false;
		break;

	case Notitia::IC_CLOSE_PRO:
		WBUG("facio IC_CLOSE_PRO dev_ok %d", dev_ok);
		if ( !dev_ok ) return false;		//如果设备没有准备好, 这里根本不处理

		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		m_error_buf = (char*)ps[1];
		pro_ok = false;
		*ret = Pro_Close();
		break;

	case Notitia::IC_SAM_COMMAND:
		WBUG("facio IC_SAM_COMMAND sam_ok %d", sam_ok);
		if ( !sam_ok ) return false;		//如果SAM卡没有准备好, 这里根本不处理

		which = 0;
		goto COMM;

	case Notitia::IC_PRO_COMMAND:
		WBUG("facio IC_PRO_COMMAND pro_ok %d", pro_ok);
		if ( !pro_ok ) return false;		//如果用户卡没有准备好, 这里根本不处理

		which = 1;
COMM:
		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		m_error_buf = (char*)ps[1];
		piSockID = (int*) ps[2];
		comm = (char*)ps[3];
		reply = (char*)ps[4];
		psw = (int*)(ps[5]);

		if (pius->ordo ==Notitia::IC_SAM_COMMAND &&  piSockID != 0 ) 
			sam_face = *piSockID; //如果指定了某个sam位置，那以后就默认用这个位置。

		if (pius->ordo ==Notitia::IC_PRO_COMMAND &&  piSockID != 0 ) 
			card_place = *piSockID; 

		*ret = CardCommand (comm, reply, which, psw);
		WBUG("%s req(%s) res(%s) %04x", which==1?"Pro":"Sam", comm, reply, *psw);
		if ( *ret != 0 ) 
		{
			if ( which == 1 ) 
				pro_ok = false;
			else
				sam_ok = false;
		}
		break;

	case Notitia::IC_RESET_SAM:
		WBUG("facio IC_RESET_SAM dev_ok %d", dev_ok);
		if ( !dev_ok ) return false;		//如果设备没有准备好, 这里根本不处理

		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		m_error_buf = (char*)ps[1];
		piSockID = (int*) ps[2];
		atr = (char*)ps[3];

		if ( piSockID != 0 ) 
			sam_face = *piSockID; //如果指定了某个sam位置，那以后就默认用这个位置。
		*ret = Sam_Reset(atr);
		if ( *ret == 0 ) 
			sam_ok = true;
		else
			sam_ok = false;

		break;
	case Notitia::IC_PRO_PRESENT:
		WBUG("facio IC_PRO_PRESENT dev_ok %d", dev_ok);
		if ( !dev_ok ) return false;		//如果设备没有准备好, 这里根本不处理

		ps = (void**)(pius->indic);
		isPresent = (int*)ps[0];
		m_error_buf = (char*)ps[1];
/*
		if (WaitForSingleObject(op_mutex, 1000) != WAIT_OBJECT_0 )
		{
			sprintf(m_error_buf, "Wait to detect card time out");
			WLOG(ERR, "%s", m_error_buf);
			*isPresent = false;
			goto PSTOP;
		} 
	PSTOP:
		ReleaseMutex(op_mutex);
*/
		if ( Pro_Present())
			*isPresent = (*isPresent)+1;
		break;

	case Notitia::ICC_Authenticate:
		WBUG("facio ICC_Authenticate dev_ok %d", dev_ok);
		if ( !dev_ok ) return false;		//如果设备没有准备好, 这里根本不处理

		ps = (void**)(pius->indic);
		*((int*)ps[0]) = icc_authenticate(hdev, *((int*)ps[2]), *((int*)ps[3]), *((int*)ps[4]), (char*)ps[5]);
		break;

	case Notitia::ICC_Read_Sector:
		WBUG("facio ICC_Read_Sector dev_ok %d", dev_ok);
		if ( !dev_ok ) return false;		//如果设备没有准备好, 这里根本不处理

		ps = (void**)(pius->indic);
		*((int*)ps[0]) = icc_readsector(hdev, *((int*)ps[2]), *((int*)ps[3]), *((int*)ps[4]), *((int*)ps[5]), (char*)ps[6]);
		break;

	case Notitia::ICC_Write_Sector:
		WBUG("facio ICC_Write_Sector dev_ok %d", dev_ok);
		if ( !dev_ok ) return false;		//如果设备没有准备好, 这里根本不处理

		ps = (void**)(pius->indic);
		*((int*)ps[0]) = icc_writesector(hdev, *((int*)ps[2]), *((int*)ps[3]), *((int*)ps[4]), *((int*)ps[5]), (char*)ps[6]);
		break;

	case Notitia::ICC_Reader_Version:
		WBUG("facio ICC_Reader_Version dev_ok %d", dev_ok);
		if ( !dev_ok ) return false;		//如果设备没有准备好, 这里根本不处理

		ps = (void**)(pius->indic);
		*((int*)ps[0]) = getreaderversion(hdev, (char*)ps[2], *((int*)ps[3]), (char*)ps[4], *((int*)ps[5]));
		break;

	case Notitia::ICC_Led_Display:
		WBUG("facio ICC_Led_Display dev_ok %d", dev_ok);
		if ( !dev_ok ) return false;		//如果设备没有准备好, 这里根本不处理

		ps = (void**)(pius->indic);
		*((int*)ps[0]) = led_display(hdev, *((unsigned char*)ps[2]), *((unsigned char*)ps[3]), *((unsigned char*)ps[4]));
		break;

	case Notitia::ICC_Audio_Control:
		WBUG("facio ICC_Audio_Control dev_ok %d", dev_ok);
		if ( !dev_ok ) return false;		//如果设备没有准备好, 这里根本不处理

		ps = (void**)(pius->indic);
		*((int*)ps[0]) = audio_control(hdev, *((unsigned char*)ps[2]));
		break;

	case Notitia::ICC_GetOpInfo:
		WBUG("facio ICC_GetOpInfo dev_ok %d", dev_ok);
		ps = (void**)(pius->indic);
		*((char**)ps[0]) = get_info(*((int*)ps[2]));
		break;

	case Notitia::ICC_Get_Card_RFID:
		WBUG("facio ICC_Get_Card_RFID dev_ok %d", dev_ok);
		if ( !dev_ok ) return false;		//如果设备没有准备好, 这里根本不处理

		ps = (void**)(pius->indic);
		*((bool*)ps[0]) = getcardno_rfid((char*)ps[2]);
		break;

	case Notitia::ICC_Get_CPC_RFID:
		WBUG("facio ICC_Get_CPC_RFID dev_ok %d", dev_ok);
		if ( !dev_ok ) return false;		//如果设备没有准备好, 这里根本不处理

		ps = (void**)(pius->indic);
		*((bool*)ps[0]) = getcpcid_rfid((char*)ps[2]);
		break;

	case Notitia::ICC_Get_Flag_RFID:
		WBUG("facio ICC_Get_Flag_RFID dev_ok %d", dev_ok);
		if ( !dev_ok ) return false;		//如果设备没有准备好, 这里根本不处理

		ps = (void**)(pius->indic);
		*((bool*)ps[0]) = getflagstationinfo_rfid( (char*)ps[2], (char*)ps[3], (int*)ps[4], (char*)ps[5]);
		break;

	case Notitia::ICC_Get_Power_RFID:
		WBUG("facio ICC_Get_Power_RFID dev_ok %d", dev_ok);
		if ( !dev_ok ) return false;		//如果设备没有准备好, 这里根本不处理

		ps = (void**)(pius->indic);
		*((bool*)ps[0]) = getpowerinfo_rfid( (char*)ps[2], (int*)ps[3]);
		break;

	case Notitia::ICC_Set433_Mode_RFID:
		WBUG("facio ICC_Set433_Mode_RFID dev_ok %d", dev_ok);
		if ( !dev_ok ) 	return false;		//如果设备没有准备好, 这里根本不处理

		ps = (void**)(pius->indic);
		*((bool*)ps[0]) = set433cardmode_rfid( (char*)ps[2], *(int*)ps[3]);
		break;

	case Notitia::ICC_Get433_Mode_RFID:
		WBUG("facio ICC_Get433_Mode_RFID dev_ok %d", dev_ok);
		if ( !dev_ok ) 	return false;		//如果设备没有准备好, 这里根本不处理

		ps = (void**)(pius->indic);
		*((bool*)ps[0]) = get433cardmode_rfid( (char*)ps[2], (int*)ps[3]);
		break;

	case Notitia::ICC_CARD_open:
		WBUG("facio IC_OPEN_PRO dev_ok %d", dev_ok);
		if ( !dev_ok ) return false;		//如果设备没有准备好, 这里根本不处理

		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		*ret=card_open(hdev, *(int*)ps[2], uid2, rst_info, &card_place, ctype);
		if ( *ret == 0 ) 
		{
			pro_ok = true;
			WBUG("ProOpen uid %s ,rst_info %s , card_place %d ,ctype %s", uid2, rst_info, card_place, ctype);
			memcpy((char*)ps[3], uid2, 8);
			memcpy( (char*)ps[4], rst_info, strlen(rst_info));
			*(int*)ps[5] = card_place;
			memcpy((char*)ps[6], ctype, 2);
		} else {
			pro_ok = false;
			*ret = -100;
		}

		break;

	case Notitia::URead_UnLoad_Dll:
		WBUG("facio URead_UnLoad_Dll");
		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		m_error_buf = (char*)ps[1];

		*ret = unload_dll();
		break;

	case Notitia::URead_Load_Dll:
		WBUG("facio URead_Load_Dll");
		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		m_error_buf = (char*)ps[1];

		*ret = load_dll();
		break;

	case Notitia::URead_ReLoad_Dll:
		WBUG("facio URead_ReLoad_Dll");
		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		m_error_buf = (char*)ps[1];
		*ret = unload_dll(); if ( *ret !=0 ) break;
		*ret = load_dll();
		break;

	default:
		return false;
	}
	return true;
}

bool URead::sponte( Amor::Pius *pius)
{
	return false;
}

URead::URead()
{
	m_error_buf = (char*) 0;
	hdev = 0;
	dev_ok = false;
	pro_ok = false;
	sam_ok = false;
	must_detect = false;

	open_dev = 0;
	close_dev = 0;

	card_open =0 ;
	card_close =0 ;
	pro_command =0 ;
	sam_command =0 ;
	pro_detect =0 ;
	sam_reset =0 ;

	unireader_dll_hd = NULL;
}

URead::~URead()
{
}

Amor* URead::clone()
{
	return (Amor*)this;
}
#define AMOR_CLS_TYPE URead
#include "hook.c"
