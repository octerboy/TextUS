// ToR531.cpp : ���� DLL �ĳ�ʼ�����̡�
//
#include "Amor.h"
#include "Notitia.h"
#include "casecmp.h"
#include "textus_string.h"
#include <stdio.h>

#include "R531DeviceFunc.h"
#include "publicFunc.h"

#define SCM_MODULE_ID  "$Workfile: ToR531.cpp $"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */


#define MINLINE inline
#define ObtainHex(s,X)   ( (s) > 9 ? (s)-10+X :(s)+'0')
#define Obtainx(s)   ObtainHex(s,'a')
#define ObtainX(s)   ObtainHex(s,'A')
#define Obtainc(s)   (s >= 'A' && s <='F' ? s-'A'+10 :(s >= 'a' && s <='f' ? s-'a'+10 : s-'0' ) )

static char* byte2hex(const unsigned char *byte, size_t blen, char *hex) 
{
	size_t i;
	for ( i = 0 ; i < blen ; i++ )
	{
		hex[2*i] =  ObtainX((byte[i] & 0xF0 ) >> 4 );
		hex[2*i+1] = ObtainX(byte[i] & 0x0F );
	}
//	hex[2*i] = '\0';
	return hex;
}

static unsigned char* hex2byte(unsigned char *byte, size_t blen, const char *hex)
{
	size_t i;
	const char *p ;	

	p = hex; i = 0;

	while ( i < blen )
	{
		byte[i] =  (0x0F & Obtainc( hex[2*i] ) ) << 4;
		byte[i] |=  Obtainc( hex[2*i+1] ) & 0x0f ;
		i++;
		p +=2;
	}
	return byte;
}



class ToR531: public Amor
{
public:
	bool dev_ok;	//true:�豸�Ѿ�OK. 
	bool card_ok;	//true:�û���Ƭ�Ѿ�OK, һ������ͨѶ����, ������ֵ��Ϊfalse
	bool sam_ok;	//true:SAM��Ƭ�Ѿ�OK, һ������ͨѶ����, ������ֵ��Ϊfalse

	int dev_init(void);
	int dev_close(void);
	int Pro_Open(char uid[17]);
	int Pro_Close();
	int Sam_Reset(char atr[64]);
	bool Pro_Present();
	int CardCommand (const char* command, char* response, int which, int *sw);
	char *m_error_buf;

	ToR531();
	~ToR531();

	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	HANDLE op_mutex;		//����������, ��ֹ̽����������

	struct G_CFG {
		const char *para;
		// PSAM��ȱʡʹ�ã�SIM1 ģ��
		unsigned char sam_face;
		inline G_CFG() {
			para= (const char*)0;
		};	
		inline ~G_CFG() {
		};
	};
	struct G_CFG *gCFG;     /* ȫ�ֹ������ */
	bool has_config;

	// ���庯���ڲ����ȫ�ֱ���
	// �û���ʹ�÷ǽ�ģ��
	unsigned char pro_face;

	int hDev;
	char comm_name[128];
	int card_type; //8: PRO��

#include "wlog.h"
};

// ��ʼ���������еĸ��ֱ���(��֧��PSAM��ʹ�õ�ģ��)
void ToR531::ignite(TiXmlElement *prop)
{
	int i_sam_face;
	pro_face = 0xFF;
		
	//MessageBox(NULL, "r531 ignite", TEXT("Unigo "), MB_OK);
	if (!prop) return;		//��һ�е��Ƕ�Ҫ�ġ�
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		gCFG->para = 0;
		has_config = true;
	}
	gCFG->sam_face = 1;

	prop->QueryIntAttribute("SIM", &i_sam_face);
	gCFG->sam_face = 0xFF & i_sam_face;
	gCFG->para = prop->Attribute("dev_para");
}

