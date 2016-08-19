/******************************************************************************************
 ** 文件名称：ATR.h						                                                 **
 ** 文件描述：IC卡上电复位信息处理代码		                                             **
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
#include <windows.h>
#include "ATR.h"


#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif


// 定义一个静态数组
static unsigned atr_num_ib_table[16] =
{
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4
};

/****************************************************************************************** 
 *******    函数名称：ftATRInitFromArray		-序号：1 -							*******
 *******	函数功能：将ATR字节流转换成ATR结构数据									*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：																*******
 *******				ATRStruct *atr：(输出)转换ATR字节流生成的ATR信息结构指针	*******
 *******				unsigned char atr_buffer[]：ATR字节流数组(BCD码)			*******
 *******				unsigned      length：ATR字节流长度							*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：ATR_OK(0)；失败返回：<0	;错误代码如下				******* 
  *******				ATR_MALFORMED(2)：ATR解析错误								*******
 ******************************************************************************************/
int ftATRInitFromArray (ATRStruct * atr, unsigned char  atr_buffer[ATR_MAX_SIZE], unsigned int length)
{
    BYTE TDi;
    unsigned pointer = 0, pn = 0;

    // 检测传入的 ATR 长度参数合法性  
    if (length < 2)
        return (ATR_MALFORMED);

    // 保存ATR 字节流的第1字节，TS[初始字节]  
    atr->TS = atr_buffer[0];

	// 保存ATR 字节流的第2字节，T0[格式字符],高4BIT为：TA1~TD1是否存在；低4BIT为：历史字节数*/
    atr->T0 = TDi = atr_buffer[1];
    pointer = 1;

    // 保存T0 低4BIT的历史字节数  
    atr->hbn = TDi & 0x0F;

    // 缺省的TCK[校验字符] 不存在  
    (atr->TCK).present = FALSE;
  

    // 根据长度循环处理ATR的相关数据  
    while (pointer < length)
    {
        
        // 检测ATR缓冲区大小  
        if (pointer + atr_num_ib_table[(0xF0 & TDi) >> 4] >= length)
        {
            return (ATR_MALFORMED);
        }
        
        // 检测TA[i] 是否存在  
        if ((TDi | 0xEF) == 0xFF)
        {		// 存储 TA[i] 的值
            pointer++;
            atr->ib[pn][ATR_INTERFACE_BYTE_TA].value = atr_buffer[pointer];
            atr->ib[pn][ATR_INTERFACE_BYTE_TA].present = TRUE;
        }
        else
            atr->ib[pn][ATR_INTERFACE_BYTE_TA].present = FALSE;
            
        // 检测TB[i] 是否存在 
        if ((TDi | 0xDF) == 0xFF)
        {		// 存储 TB[i] 的值
            pointer++;
            atr->ib[pn][ATR_INTERFACE_BYTE_TB].value = atr_buffer[pointer];
            atr->ib[pn][ATR_INTERFACE_BYTE_TB].present = TRUE;
        }
        else
            atr->ib[pn][ATR_INTERFACE_BYTE_TB].present = FALSE;

        // 检测TC[i] 是否存在  
        if ((TDi | 0xBF) == 0xFF)
        {		// 存储 TC[i] 的值
            pointer++;
            atr->ib[pn][ATR_INTERFACE_BYTE_TC].value = atr_buffer[pointer];
            atr->ib[pn][ATR_INTERFACE_BYTE_TC].present = TRUE;
        }
        else
            atr->ib[pn][ATR_INTERFACE_BYTE_TC].present = FALSE;

        // 检测TD[i] 是否存在  
        if ((TDi | 0x7F) == 0xFF)
        {
            pointer++;
            
            // 获取TDi的值
            TDi = atr->ib[pn][ATR_INTERFACE_BYTE_TD].value = atr_buffer[pointer];
            atr->ib[pn][ATR_INTERFACE_BYTE_TD].present = TRUE;
            
            // TCK存在
            (atr->TCK).present = ((TDi & 0x0F) != ATR_PROTOCOL_TYPE_T0);
            if (pn >= ATR_MAX_PROTOCOLS)
            		return (ATR_MALFORMED);
            pn++;
        }
        else
        {
            atr->ib[pn][ATR_INTERFACE_BYTE_TD].present = FALSE;
            break;
        }
    }// ATR数据循环处理结束


    // TA~TD 数据结构个数  
    atr->pn = pn + 1;

    // 检测ATR中的历史字节数和合法性 
    if (pointer + atr->hbn >= length)
        return (ATR_MALFORMED);

	// 保存历史字节数 
    memcpy (atr->hb, atr_buffer + pointer + 1, atr->hbn);
    pointer += (atr->hbn);

    // 检测TCK 是否存在，存在则保存TCK的值  
    if ((atr->TCK).present)
    {

        if (pointer + 1 >= length)
            return (ATR_MALFORMED);

        pointer++;

        (atr->TCK).value = atr_buffer[pointer];
    }

	// 保存 ATR的长度
    atr->length = pointer + 1;
    
    // 返回成功
    return (ATR_OK);
}


/****************************************************************************************** 
 *******    函数名称：ftGetATRT					-序号：2 -							*******
 *******	函数功能：获取ATR结构中TD1和TD2的协议类型(值为：0~F)					*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：																*******
 *******				ATRStruct stATR：复位应答信息(ATR)结构数据					*******
 *******				int      *TD1T： (输出)ATR中TD1定义的协议类型,				*******
 *******                        缺省为：0，即T=0,存在则TD1的低半字节为:0或1,		*******
 *******                int      *TD2T： (输出)ATR中TD2定义的协议类型，				*******	
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：ATR_OK(0)；失败返回：<0	;错误代码如下				******* 
 ******************************************************************************************/
int ftGetATRT(ATRStruct stATR, int *TD1T,int *TD2T)
{
		int nTD1T,nTD2T;
	
		// 缺省值为：T=0 
		nTD1T = 0; 
		
		// 缺省值为：TD2 不存在
		nTD2T = -1;

		// 获取TD1低半字节的值，缺省设置T=0 
		if (stATR.ib[0][3].present == TRUE)  
		{
			// 取TD1的低4位，
			nTD1T = stATR.ib[0][3].value & 0x0F;
		}

		// 获取TD2低半字节的值
		if( stATR.ib[1][3].present == TRUE)
		{
			nTD2T = stATR.ib[1][3].value & 0x0F;
		}

		// 设置输出变量值
		*TD1T = nTD1T;
		*TD2T = nTD2T;

		return ATR_OK;
}


/****************************************************************************************** 
 *******    函数名称：ftATRTCKCal				-序号：3 -							*******
 *******	函数功能：ATR字节流中TCK计算函数										*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：																*******
 *******				unsigned char atr_buffer[]：ATR字节流数组[BCD码]			*******
 *******				int           length      ：ATR字节流长度[整个长度]			*******
 *******                unsigned int  *TCK		  ：(输出)计算出的ATR的TCK值		*******	
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：ATR_OK(0)；失败返回：<0	;错误代码如下				******* 
 ******************************************************************************************/
int ftATRTCKCal(unsigned char  atr_buffer[ATR_MAX_SIZE],int length ,unsigned int *TCK)
{
		unsigned char nCH;
		int i ;

		nCH = atr_buffer[1];

		// TCK 
		for(i = 2; i< (length -1) ;i++)
		{
			nCH = (unsigned char )atr_buffer[i] ^ (unsigned char)nCH;
		}
	
		*TCK = nCH;

		return ATR_OK;
}


 
