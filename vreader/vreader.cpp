#pragma pack(1)
#include "Amor.h"
#include "Notitia.h"
#include "casecmp.h"
#include "textus_string.h"
#include "PacData.h"
#include "UniReader.h"
//#include "BTool.h"
#define SCM_MODULE_ID  "$Workfile: vreader.cpp $"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */
#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>
#include <stdio.h>
#include <conio.h>  
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <process.h>
#include "md5.c"

/* VR_toll 工作状态，这是unitoll动态库来调用,
   VR_samin 工作状态， SAMIN客户端控制 
*/
enum VReader_Who { VR_toll=1,  VR_samin =3, VR_none = 0 };
typedef struct _PSAM_Info{
	char serial[64];	//PSAM卡序列号
	char device_termid[64];	//终端机编号
	char rst_info[128];	//Psam复位信息
	char datetime[32];	//盘点时间，车道时间
	int slot;		//PSAM卡所在槽号1~4
	int result;	//0：正常，1：被锁，2：无卡，3：伪卡，4：未检测，5：检测失败
	int type;	//1: 国标, 2：地标, 0:未知
	char err[128];
	char desc[128];
} PsamInfo;

#define PSAM_SLOT_NUM 4
#pragma data_seg("v_reader_status")

	static enum VReader_Who who_open_reader = VR_none ;	//设备是否初始化, 若已经初始化, 每次计数++
	bool had_toll = false;	//若unitoll动态库已经调用，则设相应值
	bool had_samin = false;	//若samin已经调用，则设相应值

	static PsamInfo samory[PSAM_SLOT_NUM+1]={{" "," "," "," ",0,2,0,"",""}, {" "," "," "," ",0,2,0,"",""}, {" "," "," "," ",0,2,0,"",""}, {" "," "," "," ",0,2,0,"",""}, {" "," "," "," ",0,2,0,"",""}};	//假定最多有4张卡,第0个不用。
	static char lane_ip[64]= " " ;		//车道 IP
	static char lane_road[16]= " " ;
	static char lane_station[16]= " " ;
	static char lane_no[16]= " " ;
	static char psam_challenge[32]= " " ;	//来自中心的挑战数
	static char psam_should_cipher_db44[32]= " " ;//来自中心的应该的加密结果, 针对地标PSAM卡 
	static char psam_should_cipher_gb[32]= " " ;//来自中心的应该的加密结果, 针对国标PSAM卡
	static char road_magic[64] = "\0";

#pragma data_seg()

#pragma comment(linker,"/SECTION:v_reader_status,RWS")

static enum VReader_Who me_who = VR_none;	//自身的工作模式， 这不在共享数据段
static bool isInventoring;	//是否在盘点
static bool has_card;

static int max_Qry_Card_num;
static int cent_qry_interval=1; //向中心查询的时间间隔

const char *me_who_str=0;
const char *samin_named_pipe_str = "\\\\.\\pipe\\NamePipe_samin_serv";  
const char *toll_named_pipe_str = "\\\\.\\pipe\\NamePipe_toll_serv";  

static TiXmlElement *road_ele=0;	//路段表

class ICPort: public Amor
{
public:
	ICPort();
	~ICPort();

	void ignite(TiXmlElement *i);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
	void deliver(Notitia::HERE_ORDO aordo);

	enum WMOD { WM_TOP=0, WM_SAM= 1, WM_PRO=2, WM_BOTH=3};
	enum WMOD wmod;	//工作模式: 0:SAM, 1:PRO, 2:both, 默认TOP,即顶层实例

	int init_timeout;
	bool will_reload_dll;

	void error_sys_pro(const char *h_msg);
	void rcv_pipe();
	bool get_cli_pipe();
	void close_cli_pipe();
	void notify_me_up();
	void notify_me_down();
	void notify_friend(char *msg);
	void SysInit();
	int open_dev();
	int close_dev(bool should_notify=true);
	int reload_dll();

	bool inventory();

	PacketObj hi_req, hi_reply; /* 向中心传递的 */
	PacketObj *hipa[3];
	Amor::Pius loc_pro_pac;
	void center_pac();
	void to_center_up();
	void to_center_ventory(bool can);
	void to_center_query();
	//以上仅用于顶层实例

	bool dev_ok;	//顶层实例和二层实例

	char reader_ch;	//读写器特征字，以下仅用于二层实例， 顶层不用。
	char wm_str[16];
	void relay_dev_init();
	void *devInitPs[4];	//最后一个指向读写器字符
	int  devInitRet;	//
	HANDLE h_init_thread;
#include "wlog.h"
};

static HANDLE h_recv_pipe_thread = INVALID_HANDLE_VALUE;	/* 接收管道线程 */
static HANDLE h_srv_pipe= INVALID_HANDLE_VALUE;
static HANDLE h_cli_pipe = INVALID_HANDLE_VALUE;
static HANDLE pro_ev=NULL;		//event变量, 指示有工作要做
static HANDLE samin_no_read_ev=NULL;		//event变量, 指示有samin已经关闭对读写器的连接。

static Amor *mary=(Amor*)0;	//ignite时设这个值, 函数就调用mary啦
static ICPort *justme=(ICPort*)0;	//ignite时设这个值, 函数就调用mary啦

static char ok_reader=0;	//成功初始化的读写器字
static char m_sysinfo_buf[2048] = {0};
char cur_path[1024];
char m_error_buf[1024] = {0};


#define HAS_NOT_OPEN_CARD	-1111	//假定这个值都不用, 读写器底层不返回这样的码

static int CardCommandCharSlot(char * command, char * reply, SHORT which, int *psw,  int *pslot)
{
	int ret;
	void *ps[7];
	Amor::Pius para;

	ret = -1;
	m_error_buf[0] = 0;
	reply[0]= 0;
	*psw = 0;
	if ( !justme )
	{
		TEXTUS_SPRINTF(m_error_buf, "system is not ready!");
		goto END;
	}
	ps[0] = &ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) pslot;	
	ps[3] = command;
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
		para.ordo = Notitia::TEXTUS_RESERVED;
		break;
	}
	
	mary->facio(&para);
END:
	return ret;
}

static int CardCommandCharReader(SHORT which, int iSockID,int iCommandLength, char* sCommand ,int* iReplylength,char* sReply)
{
	int ret;
	char command[1024];
	int ipsw;
	unsigned char byte[8];
	
	if ( iCommandLength > 1000  ) iCommandLength = 1000;
	memcpy(command, sCommand, iCommandLength);
	command[iCommandLength] = 0;
	if ( iCommandLength > 64 && strncasecmp(command, "80DC44C83C443A00", 16) ==0 ) 
	{
		if (memcmp(&command[36], "01", 2) == 0 ||
			memcmp(&command[36], "03", 2) == 0 ||
			memcmp(&command[36], "05", 2) == 0 ||
			memcmp(&command[36], "07", 2) == 0 ||
			memcmp(&command[36], "09", 2) == 0 ) //入口
		{
			hex2byte(byte, 1 , &command[20]);
			TEXTUS_SPRINTF(lane_road, "%d", byte[0]);
			hex2byte(byte, 1 , &command[22]);
			TEXTUS_SPRINTF(lane_station, "%d", byte[0]);
			hex2byte(byte, 1 , &command[24]);
			TEXTUS_SPRINTF(lane_no, "%d", byte[0]);
		}
	}
	ret = CardCommandCharSlot(command, sReply, which, &ipsw,  &iSockID);
	if ( ret ==0 ) 
	{
		*iReplylength = strlen(sReply);
		sprintf(&sReply[*iReplylength], "%04X", ipsw);
		*iReplylength = *iReplylength + 4;
		sReply[*iReplylength] = 0;
	}
	return ret;
}

static void error_load_pro(const char* so_file) 
{ 
    char errstr[1024];
    DWORD dw = GetLastError(); 

    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        errstr, 1024, NULL );

    wsprintf(&m_error_buf[0], "Load module(%s) failed with error %d: %s", (char*)so_file, dw, errstr); 
    MessageBox(NULL, (const char*)&m_error_buf[0], TEXT("Error"), MB_OK); 
}