// ��ʼ����д��
int ToR531::dev_init(void)
{
	char szMsg[500+1];
	int  nRet;
	int  nLen;
	int  nStatus;

	// �豸������ʼ
	nRet = R531DeviceFind();
	if( nRet == 0)
	{
		memcpy(m_error_buf,"ϵͳ�޶������豸",17);
		return R531NODEV;
	}else if( nRet < 0)
	{
		R531GetErrMsg(nRet,szMsg);
		nLen = (int)strlen(szMsg);
		memcpy(m_error_buf,szMsg,nLen);
		memcpy(m_error_buf+nLen,"\x00",1);
		return nRet;
	}

	// �������ӵ�һ��������
	memcpy(comm_name,"USB1",5);
	nRet = R531ConnDev(comm_name);
	if( nRet < NO_ERROR)
	{
		R531GetErrMsg(nRet,szMsg);
		nLen = (int)strlen(szMsg);
		memcpy(m_error_buf,szMsg,nLen);
		memcpy(m_error_buf+nLen,"\x00",1);
		return nRet;
	}

	// ������������Ӿ��
	hDev = nRet;

	if ( gCFG->para )
	{
		memcpy(szMsg,"00000000000000000000",21);
		memcpy(szMsg, gCFG->para, strlen(gCFG->para));
	} else
		memcpy(szMsg,"13131313010000000000",21);
	nLen = 20;

	// ���ö���������
	nRet = R531DevModPara(hDev,nLen,szMsg);
	if( nRet != NO_ERROR)
	{
		R531GetErrMsg(nRet,szMsg);
		nLen = (int)strlen(szMsg);
		memcpy(m_error_buf,szMsg,nLen);
		memcpy(m_error_buf+nLen,"\x00",1);
		return nRet;
	}

	// ����ʱ���÷ǽӹ���ģʽ 	
	unsigned char nMode;
	unsigned char nSpeed;


	nMode  = 0x0A;
	nSpeed = 0x00; 

	// ���÷ǽӹ���ģʽ
	nRet = R531DevSetRfMode(hDev,nMode, nSpeed,&nStatus);
	if( nRet != R531OK)
	{
		R531GetErrMsg(nRet,szMsg);
		nLen = (int)strlen(szMsg);

		memcpy(m_error_buf,szMsg,nLen);
		memcpy(m_error_buf+nLen,"\x00",1);
		return nRet;
	}
	return 0;
}

// �رն�д��
int ToR531::dev_close(void)
{
	return R531CloseDev(hDev);
}

// ̽�� -- ��֧�� δ����14443-4���Ѱ��(�ǽ�) 
bool ToR531::Pro_Present(void)
{
	char szMsg[500+1];
	int  nRet;
	int  nLen;
	int  nStatus, slotStatus;


	if( hDev == -1)
	{
		memcpy(m_error_buf,"������δ����",13);
		return false;
	}

	if ( (nRet = R531DevCheckUserCardSlot(hDev, &slotStatus, &nStatus)) != R531OK )
	{
		goto ERR_PRO;
	}

	if (slotStatus == 1 || slotStatus == 2 ) 
	{
		pro_face = 0xFF;
		return true;
	} 

	if ( card_ok )
	{
		nRet = R531TypeFindCard2(hDev,&nStatus);
		if( nRet != NO_ERROR)
			goto ERR_PRO;
		if ( nStatus == 0 )
		{
			return true;
		} else {
			card_ok = false;
			return false;
		}
	} 

	// ������Ƶ�����غ��� --R531DevResetHW
	nRet = R531DevResetHW(hDev,0x01);
	if( nRet != NO_ERROR)
		goto ERR_PRO;

	nRet = R531TypeFindCard(hDev,&nStatus);
	if( nRet != NO_ERROR)
		goto ERR_PRO;

	return true;

ERR_PRO:
	R531GetErrMsg(nRet,szMsg);
	nLen = (int)strlen(szMsg);
	memcpy(m_error_buf,szMsg,nLen);
	memcpy(m_error_buf+nLen,"\x00",1);
	card_ok = false;
	return false;	
}

