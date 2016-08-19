/******************************************************************************************
 ** �ļ����ƣ�R531DeviceFunc.cpp                                                         **
 ** �ļ�������ROCKEY531�豸�ӿں���														 **
 **--------------------------------------------------------------------------------------**
 ** �� �� �ˣ����ܿ�ҵ��                                                               **
 ** �������ڣ�2012-07-05                                                                 **
 **--------------------------------------------------------------------------------------**
 ** �� �� �ˣ�                                                                           **
 ** �޸����ڣ�                                                                           **
 **--------------------------------------------------------------------------------------**
 ** �� �� �ţ� V1.0                                                                      **
 **--------------------------------------------------------------------------------------**
 **                      Copyright (c) 2012  ftsafe                                      **
 ******************************************************************************************/
#include <stdio.h> 
#include <stdlib.h>
#include <memory.h> 
#include <math.h>
#include <time.h>

// �߳�ͬ���������ͷ�ļ�����

#include <windows.h>
#include <process.h>
//#include <afxmt.h>

// �����⺯��
#include "ATR.h"
#include "publicFunc.h"
#include "R531DeviceFunc.h"

// HID �豸��ͷ�ļ�
extern "C" {
#include "hidsdi.h"			// Must link in hid.lib
#include "setupapi.h"		// Must link in setupapi.lib
}

// �����������Ϣ�ṹ
#define MAXDEVICENUM			 10			// ROCKEY531�豸�������

// �豸���������Ϣ���ݽṹ
typedef struct {
	int    nConnStatus;						// �豸����״̬��-1:�޴˶�������0-δ���ӣ�1-������ ;2-�ѹر�
	char   szDevName[128];					// HID�豸����,�����������USB�ڲ�ͬ��HID�豸���Ʋ�ͬ
	HANDLE hDev;							// HID�豸���
	int    nMaxDataLen;						// ���ݽ���������ݳ���
	int    nUserFlag;						// �û�ģ�鴦���־��0-δ�ϵ磻1-���ϵ縴λ��2-���µ�
	int    nSIM1Flag;						// SIM1ģ�鴦���־��
	int    nSIM2Flag;						// SIM2ģ�鴦���־��
	int    nSIM3Flag;						// SIM3ģ�鴦���־��
	int    nUserProtocol;					// �û����ۣ�IC��ͨ��Э�飻-1: δ���ã�0-T=0;1-T=1;
	int    nSIM1Protocol;					// SIM1���ۣ�IC��ͨ��Э�飻
	int    nSIM2Protocol;					// SIM2���ۣ�IC��ͨ��Э�飻
	int    nSIM3Protocol;					// SIM3���ۣ�IC��ͨ��Э�飻
	int    nCurrSlot;						// ��ǰָ�������ģ���ţ�-1:δ���ã�0:�û���ģ�飻1:SIM1��2:SIM2;3��SIM3;
	int    nCardType;						// �ǽӿ�����, 4 - M1S50;	2 - M1S70;	8 - MPR0
	char   szInsCls[5];						// �ϸ�ָ��INS��CLAֵ	�÷ǽӿ�����Ϊ��02 ��M1S70��	-- Add - 20120808
} DeviceStruct;



// ROCKEY501-CCID �豸��Ϣ
static DeviceStruct stDeviceStruct[MAXDEVICENUM];

// �豸���ӱ�־
static int gInitFlag = 0;
static int gDeviceTotal    = 0;			// ��ǰϵͳROCKEY501-CCID�豸����

//CCriticalSection oCriticalSection;		// �����߳��ٽ��������
static HANDLE op_mutex=NULL;


static int gCommFlag;					// �ǽ�ָ��ִ����ż��־
static int gCommFlag2;                  // �Ӵ�T=1��ָ��ִ����ż��־


//==================================================================================================
//1-R531����������������װ
//==================================================================================================
/****************************************************************************************** 
 *******    �������ƣ�R531DeviceFind		-��ţ�1 -								*******
 *******	�������ܣ���ȡϵͳ��HID-ROCKETY531,�����豸���Ƽ����Ӿ�����浽ȫ�ֱ���	*******
 *******----------------------------------------------------------------------------******* 
 *******	����������int pDeviceSeq:Ҫ�򿪵�ROCKEY501-CCID �豸���(1~9)			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ��豸������ʧ�ܷ��أ�<=0	;�����������,				******* 
 *******                 0��ϵͳ��ROCKEY501������									*******
 *******				-1��ȡϵͳHID�豸��Ϣ��ʧ��									*******
 *******				-2������HID�ӿ��豸��ϸ��Ϣ�ṹָ��ʧ��						*******
 *******				-3����ȡHID�豸��ϸ��Ϣʧ��									*******
 *******				-4��ִ��HidD_GetPreparsedData����ʧ��						*******
 *******				-5��ִ��HidP_GetCaps����ʧ��								*******
 ******************************************************************************************/
