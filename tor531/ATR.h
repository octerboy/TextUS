/******************************************************************************************
 ** 文件名称：ATR.h						                                                 **
 ** 文件描述：IC卡上电复位信息处理头文件	                                             **
 **--------------------------------------------------------------------------------------**
 ** 创 建 人：智能卡业务部                                                               **
 ** 创建日期：2012-07-09                                                                 **
 **--------------------------------------------------------------------------------------**
 ** 修 改 人：                                                                           **
 ** 修改日期：                                                                           **
 **--------------------------------------------------------------------------------------**
 ** 版 本 号：V1.0                                                                       **
 **--------------------------------------------------------------------------------------**
 **                   Copyright (c) 2012  ftsafe                                         **
 ******************************************************************************************/

#ifndef _ATR_
#define _ATR_


// 返回值常量
#define ATR_OK					0		// 成功			 
#define ATR_NOT_FOUND			1		// 无数据		 
#define ATR_MALFORMED			2		// ATR解析错误	 
#define ATR_IO_ERROR			3		// I/O 错误		 


// 参数常量
#define ATR_MAX_SIZE 			33		// ATR最大长度  
#define ATR_MAX_HISTORICAL		15		// 历史字节最大长度  
#define ATR_MAX_PROTOCOLS		7		// 协议最大数  
#define ATR_MAX_IB				4		// 每个协议的接口字节的最大数目  
#define ATR_CONVENTION_DIRECT	0		// 直接约定  
#define ATR_CONVENTION_INVERSE	1		// 反公约  
#define ATR_PROTOCOL_TYPE_T0	0		// 协议类型 T=0  
#define ATR_PROTOCOL_TYPE_T1	1		// 协议类型 T=1  
#define ATR_PROTOCOL_TYPE_T2	2		// 协议类型 T=2  
#define ATR_PROTOCOL_TYPE_T3	3		// 协议类型 T=3  
#define ATR_PROTOCOL_TYPE_T14	14		// 协议类型 T=14  
#define ATR_INTERFACE_BYTE_TA	0		// 接口字节 TAi  
#define ATR_INTERFACE_BYTE_TB	1		// 接口字节 TBi  
#define ATR_INTERFACE_BYTE_TC	2		// 接口字节 TCi  
#define ATR_INTERFACE_BYTE_TD	3		// 接口字节 TDi  
#define ATR_PARAMETER_F			0		// 参数 F  
#define ATR_PARAMETER_D			1		// 参数 D  
#define ATR_PARAMETER_I			2		// 参数 I  
#define ATR_PARAMETER_P			3		// 参数 P  
#define ATR_PARAMETER_N			4		// 参数 N  
#define ATR_INTEGER_VALUE_FI	0		// 整型值 FI  
#define ATR_INTEGER_VALUE_DI	1		// 整型值 DI  
#define ATR_INTEGER_VALUE_II	2		// 整型值 II  
#define ATR_INTEGER_VALUE_PI1	3		// 整型值 PI1  
#define ATR_INTEGER_VALUE_N		4		// 整型值 N  
#define ATR_INTEGER_VALUE_PI2	5		// 整型值 PI2  



// 参数缺省值
#define ATR_DEFAULT_F			372
#define ATR_DEFAULT_D			1
#define ATR_DEFAULT_I 			50
#define ATR_DEFAULT_N			0
#define ATR_DEFAULT_P			5

// 定义 bool 数据类型
//typedef int                bool;

// 定义ATR解析数据结构
typedef struct
{
	unsigned length;							// ATR 长度
	unsigned char  TS;							// ATR TS[初始字符]字节值
	unsigned char  T0;							// ATR T0[格式字符]字节值
	struct
	{
		BYTE value;								// TA~TD 位值；
		int present;							// TA~TD 位存在标志，TRUE(1):存在；FALSE(0):不存在
	} ib[ATR_MAX_PROTOCOLS][ATR_MAX_IB], TCK;	// 定义 TA~TD位数据结构,ATR_MAX_PROTOCOLS=7;ATR_MAX_IB = 4;
	unsigned pn;								// TA~TD 数组的个数；即 tb 实际存储的数据个数；
	unsigned char  hb[ATR_MAX_HISTORICAL];		// 历史字节流,ATR_MAX_HISTORICAL=15
	unsigned hbn;								// 历史字节个数
} ATRStruct;