typedef int (*Start_fun)(int , char *[]);
static struct ParaTh {
	Start_fun run;
	int argc;
	char *argv[3];
	char xml_file[1024];
	ParaTh ()
	{
		argc = 2;
		argv[0] = "vreader";
		argv[1] = &xml_file[0];
	};
	void go()
	{
		run(argc, argv); 
	};
	
} pth;

int __stdcall  READER_open(char* Paras)
{
	HMODULE hMod ;
	HINSTANCE ext=NULL;
	int num = 10;
	int ret = -1;
	char *p;

	if ( !mary) 
	{
		hMod = GetModuleHandle("vreader.dll");
		if(hMod != NULL) 
		{
			GetModuleFileName(hMod, cur_path, 1000);
			p = strstr(cur_path, "vreader.dll");
			*p= 0;
		}

		if ( (ext =LoadLibrary("libanimus.dll") ) 
			&& ( pth.run = (Start_fun) GetProcAddress(ext, "textus_animus_start") ) )
		{ 
			strcpy(pth.xml_file, cur_path);
			strcat(pth.xml_file, "vtoll.xml");
			pth.go();
		} else {
			error_load_pro("libanimus");
			goto END;
		}

		while (mary == (Amor *) 0 && num > 0 )
		{
			num--;
			Sleep(100);
		}
	
		if ( mary == (Amor *) 0 )
		{
			wsprintf(&m_error_buf[0], "vreader's mary is still null");
			goto END;
		}
	}
	ret = justme->open_dev();
	if (ret ) 
	{
		if ( justme->inventory() )
			justme->notify_friend("p");	//告诉samin盘点完成。
		else 
			justme->notify_friend("q");	//告诉samin盘点无法进行。
	}

END:
	return ret;
}

int __stdcall READER_close(long DevHandle)
{
	int ret = 0;
	ret = -1;
	m_error_buf[0] = 0;
	if ( !justme)
	{
		TEXTUS_SPRINTF(m_error_buf, "system is not ready!");
		goto END;
	}
	ret = justme->close_dev();
END:
	return ret;
}


int __stdcall SAM_reset(long DevHandle,int iSockID,int* iReplylength,char* sReply)
{
	int ret;
	void *ps[4];
	Amor::Pius para;

	ret = -1;
	m_error_buf[0] = 0;
	if ( !justme)
	{
		TEXTUS_SPRINTF(m_error_buf, "system is not ready!");
		goto END;
	}
	ps[0] = &ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) &iSockID;	//0不指定slot
	ps[3] = &sReply[0];
	para.indic = ps;
	para.ordo = Notitia::IC_RESET_SAM;
	
	mary->facio(&para);
	*iReplylength = strlen(sReply);
END:
	return ret;
}


int   __stdcall SAM_command(long DevHandle,int iSockID,int iCommandLength, char* sCommand ,int* iReplylength,char* sReply)
{
	return CardCommandCharReader(0, iSockID, iCommandLength, sCommand ,iReplylength, sReply);
}

int   __stdcall PRO_command(long DevHandle,int CardPlace, int iCommandLength,char* sCommand, int* iReplylength, char* sReply)
{
	return CardCommandCharReader(1, CardPlace, iCommandLength, sCommand ,iReplylength, sReply);
}

int   __stdcall CARD_open(long DevHandle, int RequestMode,char* PhysicsCardno, char* ResetInformation, int* CardPlace, char* CardType)
{
	int ret = -1;
	void *ps[8];
	Amor::Pius para;
	int find_cards;

	ret = HAS_NOT_OPEN_CARD;
	m_error_buf[0] = 0;

	if ( !justme)
	{
		TEXTUS_SPRINTF(m_error_buf, "system is not ready!");
		goto END;
	}

	if ( isInventoring ) 
	{
		goto END;
	}

	find_cards = 0;
	ps[0] = &ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) &RequestMode;
	ps[3] = &PhysicsCardno[0];
	ps[4] = &ResetInformation[0];
	ps[5] = (void*)CardPlace;
	ps[6] = &CardType[0];
	ps[7] = &find_cards;
	para.indic = ps;
	para.ordo = Notitia::ICC_CARD_open;
	
	mary->facio(&para);
	if ( isInventoring )	//最后再检查一下
	{
		ret = -1;
		goto END;
	}
	
	if ( find_cards > 1 ) 
	{
		char msg[64];
		sprintf(msg, "同时有%d张卡在读写器上, 请拿走其中的%d张。", find_cards, find_cards-1);
		MessageBox(NULL, msg, TEXT("VReader"), MB_OK);
	}
	
	if (ret ==0 ) 
		has_card = true;
	else
		has_card = false;
END:
	return ret;
}

int   __stdcall CARD_close(long DevHandle)
{
	int ret;
	void *ps[2];
	Amor::Pius para;

	ret = -1;
	m_error_buf[0] = 0;
	if ( !justme)
	{
		TEXTUS_SPRINTF(m_error_buf, "system is not ready!");
		goto END;
	}
	ps[0] = &ret;
	ps[1] = &m_error_buf[0];
	para.indic = ps;
	para.ordo = Notitia::IC_CLOSE_PRO;
	
	mary->facio(&para);
END:
	return ret;
}

int   __stdcall ICC_authenticate(long DevHandle, int CardPlace,int sector,int keytype, char* key)
{
	int ret;
	void *ps[6];
	Amor::Pius para;

	ret = -1;
	m_error_buf[0] = 0;
	if ( !justme)
	{
		TEXTUS_SPRINTF(m_error_buf, "system is not ready!");
		goto END;
	}
	ps[0] = (void*)&ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) &CardPlace;
	ps[3] = (void*) &sector;
	ps[4] = (void*) &keytype;
	ps[5] = &key[0];
	para.indic = ps;
	para.ordo = Notitia::ICC_Authenticate;
	
	mary->facio(&para);
END:
	return ret;
}

int   __stdcall ICC_readsector(long DevHandle, int CardPlace, int sector, int start, int len, char* data)
{
	int ret;
	void *ps[7];
	Amor::Pius para;

	ret = -1;
	m_error_buf[0] = 0;
	if ( !justme)
	{
		TEXTUS_SPRINTF(m_error_buf, "system is not ready!");
		goto END;
	}
	ps[0] = (void*)&ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) &CardPlace;
	ps[3] = (void*) &sector;
	ps[4] = (void*) &start;
	ps[5] = (void*) &len;
	ps[6] = &data[0];
	para.indic = ps;
	para.ordo = Notitia::ICC_Read_Sector;
	
	mary->facio(&para);
END:
	return ret;
}

int   __stdcall ICC_writesector(long DevHandle, int CardPlace, int sector, int start, int len, char* data)
{
	int ret;
	void *ps[7];
	Amor::Pius para;

	ret = -1;
	m_error_buf[0] = 0;
	if ( !justme)
	{
		TEXTUS_SPRINTF(m_error_buf, "system is not ready!");
		goto END;
	}
	ps[0] = (void*)&ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) &CardPlace;
	ps[3] = (void*) &sector;
	ps[4] = (void*) &start;
	ps[5] = (void*) &len;
	ps[6] = &data[0];
	para.indic = ps;
	para.ordo = Notitia::ICC_Write_Sector;
	
	mary->facio(&para);
END:
	return ret;
}

int   __stdcall GetReaderVersion(long DevHandle,char* sReaderVersion, int iRVerMaxLength,char* sAPIVersion, int iAPIVerMaxlength)
{
	int ret;
	void *ps[6];
	Amor::Pius para;

	ret = -1;
	m_error_buf[0] = 0;
	if ( !justme)
	{
		TEXTUS_SPRINTF(m_error_buf, "system is not ready!");
		goto END;
	}
	ps[0] = (void*)&ret;
	ps[1] = &m_error_buf[0];
	ps[2] = &sReaderVersion[0];
	ps[3] = (void*) &iRVerMaxLength;
	ps[4] = &sAPIVersion[0];
	ps[5] = (void*) &iAPIVerMaxlength;

	para.indic = ps;
	para.ordo = Notitia::ICC_Reader_Version;
	
	mary->facio(&para);
END:
	return ret;
}