// �򿪿�Ƭ
int ToR531::Pro_Open(char uid[18])
{
	int ret;
	int npCardType =0 ;
	int nStatus;
	int nLen;
	unsigned char cv = 1 ;
	int nATSLen = 0 ;
	unsigned char m_cid = 0; 

	char szAtqa[21];

	unsigned char nMode;
	unsigned char mode;
	unsigned char nSpeed;
	char          szMsg[500+1];
	char          szUID[21];
	char          szHexUID[21];

	int slotStatus;
	char szATR[81];
	int nAtrLen;
	char *fun_str;

	// ���������Ƿ�����
	if( hDev == -1)
	{
		memcpy(m_error_buf,"������δ����",13);
		return R531UNCONN;
	}
	slotStatus = 0;
	if ( (ret = R531DevCheckUserCardSlot(hDev, &slotStatus, &nStatus)) != R531OK )
	{
		fun_str = "R531DevCheckUserCardSlot";
		goto ERR_PRO;
	}

	if (slotStatus == 1 || slotStatus == 2 ) 
	{
		pro_face = 0;
	} else {
		pro_face = 0xFF;
	}

	if ( pro_face == 0 )	//�Ӵ�������û���
	{
		ret = R531CpuSetSlot(hDev, 0);
		fun_str = "R531CpuSetSlot";
		if( ret != R531OK)
			goto ERR_PRO;
	
		nAtrLen = 60;
		ret = R531CpuReset(hDev,&nAtrLen,szATR);
		fun_str = "R531CpuReset";
		if( ret != R531OK)
			goto ERR_PRO;

		memcpy(uid, &szATR[nAtrLen-16], 16);
		uid[16] = 0;
		if ( nAtrLen > 8)
			nAtrLen = 8;
		if ( ret != NO_ERROR )	
			goto ERR_PRO;
		else
			return 0;
	}

	nMode  = 0x01;				// �ȹغ�
	nSpeed = 0x32;				// ʱ����

	// 1.��Ƶ������
	ret = R531DevRField(hDev,nMode,nSpeed,&nStatus);
	if( ret != R531OK)
	{
		fun_str = "R531CpuReset";
		goto ERR_PRO;
	}

	// 2.Ѱ��  �� 0x26��������Ƶ���ڴ���IDLE״̬�Ŀ� ;  0x52��������Ƶ���ڴ���IDLE��HALT״̬�����п�
	//if( mode == 0)
	//{		
	mode = 0x26;	
	//}else 
	//{
	//	mode = 0x52;
	//}

	memset(szAtqa,0x00,sizeof(szAtqa));

	ret = R531TypeARequest(hDev,mode,szAtqa,&nStatus);
	if( ret != R531OK)
	{
		fun_str = "R531TypeARequest";
		goto ERR_PRO;
	}

	// ���ֿ�����,2 - M1S70��4 - M1S50��8 - MPRO 
	npCardType = szAtqa[1] - '0';

	memset(szUID,0x00,sizeof(szUID));
	memset(szHexUID,0x00,sizeof(szHexUID));

	//3.����ͻ,����UID
	ret = R531TypeAAntiCollision(hDev,1,szUID,&nStatus);
	if( ret != R531OK)
	{
		fun_str = "R531TypeAAntiCollision";
		goto ERR_PRO;
	}

	memcpy(uid, szUID, 8);
	uid[8] = 0;
	// ��szUIDת����BCD��,
	/*
	ftAtoh(szUID,szHexUID,4);
	memcpy(uid,szHexUID,4);
	memcpy(uid+4,"\x00",1);
	*/

	// 4.ѡ��Ƭ
	char szSAK[8];
	char szHexSAK[8];
	memset(szSAK,0x00,sizeof(szSAK));
	memset(szHexSAK,0x00,sizeof(szHexSAK));

	ret = R531TypeASelect(hDev,1,szUID,szSAK,&nStatus);
	if( ret != R531OK)
	{
		fun_str = "R531TypeASelect";
		goto ERR_PRO;
	}


	// �����ص�SAKֵת����BCD��
	ftAtoh(szSAK,szHexSAK,4);

	// ���SAK�Ƿ�Ϊ��0x20 
	if( ((*szHexSAK & 0x20) == 0x20) && ((*szHexSAK & 0x04) == 0x00))		//֧�� -4
	{

		// 5.ѡ��Ӧ��
		char szATS[81];
		memset(szATS,0x00,sizeof(szATS));

		ret = R531TypeARats(hDev,0,szATS,&nStatus);
		if( ret != R531OK)
		{
			fun_str = "R531TypeARats";
			goto ERR_PRO;
		}

	} else if( (*szHexSAK & 0x24) == 0x0)	//��֧�� -4	
	{							
		return -1;
	}

	card_type = npCardType;

	if ( *szHexSAK == 0x28 || *szHexSAK == 0x38 )
		card_type = 0x08;

	return 0;

ERR_PRO:
	R531GetErrMsg(ret,szMsg);
	nLen = (int)strlen(szMsg);
	memcpy(m_error_buf,szMsg,nLen);
	memcpy(m_error_buf+nLen,"\x00",1);
	WLOG(ERR, "%s ret=%d(%s)", fun_str, ret, m_error_buf);
	return ret;
}

