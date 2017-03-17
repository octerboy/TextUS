#include "Amor.h"
#include "Notitia.h"
#include "casecmp.h"
#include "textus_string.h"
#include "PacData.h"
#include "resource.h"
//#include "BTool.h"
#define SCM_MODULE_ID  "$Workfile: samtor.cpp $"
#define TEXTUS_MODTIME  "$Date: 16-12-09 16:02 $"
#define TEXTUS_BUILDNO  "$Revision: 7 $"
/* $NoKeywords: $ */
#include <stdio.h>
#include <conio.h>  
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <process.h>


static HWND h_tory_dlg = 0;

class Samtor: public Amor
{
public:
	Samtor();
	~Samtor();

	void ignite(TiXmlElement *i);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
	void deliver(Notitia::HERE_ORDO aordo);

	char m_error_buf[1024];

	void error_sys_pro(const char *h_msg);

	PacketObj *rcv_pac, *snd_pac; /* 向中心传递的 */
	Amor::Pius loc_pro_pac;
	HANDLE h_console_thread;
	void handle_pac();	//盘点报文
	void tor_demand();

#include "wlog.h"
};

static char psam_challeng[64];
static char db44_cipher[64];
static char gb_cipher[64];

static TiXmlElement *para_cfg=0;
#define US_SIZE 1000
static Samtor *samus[US_SIZE];
static int sam_cur_max=0;

static int tor_num = 0;


INT_PTR CALLBACK	InventProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	SetProc(HWND, UINT, WPARAM, LPARAM);


typedef void (__cdecl *my_thread_func)(void*);

static void  console_proc(Samtor  *arg)
{
	DialogBox(GetModuleHandle("samtor.dll"), MAKEINTRESOURCE(IDD_INVENTORY), GetForegroundWindow(), InventProc);
	arg->deliver(Notitia::CMD_MAIN_EXIT );
	_endthreadex(0);
}	

void Samtor::error_sys_pro(const char *h_msg) 
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


bool Samtor::facio( Amor::Pius *pius)
{
	bool ret;

	PacketObj **tmp;

	int i;

	ret = true;

	switch ( pius->ordo )
	{
	case Notitia::MAIN_PARA:
		WBUG("facio MAIN_PARA %p");
		if (h_console_thread != INVALID_HANDLE_VALUE )
				break;
		if ((h_console_thread = (HANDLE)_beginthread((my_thread_func)console_proc, 1024000, this)) == INVALID_HANDLE_VALUE )
		{
			WLOG(ERR, "_beginthread error= %08x",  GetLastError());
			CloseHandle(h_console_thread);
			h_console_thread = INVALID_HANDLE_VALUE;
			deliver(Notitia::CMD_MAIN_EXIT );
		}

		ret = false;
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		break;

	case Notitia::END_SESSION:
		WBUG("facio END_SESSION" );
		for ( i = 0; i <= sam_cur_max; i++)
		{
			if (samus[i] == this )
			{
				samus[i] = 0;
				if ( i == sam_cur_max ) 
				{
					sam_cur_max--;
					while ( sam_cur_max > 0 && samus[sam_cur_max] == 0 ) sam_cur_max--;
				}
				break;
			}
		}
		break;

	case Notitia::START_SESSION:
		WBUG("facio START_SESSION" );
		for ( i = 0; i < US_SIZE; i++)
		{
			if (samus[i] == 0 )
			{
				samus[i] = this;
				if ( i > sam_cur_max ) sam_cur_max = i;
				break;
			}
		}
		break;

	case Notitia::SET_UNIPAC:
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) rcv_pac = *tmp; 
			else {
				WBUG("facio SET_UNIPAC rcv_pac null");
			}
			tmp++;
			if ( *tmp) snd_pac = *tmp;
			else {
				WBUG("facio SET_UNIPAC snd_pac null");
			}

			WBUG("facio SET_UNIPAC rcv(%p) snd(%p)", rcv_pac, snd_pac);
		} else {
			WBUG("facio SET_TBUF null");
		}
		break;

	case Notitia::PRO_UNIPAC:    /* 有来自控制台的请求 */
		WBUG("facio PRO_UNIPAC");
		handle_pac();
		break;

	default:
		return false;
	}
	return ret;
}