int  __stdcall Led_display(long DevHandle,unsigned char cRed,unsigned char cGreen,unsigned char cBlue)
{
	int ret;
	void *ps[5];
	Amor::Pius para;

	ret = -1;
	m_error_buf[0] = 0;
	if ( !justme)
	{
		TEXTUS_SPRINTF(m_error_buf, "system is not ready!");
		goto END;
	}
	ps[0] = (void*)&ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) &cRed;
	ps[3] = (void*) &cGreen;
	ps[4] = (void*) &cBlue;

	para.indic = ps;
	para.ordo = Notitia::ICC_Led_Display;
	
	mary->facio(&para);
END:
	return ret;
}

int  __stdcall Audio_control(long DevHandle,unsigned  char cBeep)
{
	int ret;
	void *ps[3];
	Amor::Pius para;

	ret = -1;
	m_error_buf[0] = 0;
	if ( !justme)
	{
		TEXTUS_SPRINTF(m_error_buf, "system is not ready!");
		goto END;
	}
	ps[0] = (void*)&ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) &cBeep;

	para.indic = ps;
	para.ordo = Notitia::ICC_Audio_Control;
	
	mary->facio(&para);
END:
	return ret;
}

char* __stdcall GetOpInfo(int retcode)
{
	char *ret;
	void *ps[3];
	Amor::Pius para;

	ret = 0;
	m_error_buf[0] = 0;
	if ( !justme)
	{
		TEXTUS_SPRINTF(m_error_buf, "system is not ready!");
		goto END;
	}
	ps[0] = (void*) &ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) &retcode;

	para.indic = ps;
	para.ordo = Notitia::ICC_GetOpInfo;
	
	mary->facio(&para);
END:
	if ( retcode == 999987 )
		printf("inventory %d\n",justme->inventory());
	return ret;
}

bool  __stdcall GetCardNo_RFID(char* CardNo)
{
	bool ret;
	void *ps[3];
	Amor::Pius para;

	ret = false;
	m_error_buf[0] = 0;
	if ( !justme)
	{
		TEXTUS_SPRINTF(m_error_buf, "system is not ready!");
		goto END;
	}
	ps[0] = (void*) &ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) &CardNo[0];

	para.indic = ps;
	para.ordo = Notitia::ICC_Get_Card_RFID;
	
	mary->facio(&para);
END:
	return ret;
}

bool  __stdcall GetCPCID_RFID(char* CPCID)
{
	bool ret;
	void *ps[3];
	Amor::Pius para;

	ret = false;
	m_error_buf[0] = 0;
	if ( !justme)
	{
		TEXTUS_SPRINTF(m_error_buf, "system is not ready!");
		goto END;
	}
	ps[0] = (void*) &ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) &CPCID[0];

	para.indic = ps;
	para.ordo = Notitia::ICC_Get_CPC_RFID;
	
	mary->facio(&para);
END:
	return ret;

}

bool  __stdcall GetFlagStationInfo_RFID(char* CPCID,char *InitData,int *FlagStationCnt, char *FlagStationInfo)
{
	bool ret;
	void *ps[5];
	Amor::Pius para;

	ret = false;
	m_error_buf[0] = 0;
	if ( !justme)
	{
		TEXTUS_SPRINTF(m_error_buf, "system is not ready!");
		goto END;
	}
	ps[0] = (void*) &ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) CPCID;
	ps[3] = (void*) InitData;
	ps[4] = (void*) FlagStationCnt;
	ps[5] = (void*) FlagStationInfo;

	para.indic = ps;
	para.ordo = Notitia::ICC_Get_Flag_RFID;
	mary->facio(&para);

END:
	return ret;

}

bool  __stdcall GetPowerInfo_RFID(char* CPCID,int *PowerInfo)
{
	bool ret;
	void *ps[4];
	Amor::Pius para;

	ret = false;
	m_error_buf[0] = 0;
	if ( !justme)
	{
		TEXTUS_SPRINTF(m_error_buf, "system is not ready!");
		goto END;
	}
	ps[0] = (void*) &ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) &CPCID[0];
	ps[3] = (void*) PowerInfo;

	para.indic = ps;
	para.ordo = Notitia::ICC_Get_Power_RFID;
	
	mary->facio(&para);
END:
	return ret;
}

bool  __stdcall Set433CardMode_RFID(char* CPCID,int iMode)
{
	bool ret;
	void *ps[4];
	Amor::Pius para;

	ret = false;
	m_error_buf[0] = 0;
	if ( !justme)
	{
		TEXTUS_SPRINTF(m_error_buf, "system is not ready!");
		goto END;
	}
	ps[0] = (void*) &ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) &CPCID[0];
	ps[3] = (void*) &iMode;

	para.indic = ps;
	para.ordo = Notitia::ICC_Set433_Mode_RFID;
	
	mary->facio(&para);
END:
	return ret;
}

bool  __stdcall Get433CardMode_RFID(char* CPCID,int* iMode)
{
	bool ret;
	void *ps[4];
	Amor::Pius para;

	ret = 0;
	m_error_buf[0] = 0;
	if ( !justme)
	{
		TEXTUS_SPRINTF(m_error_buf, "system is not ready!");
		goto END;
	}
	ps[0] = (void*) &ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) &CPCID[0];
	ps[3] = (void*) iMode;

	para.indic = ps;
	para.ordo = Notitia::ICC_Get433_Mode_RFID;
	
	mary->facio(&para);
END:
	return ret;
}

typedef void (__cdecl *my_thread_func)(void*);

static void  rcv_pipe_thrd(ICPort  *arg)
{
	arg->rcv_pipe();
	_endthreadex(0);
}

static void  dev_init_thrd(ICPort  *arg)
{
	arg->relay_dev_init();
	_endthreadex(0);
}


void ICPort::relay_dev_init() //这是二层实例的运作
{
	Amor::Pius pius;

	pius.ordo = Notitia::IC_DEV_INIT;
	pius.indic = devInitPs;
	devInitRet = -1;
	aptus->facio(&pius);	//向实际的读卡器模块发初始化
	if ( devInitRet == 0 ) 
	{
		pius.ordo = Notitia::IC_DEV_INIT_BACK;
		*((char*)devInitPs[3]) =  this->reader_ch;
		aptus->sponte(&pius);		//向顶层实例报告设备初始化成功
		dev_ok = true;
	} else {
		dev_ok = false;
	}
}

void ICPort::error_sys_pro(const char *h_msg) 
{ 
    char errstr[1024];
    DWORD dw = GetLastError(); 
	char *s;

    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        errstr, 1024, NULL );

	s= strstr(errstr, "\r\n") ; 
	if (s )  *s = '\0'; 
    wsprintf(&m_error_buf[0], "%s failed with error %d: %s", (char*)h_msg, dw, errstr); 
	WLOG(ERR, "%s", m_error_buf);
}