int R531DeviceFind()
{
	int nRet = 1;
	int nDeviceNum;	
	int nNameSize = 21;
	int i;
	GUID guidHID;
  	DWORD length;	
	HDEVINFO hDevInfo;
	BOOL Success;
	HANDLE HIDHandleRead;					 
	SP_DEVICE_INTERFACE_DATA InterfaceData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA DetailData;
	HIDD_ATTRIBUTES Attribute;

	// ����ͬ�����ô��룬����ٽ�������
	//oCriticalSection.Lock();
	if (op_mutex == NULL)
	{
		op_mutex = CreateMutex( NULL, // default security attributes
									FALSE, // initially not owned
									"r531");
		if (op_mutex == NULL) 
		{
			return -100;
		}
	}

	if (WaitForSingleObject(op_mutex, 10000) != WAIT_OBJECT_0 )
		return -200;


	int nFindFlag  = 0 ;			 

	char szDeviceInfo[512];
	char szName[81];

 	char name[ 200 ];
	char fullname[ 1000 ] = "";
	char szFTDeviceName[256];
	char szFTDevicePath[256];
	int nDataLen;

	memset(szDeviceInfo,0x00,sizeof(szDeviceInfo));

	memset(name,0x00,sizeof(name));
	memset(fullname,0x00,sizeof(fullname));
	memset(szFTDeviceName,0x00,sizeof(szFTDeviceName));
	memset(szFTDevicePath,0x00,sizeof(szFTDevicePath));
 
	gCommFlag = 0; 
	gCommFlag2 =0;

	// ��ʼ���豸��Ϣ�ṹ
	//stDeviceStruct
	if( gInitFlag  == 0)
	{
		for(i=0; i< MAXDEVICENUM;i++ )
		{
			stDeviceStruct[i].nConnStatus	= CONNSTATNODEV;	// ��ʼ��Ϊ�޴��豸
			memset(stDeviceStruct[i].szDevName,0x00,128);		// HID�豸·������
			stDeviceStruct[i].hDev = INVALID_HANDLE_VALUE;		// �豸���
			stDeviceStruct[i].nMaxDataLen	= 0;				// ���ݽ�������󳤶�
			stDeviceStruct[i].nUserFlag		= RESETSTATNOT;		// �豸�û���ģ���ϵ��־��0-δ�ϵ磻1-���ϵ磻2-���µ�; 3-�ϵ粻�ɹ�[Add-20120319]
			stDeviceStruct[i].nSIM1Flag		= RESETSTATNOT;		// SIM1ģ���ϵ��־
			stDeviceStruct[i].nSIM2Flag		= RESETSTATNOT;		// SIM2ģ���ϵ��־
			stDeviceStruct[i].nSIM3Flag		= RESETSTATNOT;		// SIM3ģ���ϵ��־		
			stDeviceStruct[i].nUserProtocol = R531PROTOCOLNO;	// �豸�û���ģ��ͨ��Э��,-1: δ���ã�0��T=0;  1��T=1;
			stDeviceStruct[i].nSIM1Protocol	= R531PROTOCOLNO;	// SIM1ģ��ͨ��Э��
			stDeviceStruct[i].nSIM2Protocol	= R531PROTOCOLNO;	// SIM1ģ��ͨ��Э��
			stDeviceStruct[i].nSIM3Protocol	= R531PROTOCOLNO;	// SIM1ģ��ͨ��Э��
			stDeviceStruct[i].nCurrSlot		= -1;				// ��ǰ������ʹ�õĿ��ۣ�-1:δ���ã�0:�û���ģ�飻1:SIM1��2:SIM2;3��SIM3

			memset(stDeviceStruct[i].szInsCls,0x00,5);			// ��������Ϊ��0x02 �ķǽ�IC��
		}
	}


	// ��ȡHID �豸GUID 
	HidD_GetHidGuid(&guidHID);


	// ��ȡHID�豸��Ϣ��
	hDevInfo = SetupDiGetClassDevs( &guidHID, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE );
	if(hDevInfo == INVALID_HANDLE_VALUE)
	{
		//oCriticalSection.Unlock();
		ReleaseMutex(op_mutex);
		return  R531SYSERR;
	}

	memset( &InterfaceData, 0, sizeof( SP_DEVICE_INTERFACE_DATA ) );
	InterfaceData.cbSize = sizeof( SP_DEVICE_INTERFACE_DATA );
	
	// Ҫ��ȡ��ϵͳHID�豸���,
	i = 0;

	nRet = 0;
	nDeviceNum = 0;
	while(1)
	{
		// 3.1�����豸��Ϣ���л�ȡ�豸��Ϣ
		if( !SetupDiEnumDeviceInterfaces( hDevInfo, NULL, &guidHID, i, &InterfaceData ) )
		{ 
			SetupDiDestroyDeviceInfoList( hDevInfo );
			nRet = nDeviceNum;
			break;
			//return nDeviceNum;
		}

		length = 0;

		// 3.2.��ȡ�豸�ӿ���ϸ��Ϣ
		Success = SetupDiGetDeviceInterfaceDetail( hDevInfo, &InterfaceData, NULL, 0, &length, NULL );
		 
		if(Success != 0)	
		{
			SetupDiDestroyDeviceInfoList( hDevInfo );
			i++;
			continue;
		}

		// ����ӿ���ϸ��Ϣ�ṹָ��
		DetailData = ( PSP_DEVICE_INTERFACE_DETAIL_DATA )LocalAlloc( LMEM_ZEROINIT, length );
		if( NULL == DetailData )
		{
			SetupDiDestroyDeviceInfoList( hDevInfo );
			nRet = R531SYSERR;
			break;
			//return -2;
		}

		DetailData->cbSize = sizeof( SP_DEVICE_INTERFACE_DETAIL_DATA );  
		if( !SetupDiGetDeviceInterfaceDetail( hDevInfo, &InterfaceData, DetailData, length, NULL, NULL ) )
		{
			LocalFree( DetailData );
			SetupDiDestroyDeviceInfoList( hDevInfo );
			nRet = R531SYSERR;
			break;
			//return -3;
		}

		memset(szFTDevicePath,0x00,sizeof(szFTDevicePath));
			//strcpy_s(szFTDevicePath,255,DetailData->DevicePath);
		strcpy(szFTDevicePath,DetailData->DevicePath);
		//printf("%s\n", DetailData->DevicePath);


		//���豸,��ȡ���,�򿪷�ʽ��Ϊ�ɶ�д
		HIDHandleRead = CreateFile( DetailData->DevicePath, GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,			               
		                            ( LPSECURITY_ATTRIBUTES )NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

		// ���ļ�ʧ�ܴ�����һ��HID�豸��Ϣ
		if( HIDHandleRead == INVALID_HANDLE_VALUE)
		{
			i++;
			LocalFree( DetailData );
			continue;	 			
		}

		Attribute.Size = sizeof( HIDD_ATTRIBUTES );
		Success = HidD_GetAttributes(HIDHandleRead, &Attribute);
		if( !Success)
		{
			i++;
			LocalFree( DetailData );
			CloseHandle(HIDHandleRead);		// �ر��豸���
			continue;			
		}

		// ��ȡ��Ʒ����
		memset( name, 0, sizeof( name ) );
		HidD_GetProductString(HIDHandleRead, name, sizeof( name ) );   
	
		memset(szName,0x00,sizeof(szName));
		//printf("pname %s\n", name);
	
		//sprintf_s(szName,81,"%ws",name);
		sprintf(szName,"%ws",name);

		// ����Ƿ�Ϊ����� HID ,ROCKEY501�豸VID=096E PID=0603
		if((Attribute.VendorID == 0x096E)  && (Attribute.ProductID == 0x603))
		{
			memset(szFTDeviceName,0x00,sizeof(szFTDeviceName));
			//sprintf_s(szFTDeviceName,81,"%s %d",szName,nDeviceNum);
			sprintf(szFTDeviceName,"%s %d",szName,nDeviceNum);

	
			//ROCKEY501�豸ͨѶ�˿ڳ�ʼ��
			PHIDP_PREPARSED_DATA	PreparsedData;
			
			// ��ȡ���㼯��Ԥ��������
			if( !HidD_GetPreparsedData( HIDHandleRead, &PreparsedData ) )
			{ 
				LocalFree( DetailData );
				CloseHandle(HIDHandleRead);		// �ر��豸���
				nRet = R531SYSERR;
				break;
				//return -4;				
			}

			// ��ȡ���㼯�ϵ�HIDP_CAPS�ṹ
			HIDP_CAPS				Capabilities;
			if( !HidP_GetCaps( PreparsedData, &Capabilities ) )
			{
				LocalFree( DetailData );
				CloseHandle(HIDHandleRead);		// �ر��豸���
				nRet = R531SYSERR;
				//return -5;
			}

			// ��ȡROCKEY501����/�������ݵ���󳤶�
			nDataLen  = Capabilities.FeatureReportByteLength;

			LocalFree( DetailData );

			// �ر��豸���,���ϲ㺯��ȥ���豸
			CloseHandle(HIDHandleRead);		

			if( gInitFlag == 0)
			{

				// ����ROCKEY501-HID�豸��Ϣ-Add-20120106
				stDeviceStruct[nDeviceNum].nConnStatus = 0;									// �豸�Ѵ�δ����
				stDeviceStruct[nDeviceNum].nMaxDataLen = nDataLen;							// �豸���ݽ�������

				// HID�豸·������
				//strcpy_s(stDeviceStruct[nDeviceNum].szDevName,127,szFTDevicePath);
				strcpy(stDeviceStruct[nDeviceNum].szDevName,szFTDevicePath);
				nDeviceNum ++;
				gDeviceTotal ++;

			}else 
			{
				int nExitFlag = 0;
				int j;

				nNameSize = (int)strlen(szFTDevicePath);


				for(j=0; j<gDeviceTotal; j++)
				{
					if( memcmp(stDeviceStruct[j].szDevName,szFTDevicePath,nNameSize) == 0)
					{
						nExitFlag = 1;
						break;
					}

				}

				if( nExitFlag == 0)
				{
					stDeviceStruct[gDeviceTotal].nConnStatus = CONNSTATNOCONN;						// �豸�Ѵ�δ����
					stDeviceStruct[gDeviceTotal].nMaxDataLen = nDataLen;							// �豸���ݽ�������

					// HID�豸·������
					//strcpy_s(stDeviceStruct[gDeviceTotal].szDevName,127,szFTDevicePath);
					strcpy(stDeviceStruct[gDeviceTotal].szDevName,szFTDevicePath);
					gDeviceTotal ++;					
				}
			}

			// ϵͳ����R531�豸
			nFindFlag = 1;	
			// �жϻ�ȡ���豸�Ƿ�ﵽ���ֵ��10��
			if( nDeviceNum >= MAXDEVICENUM )
			{
				break;
			}
			
		}	// END IF 
		i++;
	}    // End While


	SetupDiDestroyDeviceInfoList( hDevInfo );

	// ����Ƿ��ȡ��������
	if( nRet >= 0)
	{
		// ���ö�����������ʼ����־
		gInitFlag = R531INITFLAGOK;

		// ���ֶ���������
		nRet = gDeviceTotal ;
	}
	
	
	//oCriticalSection.Unlock();
	ReleaseMutex(op_mutex);
	return nRet;
}


// ���Ӷ�����UID��֤
/****************************************************************************************** 
 *******    �������ƣ�R531ConnDev			-��ţ�2 -								*******
 *******	�������ܣ�����ָ��USB�ڵ�R531�豸										*******
 *******----------------------------------------------------------------------------******* 
 *******	����������char *pDevName:�豸����,ֵΪ��USBn[nֵΪ:1~9]					*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ��豸���ֵ��ΧΪ:1~9��ʧ�ܷ��ش������				******* 
 ******************************************************************************************/
int R531ConnDev(char *pDevName)
{	
	int    nDevNo;
	char   szDevPath[256];
	char   szName[4];
	HANDLE hDev;

	memset(szName,0x00,sizeof(szName));

	// ��鴫������ĺϷ���
	if( strlen(pDevName)!= 4)
	{	// ����������ȷǷ�
		return R531PARAMERR;
	}

	// ȡǰ�����ֽ�
	memcpy(szName,pDevName,3);

	// ���ַ���ת���ɴ�С
	ftStringToUpper(szName);

	// Update��20120708,�����USBֵ�����ִ�Сд
	if( memcmp(szName,"USB",3)!= 0)
	{	// ��������Ƿ�
		return R531PARAMERR;
	}

	if( pDevName[3]< '1' || pDevName[3]>'9')
	{
		return R531PARAMERR;
	}

	// ��ȡ����������豸���ֵ��ΧΪ��1~9
	nDevNo = pDevName[3] -'0';

	// ת��Ϊ�ڲ����ֵĶ������豸���
	nDevNo -= 1;

	// ����Ƿ��ѽ����豸��ʼ��
	if( gInitFlag == R531INITFLAGNO)
	{
		return R531NOINIT;
	}

	// ���������Ƿ����
	if( nDevNo >= gDeviceTotal)
	{
		return R531NOEXIST;
	}

	memset(szDevPath,0x00,sizeof(szDevPath));

	// ��ȡ�豸·������
	//strcpy_s(szDevPath,256,stDeviceStruct[nDevNo].szDevName);
	strcpy(szDevPath,stDeviceStruct[nDevNo].szDevName);

	// �����豸��Դ
	hDev = CreateFile( szDevPath, GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,			               
		( LPSECURITY_ATTRIBUTES )NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

	if( hDev == INVALID_HANDLE_VALUE)
	{
		return R531CONNERR;
	}

	// �����ڲ��ṹ,���ֵ
	stDeviceStruct[nDevNo].hDev        = hDev;				
	stDeviceStruct[nDevNo].nConnStatus = CONNSTATCONN;		


	// ���ú�������ֵΪ���ӵ�R531���������(��1��ʼ,���ֵΪ��9)
	return nDevNo+1;
}


/****************************************************************************************** 
 *******    �������ƣ�R531CloseDev			-��ţ�3 -								*******
 *******	�������ܣ��ر�ָ��USB�����ӵ�R531�豸									*******
 *******----------------------------------------------------------------------------******* 
 *******	����������int pDevHandle��Ҫ�رյ��豸���ֵΪ:1~9]						*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�R531OK,ʧ�ܷ���ֵ����������						******* 
 ******************************************************************************************/
int R531CloseDev(int pDevHandle)
{	
	int    nDevNo;


	// ת��Ϊ�ڲ����ֵĶ������豸���
	nDevNo = pDevHandle - 1;

	// ����Ƿ��ѽ����豸��ʼ��
	if( gInitFlag == R531INITFLAGNO)
	{
		return R531NOINIT;
	}

	// ���������Ƿ����
	if( nDevNo >= gDeviceTotal || nDevNo < 0 )
	{
		return R531NOEXIST;
	}

	// ���������Ƿ�������
	if( stDeviceStruct[nDevNo].nConnStatus != CONNSTATCONN)
	{
		return R531UNCONN;
	}

	// �ر��豸
	CloseHandle(stDeviceStruct[nDevNo].hDev );

	// ���ö�������Ϣ�ṹ���鵱ǰ��������Ϣ��Ϊδ���ӵ�״̬
	stDeviceStruct[nDevNo].hDev		     = INVALID_HANDLE_VALUE;
	stDeviceStruct[nDevNo].nConnStatus   = CONNSTATCLOSE;

	// �ϵ縴λ��־�޸�Ϊ��RESETSTATNOT(δ�ϵ�)
	stDeviceStruct[nDevNo].nUserFlag     = RESETSTATNOT;
	stDeviceStruct[nDevNo].nSIM1Flag     = RESETSTATNOT;
	stDeviceStruct[nDevNo].nSIM2Flag     = RESETSTATNOT;
	stDeviceStruct[nDevNo].nSIM3Flag     = RESETSTATNOT;

	// IC��ͨ��Э���޸�Ϊ��R531PROTOCOLNO(δ֪)
	stDeviceStruct[nDevNo].nUserProtocol = R531PROTOCOLNO;
	stDeviceStruct[nDevNo].nSIM1Protocol = R531PROTOCOLNO;
	stDeviceStruct[nDevNo].nSIM2Protocol = R531PROTOCOLNO;
	stDeviceStruct[nDevNo].nSIM3Protocol = R531PROTOCOLNO;

	// ����������ѡ���޸�Ϊ��R531SLOTNOTSET(δѡ��)
	stDeviceStruct[nDevNo].nCurrSlot     = R531SLOTNOTSET;

	return R531OK;

}


/****************************************************************************************** 
 *******	�������ƣ�R531DeviceCommand		-��ţ�4 -								*******
 *******	�������ܣ�HID-ROCKEY531�豸ͨѶ����										******* 
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******			  int  DevNo:�豸���,��1��ʼ��ΧΪ(1~9)						*******
 *******			  char *Send:�豸ָ������ASCII[ֻ������Cmdcls��CmdSubcls,Data]	*******
 *******			  int   SendLen:�豸ָ�����ݳ���[��ASCII����豸ָ���]		*******
 *******			  char *Recv:(���)�豸ָ����Ӧ����ASCII[Data]					*******
 *******			  int  *RecvLen:(���)��Ӧ����ASCII�볤��						*******
 *******			  int  *Status:(���)�豸ָ��Ӧ��״ֵ̬�ɹ�Ϊ��0				*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܼ���������									*******
 ******************************************************************************************/
int R531DeviceCommand(int DevNo,char *Send,int SendLen,char *Recv,int *RecvLen,int *Status)
{
	char szLRC[2];
	
	time_t lStartTime = 0;						// ���տ�ʼʱ�䣻��λΪ��
	time_t lTime = 0;							// ��ǰʱ��
	time_t lTimeCount = 0;						// ��������ʱ����λΪ��
	int  nDevNo;

	char szSend[1024];
	char szHexSend[1024];
	unsigned char szHeader[3];
	unsigned char szTmp[1024];					// Ҫ���͵���ʱ����
	unsigned char mybuffer[ 1024 ];
	unsigned char szDevSn[2+1];					// �豸���
	BOOLEAN  Result;
	int nStatus;								// �豸ָ��Ӧ��״̬��Ϣ
	int nMaxLen;								// �������������ݽ�������
	HANDLE HIDHandleRead;
	int  nSendLen;
	int  nPageDataLen ;							// �����ݳ���
	int  slen;
	unsigned char  cSeq = 0x00;					// ���Ͱ����
	int            nSeq = 0;					// ���Ͱ����к�
	int package_size, Pakoffset, TotalLength;
	char *p;
	int  gRecvTime = 60;						// ���ý���Ӧ����ʱ��Ϊ��30 ��

	memset(szSend,0x00,sizeof(szSend));
	memset(szHexSend,0x00,sizeof(szHexSend));

	memset(szDevSn,0x00,sizeof(szDevSn));

	memset(szLRC,0x00,sizeof(szLRC));
	memset(szHeader,0x00,sizeof(szHeader));

	szDevSn[1] =  0;

	// ��������豸���ת�����ڲ�����Ķ��������
	nDevNo = DevNo -1; 

	// ��鴫������ĺϷ���
	if( nDevNo < 0 || nDevNo >= MAXDEVICENUM)
	{
		return R531PARAMERR;
	}

	// ��鴫��ָ��ĺϷ��ԣ�ָ��ʵ�ʳ����Ƿ��뺯����ָ��Ȳ���һ�£�ASCII���ָ��Ȳ����Ƿ�Ϊ2�ı���
	nSendLen  = (int)strlen(Send);
	if( nSendLen != SendLen )
	{
		return R531PARAMERR;	
	}
	if(( SendLen % 2 ) != 0)
	{
		return R531PARAMERR;	
	}

	// ������л����ĺϷ���
	if( nDevNo >= gDeviceTotal)
	{
		return R531UNCONN;
	}

	if( stDeviceStruct[nDevNo].nConnStatus != CONNSTATCONN)
	{
		return R531UNCONN;
	}

	// �����豸��Ŵӱ���ľ�̬�ṹ�л�ȡ�豸���
	HIDHandleRead = stDeviceStruct[nDevNo].hDev;

	// ��ȡ���ݽ�������󳤶�
	nMaxLen = stDeviceStruct[nDevNo].nMaxDataLen;

 	// ������뻺����
	HidD_FlushQueue( HIDHandleRead );

	// BCD���ָ���
	nSendLen = SendLen / 2;        
	memset(szHexSend,0x00,sizeof(szHexSend));	

	// ��ASCII���ָ��ת����BCD���ָ��
	ftAtoh(Send,szHexSend,nSendLen);
 
 
	// ���͵Ŀ�ʼλ��
	Pakoffset  = 0;		
	

	TotalLength = nSendLen;
	nPageDataLen = nSendLen;
	p = szHexSend;

	// ѭ����������ָ�����ݵ�ROCKEY501������
	nSeq = 0;

	// ����2�ֽڵ�ָ��ͷ
	memcpy(szHeader,szHexSend,2);

	int nHeadFlag = 0;
	while( TotalLength > 0 )
	{
		memset(mybuffer,0x00,sizeof(mybuffer));
	
		// ���õ�ǰҪ���͵����ݳ���
		if(TotalLength > (nMaxLen - 7))
		{
			package_size = nMaxLen -7;	
			cSeq = 0x80 | nSeq;							// ���ð����
		}else 
		{
			if( nHeadFlag == 1)
				nHeadFlag = 2;							// ���Ͱ�Ϊ������
			package_size = TotalLength;
			cSeq = nSeq;								// ���ð����
		}

		memset(szTmp,0x00,sizeof(szTmp));
		 
		mybuffer[0] = (unsigned char)0x00;				// Э�鱨��ǰ���ַ���	ֵΪ��0x00;
		mybuffer[1] = (unsigned char)0x02;				// Stx:1�ֽ�[����֡��ʼͷ]	ֵΪ��0x02

		if( nHeadFlag == 0)
 			mybuffer[2] = (unsigned char)(package_size+3);	// Len:1�ֽ�,��Fnum �� Lrc ֮ǰ�����ݳ���
		else if( nHeadFlag == 1)
			mybuffer[2] = (unsigned char)(package_size+3);
		else 
			mybuffer[2] = (unsigned char)(package_size+5);
			
		mybuffer[3] = (unsigned char)cSeq;				// Fnum:1�ֽڰ����                

		mybuffer[4] = (unsigned char)szDevSn[0];		// DevSn:�豸��ţ�2�ֽ�
		mybuffer[5] = (unsigned char)szDevSn[1];
		
		if( nHeadFlag == 0 )							// ��1����
		{
			memcpy(mybuffer+6,p,package_size);			// ��ӱ��ĵ�CmdCls��CmdSubClsֵ
		}else if ( nHeadFlag == 1)						// �м����ݰ�
		{
			memcpy(mybuffer+6,szHeader,2);
			memcpy(mybuffer+8,p,package_size-2);
		}else											// ���һ�����ݰ� 
		{
			memcpy(mybuffer+6,szHeader,2);	
			memcpy(mybuffer+8,p,package_size);
			package_size+=2;
		}
		

		// ����BCD�뷢�����ݵ�LRCֵ����ӵ��������
		ftCalLRC1((char *)mybuffer+2,package_size+4,szLRC,1); 
		
		// ��LRC��ӵ����ĺ���
		memcpy(mybuffer+ package_size+6,szLRC,1);
	 

		// ��������,ע������������������ΪHidP_GetCap��������HIPD_CAPS �ṹ��Ա����ֵ
		Result =	HidD_SetFeature( HIDHandleRead, mybuffer, nMaxLen );
		if(!Result)
		{

			return R531SYSERR;
		}

		HidD_FlushQueue( HIDHandleRead );

		if( nHeadFlag == 0)
		{	
			TotalLength -= package_size;					// ����ʣ�೤��
			p += package_size;								// �´η�������ָ��
		}else 
		{
			TotalLength =TotalLength - (package_size -2) ;	// ����ʣ�೤��
			package_size -=2;
			p += package_size;
		}
		
		nSeq += 1;											// ������͵����
		nHeadFlag = 1;										// ��ʾ����İ���Ҫ����2�ֽ�ָ��ͷ

	}


 	memset(szHexSend,0x00,sizeof(szHexSend));
	memset(szSend,0x00,sizeof(szSend));

	// ���յ�һ��Ӧ����,���Ӧ������ݳ���Ϊ��0���ð����������½���
	Pakoffset = 0;											// ���յ�Ӧ������λ��ƫ����
	TotalLength = 0;										// BCD���Ӧ�������ܳ���
	slen = 0;
	nSeq = 0;												// ���հ����
	HidD_FlushQueue( HIDHandleRead );

	// ��λΪ����
	DWORD dwStartTime;
	DWORD dwCurrTime;
	DWORD dwRunTime;

	// ��ȡ��ʼʱ��
	dwStartTime = GetTickCount();

	while( 1 )
	{
		memset(mybuffer,0x00,sizeof(mybuffer));

		// �����豸Ӧ������
		Result = HidD_GetFeature( HIDHandleRead, mybuffer, nMaxLen );
		if( !Result)
		{

			return R531SYSERR;
		}
		
		// ����Ӧ��ɹ��Ĵ���
		slen = (int)mybuffer[2];		// ��Fnum �� Lrc ֮ǰ�����ݳ���
		slen -=4;						// �豸ָ��Ӧ��ģ�״̬��(1) + Ӧ����Ϣ�ĳ��ȣ�
		
		nStatus = (int)mybuffer[6];		// �豸ָ����Ӧ״̬��Ϣ
			
		// Add:20110714 ,���Ӧ��״̬Ϊ��0x88 ����æ���������
		if( nStatus == 0x88 )
		{
			
			// ��ȡ��ǰʱ��
			dwCurrTime = GetTickCount();

			// ��������ʱ��(��λΪ����)
			dwRunTime = dwCurrTime - dwStartTime;

			// ����Ƿ�ʱgRecvTimeֵΪ��
			if( (int)dwRunTime > gRecvTime*1000)
			{
				return R531SYSERR;
			}

			// ��ͣ 20 ���룬�ȴ����ݵ���
			Sleep(20);

			// ����ִ��ѭ������Ӧ������
			continue;


			/* Del - 20120716
			// ���ӽӳ�ʱʱ����
			if ( lStartTime == 0)		// ��ȡ��ʼʱ��,��λΪ����
			{
				time(&lStartTime);
				continue;
			}else {						// ��ȡ��ǰʱ��,��������յ�ǰʹ�õ�ʱ��
				time(&lTime);
			}

			// ���㵱ǰ����ʹ�õ�ʱ�䣬��λΪ����
			lTimeCount = (long)lTime - lStartTime;

			// ����Ƿ���ճ�ʱ
			if(lTimeCount >= gRecvTime)
			{	// ���ճ�ʱ
				return R531SYSERR;
			}			
			
			// ���ӵȴ�20����  -- ԭֵ
			// UPDATE :20120528 ,---�����ϵ縴λ�Ĵ���ʱ��
			Sleep(20);

			continue;
			*/
		}
	

		TotalLength += slen;

		// ����״̬�� + Ӧ����Ϣ 
		memcpy(szHexSend + Pakoffset,mybuffer+7,slen);

		// ȡFnum[����֡���],
		cSeq = mybuffer[3];
		
		Pakoffset += slen;

		// �޺�������ֱ�ӷ���
		if(cSeq == 0x00 )
		{
			break;
		}
	}	


	// Add - 20120708 ���ָ���״̬���Ƿ���ȷ������ȷ�������ر���
	if( nStatus != 0)
	{
		*Status		= nStatus;

		// Ӧ�����
		return R531SYSERR;
	}

	
	memset(szSend,0x00,sizeof(szSend));
	ftHtoa(szHexSend,szSend,TotalLength);

 
	// ���ú����������ֵ
	TotalLength = TotalLength*2;
	*RecvLen	= TotalLength;
	*Status		= nStatus;

	memcpy(Recv,szSend,TotalLength);

	return R531OK;
}



//==================================================================================================
//2-�豸�ຯ����װ
//==================================================================================================
/******************************************************************************************
 *******	�������ƣ�R531DevSeqNo					-��ţ�5						*******
 *******	�������ܣ���ȡROCKEY531�豸��Ʒ���к�,ָ��[8000]						*******
 *******----------------------------------------------------------------------------*******
 *******	����������                      										*******
 *******			  int hDev���豸�����ֵ��ΧΪ��1~9								*******
 *******			  char *OutBuf��(���)��Ʒ���к�,��ʽ���£�		    			*******
 *******			            8�ֽڵ������գ��磺20110708							*******
 *******			            6�ֽڵ�ʱ���룬�磺153030							*******
 *******			            8�ֽڵĻ����ţ��磺00000008							*******
 *******			            6�ֽڵ���ˮ�ţ��磺0x010203,�����"010203"			*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0	;�����������:						*******
 *******					-701:δ����ROCKEY501�豸ָ��״̬��Ϣ					*******
 *******					-702:״̬��Ϣ������										*******
 ******************************************************************************************/
int R531DevSeqNo(int hDev,char *OutBuf,int *Status)
{
/*
	char szDate[8+1];
	char szTime[6+1];
	char szMachineNo[8+1];		// ������
	char szDeviceNo[8+1];		// ��ˮ��
	int  nDeviceNo[3];			// ��ˮ��

	char szSeq[6+1];
	char szBitString[64+1];
	char szHexRecv[1024];
	int nVal;
*/
	char szSend[1024];
	char szRecv[1024];

	int nStatus;

	int nRet,nSendLen,nRecvLen;

/*
	int nPos;
	nDeviceNo[0] = 0;
	nDeviceNo[1] = 0;
	nDeviceNo[2] = 0;
*/

	// ���ó�ʼ��ֵ����ʧ���������Ӧ������Ϊ��NULL
	memcpy(OutBuf,"\x00",1);

//	memset(szMachineNo,0x00,sizeof(szMachineNo));

	// ���ö�ȡ��Ʒ���к�ָ��
	memset(szSend,0x00,sizeof(szSend));
	//strcpy_s(szSend,4,"8000");
	//nSendLen = (int)strlen(szSend);

	memcpy(szSend,"8000",4);
	nSendLen = 4;

	memset(szRecv,0x00,sizeof(szRecv));
	nRecvLen = 0;
	nStatus  = 0;

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	// ����豸ָ���Ӧ�������Ƿ���ȷ
	if( nRecvLen != 16 || strlen(szRecv) != 16)
	{
		return R531RESPDATALENERR;
	}
	
	memcpy(OutBuf, szRecv, nRecvLen);
	OutBuf[nRecvLen] = 0;
#ifdef MANA
	memset(szHexRecv,0x00,sizeof(szHexRecv));
	
	// ת����BCD��
	ftAtoh(szRecv,szHexRecv,nRecvLen/2);
	
	memset(szBitString,0x00,sizeof(szBitString));

	// ��BCD��ֵת����BIT������
	ftDataToBitString(szHexRecv,nRecvLen/2,szBitString);

	memset(szDate,0x00,sizeof(szDate));
	nPos = 0;

	// ת���� 6bit
	ftBitStringToVal(szBitString,6,&nVal);
	nVal += 2004;
	
	//sprintf_s(szDate,5,"%04.4d",nVal);
	sprintf(szDate,"%04.4d",nVal);

	nPos+=6;
	
	// ת���� 4bit
	ftBitStringToVal(szBitString+nPos,4,&nVal);
	//sprintf_s(szDate+4,3,"%02.2d",nVal);
	sprintf(szDate+4,"%02.2d",nVal);
	nPos+=4;

	//ת���� 5bit
	ftBitStringToVal(szBitString+nPos,5,&nVal);
	//sprintf_s(szDate+6,3,"%02.2d",nVal);
	sprintf_s(szDate+6,"%02.2d",nVal);
	nPos+=5;

	memset(szTime,0x00,sizeof(szTime));
	// ת��ʱ 5bit
	ftBitStringToVal(szBitString+nPos,5,&nVal);
	sprintf_s(szTime,3,"%02.2d",nVal);
	nPos+=5;
 
	// ת���� 6bit
	ftBitStringToVal(szBitString+nPos,6,&nVal);
	sprintf_s(szTime+2,3,"%02.2d",nVal);
	nPos+=6;

	// ת���� 6bit
	ftBitStringToVal(szBitString+nPos,6,&nVal);
	sprintf_s(szTime+4,3,"%02.2d",nVal);
	nPos+=6;

	memset(szDeviceNo,0x00,sizeof(szDeviceNo));
	// ������� 8 bit
	ftBitStringToVal(szBitString+nPos,8,&nVal);
	sprintf_s(szMachineNo,9,"%08.8d",nVal);
	nPos+=8;

	// ��ˮ�� -��1λ
	memset(szSeq,0x00,sizeof(szSeq));
	ftBitStringToVal(szBitString+nPos,8,&nVal);
	nDeviceNo[0] = nVal;
	nPos+=8;

	// ��ˮ�� -��2λ
	memset(szSeq,0x00,sizeof(szSeq));
	ftBitStringToVal(szBitString+nPos,8,&nVal);
	nDeviceNo[1] = nVal;
	nPos+=8;

	// ��ˮ�� -��3λ
	memset(szSeq,0x00,sizeof(szSeq));
	ftBitStringToVal(szBitString+nPos,8,&nVal);
	
	nDeviceNo[2] = nVal;
	nPos+=8;

	// ������ˮ��
	nPos = nDeviceNo[2] / 100 ;
	if(nPos > 0)
	{
		nDeviceNo[1] +=nPos;
		nDeviceNo[2] = nDeviceNo[2] % 100;
	}

	nPos = nDeviceNo[1] / 100 ;
	if(nPos > 0)
	{
		nDeviceNo[0] +=nPos;
		nDeviceNo[1] = nDeviceNo[1] % 100;
	}

	// ��λ����100 ֱ�Ӷ���

	nDeviceNo[0] = nDeviceNo[0] % 100;

	memset(szDeviceNo,0x00,sizeof(szDeviceNo));

	sprintf_s(szDeviceNo,9,"%02.2d%02.2d%02.2d",
				nDeviceNo[0],
				nDeviceNo[1],
				nDeviceNo[2]);


	memset(szRecv,0x00,sizeof(szRecv));

	sprintf_s(szRecv,1024,"%08.8s%06.6s%08.8s%06.6s",
				szDate,				// 8 �ֽ���
				szTime,				// 6 �ֽ���
				szMachineNo,		// 8 �ֽڻ������
				szDeviceNo			// 6 �ֽ���ˮ��
				);			
	nRecvLen = 8+6+8+6;
	memcpy(OutBuf,szRecv,nRecvLen);
	memcpy(OutBuf+nRecvLen,"\x00",1);

#endif
	return R531OK;
}


/******************************************************************************************
 *******	�������ƣ�R531DevBeep		-��ţ�6									*******
 *******	�������ܣ��豸������,ָ��[8001]											*******
 *******              ����˳��Ϊ������ͣ��ѭ������Ϊ:0,�������ʱ��Ϊ��0����ر�	*******
 *******              ʱ�䲻Ϊ:0,��һֱ������										*******
 *******			  ѭ������1~FF ʱ������������ʱ��ͷ�����Ъѭ��					*******
 *******----------------------------------------------------------------------------*******
 *******	����������unsigned char CycleNum��ѭ������;								*******
 *******			  unsigned char Times   ������ʱ��;								*******
 *******			  unsigned char Interval��������Ъ								*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
  *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0	;�����������:						*******
 *******					-701:δ����ROCKEY501�豸ָ��״̬��Ϣ					*******
 *******					-702:״̬��Ϣ������										*******
 ******************************************************************************************/
int R531DevBeep(int hDev,unsigned char CycleNum,unsigned char  Times,unsigned char Interval,int *Status)
{
	char szSend[1024];
	char szRecv[1024];

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;


	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//sprintf_s(szSend,1024,"8001%2.2X%2.2X%2.2X",CycleNum,Times,Interval);
	sprintf(szSend,"8001%2.2X%2.2X%2.2X",CycleNum,Times,Interval);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	return R531OK;
}



/******************************************************************************************
 *******	�������ƣ�R531DevResetHW	-��ţ�7									*******
 *******	�������ܣ���Ƶģ��Ӳ��λ,ָ��[8002]										*******
 *******----------------------------------------------------------------------------*******
 *******	����������unsigned char Msec:��λʱ��(1~FF)								*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0	;�����������:						*******
 *******					-701:δ����ROCKEY501�豸ָ��״̬��Ϣ					*******
 *******					-702:״̬��Ϣ������										*******
 ******************************************************************************************/
int R531DevResetHW(int hDev,unsigned char Msec,int *Status )
{
	char szSend[1024];
	char szRecv[1024];

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;

	
	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	// ���� ָ�� 8001 ѭ������(1�ֽ�) ����ʱ��(1�ֽ�) ������Ъ
	//sprintf_s(szSend,1024,"8002%2.2X",Msec);
	sprintf(szSend,"8002%2.2X",Msec);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	return R531OK;
}


/******************************************************************************************
 *******	�������ƣ�R531DevRField	-��ţ�8										*******
 *******	�������ܣ���Ƶ�����غ���,ָ��[8003]										*******
 *******----------------------------------------------------------------------------*******
 *******	����������unsigned char Mode:������ʽ������ֵ���£�						*******
 *******						0x00���رգ������Ч��								*******
 *******						0x01���ȹغ󿪣����Ϊ��Interval					*******
 *******						0x02���ȿ���أ����Ϊ��Interval					*******
 *******						0x03���򿪣����ֵ��Ч��							*******
 *******			  unsigned char Interval�����ʱ��;								*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0	;�����������:						*******
 ******************************************************************************************/
int R531DevRField(int hDev,unsigned char Mode,unsigned char Inteval, int *Status )
{
	char szSend[1024];
	char szRecv[1024];

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	// ��鴫��Ĳ�����ʽ�Ƿ���Ч
	if( Mode > 0x03 )
	{
		return R531PARAMERR;
	}

	// 8003:��Ƶ������
	//sprintf_s(szSend,1024,"8003%02.2X%02.2X",Mode,Inteval);
	sprintf(szSend,"8003%02.2X%02.2X",Mode,Inteval);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}
	return R531OK;
}


/******************************************************************************************
 *******	�������ƣ�R531DevSetRfMode	-��ţ�9									*******
 *******	�������ܣ��ǽӹ���ģʽ���ú���,ָ��[8004]								*******
 *******----------------------------------------------------------------------------*******
 *******	����������unsigned char Mode:������ʽ������ֵ���£�(Ĭ��ֵΪ��0x0A)		*******
 *******						0x0A����Ƶ������ģʽΪ��TypeA,M1ϵͳ��Ҳ����ģʽ��	*******
 *******						0x0B���ȹغ󿪣����Ϊ��Interval					*******
 *******			  unsigned char Speed�������ʣ�Mode= 0x0B[TypeB]ʹ��,ֵ���£�	*******
 *******						0x00:106KB��										*******
 *******						0x01:212KB;											*******
 *******						0x02:424KB;											*******
 *******						0x03:848KB;											*******
 *******			  ���Լ���4λ��ʾ��CwConductance�Ĵ�������ֵ����Χ��0~0x0F		*******
 *******			  ������ģʽΪTypeB����4�ֽ�Ϊ��CwConductance����ֵ��Χ0~0x0F	*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0	;�����������:						*******
 ******************************************************************************************/
int R531DevSetRfMode(int hDev,unsigned char Mode,unsigned char Speed ,int *Status )
{
	char szSend[1024];
	char szRecv[1024];

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	// ��鴫��Ĳ�����ʽ�Ƿ���Ч
	if( !(Mode == 0x0A  || Mode == 0x0B ))
	{
		return R531PARAMERR;
	}

	// ��鴫��ĵڶ��������ĺϷ���
	if( Mode == 0x0A)				// TypeA ʱ��2������ֵΪ��0x00��0x01��0x02��0x03
	{
		if( Speed < 0x00 || Speed > 0x03)
		{
			return R531PARAMERR;
		}
	}else							// TypeB ʱ��2������ֵΪ��0x00 ~ 0x0F
	{
		if( Speed < 0x00 || Speed > 0x0F)
		{
			return R531PARAMERR;
		}
	}

	// 8004:�ǽӹ���ģʽ����
	//sprintf_s(szSend,1024,"8004%02.2X%02.2X",Mode,Speed);
	sprintf(szSend,"8004%02.2X%02.2X",Mode,Speed);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}


	return R531OK;
}