// 将ATR字节流解析到ATRStruct结构
int ftATRInitFromArray (ATRStruct * atr, unsigned char  atr_buffer[ATR_MAX_SIZE], unsigned int length);


// 获取 ATR结构中TD1和TD2的低半字节即协议类型的值,TD1缺少为：0； TD2不存在返回：-1
int ftGetATRT(ATRStruct stATR, int *TD1T,int *TD2T);

// T !=0 的ATR的TCK值计算函数,
int ftATRTCKCal(unsigned char  atr_buffer[ATR_MAX_SIZE],int length ,unsigned int *TCK);


#endif 

/*  字节序号	字节描述				值及说明
	====================================================================================
	0			TS[初始字符]			\x3B:正向约定；\x3F:反向约定(以后版本不再支持)
	1			T0[格式字符]			高半字节：TA1~TD1是否存在，第半字节：李时珍字符数目(0~15) [从高到低分别:TD1~TA1]
	2			TA1						高半字节：FI，用于确定F的值，F为时钟速度转换因子,缺省FI=1,标识 F= 372;
	                                    低半字节：DI，用于确定D的值，D为位速率调节因子，缺省值为：DI=1,即：D=1;
	3			TB1						b1~b5：PI1值，确定IC卡所需的编程电压P的值，=0 表示IC卡不使用VPP
										b6~b7：II值,  确定IC卡所需的最大编程电流I值，=0,PI1=0,不使用
	4			TC1						传输N值，N用于表示增加到最小持续时间的额外保护时间；
	5			TD1						高半字节：表示TA2~TD2是否存在;[从高到低分别:TD2~TA2]
	                                    低半字节：后续信息交换所使用的协议类型,（无TD1位且T=0作为后续传输类型的缺省值）
	6			TA2[特定模式/协商模式]	b8[],b7,b6[保留],
	7			TB2[PI2]				确定IC卡所需的编程电压P的值，有PI2,则PI1无效；
	8			TC2[WI]					专用用T=0协议，传输工作等待时间整数(WI);
	9			TD2						高半字节：表示TA3~TD3是否存在;[从高到低分别:TD3~TA3],低半字节：协议类型；
	10			TA3						如果TD2中指明T=1)回送IC卡的信息域大小整数(IFSI)，可接收的块信息域的最大长度(INF):01~FE
	11			TB3						如果TD2中指明T=1)表明用来计算CWT和BWT的CWI和BWI值，高半字节为：BWI,第半字节为：CWI
	12			TC3						如果TD2中指明T=1)表明用纵向冗余校验(LRC)作为错误代码
	13			TCK[校验字符]			TCK的值应使从T0到TCK在内的所有字节进行异或运算的结果为零；							
*/

/*
 T=0 ATR =3B 6F 00 00 86 05 47 44 11 86 50 32 02 00 01 74 60 47 44             [T0=6F,TC1,TB1存在]
 T=1 ATR =3B 9F 95 81 31 F0 9F 00 66 46 53 00 10 01 21 71 DF 00 00 81 90 00 29 [T0=9F,TD1,TA1存在]

 TA1：缺省：F=372,D=1,TA1= 95,[F=512,D=16]
 TB1：[00]：b1~b5=0：IC卡不适用VPP(编程电压);
 TC1：[00]：传输N值，N用于表示增加到最小持续时间的额外保护时间；
 TD1：无：无后续接口字节；[81]：TD2:存在、协议类型=T=1
 TD2：[31]：TB3,TA3存在，协议类型为：T=1
 TA3：[F0]：
 TB3：[9F]：
 */