bool ICPort::facio( Amor::Pius *pius)
{
	bool ret;
	int *pexist, exist_old;
	void **ps;
	int *iRet, iRet_old, *found_card;

	int *pslot;
	Amor::Pius tmp_pius;

	ret = true;
	if ( h_init_thread != INVALID_HANDLE_VALUE ) 
	{
		WaitForSingleObject( h_init_thread, 10000); //等10秒, 不管怎么样, 关闭线程
		CloseHandle(h_init_thread);
		h_init_thread = INVALID_HANDLE_VALUE;
	}
	switch ( pius->ordo )
	{
	case Notitia::MAIN_PARA:
/* 如果已经有，则不再设。这样, 如果ICPort下还有ICPort， 即vreader在xml中配置的嵌套, justme是最顶层的实例。 */
/* 不会调用justme->facio, 而mary->facio将调用各子ICPort. justme这个顶层实例不会收到facio*/
		WBUG("facio MAIN_PARA %p", justme);
		if (!mary) 
		{
			mary = this->aptus;	
			justme = this;	
		}

		if ( wmod == WM_TOP ) 
			SysInit(); /* 顶层实例才做各种初始化 */

		ret = false;
/* 这样子, 如同最顶级调用, 到时以mary->facio的方式, 传到下级各节点. 各节点可能也是vreader，然后再各读写器. 
   ICPort的facio函数， 就知道是二层节点的unigo了。
*/
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		if ( wmod == WM_TOP ) 
		{	/* 顶层实例才做各种初始化 */
			hi_req.produce(32) ;
			hi_reply.produce(32) ;
			tmp_pius.indic = &hipa[0];
			tmp_pius.ordo = Notitia::SET_UNIPAC;
			aptus->facio(&tmp_pius);
			if ( !had_toll && !had_samin )
			{
				memset(lane_ip,0,sizeof(lane_ip));	lane_ip[0] = ' ';
				memset(psam_challenge, 0, sizeof(psam_challenge));
				memset(psam_challenge,'0', 16);
				memset(psam_should_cipher_db44,0,sizeof(psam_should_cipher_db44));
				memset(psam_should_cipher_gb,0,sizeof(psam_should_cipher_gb));

				memset(lane_road,0,sizeof(lane_road)); lane_road[0] = ' ';
				memset(lane_station,0,sizeof(lane_station)); lane_station[0] = ' ';
				memset(lane_no,0,sizeof(lane_no)); lane_no[0] = ' ';
			}

			isInventoring = false;	//不在盘点
			has_card = false;	
		}
		dev_ok = false; //包括顶层实例。
		break;

	case Notitia::IC_DEV_INIT:
		WBUG("facio IC_DEV_INIT ");
		ps = (void**)(pius->indic);
		devInitPs[0] = &devInitRet;	//在这里，上一级的ret被替换了，所上一级的ret不再真实。
		devInitPs[1] = ps[1];
		devInitPs[2] = ps[2];
		devInitPs[3] = ps[3];

		if (  (h_init_thread=(HANDLE)_beginthread((my_thread_func)dev_init_thrd, 1024000, this)) == INVALID_HANDLE_VALUE )
		{
			WLOG(ERR, "_beginthread error= %08x",  GetLastError());
		}
		break;

	case Notitia::IC_DEV_QUIT:
		
		if ( dev_ok)
		{
			WBUG("facio IC_DEV_QUIT, %s", dev_ok ? "dev_ok" : "dev_failed");
			aptus->facio(pius);
			dev_ok = false;
		}
		
		break;

	case Notitia::IC_OPEN_PRO:
		if ( wmod != WM_SAM && dev_ok)
		{
			WBUG("facio IC_OPEN_PRO %s", wm_str);
			ps = (void**)(pius->indic);
			iRet = (int*)ps[0];
			iRet_old = *iRet;		//原值
			aptus->facio(pius);
			ps = (void**)(pius->indic);
			iRet = (int*)ps[0];
			if ( iRet_old == 0 )	//原来卡片就OK的, 保持
			{
				*iRet = 0;
			} else if ( *iRet == -100 && iRet_old != HAS_NOT_OPEN_CARD ) 	//原来未成功的, 这里的-100最后才保持, -100表示无卡
			{
				*iRet = iRet_old;	//赋回原值，当前是无卡，保持上一个读写器访问卡片的状态.
			} 
		}
		break;

	case Notitia::IC_CLOSE_PRO:
		if ( wmod != WM_SAM && dev_ok)
		{
			WBUG("facio IC_CLOSE_PRO");
			ret = false;
		}
		break;

	case Notitia::ICC_Authenticate:
		if ( wmod != WM_SAM && dev_ok)
		{
			WBUG("facio ICC_Authenticate");
			ret = false;
		}
		break;

	case Notitia::ICC_Read_Sector:
		if ( wmod != WM_SAM && dev_ok)
		{
			WBUG("facio ICC_Read_Sector");
			ret = false;
		}
		break;

	case Notitia::ICC_Write_Sector:
		if ( wmod != WM_SAM && dev_ok)
		{
			WBUG("facio ICC_Write_Sector");
			ret = false;
		}
		break;

	case Notitia::ICC_Reader_Version:
		if ( wmod != WM_SAM && dev_ok)
		{
			WBUG("facio ICC_Reader_Version");
			ret = false;
		}
		break;

	case Notitia::ICC_Led_Display:
		if ( wmod != WM_SAM &&	dev_ok)
		{
			WBUG("facio ICC_Led_Display");
			ret = false;
		}
		break;

	case Notitia::ICC_Audio_Control:
		if ( wmod != WM_SAM && dev_ok)
		{
			WBUG("facio ICC_Audio_Control");
			ret = false;
		}
		break;

	case Notitia::ICC_GetOpInfo:
		if ( wmod != WM_SAM && dev_ok)
		{
			WBUG("facio ICC_GetOpInfo");
			ret = false;
		}
		break;

	case Notitia::ICC_Get_Card_RFID:
		if ( wmod != WM_SAM && dev_ok)
		{
			WBUG("facio ICC_Get_Card_RFID");
			ret = false;
		}
		break;

	case Notitia::ICC_Get_CPC_RFID:
		if ( wmod != WM_SAM && dev_ok)
		{
			WBUG("facio ICC_Get_CPC_RFID");
			ret = false;
		}
		break;

	case Notitia::ICC_Get_Flag_RFID:
		if ( wmod != WM_SAM && dev_ok)
		{
			WBUG("facio ICC_Get_Flag_RFID");
			ret = false;
		}
		break;

	case Notitia::ICC_Get_Power_RFID:
		if ( wmod != WM_SAM && dev_ok)
		{
			WBUG("facio ICC_Get_Power_RFID");
			ret = false;
		}
		break;

	case Notitia::ICC_Set433_Mode_RFID:
		if ( wmod != WM_SAM && dev_ok)
		{
			WBUG("facio ICC_Set433_Mode_RFID");
			ret = false;
		}
		break;

	case Notitia::ICC_Get433_Mode_RFID:
		if ( wmod != WM_SAM && dev_ok)
		{
			WBUG("facio ICC_Get433_Mode_RFID");
			ret = false;
		}
		break;

	case Notitia::ICC_CARD_open:
		if ( wmod != WM_SAM && dev_ok)
		{
			//WBUG("facio ICC_CARD_open %s", wm_str);
			ps = (void**)(pius->indic);
			iRet = (int*)ps[0];
			found_card = (int*)ps[7];
			iRet_old = *iRet;		//原值
			aptus->facio(pius);
			ps = (void**)(pius->indic);
			iRet = (int*)ps[0];
			if ( *iRet ==0 ) 
				*found_card =  *found_card + 1;
			if ( iRet_old == 0 )	//原来卡片就OK的, 保持
			{
				*iRet = 0;
			} else if ( *iRet == -100 && iRet_old != HAS_NOT_OPEN_CARD ) 	//原来未成功的, 这里的-100最后才保持, -100表示无卡
			{
				*iRet = iRet_old;	//赋回原值，当前是无卡，保持上一个读写器访问卡片的状态.
			} 
		}
		break;

	case Notitia::IC_PRO_COMMAND:
		if ( wmod != WM_SAM && dev_ok )
		{
			//WBUG("facio IC_PRO_COMMAND %s", wm_str);
			ret = false;	//卡操作由animus继续向右
		} else 
			break;
		ps = (void**)(pius->indic);
		goto COMM_PRO;
		break;

	case Notitia::IC_SAM_COMMAND:
		if ( wmod != WM_PRO && dev_ok )
		{
			//WBUG("facio IC_SAM_COMMAND %s", wm_str);
			ret = false;	//卡操作由animus继续向右
		} else 
			break;
		ps = (void**)(pius->indic);
		pslot = (int*)ps[2];
		//printf("ret .. %d, pslot %d map %d, i %d\n", ret, *pslot, sam_num_map, i);
COMM_PRO:
		iRet = (int*)ps[0];
		break;

	case Notitia::IC_RESET_SAM:
		if ( wmod != WM_PRO && dev_ok )
		{
			WBUG("facio IC_RESET_SAM %s", wm_str);
			ps = (void**)(pius->indic);
			iRet = (int*)ps[0];
			iRet_old = *iRet;		//原值
			pslot = (int*)ps[2];
			iRet = (int*)ps[0];

			aptus->facio(pius);
			ps = (void**)(pius->indic);
			iRet = (int*)ps[0];
			if ( *iRet != 0 ) 
				*iRet = iRet_old;	//赋回原值，也就是说，卡片未连上的, 不更新原来的值
		}
		break;

	case Notitia::IC_PRO_PRESENT:
		if ( wmod != WM_SAM && dev_ok )
		{
			//WBUG("facio IC_PRO_PRESENT %s", wm_str);
			ps = (void**)(pius->indic);
			pexist = (int*)ps[0];
			exist_old = *pexist;		//原值
			aptus->facio(pius);
			ps = (void**)(pius->indic);
			pexist = (int*)ps[0];
			if ( !(*pexist) ) 
				*pexist = exist_old;	//赋回原值，也就是说，卡片不在的, 不更新原来的值. 原假定不存在。
		}
		break;

	case Notitia::TIMER:	/* 定时信号 */
		WBUG("facio TIMER" );
		deliver(Notitia::DMD_SET_ALARM);
		if ( who_open_reader == VR_samin ) //如果unitoll未启动，这里定时盘点
		{	
			inventory();
			to_center_ventory(true);
		}

		break;

	default:
		return false;
	}
	return ret;
}