// �رտ�Ƭ
int ToR531::Pro_Close(void)
{
	char          szMsg[500+1];
	int nRet;
	int nLen;
	int nStatus;

	unsigned char nMode;
	unsigned char nSpeed;

	// �ж�
	R531TypeAHalt(hDev,&nStatus);

	nMode  = 0x00;
	nSpeed = 0x00;


	// �ر���Ƶ��
	nRet = R531DevRField(hDev,nMode,nSpeed,&nStatus);
	if( nRet != R531OK)
	{
		R531GetErrMsg(nRet,szMsg);
		nLen = (int)strlen(szMsg);
		memcpy(m_error_buf,szMsg,nLen);
		memcpy(m_error_buf+nLen,"\x00",1);
		return nRet;
	}
	return 0;
}


// SAM��λ
int ToR531::Sam_Reset(char atr[64])
{
	char szMsg[500+1];
	char szATR[81];
	int nSlot;
	int nRet;
	int nLen;
	int nAtrLen;

	memset(atr,0x00,sizeof(atr));

	// ����ѡ��SAM���۱��(1~3) ,ȱʡֵΪ��1 -- SIM1����
	nSlot = gCFG->sam_face;

	// 1.ѡ�񿨲�
	nRet = R531CpuSetSlot(hDev,nSlot);
	if( nRet != NO_ERROR)
	{
		R531GetErrMsg(nRet,szMsg);
		nLen = (int)strlen(szMsg);
		memcpy(m_error_buf,szMsg,nLen);
		memcpy(m_error_buf+nLen,"\x00",1);
		WLOG(ERR, "R531CpuSetSlot ret=%d(%s)", nRet, m_error_buf);
		return nRet;
	}

	// �ϵ縴λ
	nRet = R531CpuReset(hDev,&nAtrLen,szATR);
	if( nRet != NO_ERROR)
	{
		R531GetErrMsg(nRet,szMsg);
		nLen = (int)strlen(szMsg);
		memcpy(m_error_buf,szMsg,nLen);
		memcpy(m_error_buf+nLen,"\x00",1);	
		WLOG(ERR, "R531CpuReset ret=%d(%s)", nRet, m_error_buf);
		return nRet;
	}

	// ATR���浽�������
	memcpy(atr,szATR,nAtrLen);
	memcpy(atr+nAtrLen,"\x00",1);
	return 0;
}