/******************************************************************************************
 *******	�������ƣ�R531DevGetRfMode	-��ţ�10									*******
 *******	�������ܣ���ȡ�ǽӹ���ģʽ����,ָ��[8005]								*******
 *******----------------------------------------------------------------------------*******
 *******	����������int* Mode:(���)��0x0A-TypeA��0x0B - TypeB					*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0	;�����������:						*******
 ******************************************************************************************/
int R531DevGetRfMode(int hDev, int *Mode,int *Status )
{
	char szSend[1024];
	char szRecv[1024];

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//sprintf_s(szSend,1024,"8005");
	sprintf(szSend,"8005");
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}


	// ��Ӧ���ģʽֵת�������� 0x0A --> 10 ; 0x0B --> 11 ;
	*Mode = (int)ftHexToLong(szRecv);

	return R531OK;
}

// 8006:д��ƵоƬEEPROM  �ݲ�֧�֣�
// 8007:����ƵоƬEEPROM  �ݲ�֧�֣�
// 8008:д��ƵоƬ�Ĵ���  �ݲ�֧�֣�
// 8009:����ƵоƬ�Ĵ���  �ݲ�֧�֣� 


/******************************************************************************************
 *******	�������ƣ�R531DevCheckUserCard	-��ţ�11								*******
 *******	�������ܣ�����û��������Ƿ��п������Ƿ��ϵ�,ָ��[800A]					*******
 *******----------------------------------------------------------------------------*******
 *******	����������int *SlotStatus��0�����û�����1.�û���δ�ϵ磻2���û����ϵ�	*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			******* 
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0	;�����������:						*******
 ******************************************************************************************/
int R531DevCheckUserCardSlot(int hDev,int *SlotStatus,int *Status )
{   
		
	char szSend[1024];
	char szRecv[1024];
	char szHexRecv[1024];
	char szBitString[8+1];
	int nSlotStatus;
	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//sprintf_s(szSend,1024,"800A");
	sprintf(szSend,"800A");
	nSendLen = (int)strlen(szSend);
	
	nSlotStatus = 0;

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}


	// Ӧ�����ݴ��󣬸�ʽΪ��800A XX :����Ϊ6�ֽڵ�ASCII��
	if(nRecvLen < 6) 
	{
		return R531RESPDATALENERR;
	}
	
	// ����Ӧ��,Ӧ������
	memset(szHexRecv,0x00,sizeof(szHexRecv));
	memset(szBitString,0x00,sizeof(szBitString));

	// ��Ӧ��ASCII�ַ�����STAת����BCD��
	ftAtoh(szRecv+4,szHexRecv,1);

	ftCharToBitString((unsigned char)szHexRecv[0],szBitString);

	
	// ����STA�����ֽڵ�ֵ�����û�������״̬
	if( szBitString[0] == '0')
	{
		*SlotStatus = R531CARDSTATNO;					// ���û���
	}
	else 
	{
		if(szBitString[1] == '0')
		{
			*SlotStatus = R531CARDSTATNOPOWER;			// �û���δ�ϵ�
		}else 
		{
			*SlotStatus = R531CARDSTATNORMAL;			// �û������ϵ�
		}
	}
	
	return R531OK;
}




