/******************************************************************************************
 ** 文件名称：R531DeviceFunc.cpp                                                         **
 ** 文件描述：ROCKEY531设备接口函数														 **
 **--------------------------------------------------------------------------------------**
 ** 创 建 人：智能卡业务部                                                               **
 ** 创建日期：2012-07-05                                                                 **
 **--------------------------------------------------------------------------------------**
 ** 修 改 人：                                                                           **
 ** 修改日期：                                                                           **
 **--------------------------------------------------------------------------------------**
 ** 版 本 号： V1.0                                                                      **
 **--------------------------------------------------------------------------------------**
 **                      Copyright (c) 2012  ftsafe                                      **
 ******************************************************************************************/
#include <stdio.h> 
#include <stdlib.h>
#include <memory.h> 
#include <math.h>
#include <time.h>

// 线程同步处理相关头文件引用

#include <windows.h>
#include <process.h>
//#include <afxmt.h>

// 公共库函数
#include "ATR.h"
#include "publicFunc.h"
#include "R531DeviceFunc.h"

// HID 设备库头文件
extern "C" {
#include "hidsdi.h"			// Must link in hid.lib
#include "setupapi.h"		// Must link in setupapi.lib
}

// 定义读卡器信息结构
#define MAXDEVICENUM			 10			// ROCKEY531设备最大数量

// 设备连接相关信息数据结构
typedef struct {
	int    nConnStatus;						// 设备连接状态；-1:无此读卡器；0-未连接；1-已连接 ;2-已关闭
	char   szDevName[128];					// HID设备名称,读卡器插入的USB口不同，HID设备名称不同
	HANDLE hDev;							// HID设备句柄
	int    nMaxDataLen;						// 数据交换最大数据长度
	int    nUserFlag;						// 用户模块处理标志；0-未上电；1-已上电复位；2-已下电
	int    nSIM1Flag;						// SIM1模块处理标志；
	int    nSIM2Flag;						// SIM2模块处理标志；
	int    nSIM3Flag;						// SIM3模块处理标志；
	int    nUserProtocol;					// 用户卡槽，IC卡通信协议；-1: 未设置；0-T=0;1-T=1;
	int    nSIM1Protocol;					// SIM1卡槽，IC卡通信协议；
	int    nSIM2Protocol;					// SIM2卡槽，IC卡通信协议；
	int    nSIM3Protocol;					// SIM3卡槽，IC卡通信协议；
	int    nCurrSlot;						// 当前指令操作的模块编号，-1:未设置；0:用户卡模块；1:SIM1；2:SIM2;3：SIM3;
	int    nCardType;						// 非接卡类型, 4 - M1S50;	2 - M1S70;	8 - MPR0
	char   szInsCls[5];						// 上个指令INS和CLA值	用非接卡类型为：02 的M1S70卡	-- Add - 20120808
} DeviceStruct;



// ROCKEY501-CCID 设备信息
static DeviceStruct stDeviceStruct[MAXDEVICENUM];

// 设备连接标志
static int gInitFlag = 0;
static int gDeviceTotal    = 0;			// 当前系统ROCKEY501-CCID设备个数

//CCriticalSection oCriticalSection;		// 定义线程临界区类对象
static HANDLE op_mutex=NULL;


static int gCommFlag;					// 非接指令执行奇偶标志
static int gCommFlag2;                  // 接触T=1的指令执行奇偶标志


//==================================================================================================
//1-R531读卡器公共函数封装
//==================================================================================================
/****************************************************************************************** 
 *******    函数名称：R531DeviceFind		-序号：1 -								*******
 *******	函数功能：获取系统的HID-ROCKETY531,并将设备名称及连接句柄保存到全局变量	*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：int pDeviceSeq:要打开的ROCKEY501-CCID 设备序号(1~9)			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：设备个数；失败返回：<=0	;错误代码如下,				******* 
 *******                 0：系统无ROCKEY501读卡器									*******
 *******				-1：取系统HID设备信息集失败									*******
 *******				-2：分配HID接口设备详细信息结构指针失败						*******
 *******				-3：获取HID设备详细信息失败									*******
 *******				-4：执行HidD_GetPreparsedData函数失败						*******
 *******				-5：执行HidP_GetCaps函数失败								*******
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

	// 增加同步调用代码，获得临界区对象
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

	// 初始化设备信息结构
	//stDeviceStruct
	if( gInitFlag  == 0)
	{
		for(i=0; i< MAXDEVICENUM;i++ )
		{
			stDeviceStruct[i].nConnStatus	= CONNSTATNODEV;	// 初始化为无此设备
			memset(stDeviceStruct[i].szDevName,0x00,128);		// HID设备路径名称
			stDeviceStruct[i].hDev = INVALID_HANDLE_VALUE;		// 设备句柄
			stDeviceStruct[i].nMaxDataLen	= 0;				// 数据交换的最大长度
			stDeviceStruct[i].nUserFlag		= RESETSTATNOT;		// 设备用户卡模块上电标志，0-未上电；1-已上电；2-已下电; 3-上电不成功[Add-20120319]
			stDeviceStruct[i].nSIM1Flag		= RESETSTATNOT;		// SIM1模块上电标志
			stDeviceStruct[i].nSIM2Flag		= RESETSTATNOT;		// SIM2模块上电标志
			stDeviceStruct[i].nSIM3Flag		= RESETSTATNOT;		// SIM3模块上电标志		
			stDeviceStruct[i].nUserProtocol = R531PROTOCOLNO;	// 设备用户卡模块通信协议,-1: 未设置；0：T=0;  1：T=1;
			stDeviceStruct[i].nSIM1Protocol	= R531PROTOCOLNO;	// SIM1模块通信协议
			stDeviceStruct[i].nSIM2Protocol	= R531PROTOCOLNO;	// SIM1模块通信协议
			stDeviceStruct[i].nSIM3Protocol	= R531PROTOCOLNO;	// SIM1模块通信协议
			stDeviceStruct[i].nCurrSlot		= -1;				// 当前读卡器使用的卡槽；-1:未设置；0:用户卡模块；1:SIM1；2:SIM2;3：SIM3

			memset(stDeviceStruct[i].szInsCls,0x00,5);			// 用于类型为：0x02 的非接IC卡
		}
	}


	// 获取HID 设备GUID 
	HidD_GetHidGuid(&guidHID);


	// 获取HID设备信息集
	hDevInfo = SetupDiGetClassDevs( &guidHID, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE );
	if(hDevInfo == INVALID_HANDLE_VALUE)
	{
		//oCriticalSection.Unlock();
		ReleaseMutex(op_mutex);
		return  R531SYSERR;
	}

	memset( &InterfaceData, 0, sizeof( SP_DEVICE_INTERFACE_DATA ) );
	InterfaceData.cbSize = sizeof( SP_DEVICE_INTERFACE_DATA );
	
	// 要获取的系统HID设备序号,
	i = 0;

	nRet = 0;
	nDeviceNum = 0;
	while(1)
	{
		// 3.1：从设备信息集中获取设备信息
		if( !SetupDiEnumDeviceInterfaces( hDevInfo, NULL, &guidHID, i, &InterfaceData ) )
		{ 
			SetupDiDestroyDeviceInfoList( hDevInfo );
			nRet = nDeviceNum;
			break;
			//return nDeviceNum;
		}

		length = 0;

		// 3.2.获取设备接口详细信息
		Success = SetupDiGetDeviceInterfaceDetail( hDevInfo, &InterfaceData, NULL, 0, &length, NULL );
		 
		if(Success != 0)	
		{
			SetupDiDestroyDeviceInfoList( hDevInfo );
			i++;
			continue;
		}

		// 分配接口详细信息结构指针
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


		//打开设备,获取句柄,打开方式改为可读写
		HIDHandleRead = CreateFile( DetailData->DevicePath, GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,			               
		                            ( LPSECURITY_ATTRIBUTES )NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

		// 打开文件失败处理下一条HID设备信息
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
			CloseHandle(HIDHandleRead);		// 关闭设备句柄
			continue;			
		}

		// 获取产品名称
		memset( name, 0, sizeof( name ) );
		HidD_GetProductString(HIDHandleRead, name, sizeof( name ) );   
	
		memset(szName,0x00,sizeof(szName));
		//printf("pname %s\n", name);
	
		//sprintf_s(szName,81,"%ws",name);
		sprintf(szName,"%ws",name);

		// 检查是否为飞天的 HID ,ROCKEY501设备VID=096E PID=0603
		if((Attribute.VendorID == 0x096E)  && (Attribute.ProductID == 0x603))
		{
			memset(szFTDeviceName,0x00,sizeof(szFTDeviceName));
			//sprintf_s(szFTDeviceName,81,"%s %d",szName,nDeviceNum);
			sprintf(szFTDeviceName,"%s %d",szName,nDeviceNum);

	
			//ROCKEY501设备通讯端口初始化
			PHIDP_PREPARSED_DATA	PreparsedData;
			
			// 获取顶层集合预解析数据
			if( !HidD_GetPreparsedData( HIDHandleRead, &PreparsedData ) )
			{ 
				LocalFree( DetailData );
				CloseHandle(HIDHandleRead);		// 关闭设备句柄
				nRet = R531SYSERR;
				break;
				//return -4;				
			}

			// 获取顶层集合的HIDP_CAPS结构
			HIDP_CAPS				Capabilities;
			if( !HidP_GetCaps( PreparsedData, &Capabilities ) )
			{
				LocalFree( DetailData );
				CloseHandle(HIDHandleRead);		// 关闭设备句柄
				nRet = R531SYSERR;
				//return -5;
			}

			// 获取ROCKEY501接收/发送数据的最大长度
			nDataLen  = Capabilities.FeatureReportByteLength;

			LocalFree( DetailData );

			// 关闭设备句柄,由上层函数去打开设备
			CloseHandle(HIDHandleRead);		

			if( gInitFlag == 0)
			{

				// 保存ROCKEY501-HID设备信息-Add-20120106
				stDeviceStruct[nDeviceNum].nConnStatus = 0;									// 设备已打开未连接
				stDeviceStruct[nDeviceNum].nMaxDataLen = nDataLen;							// 设备数据交换长度

				// HID设备路径名称
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
					stDeviceStruct[gDeviceTotal].nConnStatus = CONNSTATNOCONN;						// 设备已打开未连接
					stDeviceStruct[gDeviceTotal].nMaxDataLen = nDataLen;							// 设备数据交换长度

					// HID设备路径名称
					//strcpy_s(stDeviceStruct[gDeviceTotal].szDevName,127,szFTDevicePath);
					strcpy(stDeviceStruct[gDeviceTotal].szDevName,szFTDevicePath);
					gDeviceTotal ++;					
				}
			}

			// 系统存在R531设备
			nFindFlag = 1;	
			// 判断获取的设备是否达到最大值即10个
			if( nDeviceNum >= MAXDEVICENUM )
			{
				break;
			}
			
		}	// END IF 
		i++;
	}    // End While


	SetupDiDestroyDeviceInfoList( hDevInfo );

	// 检查是否获取到读卡器
	if( nRet >= 0)
	{
		// 设置读卡器环境初始化标志
		gInitFlag = R531INITFLAGOK;

		// 保持读卡器个数
		nRet = gDeviceTotal ;
	}
	
	
	//oCriticalSection.Unlock();
	ReleaseMutex(op_mutex);
	return nRet;
}


// 增加读卡器UID验证
/****************************************************************************************** 
 *******    函数名称：R531ConnDev			-序号：2 -								*******
 *******	函数功能：连接指定USB口的R531设备										*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：char *pDevName:设备名称,值为：USBn[n值为:1~9]					*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：设备序号值范围为:1~9；失败返回错误代码				******* 
 ******************************************************************************************/