// ��Ƭ������SAM��ָ���뷵��
//commandΪAPDUָ�16�����ַ�����null�ַ���β				--- ��ΪASCII�룬�� 0x00 ����
//response Ϊ��Ӧ��������SW��16�����ַ�����null�ַ���β		--- ��ΪASCII�룬�� 0x00 ����
//which�����룬0��SAM��1��ʾ�û���
//sw�������SW����*sw=0x9000�� 
int ToR531::CardCommand(const char *command, char *response, int which, int *sw)
{
	char szMsg[500+1];
	char szSend[1024];
	char szRecv[1024];
	char szSW[4+1];
	char szHSW[2+1];

	int  nLen;
	int  nRet;
	int  nSW;
	int  nStatus;

	memset(szSend,0x00,sizeof(szSend));
	memcpy(response,"\x00",1);

	// ��鴫�����
	if( !(which == 0 || which == 1 ))
	{
		memcpy(m_error_buf,"��������",9);
		return R531PARAMERR;
	}

	// ���������Ƿ�������
	// ���������Ƿ�����
	if( hDev == -1)
	{
		memcpy(m_error_buf,"������δ����",13);
		return R531UNCONN;
	}

	nLen =(int)strlen(command);

	memset(szSend,0x00,sizeof(szSend));
	memcpy(szSend,command,nLen);

	// ���� which ����������غ���ִ��APDUָ��
	if( which == 1 && pro_face == 0xFF )
	{	// �û���APDUִ��
		memset(szRecv, 0x00, sizeof(szRecv));
		nRet = R531TypeAPDU(hDev,szSend,szRecv, &nStatus, card_type);
		if( nRet != NO_ERROR)
		{
			R531GetErrMsg(nRet,szMsg);
			nLen = (int)strlen(szMsg);
			memcpy(m_error_buf,szMsg,nLen);
			memcpy(m_error_buf+nLen,"\x00",1);
			WLOG(ERR, "R531TypeAPDU(%s), ret=%d(%s)", command, nRet, m_error_buf);
			return nRet;
		}
	} else 
	{	// PSAM����Ӵ������û��� APDUִ��
		if ( which == 1 ) 
		{
			nRet = R531CpuSetSlot(hDev, 0);
		} else {
			nRet = R531CpuSetSlot(hDev, gCFG->sam_face);
		}
		nRet = R531CpuAPDU(hDev,szSend,szRecv,&nStatus);
		if( nRet != NO_ERROR)
		{
			R531GetErrMsg(nRet,szMsg);
			nLen = (int)strlen(szMsg);
			memcpy(m_error_buf,szMsg,nLen);
			memcpy(m_error_buf+nLen,"\x00",1);
			WLOG(ERR, "R531CpuAPDU(%s), ret=%d(%s)", command, nRet, m_error_buf);
			return nRet;
		}
	}

	// ����Ӧ����
	nLen = (int)strlen(szRecv);

	if( nLen < 4)
	{
		memcpy(m_error_buf,"Ӧ�����ݴ���",13);
		WLOG(ERR, "R531APDU(%s), recvlen=%d(%s)", command, nLen, m_error_buf);
		return R531RESPDATALENERR;
	}

	// ��SW��Ӧ������
	nLen = nLen -4;

	// ����Ӧ�����ݵ�SW
	memset(szSW,0x00,sizeof(szSW));
	memcpy(szSW,szRecv+nLen,4);


	// ��SWת����BCD��
	ftAtoh(szSW,szHSW,2);

	// ��BCD���SWֵת����int ֵ���浽�����������
	nSW = (unsigned char)szHSW[0] *256 + (unsigned char )szHSW[1];
	*sw = nSW;

	// ��Ӧ����������SW�����ݱ��浽�����������
	if( nLen > 0)
	{
		memcpy(response,szRecv,nLen);
		memcpy(response+nLen,"\x00",1);
	}

	return 0;
}