/******************************************************************************************
 *******	�������ƣ�R531DevRedLightCtl	-��ţ�12								*******
 *******	�������ܣ�ROCKEY531��������ƿ���,ָ��[800B]							*******
 *******			  ����˸˳��Ϊ��������;											*******
 *******			  ѭ������Ϊ0ʱ���������ʱ��Ϊ0�����������ܵ���ʱ��ֵ		*******
 *******			  �������ʱ�䲻Ϊ�㣬���һֱ����ѭ������1��FFʱ��				*******
 *******                �ư�����ʱ��ѭ����											*******
 *******----------------------------------------------------------------------------*******
 *******	����������unsigned char CycleNum��ѭ������								*******
 *******              unsigned char LightTime������ʱ��								*******
 *******              unsigned char OffTime������ʱ��								*******
 *******              int *Status��(���)ָ��Ӧ��״̬���ɹ����أ�0��				*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0	;�����������:						*******
 ******************************************************************************************/
int R531DevRedLightCtl(int hDev,unsigned char CycleNum,unsigned char LightTime,unsigned char OffTime,int *Status )
{
	char szSend[1024];
	char szRecv[1024];
	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	// ��ƿ���ָ��800B + 1�ֽ�ѭ������ + 1�ֽڵ���ʱ�� + 1�ֽڵ���ʱ��
	//sprintf_s(szSend,1024,"800B%02.2X%02.2X%02.2X",CycleNum,LightTime,OffTime);
	sprintf(szSend,"800B%02.2X%02.2X%02.2X",CycleNum,LightTime,OffTime);
	nSendLen = (int)strlen(szSend);
	

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	return R531OK;
}




/******************************************************************************************
 *******	�������ƣ�R531DevSetRFTimeOut	-��ţ�12								*******
 *******	�������ܣ�ROCKEY531��������Ƶ����ʱʱ�䶨��,ָ��[8011]					*******
 *******			  ���Զ���Ǳ���Ƶ����д������ʱʱ��							*******
 *******----------------------------------------------------------------------------*******
 *******	����������unsigned char TimeOut����ʱʱ��,����ֵ���£�					*******
 *******                 0->0.3��1->0.6��2->1.2��3->2.5��4->5��5->10��6->20��7->40	*******
 *******			     8->80�� 9->160��A->320; B->620; C->1300;D->2500;E->5000	*******
 *******			     ��ʱʱ�䵥λΪ����											*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0	;�����������:						*******
 ******************************************************************************************/
int R531DevSetRFTimeOut(int hDev,unsigned char TimeOut,int *Status )
{
	unsigned char nTimeOut;
	char szSend[1024];
	char szRecv[1024];
	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;

	// ��鴫��ĳ�ʱʱ��
	nTimeOut = TimeOut;
	if(nTimeOut > 14 )
	{
		nTimeOut = 14;
	}

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	// ��ƿ���ָ��8011 + 1�ֽڳ�ʱʱ��
 	//sprintf_s(szSend,1024,"8011%02.2X",TimeOut);
 	sprintf(szSend,"8011%02.2X",TimeOut);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	return R531OK;

}



/******************************************************************************************
 *******	�������ƣ�R531DevForceQuit	-��ţ�13									*******
 *******	�������ܣ���ֹ��ǰ������ִ�е�ָ��,ָ��[801F]							*******
 *******			  ����ǿ�ƽ�������д��������˸������������ָ��					*******
 *******----------------------------------------------------------------------------*******
 *******	����������  															*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0	;�����������:						*******
 ******************************************************************************************/
int R531DevForceQuit(int hDev,int *Status )
{
	char szSend[1024];
	char szRecv[1024];
	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//����ִ�е�ָ��
	//sprintf_s(szSend,1024,"801F");
	sprintf(szSend,"801F");
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}
	return R531OK;

}

/******************************************************************************************
 *******	�������ƣ�R531DevGenUID	-��ţ�14										*******
 *******	�������ܣ������û�UID,ָ��[800C]										*******
 *******----------------------------------------------------------------------------*******
 *******	����������int DataLen:�������ݳ��ȣ�ֵ��ΧΪ��0x00~0x30					*******
 *******			  unsigned char *Data:Len�ֽڵ���������,�������ݸ�ʽ���£�		*******
 *******					0x00 + LEN + DATA_key; ��������Ϊ��ASCII��				*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******					�ɹ����أ�00x00;ʧ�ܷ��أ�0x01;��֧�ַ��أ�0x02			*******
 *******                    UID�Ѵ��ڷ��أ�0x03										*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0	;�����������:						*******
 ******************************************************************************************/
int R531DevGenUID(int hDev,int DataLen,unsigned char *Data ,int *Status )
{
	char szSend[1024];
	char szRecv[1024];
	char szASCIIData[96+1];			// �������ݳ���Ϊ��0~0x30 ;
	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;

	// ��鳤���Ƿ�Ϸ�
	if(DataLen <=0 || DataLen > 48)
	{
		return R531PARAMERR;
	}

	// ����������
	memset(szASCIIData,0x00,sizeof(szASCIIData));

	// ��һ���ֽ�ת����2���ֽ�
	ftHtoa((char *)Data,szASCIIData,DataLen);


	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	

	//sprintf_s(szSend,1024,"800C001031323334353637383930313233343536");
	
	//����ִ�е�ָ��
	//sprintf_s(szSend,1024,"800C00%02.2X%s", DataLen, szASCIIData);
	sprintf(szSend,"800C00%02.2X%s", DataLen, szASCIIData);

	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	return R531OK;


}

/******************************************************************************************
 *******	�������ƣ�R531DevGetUID	-��ţ�15										*******
 *******	�������ܣ���ȡ�û�UID,ָ��[800C]										*******
 *******----------------------------------------------------------------------------*******
 *******	����������int* DataLen:(���)�������ݳ��ȣ�ֵ��ΧΪ��0x00~0x30			*******
 *******			  unsigned char *Data:(���)Len�ֽڵ���������					*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******					�ɹ����أ�00x00;ʧ�ܷ��أ�0x01;��֧�ַ��أ�0x02			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0	;�����������:						*******
 ******************************************************************************************/
int R531DevGetUID(int hDev,int* DataLen,unsigned char *Data ,int *Status )
{
	char szSend[1024];
	char szRecv[1024];
	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	memcpy(Data,"\x00",1);

	//����ִ�е�ָ��
	//sprintf_s(szSend,1024,"800C01");
	sprintf(szSend,"800C01");
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	
	// ����ȡ�����������Ƿ����
	if(nRecvLen <= 0)
	{
		return R531RESPDATALENERR;
	}

	// ���ú���Ӧ��ĳ��ȱ���
	*DataLen = nRecvLen;

	// ���ú��������������������
	memcpy(Data,szRecv,nRecvLen);
	memcpy(Data+nRecvLen,"\x00",1);

	return R531OK;
}



/******************************************************************************************
 *******	�������ƣ�R531DevModUID	-��ţ�16										*******
 *******	�������ܣ����������û�UID,ָ��[800C]									*******
 *******----------------------------------------------------------------------------*******
 *******	����������int DataLen:�������ݳ��ȣ�ֵ��ΧΪ��0x00~0x30					*******
 *******			  unsigned char *Data:Len�ֽڵ���������,�������ݸ�ʽ���£�		*******
 *******					0x00 + LEN + DATA_key; ��������Ϊ��ASCII��				*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******					�ɹ����أ�00x00;ʧ�ܷ��أ�0x01;��֧�ַ��أ�0x02			*******
 *******                    UID�Ѵ��ڷ��أ�0x03										*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0	;�����������:						*******
 ******************************************************************************************/
int R531DevModUID(int hDev,int DataLen,unsigned char *Data ,int *Status )
{
	char szSend[1024];
	char szRecv[1024];
	char szASCIIData[96+1];			// �������ݳ���Ϊ��0~0x30 ;
	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;



	// ��鳤���Ƿ�Ϸ�
	if(DataLen <=0 || DataLen > 48)
	{
		return R531PARAMERR;
	}

	// ����������
	memset(szASCIIData,0x00,sizeof(szASCIIData));

	// ��һ���ֽ�ת����2���ֽ�
	ftHtoa((char *)Data,szASCIIData,DataLen);


	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//����ִ�е�ָ��
	//sprintf_s(szSend,1024,"800C02%02.2X%s", DataLen, szASCIIData);
	sprintf(szSend,"800C02%02.2X%s", DataLen, szASCIIData);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	return R531OK;
}




/******************************************************************************************
 *******	�������ƣ�R531DevModPara	-��ţ�17									*******
 *******	�������ܣ��޸Ķ���������,ָ��[8010]										*******
 *******----------------------------------------------------------------------------*******
 *******	����������int ParaLen:�����ֽڳ���,	10�ֽ�=2*10							*******
 *******			  unsigned char *Data:�����ֽڣ�˵�����£�(û������ռ�����ַ�)	*******
 *******		   ���	������ʶ		��������			ֵʵ��					*******
 *******           ===============================================================	*******
 *******           1	ATR_Band_User	�û�����ATR������   0x00��ʶ������7816����	******* 
 *******		   2    ATR_Band_Sam1   SAM1����			(��ͨ�����óɣ�13)		*******
 *******		   3    ATR_Band_Sam2												*******
 *******		   4	ATR_Band_Sam3												*******
 *******		   5    WKT								     (��ͨ�����óɣ�0x10)	*******
 *******		   6	RFU									 (����ֵΪ��00)			*******
 *******		   7	RFU															*******
 *******		   8	RFU															*******
 *******		   9	RFU															*******
 *******		   10	RFU															*******		
 *******		   ---------------------------------------------------------------	*******
 *******			������óɣ�0x00,��ʾ��������7816���У����Ϊ����ֵ����ʼ�հ�	*******
 *******				ֵ����ĺ㶨�Ĳ�����ͨѶ������ATR��APDU���̣�				*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0	;�����������:						*******
 ******************************************************************************************/
// ��ͨ���Ǳ꿨����������������ָ��ʵ����8010 13 11 11 11 01 00 00 00 00 00  [10�ֽڣ�20��ASCII��],�û����۷Ǳ꣬������׼
// ����˵����ATR_Baud_user �������óɣ�0x13; SKT ���óɣ�0x10				
int R531DevModPara(int hDev,int ParaLen, char *Para,int *Status )
{
	char szSend[1024];
	char szRecv[1024];
	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;

	// ��鴫��ĳ��Ȳ���ֵ�Ƿ���ȷ
	if( (ParaLen != 20) )
	{
		return R531PARAMERR;
	}

	if( (int)strlen((char *)Para) != ParaLen )
	{
		return R531PARAMERR;
	}

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//����ִ�е�ָ��
	//sprintf_s(szSend,1024,"8010%s",Para);
	sprintf(szSend,"8010%s",Para);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	return R531OK;
}



/******************************************************************************************
 *******	�������ƣ�R531DevRecoverPara	-��ţ�18								*******
 *******	�������ܣ��ָ�����������,ָ��[8015]										*******
 *******----------------------------------------------------------------------------*******
 *******	����������                                  							******* 
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0	;�����������:						*******
 ******************************************************************************************/
int R531DevRecoverPara(int hDev,int *Status )
{
	char szSend[1024];
	char szRecv[1024];
	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//����ִ�е�ָ��
	//sprintf_s(szSend,1024,"8015");
	sprintf(szSend,"8015");
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK )
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	return R531OK;
}

//==================================================================================================
//3-�Ӵ����ຯ����װ
//==================================================================================================
/******************************************************************************************
 *******	�������ƣ�R531CpuSetSlot	-��ţ�19									*******
 *******	�������ܣ�ѡ�񿨲�,ָ��[B001]											*******
 *******----------------------------------------------------------------------------*******
 *******	����������int Slot:���۱�ţ�0:�û����ۣ�1~3��SAM����					******* 
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0	;�����������:						*******
 ******************************************************************************************/
int R531CpuSetSlot(int hDev,int Slot,int *Status )
{
	char szSend[1024];
	char szRecv[1024];
	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;
	int nDevNo;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));


	nDevNo = hDev;

	// ���豸���ת�����ڲ������������ţ�
	nDevNo -= 1;

	// ������Ϸ���
	if( nDevNo < 0 || nDevNo > R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDevNo >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鿨�۲�����ȷ��
	if(Slot <0 || Slot >3)
	{
		return R531SLOTERR;
	}

	//����ִ�е�ָ��
	//sprintf_s(szSend,1024,"B001%02.2X",Slot);
	sprintf(szSend,"B001%02.2X",Slot);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;

		return nRet;
	}

	// Add-20120706 -- ���ö������ṹ�е�ǰ�������Ŀ���ѡ��״̬������ǰѡ��Ŀ��۱��
	stDeviceStruct[nDevNo].nCurrSlot = Slot;

	return R531OK;
}


/******************************************************************************************
 *******	�������ƣ�R531CpuReset	-��ţ�20										*******
 *******	�������ܣ�IC���ϵ縴λ,ָ��[B002]										*******
 *******----------------------------------------------------------------------------*******
 *******	����������int *AtrLen:(���)��λӦ����Ϣ����[ASCII]						*******
 *******			  char *Atr�� (���)��λӦ����Ϣ[ASCII]							*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0	;�����������:						*******
 ******************************************************************************************/