void Samtor::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;

	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0x0;
	switch (aordo )
	{
	case Notitia::DMD_START_SESSION:
		WBUG("deliver(sponte) DMD_START_SESSION");
		tmp_pius.indic = 0;
		aptus->facio(&tmp_pius);
		break;

	case Notitia::CMD_MAIN_EXIT:
		WBUG("deliver(sponte) CMD_MAIN_EXIT");
		tmp_pius.indic = 0;
		aptus->sponte(&tmp_pius);
		break;

	default:
		WBUG("deliver Notitia::%d", aordo);
		break;
	}
}

bool Samtor::sponte( Amor::Pius *pius)
{		
	switch ( pius->ordo )
	{

	case Notitia::PRO_UNIPAC:    /*  */
		WBUG("sponte PRO_UNIPAC");
		break;

	case Notitia::START_SESSION:    /* 与中心有连接 */
		WBUG("sponte START_SESSION");
		break;

	case Notitia::END_SESSION:    /* 与中心有连接 */
		WBUG("sponte END_SESSION");


	default:
		return false;
	}
	return true;
}

Samtor::Samtor()
{
	h_console_thread = INVALID_HANDLE_VALUE;

	rcv_pac = 0;
	snd_pac = 0;

	loc_pro_pac.ordo = Notitia::PRO_UNIPAC;
	loc_pro_pac.indic = 0;
	loc_pro_pac.subor = -1;
}

Samtor::~Samtor()
{
}

Amor* Samtor::clone()
{
	Samtor *child = new Samtor();
	return (Amor*)child;
}