void ICPort::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	void *arr[3];

	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0x0;
	switch (aordo )
	{
	case Notitia::DMD_SET_ALARM:
		WBUG("deliver(sponte) DMD_SET_ALARM");
		tmp_pius.indic = &arr[0];
		arr[0] = this;
		arr[1] = &cent_qry_interval;
		arr[2] = 0;
		aptus->sponte(&tmp_pius);
		break;

	case Notitia::DMD_START_SESSION:
		WBUG("deliver(sponte) DMD_START_SESSION");
		tmp_pius.indic = 0;
		aptus->facio(&tmp_pius);
		break;

	default:
		WBUG("deliver Notitia::%d", aordo);
		break;
	}
}


bool ICPort::sponte( Amor::Pius *pius)
{		
	void **ps_ind;
	switch ( pius->ordo )
	{
	case Notitia::IC_DEV_INIT_BACK:
		WBUG("sponte IC_DEV_INIT_BACK");
		ps_ind = (void**)(pius->indic);
		if( (*((int*)ps_ind[0])) == 0 ) //有返回成功
		{
			dev_ok = true;
			SetEvent(pro_ev);
		}
		break;

	case Notitia::PRO_UNIPAC:    /* 有来自中心的报文 */
		WBUG("sponte PRO_UNIPAC");
		center_pac();
		break;

	case Notitia::START_SESSION:    /* 与中心有连接 */
		WBUG("sponte START_SESSION");
		if ( who_open_reader == VR_samin ) //如果unitoll未启动，这里连接时盘点
		{
			to_center_query();
		}
		break;

	case Notitia::END_SESSION:    /* 与中心有连接 */
		WBUG("sponte END_SESSION");
		deliver(Notitia::DMD_START_SESSION);

	default:
		return false;
	}
	return true;
}

ICPort::ICPort()
{
	wmod = WM_TOP;
	dev_ok = false;
	h_init_thread = INVALID_HANDLE_VALUE;

	hipa[0] = &hi_req;
	hipa[1] = &hi_reply;
	hipa[2] = 0;

	loc_pro_pac.ordo = Notitia::PRO_UNIPAC;
	loc_pro_pac.indic = 0;
	loc_pro_pac.subor = -1;
}

ICPort::~ICPort()
{
}

Amor* ICPort::clone()	//其实用不到
{
	ICPort *child = new ICPort();
	child->wmod =wmod;
	return (Amor*)child;
}


/* 这个以后看有什么参数可以搞 */
void ICPort::ignite(TiXmlElement *cfg)
{  
	const char *comm;
	int vmany = 0;
	will_reload_dll = false;

	comm = cfg->Attribute("reload_dll");
	if ( comm ) 
	{
		if ( comm[0] == 'y' ||  comm[0] == 'Y') 
		{
			will_reload_dll = true;
		}
	}

	comm = cfg->Attribute("reader_call");
	if ( comm ) 
	{
		me_who_str = comm;
		if ( strcasecmp(comm, "toll") == 0 ) 
		{
			/* 如果 从unitoll动态库调用来的，xml文件中就设置此值 */
			me_who = VR_toll;
			had_toll = true;
		}
		if ( strcasecmp(comm, "samin") == 0 ) 
		{
			/* 如果 从samin客户端启动来的，xml文件中就设置此值 */
			me_who = VR_samin;
			had_samin = true;
		}
	}

	init_timeout = 3000;
	cfg->QueryIntAttribute("init_timeout", &init_timeout);
	max_Qry_Card_num = 10;
	cfg->QueryIntAttribute("try_num_while_has_card", &max_Qry_Card_num);

	reader_ch = 0;
	comm = cfg->Attribute("reader_ch");
	if ( comm ) reader_ch = comm[0];

	comm = cfg->Attribute("mode");
	if ( comm ) 
	{
		if ( strcasecmp(comm, "sam") == 0 ) 
		{
			wmod=  WM_SAM;
			TEXTUS_STRCPY(wm_str,"SAM");
		}
		if ( strcasecmp(comm, "pro") == 0 ) 
		{
			wmod=  WM_PRO;
			TEXTUS_STRCPY(wm_str,"PRO");
		}
		if ( strcasecmp(comm, "both") == 0 ) 
		{
			wmod=  WM_BOTH;
			TEXTUS_STRCPY(wm_str,"BOTH");
		}
	} else {
		wmod=  WM_TOP;
		TEXTUS_STRCPY(wm_str,"TOP");
	}

	if ( !road_ele )
	{
		road_ele = cfg->FirstChildElement("Road");
	}

	comm=  cfg->Attribute("road_match");
	if ( !road_magic[0] && comm )
	{
		memset(road_magic, 0, sizeof(road_magic));
		if ( strlen(comm) > sizeof(road_magic) - 1)
		{
			memcpy(road_magic, comm, sizeof(road_magic) - 1);
		} else {
			memcpy(road_magic, comm, strlen(comm));
		}
	}

	return ;
}

void ICPort::SysInit()
{
	const char *s_pipe = 0;
	int i;
	char host_name[255];
	struct hostent *phe;
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR)
	{
		error_sys_pro("SysInit WSAStartup");
		goto END;
	}

//获取本地主机名称
	if (gethostname(host_name, sizeof(host_name)) == SOCKET_ERROR) {
		error_sys_pro("SysInit gethostname");
		goto PIPE_PRO;
	}


	phe = gethostbyname(host_name);
	if (phe == 0) {
		error_sys_pro("SysInit gethostbyname");
		goto PIPE_PRO;
	}

	//循环得出本地机器所有IP地址
	for ( i = 0; phe->h_addr_list[i] != 0; ++i) 
	{
		struct in_addr addr;
		memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
		strcpy(lane_ip, inet_ntoa(addr));
		break;
	}