int R531CpuReset(int hDev,int *AtrLen,char *Atr,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];
	char szATRLen[4+1];
	char szHexAtrLen[2+1];
	char szHexAtr[256];


	int nATRLen;
	int nHexAtrLen;
	int nCurrSlot;
	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nRet;
	int nVoltage = 0;		// ��ѹֱ�����óɣ�0 - ��5V

	ATRStruct stAtr;		// ATR�ṹ����
	int   nTD1Protocol;		// TD1�����ͨ��Э�飬�Ϸ�ֵΪ��0��1��
	int   nTD2Protocol;		// TD2�����ͨ��Э�飬�Ϸ�ֵΪ��15 

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));
	memset(szATRLen,0x00,sizeof(szATRLen));
	memset(szHexAtrLen,0x00,sizeof(szHexAtrLen));

	// ���л������

	nDev = hDev -1;
	
	// ������Ϸ���
	if( nDev < 0 || nDev >= MAXDEVICENUM)
	{
		// ����Ķ������������
		return R531PARAMERR;
	}

	
	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}


	// ����Ƿ������ýӴ�ʽIC��ָ��͵Ŀ��ۣ�
	if( stDeviceStruct[nDev].nCurrSlot == R531SLOTNOTSET)
	{
		// δѡ��Ӵ����Ŀ���
		return R531NOSETSLOT;
	}

	nCurrSlot = stDeviceStruct[nDev].nCurrSlot;

	//����ִ�е�ָ��
	//sprintf_s(szSend,1024,"B0028%01.1X",nVoltage);
	sprintf(szSend,"B0028%01.1X",nVoltage);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	// ����Ӧ������
	// ��ȡ�ϵ縴λ��Ϣ��2�ֽڳ���,��ת��������s
	memcpy(szATRLen,szRecv,4);
	ftAtoh(szATRLen,szHexAtrLen,2);

	// ������ת����10���Ƶ�����ֵ
	nATRLen = (unsigned int)szHexAtrLen[0] * 256 + (unsigned int) szHexAtrLen[1];

	
	// ���ú������������ATR����
	*AtrLen = nATRLen * 2;

	// ���ú������������ATRֵ
	memcpy(Atr,szRecv+4,nATRLen*2);
	memcpy(Atr+(nATRLen*2),"\x00",1);

	// Add-2012-07-06,���ݸ�λӦ����Ϣ,�豸�������ṹ�е�ǰ���������ϵ縴λ��־��IC��ͨ��Э������
	nHexAtrLen = nATRLen;


	memset(szHexAtr,0x00,sizeof(szHexAtr));

	// ��ASCII���ATRת����BCD��
	ftAtoh((char *)szRecv+4,szHexAtr,nHexAtrLen);

	// ��ʼ��ATR�ṹ����
	memset((char *)&stAtr,0x00,sizeof(ATRStruct));

	// ��BCD���ATRת����ATR�ṹ��
	nRet = ftATRInitFromArray(&stAtr,(unsigned char *)szHexAtr,(unsigned int)nHexAtrLen);
	if( nRet != R531OK)
	{		
		return R531RESETERR;
	}	
 		
	ftGetATRT(stAtr,&nTD1Protocol,&nTD2Protocol);
 
	// �ϵ縴λ�ɹ����޸Ķ��������ݽṹ�����е�ǰ������,���ϵ縴λ��־ֵ��ͨ��Э��
	switch(nCurrSlot)
	{
		// �û���ģ��,
		case R531SLOTUSER:
			stDeviceStruct[nDev].nUserFlag = RESETSTATON;
			stDeviceStruct[nDev].nUserProtocol = nTD1Protocol;
			break;


		// SIM1ģ��
		case R531SLOTSAM1:
			stDeviceStruct[nDev].nSIM1Flag	   = RESETSTATON;
			stDeviceStruct[nDev].nSIM1Protocol = nTD1Protocol;
			break;

		// SIM2ģ��
		case R531SLOTSAM2:
			stDeviceStruct[nDev].nSIM2Flag     = RESETSTATON;
			stDeviceStruct[nDev].nSIM2Protocol = nTD1Protocol;
			break;

		// SIM3ģ��
		case R531SLOTSAM3:
			stDeviceStruct[nDev].nSIM3Flag     = RESETSTATON;
			stDeviceStruct[nDev].nSIM3Protocol = nTD1Protocol;
			break;
	}
	

	// IC��ͨ��Э��Ϊ��T=0 ֱ�ӷ���
	if( nTD1Protocol == R531PROTOCOLT0)
	{
		return R531OK;	
	}

	// ���TD1��ͨ��Э��Ϊ��T=1,�����PPS����,--
	
	char          szPPS[21];
	unsigned char szHexPPS[21];
	char          szPCK[3];
	char          szResponse[1024];
	unsigned int  nPCK;

	memset(szPPS,0x00,sizeof(szPPS));
	memset(szHexPPS,0x00,sizeof(szHexPPS));
	memset(szPCK,0x00,sizeof(szPCK));
	memset(szResponse,0x00,sizeof(szResponse));

	//sprintf_s(szPPS,21,"FF1%1.1d%02.2X",nTD1Protocol,stAtr.ib[0][0].value);
	sprintf(szPPS,"FF1%1.1d%02.2X",nTD1Protocol,stAtr.ib[0][0].value);

	// ��ASCII��ļ�������ת��BCD��ļ�������
	ftAtoh(szPPS,(char *)szHexPPS,3);

	ftCalPCK(szHexPPS,3,&nPCK);

	//sprintf_s(szPCK,3,"%02.2X",nPCK);
	sprintf(szPCK,"%02.2X",nPCK);


	// ���ã�ftCalPCK(szPPS,3,&nPCK);
		
	// �������PCK��ӵ��ַ�����PPS��
	//strcat_s(szPPS,21,szPCK);
	strcat(szPPS,szPCK);

	// ����PPSָ�������
	nRet = R531CpuPPS(hDev,4,szPPS,szResponse,&nStatus);
	if( nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	gCommFlag2 = 0;
	return R531OK;
}


/******************************************************************************************
 *******	�������ƣ�R531CpuPowerOff	-��ţ�21									*******
 *******	�������ܣ�IC���µ�,ָ��[B002]											*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��													*******
 ******************************************************************************************/
int R531CpuPowerOff(int hDev,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];
	
	int nCurrSlot;
	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	nDev = hDev -1;
	
	// ������Ϸ���
	if( nDev < 0 || nDev >= MAXDEVICENUM)
	{
		// ����Ķ������������
		return R531PARAMERR;
	}

	
	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}


	// ����Ƿ������ýӴ�ʽIC��ָ��͵Ŀ��ۣ�
	if( stDeviceStruct[nDev].nCurrSlot == R531SLOTNOTSET)
	{
		// δѡ��Ӵ����Ŀ���
		return R531NOSETSLOT;
	}

	nCurrSlot = stDeviceStruct[nDev].nCurrSlot;


	//����ִ�е�ָ��
	//strncpy_s(szSend,1024,"B00200",6);
	strncpy(szSend,"B00200",6);

	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}


	// �޸Ķ������ṹ�����е�ǰģ����ϵ縴λ��־ֵ,Ϊ�����µ�
	switch(nCurrSlot)
	{
		// �û���ģ��,
		case R531SLOTUSER:
			stDeviceStruct[nDev].nUserFlag     = RESETSTATOFF;
			break;

		// SIM1ģ��
		case R531SLOTSAM1:
			stDeviceStruct[nDev].nSIM1Flag	   = RESETSTATOFF;
				break;

		// SIM2ģ��
		case R531SLOTSAM2:
			stDeviceStruct[nDev].nSIM2Flag     = RESETSTATOFF;
			break;

		// SIM3ģ��
		case R531SLOTSAM3:
			stDeviceStruct[nDev].nSIM3Flag     = RESETSTATOFF;
			break;
	}

	return R531OK;
}



/******************************************************************************************
 *******	�������ƣ�R531CpuPPS	-��ţ�22										*******
 *******	�������ܣ�PPS,ָ��[B010]												*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******			  int PPSLen	��PPS����										*******
 *******			  char *PPS		��PPS���ݣ��磺ff 11 00 ee(API�̶�Ϊ4�ֽ�)		*******
 *******								[0]���̶�ֵΪ:FF							*******
 *******								[1]���߰��ֽڹ̶�Ϊ��1���Ͱ��ֽ�ΪЭ������	*******
 *******								[2]��ATR��TA1��ֵ							*******
 *******								[3]��0~2 �ֽڵ���żУ��ֵ					*******
 *******			  int *Status	��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��		*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��													*******
 ******************************************************************************************/
int R531CpuPPS(int hDev,int PPSLen,char *PPS,char *Recv,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];

	int nDevNo;
	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;
	

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	// Add - 20120709 -- ���Ӳ��������л������
	nDevNo = hDev -1;


	// ������Ϸ���
	if( nDevNo < 0 || nDevNo > R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDevNo >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDevNo].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}


	// ����Ƿ������ýӴ�ʽIC��ָ��͵Ŀ��ۣ�
	if( stDeviceStruct[nDevNo].nCurrSlot == R531SLOTNOTSET)
	{
		// δѡ��Ӵ����Ŀ���
		return R531NOSETSLOT;
	}


	//sprintf_s(szSend,1024,"B010%02.2X%s",PPSLen,PPS);
	sprintf(szSend,"B010%02.2X%s",PPSLen,PPS);
	nSendLen = (int)strlen(szSend);

	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
	// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	
	// Add- 20120709 ����Ӧ������
	memcpy(Recv,szRecv,nRecvLen);


	return R531OK;
}



/******************************************************************************************
 *******	�������ƣ�R531CpuAPDU	-��ţ�23										*******
 *******	�������ܣ�ִ��IC����APDU,ָ��[B011]										*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******              int hDev		����������ţ��豸��ţ�1~9 [��1��Ϊ1]			*******
 *******			  int SendLen	�����͵�IC��ָ���(ASCII����)					*******
 *******			  char *Send	�����͵�IC��ָ��(ASCII��)						*******
 *******			  int *RecvLen	��(���)Ӧ�����ݳ���(ASCII����)					*******
 *******			  char *Recv	��(���)Ӧ������(ASCII��)						*******
 *******			  int *Status	��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��		*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��													*******
 ******************************************************************************************/
int R531CpuAPDU(int hDev,int SendLen,char *Send,int *RecvLen,char *Recv,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];
	char szTmp[1024];						// T=1 ��ʱ���ݼ���
	char szHexLRC[2];						// T=1 LRCֵ
//	char szLRC[2+1];						// ASCII���LRCֵ


	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nRet;
	int nProtocolType;						// ����IC��Э������
	int nHLen,nLLen;						// ָ��ȸߡ����ֽ�ֵ
	int nPHLen,nPLLen;						// 
	int nTmpLen;
	int nSlotNo;
	int nResetFlag;							// ������ģ���ϵ縴λ��־

	nTmpLen = 0;
	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));



	// Add - 20120710 �������л������
	
	// �����������ת�����ڲ���������Ϣ�ṹ�������(��0��ʼ)
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}

	// ����Ƿ������ýӴ�ʽIC��ָ��͵Ŀ��ۣ�
	if( stDeviceStruct[nDev].nCurrSlot == R531SLOTNOTSET)
	{
		// δѡ��Ӵ����Ŀ���
		return R531NOSETSLOT;
	}

	// END Add 
	
	// ��鴫���IC��ָ��ĺϷ���
	if( SendLen % 2 != 0)
	{
		return R531PARAMERR;
	}

	if( SendLen != (int)strlen(Send) )
	{
		return R531PARAMERR;
	}

	// ����ָ��ȳ���
	nHLen =  (SendLen / 2) /256 ;			// ���ֽ�
	nLLen = (SendLen  / 2) %256 ;			// ���ֽ�

	
	// ��ȡ��������ǰ������ģ�飻
	nSlotNo = stDeviceStruct[hDev-1].nCurrSlot;


	nResetFlag =  RESETSTATNOT;

	// ��ȡģ��IC��ͨ��Э������
	switch(nSlotNo)
	{
	case 0:
		nProtocolType = stDeviceStruct[hDev-1].nUserProtocol;
		nResetFlag    = stDeviceStruct[hDev-1].nUserFlag;
		break;

	case 1:
		nProtocolType = stDeviceStruct[hDev-1].nSIM1Protocol;
		nResetFlag    = stDeviceStruct[hDev-1].nSIM1Flag;
		break;

	case 2:
		nProtocolType = stDeviceStruct[hDev-1].nSIM2Protocol;
		nResetFlag    = stDeviceStruct[hDev-1].nSIM2Flag;
		break;

	case 3:
		nProtocolType = stDeviceStruct[hDev-1].nSIM3Protocol;
		nResetFlag    = stDeviceStruct[hDev-1].nSIM3Flag;
		break;
	}

	// Add -20120710 ,����������ǰģ��IC�Ƿ��ϵ縴λ
	if( nResetFlag != RESETSTATON)
	{
		return R531RESETNOT;
	}
	

	// ���Э�����͵ĺϷ���,��0��1���óɹ�
	if( nProtocolType != R531PROTOCOLT1 )
	{
		nProtocolType = R531PROTOCOLT0;
	}

	// ����IC��ͨѶЭ��Ϊ��T = 0 �Ĵ���
	if( nProtocolType == 0)
	{	// T=0,ָ��ʵ����b0 11 00  05 00 00 00  84 00 00 04
		// T=0,ָ���ʽ��B011 + 2�ֽ�ָ��� + 2�ֽ�Ԥ�ڷ��س��� + APDU 
		//sprintf_s(szSend,1024,"B011%02.2X%02.2X0000%s", (unsigned int)nHLen, (unsigned int)nLLen, Send);
		sprintf(szSend,"B011%02.2X%02.2X0000%s", (unsigned int)nHLen, (unsigned int)nLLen, Send);

		nSendLen = (int)strlen(szSend);
	}else		// T = 1 ָ��������֯
	{
		memset(szRecv,0x00,sizeof(szRecv));


		// B0 11 + 2�ֽ� 
		memset(szTmp,0x00,sizeof(szTmp));
		
		nPHLen = nHLen;
		nPLLen = nLLen;
		
		nPLLen += 4;			// T=1 ��4�ֽڵĿ�ʼ�ֶκͽ����ֶ�

		if((nPLLen / 256) > 0)
		{
			nPHLen += 1;
			nPLLen = nPLLen % 256;
		}
		
		// UDPATE:20131118 -- ��� �Ӵ�ģ�� T=1 �Ŀ���ָ��ִ������
		if(gCommFlag2 == 0)
		{
			// ָ�����ݲ��֣�2�ֽ�ָ��ȡ�2�ֽ�Ԥ�ڷ������ݳ������óɣ�0000��3�ֽ�APDU����
			//sprintf_s(szTmp,1024,"%02.2X%02.2X000000%02.2X%02.2X%s",
			sprintf(szTmp,"%02.2X%02.2X000000%02.2X%02.2X%s",
				nPHLen,					// ���뿪ʼ�ֶκͽ����ֶε�APDU���ȣ����ֽ�
				nPLLen,					// ���뿪ʼ�ֶκͽ����ֶε�APDU���ȣ����ֽ�
				nHLen,					// ָ��ȸ��ֽ�
				nLLen,					// ָ��ȵ��ֽ�
				Send);

			nTmpLen = (int)strlen(szTmp);

			gCommFlag2 += 1;

		}else 
		{
			// ָ�����ݲ��֣�2�ֽ�ָ��ȡ�2�ֽ�Ԥ�ڷ������ݳ������óɣ�0000��3�ֽ�APDU����
			//sprintf_s(szTmp,1024,"%02.2X%02.2X000000%02.2X%02.2X%s",
			sprintf(szTmp,"%02.2X%02.2X000000%02.2X%02.2X%s",
				nPHLen,					// ���뿪ʼ�ֶκͽ����ֶε�APDU���ȣ����ֽ�
				nPLLen,					// ���뿪ʼ�ֶκͽ����ֶε�APDU���ȣ����ֽ�
				nHLen+0x40,				// ָ��ȸ��ֽ�
				nLLen,					// ָ��ȵ��ֽ�
				Send);

			nTmpLen = (int)strlen(szTmp);

			gCommFlag2 = 0;
		}

		// �������ݵ�LRCֵ
		memset(szHexLRC,0x00,sizeof(szHexLRC));
		ftCalLRC1(szTmp+8,nTmpLen-8,szHexLRC);

		memset(szSend,0x00,sizeof(szSend));

		//sprintf_s(szSend,1024,"B011%s%02.2X",szTmp,(unsigned char)szHexLRC[0]);
		sprintf(szSend,"B011%s%02.2X",szTmp,(unsigned char)szHexLRC[0]);
		nSendLen = (int)strlen(szSend);
	}	

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}


	char szLen[4+1];
	memset(szLen,0x00,sizeof(szLen));

	if( nProtocolType == R531PROTOCOLT0)
	{	// Ӧ�����ݣ�00 06  e8 ed 68 65  90 00
		*RecvLen = nRecvLen -4;
		memcpy(Recv,szRecv+4,(nRecvLen-4));
		memcpy(Recv+(nRecvLen-4),"\x00",1);
	}else 
	{

		memcpy(szLen,szRecv,4);					// ��ȡ�����ַ���
		nSendLen = (int)ftHexToLong(szLen);		// ��BCD���ַ����ĳ���ת����int
		
		if( nRecvLen != nSendLen +2 )
		{
			nSendLen = nSendLen *2 - 8;
			*RecvLen  =nSendLen ;
			memcpy(Recv,szRecv+10,nSendLen);
			memcpy(Recv+nSendLen,"\x00",1);
		}
		else 
		{
			*RecvLen = nRecvLen;
			memcpy(Recv,szRecv,nRecvLen);
			memcpy(Recv+nRecvLen,"\x00",1);
		}
	}

	return R531OK;
}