void Samtor::ignite(TiXmlElement *cfg)
{  
	const char *comm;
	para_cfg = cfg;

	memset(psam_challeng, 0, sizeof(psam_challeng));
	memset(db44_cipher, 0, sizeof(db44_cipher));
	memset(gb_cipher, 0, sizeof(gb_cipher));
	comm = cfg->Attribute("psam_challeng");
	if ( comm ) 
	{
		TEXTUS_STRCPY(psam_challeng, comm);
	}

	comm = cfg->Attribute("db44_cipher");
	if ( comm ) 
	{
		TEXTUS_STRCPY(db44_cipher, comm);
	}

	comm = cfg->Attribute("gb_cipher");
	if ( comm ) 
	{
		TEXTUS_STRCPY(gb_cipher, comm);
	}
	memset(samus, 0, sizeof(Samtor*)*US_SIZE);

	return ;
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
#define PSamDesc_Fld 17
#define PSamType_Fld 18
#define PSamRstInf_Fld 19

void Samtor::handle_pac()
{
	unsigned char *actp;
	unsigned long alen;
	const int msg_size = 128;
	char msg[msg_size];
	char fun;
	LVITEM lv;
	HWND he= NULL;

	actp=rcv_pac->getfld(Fun_Fld, &alen);		//取得功能代码，来自中心，只有一个。 有Fun_Fld， QryInterval_Fld， Challenge_Fld，Challenge_Fld，DB44Cipher_Fld，GBCipher_Fld
	if ( !actp ) return ;
	fun = *actp;
	he = GetDlgItem(h_tory_dlg, IDC_SAM_INFO);
	switch ( fun ) 
	{
	case 'I':
	case 'i':
		tor_num++;
		TEXTUS_SPRINTF(msg, "%d", tor_num);

		lv.mask = LVIF_TEXT;
		lv.iItem = tor_num-1;
		lv.pszText = msg;
		lv.iSubItem = 0;
		ListView_InsertItem(he, &lv);

#define DISP_A_PSAM(FLD, ITEM) \
		actp=rcv_pac->getfld(FLD, &alen);	\
		if ( actp && alen < msg_size)		\
		{							\
			memcpy(msg, actp, alen);\
			msg[alen] = 0;			\
			lv.mask = LVIF_TEXT;	\
			lv.iItem = tor_num-1;			\
			lv.pszText = msg;		\
			lv.iSubItem = ITEM;		\
			ListView_SetItem(he, &lv); \
		}

		DISP_A_PSAM(IP_Fld, 4)
		if (fun == 'i' )
		{
			lv.mask = LVIF_TEXT;
			lv.iItem = tor_num-1;
			TEXTUS_STRCPY(msg, "无法盘点");
			lv.pszText = msg;
			lv.iSubItem = 8;
			ListView_SetItem(he, &lv);
			break;
		}
		DISP_A_PSAM(PSamSlot_Fld, 5)
		DISP_A_PSAM(PSamSerial_Fld, 2)
		DISP_A_PSAM(PSamTermNo_Fld, 1)

		DISP_A_PSAM(PSamDesc_Fld, 3)
		DISP_A_PSAM(PSamErrStr_Fld, 8)

		DISP_A_PSAM(InventoryTime_Fld, 6)
		DISP_A_PSAM(PSamRstInf_Fld, 7)
		actp=rcv_pac->getfld(PSamStat_Fld, &alen);

		//if ( actp && *actp !='0' ) 
		//{
		//	ListView_SetCheckState(he, tor_num-1, true);
		//} else {
		//	ListView_SetCheckState(he, tor_num-1, false);
		//}
		break;

	case 'Q':
		tor_demand();
		break;

	default:
		break;
	}
}

void Samtor::tor_demand()
{
	snd_pac->reset();
	snd_pac->input(Fun_Fld, 'P');
	snd_pac->input(QryInterval_Fld, '0');
	snd_pac->input(Challenge_Fld, psam_challeng, strlen(psam_challeng));
	snd_pac->input(DB44Cipher_Fld, db44_cipher, strlen(db44_cipher));
	snd_pac->input(GBCipher_Fld, gb_cipher, strlen(gb_cipher));
	aptus->sponte(&loc_pro_pac);
}

INT_PTR CALLBACK InventProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND he= NULL;
	UNREFERENCED_PARAMETER(lParam);
	h_tory_dlg = hDlg;
	LVCOLUMN list;
	int sret = 0, i;

	switch (message)
	{
	case WM_INITDIALOG:
		he = GetDlgItem(hDlg, IDC_SAM_INFO);
		sret = SendMessage (he, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP|LVS_EX_GRIDLINES/*|LVS_EX_CHECKBOXES*/);

		list.fmt = LVCFMT_CENTER;
		list.iSubItem = 8;
		list.cx = 120;
		list.pszText = "错误描述";
		list.mask= LVCF_TEXT|LVCF_WIDTH|LVCF_FMT|LVCF_SUBITEM;
		sret = SendMessage (he, LVM_INSERTCOLUMN, 0, (LPARAM)&list);

		list.fmt = LVCFMT_CENTER;
		list.iSubItem = 7;
		list.cx = 160;
		list.pszText = "复位信息";
		list.mask= LVCF_TEXT|LVCF_WIDTH|LVCF_FMT|LVCF_SUBITEM;
		sret = SendMessage (he, LVM_INSERTCOLUMN, 0, (LPARAM)&list);

		list.fmt = LVCFMT_CENTER;
		list.iSubItem = 6;
		list.cx = 200;
		list.pszText = "盘点时间";
		list.mask= LVCF_TEXT|LVCF_WIDTH|LVCF_FMT|LVCF_SUBITEM;
		sret = SendMessage (he, LVM_INSERTCOLUMN, 0, (LPARAM)&list);

		list.fmt = LVCFMT_CENTER;
		list.iSubItem = 5;
		list.cx = 50;
		list.pszText = "卡槽";
		list.mask= LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
		sret = SendMessage (he, LVM_INSERTCOLUMN, 0, (LPARAM)&list);

		list.iSubItem = 4;
		list.cx = 160;
		list.pszText = "车道网址";
		list.cchTextMax = 50;
		sret=SendMessage (he, LVM_INSERTCOLUMN, 0, (LPARAM)&list);

		list.fmt = LVCFMT_CENTER;
		list.iSubItem = 3;
		list.cx = 100;
		list.pszText = "状态";
		list.mask= LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
		sret = SendMessage (he, LVM_INSERTCOLUMN, 0, (LPARAM)&list);

		list.fmt = LVCFMT_CENTER;
		list.iSubItem = 2;
		list.cx = 200;
		list.pszText = "表面号";
		list.mask= LVCF_TEXT|LVCF_WIDTH|LVCF_FMT|LVCF_SUBITEM;
		sret = SendMessage (he, LVM_INSERTCOLUMN, 0, (LPARAM)&list);

		list.fmt = LVCFMT_LEFT;
		list.iSubItem = 1;
		list.cx = 130;
		list.pszText = "终端号";
		list.mask= LVCF_TEXT|LVCF_WIDTH|LVCF_FMT|LVCF_SUBITEM;
		sret = SendMessage (he, LVM_INSERTCOLUMN, 0, (LPARAM)&list);

		list.fmt = LVCFMT_LEFT;
		list.iSubItem = 0;
		list.cx = 50;
		list.pszText = "序号";
		list.mask= LVCF_TEXT|LVCF_WIDTH|LVCF_FMT|LVCF_SUBITEM;
		list.cchTextMax = 50;
		sret=SendMessage (he, LVM_INSERTCOLUMN, 0, (LPARAM)&list);

		return (INT_PTR)TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:	//盘点
			for ( i = 0; i <=sam_cur_max; i++)
			{
				if (samus[i] != 0 )
				{
					samus[i]->tor_demand();	//向所有客户端发出盘点要求
				}
			}
			return (INT_PTR)TRUE;
			break;

		case ID_CLEAR:	//清屏
			he = GetDlgItem(hDlg, IDC_SAM_INFO);
			tor_num = 0 ;
			ListView_DeleteAllItems(he);
			break;
		case IDC_SET_CIPHER:
			DialogBox(GetModuleHandle("samtor.dll"), MAKEINTRESOURCE(IDC_SET_CIPHER), GetForegroundWindow(), SetProc);
			return (INT_PTR)TRUE;
			break;

		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
			break;

		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK SetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND he= NULL;
	UNREFERENCED_PARAMETER(lParam);
	bool sret = false;

	switch (message)
	{
	case WM_INITDIALOG:
		he = GetDlgItem(hDlg, IDC_RANDOM);
		SendMessage (he, WM_SETTEXT, 0, (LPARAM)psam_challeng);
		he = GetDlgItem(hDlg, IDC_DB44CIPHER);
		SendMessage (he, WM_SETTEXT, 0, (LPARAM)db44_cipher);
		he = GetDlgItem(hDlg, IDC_GBCIPHER);
		SendMessage (he, WM_SETTEXT, 0, (LPARAM)gb_cipher);

		return (INT_PTR)TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			he = GetDlgItem(hDlg, IDC_RANDOM);
			SendMessage (he, WM_GETTEXT, sizeof(psam_challeng), (LPARAM)psam_challeng);
			he = GetDlgItem(hDlg, IDC_DB44CIPHER);
			SendMessage (he, WM_GETTEXT, sizeof(db44_cipher), (LPARAM)db44_cipher);
			he = GetDlgItem(hDlg, IDC_GBCIPHER);
			SendMessage (he, WM_GETTEXT, sizeof(gb_cipher), (LPARAM)gb_cipher);

			para_cfg->SetAttribute("psam_challeng", psam_challeng);			
			para_cfg->SetAttribute("db44_cipher", db44_cipher);			
			para_cfg->SetAttribute("gb_cipher", gb_cipher);			
			sret = para_cfg->GetDocument()->SaveFile();
			if (sret )
				MessageBox(NULL, "参数设置成功!", TEXT("PSAM"), MB_OK);
			else
				MessageBox(NULL, "参数设置失败!", TEXT("Inventory"), MB_OK);
			return (INT_PTR)TRUE;
			break;

		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
			break;

		}
		break;
	}
	return (INT_PTR)FALSE;
}

#define AMOR_CLS_TYPE Samtor
#include "hook.c"