PIPE_PRO:
	if ( me_who == VR_samin)
		s_pipe = samin_named_pipe_str;

	if ( me_who == VR_toll)
		s_pipe = toll_named_pipe_str;

	if (pro_ev == NULL)
	{
		pro_ev = CreateEvent( NULL, // default security attributes
			FALSE, //not Manual Reset (Auto Reset)
			FALSE, // 开始无信号 
			NULL);
		if (pro_ev == NULL) 
		{	
			error_sys_pro("SysInit CreateEvent");
			goto END;
		}
	}

	if (samin_no_read_ev == NULL)
	{
		samin_no_read_ev = CreateEvent( NULL, // default security attributes
			FALSE, //not Manual Reset (Auto Reset)
			FALSE, // 开始无信号 
			NULL);
		if (samin_no_read_ev == NULL) 
		{	
			error_sys_pro("SysInit CreateEvent");
			goto END;
		}
	}

	WBUG("创建命名管道(%s)并等待连接", s_pipe);  
	if ( h_srv_pipe !=INVALID_HANDLE_VALUE) goto Next_Step1;

	h_srv_pipe = CreateNamedPipe(s_pipe, PIPE_ACCESS_DUPLEX,   
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,   
            PIPE_UNLIMITED_INSTANCES, 0, 0, NMPWAIT_WAIT_FOREVER, 0);  

	if ( h_srv_pipe == INVALID_HANDLE_VALUE)
	{
		error_sys_pro("SysInit CreateNamedPipe");
		return; 
	}

	if (  (h_recv_pipe_thread=(HANDLE)_beginthread((my_thread_func)rcv_pipe_thrd, 1024000, this)) == INVALID_HANDLE_VALUE )
	{
			WLOG(ERR, "_beginthread error= %08x",  GetLastError());
			CloseHandle(h_srv_pipe);
			h_srv_pipe = INVALID_HANDLE_VALUE;
	}

Next_Step1:
	if ( (me_who == VR_toll && had_samin) || (me_who == VR_samin && had_toll))	/* 向对方通知自己的存在。这放在最后。 */
	{
		if ( get_cli_pipe() ) 
			notify_me_up();     
	}

	if ( (me_who == VR_samin ))	/* samin在这种情况下去连接读写器。toll自己另外操作。 */
	{
		deliver(Notitia::DMD_SET_ALARM); /* 设定时，sched来询问 */ 
		open_dev();
	}

END:
	return;
}

void ICPort::rcv_pipe()
{
	const int BUFFER_MAX_LEN = 512;  
	char szBuffer[BUFFER_MAX_LEN];  
	DWORD dwLen;  
	bool s_never = false;

LoopConnect:
	if (ConnectNamedPipe(h_srv_pipe, NULL) != NULL)//等待连接
	{  
		WBUG("%s 连接 h_srv_pipe 成功，开始接收数据", me_who_str );  
      
		//接收客户端发送的数据  
LoopRead:
		s_never = true; //假定对方不关闭，所以管道存在
		if ( !ReadFile(h_srv_pipe, szBuffer, BUFFER_MAX_LEN, &dwLen, NULL)) //读取管道中的内容（管道是一种特殊的文件）  
		{
			//要记一下错误日志
			error_sys_pro("rcv_pipe ReadFile");
			goto KillSrvPipe;
		}
		WBUG("%s接收到数据长度为%d字节: %s", me_who_str, dwLen, szBuffer);  

		switch ( szBuffer[0] ) 
		{
		case 't':	//toll启动，这是samin才收到的， 
			get_cli_pipe();
			break;

		case 's':	//samin启动，这是toll才收到。
			get_cli_pipe();
			break;

		case 'T':	//toll关闭，这是samin才收到的
			s_never = false; //管道不存在了
			close_cli_pipe();
			if ( who_open_reader == VR_toll && me_who == VR_samin ) 
			{
				//toll居然不关闭读卡器，这里置标志
				Sleep(1000);
				who_open_reader = VR_none;
				open_dev();
			}
			break;

		case 'S':	//samin关闭，这是toll才收到。
			s_never = false;//管道不存在了
			close_cli_pipe();
			//open_dev();
			break;

		case 'F':	//toll要连读卡器，这是samin收到的,samin应该关闭自己的读卡器连接，并通知toll
			close_dev(false);	//这里就不通知了，下面特别通知。
			notify_friend("f");	//告诉toll，应toll的要求，这里已经关闭。
			break;

		case 'f':	//toll收到的: samin已不再连读写器
			SetEvent(samin_no_read_ev);
			break;

		case 'd':	//已关闭读卡器连接，samin和toll都可收到。各自应该自己连接读卡器
			open_dev();
			break;

		case 'P':	//PSAM卡盘点请求，这是toll才收到的。
			if ( inventory() )
				notify_friend("p");	//告诉samin盘点完成。
			else 
				notify_friend("q");	//告诉samin盘点无法进行。
			break;

		case 'p':	//PSAM卡盘点完成，这是samin才收到的。
			to_center_ventory(true);	//向中心告
			break;

		case 'q':	//PSAM卡盘点完成，这是samin才收到的。
			to_center_ventory(false);	//向中心告
			break;

		default:
			break;
		}      
		if (s_never )  //管道存在，断续
			goto LoopRead;            
	}
//now, s_never = true, 
KillSrvPipe:
	DisconnectNamedPipe(h_srv_pipe);   
	goto LoopConnect;
}

void ICPort::close_cli_pipe()
{
	CloseHandle(h_cli_pipe);//关闭管道  
	h_cli_pipe = INVALID_HANDLE_VALUE;
}

bool ICPort::get_cli_pipe()
{
	const char *c_pipe;
	if ( h_cli_pipe != INVALID_HANDLE_VALUE)
	{
		//记日志
		WBUG("h_cli_pipe already ok");
		return true;
	}

	if ( me_who == VR_samin)
		c_pipe= toll_named_pipe_str;

	if ( me_who == VR_toll)
		c_pipe= samin_named_pipe_str;

	if (WaitNamedPipe(c_pipe, NMPWAIT_WAIT_FOREVER) == FALSE)  
	{  
		//记错误日志
		error_sys_pro("get_cli_pipe WaitNamedPipe");	
		return false;  
	}  

	h_cli_pipe = CreateFile(c_pipe, GENERIC_READ | GENERIC_WRITE, 0,  
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);  

	if ( h_cli_pipe == INVALID_HANDLE_VALUE)
	{
		//记错误日志
		error_sys_pro("get_cli_pipe write cli pipe");	
		return false;
	}
	WBUG("h_cli_pipe open ok");
	return true;
}

void ICPort::notify_friend(char *msg)
{
	DWORD dwLen = 0;  
	if ( h_cli_pipe == INVALID_HANDLE_VALUE)
	{
		WLOG(ALERT,"notify_friend %s: h_cli_pipe not ready", msg);
		return ;
	}

	if (!WriteFile(h_cli_pipe, msg, strlen(msg), &dwLen, NULL))
	{
		error_sys_pro("notify_friend write cli pipe");	
		close_cli_pipe();
		return;
	}
	WBUG("%s 数据写入完毕共 %d 字节 %s in notify_friend", me_who_str, dwLen, msg);  
}

void ICPort::notify_me_up()
{
	const int BUFFER_MAX_LEN = 32;  
	char szBuffer[BUFFER_MAX_LEN];  
	DWORD dwLen = 0;  
	if ( h_cli_pipe == INVALID_HANDLE_VALUE)
	{
		WLOG(ALERT,"notify_me_up: h_cli_pipe not ready");
		return ;
	}

	if ( me_who == VR_toll) //向samin通知
		szBuffer[0] = 't'; 

	if ( me_who == VR_samin) //向toll通知
		szBuffer[0] = 's'; 

	szBuffer[1] = '\0';
      
        //向服务端发送数据  
	if (!WriteFile(h_cli_pipe, szBuffer, 1, &dwLen, NULL))
	{
		error_sys_pro("notify_me_up write cli pipe");	
		close_cli_pipe();
		return;
	}
	WBUG("%s 数据写入完毕共 %d 字节 in notify_me_up", me_who_str, dwLen);  
}

void ICPort::notify_me_down()
{
	const int BUFFER_MAX_LEN = 32;  
	char szBuffer[BUFFER_MAX_LEN];  
	DWORD dwLen = 0;  

	if ( h_cli_pipe == INVALID_HANDLE_VALUE)
	{
		WLOG(ALERT,"notify_me_down: h_cli_pipe not ready");
		return ;
	}

	if ( me_who == VR_toll) //向samin通知
		szBuffer[0] = 'T'; 

	if ( me_who == VR_samin) //向toll通知
		szBuffer[0] = 'S'; 

	szBuffer[1] = '\0';
      
       //向服务端发送数据  
	if (!WriteFile(h_cli_pipe, szBuffer, 1, &dwLen, NULL))
	{
		error_sys_pro("notify_me_down write cli pipe");
		close_cli_pipe();
		return;
	}
	WBUG("%s 数据写入完毕共 %d 字节 in notify_me_down", me_who_str, dwLen);  
}