int R531CpuAPDU(int hDev,char *Send,char *Recv,int *Status )
{
	int nRet ;
	int nSndLen;
	int nRcvLen;
	char szSend[1024];

	nSndLen = (int)strlen(Send);

	nRet = R531CpuAPDU(hDev,nSndLen,Send,&nRcvLen,Recv,Status);
	if( nRet == 0)
	{
		// ���Ӧ���SW�Ƿ�Ϊ��61XX ,��ִ�� 00C00000xx ��ȡӦ������
		if( strlen(Recv) == 4 && memcmp(Recv,"61",2)== 0)
		{
			memset(szSend,0x00,sizeof(szSend));
			memcpy(szSend,"00C00000",8);
			memcpy(szSend+8,Recv+2,2);

			nSndLen = (int)strlen(szSend);

			nRet = R531CpuAPDU(hDev,nSndLen,szSend,&nRcvLen,Recv,Status);
		}
	}

	return nRet;
}



//==================================================================================================
//4-�ǽ�TypeA�๤��ģʽ������װ
//==================================================================================================

/******************************************************************************************
 *******	�������ƣ�R531TypeARequest	-��ţ�24									*******
 *******	�������ܣ��ǽ�IC��Ѱ��,ָ��[C100]										*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******			  unsigned char ReqMode��Ѱ����ʽ������ֵ���£�					*******
 *******							0x26��������Ƶ���ڴ���IDLE״̬�Ŀ�				*******
 *******							0x52��������Ƶ���ڴ���IDLE��HALT״̬�����п�	*******
 *******			  char *Atqa��(���)���ɹ�����ATQA��Ӧ�����˵�����£�			*******
 *******							[0]�������ͣ� 02 - S70��04 - M1��08 - MPRO		*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ָ��ִ��ʧ�ܷ��أ�-1��ִ��ִ�д��󷵻أ�-2		*******
 ******************************************************************************************/
int R531TypeARequest(int hDev,unsigned char ReqMode,char *Atqa,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	
	// ���Ӵ�������Ϸ��Լ��

	// ������Ķ����������ת�����ڲ��������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}



	// ��鴫��Ѱ����ʽ����:ReqMode�ĺϷ���
	if(!( ReqMode == 0x26 || ReqMode == 0x52 ))
	{
		return -1;
	}

	//����ִ�е�ָ��
	//sprintf_s(szSend,1024,"C100%02.2X",ReqMode);
	sprintf(szSend,"C100%02.2X",ReqMode);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��(Ӧ������ΪASCII��)
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	// ����Ӧ�����������:ATQA �ĵ�1�ֽڵ�ASCII�ַ�����"04" -- M1  ;  "08" -- MPRO ;  "02" -- S70��
	memcpy(Atqa,szRecv,nRecvLen);

	// Add -2012-07-23 ����Ӧ���ATQA ���õ�ǰ�������ǽ�IC���Ŀ�����
	stDeviceStruct[nDev].nCardType = szRecv[1] -'0';	// 4-M1-S50; 2 - M1-S70;	8 - MPRO


	gCommFlag = 0;
	return 0;
}


/******************************************************************************************
 *******	�������ƣ�R531TypeAAntiCollision	-��ţ�25							*******
 *******	�������ܣ�����ͻ,ָ��[C101]												*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******			  int  CLVL����������ֵ��ΧΪ:1~3	;ͨ�����óɣ�1				*******
 *******			  char *UID��(���)���ɹ�����UID[4�ֽ�]�磺"FFFFFFFF"			*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0,���������£�						*******
 ******************************************************************************************/
int R531TypeAAntiCollision(int hDev,int CLVL,char *UID,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));


	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}
 
	// END Add 
	

	// �������Ĵ�����������Ƿ�Ϸ�
	if( CLVL <=0 || CLVL > 3)
	{
		return R531PARAMERR;
	}

	//����ִ�е�ָ��
	//sprintf_s(szSend,1024,"C101%02.2d",CLVL);
	sprintf(szSend,"C101%02.2d",CLVL);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}


	// ����Ӧ�����,ASCII���UID,ͨ��Ϊ8��ASCII�ַ�
	memcpy(UID,szRecv,nRecvLen);

	return R531OK;
}




/******************************************************************************************
 *******	�������ƣ�R531TypeASelect	-��ţ�26									*******
 *******	�������ܣ�ѡ��,ָ��[C102]												*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******			  int  CLVL����������ֵ��ΧΪ:1~3	;ͨ�����óɣ�1				*******
 *******			  char *UID��UID�����÷���ͻ�����ķ���ֵ						*******
 *******			  char *SAK��Ӧ�����ݣ�1�ֽ�									*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0,���������£�						*******
 *******							-1�������������CLVL����						*******
 *******							-2��UID����Ϊ4�ֽ�								*******
 *******							-3��ָ��ִ��ʧ��								*******
 *******							-4��ָ��ִ�д���								*******
 ******************************************************************************************/
int R531TypeASelect(int hDev,int CLVL,char *UID,char *SAK,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));


	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}
 
	// END Add 


	// �������Ĵ�����������Ƿ�Ϸ�
	if( CLVL <=0 || CLVL > 3)
	{
		return R531PARAMERR;
	}

	// UID����Ϊ8��4�ֽ�BCD��ֵ
	if(strlen(UID) != 8)
	{
		return R531PARAMERR;
	}


	//����ִ�е�ָ��
	//sprintf_s(szSend,1024,"C102%02.2d%s",CLVL,UID);
	sprintf(szSend,"C102%02.2d%s",CLVL,UID);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

 	// ����Ӧ�����,SAK��ASCII��ֵ����2��ASCII��ֵ
	memcpy(SAK,szRecv,nRecvLen);

	return R531OK;
}

/******************************************************************************************
 *******	�������ƣ�R531TypeAHalt		-��ţ�27									*******
 *******	�������ܣ��ж�,ָ��[C103]												*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
  *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0,���������£�						*******
 *******							-1��ָ��ִ��ʧ��								*******
 *******							-2��ָ��ִ�д���								*******
 ******************************************************************************************/
int R531TypeAHalt(int hDev,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}
 
	// END Add 

	//����ִ�е�ָ��
	//strncpy_s(szSend,1024,"C103",4);
	strncpy(szSend,"C103",4);
	nSendLen =(int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	return R531OK;
}



/******************************************************************************************
 *******	�������ƣ�R531TypeARats		-��ţ�28									*******
 *******	�������ܣ�ѡ��Ӧ��,ָ��[C104]											*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******			  int CID����Ƭʶ�������Ѱַ��Ƭ���߼��ţ�ֵ��Χ 0~E;ͨ��Ϊ:0	*******
 *******			  char *ATS��(���)ATS����										*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0,���������£�						*******
 ******************************************************************************************/
int R531TypeARats(int hDev,int CID,char *ATS,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}
 
	// END Add 

	if( CID < 0 || CID > 14)
	{
		return R531PARAMERR;
	}

	//����ִ�е�ָ��
	//sprintf_s(szSend,1024,"C104%02.2X",CID);
	sprintf(szSend,"C104%02.2X",CID);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}


	// ���ú���Ӧ�����ݵ�ATSֵ(ASCIIֵ)
	memcpy(ATS,szRecv,nRecvLen);


	return R531OK;
}


//==================================================================================================
//5-TypeB�����������װ
//==================================================================================================
/******************************************************************************************
 *******	�������ƣ�R531TypeBRequest	-��ţ�29									*******
 *******	�������ܣ��ǽ�IC��Ѱ��,ָ��[C000]										*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******			  unsigned char ReqMode��Ѱ����ʽ������ֵ���£�					*******
 *******							0x00��������Ƶ���ڴ���IDLE״̬�Ŀ�				*******
 *******							0x08��������Ƶ���ڴ���IDLE��HALT״̬�����п�	*******
 *******			  unsigned char AFI����Ӧ�ñ�ʶ									*******
 *******			  unsigned char TimeN��ʱ϶��,���ڷ���ͻ�����趨���Ƭ������	*******
 *******							0x00��1�ſ���0x01��2�ſ���0x02��4�ſ�			*******
 *******                            0x03��8�ſ���0x04��16�ſ�������ֵ��Ч			*******
 *******			  char *Atqb��(���)���ɹ�����ATQB��12�ֽ�Ӧ�����ݣ�ֵ���£�	*******
 *******							Byte1:50,�̶�ֵ									*******
 *******							Byte2~Byte5��4�ֽ�PUPI							*******
 *******							Byte6~Byte9��Ӧ����Ϣ�ֽڣ�						*******
 *******							Byte10~Byte12��Э����Ϣ�ֽ�						*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ָ��ִ��ʧ�ܷ��أ�<0;���������£�				*******
 *******								-1��Ѱ����ʽ����ReqMode����					*******
 *******								-2��ʱ϶������								*******
 *******								-3��ָ��ִ��ʧ��							*******
 *******								-4��ָ��ִ�д���							*******
 ******************************************************************************************/
int R531TypeBRequest(int hDev,unsigned char ReqMode,unsigned char AFI,unsigned char TimeN,char *Atqb ,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}
 
	// END Add 


	// ���Ѱ����ʽ�����ĺϷ���
	if(!( ReqMode == R531REQBIDLE || ReqMode  == R531REQBALL) )
	{
		return R531PARAMERR;
	}

	if(!( TimeN == 0 || TimeN == 1 || TimeN == 2 || TimeN == 3 || TimeN == 4 ))
	{
		return R531PARAMERR;
	}
	//����ִ�е�ָ��
	//sprintf_s(szSend,1024,"C100%02.2X%02.2X%02.2X",ReqMode,AFI,TimeN);
	sprintf(szSend,"C100%02.2X%02.2X%02.2X",ReqMode,AFI,TimeN);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}


	// ����Ӧ�����
	memcpy(Atqb,szRecv,nRecvLen);

	return R531OK;
}



/******************************************************************************************
 *******	�������ƣ�R531TypeBSlotMarker	-��ţ�30								*******
 *******	�������ܣ�TypeB��Ƭ����ͻ,ָ��[C001]									*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******			  int  SlotNum��ʱ϶��,��1��15,������ͻ����ʱ,���Բ���˳�����	*******
 *******			  char *ATQB��(���)�� 											*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0,���������£�						*******
 *******							-1��ʱ϶������									*******
 *******							-2��ָ��ִ��ʧ��								*******
 *******							-3��ָ��ִ�д���								*******
 ******************************************************************************************/
int R531TypeBSlotMarker(int hDev,int SlotNum,char *ATQB,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}
 
	// END Add 

	// ��������ʱ϶���Ƿ�Ϸ�
	if( SlotNum  < 1 || SlotNum > 15)
	{
		return R531PARAMERR;
	}

	//����ִ�е�ָ��
	//sprintf_s(szSend,1024,"C001%02.2X",SlotNum);
	sprintf(szSend,"C001%02.2X",SlotNum);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	// ����Ӧ�����ֵ
	memcpy(ATQB,szRecv,nRecvLen);

	return R531OK;
}


/******************************************************************************************
 *******	�������ƣ�R531TypeBAttrib	-��ţ�31									*******
 *******	�������ܣ�TypeB��Ƭѡ��,ָ��[C002]										*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******			  int  SlotNum��ʱ϶��,��1��15,������ͻ����ʱ,���Բ���˳�����	*******
 *******			  char *ATQB��(���)�� 											*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0,���������£�						*******
 *******							-1��ʱ϶������									*******
 *******							-2��ָ��ִ��ʧ��								*******
 *******							-3��ָ��ִ�д���								*******
 ******************************************************************************************/
int R531TypeBAttrib(int hDev,int SlotNum,char *ATQB,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}
 
	// END Add 


	// ��������ʱ϶���Ƿ�Ϸ�
	if( SlotNum  < 1 || SlotNum > 15)
	{
		return R531PARAMERR;
	}

	//����ִ�е�ָ��
	//sprintf_s(szSend,1024,"C001%02.2X",SlotNum);
	sprintf(szSend,"C001%02.2X",SlotNum);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}


	// ����Ӧ�����
	memcpy(ATQB,szRecv,nRecvLen);

	return R531OK;
}

/******************************************************************************************
 *******	�������ƣ�R531TypeBHalt	-��ţ�32										*******
 *******	�������ܣ�TypeB��Ƭѡ��,ָ��[C003]										*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
  *******			  char *PUPI�� 	4�ֽ�											*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�R531OK��ʧ�ܷ��أ��������							*******
 ******************************************************************************************/
int R531TypeBHalt(int hDev,char *PUPI,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));


	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}
 
	// END Add 

	// ��������ʱ϶���Ƿ�Ϸ�
	if( strlen(PUPI) != 8)
	{
		return -1;
	}

	//����ִ�е�ָ��
	//sprintf_s(szSend,1024,"C003%s",PUPI);
	sprintf(szSend,"C003%s",PUPI);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}


	return R531OK;
}



//==================================================================================================
//6-14443��4���ִ����������װ
//==================================================================================================

/******************************************************************************************
 *******	�������ƣ�R531TypeDeselect	-��ţ�33									*******
 *******	�������ܣ�ȡ����Ƭѡ��,ָ��[C200]										*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
  *******			  int CID����Ƭʶ�������Ѱַ��Ƭ���߼��ţ�ֵ��Χ 0~E,ͨ��Ϊ:0	*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0,���������£�						*******
 *******							-1��PUPI��������								*******
 *******							-2��ָ��ִ��ʧ��								*******
 *******							-3��ָ��ִ�д���								*******
 ******************************************************************************************/
int R531TypeDeselect(int hDev,int CID,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}
 
	// END Add 

	// ��������CID�Ƿ�Ϸ�
	if( CID < 0 || CID > 14 )
	{
		return R531PARAMERR;
	}

	//����ִ�е�ָ��
	//sprintf_s(szSend,1024,"C200%02.2X",CID);
	sprintf(szSend,"C200%02.2X",CID);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
 	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	return R531OK;
}

/******************************************************************************************
 *******	�������ƣ�R531TypeAPDU			-��ţ�34								*******
 *******	�������ܣ�ִ�зǽ�IC��ָ��,ָ��[C201]									*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******			  int SendLen������ָ���[ASCII�볤��]						*******
 *******			  char *Send������ָ��[ָ��ASCII�ַ���]							*******
 *******			  int *RecvLen��(���)Ӧ�����ݳ���[ASCII�볤��]					*******
 *******			  char *Recv��(���)Ӧ������[ASCII���ʽ]						*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******              int CardType = 04:�����ͣ�04-M1��08-MPRO;02-S70,ȱʡΪ��04	*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0,���������£�						*******
 ******************************************************************************************/
