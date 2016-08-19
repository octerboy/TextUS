/******************************************************************************************
 ** 文件名称：R531DeviceFunc.h	                                                         **
 ** 文件描述：ROCKEY531设备接口函数					                                     **
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



// 第一级函数：设备初始化、连接及关闭函数
//1 R531设备环境初始化，设备处理的第一个函数只需调用一次，
int R531DeviceFind();

//2 连接指定读卡器设备
int R531ConnDev(char *pDevName);

//3 关闭指定读卡器设备连接
int R531CloseDev(int pDevHandle);



// 第二级函数：设备接口指令处理函数
// -------------------------------------------------------------------------------------------------
// 设备类函数
// -------------------------------------------------------------------------------------------------

//4 获取产品序号
int R531DevSeqNo(int hDev,char *OutBuf,int *Status=NULL);

//5 蜂鸣
int R531DevBeep(int hDev,unsigned char CycleNum,unsigned char  Times,unsigned char Interval,int *Status=NULL );

//6 射频模块硬复位
int R531DevResetHW(int hDev,unsigned char Msec,int *Status =NULL);

//7 射频场开关函数
int R531DevRField(int hDev,unsigned char Mode,unsigned char Inteval, int *Status =NULL);

//8 设置非接工作模式函数
int R531DevSetRfMode(int hDev,unsigned char Mode,unsigned char Speed ,int *Status =NULL);

//9 获取非接工作模式函数
int R531DevGetRfMode(int hDev, int *Mode,int *Status =NULL);

//10 检查用户卡卡槽是否有卡及卡是否上电
int R531DevCheckUserCardSlot(int hDev,int *SlotStatus,int *Status =NULL);

//11 ROCKEY501读卡器红灯控制
int R531DevRedLightCtl(int hDev,unsigned char CycleNum,unsigned char LightTime,unsigned char OffTime,int *Status =NULL);

//12 自定义射频卡超时时间
int R531DevSetRFTimeOut(int hDev,unsigned char TimeOut,int *Status =NULL);

//13 强制退出
int R531DevForceQuit(int hDev,int *Status =NULL);

//14 生成用户UID
int R531DevGenUID(int nDev,int DataLen,unsigned char *Data ,int *Status = NULL);

//15 获取用户UID 
int R531DevGetUID(int hDev,int *DataLen,unsigned char *Data ,int *Status = NULL);

//16 修改用户UID
int R531DevModUID(int hDev,int DataLen,unsigned char *Data ,int *Status = NULL);

//17 修改读卡器参数
int R531DevModPara(int hDev,int ParaLen, char *Para,int *Status = NULL);

//18 恢复读卡器参数
int R531DevRecoverPara(int hDev,int *Status = NULL);

// -------------------------------------------------------------------------------------------------
// 接触卡类操作函数
// -------------------------------------------------------------------------------------------------

//19 选择卡槽号
int R531CpuSetSlot(int hDev,int Slot,int *Status =NULL);

//20 上电复位  
int R531CpuReset(int hDev,int *AtrLen,char *Atr,int *Status = NULL );

//21 下电
int R531CpuPowerOff(int hDev,int *Status = NULL);

//22 PPS
int R531CpuPPS(int hDev,int PPSLen,char *PPS,char *Response,int *Status = NULL);

//23 执行APDU指令
int R531CpuAPDU(int hDev,int SendLen,char *Send,int *RecvLen,char *Recv,int *Status=NULL);
int R531CpuAPDU(int hDev,char *Send,char *Recv,int *Sttus=NULL);


// -------------------------------------------------------------------------------------------------
// 非接TypeA工作模式命令函数
// -------------------------------------------------------------------------------------------------

//24 寻卡
int R531TypeARequest(int hDev,unsigned char ReqMode,char *Atqa,int *Status =NULL);

//25 防冲突
int R531TypeAAntiCollision(int hDev,int CLVL,char *UID,int *Status = NULL);

//26 选择卡片
int R531TypeASelect(int hDev,int CLVL,char *UID,char *SAK,int *Status =NULL) ;

//27 中断
int R531TypeAHalt(int hDev,int *Status =NULL);

//28 选择应用
int R531TypeARats(int hDev,int CID,char *ATS,int *Status=NULL);


// -------------------------------------------------------------------------------------------------
// 非接TypeB工作模式命令函数
// -------------------------------------------------------------------------------------------------

// 29 TypeB工作模式非接IC卡寻卡
int R531TypeBRequest(int hDev,unsigned char ReqMode,unsigned char AFI,unsigned char TimeN,char *Atqb ,int *Status =NULL);


// 30 TypeB工作模式非接IC卡防冲突
int R531TypeBSlotMarker(int hDev,int SlotNum,char *ATQB,int *Status = NULL);

// 31 TypeB工作模式非接IC卡卡片选择
int R531TypeBAttrib(int hDev,int SlotNum,char *ATQB,int *Status = NULL);

// 32.TypeB工作模式非接IC卡中断
int R531TypeBHalt(int hDev,char *PUPI,int *Status = NULL);


// -------------------------------------------------------------------------------------------------
// 14443-4传输命令函数
// -------------------------------------------------------------------------------------------------

//33 取消卡片选择
int R531TypeDeselect(int hDev,int CID,int *Status =NULL);

//34 非接TypeA、TypeB APDU指令执行
int R531TypeAPDU(int hDev,int SendLen,char *Send,int *RecvLen,char *Recv,int *Status=NULL,int CardType = 4 );
int R531TypeAPDU(int hDev,char *Send,char *Recv,int *Status=NULL,int CardType=4);


// -------------------------------------------------------------------------------------------------
// M1卡命令函数
// -------------------------------------------------------------------------------------------------

// 35 M1-认证
int R531M1Auth(int hDev,unsigned char ABLK,unsigned char KT, char *KData, char *SN,int *Status  =NULL);


// 36 M1-读块
int R531M1ReadBlock(int hDev,unsigned char ABLK,int *RecvLen,char *Recv,int *Status  = NULL,unsigned char Lc = 0x10);

// 37 M1-写块
int R531M1WriteBlock(int hDev,unsigned char ABLK,unsigned char Lc, char *WriteData,int *Status = NULL);

// 38 M1-增值
int R531M1AddValue(int hDev,unsigned char ABLK,unsigned char Lc,char *Data,int *Status =NULL);

// 39 M1-减值
int R531M1SubValue(int hDev,unsigned char ABLK,unsigned char Lc, char *Data,int *Status =NULL);

// 40 M1-增值、减值的值存储(即将临时寄存器的内容写入值存储段)
int R531M1Transfer(int hDev,unsigned char ABLK,int *Status = NULL);

// 41 M1-将值存储段的内容移至内部临时寄存器
int R531M1Restore(int hDev,unsigned char ABLK,int *Status = NULL);




// Add-2013-11-06 -- 空闲状态寻卡(非接模块寻卡)
//42.未进入14443-4层的寻卡
int R531TypeFindCard(int hDev,int *Status);

//43.已进入14443-4层的寻卡
int R531TypeFindCard2(int hDev,int *Status);


//44.获取错误代码错误信息
int R531GetErrMsg(int ErrCode,char *ErrMsg);


// 系统静态全局变量测试函数
int R531Test();




// 第三级：IC卡应用类函数(对底层设备函数进行应用封装)



// 系统内部处理的R531设备最大个数
#define R531DEVMAX           9				// 系统内部支持的R531读卡器最大个数


// M1卡非接IC卡类型,M1卡属于TypeA工作模式
#define R531MODES50			 4				// M1-S40 卡 
#define R531MODES70			 2				// M1-S70 卡
#define R531MODEML			 8				// ML 卡

// M1卡认证的密钥类型值常量
#define R531KTUSERKEYA		 1				// 用户传输密钥认证KEYA
#define R531KTUSERKEYB		 2				// 用户传输密钥认证KEYB
#define R531KTROMKEYA		 3				// 使用射频模块中存储的密钥认证KEYA
#define R531KTROMKEYB		 4				// 使用射频模块中存储的密钥认证KEYB



// 非接TypeB工作模式寻卡方式
#define R531REQBIDLE		 0				//  搜索射频场内处与IDLE状态的卡
#define R531REQBALL			 8				// 搜索射频场内处与IDLE和HALT状态的所有卡


// 读卡器接触卡通信协议
#define R531PROTOCOLNO		-1				// 接触卡通信协议未知
#define R531PROTOCOLT0		 0				// 接触卡通信协议为：T=0
#define R531PROTOCOLT1		 1				// 接触卡通信协议为：T=1


// 检查用户卡卡到位，状态常量
#define R531CARDSTATNO		 0				// 无用户卡无
#define R531CARDSTATNOPOWER  1				// 用户卡未上电
#define R531CARDSTATNORMAL   2				// 用户卡已上电



// 读卡器设备初始化标志常量
#define R531INITFLAGNO		 0				// R531设备环境未初始化
#define R531INITFLAGOK		 1				// R531设备环境已初始化

// 读卡器设备连接状态常量
#define CONNSTATNODEV		-1				// 无此读卡器
#define CONNSTATNOCONN		 0				// 未连接
#define CONNSTATCONN		 1				// 已连接
#define CONNSTATCLOSE		 2				// 已关闭

// IC卡复位标志常量
#define RESETSTATNOT		 0				// 未上电复位
#define RESETSTATON			 1				// 已上电复位
#define RESETSTATOFF		 2				// 已下电

// 设备卡槽常量
#define R531SLOTNOTSET		-1				// 未选择卡槽
#define R531SLOTUSER		 0				// 用户卡槽
#define R531SLOTSAM1		 1				// SAM1卡槽
#define R531SLOTSAM2		 2				// SAM2卡槽
#define R531SLOTSAM3		 3				// SAM3卡槽


// 函数返回值常量
#define R531OK				 0				// 成功完成
#define R531SYSERR			-1				// 系统错误(获取系统R531设备时调用系统函数出错)
#define R531NODEV			-2				// 系统无R531设备；
#define R531NOEXIST			-3				// 当前设备不存在
#define R531CONNERR			-4				// 设备连接错误
#define R531UNCONN			-5				// 读卡器未连接
#define R531PARAMERR		-6				// 函数参数错误
#define R531NOCARD			-7				// 无卡
#define R531NORESPONSE		-8				// 卡片无应答
#define R531NOINIT			-9				// 未初始化设备
#define R531RESPDATALENERR	-10				// IC卡无应答或应答数据长度错误
#define R531EXECUTEERR		-11				// 底层设备指令执行失败
#define R531SLOTERR			-12				// 卡槽参数错误
#define R531NOSETSLOT		-13				// 未设置卡槽
#define R531DEVERR			-14				// 读卡器句柄非法
#define R531RESETERR		-15				// 上电复位失败
#define R531RESETNOT		-16				// 未上电复位
#define R531NOUSE           -17				// 设备未授权不可使用

// 粤通库底层错误代码
#define R531SAMNOERR		-100			// PSAM和管理卡不一致(即两个卡的卡号不相同)
#define R531VERMACERR		-101			// 验证报文的MAC执行的指令错误
#define R531MACERR          -107			// MAC验证错误
#define R531RESPLENERR		-108			// 应答数据长度错误
#define R531RESPERR			-109			// APDU执行错误返回的SW值不为：9000