int ICPort::open_dev()
{
	int ret = 0;
	void *ps[4];
	Amor::Pius para;

	ret = -1;
	m_error_buf[0] = 0;
	if ( who_open_reader == VR_samin && me_who == VR_toll ) 
	{
		//toll要连读写器，而samin已经连上，则要求对方断开
		ResetEvent(samin_no_read_ev);
		notify_friend("F");
		if (WaitForSingleObject(samin_no_read_ev, justme->init_timeout) != WAIT_OBJECT_0 )	//默认3秒
		{
			error_sys_pro("open_dev for wait samin_no_read_ev");
			goto OEnd;
		}
	}

	if ( who_open_reader == VR_none  ) 
	{
		m_error_buf[0] = 0;
		ok_reader = '\0';
		ps[0] = &ret;
		ps[1] = &m_error_buf[0];
		ps[2] = (void*) 0;	//不指定参数
		ps[3] = (void*)&ok_reader;
		para.indic = ps;
		para.ordo = Notitia::IC_DEV_INIT; 

		ResetEvent(pro_ev);
		dev_ok = false;
		mary->facio(&para);
		if (WaitForSingleObject(pro_ev, justme->init_timeout) == WAIT_OBJECT_0 )	//默认3秒
		{
			who_open_reader = me_who;
		} else {
			error_sys_pro("open_dev for wait pro_ev");
		}
	}
	WBUG("who_open_reader %d", who_open_reader);

	if ( dev_ok )
		ret = 1;
	else 
		ret = 0;
OEnd:
	return ret;
}

int ICPort::close_dev(bool should_notify)
{
	const int BUFFER_MAX_LEN = 32;  
	char szBuffer[BUFFER_MAX_LEN];  
	DWORD dwLen = 0;  

	int ret = 0;
	void *ps[2];
	Amor::Pius para;
		
	ret = -1;
	m_error_buf[0] = 0;

	if ( who_open_reader != VR_none  ) 
	{
		ps[0] = &ret;
		ps[1] = &m_error_buf[0];
		para.indic = ps;
		para.ordo = Notitia::IC_DEV_QUIT;
		mary->facio(&para);
		if ( ret != 0 ) 
		{
			WLOG(ERR, "close_dev %s", m_error_buf);
		}

		who_open_reader = VR_none;
		szBuffer[0] = 'd'; szBuffer[1] = '\0';
		Sleep(100);
		if (should_notify)
		{
			if ( h_cli_pipe != INVALID_HANDLE_VALUE)
			{
		        if ( !WriteFile(h_cli_pipe, szBuffer, 1, &dwLen, NULL)) //通知，这里已经断开读写器
				{
					//记错误日志
					error_sys_pro("close_dev write cli pipe");
					close_cli_pipe();
				} 
			    WBUG("%s 数据d写入完毕共%d字节 in close_dev", me_who_str, dwLen);  
			} else {
				WLOG(ALERT, "close_dev: h_cli_pipe not ready");
			}
		}
	}
	return ret;
}

int ICPort::reload_dll()
{
	int ret = 0;
	void *ps[2];
	Amor::Pius para;
	
	ret = -1;
	m_error_buf[0] = 0;

	ps[0] = &ret;
	ps[1] = &m_error_buf[0];
	para.indic = ps;
	para.ordo = Notitia::URead_ReLoad_Dll;
	mary->facio(&para);
	if ( ret != 0 ) 
	{
		WLOG(ERR, "%s", m_error_buf);
	}
	return ret;
}

bool ICPort::inventory()
{
	//samory[0~3]
	int iSlot;
	int ret = 0,i;
	int sw;
	char answer[512], command[128];
	int len, qry_num;
	bool in_ret = true;

#if defined(_WIN32) && (_MSC_VER < 1400 )
	struct _timeb now;
#else
	struct timeb now;
#endif
	struct tm *tdatePtr;
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
	struct tm tdate;
#endif

#define SAM(C,Y,E) sw = 0; ret = CardCommandCharSlot(C, Y, 0, &sw, &iSlot); if ( ret !=0 || sw != 0x9000 ) { samory[iSlot].result = E; TEXTUS_SPRINTF(samory[iSlot].err, "%s return %s, sw=%04X", C, Y, sw); continue; }	
	MD5_CTX Md5Ctx;
	unsigned char md2[32], md3[32];

	qry_num = 0;
QryAgain:
	if (qry_num > max_Qry_Card_num ) 
		return false;
	if ( has_card ) 
	{
		WBUG("has card");
		qry_num++;
		Sleep(100);
		goto QryAgain;
	}

	isInventoring = true;
	for ( iSlot = 1; iSlot <= PSAM_SLOT_NUM;iSlot++ )
	{
		memset(&samory[iSlot], 0 ,sizeof(PsamInfo));
		if ( justme->will_reload_dll)
		{
			justme->close_dev(false);	//这是在toll模式下工作的，这里断开不要通知对方，因为自己要连
			if (justme->reload_dll() != 0 ) 
			{
				in_ret = false;
				goto IN_END;
			}

			if (justme->open_dev() == 0 ) 
			{
				in_ret = false;
				goto IN_END;
			}
		}

		memset(&samory[iSlot], 0, sizeof (PsamInfo));
		samory[iSlot].slot = 0;	//以此标志无PSAM卡。
#if defined(_WIN32) && (_MSC_VER < 1400 )
		_ftime(&now);
#else
		ftime(&now);
#endif
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
		tdatePtr = &tdate;
		localtime_s(tdatePtr, &now.time);
#else
		tdatePtr = localtime(&now.time);
#endif
		strftime(samory[iSlot].datetime, sizeof(samory[iSlot].datetime), "%y%m%d%H%M%S", tdatePtr);
		MD5Init (&Md5Ctx);
		if ( road_magic[0])
			MD5Update (&Md5Ctx, (unsigned char*)road_magic, strlen(road_magic));
		MD5Update (&Md5Ctx, (unsigned char*)&samory[iSlot].datetime[0], strlen(samory[iSlot].datetime));	
		MD5Update (&Md5Ctx, (unsigned char*)"213423_13axcc3q  3qqaab", 23);
		MD5Final (&Md5Ctx);

		len = 100;
		ret = SAM_reset(1, iSlot, &len, samory[iSlot].rst_info);
		WBUG("iSlot %d ret %d，%s", iSlot, ret, samory[iSlot].rst_info);
		if (ret !=0 ) 
		{
			//printf("get %s\n",GetOpInfo(ret));
			continue;
		}
		
		samory[iSlot].serial[0] = ' ';
		samory[iSlot].device_termid[0] = ' ';
		samory[iSlot].desc[0] = ' ';
		samory[iSlot].err[0] = ' ';
		samory[iSlot].slot = iSlot;		//复位成功，就算有卡了。
		samory[iSlot].result = 4;	//先假定未检测
		samory[iSlot].type = 0;		//未知卡
		GetReaderVersion(1, samory[iSlot].desc, sizeof(samory[iSlot].desc)-1, answer, sizeof(answer)-1); //这里answer没用
		SAM("00A40000023F00", answer,1)

		/* 取SAM卡的卡号*/
		SAM("00B095000A", &(samory[iSlot].serial[0]), 5)

		/* 取SAM卡的终端号*/
		SAM("00B0960006", samory[iSlot].device_termid, 5)

		if ( memcmp( samory[iSlot].serial, "4401", 4) == 0 ) //国标卡
		{
			samory[iSlot].type = 1;
			SAM("00A4000002DF01", answer, 1)
			TEXTUS_SPRINTF(command, "801A480110%s%s", psam_challenge, "B9E3B6ABB9E3B6AB");
			SAM(command, answer, 5)
			TEXTUS_SPRINTF(command, "80FA000008%s", psam_challenge);
			SAM(command, answer, 5)
			if ( memcmp(answer, psam_should_cipher_gb, 16) != 0 ) 
			{
				samory[iSlot].result = 3;
				TEXTUS_SPRINTF(samory[iSlot].err, "cipher is %s", answer);
			} else
				samory[iSlot].result = 0;	//至此， 检测OK.
		} else {	//地标卡
			samory[iSlot].type = 2;
			ret = CardCommandCharSlot("00A40000023F01", answer, 0, &sw,  &iSlot);
			SAM("00A40000023F01", answer, 1)
			TEXTUS_SPRINTF(command, "801A270108%s", psam_challenge);
			SAM(command, answer, 5)
			TEXTUS_SPRINTF(command, "80FA000008%s", psam_challenge);
			SAM(command, answer, 5)
			if ( memcmp(answer, psam_should_cipher_db44, 16) != 0 ) 
			{
				samory[iSlot].result = 3;
				TEXTUS_SPRINTF(samory[iSlot].err, "cipher is %s", answer);
			} else
				samory[iSlot].result = 0;	//至此， 检测OK.
		}
		hex2byte(md2, 10,  &samory[iSlot].serial[0]); 
		for ( i = 0 ; i < 10; i++)
			md3[i] = Md5Ctx.digest[i] ^ md2[i];
		byte2hex(md3, 10,  &samory[iSlot].serial[0]); 

		hex2byte(md2, 6,  &samory[iSlot].device_termid[0]);
		for ( i = 0 ; i < 6; i++) 
			md3[i] = Md5Ctx.digest[i] ^ md2[i];
		byte2hex(md3, 6,  &samory[iSlot].device_termid[0]); 
	}
IN_END:
	isInventoring = false;
	return in_ret;
}