int R531TypeAPDU(int hDev,int SendLen,char *Send,int *RecvLen,char *Recv,int *Status ,int CardType )
{
	char szSend[1024];				
	char szRecv[1024];

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nRet;
	int i;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}
 
	// END Add 

	// ��鴫���IC��ָ��ĺϷ���
	if( SendLen % 2 != 0)
	{
		return R531PARAMERR;
	}

	if( SendLen != (int)strlen(Send) )
	{
		return R531PARAMERR;
	}

	// ���ݺ����Ŀ����Ͳ���������ִ�е�APDUָ����
	if( CardType == 4)					// ������Ϊ��M1
	{
		if( gCommFlag == 0)
		{
			//sprintf_s(szSend,1024,"C2010E0000%s",Send);
			sprintf(szSend,"C2010E0000%s",Send);
			gCommFlag += 1;
		}else
		{
			//sprintf_s(szSend,1024,"C2010F0000%s",Send);
			sprintf(szSend,"C2010F0000%s",Send);
			gCommFlag = 0;
		}

	}else if( CardType == 8)			// ������Ϊ��MPRO
	{

		if( gCommFlag == 0)
		{
			//sprintf_s(szSend,1024,"C2010A00%s",Send);
			sprintf(szSend,"C2010A00%s",Send);
			gCommFlag += 1;
		}else
		{
			//sprintf_s(szSend,1024,"C2010B00%s",Send);
			sprintf(szSend,"C2010B00%s",Send);
			gCommFlag = 0;
		}

	}else if ( CardType == 2)			// ������Ϊ��S70   -- Update -- 20120808 -- ָ���INS��CLS�������ı䣬���޸�gCommFlag ֵ
	{

		if( gCommFlag == 0)
		{
			//sprintf_s(szSend,1024,"C2010A00%s",Send);
			sprintf(szSend,"C2010A00%s",Send);

			if( memcmp(Send,stDeviceStruct[nDev].szInsCls,4) != 0)
			{
				gCommFlag += 1;

				// �����ϸ�ָ���INS��CLA
				memcpy(stDeviceStruct[nDev].szInsCls,Send,4);
			}
		}else
		{
			//sprintf_s(szSend,1024,"C2010B00%s",Send);
			sprintf(szSend,"C2010B00%s",Send);
			if( memcmp(Send,stDeviceStruct[nDev].szInsCls,4) != 0)
			{			
				gCommFlag = 0;

				// �����ϸ�ָ���INS��CLA
				memcpy(stDeviceStruct[nDev].szInsCls,Send,4);
			}
		}

	}else 
	{
		if( gCommFlag == 0)
		{
			//sprintf_s(szSend,1024,"C2010E0000%s",Send);
			sprintf(szSend,"C2010E0000%s",Send);
			gCommFlag += 1;
		}else
		{
			//sprintf_s(szSend,1024,"C2010F0000%s",Send);
			sprintf(szSend,"C2010F0000%s",Send);
			gCommFlag = 0;
		}
	}

	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
 	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}


	// ���ݿ�����������������Ӧ������
	if(CardType == 4)						// M1��Ӧ�����������
	{
		i = nRecvLen-6;
		memcpy(Recv,szRecv+6,nRecvLen-6);
		memcpy(Recv+i,"\x00",1);
		*RecvLen = nRecvLen - 6;

	}else if(CardType == 8)					// MPRO��Ӧ�����������
	{
		i = nRecvLen -4;
		memcpy(Recv,szRecv+4,nRecvLen-4);
		memcpy(Recv+i,"\x00",1);
		*RecvLen = nRecvLen - 4;

	}else if( CardType == 2)				// S70��Ӧ�����������
	{
		i = nRecvLen -4;
		memcpy(Recv,szRecv+4,nRecvLen-4);
		memcpy(Recv+i,"\x00",1);
		*RecvLen = nRecvLen - 4;

	}else									// �������Ϳ���Ӧ�����������
	{
		i = nRecvLen;
		*RecvLen = nRecvLen;
		memcpy(Recv,szRecv,nRecvLen);
		memcpy(Recv+i,"\x00",1);
	}

	return R531OK;
}

// int R531TypeAPDU(int hDev,char *Send,char *Recv,int *Status=NULL,int CardType=4);
int R531TypeAPDU(int hDev,char *Send,char *Recv,int *Status ,int CardType )
{
	int nSndLen;
	int nRcvLen;
	int nStatus;
	int nRet;

	nSndLen = (int)strlen(Send);

	//int R531TypeAPDU(int hDev,int SendLen,char *Send,int *RecvLen,char *Recv,int *Status=NULL,int CardType = 4 );
	nRet = R531TypeAPDU(hDev,nSndLen,Send,&nRcvLen,Recv,&nStatus,CardType);
	*Status = nStatus;
	return nRet;
}


//==================================================================================================
//7-M1���������װ
//==================================================================================================

/******************************************************************************************
 *******	�������ƣ�R531M1Auth	-��ţ�35										*******
 *******	�������ܣ�M1����֤,ָ��[C300]											*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******			  unsigned char ABLK�����ַ��[��������]������ֵ���£�			*******
 *******					M1S50��(04)��00��3F,��1�����ַΪ��0x00					*******
 *******					M1S70��(02)��00��255[FF]��								*******
 *******					ML��   (08)��00��0B										*******
 *******			  unsigned char KT����Կ���ͣ�ֵ���£�							*******
 *******                    R531KTUSERKEYA	:�û�������Կ��֤KEYA					*******
 *******					R531KTUSERKEYB	:�û�������Կ��֤KEYB					*******
 *******					R531KTROMKEYA	:ʹ����Ƶģ���д洢����Կ��֤KEYA		*******
 *******					R531KTROMKEYB	:ʹ����Ƶģ���д洢����Կ��֤KEYB		*******
 *******			  char *KData��KTΪR531KTROMKEYA��R531KTROMKEYB					******* 
 *******                                       ��ֵΪһ�ֽ���Կ����,��Χ0��1F		*******
 *******							 ΪR531KTUSERKEYA��R531KTUSERKEYB				*******
 *******									��ֵΪ12�ֽ���Կ						*******
 *******			  char *SN:�����,4�ֽڵ�UID,M1��Ҫ��C101ָ���				*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 ******* ָ��ʵ����c300 0000c0 0f0f0f0f0f0f0f0f0f0f0f0f e7018bae					*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�R531OK��ʧ�ܷ��أ��������							*******
 ******************************************************************************************/
int R531M1Auth(int hDev,unsigned char ABLK,unsigned char KT, char *KData, char *SN,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];
	unsigned char szKeyType;	
	unsigned char CardType;

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}
 
	// END Add 
	
	CardType = (unsigned char)stDeviceStruct[nDev].nCardType;



	// ����Ŀ����Ͳ����Ϸ��Լ��
	if(  !(CardType ==  R531MODES50 || CardType == R531MODES70 || CardType == R531MODEML ) )
	{	// �����M1�����Ͳ�������(R531TypeARequest ��������)
		return R531PARAMERR;
	}
	
	// ���ַ�����Ϸ��Լ��
	if( CardType == R531MODES50)
	{
		if( ABLK < 0 || ABLK > 0x3F)
		{
			return R531PARAMERR;
		}

	}else if ( CardType == R531MODES70)
	{
		if( ABLK < 0 || ABLK > 0xFF)
		{
			return R531PARAMERR;
		}

	}else if( CardType == R531MODEML)
	{
		if( ABLK < 0 || ABLK > 0x0B)
		{
			return R531PARAMERR;
		}

	} 	


	// �������Կ���Ͳ����Ϸ��Լ��
	if( !( KT == R531KTUSERKEYA || KT == R531KTUSERKEYB || KT == R531KTROMKEYA || KT == R531KTROMKEYB))
	{
		return R531PARAMERR;
	}

	// ���ݴ������Կ���Ͳ�������ָ��ĵ���Կ����ֵ
	switch(KT)
	{
		case R531KTUSERKEYA:
			szKeyType =(unsigned char) 0xC0;
			break;

	case R531KTUSERKEYB:
			szKeyType =(unsigned char) 0xC1;
			break;

	case R531KTROMKEYA:
			szKeyType = (unsigned char) 0x40;
			break;

	case R531KTROMKEYB:
			szKeyType = 0x41;
			break;
	}

	// ���ݴ������Կ���ͣ���鴫�����Կ�����ĺϷ���
	if( KT == R531KTUSERKEYA || KT == R531KTUSERKEYB)		// 12�ֽ���Կ
	{
		if( strlen(KData) != 24 )
		{
			return R531PARAMERR;
		}
	} else													//  1�ֽڵ���Կ����
	{
		if( strlen(KData) != 2)
		{
			return R531PARAMERR;
		}
	}


	//��鴫���UID�ĺϷ���
	if( strlen(SN) != 8)
	{
		return R531PARAMERR;
	}

	//����ִ�е�ָ��
	//c3 00 00  00 c0 0f 0f   0f 0f 0f 0f   0f 0f 0f 0f   0f 0f e7 01  8b ae 
	if(CardType == R531MODES50 || CardType == R531MODES70)			// M1-S50�� ��M1-S70��
	{
		// ����:C30000 +1�ֽڿ��ַ + ��Կ����
		//sprintf_s(szSend,1024,"C30000%02.2X%02.2X%s%s",
		sprintf(szSend,"C30000%02.2X%02.2X%s%s",
						ABLK,		// 1�ֽڿ��ַ
						szKeyType,	// 1�ֽ���Կ����
						KData,		// KT b7 = 0:1�ֽڵ���Կ������b7 =1:12�ֽ���Կ
						SN);
	}else							// MPRO��S70��
	{
		//sprintf_s(szSend,1024,"C30000%02.2X%02.2X%s",
		sprintf(szSend,"C30000%02.2X%02.2X%s",
						ABLK,
						szKeyType,
						KData);

	}

	nSendLen = (int)strlen(szSend);
	

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	return R531OK;
}



/******************************************************************************************
 *******	�������ƣ�R531ReadBlock		-��ţ�36									*******
 *******	�������ܣ�����,ָ��[C301]												*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
  *******			  unsigned char ABLK�����ַ��[��������]������ֵ���£�			*******
 *******					M1S50����00��3F,��1�����ַΪ��0x00						*******
 *******					M1S70����00��255[FF]��									*******
 *******					ML��   ��00��0B											*******
 *******			  unsigned char Lc��Ҫ��ȡ�����ݳ���;M1��󳤶�Ϊ��0x10,16�ֽ�	*******
 *******			  int *RecvLen��(���)Ӧ�����ݳ���[ASCII�볤��]					*******
 *******			  char *Recv��(���)Ӧ������[ASCII���ʽ]						*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�R531OK��ʧ�ܷ��أ��������							*******
 ******************************************************************************************/
int R531M1ReadBlock(int hDev,unsigned char ABLK,int *RecvLen,char *Recv,int *Status,unsigned char Lc )
{
	char szSend[1024];				
	char szRecv[1024];
	unsigned char CardType;
	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}
 
	// END Add 

	CardType = (unsigned char)stDeviceStruct[nDev].nCardType;


	// ����Ŀ����Ͳ����Ϸ��Լ��
	if(  !(CardType ==  R531MODES50 || CardType == R531MODES70 || CardType == R531MODEML ) )
	{	// �����M1�����Ͳ�������(R531TypeARequest ��������)
		return R531PARAMERR;
	}
	
	// ���ַ�����Ϸ��Լ��
	if( CardType == R531MODES50)
	{
		if( ABLK < 0 || ABLK > 0x3F)
		{
			return R531PARAMERR;
		}

	}else if ( CardType == R531MODES70)
	{
		if( ABLK < 0 || ABLK > 0xFF)
		{
			return R531PARAMERR;
		}

	}else if( CardType == R531MODEML)
	{
		if( ABLK < 0 || ABLK > 0x0B)
		{
			return R531PARAMERR;
		}

	} 	

	//����ִ�е�ָ��;C301 +1�ֽڿ����ͣ��̶�Ϊ��0x00;+1�ֽڵĿ��ַ; +1�ֽڵ�Ҫ��ȡ�����ݳ���
	//c3 01 00  00 10
	//sprintf_s(szSend,1024,"C30100%02.2X%02.2X",
	sprintf(szSend,"C30100%02.2X%02.2X",
					ABLK,					// 1�ֽڵĶ�ȡ���ַ
					Lc);					// 1�ֽڵĶ�ȡ���ݳ���

	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}


	// ȡӦ����Ϣ
	memcpy(Recv,szRecv,nRecvLen);
	*RecvLen = nRecvLen ;

	return R531OK;
}




/******************************************************************************************
 *******	�������ƣ�R531M1WriteBlock	-��ţ�37									*******
 *******	�������ܣ�д��,ָ��[C302]												*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******			  unsigned char cardType:M1�����ͣ���:C100ָ��أ�ֵ���£�	*******
 *******					0x04��M1-S50����										*******
 *******					0x08��MPRO����											*******
 *******					0x02��M1-S70��											*******
 *******			  unsigned char ABLK�����ַ��[��������]������ֵ���£�			*******
 *******					M1S50����00��3F,��1�����ַΪ��0x00						*******
 *******					M1S70����00��255[FF]��									*******
 *******					ML��   ��00��0B											*******
 *******			  unsigned char Lc��Ҫд������ݳ���;M1��󳤶�Ϊ��0x10,16�ֽ�	*******
 *******              unsigned char *WriteData:Ҫд�������							*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0,���������£�						*******
 *******							-1��ָ��ȴ���								*******
 *******							-2��ָ�����									*******
 *******							-3��ָ��ִ��ʧ��								*******
 *******							-4��ָ��ִ�д���								*******
 ******************************************************************************************/
int R531M1WriteBlock(int hDev,unsigned char ABLK,unsigned char Lc, char *WriteData,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];
	
	unsigned char CardType;

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDataLen;
	int nDev;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}
 
	// END Add 

	CardType = (unsigned char )stDeviceStruct[nDev].nCardType;

	// ����Ŀ����Ͳ����Ϸ��Լ��
	if(  !(CardType ==  R531MODES50 || CardType == R531MODES70 || CardType == R531MODEML ) )
	{	// �����M1�����Ͳ�������(R531TypeARequest ��������)
		return R531PARAMERR;
	}
	
	// ���ַ�����Ϸ��Լ��
	if( CardType == R531MODES50)
	{
		if( ABLK < 0 || ABLK > 0x3F)
		{
			return R531PARAMERR;
		}

	}else if ( CardType == R531MODES70)
	{
		if( ABLK < 0 || ABLK > 0xFF)
		{
			return R531PARAMERR;
		}

	}else if( CardType == R531MODEML)
	{
		if( ABLK < 0 || ABLK > 0x0B)
		{
			return R531PARAMERR;
		}

	} 		
 
	nDataLen = (int)Lc;
	
	// ������ݳ��Ȳ���������ʵ�ʳ��ȵ�һ����
	if( nDataLen*2 != (int)strlen( (char *)WriteData ) )
	{
		return R531PARAMERR;
	}

	//����ִ�е�ָ��;C302 +1�ֽڿ����ͣ��̶�Ϊ��0x00[M1��];+1�ֽڵĿ��ַ; +1�ֽ�д�����ݳ��� + д������
	//sprintf_s(szSend,1024,"C30200%02.2X%02.2X%s",
	sprintf(szSend,"C30200%02.2X%02.2X%s",
					ABLK,				// д��Ŀ��ַ
					Lc,					// д������ݳ���
					WriteData);			// д�������,
				
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	return R531OK;
}




/******************************************************************************************
 *******	�������ƣ�R531M1AddValue	-��ţ�38									*******
 *******	�������ܣ���ֵ,ָ��[C303]												*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******			  unsigned char cardType:M1�����ͣ���:C100ָ��أ�ֵ���£�	*******
 *******					0x04��M1-S50����										*******
 *******					0x08��MPRO����											*******
 *******					0x02��S70��												*******
 *******			  unsigned char ABLK�����ַ��[��������]������ֵ���£�			*******
 *******					M1S50����00��3F,��1�����ַΪ��0x00						*******
 *******					M1S70����00��255[FF]��									*******
 *******					ML��   ��00��0B											*******
 *******			  unsigned char Lc��Ҫ���ӵ�����ֵ����;							*******
 *******              unsigned char *Data:Ҫ���ӵ�����ֵ4�ֽ�						*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0,���������£�						*******
 *******							-1��ָ��ȴ���								*******
 *******							-2��ָ�����									*******
 *******							-3��ָ��ִ��ʧ��								*******
 *******							-4��ָ��ִ�д���								*******
 ******************************************************************************************/