bool ToR531::facio( Amor::Pius *pius)
{
	void **ps;
	int *ret;
	char *comm, *reply, *uid, *atr;
	int *psw;
	int *isPresent;
	int which;
	int *pslot;

	assert(pius);
	switch(pius->ordo )
	{
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		break;

	case Notitia::IC_DEV_INIT:
		WBUG("facio IC_DEV_INIT");
		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		m_error_buf = (char*)ps[1];
		*ret = dev_init();
		if ( *ret == 0 ) 
		{
			dev_ok = true;
			WBUG("R531 %s ok", comm_name);
		}
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
			return false;		//����豸û��׼����, �������������
		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		m_error_buf = (char*)ps[1];
		uid = (char*)ps[3];
		*ret = Pro_Open(uid);
		if ( *ret == 0 ) 
		{
			card_ok = true;
			WLOG(INFO, "Open Card ok uid=%s", uid);
		} else {
			card_ok = false;
			WLOG(INFO, "Open Card error: %s", m_error_buf);
		}
		break;

	case Notitia::IC_CLOSE_PRO:
		WBUG("facio IC_CLOSE_PRO dev_ok %d", dev_ok);
		if ( !dev_ok ) 
			return false;		//����豸û��׼����, �������������
		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		m_error_buf = (char*)ps[1];
		card_ok = false;
		*ret = Pro_Close();
		break;

	case Notitia::IC_SAM_COMMAND:
		WBUG("facio IC_SAM_COMMAND sam_ok %d", card_ok);
		if ( !sam_ok ) 
			return false;		//����û���û��׼����, �������������
		which = 0;
		goto COMM;

	case Notitia::IC_PRO_COMMAND:
		WBUG("facio IC_PRO_COMMAND pro_ok %d", card_ok);
		if ( !card_ok ) 
			return false;		//����û���û��׼����, �������������
		which = 1;
COMM:
		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		m_error_buf = (char*)ps[1];
		if ( which == 0 ) 
		{
			pslot = (int*)ps[2];
			if ( pslot)
			{
				gCFG->sam_face = 0xFF & (*pslot);
			}
		}
		comm = (char*)ps[3];
		reply = (char*)ps[4];
		psw = (int*)(ps[5]);

		*ret = CardCommand (comm, reply, which, psw);
		if ( *ret != 0 ) 
		{
			card_ok = false;
		} else {
			WLOG(INFO, "%s req(%s) res(%s) sw(%04x)", which==0?"SamCom":"ProCom", comm, reply, *psw);
		}
		break;

	case Notitia::IC_RESET_SAM:
		WBUG("facio IC_RESET_SAM dev_ok %d", dev_ok);
		if ( !dev_ok ) 
			return false;		//����豸û��׼����, �������������
		ps = (void**)(pius->indic);
		ret = (int*)ps[0];
		m_error_buf = (char*)ps[1];
		pslot = (int*)ps[2];
		if ( pslot)
		{
			gCFG->sam_face = 0xFF & (*pslot);
		}
		atr = (char*)ps[3];
		*ret = Sam_Reset(atr);
		if ( *ret == 0 ) 
		{
			WLOG(INFO, "SAM reset ok %s", atr);
			sam_ok = true;
		} else {
			WLOG(ERR, "SAM reset failed");
			sam_ok = false;
		}
		break;

	case Notitia::IC_PRO_PRESENT:
		WLOG(NOTICE, "facio IC_PRO_PRESENT dev_ok %d", dev_ok);
		if ( !dev_ok ) 
			return false;		//����豸û��׼����, �������������
		ps = (void**)(pius->indic);
		isPresent = (int*)ps[0];
		m_error_buf = (char*)ps[1];
		if ( Pro_Present() )
			*isPresent = (*isPresent)+1;
		break;
	default:
		return false;
	}
	return true;
}

bool ToR531::sponte( Amor::Pius *pius)
{
	return false;
}

ToR531::ToR531()
{
	m_error_buf = (char*) 0;
	dev_ok = false;
	card_ok = false;
	sam_ok = false;
	hDev = -1;
	op_mutex=NULL;		//����������, ��ֹ̽����������
	gCFG = 0;
	has_config = false;
}

ToR531::~ToR531()
{
	if ( has_config && gCFG )
		delete gCFG;
}

Amor* ToR531::clone()
{
	ToR531 *child = new ToR531();
	child->gCFG =gCFG;
	return (Amor*)child;
}
#define AMOR_CLS_TYPE ToR531
#include "hook.c"