/* 与中心通讯的域定义 */
#define Fun_Fld 2
#define IP_Fld 3
#define QryInterval_Fld 4
#define Challenge_Fld 5
#define DB44Cipher_Fld 6
#define GBCipher_Fld 7
#define Road_Fld 8
#define Station_Fld 9
#define Lane_Fld 10
#define PSamSlot_Fld 11
#define PSamSerial_Fld 12
#define PSamTermNo_Fld 13
#define PSamStat_Fld 14
#define PSamErrStr_Fld 15
#define InventoryTime_Fld 16
#define ReaderDesc_Fld 17
#define PSamType_Fld 18
#define PSamRstInf_Fld 19

void ICPort::center_pac()
{
	unsigned char *actp;
	unsigned long alen;
	char tmp[16];
	int iVal =0;

	actp=hi_reply.getfld(Fun_Fld, &alen);		//取得功能代码，来自中心，只有一个。 有Fun_Fld， QryInterval_Fld， Challenge_Fld，Challenge_Fld，DB44Cipher_Fld，GBCipher_Fld
	if ( !actp ) return ;
	if (*actp != 'P') return;
	actp=hi_reply.getfld(QryInterval_Fld, &alen);
	if ( alen < 6)
	{
		memcpy(tmp, actp, alen);
		tmp[alen] = 0;
		iVal = atoi(tmp);
	}

	actp=hi_reply.getfld(Challenge_Fld, &alen);
	if ( alen == 16 ) 
	{
		memcpy(psam_challenge, actp, 16);
		psam_challenge[16] = 0; 
	}
	actp=hi_reply.getfld(DB44Cipher_Fld, &alen);
	if ( alen == 16 ) 
	{
		memcpy(psam_should_cipher_db44, actp, 16);
		psam_should_cipher_db44[16] = 0; 
	}

	actp=hi_reply.getfld(GBCipher_Fld, &alen);
	if ( alen == 16 ) 
	{	
		memcpy(psam_should_cipher_gb, actp, 16);
		psam_should_cipher_gb[16] = 0; 
	}

	if ( iVal == 0 )	//查询间隔为0，立即盘点
	{
		if ( who_open_reader == VR_toll ) //对于toll打开的
			notify_friend("P");	//通知要盘点,完成后，samin收到，即向中心报告。
		else if ( who_open_reader == VR_samin )
		{	
			inventory();
			to_center_ventory(true);
		}
	}
}

void ICPort::to_center_query()	//向中心查询
{
	hi_req.reset();	//请求复位
	hi_req.input(Fun_Fld, 'Q');
	hi_req.input(IP_Fld, lane_ip, strlen(lane_ip));
	aptus->facio(&loc_pro_pac);     //向右发出
}

void ICPort::to_center_ventory(bool can)
{
	int i;
	char tmp[32];
	TiXmlElement *road_no = road_ele;

	if ( road_no && lane_road[0] != ' ')	//有路段定义，且确定是入口的
	{
		while ( road_no )
		{
			if ( strcmp(road_no->GetText(), lane_road) ==0 ) //有路段号匹配就中止
				break;
			road_no = road_no->NextSiblingElement("Road");
		}
		if ( !road_no )	//有路段号匹配这里不为0
			can = false;
	}

	if ( !can) 
	{
			hi_req.reset();	//请求复位
			hi_req.input(Fun_Fld, 'i');	//车道盘点无法进行
			hi_req.input(IP_Fld, lane_ip,strlen(lane_ip));
			hi_req.input(Road_Fld, lane_road, strlen(lane_road));
			hi_req.input(Station_Fld, lane_station, strlen(lane_station));
			hi_req.input(Lane_Fld, lane_no, strlen(lane_station));
			aptus->facio(&loc_pro_pac);     //向右发出

	} else {
		for ( i = 1; i <= PSAM_SLOT_NUM; i++)
		{
			if (samory[i].slot == 0  ) continue;
			hi_req.reset();	//请求复位
			hi_req.input(Fun_Fld, 'I');

			hi_req.input(IP_Fld, lane_ip,strlen(lane_ip));
			hi_req.input(Road_Fld, lane_road,strlen(lane_road));
			hi_req.input(Station_Fld, lane_station,strlen(lane_station));
			hi_req.input(Lane_Fld, lane_no,strlen(lane_no));
			//PSAM
			TEXTUS_SPRINTF(tmp, "%d", samory[i].slot);
			hi_req.input(PSamSlot_Fld, tmp,strlen(tmp));
			hi_req.input(PSamSerial_Fld, samory[i].serial,strlen(samory[i].serial));
			hi_req.input(PSamTermNo_Fld, samory[i].device_termid,strlen(samory[i].device_termid));
			TEXTUS_SPRINTF(tmp, "%d", samory[i].result);
			hi_req.input(PSamStat_Fld, tmp,strlen(tmp));
			hi_req.input(PSamErrStr_Fld, samory[i].err,strlen(samory[i].err));
			hi_req.input(InventoryTime_Fld, samory[i].datetime,strlen(samory[i].datetime));
			hi_req.input(ReaderDesc_Fld, samory[i].desc,strlen(samory[i].desc));
			TEXTUS_SPRINTF(tmp, "%d", samory[i].type);
			hi_req.input(PSamType_Fld, tmp,strlen(tmp));
			hi_req.input(PSamRstInf_Fld, samory[i].rst_info,strlen(samory[i].rst_info));
			aptus->facio(&loc_pro_pac);     //向右发出
		}
	}
}


BOOL APIENTRY DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
    // Perform actions based on the reason for calling.
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:
         // Initialize once for each new process.
         // Return FALSE to fail DLL load.
            break;


        case DLL_THREAD_ATTACH:
         // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
         // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
         // Perform any necessary cleanup.
			if ( me_who == VR_toll) //向samin通知
			{
				printf("TOLL DLL_PROCESS_DETACH\n");
				if (justme) 
				{
					//justme->close_dev();
					justme->notify_me_down();
				}
				had_toll = false;
			}

			if ( me_who == VR_samin) //向toll通知
			{
				printf("SAMIN DLL_PROCESS_DETACH\n");
				if (justme) 
				{
					//if ( who_open_reader == VR_samin ) //对于toll打开的，不管
					//	justme->close_dev();
					justme->notify_me_down();
				}
				had_samin = false;
			}
            break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

#define AMOR_CLS_TYPE ICPort
#include "hook.c"