int R531M1AddValue(int hDev,unsigned char ABLK,unsigned char Lc,char *Data,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];
	unsigned char CardType;

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nDataLen;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}
 
	// END Add 
	CardType = (unsigned char)stDeviceStruct[nDev].nCardType;

	// ����Ŀ����Ͳ����Ϸ��Լ��
	if(  !(CardType ==  R531MODES50 || CardType == R531MODES70 || CardType == R531MODEML ) )
	{	// �����M1�����Ͳ�������(R531TypeARequest ��������)
		return R531PARAMERR;
	}
	
	// ���ַ�����Ϸ��Լ��
	if( CardType == R531MODES50)
	{
		if( ABLK < 0 || ABLK > 0x3F)
		{
			return R531PARAMERR;
		}

	}else if ( CardType == R531MODES70)
	{
		if( ABLK < 0 || ABLK > 0xFF)
		{
			return R531PARAMERR;
		}

	}else if( CardType == R531MODEML)
	{
		if( ABLK < 0 || ABLK > 0x0B)
		{
			return R531PARAMERR;
		}

	} 		
 
	nDataLen = (int)Lc;

	// Lc ֵ����Ϊ��0x10
	if( nDataLen != 16 )
	{
		return R531PARAMERR;
	}


	// ������Ϊ4�ֽ�
	if( strlen( Data ) != 8)
	{
		return R531PARAMERR;
	}
	 

	//ָ��ʵ����c3 03 00  00 10 01 00  00 00
	//����ִ�е�ָ��;C303 +1�ֽڿ����ͣ��̶�Ϊ��0x00[M1��];+1�ֽڵĿ��ַ; +1�ֽ����ݳ��� + ����
	//sprintf_s(szSend,1024,"C30300%02.2X%02.2X%s",
	sprintf(szSend,"C30300%02.2X%02.2X%s",
					ABLK,					// ��ֵ���ַ
					Lc,						// ����
					Data					// ��ֵ������
					);

	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	return R531OK;
}



/******************************************************************************************
 *******	�������ƣ�R531M1SubValue	-��ţ�39									*******
 *******	�������ܣ���ֵ,ָ��[C304]												*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******			  unsigned char cardType:M1�����ͣ���:C100ָ��أ�ֵ���£�	*******
 *******					0x04��M1-S50����										*******
 *******					0x08��MPRO����											*******
 *******					0x02��S70��												*******
 *******			  unsigned char ABLK�����ַ��[��������]������ֵ���£�			*******
 *******					M1S50����00��3F,��1�����ַΪ��0x00						*******
 *******					M1S70����00��255[FF]��									*******
 *******					ML��   ��00��0B											*******
 *******			  unsigned char Lc��Ҫ��������ֵ����;							*******
 *******              unsigned char *Data:Ҫ��������ֵ4�ֽ�							*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0,���������£�						*******
 *******							-1��ָ��ȴ���								*******
 *******							-2��ָ�����									*******
 *******							-3��ָ��ִ��ʧ��								*******
 *******							-4��ָ��ִ�д���								*******
 ******************************************************************************************/
int R531M1SubValue(int hDev,unsigned char ABLK,unsigned char Lc, char *Data,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];
	unsigned char CardType;

	int nSendLen;
	int nRecvLen;
	int nDataLen;
	int nStatus;
	int nDev;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}
 
	// END Add 

	// Update��2012-07-23 ,M1�������޸�Ϊ�Ӷ������ṹ�����л�ȡ����ֵ��TypeA����ģʽѰ��ʱ���ݷ���ֵ���ã�
	CardType = (unsigned char)stDeviceStruct[nDev].nCardType;

	// ����Ŀ����Ͳ����Ϸ��Լ��
	if(  !(CardType ==  R531MODES50 || CardType == R531MODES70 || CardType == R531MODEML ) )
	{	// �����M1�����Ͳ�������(R531TypeARequest ��������)
		return R531PARAMERR;
	}
	
	// ���ַ�����Ϸ��Լ��
	if( CardType == R531MODES50)
	{
		if( ABLK < 0 || ABLK > 0x3F)
		{
			return R531PARAMERR;
		}

	}else if ( CardType == R531MODES70)
	{
		if( ABLK < 0 || ABLK > 0xFF)
		{
			return R531PARAMERR;
		}

	}else if( CardType == R531MODEML)
	{
		if( ABLK < 0 || ABLK > 0x0B)
		{
			return R531PARAMERR;
		}

	} 		
 
	nDataLen = (int)Lc;

	// Lc ֵ����Ϊ��0x10
	if( nDataLen != 16 )
	{
		return R531PARAMERR;
	}


	// ������Ϊ4�ֽ�
	if( strlen( Data ) != 8)
	{
		return R531PARAMERR;
	}

	//ָ��ʵ����c3 03 00  00 10 01 00  00 00
	//����ִ�е�ָ��;C303 +1�ֽڿ����ͣ��̶�Ϊ��0x00[M1��];+1�ֽڵĿ��ַ; +1�ֽ����ݳ��� + ����
	//sprintf_s(szSend,1024,"C30400%02.2X%02.2X%s",
	sprintf(szSend,"C30400%02.2X%02.2X%s",
					ABLK,					// ��ֵ���ַ
					Lc,						// ����
					Data					// ��ֵ������
					);

	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;
	}

	return R531OK;
}



/******************************************************************************************
 *******	�������ƣ�R531M1Transfer	-��ţ�40									*******
 *******	�������ܣ����ڲ���ʱ�Ĵ���������д��ֵ�洢��,ָ��[C306]					*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******			  unsigned char ABLK�����ַ��[��������]������ֵ���£�			*******
 *******					M1S50����00��3F,��1�����ַΪ��0x00						*******
 *******					M1S70����00��255[FF]��									*******
 *******					ML��   ��00��0B											*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�<0,���������£�						*******
 *******							-1��ָ��ȴ���								*******
 *******							-2��ָ�����									*******
 *******							-3��ָ��ִ��ʧ��								*******
 *******							-4��ָ��ִ�д���								*******
 ******************************************************************************************/
int R531M1Transfer(int hDev,unsigned char ABLK,int *Status )
{
	char szSend[1024];	
	char szRecv[1024];
	unsigned char CardType;

	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}
 
	// END Add 

	// Update��2012-07-23 ,M1�������޸�Ϊ�Ӷ������ṹ�����л�ȡ����ֵ��TypeA����ģʽѰ��ʱ���ݷ���ֵ���ã�
	CardType = (unsigned char)stDeviceStruct[nDev].nCardType;

	// ����Ŀ����Ͳ����Ϸ��Լ��
	if(  !(CardType ==  R531MODES50 || CardType == R531MODES70 || CardType == R531MODEML ) )
	{	// �����M1�����Ͳ�������(R531TypeARequest ��������)
		return R531PARAMERR;
	}
	
	// ���ַ�����Ϸ��Լ��
	if( CardType == R531MODES50)
	{
		if( ABLK < 0 || ABLK > 0x3F)
		{
			return R531PARAMERR;
		}

	}else if ( CardType == R531MODES70)
	{
		if( ABLK < 0 || ABLK > 0xFF)
		{
			return R531PARAMERR;
		}

	}else if( CardType == R531MODEML)
	{
		if( ABLK < 0 || ABLK > 0x0B)
		{
			return R531PARAMERR;
		}

	} 		

	//ָ��ʵ����c3 06 00  08 
	//����ִ�е�ָ��;C306 +1�ֽڿ����ͣ��̶�Ϊ��0x00[M1��];+1�ֽڵĿ��ַ; 
	//sprintf_s(szSend,1024,"C30600%02.2X",
	sprintf(szSend,"C30600%02.2X",
					ABLK					// ��ֵ���ַ
					);

	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// ����ָ���״ֵ̬�������������������ѡ���ǿ�ʱ���ô�ֵ
		if(Status != NULL)
		{
			*Status = nStatus;
		}

		if(nStatus != R531OK)
		{
			return R531RESPDATALENERR;
		}
		return nRet;

	}

	return R531OK;
}




/******************************************************************************************
 *******	�������ƣ�R531M1Restore		-��ţ�41									*******
 *******	�������ܣ���ֵ�洢�ε����������ڲ���ʱ�Ĵ���,ָ��[C305]					*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******			  unsigned char ABLK�����ַ��[��������]������ֵ���£�			*******
 *******					M1S50����00��3F,��1�����ַΪ��0x00						*******
 *******					M1S70����00��255[FF]��									*******
 *******					ML��   ��00��0B											*******
 *******			  int *Status��(��ѡ�����)ָ��ִ��״̬,�ɹ����أ�0��			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�R531OK��ʧ�ܷ��أ�������							*******
 ******************************************************************************************/
int R531M1Restore(int hDev,unsigned char ABLK,int *Status )
{
	char szSend[1024];	
	char szRecv[1024];
	unsigned char CardType;

	int nSendLen;
	int nRecvLen;
	int nDev;
	int nStatus;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}
 
	// END Add 

	// Update��2012-07-23 ,M1�������޸�Ϊ�Ӷ������ṹ�����л�ȡ����ֵ��TypeA����ģʽѰ��ʱ���ݷ���ֵ���ã�
	CardType = (unsigned char)stDeviceStruct[nDev].nCardType;

	// ����Ŀ����Ͳ����Ϸ��Լ��
	if(  !(CardType ==  R531MODES50 || CardType == R531MODES70 || CardType == R531MODEML ) )
	{	// �����M1�����Ͳ�������(R531TypeARequest ��������)
		return R531PARAMERR;
	}
	
	// ���ַ�����Ϸ��Լ��
	if( CardType == R531MODES50)
	{
		if( ABLK < 0 || ABLK > 0x3F)
		{
			return R531PARAMERR;
		}

	}else if ( CardType == R531MODES70)
	{
		if( ABLK < 0 || ABLK > 0xFF)
		{
			return R531PARAMERR;
		}

	}else if( CardType == R531MODEML)
	{
		if( ABLK < 0 || ABLK > 0x0B)
		{
			return R531PARAMERR;
		}

	} 	

	//ָ��ʵ����c3 05 00  08 
	//����ִ�е�ָ��;C305 +1�ֽڿ����ͣ��̶�Ϊ��0x00[M1��];+1�ֽڵĿ��ַ; 
	//sprintf_s(szSend,1024,"C30500%02.2X",
	sprintf(szSend,"C30500%02.2X",
					ABLK					// ��ֵ���ַ
					);

	nSendLen = (int)strlen(szSend);


	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		return nRet;
	}


	return R531OK;
}


//ADD-2013-11-06 
/******************************************************************************************
 *******	�������ƣ�R531TypeFindCard		-��ţ�42								*******
 *******	�������ܣ�δ����14443-4���Ѱ��,ָ��[C400]								*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
  *******			  int *Status��0x00--�п���0x01 --�޿�							*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�R531OK��ʧ�ܷ��أ�������							*******
 ******************************************************************************************/
int R531TypeFindCard(int hDev,int *Status )
{
	char szSend[1024];	
	char szRecv[1024];

	int nSendLen;
	int nRecvLen;
	int nDev;
	int nStatus;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	}
 
	//����ִ�е�ָ��:C400
	memcpy(szSend,"C400",5);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{		
		*Status = nStatus;
		return R531NOCARD;
	}
	
	*Status = 0x00;

	return R531OK;
}


/******************************************************************************************
 *******	�������ƣ�R531TypeFindCard1		-��ţ�43								*******
 *******	�������ܣ��ѽ���14443-4���Ѱ��,ָ��[C401]								*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
  *******			  int *Status��0x00--�п���0x01 --�޿�							*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�R531OK��ʧ�ܷ��أ�������							*******
 ******************************************************************************************/
int R531TypeFindCard2(int hDev,int *Status )
{
	char szSend[1024];	
	char szRecv[1024];

	int nSendLen;
	int nRecvLen;
	int nDev;
	int nStatus;
	int nRet;

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//ADD-20120716 ���������л������
	nDev = hDev -1;

	// ������Ϸ���
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// ������������Ƿ񳬷�Χ
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;		
	}

	// ��鵱ǰ�������Ƿ�������
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// ������δ����
		return R531UNCONN;
	} 
	// END Add 

	//����ִ�е�ָ��:C401
	memcpy(szSend,"C401",5);
	nSendLen = (int)strlen(szSend);

	// ִ��ָ���ȡӦ��
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	*Status = nStatus;
	if( nStatus == 0)
	{
		return NO_ERROR;
	}else 
	{
		return R531NOCARD;
	}
	return R531OK;
}

/******************************************************************************************
 *******	�������ƣ�R531GetErrMsg			-��ţ�44								*******
 *******	�������ܣ���ȡAPI�����������Ĵ�����Ϣ									*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
  *******			  int *Status��0x00--�п���0x01 --�޿�							*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�R531OK��ʧ�ܷ��أ�������							*******
 ******************************************************************************************/
int R531GetErrMsg(int ErrCode,char *ErrMsg)
{
	memcpy(ErrMsg,"\x00",1);

	switch(ErrCode)
	{
	case R531OK:
		memcpy(ErrMsg,"�ɹ����",9);
		break;
	case R531SYSERR:
		memcpy(ErrMsg,"ϵͳ����",9);
		break;
	case R531NODEV:
		memcpy(ErrMsg,"ϵͳ�޶������豸",17);
		break;
	case R531NOEXIST:
		memcpy(ErrMsg,"�豸������",11);
		break;
	case R531CONNERR:
		memcpy(ErrMsg,"�豸���Ӵ���",13);
		break;
	case R531UNCONN:
		memcpy(ErrMsg,"������δ����",13);
		break;
	case R531PARAMERR:
		memcpy(ErrMsg,"��������",9);
		break;
	case R531NOCARD:
		memcpy(ErrMsg,"�޿�",5);
		break;
	case R531NORESPONSE:
		memcpy(ErrMsg,"��Ƭ��Ӧ��",11);
		break;
	case R531NOINIT:
		memcpy(ErrMsg,"δ��ʼ���豸",13);
		break;
	case R531RESPDATALENERR:
		memcpy(ErrMsg,"IC����Ӧ���Ӧ�����ݳ��ȴ���",29);
		break;
	case R531EXECUTEERR:
		memcpy(ErrMsg,"�ײ��豸ָ��ִ��ʧ��",21);
		break;
	case R531SLOTERR:
		memcpy(ErrMsg,"���۲�������",13);
		break;
	case R531NOSETSLOT:
		memcpy(ErrMsg,"δ���ÿ���",11);
		break;
	case R531DEVERR:
		memcpy(ErrMsg,"����������Ƿ�",15);
		break;
	case R531RESETERR:
		memcpy(ErrMsg,"�ϵ縴λʧ��",13);
		break;
	case R531RESETNOT:
		memcpy(ErrMsg,"δ�ϵ縴λ",11);
		break;
	case R531NOUSE:
		memcpy(ErrMsg,"δ��Ȩ����ʹ��",15);
		break;
	case R531SAMNOERR:
		memcpy(ErrMsg,"PSAM�͹�����һ��",19);
		break;
	case R531VERMACERR:
		memcpy(ErrMsg,"��֤����MACִ�е�ָ�����",26);
		break;
	case R531MACERR:
		memcpy(ErrMsg,"MAC��֤����",12);
		break;
	case R531RESPLENERR:
		memcpy(ErrMsg,"Ӧ�����ݳ��ȴ���",17);
		break;
	case R531RESPERR:
		memcpy(ErrMsg,"APDUִ�д��󷵻ص�SWֵ��Ϊ:9000",32);
		break;
	default:
		memcpy(ErrMsg,"δ֪����",9);
		break;
	}

	return NO_ERROR;
}


// ��ȡϵͳ��̬ȫ������ֵ
#ifdef MANA
int R531Test()
{



	ftWriteLog("R531Test.log","#############################��ؾ�̬�������ݻ�ȡ############################");
	ftWriteLog("R531Test.log","��ʼ����־[%d]",gInitFlag);
	ftWriteLog("R531Test.log","R531�豸����=[%d]",gDeviceTotal);

	int i=0;

	for(i=0;i<gDeviceTotal;i++)
	{
		ftWriteLog("R531Test.log","�豸���=[%d] ״̬=[%d] �ϵ��־=[%d][%d][%d][%d],Э������=[%d][%d][%d][%d]",(i+1),
					stDeviceStruct[i].nConnStatus,
					stDeviceStruct[i].nUserFlag,
					stDeviceStruct[i].nSIM1Flag,
					stDeviceStruct[i].nSIM2Flag,
					stDeviceStruct[i].nSIM3Flag,
					stDeviceStruct[i].nUserProtocol,
					stDeviceStruct[i].nSIM1Protocol,
					stDeviceStruct[i].nSIM2Protocol,
					stDeviceStruct[i].nSIM3Protocol);
	}

	return 0;
}
#endif