int R531ConnDev(char *pDevName)
{	
	int    nDevNo;
	char   szDevPath[256];
	char   szName[4];
	HANDLE hDev;

	memset(szName,0x00,sizeof(szName));

	// 检查传入参数的合法性
	if( strlen(pDevName)!= 4)
	{	// 传入参数长度非法
		return R531PARAMERR;
	}

	// 取前三个字节
	memcpy(szName,pDevName,3);

	// 将字符串转化成大小
	ftStringToUpper(szName);

	// Update：20120708,传入的USB值不区分大小写
	if( memcmp(szName,"USB",3)!= 0)
	{	// 传入参数非法
		return R531PARAMERR;
	}

	if( pDevName[3]< '1' || pDevName[3]>'9')
	{
		return R531PARAMERR;
	}

	// 获取传入参数的设备编号值范围为：1~9
	nDevNo = pDevName[3] -'0';

	// 转化为内部保持的读卡器设备序号
	nDevNo -= 1;

	// 检查是否已进行设备初始化
	if( gInitFlag == R531INITFLAGNO)
	{
		return R531NOINIT;
	}

	// 检查读卡器是否存在
	if( nDevNo >= gDeviceTotal)
	{
		return R531NOEXIST;
	}

	memset(szDevPath,0x00,sizeof(szDevPath));

	// 获取设备路径名称
	//strcpy_s(szDevPath,256,stDeviceStruct[nDevNo].szDevName);
	strcpy(szDevPath,stDeviceStruct[nDevNo].szDevName);

	// 创建设备资源
	hDev = CreateFile( szDevPath, GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,			               
		( LPSECURITY_ATTRIBUTES )NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

	if( hDev == INVALID_HANDLE_VALUE)
	{
		return R531CONNERR;
	}

	// 设置内部结构,相关值
	stDeviceStruct[nDevNo].hDev        = hDev;				
	stDeviceStruct[nDevNo].nConnStatus = CONNSTATCONN;		


	// 设置函数返回值为连接的R531读卡器序号(从1开始,最大值为：9)
	return nDevNo+1;
}


/****************************************************************************************** 
 *******    函数名称：R531CloseDev			-序号：3 -								*******
 *******	函数功能：关闭指定USB口连接的R531设备									*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：int pDevHandle：要关闭的设备句柄值为:1~9]						*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：R531OK,失败返回值见错误代码表						******* 
 ******************************************************************************************/
int R531CloseDev(int pDevHandle)
{	
	int    nDevNo;


	// 转化为内部保持的读卡器设备序号
	nDevNo = pDevHandle - 1;

	// 检查是否已进行设备初始化
	if( gInitFlag == R531INITFLAGNO)
	{
		return R531NOINIT;
	}

	// 检查读卡器是否存在
	if( nDevNo >= gDeviceTotal || nDevNo < 0 )
	{
		return R531NOEXIST;
	}

	// 检查读卡器是否已连接
	if( stDeviceStruct[nDevNo].nConnStatus != CONNSTATCONN)
	{
		return R531UNCONN;
	}

	// 关闭设备
	CloseHandle(stDeviceStruct[nDevNo].hDev );

	// 设置读卡器信息结构数组当前读卡器信息，为未连接的状态
	stDeviceStruct[nDevNo].hDev		     = INVALID_HANDLE_VALUE;
	stDeviceStruct[nDevNo].nConnStatus   = CONNSTATCLOSE;

	// 上电复位标志修改为：RESETSTATNOT(未上电)
	stDeviceStruct[nDevNo].nUserFlag     = RESETSTATNOT;
	stDeviceStruct[nDevNo].nSIM1Flag     = RESETSTATNOT;
	stDeviceStruct[nDevNo].nSIM2Flag     = RESETSTATNOT;
	stDeviceStruct[nDevNo].nSIM3Flag     = RESETSTATNOT;

	// IC卡通信协议修改为：R531PROTOCOLNO(未知)
	stDeviceStruct[nDevNo].nUserProtocol = R531PROTOCOLNO;
	stDeviceStruct[nDevNo].nSIM1Protocol = R531PROTOCOLNO;
	stDeviceStruct[nDevNo].nSIM2Protocol = R531PROTOCOLNO;
	stDeviceStruct[nDevNo].nSIM3Protocol = R531PROTOCOLNO;

	// 读卡器卡槽选择修改为：R531SLOTNOTSET(未选择)
	stDeviceStruct[nDevNo].nCurrSlot     = R531SLOTNOTSET;

	return R531OK;

}


/****************************************************************************************** 
 *******	函数名称：R531DeviceCommand		-序号：4 -								*******
 *******	函数功能：HID-ROCKEY531设备通讯函数										******* 
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******			  int  DevNo:设备序号,从1开始范围为(1~9)						*******
 *******			  char *Send:设备指令数据ASCII[只包括：Cmdcls、CmdSubcls,Data]	*******
 *******			  int   SendLen:设备指令数据长度[即ASCII码的设备指令长度]		*******
 *******			  char *Recv:(输出)设备指令响应数据ASCII[Data]					*******
 *******			  int  *RecvLen:(输出)响应数据ASCII码长度						*******
 *******			  int  *Status:(输出)设备指令应答状态值成功为：0				*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败见错误代码表									*******
 ******************************************************************************************/
int R531DeviceCommand(int DevNo,char *Send,int SendLen,char *Recv,int *RecvLen,int *Status)
{
	char szLRC[2];
	
	time_t lStartTime = 0;						// 接收开始时间；单位为秒
	time_t lTime = 0;							// 当前时间
	time_t lTimeCount = 0;						// 接收总用时；单位为秒
	int  nDevNo;

	char szSend[1024];
	char szHexSend[1024];
	unsigned char szHeader[3];
	unsigned char szTmp[1024];					// 要发送的临时数据
	unsigned char mybuffer[ 1024 ];
	unsigned char szDevSn[2+1];					// 设备序号
	BOOLEAN  Result;
	int nStatus;								// 设备指令应答状态信息
	int nMaxLen;								// 读卡器最大的数据交换长度
	HANDLE HIDHandleRead;
	int  nSendLen;
	int  nPageDataLen ;							// 包数据长度
	int  slen;
	unsigned char  cSeq = 0x00;					// 发送包序号
	int            nSeq = 0;					// 发送包序列号
	int package_size, Pakoffset, TotalLength;
	char *p;
	int  gRecvTime = 60;						// 设置接收应答的最长时间为：30 秒

	memset(szSend,0x00,sizeof(szSend));
	memset(szHexSend,0x00,sizeof(szHexSend));

	memset(szDevSn,0x00,sizeof(szDevSn));

	memset(szLRC,0x00,sizeof(szLRC));
	memset(szHeader,0x00,sizeof(szHeader));

	szDevSn[1] =  0;

	// 将传入的设备序号转换成内部保存的读卡器编号
	nDevNo = DevNo -1; 

	// 检查传入参数的合法性
	if( nDevNo < 0 || nDevNo >= MAXDEVICENUM)
	{
		return R531PARAMERR;
	}

	// 检查传入指令的合法性，指令实际长度是否与函数的指令长度参数一致；ASCII码的指令长度参数是否为2的倍数
	nSendLen  = (int)strlen(Send);
	if( nSendLen != SendLen )
	{
		return R531PARAMERR;	
	}
	if(( SendLen % 2 ) != 0)
	{
		return R531PARAMERR;	
	}

	// 检查运行环境的合法性
	if( nDevNo >= gDeviceTotal)
	{
		return R531UNCONN;
	}

	if( stDeviceStruct[nDevNo].nConnStatus != CONNSTATCONN)
	{
		return R531UNCONN;
	}

	// 根据设备序号从保存的静态结构中获取设备句柄
	HIDHandleRead = stDeviceStruct[nDevNo].hDev;

	// 获取数据交换的最大长度
	nMaxLen = stDeviceStruct[nDevNo].nMaxDataLen;

 	// 清空输入缓冲区
	HidD_FlushQueue( HIDHandleRead );

	// BCD码的指令长度
	nSendLen = SendLen / 2;        
	memset(szHexSend,0x00,sizeof(szHexSend));	

	// 将ASCII码的指令转换成BCD码的指令
	ftAtoh(Send,szHexSend,nSendLen);
 
 
	// 发送的开始位置
	Pakoffset  = 0;		
	

	TotalLength = nSendLen;
	nPageDataLen = nSendLen;
	p = szHexSend;

	// 循环发送请求指令数据到ROCKEY501读卡器
	nSeq = 0;

	// 保存2字节的指令头
	memcpy(szHeader,szHexSend,2);

	int nHeadFlag = 0;
	while( TotalLength > 0 )
	{
		memset(mybuffer,0x00,sizeof(mybuffer));
	
		// 设置当前要发送的数据长度
		if(TotalLength > (nMaxLen - 7))
		{
			package_size = nMaxLen -7;	
			cSeq = 0x80 | nSeq;							// 设置包序号
		}else 
		{
			if( nHeadFlag == 1)
				nHeadFlag = 2;							// 发送包为结束包
			package_size = TotalLength;
			cSeq = nSeq;								// 设置包序号
		}

		memset(szTmp,0x00,sizeof(szTmp));
		 
		mybuffer[0] = (unsigned char)0x00;				// 协议报文前导字符，	值为：0x00;
		mybuffer[1] = (unsigned char)0x02;				// Stx:1字节[数据帧起始头]	值为：0x02

		if( nHeadFlag == 0)
 			mybuffer[2] = (unsigned char)(package_size+3);	// Len:1字节,从Fnum 到 Lrc 之前的数据长度
		else if( nHeadFlag == 1)
			mybuffer[2] = (unsigned char)(package_size+3);
		else 
			mybuffer[2] = (unsigned char)(package_size+5);
			
		mybuffer[3] = (unsigned char)cSeq;				// Fnum:1字节包编号                

		mybuffer[4] = (unsigned char)szDevSn[0];		// DevSn:设备序号，2字节
		mybuffer[5] = (unsigned char)szDevSn[1];
		
		if( nHeadFlag == 0 )							// 第1个包
		{
			memcpy(mybuffer+6,p,package_size);			// 添加报文的CmdCls和CmdSubCls值
		}else if ( nHeadFlag == 1)						// 中间数据包
		{
			memcpy(mybuffer+6,szHeader,2);
			memcpy(mybuffer+8,p,package_size-2);
		}else											// 最后一个数据包 
		{
			memcpy(mybuffer+6,szHeader,2);	
			memcpy(mybuffer+8,p,package_size);
			package_size+=2;
		}
		

		// 计算BCD码发送数据的LRC值并添加到缓冲后面
		ftCalLRC1((char *)mybuffer+2,package_size+4,szLRC,1); 
		
		// 将LRC添加到报文后面
		memcpy(mybuffer+ package_size+6,szLRC,1);
	 

		// 发送数据,注：函数第三个参数，为HidP_GetCap函数返回HIPD_CAPS 结构成员变量值
		Result =	HidD_SetFeature( HIDHandleRead, mybuffer, nMaxLen );
		if(!Result)
		{

			return R531SYSERR;
		}

		HidD_FlushQueue( HIDHandleRead );

		if( nHeadFlag == 0)
		{	
			TotalLength -= package_size;					// 数据剩余长度
			p += package_size;								// 下次发送数据指针
		}else 
		{
			TotalLength =TotalLength - (package_size -2) ;	// 数据剩余长度
			package_size -=2;
			p += package_size;
		}
		
		nSeq += 1;											// 多包发送的序号
		nHeadFlag = 1;										// 表示后面的包需要增加2字节指令头

	}


 	memset(szHexSend,0x00,sizeof(szHexSend));
	memset(szSend,0x00,sizeof(szSend));

	// 接收第一个应答报文,如果应答的数据长度为：0，该包丢弃，重新接收
	Pakoffset = 0;											// 接收的应答数据位置偏移量
	TotalLength = 0;										// BCD码的应答数据总长度
	slen = 0;
	nSeq = 0;												// 接收包序号
	HidD_FlushQueue( HIDHandleRead );

	// 单位为毫秒
	DWORD dwStartTime;
	DWORD dwCurrTime;
	DWORD dwRunTime;

	// 获取开始时间
	dwStartTime = GetTickCount();

	while( 1 )
	{
		memset(mybuffer,0x00,sizeof(mybuffer));

		// 接收设备应答数据
		Result = HidD_GetFeature( HIDHandleRead, mybuffer, nMaxLen );
		if( !Result)
		{

			return R531SYSERR;
		}
		
		// 接收应答成功的处理
		slen = (int)mybuffer[2];		// 从Fnum 到 Lrc 之前的数据长度
		slen -=4;						// 设备指令应答的：状态码(1) + 应答信息的长度；
		
		nStatus = (int)mybuffer[6];		// 设备指令响应状态信息
			
		// Add:20110714 ,如果应答状态为：0x88 即：忙则继续接收
		if( nStatus == 0x88 )
		{
			
			// 获取当前时间
			dwCurrTime = GetTickCount();

			// 计算运行时间(单位为毫秒)
			dwRunTime = dwCurrTime - dwStartTime;

			// 检查是否超时gRecvTime值为秒
			if( (int)dwRunTime > gRecvTime*1000)
			{
				return R531SYSERR;
			}

			// 暂停 20 毫秒，等待数据到达
			Sleep(20);

			// 继续执行循环接收应答数据
			continue;


			/* Del - 20120716
			// 增加接超时时间检查
			if ( lStartTime == 0)		// 获取开始时间,单位为：秒
			{
				time(&lStartTime);
				continue;
			}else {						// 获取当前时间,并计算接收当前使用的时间
				time(&lTime);
			}

			// 计算当前接收使用的时间，单位为：秒
			lTimeCount = (long)lTime - lStartTime;

			// 检查是否接收超时
			if(lTimeCount >= gRecvTime)
			{	// 接收超时
				return R531SYSERR;
			}			
			
			// 增加等待20毫秒  -- 原值
			// UPDATE :20120528 ,---减少上电复位的处理时间
			Sleep(20);

			continue;
			*/
		}
	

		TotalLength += slen;

		// 保存状态码 + 应答信息 
		memcpy(szHexSend + Pakoffset,mybuffer+7,slen);

		// 取Fnum[数据帧编号],
		cSeq = mybuffer[3];
		
		Pakoffset += slen;

		// 无后续包，直接返回
		if(cSeq == 0x00 )
		{
			break;
		}
	}	


	// Add - 20120708 检测指令的状态码是否正确，不正确则函数返回报错；
	if( nStatus != 0)
	{
		*Status		= nStatus;

		// 应答错误
		return R531SYSERR;
	}

	
	memset(szSend,0x00,sizeof(szSend));
	ftHtoa(szHexSend,szSend,TotalLength);

 
	// 设置函数输出变量值
	TotalLength = TotalLength*2;
	*RecvLen	= TotalLength;
	*Status		= nStatus;

	memcpy(Recv,szSend,TotalLength);

	return R531OK;
}



//==================================================================================================
//2-设备类函数封装
//==================================================================================================
/******************************************************************************************
 *******	函数名称：R531DevSeqNo					-序号：5						*******
 *******	函数功能：获取ROCKEY531设备产品序列号,指令[8000]						*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：                      										*******
 *******			  int hDev：设备句柄，值范围为：1~9								*******
 *******			  char *OutBuf：(输出)产品序列号,格式如下：		    			*******
 *******			            8字节的年月日，如：20110708							*******
 *******			            6字节的时分秒，如：153030							*******
 *******			            8字节的机器号，如：00000008							*******
 *******			            6字节的流水号，如：0x010203,输出："010203"			*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0	;错误代码如下:						*******
 *******					-701:未加载ROCKEY501设备指令状态信息					*******
 *******					-702:状态信息不存在										*******
 ******************************************************************************************/
int R531DevSeqNo(int hDev,char *OutBuf,int *Status)
{
/*
	char szDate[8+1];
	char szTime[6+1];
	char szMachineNo[8+1];		// 机器号
	char szDeviceNo[8+1];		// 流水号
	int  nDeviceNo[3];			// 流水号

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

	// 设置初始化值，则失败输出变量应答数据为：NULL
	memcpy(OutBuf,"\x00",1);

//	memset(szMachineNo,0x00,sizeof(szMachineNo));

	// 设置读取产品序列号指令
	memset(szSend,0x00,sizeof(szSend));
	//strcpy_s(szSend,4,"8000");
	//nSendLen = (int)strlen(szSend);

	memcpy(szSend,"8000",4);
	nSendLen = 4;

	memset(szRecv,0x00,sizeof(szRecv));
	nRecvLen = 0;
	nStatus  = 0;

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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

	// 检查设备指令的应答数据是否正确
	if( nRecvLen != 16 || strlen(szRecv) != 16)
	{
		return R531RESPDATALENERR;
	}
	
	memcpy(OutBuf, szRecv, nRecvLen);
	OutBuf[nRecvLen] = 0;
#ifdef MANA
	memset(szHexRecv,0x00,sizeof(szHexRecv));
	
	// 转换成BCD码
	ftAtoh(szRecv,szHexRecv,nRecvLen/2);
	
	memset(szBitString,0x00,sizeof(szBitString));

	// 将BCD码值转换成BIT字数组
	ftDataToBitString(szHexRecv,nRecvLen/2,szBitString);

	memset(szDate,0x00,sizeof(szDate));
	nPos = 0;

	// 转换年 6bit
	ftBitStringToVal(szBitString,6,&nVal);
	nVal += 2004;
	
	//sprintf_s(szDate,5,"%04.4d",nVal);
	sprintf(szDate,"%04.4d",nVal);

	nPos+=6;
	
	// 转换月 4bit
	ftBitStringToVal(szBitString+nPos,4,&nVal);
	//sprintf_s(szDate+4,3,"%02.2d",nVal);
	sprintf(szDate+4,"%02.2d",nVal);
	nPos+=4;

	//转换日 5bit
	ftBitStringToVal(szBitString+nPos,5,&nVal);
	//sprintf_s(szDate+6,3,"%02.2d",nVal);
	sprintf_s(szDate+6,"%02.2d",nVal);
	nPos+=5;

	memset(szTime,0x00,sizeof(szTime));
	// 转换时 5bit
	ftBitStringToVal(szBitString+nPos,5,&nVal);
	sprintf_s(szTime,3,"%02.2d",nVal);
	nPos+=5;
 
	// 转换分 6bit
	ftBitStringToVal(szBitString+nPos,6,&nVal);
	sprintf_s(szTime+2,3,"%02.2d",nVal);
	nPos+=6;

	// 转换秒 6bit
	ftBitStringToVal(szBitString+nPos,6,&nVal);
	sprintf_s(szTime+4,3,"%02.2d",nVal);
	nPos+=6;

	memset(szDeviceNo,0x00,sizeof(szDeviceNo));
	// 机器序号 8 bit
	ftBitStringToVal(szBitString+nPos,8,&nVal);
	sprintf_s(szMachineNo,9,"%08.8d",nVal);
	nPos+=8;

	// 流水号 -第1位
	memset(szSeq,0x00,sizeof(szSeq));
	ftBitStringToVal(szBitString+nPos,8,&nVal);
	nDeviceNo[0] = nVal;
	nPos+=8;

	// 流水号 -第2位
	memset(szSeq,0x00,sizeof(szSeq));
	ftBitStringToVal(szBitString+nPos,8,&nVal);
	nDeviceNo[1] = nVal;
	nPos+=8;

	// 流水号 -第3位
	memset(szSeq,0x00,sizeof(szSeq));
	ftBitStringToVal(szBitString+nPos,8,&nVal);
	
	nDeviceNo[2] = nVal;
	nPos+=8;

	// 处理流水号
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

	// 高位大于100 直接丢弃

	nDeviceNo[0] = nDeviceNo[0] % 100;

	memset(szDeviceNo,0x00,sizeof(szDeviceNo));

	sprintf_s(szDeviceNo,9,"%02.2d%02.2d%02.2d",
				nDeviceNo[0],
				nDeviceNo[1],
				nDeviceNo[2]);


	memset(szRecv,0x00,sizeof(szRecv));

	sprintf_s(szRecv,1024,"%08.8s%06.6s%08.8s%06.6s",
				szDate,				// 8 字节年
				szTime,				// 6 字节月
				szMachineNo,		// 8 字节机器序号
				szDeviceNo			// 6 字节流水号
				);			
	nRecvLen = 8+6+8+6;
	memcpy(OutBuf,szRecv,nRecvLen);
	memcpy(OutBuf+nRecvLen,"\x00",1);

#endif
	return R531OK;
}


/******************************************************************************************
 *******	函数名称：R531DevBeep		-序号：6									*******
 *******	函数功能：设备蜂鸣器,指令[8001]											*******
 *******              蜂鸣顺序为先鸣后停；循环次数为:0,如果蜂鸣时间为：0，则关闭	*******
 *******              时间不为:0,则一直蜂鸣；										*******
 *******			  循环次数1~FF 时，蜂鸣按蜂鸣时间和蜂鸣间歇循环					*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：unsigned char CycleNum：循环次数;								*******
 *******			  unsigned char Times   ：蜂鸣时间;								*******
 *******			  unsigned char Interval：蜂鸣间歇								*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
  *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0	;错误代码如下:						*******
 *******					-701:未加载ROCKEY501设备指令状态信息					*******
 *******					-702:状态信息不存在										*******
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

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
 *******	函数名称：R531DevResetHW	-序号：7									*******
 *******	函数功能：射频模块硬复位,指令[8002]										*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：unsigned char Msec:复位时间(1~FF)								*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0	;错误代码如下:						*******
 *******					-701:未加载ROCKEY501设备指令状态信息					*******
 *******					-702:状态信息不存在										*******
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

	// 蜂鸣 指令 8001 循环次数(1字节) 蜂鸣时间(1字节) 蜂鸣间歇
	//sprintf_s(szSend,1024,"8002%2.2X",Msec);
	sprintf(szSend,"8002%2.2X",Msec);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
 *******	函数名称：R531DevRField	-序号：8										*******
 *******	函数功能：射频场开关函数,指令[8003]										*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：unsigned char Mode:操作方式，可以值如下：						*******
 *******						0x00：关闭，间隔无效；								*******
 *******						0x01：先关后开，间隔为：Interval					*******
 *******						0x02：先开后关，间隔为：Interval					*******
 *******						0x03：打开；间隔值无效；							*******
 *******			  unsigned char Interval：间隔时间;								*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0	;错误代码如下:						*******
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

	// 检查传入的操作方式是否有效
	if( Mode > 0x03 )
	{
		return R531PARAMERR;
	}

	// 8003:射频场开关
	//sprintf_s(szSend,1024,"8003%02.2X%02.2X",Mode,Inteval);
	sprintf(szSend,"8003%02.2X%02.2X",Mode,Inteval);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
 *******	函数名称：R531DevSetRfMode	-序号：9									*******
 *******	函数功能：非接工作模式设置函数,指令[8004]								*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：unsigned char Mode:操作方式，可以值如下：(默认值为：0x0A)		*******
 *******						0x0A：射频卡工作模式为：TypeA,M1系统卡也属此模式；	*******
 *******						0x0B：先关后开，间隔为：Interval					*******
 *******			  unsigned char Speed：卡速率，Mode= 0x0B[TypeB]使用,值如下：	*******
 *******						0x00:106KB；										*******
 *******						0x01:212KB;											*******
 *******						0x02:424KB;											*******
 *******						0x03:848KB;											*******
 *******			  该自己高4位表示：CwConductance寄存器调整值；范围：0~0x0F		*******
 *******			  当工作模式为TypeB，第4字节为：CwConductance调整值范围0~0x0F	*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0	;错误代码如下:						*******
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

	// 检查传入的操作方式是否有效
	if( !(Mode == 0x0A  || Mode == 0x0B ))
	{
		return R531PARAMERR;
	}

	// 检查传入的第二个参数的合法性
	if( Mode == 0x0A)				// TypeA 时第2个参数值为：0x00、0x01、0x02、0x03
	{
		if( Speed < 0x00 || Speed > 0x03)
		{
			return R531PARAMERR;
		}
	}else							// TypeB 时第2个参数值为：0x00 ~ 0x0F
	{
		if( Speed < 0x00 || Speed > 0x0F)
		{
			return R531PARAMERR;
		}
	}

	// 8004:非接工作模式设置
	//sprintf_s(szSend,1024,"8004%02.2X%02.2X",Mode,Speed);
	sprintf(szSend,"8004%02.2X%02.2X",Mode,Speed);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
 *******	函数名称：R531DevGetRfMode	-序号：10									*******
 *******	函数功能：获取非接工作模式函数,指令[8005]								*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：int* Mode:(输出)：0x0A-TypeA；0x0B - TypeB					*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0	;错误代码如下:						*******
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

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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


	// 将应答的模式值转换成整型 0x0A --> 10 ; 0x0B --> 11 ;
	*Mode = (int)ftHexToLong(szRecv);

	return R531OK;
}

// 8006:写射频芯片EEPROM  暂不支持；
// 8007:读射频芯片EEPROM  暂不支持；
// 8008:写射频芯片寄存器  暂不支持；
// 8009:读射频芯片寄存器  暂不支持； 


/******************************************************************************************
 *******	函数名称：R531DevCheckUserCard	-序号：11								*******
 *******	函数功能：检查用户卡卡槽是否有卡及卡是否上电,指令[800A]					*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：int *SlotStatus：0：无用户卡；1.用户卡未上电；2：用户已上电	*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			******* 
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0	;错误代码如下:						*******
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

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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


	// 应答数据错误，格式为：800A XX :长度为6字节的ASCII码
	if(nRecvLen < 6) 
	{
		return R531RESPDATALENERR;
	}
	
	// 处理应答,应答数据
	memset(szHexRecv,0x00,sizeof(szHexRecv));
	memset(szBitString,0x00,sizeof(szBitString));

	// 将应答ASCII字符串中STA转换成BCD码
	ftAtoh(szRecv+4,szHexRecv,1);

	ftCharToBitString((unsigned char)szHexRecv[0],szBitString);

	
	// 根据STA高两字节的值设置用户卡卡槽状态
	if( szBitString[0] == '0')
	{
		*SlotStatus = R531CARDSTATNO;					// 无用户卡
	}
	else 
	{
		if(szBitString[1] == '0')
		{
			*SlotStatus = R531CARDSTATNOPOWER;			// 用户卡未上电
		}else 
		{
			*SlotStatus = R531CARDSTATNORMAL;			// 用户卡已上电
		}
	}
	
	return R531OK;
}




/******************************************************************************************
 *******	函数名称：R531DevRedLightCtl	-序号：12								*******
 *******	函数功能：ROCKEY531读卡器红灯控制,指令[800B]							*******
 *******			  灯闪烁顺序为先亮后灭;											*******
 *******			  循环次数为0时，如果灯亮时间为0，则灯灭而不管灯灭时间值		*******
 *******			  如果灯亮时间不为零，则灯一直亮。循环次数1～FF时，				*******
 *******                灯按亮灭时间循环；											*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：unsigned char CycleNum：循环次数								*******
 *******              unsigned char LightTime：灯亮时间								*******
 *******              unsigned char OffTime：灯灭时间								*******
 *******              int *Status：(输出)指令应答状态，成功返回：0；				*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0	;错误代码如下:						*******
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

	// 红灯控制指令800B + 1字节循环次数 + 1字节灯亮时间 + 1字节灯灭时间
	//sprintf_s(szSend,1024,"800B%02.2X%02.2X%02.2X",CycleNum,LightTime,OffTime);
	sprintf(szSend,"800B%02.2X%02.2X%02.2X",CycleNum,LightTime,OffTime);
	nSendLen = (int)strlen(szSend);
	

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
 *******	函数名称：R531DevSetRFTimeOut	-序号：12								*******
 *******	函数功能：ROCKEY531读卡器射频卡超时时间定义,指令[8011]					*******
 *******			  可以定义非标射频卡读写操作超时时间							*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：unsigned char TimeOut：超时时间,参数值如下：					*******
 *******                 0->0.3；1->0.6；2->1.2；3->2.5；4->5；5->10；6->20；7->40	*******
 *******			     8->80； 9->160；A->320; B->620; C->1300;D->2500;E->5000	*******
 *******			     超时时间单位为毫秒											*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0	;错误代码如下:						*******
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

	// 检查传入的超时时间
	nTimeOut = TimeOut;
	if(nTimeOut > 14 )
	{
		nTimeOut = 14;
	}

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	// 红灯控制指令8011 + 1字节超时时间
 	//sprintf_s(szSend,1024,"8011%02.2X",TimeOut);
 	sprintf(szSend,"8011%02.2X",TimeOut);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
 *******	函数名称：R531DevForceQuit	-序号：13									*******
 *******	函数功能：终止当前读卡器执行的指令,指令[801F]							*******
 *******			  可用强制结束：读写卡、灯闪烁、蜂鸣这三类指令					*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：  															*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0	;错误代码如下:						*******
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

	//设置执行的指令
	//sprintf_s(szSend,1024,"801F");
	sprintf(szSend,"801F");
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
 *******	函数名称：R531DevGenUID	-序号：14										*******
 *******	函数功能：生成用户UID,指令[800C]										*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：int DataLen:种子数据长度，值范围为：0x00~0x30					*******
 *******			  unsigned char *Data:Len字节的种子数据,种子数据格式如下：		*******
 *******					0x00 + LEN + DATA_key; 种子数据为：ASCII码				*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******					成功返回：00x00;失败返回：0x01;不支持返回：0x02			*******
 *******                    UID已存在返回：0x03										*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0	;错误代码如下:						*******
 ******************************************************************************************/
int R531DevGenUID(int hDev,int DataLen,unsigned char *Data ,int *Status )
{
	char szSend[1024];
	char szRecv[1024];
	char szASCIIData[96+1];			// 种子数据长度为：0~0x30 ;
	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;

	// 检查长度是否合法
	if(DataLen <=0 || DataLen > 48)
	{
		return R531PARAMERR;
	}

	// 处理传入数据
	memset(szASCIIData,0x00,sizeof(szASCIIData));

	// 将一个字节转换成2个字节
	ftHtoa((char *)Data,szASCIIData,DataLen);


	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	

	//sprintf_s(szSend,1024,"800C001031323334353637383930313233343536");
	
	//设置执行的指令
	//sprintf_s(szSend,1024,"800C00%02.2X%s", DataLen, szASCIIData);
	sprintf(szSend,"800C00%02.2X%s", DataLen, szASCIIData);

	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
 *******	函数名称：R531DevGetUID	-序号：15										*******
 *******	函数功能：读取用户UID,指令[800C]										*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：int* DataLen:(输出)种子数据长度，值范围为：0x00~0x30			*******
 *******			  unsigned char *Data:(输出)Len字节的种子数据					*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******					成功返回：00x00;失败返回：0x01;不支持返回：0x02			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0	;错误代码如下:						*******
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

	//设置执行的指令
	//sprintf_s(szSend,1024,"800C01");
	sprintf(szSend,"800C01");
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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

	
	// 检查获取的种子数据是否存在
	if(nRecvLen <= 0)
	{
		return R531RESPDATALENERR;
	}

	// 设置函数应答的长度变量
	*DataLen = nRecvLen;

	// 设置函数输出变量的种子数据
	memcpy(Data,szRecv,nRecvLen);
	memcpy(Data+nRecvLen,"\x00",1);

	return R531OK;
}



/******************************************************************************************
 *******	函数名称：R531DevModUID	-序号：16										*******
 *******	函数功能：重新生成用户UID,指令[800C]									*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：int DataLen:种子数据长度，值范围为：0x00~0x30					*******
 *******			  unsigned char *Data:Len字节的种子数据,种子数据格式如下：		*******
 *******					0x00 + LEN + DATA_key; 种子数据为：ASCII码				*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******					成功返回：00x00;失败返回：0x01;不支持返回：0x02			*******
 *******                    UID已存在返回：0x03										*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0	;错误代码如下:						*******
 ******************************************************************************************/
int R531DevModUID(int hDev,int DataLen,unsigned char *Data ,int *Status )
{
	char szSend[1024];
	char szRecv[1024];
	char szASCIIData[96+1];			// 种子数据长度为：0~0x30 ;
	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;



	// 检查长度是否合法
	if(DataLen <=0 || DataLen > 48)
	{
		return R531PARAMERR;
	}

	// 处理传入数据
	memset(szASCIIData,0x00,sizeof(szASCIIData));

	// 将一个字节转换成2个字节
	ftHtoa((char *)Data,szASCIIData,DataLen);


	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));

	//设置执行的指令
	//sprintf_s(szSend,1024,"800C02%02.2X%s", DataLen, szASCIIData);
	sprintf(szSend,"800C02%02.2X%s", DataLen, szASCIIData);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
 *******	函数名称：R531DevModPara	-序号：17									*******
 *******	函数功能：修改读卡器参数,指令[8010]										*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：int ParaLen:参数字节长度,	10字节=2*10							*******
 *******			  unsigned char *Data:参数字节，说明如下：(没个参数占两个字符)	*******
 *******		   序号	参数标识		参数描述			值实例					*******
 *******           ===============================================================	*******
 *******           1	ATR_Band_User	用户卡槽ATR波特率   0x00标识按正常7816进行	******* 
 *******		   2    ATR_Band_Sam1   SAM1卡槽			(粤通卡设置成：13)		*******
 *******		   3    ATR_Band_Sam2												*******
 *******		   4	ATR_Band_Sam3												*******
 *******		   5    WKT								     (粤通卡设置成：0x10)	*******
 *******		   6	RFU									 (保留值为：00)			*******
 *******		   7	RFU															*******
 *******		   8	RFU															*******
 *******		   9	RFU															*******
 *******		   10	RFU															*******		
 *******		   ---------------------------------------------------------------	*******
 *******			如果设置成：0x00,表示按正常的7816进行，如果为其他值，则始终按	*******
 *******				值定义的恒定的波特率通讯，包括ATR和APDU过程；				*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0	;错误代码如下:						*******
 ******************************************************************************************/
// 粤通卡非标卡，读卡器参数设置指令实例：8010 13 11 11 11 01 00 00 00 00 00  [10字节，20个ASCII码],用户卡槽非标，其他标准
// 参数说明：ATR_Baud_user 参数设置成：0x13; SKT 设置成：0x10				
int R531DevModPara(int hDev,int ParaLen, char *Para,int *Status )
{
	char szSend[1024];
	char szRecv[1024];
	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nRet;

	// 检查传入的长度参数值是否正确
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

	//设置执行的指令
	//sprintf_s(szSend,1024,"8010%s",Para);
	sprintf(szSend,"8010%s",Para);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
 *******	函数名称：R531DevRecoverPara	-序号：18								*******
 *******	函数功能：恢复读卡器参数,指令[8015]										*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：                                  							******* 
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0	;错误代码如下:						*******
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

	//设置执行的指令
	//sprintf_s(szSend,1024,"8015");
	sprintf(szSend,"8015");
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK )
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
//3-接触卡类函数封装
//==================================================================================================
/******************************************************************************************
 *******	函数名称：R531CpuSetSlot	-序号：19									*******
 *******	函数功能：选择卡槽,指令[B001]											*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：int Slot:卡槽编号；0:用户卡槽；1~3：SAM卡槽					******* 
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0	;错误代码如下:						*******
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

	// 将设备句柄转换成内部读卡器数组编号；
	nDevNo -= 1;

	// 检查句柄合法性
	if( nDevNo < 0 || nDevNo > R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDevNo >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查卡槽参数正确性
	if(Slot <0 || Slot >3)
	{
		return R531SLOTERR;
	}

	//设置执行的指令
	//sprintf_s(szSend,1024,"B001%02.2X",Slot);
	sprintf(szSend,"B001%02.2X",Slot);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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

	// Add-20120706 -- 设置读卡器结构中当前读卡器的卡槽选择状态，及当前选择的卡槽编号
	stDeviceStruct[nDevNo].nCurrSlot = Slot;

	return R531OK;
}


/******************************************************************************************
 *******	函数名称：R531CpuReset	-序号：20										*******
 *******	函数功能：IC卡上电复位,指令[B002]										*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：int *AtrLen:(输出)复位应答信息长度[ASCII]						*******
 *******			  char *Atr： (输出)复位应答信息[ASCII]							*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0	;错误代码如下:						*******
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
	int nVoltage = 0;		// 电压直接设置成：0 - 即5V

	ATRStruct stAtr;		// ATR结构变量
	int   nTD1Protocol;		// TD1定义的通信协议，合法值为：0和1；
	int   nTD2Protocol;		// TD2定义的通信协议，合法值为：15 

	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));
	memset(szATRLen,0x00,sizeof(szATRLen));
	memset(szHexAtrLen,0x00,sizeof(szHexAtrLen));

	// 运行环境检查

	nDev = hDev -1;
	
	// 监测句柄合法性
	if( nDev < 0 || nDev >= MAXDEVICENUM)
	{
		// 传入的读卡器句柄错误
		return R531PARAMERR;
	}

	
	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}


	// 检查是否已设置接触式IC卡指令发送的卡槽；
	if( stDeviceStruct[nDev].nCurrSlot == R531SLOTNOTSET)
	{
		// 未选择接触卡的卡槽
		return R531NOSETSLOT;
	}

	nCurrSlot = stDeviceStruct[nDev].nCurrSlot;

	//设置执行的指令
	//sprintf_s(szSend,1024,"B0028%01.1X",nVoltage);
	sprintf(szSend,"B0028%01.1X",nVoltage);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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

	// 处理应答数据
	// 获取上电复位信息的2字节长度,并转化成整型s
	memcpy(szATRLen,szRecv,4);
	ftAtoh(szATRLen,szHexAtrLen,2);

	// 将长度转换成10进制的整型值
	nATRLen = (unsigned int)szHexAtrLen[0] * 256 + (unsigned int) szHexAtrLen[1];

	
	// 设置函数输出变量的ATR长度
	*AtrLen = nATRLen * 2;

	// 设置函数输出变量的ATR值
	memcpy(Atr,szRecv+4,nATRLen*2);
	memcpy(Atr+(nATRLen*2),"\x00",1);

	// Add-2012-07-06,根据复位应答信息,设备读卡器结构中当前读卡器的上电复位标志和IC卡通信协议类型
	nHexAtrLen = nATRLen;


	memset(szHexAtr,0x00,sizeof(szHexAtr));

	// 将ASCII码的ATR转化成BCD码
	ftAtoh((char *)szRecv+4,szHexAtr,nHexAtrLen);

	// 初始化ATR结构变量
	memset((char *)&stAtr,0x00,sizeof(ATRStruct));

	// 将BCD码的ATR转化到ATR结构中
	nRet = ftATRInitFromArray(&stAtr,(unsigned char *)szHexAtr,(unsigned int)nHexAtrLen);
	if( nRet != R531OK)
	{		
		return R531RESETERR;
	}	
 		
	ftGetATRT(stAtr,&nTD1Protocol,&nTD2Protocol);
 
	// 上电复位成功，修改读卡器数据结构数组中当前读卡器,的上电复位标志值及通信协议
	switch(nCurrSlot)
	{
		// 用户卡模块,
		case R531SLOTUSER:
			stDeviceStruct[nDev].nUserFlag = RESETSTATON;
			stDeviceStruct[nDev].nUserProtocol = nTD1Protocol;
			break;


		// SIM1模块
		case R531SLOTSAM1:
			stDeviceStruct[nDev].nSIM1Flag	   = RESETSTATON;
			stDeviceStruct[nDev].nSIM1Protocol = nTD1Protocol;
			break;

		// SIM2模块
		case R531SLOTSAM2:
			stDeviceStruct[nDev].nSIM2Flag     = RESETSTATON;
			stDeviceStruct[nDev].nSIM2Protocol = nTD1Protocol;
			break;

		// SIM3模块
		case R531SLOTSAM3:
			stDeviceStruct[nDev].nSIM3Flag     = RESETSTATON;
			stDeviceStruct[nDev].nSIM3Protocol = nTD1Protocol;
			break;
	}
	

	// IC卡通信协议为：T=0 直接返回
	if( nTD1Protocol == R531PROTOCOLT0)
	{
		return R531OK;	
	}

	// 如果TD1的通信协议为：T=1,则进行PPS设置,--
	
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

	// 将ASCII码的计算数据转换BCD码的计算数据
	ftAtoh(szPPS,(char *)szHexPPS,3);

	ftCalPCK(szHexPPS,3,&nPCK);

	//sprintf_s(szPCK,3,"%02.2X",nPCK);
	sprintf(szPCK,"%02.2X",nPCK);


	// 调用：ftCalPCK(szPPS,3,&nPCK);
		
	// 将计算的PCK添加到字符串的PPS后
	//strcat_s(szPPS,21,szPCK);
	strcat(szPPS,szPCK);

	// 发送PPS指令到读卡器
	nRet = R531CpuPPS(hDev,4,szPPS,szResponse,&nStatus);
	if( nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
 *******	函数名称：R531CpuPowerOff	-序号：21									*******
 *******	函数功能：IC卡下电,指令[B002]											*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；													*******
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
	
	// 监测句柄合法性
	if( nDev < 0 || nDev >= MAXDEVICENUM)
	{
		// 传入的读卡器句柄错误
		return R531PARAMERR;
	}

	
	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}


	// 检查是否已设置接触式IC卡指令发送的卡槽；
	if( stDeviceStruct[nDev].nCurrSlot == R531SLOTNOTSET)
	{
		// 未选择接触卡的卡槽
		return R531NOSETSLOT;
	}

	nCurrSlot = stDeviceStruct[nDev].nCurrSlot;


	//设置执行的指令
	//strncpy_s(szSend,1024,"B00200",6);
	strncpy(szSend,"B00200",6);

	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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


	// 修改读卡器结构数组中当前模块的上电复位标志值,为：已下电
	switch(nCurrSlot)
	{
		// 用户卡模块,
		case R531SLOTUSER:
			stDeviceStruct[nDev].nUserFlag     = RESETSTATOFF;
			break;

		// SIM1模块
		case R531SLOTSAM1:
			stDeviceStruct[nDev].nSIM1Flag	   = RESETSTATOFF;
				break;

		// SIM2模块
		case R531SLOTSAM2:
			stDeviceStruct[nDev].nSIM2Flag     = RESETSTATOFF;
			break;

		// SIM3模块
		case R531SLOTSAM3:
			stDeviceStruct[nDev].nSIM3Flag     = RESETSTATOFF;
			break;
	}

	return R531OK;
}



/******************************************************************************************
 *******	函数名称：R531CpuPPS	-序号：22										*******
 *******	函数功能：PPS,指令[B010]												*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******			  int PPSLen	：PPS长度										*******
 *******			  char *PPS		：PPS数据，如：ff 11 00 ee(API固定为4字节)		*******
 *******								[0]：固定值为:FF							*******
 *******								[1]：高半字节固定为：1，低半字节为协议类型	*******
 *******								[2]：ATR中TA1的值							*******
 *******								[3]：0~2 字节的奇偶校验值					*******
 *******			  int *Status	：(可选、输出)指令执行状态,成功返回：0；		*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；													*******
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

	// Add - 20120709 -- 增加参数及运行环境检查
	nDevNo = hDev -1;


	// 检查句柄合法性
	if( nDevNo < 0 || nDevNo > R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDevNo >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDevNo].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}


	// 检查是否已设置接触式IC卡指令发送的卡槽；
	if( stDeviceStruct[nDevNo].nCurrSlot == R531SLOTNOTSET)
	{
		// 未选择接触卡的卡槽
		return R531NOSETSLOT;
	}


	//sprintf_s(szSend,1024,"B010%02.2X%s",PPSLen,PPS);
	sprintf(szSend,"B010%02.2X%s",PPSLen,PPS);
	nSendLen = (int)strlen(szSend);

	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
	// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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

	
	// Add- 20120709 设置应答数据
	memcpy(Recv,szRecv,nRecvLen);


	return R531OK;
}



/******************************************************************************************
 *******	函数名称：R531CpuAPDU	-序号：23										*******
 *******	函数功能：执行IC卡的APDU,指令[B011]										*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******              int hDev		：读卡器序号，设备序号，1~9 [第1个为1]			*******
 *******			  int SendLen	：发送的IC卡指令长度(ASCII长度)					*******
 *******			  char *Send	：发送的IC卡指令(ASCII码)						*******
 *******			  int *RecvLen	：(输出)应答数据长度(ASCII长度)					*******
 *******			  char *Recv	：(输出)应答数据(ASCII码)						*******
 *******			  int *Status	：(可选、输出)指令执行状态,成功返回：0；		*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；													*******
 ******************************************************************************************/
int R531CpuAPDU(int hDev,int SendLen,char *Send,int *RecvLen,char *Recv,int *Status )
{
	char szSend[1024];				
	char szRecv[1024];
	char szTmp[1024];						// T=1 临时数据计算
	char szHexLRC[2];						// T=1 LRC值
//	char szLRC[2+1];						// ASCII码的LRC值


	int nSendLen;
	int nRecvLen;
	int nStatus;
	int nDev;
	int nRet;
	int nProtocolType;						// 卡槽IC卡协议类型
	int nHLen,nLLen;						// 指令长度高、低字节值
	int nPHLen,nPLLen;						// 
	int nTmpLen;
	int nSlotNo;
	int nResetFlag;							// 读卡器模块上电复位标志

	nTmpLen = 0;
	memset(szSend,0x00,sizeof(szSend));
	memset(szRecv,0x00,sizeof(szRecv));



	// Add - 20120710 增加运行环境检查
	
	// 将读卡器句柄转换成内部读卡器信息结构保持序号(从0开始)
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}

	// 检查是否已设置接触式IC卡指令发送的卡槽；
	if( stDeviceStruct[nDev].nCurrSlot == R531SLOTNOTSET)
	{
		// 未选择接触卡的卡槽
		return R531NOSETSLOT;
	}

	// END Add 
	
	// 检查传入的IC卡指令的合法性
	if( SendLen % 2 != 0)
	{
		return R531PARAMERR;
	}

	if( SendLen != (int)strlen(Send) )
	{
		return R531PARAMERR;
	}

	// 处理指令长度长度
	nHLen =  (SendLen / 2) /256 ;			// 高字节
	nLLen = (SendLen  / 2) %256 ;			// 低字节

	
	// 获取读卡器当前操作的模块；
	nSlotNo = stDeviceStruct[hDev-1].nCurrSlot;


	nResetFlag =  RESETSTATNOT;

	// 获取模块IC卡通信协议类型
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

	// Add -20120710 ,检查读卡器当前模块IC是否上电复位
	if( nResetFlag != RESETSTATON)
	{
		return R531RESETNOT;
	}
	

	// 检测协议类型的合法性,非0和1设置成功
	if( nProtocolType != R531PROTOCOLT1 )
	{
		nProtocolType = R531PROTOCOLT0;
	}

	// 卡槽IC卡通讯协议为：T = 0 的处理
	if( nProtocolType == 0)
	{	// T=0,指令实例：b0 11 00  05 00 00 00  84 00 00 04
		// T=0,指令格式：B011 + 2字节指令长度 + 2字节预期返回长度 + APDU 
		//sprintf_s(szSend,1024,"B011%02.2X%02.2X0000%s", (unsigned int)nHLen, (unsigned int)nLLen, Send);
		sprintf(szSend,"B011%02.2X%02.2X0000%s", (unsigned int)nHLen, (unsigned int)nLLen, Send);

		nSendLen = (int)strlen(szSend);
	}else		// T = 1 指令数据组织
	{
		memset(szRecv,0x00,sizeof(szRecv));


		// B0 11 + 2字节 
		memset(szTmp,0x00,sizeof(szTmp));
		
		nPHLen = nHLen;
		nPLLen = nLLen;
		
		nPLLen += 4;			// T=1 多4字节的开始字段和结束字段

		if((nPLLen / 256) > 0)
		{
			nPHLen += 1;
			nPLLen = nPLLen % 256;
		}
		
		// UDPATE:20131118 -- 解决 接触模块 T=1 的卡的指令执行问题
		if(gCommFlag2 == 0)
		{
			// 指定数据部分：2字节指令长度、2字节预期返回数据长度设置成：0000、3字节APDU长度
			//sprintf_s(szTmp,1024,"%02.2X%02.2X000000%02.2X%02.2X%s",
			sprintf(szTmp,"%02.2X%02.2X000000%02.2X%02.2X%s",
				nPHLen,					// 加入开始字段和结束字段的APDU长度，高字节
				nPLLen,					// 加入开始字段和结束字段的APDU长度，低字节
				nHLen,					// 指令长度高字节
				nLLen,					// 指令长度低字节
				Send);

			nTmpLen = (int)strlen(szTmp);

			gCommFlag2 += 1;

		}else 
		{
			// 指定数据部分：2字节指令长度、2字节预期返回数据长度设置成：0000、3字节APDU长度
			//sprintf_s(szTmp,1024,"%02.2X%02.2X000000%02.2X%02.2X%s",
			sprintf(szTmp,"%02.2X%02.2X000000%02.2X%02.2X%s",
				nPHLen,					// 加入开始字段和结束字段的APDU长度，高字节
				nPLLen,					// 加入开始字段和结束字段的APDU长度，低字节
				nHLen+0x40,				// 指令长度高字节
				nLLen,					// 指令长度低字节
				Send);

			nTmpLen = (int)strlen(szTmp);

			gCommFlag2 = 0;
		}

		// 计算数据的LRC值
		memset(szHexLRC,0x00,sizeof(szHexLRC));
		ftCalLRC1(szTmp+8,nTmpLen-8,szHexLRC);

		memset(szSend,0x00,sizeof(szSend));

		//sprintf_s(szSend,1024,"B011%s%02.2X",szTmp,(unsigned char)szHexLRC[0]);
		sprintf(szSend,"B011%s%02.2X",szTmp,(unsigned char)szHexLRC[0]);
		nSendLen = (int)strlen(szSend);
	}	

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
	{	// 应答数据：00 06  e8 ed 68 65  90 00
		*RecvLen = nRecvLen -4;
		memcpy(Recv,szRecv+4,(nRecvLen-4));
		memcpy(Recv+(nRecvLen-4),"\x00",1);
	}else 
	{

		memcpy(szLen,szRecv,4);					// 获取长度字符串
		nSendLen = (int)ftHexToLong(szLen);		// 将BCD码字符串的长度转换成int
		
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
		// 检查应答的SW是否为：61XX ,则执行 00C00000xx 获取应答数据
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
//4-非接TypeA类工作模式函数封装
//==================================================================================================

/******************************************************************************************
 *******	函数名称：R531TypeARequest	-序号：24									*******
 *******	函数功能：非接IC卡寻卡,指令[C100]										*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******			  unsigned char ReqMode：寻卡方式，可用值如下：					*******
 *******							0x26：搜索射频场内处于IDLE状态的卡				*******
 *******							0x52：搜索射频场内处与IDLE和HALT状态的所有卡	*******
 *******			  char *Atqa：(输出)：成功返回ATQA，应答变量说明如下：			*******
 *******							[0]：卡类型； 02 - S70；04 - M1；08 - MPRO		*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；指令执行失败返回：-1；执行执行错误返回：-2		*******
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

	
	// 增加传入参数合法性检查

	// 将传入的读卡器句柄，转换成内部保持序号
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}



	// 检查传入寻卡方式参数:ReqMode的合法性
	if(!( ReqMode == 0x26 || ReqMode == 0x52 ))
	{
		return -1;
	}

	//设置执行的指令
	//sprintf_s(szSend,1024,"C100%02.2X",ReqMode);
	sprintf(szSend,"C100%02.2X",ReqMode);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答(应答数据为ASCII码)
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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

	// 设置应答变量，返回:ATQA 的第1字节的ASCII字符，【"04" -- M1  ;  "08" -- MPRO ;  "02" -- S70】
	memcpy(Atqa,szRecv,nRecvLen);

	// Add -2012-07-23 根据应答的ATQA 设置当前读卡器非接IC卡的卡类型
	stDeviceStruct[nDev].nCardType = szRecv[1] -'0';	// 4-M1-S50; 2 - M1-S70;	8 - MPRO


	gCommFlag = 0;
	return 0;
}


/******************************************************************************************
 *******	函数名称：R531TypeAAntiCollision	-序号：25							*******
 *******	函数功能：防冲突,指令[C101]												*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******			  int  CLVL：串联级别，值范围为:1~3	;通常设置成：1				*******
 *******			  char *UID：(输出)：成功返回UID[4字节]如："FFFFFFFF"			*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0,错误码如下：						*******
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


	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}
 
	// END Add 
	

	// 检查输入的串联级别参数是否合法
	if( CLVL <=0 || CLVL > 3)
	{
		return R531PARAMERR;
	}

	//设置执行的指令
	//sprintf_s(szSend,1024,"C101%02.2d",CLVL);
	sprintf(szSend,"C101%02.2d",CLVL);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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


	// 设置应答变量,ASCII码的UID,通常为8个ASCII字符
	memcpy(UID,szRecv,nRecvLen);

	return R531OK;
}




/******************************************************************************************
 *******	函数名称：R531TypeASelect	-序号：26									*******
 *******	函数功能：选卡,指令[C102]												*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******			  int  CLVL：串联级别，值范围为:1~3	;通常设置成：1				*******
 *******			  char *UID：UID，调用防冲突函数的返回值						*******
 *******			  char *SAK：应答数据，1字节									*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0,错误码如下：						*******
 *******							-1：串联级别参数CLVL错误						*******
 *******							-2：UID必须为4字节								*******
 *******							-3：指令执行失败								*******
 *******							-4：指令执行错误								*******
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


	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}
 
	// END Add 


	// 检查输入的串联级别参数是否合法
	if( CLVL <=0 || CLVL > 3)
	{
		return R531PARAMERR;
	}

	// UID长度为8即4字节BCD码值
	if(strlen(UID) != 8)
	{
		return R531PARAMERR;
	}


	//设置执行的指令
	//sprintf_s(szSend,1024,"C102%02.2d%s",CLVL,UID);
	sprintf(szSend,"C102%02.2d%s",CLVL,UID);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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

 	// 设置应答变量,SAK的ASCII码值，即2个ASCII码值
	memcpy(SAK,szRecv,nRecvLen);

	return R531OK;
}

/******************************************************************************************
 *******	函数名称：R531TypeAHalt		-序号：27									*******
 *******	函数功能：中断,指令[C103]												*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
  *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0,错误码如下：						*******
 *******							-1：指令执行失败								*******
 *******							-2：指令执行错误								*******
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

	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}
 
	// END Add 

	//设置执行的指令
	//strncpy_s(szSend,1024,"C103",4);
	strncpy(szSend,"C103",4);
	nSendLen =(int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
 *******	函数名称：R531TypeARats		-序号：28									*******
 *******	函数功能：选择应用,指令[C104]											*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******			  int CID：卡片识别符；即寻址卡片的逻辑号，值范围 0~E;通常为:0	*******
 *******			  char *ATS：(输出)ATS数据										*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0,错误码如下：						*******
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

	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}
 
	// END Add 

	if( CID < 0 || CID > 14)
	{
		return R531PARAMERR;
	}

	//设置执行的指令
	//sprintf_s(szSend,1024,"C104%02.2X",CID);
	sprintf(szSend,"C104%02.2X",CID);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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


	// 设置函数应答数据的ATS值(ASCII值)
	memcpy(ATS,szRecv,nRecvLen);


	return R531OK;
}


//==================================================================================================
//5-TypeB－类命令函数封装
//==================================================================================================
/******************************************************************************************
 *******	函数名称：R531TypeBRequest	-序号：29									*******
 *******	函数功能：非接IC卡寻卡,指令[C000]										*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******			  unsigned char ReqMode：寻卡方式，可用值如下：					*******
 *******							0x00：搜索射频场内处于IDLE状态的卡				*******
 *******							0x08：搜索射频场内处与IDLE和HALT状态的所有卡	*******
 *******			  unsigned char AFI：卡应用标识									*******
 *******			  unsigned char TimeN：时隙数,用于防冲突处理设定最大卡片数量，	*******
 *******							0x00：1张卡；0x01：2张卡；0x02：4张卡			*******
 *******                            0x03：8张卡；0x04：16张卡，其他值无效			*******
 *******			  char *Atqb：(输出)：成功返回ATQB，12字节应答数据，值如下：	*******
 *******							Byte1:50,固定值									*******
 *******							Byte2~Byte5：4字节PUPI							*******
 *******							Byte6~Byte9：应用信息字节；						*******
 *******							Byte10~Byte12：协议信息字节						*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；指令执行失败返回：<0;错误码如下：				*******
 *******								-1：寻卡方式参数ReqMode错误					*******
 *******								-2：时隙数错误								*******
 *******								-3：指令执行失败							*******
 *******								-4：指令执行错误							*******
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

	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}
 
	// END Add 


	// 检查寻卡方式参数的合法性
	if(!( ReqMode == R531REQBIDLE || ReqMode  == R531REQBALL) )
	{
		return R531PARAMERR;
	}

	if(!( TimeN == 0 || TimeN == 1 || TimeN == 2 || TimeN == 3 || TimeN == 4 ))
	{
		return R531PARAMERR;
	}
	//设置执行的指令
	//sprintf_s(szSend,1024,"C100%02.2X%02.2X%02.2X",ReqMode,AFI,TimeN);
	sprintf(szSend,"C100%02.2X%02.2X%02.2X",ReqMode,AFI,TimeN);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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


	// 设置应答变量
	memcpy(Atqb,szRecv,nRecvLen);

	return R531OK;
}



/******************************************************************************************
 *******	函数名称：R531TypeBSlotMarker	-序号：30								*******
 *******	函数功能：TypeB卡片防冲突,指令[C001]									*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******			  int  SlotNum：时隙数,从1～15,作防冲突处理时,可以不按顺序进行	*******
 *******			  char *ATQB：(输出)： 											*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0,错误码如下：						*******
 *******							-1：时隙数错误									*******
 *******							-2：指令执行失败								*******
 *******							-3：指令执行错误								*******
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

	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}
 
	// END Add 

	// 检查输入的时隙数是否合法
	if( SlotNum  < 1 || SlotNum > 15)
	{
		return R531PARAMERR;
	}

	//设置执行的指令
	//sprintf_s(szSend,1024,"C001%02.2X",SlotNum);
	sprintf(szSend,"C001%02.2X",SlotNum);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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

	// 设置应答变量值
	memcpy(ATQB,szRecv,nRecvLen);

	return R531OK;
}


/******************************************************************************************
 *******	函数名称：R531TypeBAttrib	-序号：31									*******
 *******	函数功能：TypeB卡片选择,指令[C002]										*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******			  int  SlotNum：时隙数,从1～15,作防冲突处理时,可以不按顺序进行	*******
 *******			  char *ATQB：(输出)： 											*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0,错误码如下：						*******
 *******							-1：时隙数错误									*******
 *******							-2：指令执行失败								*******
 *******							-3：指令执行错误								*******
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

	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}
 
	// END Add 


	// 检查输入的时隙数是否合法
	if( SlotNum  < 1 || SlotNum > 15)
	{
		return R531PARAMERR;
	}

	//设置执行的指令
	//sprintf_s(szSend,1024,"C001%02.2X",SlotNum);
	sprintf(szSend,"C001%02.2X",SlotNum);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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


	// 设置应答变量
	memcpy(ATQB,szRecv,nRecvLen);

	return R531OK;
}

/******************************************************************************************
 *******	函数名称：R531TypeBHalt	-序号：32										*******
 *******	函数功能：TypeB卡片选择,指令[C003]										*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
  *******			  char *PUPI： 	4字节											*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：R531OK；失败返回：错误代码							*******
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


	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}
 
	// END Add 

	// 检查输入的时隙数是否合法
	if( strlen(PUPI) != 8)
	{
		return -1;
	}

	//设置执行的指令
	//sprintf_s(szSend,1024,"C003%s",PUPI);
	sprintf(szSend,"C003%s",PUPI);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
//6-14443－4部分传输命令函数封装
//==================================================================================================

/******************************************************************************************
 *******	函数名称：R531TypeDeselect	-序号：33									*******
 *******	函数功能：取消卡片选择,指令[C200]										*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
  *******			  int CID：卡片识别符；即寻址卡片的逻辑号，值范围 0~E,通常为:0	*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0,错误码如下：						*******
 *******							-1：PUPI参数错误								*******
 *******							-2：指令执行失败								*******
 *******							-3：指令执行错误								*******
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

	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}
 
	// END Add 

	// 检查输入的CID是否合法
	if( CID < 0 || CID > 14 )
	{
		return R531PARAMERR;
	}

	//设置执行的指令
	//sprintf_s(szSend,1024,"C200%02.2X",CID);
	sprintf(szSend,"C200%02.2X",CID);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
 	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
 *******	函数名称：R531TypeAPDU			-序号：34								*******
 *******	函数功能：执行非接IC卡指令,指令[C201]									*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******			  int SendLen：请求指令长度[ASCII码长度]						*******
 *******			  char *Send：请求指令[指令ASCII字符串]							*******
 *******			  int *RecvLen：(输出)应答数据长度[ASCII码长度]					*******
 *******			  char *Recv：(输出)应答数据[ASCII码格式]						*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******              int CardType = 04:卡类型；04-M1；08-MPRO;02-S70,缺省为：04	*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0,错误码如下：						*******
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

	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}
 
	// END Add 

	// 检查传入的IC卡指令的合法性
	if( SendLen % 2 != 0)
	{
		return R531PARAMERR;
	}

	if( SendLen != (int)strlen(Send) )
	{
		return R531PARAMERR;
	}

	// 根据函数的卡类型参数，设置执行的APDU指令流
	if( CardType == 4)					// 卡类型为：M1
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

	}else if( CardType == 8)			// 卡类型为：MPRO
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

	}else if ( CardType == 2)			// 卡类型为：S70   -- Update -- 20120808 -- 指令的INS、CLS不发生改变，不修改gCommFlag 值
	{

		if( gCommFlag == 0)
		{
			//sprintf_s(szSend,1024,"C2010A00%s",Send);
			sprintf(szSend,"C2010A00%s",Send);

			if( memcmp(Send,stDeviceStruct[nDev].szInsCls,4) != 0)
			{
				gCommFlag += 1;

				// 保持上个指令的INS和CLA
				memcpy(stDeviceStruct[nDev].szInsCls,Send,4);
			}
		}else
		{
			//sprintf_s(szSend,1024,"C2010B00%s",Send);
			sprintf(szSend,"C2010B00%s",Send);
			if( memcmp(Send,stDeviceStruct[nDev].szInsCls,4) != 0)
			{			
				gCommFlag = 0;

				// 保持上个指令的INS和CLA
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

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
 	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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


	// 根据卡类型设置类型设置应答数据
	if(CardType == 4)						// M1卡应答变量的设置
	{
		i = nRecvLen-6;
		memcpy(Recv,szRecv+6,nRecvLen-6);
		memcpy(Recv+i,"\x00",1);
		*RecvLen = nRecvLen - 6;

	}else if(CardType == 8)					// MPRO卡应答变量的设置
	{
		i = nRecvLen -4;
		memcpy(Recv,szRecv+4,nRecvLen-4);
		memcpy(Recv+i,"\x00",1);
		*RecvLen = nRecvLen - 4;

	}else if( CardType == 2)				// S70卡应答变量的设置
	{
		i = nRecvLen -4;
		memcpy(Recv,szRecv+4,nRecvLen-4);
		memcpy(Recv+i,"\x00",1);
		*RecvLen = nRecvLen - 4;

	}else									// 其他类型卡的应答变量的设置
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
//7-M1卡命令函数封装
//==================================================================================================

/******************************************************************************************
 *******	函数名称：R531M1Auth	-序号：35										*******
 *******	函数功能：M1卡认证,指令[C300]											*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******			  unsigned char ABLK：块地址，[即扇区号]，可用值如下：			*******
 *******					M1S50卡(04)：00～3F,第1个块地址为：0x00					*******
 *******					M1S70卡(02)：00～255[FF]；								*******
 *******					ML卡   (08)：00～0B										*******
 *******			  unsigned char KT：密钥类型，值如下：							*******
 *******                    R531KTUSERKEYA	:用户传输密钥认证KEYA					*******
 *******					R531KTUSERKEYB	:用户传输密钥认证KEYB					*******
 *******					R531KTROMKEYA	:使用射频模块中存储的密钥认证KEYA		*******
 *******					R531KTROMKEYB	:使用射频模块中存储的密钥认证KEYB		*******
 *******			  char *KData：KT为R531KTROMKEYA、R531KTROMKEYB					******* 
 *******                                       其值为一字节密钥索引,范围0～1F		*******
 *******							 为R531KTUSERKEYA、R531KTUSERKEYB				*******
 *******									其值为12字节密钥						*******
 *******			  char *SN:卡序号,4字节的UID,M1需要，C101指令返回				*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 ******* 指令实例：c300 0000c0 0f0f0f0f0f0f0f0f0f0f0f0f e7018bae					*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：R531OK；失败返回：错误代码							*******
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

	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}
 
	// END Add 
	
	CardType = (unsigned char)stDeviceStruct[nDev].nCardType;



	// 传入的卡类型参数合法性检查
	if(  !(CardType ==  R531MODES50 || CardType == R531MODES70 || CardType == R531MODEML ) )
	{	// 传入的M1卡类型参数错误，(R531TypeARequest 函数返回)
		return R531PARAMERR;
	}
	
	// 块地址参数合法性检查
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


	// 传入的密钥类型参数合法性检查
	if( !( KT == R531KTUSERKEYA || KT == R531KTUSERKEYB || KT == R531KTROMKEYA || KT == R531KTROMKEYB))
	{
		return R531PARAMERR;
	}

	// 根据传入的密钥类型参数设置指令报文的密钥类型值
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

	// 根据传入的密钥类型，检查传入的密钥参数的合法性
	if( KT == R531KTUSERKEYA || KT == R531KTUSERKEYB)		// 12字节密钥
	{
		if( strlen(KData) != 24 )
		{
			return R531PARAMERR;
		}
	} else													//  1字节的密钥索引
	{
		if( strlen(KData) != 2)
		{
			return R531PARAMERR;
		}
	}


	//检查传入的UID的合法性
	if( strlen(SN) != 8)
	{
		return R531PARAMERR;
	}

	//设置执行的指令
	//c3 00 00  00 c0 0f 0f   0f 0f 0f 0f   0f 0f 0f 0f   0f 0f e7 01  8b ae 
	if(CardType == R531MODES50 || CardType == R531MODES70)			// M1-S50卡 、M1-S70卡
	{
		// 数据:C30000 +1字节块地址 + 密钥类型
		//sprintf_s(szSend,1024,"C30000%02.2X%02.2X%s%s",
		sprintf(szSend,"C30000%02.2X%02.2X%s%s",
						ABLK,		// 1字节块地址
						szKeyType,	// 1字节密钥类型
						KData,		// KT b7 = 0:1字节的密钥索引；b7 =1:12字节密钥
						SN);
	}else							// MPRO或S70卡
	{
		//sprintf_s(szSend,1024,"C30000%02.2X%02.2X%s",
		sprintf(szSend,"C30000%02.2X%02.2X%s",
						ABLK,
						szKeyType,
						KData);

	}

	nSendLen = (int)strlen(szSend);
	

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
 *******	函数名称：R531ReadBlock		-序号：36									*******
 *******	函数功能：读块,指令[C301]												*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
  *******			  unsigned char ABLK：块地址，[即扇区号]，可用值如下：			*******
 *******					M1S50卡：00～3F,第1个块地址为：0x00						*******
 *******					M1S70卡：00～255[FF]；									*******
 *******					ML卡   ：00～0B											*******
 *******			  unsigned char Lc：要读取的数据长度;M1最大长度为：0x10,16字节	*******
 *******			  int *RecvLen：(输出)应答数据长度[ASCII码长度]					*******
 *******			  char *Recv：(输出)应答数据[ASCII码格式]						*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：R531OK；失败返回：错误代码							*******
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

	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}
 
	// END Add 

	CardType = (unsigned char)stDeviceStruct[nDev].nCardType;


	// 传入的卡类型参数合法性检查
	if(  !(CardType ==  R531MODES50 || CardType == R531MODES70 || CardType == R531MODEML ) )
	{	// 传入的M1卡类型参数错误，(R531TypeARequest 函数返回)
		return R531PARAMERR;
	}
	
	// 块地址参数合法性检查
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

	//设置执行的指令;C301 +1字节卡类型，固定为：0x00;+1字节的块地址; +1字节的要读取的数据长度
	//c3 01 00  00 10
	//sprintf_s(szSend,1024,"C30100%02.2X%02.2X",
	sprintf(szSend,"C30100%02.2X%02.2X",
					ABLK,					// 1字节的读取块地址
					Lc);					// 1字节的读取数据长度

	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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


	// 取应答信息
	memcpy(Recv,szRecv,nRecvLen);
	*RecvLen = nRecvLen ;

	return R531OK;
}




/******************************************************************************************
 *******	函数名称：R531M1WriteBlock	-序号：37									*******
 *******	函数功能：写块,指令[C302]												*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******			  unsigned char cardType:M1卡类型，由:C100指令返回，值如下：	*******
 *******					0x04：M1-S50卡；										*******
 *******					0x08：MPRO卡；											*******
 *******					0x02：M1-S70卡											*******
 *******			  unsigned char ABLK：块地址，[即扇区号]，可用值如下：			*******
 *******					M1S50卡：00～3F,第1个块地址为：0x00						*******
 *******					M1S70卡：00～255[FF]；									*******
 *******					ML卡   ：00～0B											*******
 *******			  unsigned char Lc：要写入的数据长度;M1最大长度为：0x10,16字节	*******
 *******              unsigned char *WriteData:要写入的数据							*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0,错误码如下：						*******
 *******							-1：指令长度错误								*******
 *******							-2：指令错误									*******
 *******							-3：指令执行失败								*******
 *******							-4：指令执行错误								*******
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

	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}
 
	// END Add 

	CardType = (unsigned char )stDeviceStruct[nDev].nCardType;

	// 传入的卡类型参数合法性检查
	if(  !(CardType ==  R531MODES50 || CardType == R531MODES70 || CardType == R531MODEML ) )
	{	// 传入的M1卡类型参数错误，(R531TypeARequest 函数返回)
		return R531PARAMERR;
	}
	
	// 块地址参数合法性检查
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
	
	// 检查数据长度参数及数据实际长度的一致性
	if( nDataLen*2 != (int)strlen( (char *)WriteData ) )
	{
		return R531PARAMERR;
	}

	//设置执行的指令;C302 +1字节卡类型，固定为：0x00[M1卡];+1字节的块地址; +1字节写入数据长度 + 写入数据
	//sprintf_s(szSend,1024,"C30200%02.2X%02.2X%s",
	sprintf(szSend,"C30200%02.2X%02.2X%s",
					ABLK,				// 写入的块地址
					Lc,					// 写入的数据长度
					WriteData);			// 写入的数据,
				
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
 *******	函数名称：R531M1AddValue	-序号：38									*******
 *******	函数功能：增值,指令[C303]												*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******			  unsigned char cardType:M1卡类型，由:C100指令返回，值如下：	*******
 *******					0x04：M1-S50卡；										*******
 *******					0x08：MPRO卡；											*******
 *******					0x02：S70卡												*******
 *******			  unsigned char ABLK：块地址，[即扇区号]，可用值如下：			*******
 *******					M1S50卡：00～3F,第1个块地址为：0x00						*******
 *******					M1S70卡：00～255[FF]；									*******
 *******					ML卡   ：00～0B											*******
 *******			  unsigned char Lc：要增加的数据值长度;							*******
 *******              unsigned char *Data:要增加的数据值4字节						*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0,错误码如下：						*******
 *******							-1：指令长度错误								*******
 *******							-2：指令错误									*******
 *******							-3：指令执行失败								*******
 *******							-4：指令执行错误								*******
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

	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}
 
	// END Add 
	CardType = (unsigned char)stDeviceStruct[nDev].nCardType;

	// 传入的卡类型参数合法性检查
	if(  !(CardType ==  R531MODES50 || CardType == R531MODES70 || CardType == R531MODEML ) )
	{	// 传入的M1卡类型参数错误，(R531TypeARequest 函数返回)
		return R531PARAMERR;
	}
	
	// 块地址参数合法性检查
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

	// Lc 值必须为：0x10
	if( nDataLen != 16 )
	{
		return R531PARAMERR;
	}


	// 块数据为4字节
	if( strlen( Data ) != 8)
	{
		return R531PARAMERR;
	}
	 

	//指令实例：c3 03 00  00 10 01 00  00 00
	//设置执行的指令;C303 +1字节卡类型，固定为：0x00[M1卡];+1字节的块地址; +1字节数据长度 + 数据
	//sprintf_s(szSend,1024,"C30300%02.2X%02.2X%s",
	sprintf(szSend,"C30300%02.2X%02.2X%s",
					ABLK,					// 加值块地址
					Lc,						// 长度
					Data					// 增值的数据
					);

	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
 *******	函数名称：R531M1SubValue	-序号：39									*******
 *******	函数功能：减值,指令[C304]												*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******			  unsigned char cardType:M1卡类型，由:C100指令返回，值如下：	*******
 *******					0x04：M1-S50卡；										*******
 *******					0x08：MPRO卡；											*******
 *******					0x02：S70卡												*******
 *******			  unsigned char ABLK：块地址，[即扇区号]，可用值如下：			*******
 *******					M1S50卡：00～3F,第1个块地址为：0x00						*******
 *******					M1S70卡：00～255[FF]；									*******
 *******					ML卡   ：00～0B											*******
 *******			  unsigned char Lc：要减的数据值长度;							*******
 *******              unsigned char *Data:要减的数据值4字节							*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0,错误码如下：						*******
 *******							-1：指令长度错误								*******
 *******							-2：指令错误									*******
 *******							-3：指令执行失败								*******
 *******							-4：指令执行错误								*******
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

	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}
 
	// END Add 

	// Update：2012-07-23 ,M1卡类型修改为从读卡器结构数组中获取，此值在TypeA工作模式寻卡时根据返回值设置；
	CardType = (unsigned char)stDeviceStruct[nDev].nCardType;

	// 传入的卡类型参数合法性检查
	if(  !(CardType ==  R531MODES50 || CardType == R531MODES70 || CardType == R531MODEML ) )
	{	// 传入的M1卡类型参数错误，(R531TypeARequest 函数返回)
		return R531PARAMERR;
	}
	
	// 块地址参数合法性检查
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

	// Lc 值必须为：0x10
	if( nDataLen != 16 )
	{
		return R531PARAMERR;
	}


	// 块数据为4字节
	if( strlen( Data ) != 8)
	{
		return R531PARAMERR;
	}

	//指令实例：c3 03 00  00 10 01 00  00 00
	//设置执行的指令;C303 +1字节卡类型，固定为：0x00[M1卡];+1字节的块地址; +1字节数据长度 + 数据
	//sprintf_s(szSend,1024,"C30400%02.2X%02.2X%s",
	sprintf(szSend,"C30400%02.2X%02.2X%s",
					ABLK,					// 减值块地址
					Lc,						// 长度
					Data					// 减值的数据
					);

	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
 *******	函数名称：R531M1Transfer	-序号：40									*******
 *******	函数功能：将内部临时寄存器的内容写入值存储段,指令[C306]					*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******			  unsigned char ABLK：块地址，[即扇区号]，可用值如下：			*******
 *******					M1S50卡：00～3F,第1个块地址为：0x00						*******
 *******					M1S70卡：00～255[FF]；									*******
 *******					ML卡   ：00～0B											*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：<0,错误码如下：						*******
 *******							-1：指令长度错误								*******
 *******							-2：指令错误									*******
 *******							-3：指令执行失败								*******
 *******							-4：指令执行错误								*******
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

	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}
 
	// END Add 

	// Update：2012-07-23 ,M1卡类型修改为从读卡器结构数组中获取，此值在TypeA工作模式寻卡时根据返回值设置；
	CardType = (unsigned char)stDeviceStruct[nDev].nCardType;

	// 传入的卡类型参数合法性检查
	if(  !(CardType ==  R531MODES50 || CardType == R531MODES70 || CardType == R531MODEML ) )
	{	// 传入的M1卡类型参数错误，(R531TypeARequest 函数返回)
		return R531PARAMERR;
	}
	
	// 块地址参数合法性检查
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

	//指令实例：c3 06 00  08 
	//设置执行的指令;C306 +1字节卡类型，固定为：0x00[M1卡];+1字节的块地址; 
	//sprintf_s(szSend,1024,"C30600%02.2X",
	sprintf(szSend,"C30600%02.2X",
					ABLK					// 减值块地址
					);

	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		// 保存指令的状态值到函数的输出变量，可选，非空时设置此值
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
 *******	函数名称：R531M1Restore		-序号：41									*******
 *******	函数功能：将值存储段的内容移至内部临时寄存器,指令[C305]					*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******			  unsigned char ABLK：块地址，[即扇区号]，可用值如下：			*******
 *******					M1S50卡：00～3F,第1个块地址为：0x00						*******
 *******					M1S70卡：00～255[FF]；									*******
 *******					ML卡   ：00～0B											*******
 *******			  int *Status：(可选、输出)指令执行状态,成功返回：0；			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：R531OK；失败返回：错误码							*******
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

	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;
		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}
 
	// END Add 

	// Update：2012-07-23 ,M1卡类型修改为从读卡器结构数组中获取，此值在TypeA工作模式寻卡时根据返回值设置；
	CardType = (unsigned char)stDeviceStruct[nDev].nCardType;

	// 传入的卡类型参数合法性检查
	if(  !(CardType ==  R531MODES50 || CardType == R531MODES70 || CardType == R531MODEML ) )
	{	// 传入的M1卡类型参数错误，(R531TypeARequest 函数返回)
		return R531PARAMERR;
	}
	
	// 块地址参数合法性检查
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

	//指令实例：c3 05 00  08 
	//设置执行的指令;C305 +1字节卡类型，固定为：0x00[M1卡];+1字节的块地址; 
	//sprintf_s(szSend,1024,"C30500%02.2X",
	sprintf(szSend,"C30500%02.2X",
					ABLK					// 减值块地址
					);

	nSendLen = (int)strlen(szSend);


	// 执行指令获取应答
	nRet = R531DeviceCommand(hDev,szSend,nSendLen,szRecv,&nRecvLen,&nStatus);
	if(nRet != R531OK)
	{
		return nRet;
	}


	return R531OK;
}


//ADD-2013-11-06 
/******************************************************************************************
 *******	函数名称：R531TypeFindCard		-序号：42								*******
 *******	函数功能：未进入14443-4层的寻卡,指令[C400]								*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
  *******			  int *Status：0x00--有卡；0x01 --无卡							*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：R531OK；失败返回：错误码							*******
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

	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	}
 
	//设置执行的指令:C400
	memcpy(szSend,"C400",5);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
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
 *******	函数名称：R531TypeFindCard1		-序号：43								*******
 *******	函数功能：已进入14443-4层的寻卡,指令[C401]								*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
  *******			  int *Status：0x00--有卡；0x01 --无卡							*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：R531OK；失败返回：错误码							*******
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

	//ADD-20120716 参数及运行环境检查
	nDev = hDev -1;

	// 检查句柄合法性
	if( nDev < 0 || nDev >= R531DEVMAX)
	{
		return R531DEVERR;
	}

	// 检查读卡器序号是否超范围
	if( nDev >= gDeviceTotal)
	{
		return R531DEVERR;		
	}

	// 检查当前读卡器是否已连接
	if( stDeviceStruct[nDev].nConnStatus != CONNSTATCONN)
	{
		// 读卡器未连接
		return R531UNCONN;
	} 
	// END Add 

	//设置执行的指令:C401
	memcpy(szSend,"C401",5);
	nSendLen = (int)strlen(szSend);

	// 执行指令获取应答
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
 *******	函数名称：R531GetErrMsg			-序号：44								*******
 *******	函数功能：获取API函数错误代码的错误信息									*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
  *******			  int *Status：0x00--有卡；0x01 --无卡							*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：R531OK；失败返回：错误码							*******
 ******************************************************************************************/
int R531GetErrMsg(int ErrCode,char *ErrMsg)
{
	memcpy(ErrMsg,"\x00",1);

	switch(ErrCode)
	{
	case R531OK:
		memcpy(ErrMsg,"成功完成",9);
		break;
	case R531SYSERR:
		memcpy(ErrMsg,"系统错误",9);
		break;
	case R531NODEV:
		memcpy(ErrMsg,"系统无读卡器设备",17);
		break;
	case R531NOEXIST:
		memcpy(ErrMsg,"设备不存在",11);
		break;
	case R531CONNERR:
		memcpy(ErrMsg,"设备连接错误",13);
		break;
	case R531UNCONN:
		memcpy(ErrMsg,"读卡器未连接",13);
		break;
	case R531PARAMERR:
		memcpy(ErrMsg,"参数错误",9);
		break;
	case R531NOCARD:
		memcpy(ErrMsg,"无卡",5);
		break;
	case R531NORESPONSE:
		memcpy(ErrMsg,"卡片无应答",11);
		break;
	case R531NOINIT:
		memcpy(ErrMsg,"未初始化设备",13);
		break;
	case R531RESPDATALENERR:
		memcpy(ErrMsg,"IC卡无应答或应答数据长度错误",29);
		break;
	case R531EXECUTEERR:
		memcpy(ErrMsg,"底层设备指令执行失败",21);
		break;
	case R531SLOTERR:
		memcpy(ErrMsg,"卡槽参数错误",13);
		break;
	case R531NOSETSLOT:
		memcpy(ErrMsg,"未设置卡槽",11);
		break;
	case R531DEVERR:
		memcpy(ErrMsg,"读卡器句柄非法",15);
		break;
	case R531RESETERR:
		memcpy(ErrMsg,"上电复位失败",13);
		break;
	case R531RESETNOT:
		memcpy(ErrMsg,"未上电复位",11);
		break;
	case R531NOUSE:
		memcpy(ErrMsg,"未授权不可使用",15);
		break;
	case R531SAMNOERR:
		memcpy(ErrMsg,"PSAM和管理卡不一致",19);
		break;
	case R531VERMACERR:
		memcpy(ErrMsg,"验证报文MAC执行的指令错误",26);
		break;
	case R531MACERR:
		memcpy(ErrMsg,"MAC验证错误",12);
		break;
	case R531RESPLENERR:
		memcpy(ErrMsg,"应答数据长度错误",17);
		break;
	case R531RESPERR:
		memcpy(ErrMsg,"APDU执行错误返回的SW值不为:9000",32);
		break;
	default:
		memcpy(ErrMsg,"未知错误",9);
		break;
	}

	return NO_ERROR;
}


// 获取系统静态全部变量值
#ifdef MANA
int R531Test()
{



	ftWriteLog("R531Test.log","#############################相关静态变量数据获取############################");
	ftWriteLog("R531Test.log","初始化标志[%d]",gInitFlag);
	ftWriteLog("R531Test.log","R531设备个数=[%d]",gDeviceTotal);

	int i=0;

	for(i=0;i<gDeviceTotal;i++)
	{
		ftWriteLog("R531Test.log","设备序号=[%d] 状态=[%d] 上电标志=[%d][%d][%d][%d],协议类型=[%d][%d][%d][%d]",(i+1),
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
