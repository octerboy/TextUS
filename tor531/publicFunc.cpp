/******************************************************************************************
 ** 文件名称：publicFunc.cpp                                                             **
 ** 文件描述：公共函数                                                                   **
 **--------------------------------------------------------------------------------------**
 ** 创 建 人：智能卡业务部                                                               **
 ** 创建日期：2012-6-18                                                                  **
 **--------------------------------------------------------------------------------------**
 ** 修 改 人：                                                                           **
 ** 修改日期：                                                                           **
 **--------------------------------------------------------------------------------------**
 ** 版 本 号：                                                                           **
 **--------------------------------------------------------------------------------------**
 **                     Copyright (c) 2012  ftsafe                                       **
 ******************************************************************************************/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "DES.h"

#include "publicFunc.h"


/******************************************************************************************
 *******	函数名称：ftAtoh		-序号：1-										*******
 *******	函数功能：ASCII 码转换成 BCD码 即16进制码								*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：																*******
 *******			 char *ascstr:需转换的ASCII字符串								*******
 *******			 char *bcdstr:转换生成的BCD码									*******
 *******			 int  bcdlen:生成的BCD码长度									*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：生成的BCD码													*******
 ******************************************************************************************/
char *  ftAtoh(char *ascstr, char *bcdstr,int bcdlen)
{
    unsigned char hi,lo;
    int  i,n;

    for(i=n=0; n<bcdlen;) 
	{
        hi = toupper(ascstr[i++]);
        lo = toupper(ascstr[i++]);
        bcdstr[n++] = (((hi>='A')?(hi-'A'+10):(hi-'0'))<<4)|((lo>='A')?(lo-'A'+10):(lo-'0'));
    }
    return(bcdstr);
}


/******************************************************************************************
 *******	函数名称：ftHtoa		-序号：2-										*******
 *******	函数功能：BCD码(即16进制)转换成ASCII									*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：																*******
 *******			char *hexstr:需转换的BCD码										*******
 *******			char *ascstr:生成的ASCII码										*******
 *******			int  length:需要转换的BCD码的长度								*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功：返回生成的ASCII长度；失败返回：0						*******
 ******************************************************************************************/
int  ftHtoa(char *hexstr, char *ascstr,int  length)
{
    int    h,a;
    unsigned char uc;

    ascstr[0] = 0x0;
    if ( length < 1 ) return 0;

    h = length-1;
    a = length+length-1;
    ascstr[a+1]='\0';
    while(h>=0) 
	{
       uc = hexstr[h]&0x0f;
       ascstr[a--] = uc + ((uc>9)?('A'-10):'0');
       uc = (hexstr[h--]&0xf0)>>4;
       ascstr[a--] = uc + ((uc>9)?('A'-10):'0');
    }

	ascstr[length*2]='\0';
    return length*2;
}


/******************************************************************************************
 *******	函数名称：ftStringToUpper	   -序号：3-								*******
 *******	函数功能：将字符串中的字母转换成大写									*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：char *Str:(输入/输出)需要进行转换的字符						*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功：返回生成的ASCII长度；失败返回：0						*******
 ******************************************************************************************/
void ftStringToUpper(char *Str)
{
	int nLen; 
	int i;
	char *pStr;
	
	nLen = (int)strlen(Str);

	pStr = (char *)malloc(nLen+1);

	memset(pStr,0x00,nLen+1);

	memcpy(pStr,Str,nLen);

	for(i=0;i<nLen;i++)
	{
		if( pStr[i] >= 'a' && pStr[i] <='z')
		{
			pStr[i] -= 32;
		}
	}
	
	memcpy(Str,pStr,nLen);
	free(pStr);
	pStr = NULL;
	return ;
}


/******************************************************************************************
 *******	函数名称：ftStringToLower	   -序号：4-								*******
 *******	函数功能：将字符串中的字母转换成小写									*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：char *Str:(输入/输出)需要进行转换的字符						*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功：返回生成的ASCII长度；失败返回：0						*******
 ******************************************************************************************/
void ftStringToLower(char *Str)
{
	int nLen; 
	int i;
	char *pStr;
 
    nLen = (int)strlen(Str);
	pStr = (char *)malloc((unsigned int)nLen+1);

	memset(pStr,0x00,nLen+1);

	//strcpy_s(pStr,nLen,Str);
	memcpy(pStr,Str,nLen);

	for(i=0;i<nLen;i++)
	{
		if( pStr[i] >= 'A' && pStr[i] <='Z')
		{
			pStr[i] += 32;
		}
	}
	
	memcpy(Str,pStr,nLen);
	free(pStr);
	pStr = NULL;
	return ;
}


/******************************************************************************************
 *******	函数名称：ftCalXOR	   -序号：5-										*******
 *******	函数功能：异或计算函数													*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：																******* 
 *******			 char *Param1：要进行异或计算的第一个参数						*******
 *******			 char *Param2：要进行异或计算的第二个参数						******* 
 *******			 int   Len：   要进行异或计算的数据长度							*******
 *******			 char *Out：   (输出)异或计算返回值								*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功：成功返回：0；失败返回：-1								*******
 ******************************************************************************************/
int ftCalXOR(char *Param1,char *Param2,int Len,char *Out)
{
	int i = 0;
	char *pVal;

	if( Len <= 0)
	{
		return -1;
	}
    
	pVal = (char *)malloc(Len);

	memset(pVal,0x00,Len);
    
	for(i=0; i<Len;  i++)
	{
	    pVal[i] = Param1[i] ^ Param2[i];
	}
    
    memcpy(Out,pVal,Len);
	free(pVal);
	pVal =NULL;
	return 0;
}



/******************************************************************************************
 *******	函数名称：ftHexToLong	   -序号：6-									*******
 *******	函数功能：将16进制的金额转换成长整型									*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：																*******
 *******			char *hexstr:十六进制金额值字符串(如："65F")					*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功：失败返回：-1；成功返回：长整型的金额值					*******
 ******************************************************************************************/
long ftHexToLong(char *HexStr)
{
	int nFlag = 0;
	long lVal;
	int nLen;
	int i;


	nLen = (int)strlen(HexStr);

	for(i=0;i<nLen;i++)
	{
        if(!(isxdigit(HexStr[i])))
		{
            nFlag = 1;
		}
	}
    
    if(nFlag)
	{
		return -1;
	}

    lVal = strtol(HexStr,NULL,16);
	return lVal;
}




/******************************************************************************************
 *******	函数名称：ftLongToHex	   -序号：7-									*******
 *******	函数功能：十进制金额值转换成16进制金额字符串							*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：																*******
  *******			long Val：要转换的十进制金额值									*******
 *******			char *hexstr:(输出)转换生成的16进制金额值字符串					*******
 *******			int Flag：金额值输出类型标志；缺省值为1：BCD码；0：16进制字符串	*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功：失败返回：-1；成功返回：生成的16进制金额值字符串长度	*******
 ******************************************************************************************/
int ftLongToHex(long Val,char *HexStr,int Flag )
{
	int nLen;
	long MaxVal = 2147483647 ;
	long aHexVal[10];
	long lVal;
	long lInVal;
	char szHexStr[10+1];
	char szAscStr[10+1];
	int i = 0;

	memset(szHexStr,0x00,sizeof(szHexStr));
	memset(szAscStr,0x00,sizeof(szAscStr));

	if( (Val > MaxVal)  || Val < 0)
	{
		return -1;
	}

	for(i = 0 ; i < 10;i++)
	{
        aHexVal[i] = 0L;
	}
	
	i = 0;
	lInVal = Val;
	while(1)
	{
		lVal = lInVal % 16 ; 
        aHexVal[i] = lVal;		
		i++;
		lInVal = lInVal / 16;
		if(lInVal == 0)
		{
			break;
		}
	}

	nLen = i;			// ASCII码数据长度

	for(i=0;i<nLen;i++)
	{
        if(aHexVal[i]>=0 && aHexVal[i]<=9)
		{
		    szAscStr[i] = (int)aHexVal[i] + 48 ;	
		}else 
		{
            szAscStr[i] = ((int)aHexVal[i]-10) + 'A'; 
		}
	}
	

	memset(szHexStr,0x00,sizeof(szHexStr));

	for(i=0;i<nLen;i++)
	{
		szHexStr[nLen-1-i] = szAscStr[i];
	}

	memset(szAscStr,0x00,sizeof(szAscStr));

	i = nLen % 2;

	if( i>0)
	{
		memcpy(szAscStr,"0",1);
		memcpy(szAscStr+1,szHexStr,nLen);
		nLen+=1;
	}else 
	{
		memcpy(szAscStr,szHexStr,nLen);
	}

	// Flag 参数为：0 返回的金额为16进制的ASCII字符串金额
	if( Flag == 0)
	{
		memcpy(HexStr,szAscStr,nLen);
		return nLen;
	}

	// Flag 参数为：1，返回的金额为BCD码的金额 
	nLen = nLen/2;
	memset(szHexStr,0x00,sizeof(szHexStr));
    
	ftAtoh(szAscStr,szHexStr,nLen);
	memcpy(HexStr,szHexStr,nLen);

	return nLen;
}


/******************************************************************************************
 *******	函数名称：ftLongToAmount   -序号：7A-									*******
 *******	函数功能：十进制金额值转换成16进制金额字符串(8个ASCII字符前补0)			*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：																*******
  *******			long Val：要转换的十进制金额值									*******
 *******			char *hexstr:(输出)转换生成的16进制金额值字符串					*******
 *******			int Flag：金额值输出类型标志；缺省值为1：BCD码；0：16进制字符串	*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功：失败返回：-1；成功返回：生成的16进制金额值字符串长度	*******
 ******************************************************************************************/
int ftLongToAmount(long Val,char *HexStr)
{
	char szAmount[9];
	char szVal[29];
	int  nLen;
	int  nLen2;

	memset(szAmount,0x00,sizeof(szAmount));
	memset(szVal,0x00,sizeof(szVal));

	ftLongToHex(Val,szVal);

	nLen = (int)strlen(szVal);
	if( nLen >=8) 
	{
		memcpy(HexStr,szVal,8);
		return 0;
	}

	nLen2 = 8 - nLen;

	// 设置前面要补充的：0
	memcpy(szAmount,"00000000",nLen2);
	memcpy(szAmount+nLen2,szVal,nLen);

	memcpy(HexStr,szAmount,8);

	return 0;

}

/******************************************************************************************
 *******	函数名称：ftFillAmount	   -序号：8-									*******
 *******	函数功能：将BCD码的金额字符串填充到4字节长度(前补0x00)					*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：																*******
 *******			char *Str：(输入/输出)要进行填充的16进制金额字符串				*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功：失败返回：-1；成功返回：返回：0或8						*******
 ******************************************************************************************/
 int  ftFillAmount(char *Str)
{
	char sFill[8+1];
	char sAmount[8+1];
	int nLen;
	int nFillLen;

	memset(sFill,  0x00,sizeof(sFill));
	memset(sAmount,0x00,sizeof(sAmount));

	strcpy(sFill,"00000000");
	nLen = (int)strlen(Str);

	if(nLen > 8)
	{
		return -1;
	}

	if(nLen == 8)
	{
		return 0;
	}

	// 计算要填充的字符数
	nFillLen = 8 - nLen;
	//strncpy(sAmount,sFill,nFillLen);
	memcpy(sAmount,sFill,nFillLen);
	memcpy(sAmount+nFillLen,Str,nLen);

	memcpy(Str,sAmount,8);
	return 8;
}


/******************************************************************************************
 *******	函数名称：ftFillString	   -序号：9-									*******
 *******	函数功能：MAC/TAC计算的字符串填充函数(填充80或8000..到8的倍数长度)		*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：																*******
 *******			char *Str：(输入/输出)要进行填充的16进制字符串					*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功：失败返回：-1；成功返回：填充后的ASCII数据长度			*******
 ******************************************************************************************/
int  ftFillString(char *Str)
{
	char sFill[8*2+1];			 		
	int nLen;
	int nFillLen;

	nLen = (int)strlen(Str);			 
	if(nLen<=0)
	{
		return -1;
	}
	

	nFillLen = nLen % 16;	
	nFillLen = 16 - nFillLen;	 


	// 填充字符串
	memset(sFill,0x00,sizeof(sFill));
	memcpy(sFill,"8000000000000000",16);	
	
	memcpy(Str+nLen,sFill,nFillLen);

	memcpy(Str+nLen+nFillLen,"\x00",1);

	return nLen + nFillLen;

}


/******************************************************************************************
 *******	函数名称：ftFillString	   -序号：10-									*******
 *******	函数功能：将字符串填充指定字符到指定长度,大于此长度则截取到此长度		*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：																*******
 *******			char *Str：(输入/输出)要进行填充的字符串						*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功：失败返回：-1；成功返回：0								*******
 ******************************************************************************************/
int  ftFillStringF(char *Str,int Len,char Val)
{
	char *sFill;
	char *pData;
	int nLen;
	int nFillLen;

	pData = (char *)malloc(Len +1);
	memset(pData,0x00,Len+1);

	nLen = (int)strlen(Str);			 
	if(nLen<=0)
	{
		free(pData);
		pData = NULL;
		return -1;
	}

	if(nLen > Len)
	{
		memcpy(pData,Str,Len);
		memcpy(Str,pData,Len+1);   // 将 0x00 字符返回	
		free(pData);
		pData = NULL;
		return 0;
	}

	if(nLen == Len)
	{
		free(pData);
		pData = NULL;
		return 0;
	}

	memcpy(pData,Str,nLen);
	nFillLen = Len -nLen  ;

	sFill = (char *)malloc(nFillLen); 
	memset(sFill,Val,nFillLen);

	memcpy(pData+nLen,sFill,nFillLen);
	memcpy(Str,pData,Len+1);           // 设置应答数据时后面增加一个'\0' 字符串结束字符
	
	free(sFill);
	sFill = NULL;
	
	free(pData);
	pData = NULL;

	return 0;
}



 
/******************************************************************************************
 *******	函数名称：ftCalLRC	   -序号：11-										*******
 *******	函数功能：数据异或和计算函数											*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：char *DataBuf：(输入输出)要进行异或和计算的缓冲区				*******
 *******              int   Len:数据长度											*******
 *******			  int Flag：数据格式标准；1：BCD码；0:ASCII(缺省值)	;			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：-1		          						******* 
 ******************************************************************************************/
int  ftCalLRC(char *DataBuf,int Len,int Flag)
{
	char *pBuf;
	char  szHexVal[4+1];	// 异或和计算值BCD码
	char cVal;

	
	int i;
	int nBcdLen = 0;
	
	// 分配字符指针
	pBuf = (char *)malloc(Len+2);
	memset(pBuf,0x00,Len+2);



	if(Flag == 0)		//  非零计算数据为ASCII数据
	{
		nBcdLen = Len / 2;
		
		ftAtoh(DataBuf,pBuf,nBcdLen);			
	}else				// 为零计算数据为BCD数据
	{
		nBcdLen = Len;
		memcpy(pBuf,DataBuf,Len);

	}

	cVal = pBuf[0];

	for(i=1; i < nBcdLen ; i++)
	{
		cVal = cVal ^ pBuf[i];
	}


	// 将计算出来的异或和追加到输出变量后面
	if(Flag == 0)		// 计算数据为ASCII数据
	{
		memset(szHexVal,0x00,sizeof(szHexVal));
		sprintf(szHexVal,"%02.2X",(unsigned char)cVal);
		memcpy(DataBuf+Len,szHexVal,2);
	}else				// 计算数据为BCD码数据
	{
		memset(szHexVal,0x00,sizeof(szHexVal));
		szHexVal[0] = (unsigned char)cVal;
		
		memcpy(DataBuf+Len,"\x85",1);
		//memcpy(DataBuf+Len,szHexVal,1);
	}

	free(pBuf);
	pBuf = NULL;

	return 0;
}

// Flag 缺省值为：= 0；
int  ftCalLRC1(char *DataBuf,int Len,char *RetVal,int Flag)
{
	char *pBuf;
	char  szHexVal[4+1];	// 异或和计算值BCD码
	char cVal;

	
	int i;
	int nBcdLen = 0;
	
	// 分配字符指针
	pBuf = (char *)malloc(Len+2);
	memset(pBuf,0x00,Len+2);



	if(Flag == 0)		//  零计算数据为ASCII数据
	{
		nBcdLen = Len / 2;
		
		ftAtoh(DataBuf,pBuf,nBcdLen);			
	}else				// 零计算数据为BCD数据
	{
		nBcdLen = Len;
		memcpy(pBuf,DataBuf,Len);

	}

	cVal = pBuf[0];

	for(i=1; i < nBcdLen ; i++)
	{
		cVal = cVal ^ pBuf[i];
	}

	memset(szHexVal,0x00,sizeof(szHexVal));

	szHexVal[0] = cVal; 

	memcpy(RetVal,szHexVal,2);

	/*
	if(Flag == 0)
	{
		memset(szHexVal,0x00,sizeof(szHexVal));
		memset(pBuf,0x00,4);

		sprintf(pBuf,"%2.2X",(unsigned int) cVal);
		memcpy(RetVal,pBuf,2);
	}*/
 
	free(pBuf);
	pBuf = NULL;

	return 0;
}

/******************************************************************************************
 *******	函数名称：ftCharToBitString	   -序号：12-								*******
 *******	函数功能：将字符转换成BIT字符串1字节转换成8个0、1的字符函数				*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：unsigned char DataBuf：要进行转换的字符						*******
 *******              int   Len:数据长度											*******
 *******			  char *OutBuf：转换生成的BIT字符数组							*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：-1		          						******* 
 ******************************************************************************************/
int ftCharToBitString(unsigned char Val,char *OutBuf)
{
		
	char szBitString[8+1];
	unsigned  int nVal;

	memset(szBitString,0x00,sizeof(szBitString));
	memcpy(szBitString,"00000000",8);

	for(int i=0;i<8;i++)
	{
		nVal = (unsigned int)pow((float)2,(int)7-i);
		if( Val & nVal )
		{
			szBitString[i] = '1';		
		}
	}

	memcpy(OutBuf,szBitString,8);

	return 0;

}

 
/******************************************************************************************
 *******	函数名称：ftDataToBitString	   -序号：13-								*******
 *******	函数功能：将BCD码数据转换成BIT字符串1字节转换成8个0、1的字符函数		*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：char *InData：要进行转换的数据指针							*******
 *******              int   Len:数据长度											*******
 *******			  char *OutBuf：转换生成的BIT字符数组							*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：-1		          						******* 
 ******************************************************************************************/
int ftDataToBitString(char *InData,int Len,char *OutBuf)
{
	char szCharStr[8+1];		 

	char *pOutBuf;
	int nOutLen;

	nOutLen = 8 * Len +1;

	//  分配输出指针
	pOutBuf = (char *)malloc(nOutLen);

	memset(pOutBuf,0x00,nOutLen);

	for(int i = 0;i< Len;i++)
	{
		memset(szCharStr,0x00,sizeof(szCharStr));

		ftCharToBitString((unsigned char)InData[i],szCharStr);

		memcpy(pOutBuf+i*8,szCharStr,8);

	}

	// 设置输出变量
	memcpy(OutBuf,pOutBuf,8*Len);

	free(pOutBuf);
	pOutBuf = NULL;

	return 8*Len;
}




/******************************************************************************************
 *******	函数名称：ftBitStringToVal	   -序号：14-								*******
 *******	函数功能：将BIT数组转换成整型值											*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：char *BitString：要进行转换的BIT指针							*******
 *******              int   Len:BIT指针长度											*******
 *******			  int  *Val：(输出)	Bit转换生成的INT值							*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：-1		          						*******
 ******************************************************************************************/
int ftBitStringToVal(char *BitString,int Len,int *Val)
{
	char *szBitString;	
	int nLen;
	int nVal = 0;
	int nReturn =0;

	nLen = Len;

	szBitString = (char *)malloc(nLen +1);

	memset(szBitString,0x00,nLen+1);

	memcpy(szBitString,BitString,Len);

	for(int i=0;i< Len;i++)
	{
		if( szBitString[i] == '1')
		{
			nVal =  (int)pow((double)2,(int)Len-1-i);
			if(nVal >0)
			{
				nReturn += nVal;
			}
		}
	}

	*Val = nReturn;
	return nReturn;
}




/******************************************************************************************
 *******	函数名称：ftDataEnc	   -序号：15-										*******
 *******	函数功能：数据加密函数(用于密码的简单加密)								*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：char *pIn:要加密的数据										*******
 *******              int   Len:要加密的数据长度									*******
 *******			  char *pOut:(输出)	加密生成数据								*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：-1		          						*******
 ******************************************************************************************/
int ftDataEnc(char *pIn,int pLen,char *pOut)
{
	char szKey1[32+1];
	char szData[32+1];

	memset(szKey1,0x00,sizeof(szKey1));
	memset(szData,0x00,sizeof(szData));

	if( pLen > 32)
	{
		return -1;
	}
	
	// 设置加密因子
	memcpy(szKey1,"a1g4d8k7i0f6m2ue3r5yo9bxcztvlrhv",32);

	for(int i = 0;i< pLen ;i++ )
	{
		szData[i] = pIn[i] ^ szKey1[i];	
	}

	memcpy(pOut,szData,pLen);
	
	return 0;

}



/******************************************************************************************
 *******	函数名称：ftDataDec	   -序号：16-										*******
 *******	函数功能：数据解密函数(用于密码的简单解密)								*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：char *pIn:要解密的数据										*******
 *******              int   pLen:要解密的数据长度									*******
 *******			  int  *pOut：(输出)解密生成数据								*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：-1		          						*******
 ******************************************************************************************/
int ftDataDec(char *pIn,int pLen,char *pOut)
{
	char szKey[32+1];
	char szData[32+1];

	memset(szKey,0x00,sizeof(szKey));
	memset(szData,0x00,sizeof(szData));

	if( pLen > 32)
	{
		return -1;
	}
	
	// 设置加密因子
	memcpy(szKey,"a1g4d8k7i0f6m2ue3r5yo9bxcztvlrhv",32);
	for(int i = 0;i< pLen ;i++ )
	{
		szData[i] = pIn[i] ^ szKey[i];	
	}

	memcpy(pOut,szData,pLen);
	
	return 0;
}


/******************************************************************************************
 *******	函数名称：ftDiversify   -序号：17-										*******
 *******	函数功能：密钥分散函数													*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：char *Data：  分散因子，通常为8字节的卡号	    ASCII码值		*******
 *******			  char *KEY：   要进行分散的KEY				    ASCII码值		*******
 *******			  char *Out：   密钥分散输出数据(即过程密钥SK)	ASCII码值		*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回：-1		          						*******
 ******************************************************************************************/
int ftDiversify(char *Data,char *Key,char *Out)
{
	char szHexData[16+1];
	char szHexKey[16+1];
	char szHexOut[16+1];
	char szOut[32+1];

	int i;

	memset(szHexData, 0x00, sizeof(szHexData));
	memset(szHexKey,  0x00, sizeof(szHexKey));
	memset(szHexOut,  0x00, sizeof(szHexOut));
	memset(szOut,     0x00, sizeof(szOut));


	// 检查传入数据的合法性；分散因子为16个ASCII字符
	if(strlen(Data) != 16)
	{
         return -1;
	}

	// 分散计算密钥为32个ASCII字符
	if(strlen(Key) != 32 )
	{
		return -2;
	}

	// 将传入数据转换成16进制的BCD码
	ftAtoh(Data,szHexData,8);
	ftAtoh(Key,szHexKey,16);

	// 对传入分散因子取反并添加到数据后面生成16字节的加密数据
	for(i = 0;i < 8;i++)
	{
		szHexData[8+i] =~((unsigned char )szHexData[i]);	
	}

	// 对处理后的分散因子进行3DES加密,生成过程密钥SK
	CDES::RunDes(0,szHexData,16,szHexKey,16,szHexOut);
	
	// 将16进制的过程密钥转换成ASCII码的过程密钥
	ftHtoa(szHexOut,szOut,16);
	
	// 将过程密钥保存的函数输出变量
	memcpy(Out,szOut,32);

	return 0;
}

/******************************************************************************************
 *******	函数名称：ftDesEnc   -序号：18-											*******
 *******	函数功能：DES数据加密													*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：char *pKey   :要进行DES加密的密钥,ASCII码16字节，BCD为8字节	*******
 *******			  int   pKeyLen:密钥长度,值为8:BCD码;16:ASCII码					*******
 *******			  char *pIn    :加密数据，ASCII为16字节、BCD码为8字节的倍数 	*******
 *******			  char *pOut   :(输出)加密生成的数据							*******
 *******              int   pLen   :要加密的数据长度必须是8的倍数					*******
 *******              int   pFlag  :数据格式标志，缺省为：0，BCD码；其他值为ASCII   *******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回<0	;错误代码如下：						*******
 *******								-1：输入密钥错误或输入加密数据错误			*******
 *******								-2：内部分配指针失败						*******
 ******************************************************************************************/
int ftDesEnc(char *pKey,int pKeyLen,char *pIn,char *pOut,int Len,int pFlag)
{
	unsigned char szHexKey[8+1];		// 十六进制的KEY
	unsigned char *pHexData;
	unsigned char szTmpInData[8+1];		// 加密用输入变量
	unsigned char szTmpOutData[8+1];	// 加密临时输出变量
	unsigned char *pOutData;			// 临时输出的ASCII数据
	unsigned char *pHexOutData;			// 临时输出的十六进制数据

	int   nHexDataLen;					// 十六进制的数据长度
	int   nDataItem;					// 数据项个数
	int   i;

	memset(szHexKey,0x00,sizeof(szHexKey));

	// 根据传入的数据类型检查传入数据的合法性
	if(pFlag == 0)						// 传入数据为BCD码
	{
		if((pKeyLen != 8)  || ((Len % 8) != 0) || (Len == 0)  )
		{
			return -1;
		}
		
		memcpy(szHexKey,pKey,8);		// 加密密钥
		nHexDataLen = Len;
		
		// 分配指针
		pHexData     = new unsigned char [Len +1];
		pHexOutData  = new unsigned char [Len +1];
		pOutData     = new unsigned char [Len +1];
		if( (pHexData == NULL) || (pHexOutData == NULL) || (pOutData == NULL ) )
		{
			return -2;
		}

		memset(pHexData   ,0x00,Len+1);
		memset(pHexOutData,0x00,Len+1);
		memset(pOutData   ,0x00,Len+1);

		memcpy(pHexData,pIn,Len);
	}else								// 传入数据为ASCII码
	{
		if( (pKeyLen != 16) || ((Len % 16 ) != 0)  || ( Len == 0))
		{
			return -1;
		}
		
		// 将ASCII码的密钥转换成BCD码的密钥
		ftAtoh(pKey,(char *)szHexKey,8);

		// 将ASCII码的加密数据转换成BCD码的数据 
		nHexDataLen  = Len / 2;

		// 分配指针
		pHexData     = new unsigned char [nHexDataLen +1];
		pHexOutData  = new unsigned char [nHexDataLen +1];
		pOutData     = new unsigned char [Len +1];

		if( (pHexData == NULL) || (pHexOutData == NULL) || (pOutData == NULL) )
		{
			return -2;
		}

		memset(pHexData,0x00,nHexDataLen +1);
		memset(pHexOutData,0x00,nHexDataLen +1);
		memset(pOutData,0x00,Len +1);

		// 将传入的ASCII码数据转换成
		ftAtoh(pIn,(char *)pHexData,nHexDataLen);
		
	}
 
	// 计算用进行计算的次数
	nDataItem = nHexDataLen / 8;

	for(i = 0;i< nDataItem ;i++)
	{
		memset(szTmpInData,0x00,sizeof(szTmpInData));
		memset(szTmpOutData,0x00,sizeof(szTmpOutData));

		//memcpy(pHexData + i*8,szTmpInData,8);
		memcpy(szTmpInData,pHexData + i*8,8);
		// DES数据加密
		CDES::RunDes(0,(char *)szTmpInData,8,(char *)szHexKey,8,(char *)szTmpOutData);

		//保存加密的数据
		memcpy(pHexOutData+i*8,szTmpOutData,8);		
	}

	if( pFlag == 0)						// 传入数据为BCD码
	{
		memcpy(pOut,pHexOutData,nHexDataLen);

	}else 
	{
		// 将加密的16进制数据转换成ASCII
		ftHtoa((char *)pHexOutData,(char *)pOutData,nHexDataLen);
		memcpy(pOut,pOutData,nHexDataLen*2);		
	}

	// 释放指针
	free(pHexData);
	free(pHexOutData);
	free(pOutData);
	
	pHexData    = NULL;
	pHexOutData = NULL;
	pOutData    = NULL;

	return 0;
}

/******************************************************************************************
 *******	函数名称：ft3DesEnc   -序号：19-										*******
 *******	函数功能：3DES数据加密													*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：char *pKey   :要进行3DES加密的密钥,ASCII码32字节，BCD为16字节	*******
 *******			  int   pKeyLen:密钥长度,值为16:BCD码;32:ASCII码				*******
 *******			  char *pIn    :加密数据，ASCII为16字节、BCD码为8字节的倍数 	*******
 *******			  char *pOut   :(输出)加密生成的数据							*******
 *******              int   pLen   :要加密的数据长度必须是8的倍数					*******
 *******              int   pFlag  :数据格式标志，缺省为：0，BCD码；其他值为ASCII   *******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回<0	;错误代码如下：						*******
 *******								-1：输入密钥错误或输入加密数据错误			*******
 *******								-2：内部分配指针失败						*******
 ******************************************************************************************/
int ft3DesEnc(char *pKey,int pKeyLen,char *pIn,char *pOut,int Len,int pFlag)
{
	unsigned char szHexKey[16+1];		// 十六进制的KEY
	unsigned char *pHexData;
	unsigned char szTmpInData[8+1];		// 加密用输入变量
	unsigned char szTmpOutData[8+1];	// 加密临时输出变量
	unsigned char *pOutData;			// 临时输出的ASCII数据
	unsigned char *pHexOutData;			// 临时输出的十六进制数据

	int   nHexDataLen;					// 十六进制的数据长度
	int   nDataItem;					// 数据项个数
	int   i;

	memset(szHexKey,0x00,sizeof(szHexKey));

	// 根据传入的数据类型检查传入数据的合法性
	if(pFlag == 0)						// 传入数据为BCD码
	{
		if((pKeyLen != 16)  || ((Len % 8) != 0) || (Len == 0)  )
		{
			return -1;
		}
		
		memcpy(szHexKey,pKey,16);		// 加密密钥
		nHexDataLen = Len;
		
		// 分配指针
		pHexData     = new unsigned char [Len +1];
		pHexOutData  = new unsigned char [Len +1];
		pOutData     = new unsigned char [Len +1];
		if( (pHexData == NULL) || (pHexOutData == NULL) || (pOutData == NULL ) )
		{
			return -2;
		}

		memset(pHexData   ,0x00,Len+1);
		memset(pHexOutData,0x00,Len+1);
		memset(pOutData   ,0x00,Len+1);

		memcpy(pHexData,pIn,Len);
	}else								// 传入数据为ASCII码
	{
		if( (pKeyLen != 32) || ((Len % 16 ) != 0)  || ( Len == 0))
		{
			return -1;
		}
		
		// 将ASCII码的密钥转换成BCD码的密钥
		ftAtoh(pKey,(char *)szHexKey,16);

		// 将ASCII码的加密数据转换成BCD码的数据 
		nHexDataLen  = Len / 2;

		// 分配指针
		pHexData     = new unsigned char [nHexDataLen +1];
		pHexOutData  = new unsigned char [nHexDataLen +1];
		pOutData     = new unsigned char [Len +1];

		if( (pHexData == NULL) || (pHexOutData == NULL) || (pOutData == NULL) )
		{
			return -2;
		}

		memset(pHexData,0x00,nHexDataLen +1);
		memset(pHexOutData,0x00,nHexDataLen +1);
		memset(pOutData,0x00,Len +1);

		// 将传入的ASCII码数据转换成
		ftAtoh(pIn,(char *)pHexData,nHexDataLen);
		
	}
 
	// 计算用进行计算的次数
	nDataItem = nHexDataLen / 8;

	for(i = 0;i< nDataItem ;i++)
	{
		memset(szTmpInData,0x00,sizeof(szTmpInData));
		memset(szTmpOutData,0x00,sizeof(szTmpOutData));

		//memcpy(pHexData + i*8,szTmpInData,8);
		memcpy(szTmpInData,pHexData + i*8,8);
		// DES数据加密
		CDES::RunDes(0,(char *)szTmpInData,8,(char *)szHexKey,16,(char *)szTmpOutData);

		//保存加密的数据
		memcpy(pHexOutData+i*8,szTmpOutData,8);		
	}

	if( pFlag == 0)						// 传入数据为BCD码
	{
		memcpy(pOut,pHexOutData,nHexDataLen);

	}else 
	{
		// 将加密的16进制数据转换成ASCII
		ftHtoa((char *)pHexOutData,(char *)pOutData,nHexDataLen);
		memcpy(pOut,pOutData,nHexDataLen*2);		
	}

	// 释放指针
	free(pHexData);
	free(pHexOutData);
	free(pOutData);
	
	pHexData    = NULL;
	pHexOutData = NULL;
	pOutData    = NULL;

	return 0;
}


/******************************************************************************************
 *******	函数名称：ftDesDec   -序号：20-											*******
 *******	函数功能：DES数据解密													*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：char *pKey   :要进行DES解密的密钥,ASCII码16字节，BCD为8字节	*******
 *******			  int   pKeyLen:密钥长度,值为8:BCD码;16:ASCII码					*******
 *******			  char *pIn    :解密数据，ASCII为16字节、BCD码为8字节的倍数 	*******
 *******			  char *pOut   :(输出)解密生成的数据							*******
 *******              int   pLen   :要解密的数据长度必须是8的倍数					*******
 *******              int   pFlag  :数据格式标志，缺省为：0，BCD码；其他值为ASCII   *******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回<0	;错误代码如下：						*******
 *******								-1：输入密钥错误或输入加密数据错误			*******
 *******								-2：内部分配指针失败						*******
 ******************************************************************************************/
int ftDesDec(char *pKey,int pKeyLen,char *pIn,char *pOut,int Len,int pFlag)
{
	unsigned char szHexKey[8+1];		// 十六进制的KEY
	unsigned char *pHexData;
	unsigned char szTmpInData[8+1];		// 加密用输入变量
	unsigned char szTmpOutData[8+1];	// 加密临时输出变量
	unsigned char *pOutData;			// 临时输出的ASCII数据
	unsigned char *pHexOutData;			// 临时输出的十六进制数据

	int   nHexDataLen;					// 十六进制的数据长度
	int   nDataItem;					// 数据项个数
	int   i;

	memset(szHexKey,0x00,sizeof(szHexKey));

	// 根据传入的数据类型检查传入数据的合法性
	if(pFlag == 0)						// 传入数据为BCD码
	{
		if((pKeyLen != 8)  || ((Len % 8) != 0) || (Len == 0)  )
		{
			return -1;
		}
		
		memcpy(szHexKey,pKey,8);		// 加密密钥
		nHexDataLen = Len;
		
		// 分配指针
		pHexData     = new unsigned char [Len +1];
		pHexOutData  = new unsigned char [Len +1];
		pOutData     = new unsigned char [Len +1];
		if( (pHexData == NULL) || (pHexOutData == NULL) || (pOutData == NULL ) )
		{
			return -2;
		}

		memset(pHexData   ,0x00,Len+1);
		memset(pHexOutData,0x00,Len+1);
		memset(pOutData   ,0x00,Len+1);

		memcpy(pHexData,pIn,Len);
	}else								// 传入数据为ASCII码
	{
		if( (pKeyLen != 16) || ((Len % 16 ) != 0)  || ( Len == 0))
		{
			return -1;
		}
		
		// 将ASCII码的密钥转换成BCD码的密钥
		ftAtoh(pKey,(char *)szHexKey,8);

		// 将ASCII码的加密数据转换成BCD码的数据 
		nHexDataLen  = Len / 2;

		// 分配指针
		pHexData     = new unsigned char [nHexDataLen +1];
		pHexOutData  = new unsigned char [nHexDataLen +1];
		pOutData     = new unsigned char [Len +1];

		if( (pHexData == NULL) || (pHexOutData == NULL) || (pOutData == NULL) )
		{
			return -2;
		}

		memset(pHexData,0x00,nHexDataLen +1);
		memset(pHexOutData,0x00,nHexDataLen +1);
		memset(pOutData,0x00,Len +1);

		// 将传入的ASCII码数据转换成
		ftAtoh(pIn,(char *)pHexData,nHexDataLen);
		
	}
 
	// 计算用进行计算的次数
	nDataItem = nHexDataLen / 8;

	for(i = 0;i< nDataItem ;i++)
	{
		memset(szTmpInData,0x00,sizeof(szTmpInData));
		memset(szTmpOutData,0x00,sizeof(szTmpOutData));

		//memcpy(pHexData + i*8,szTmpInData,8);
		memcpy(szTmpInData,pHexData + i*8,8);
		// DES数据加密
		CDES::RunDes(1,(char *)szTmpInData,8,(char *)szHexKey,8,(char *)szTmpOutData);

		//保存加密的数据
		memcpy(pHexOutData+i*8,szTmpOutData,8);		
	}

	if( pFlag == 0)						// 传入数据为BCD码
	{
		memcpy(pOut,pHexOutData,nHexDataLen);

	}else 
	{
		// 将加密的16进制数据转换成ASCII
		ftHtoa((char *)pHexOutData,(char *)pOutData,nHexDataLen);
		memcpy(pOut,pOutData,nHexDataLen*2);		
	}

	// 释放指针
	free(pHexData);
	free(pHexOutData);
	free(pOutData);
	
	pHexData    = NULL;
	pHexOutData = NULL;
	pOutData    = NULL;

	return 0;
}


/******************************************************************************************
 *******	函数名称：ft3DesDec   -序号：21-										*******
 *******	函数功能：3DES数据解密													*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：char *pKey   :要进行3DES解密的密钥,ASCII码32字节，BCD为16字节	*******
 *******			  int   pKeyLen:密钥长度,值为16:BCD码;32:ASCII码				*******
 *******			  char *pIn    :解密数据，ASCII为16字节、BCD码为8字节的倍数 	*******
 *******			  char *pOut   :(输出)解密生成的数据							*******
 *******              int   pLen   :要解密的数据长度必须是8的倍数					*******
 *******              int   pFlag  :数据格式标志，缺省为：0，BCD码；其他值为ASCII   *******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回<0	;错误代码如下：						*******
 *******								-1：输入密钥错误或输入加密数据错误			*******
 *******								-2：内部分配指针失败						*******
 ******************************************************************************************/
int ft3DesDec(char *pKey,int pKeyLen,char *pIn,char *pOut,int Len,int pFlag)
{
	unsigned char szHexKey[16+1];		// 十六进制的KEY
	unsigned char *pHexData;
	unsigned char szTmpInData[8+1];		// 加密用输入变量
	unsigned char szTmpOutData[8+1];	// 加密临时输出变量
	unsigned char *pOutData;			// 临时输出的ASCII数据
	unsigned char *pHexOutData;			// 临时输出的十六进制数据

	int   nHexDataLen;					// 十六进制的数据长度
	int   nDataItem;					// 数据项个数
	int   i;

	memset(szHexKey,0x00,sizeof(szHexKey));

	// 根据传入的数据类型检查传入数据的合法性
	if(pFlag == 0)						// 传入数据为BCD码
	{
		if((pKeyLen != 16)  || ((Len % 8) != 0) || (Len == 0)  )
		{
			return -1;
		}
		
		memcpy(szHexKey,pKey,16);		// 加密密钥
		nHexDataLen = Len;
		
		// 分配指针
		pHexData     = new unsigned char [Len +1];
		pHexOutData  = new unsigned char [Len +1];
		pOutData     = new unsigned char [Len +1];
		if( (pHexData == NULL) || (pHexOutData == NULL) || (pOutData == NULL ) )
		{
			return -2;
		}

		memset(pHexData   ,0x00,Len+1);
		memset(pHexOutData,0x00,Len+1);
		memset(pOutData   ,0x00,Len+1);

		memcpy(pHexData,pIn,Len);
	}else								// 传入数据为ASCII码
	{
		if( (pKeyLen != 32) || ((Len % 16 ) != 0)  || ( Len == 0))
		{
			return -1;
		}
		
		// 将ASCII码的密钥转换成BCD码的密钥
		ftAtoh(pKey,(char *)szHexKey,16);

		// 将ASCII码的加密数据转换成BCD码的数据 
		nHexDataLen  = Len / 2;

		// 分配指针
		pHexData     = new unsigned char [nHexDataLen +1];
		pHexOutData  = new unsigned char [nHexDataLen +1];
		pOutData     = new unsigned char [Len +1];

		if( (pHexData == NULL) || (pHexOutData == NULL) || (pOutData == NULL) )
		{
			return -2;
		}

		memset(pHexData,0x00,nHexDataLen +1);
		memset(pHexOutData,0x00,nHexDataLen +1);
		memset(pOutData,0x00,Len +1);

		// 将传入的ASCII码数据转换成
		ftAtoh(pIn,(char *)pHexData,nHexDataLen);
		
	}
 
	// 计算用进行计算的次数
	nDataItem = nHexDataLen / 8;

	for(i = 0;i< nDataItem ;i++)
	{
		memset(szTmpInData,0x00,sizeof(szTmpInData));
		memset(szTmpOutData,0x00,sizeof(szTmpOutData));

		//memcpy(pHexData + i*8,szTmpInData,8);
		memcpy(szTmpInData,pHexData + i*8,8);
		// DES数据加密
		CDES::RunDes(1,(char *)szTmpInData,8,(char *)szHexKey,16,(char *)szTmpOutData);

		//保存加密的数据
		memcpy(pHexOutData+i*8,szTmpOutData,8);		
	}

	if( pFlag == 0)						// 传入数据为BCD码
	{
		memcpy(pOut,pHexOutData,nHexDataLen);

	}else 
	{
		// 将加密的16进制数据转换成ASCII
		ftHtoa((char *)pHexOutData,(char *)pOutData,nHexDataLen);
		memcpy(pOut,pOutData,nHexDataLen*2);		
	}

	// 释放指针
	free(pHexData);
	free(pHexOutData);
	free(pOutData);
	
	pHexData    = NULL;
	pHexOutData = NULL;
	pOutData    = NULL;

	return 0;
}


/******************************************************************************************
 *******	函数名称：ftCalSK   -序号：22-											*******
 *******	函数功能：金融交易过程密钥(SK)计算函数[圈存、圈提、取现、消费等]		*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：char *pKey   :要进行SK计算的密钥,ASCII码32字节，BCD为16字节	*******
 *******			  int   pKeyLen:密钥长度,值为16:BCD码;32:ASCII码				*******
 *******			  char *pIn    :计算过程密钥数据，ASCII为16字节、BCD码为8字节 	*******
 *******			  int   pLen   :计算数据长度，ASCII为16字节、BCD码为8字节 		*******
 *******			  char *pOut   :(输出)计算生成的过程密钥						*******
 *******              int   pFlag  :数据格式标志，缺省为：0，BCD码；其他值为ASCII   *******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回<0	;错误代码如下：						*******
 *******								-1：输入密钥错误或输入数据错误				*******
 *******                                -2：计算过程密钥错误						*******
 ******************************************************************************************/
int ftCalSK(char *pKey,int pKeyLen,char *pIn,int pLen,char *pOut,int pFlag  )
{
	char szKey[16+1];
	char szHexIn[8+1];
	char szHexOut[8+1];
	char szOut[16+1];

	int nRet;
	
	memset(szKey   ,0x00,sizeof(szKey));
	memset(szHexIn ,0x00,sizeof(szHexIn));
	memset(szHexOut,0x00,sizeof(szHexOut));


	if(pFlag == 0)				// 传入参数数据为BCD码
	{
		// 检查数据合法性
		if(( pKeyLen != 16)  || (pLen != 8))
		{
			return -1;
		}

		// 处理传入数据
		memcpy(szKey,pKey,pKeyLen);
		memcpy(szHexIn,pIn,pLen);

	}else						// 传入参数数据为ASCII码 
	{
		// 检查数据合法性
		if( ( (int)strlen(pKey) != pKeyLen ) || (pKeyLen != 32) ||
			( (int)strlen(pIn)  != pLen )    || ( pLen != 16 ) )
		{
			return -1;
		}

		// 处理传入数据
		ftAtoh(pKey,szKey,16);
		ftAtoh(pIn,szHexIn,8);
	}

	// 使用3DES加密生成过程密钥
	nRet = ft3DesEnc(szKey,16,szHexIn,szHexOut,8,0);
	if(nRet != 0)
	{
		return -2;
	}


	if(pFlag == 0)				// 传入参数数据为BCD码
	{
		memcpy(pOut,szHexOut,8);
	}else						// 传入参数数据为ASCII码
	{
		ftHtoa(szHexOut,szOut,8);
		memcpy(pOut,szOut,16);
	}

	return 0;
}



/******************************************************************************************
 *******	函数名称：CalMac   -序号：23-											*******
 *******	函数功能：金融交易MAC计算函数[圈存、圈提、取现、消费等]					*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：char *Key    :计算MAC的过程密钥，8字节,BCD码					*******
 *******			  char *Vector :计算MAC的初始化向量，8字节，BCD码,通常为：\x00	*******
 *******			  char *Data   :计算MAC的数据, BCD码,为8字节的倍数          	*******
 *******			  int   pLen   :计算MAC的数据长度，								*******
 *******			  char *pOut   :(输出)计算返回的MAC值，BCD码，长度为4字节		*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回<0	;错误代码如下：						*******
 *******								-1：输入密钥错误或输入数据错误				*******
 *******                                -2：计算过程密钥错误						*******
 ******************************************************************************************/
int CalMac(char *Key,char *Vector,char *Data,char *AscMAC)
{
	char szCommand[512];
	char szVector[64+1];
	char szBcdVector[32+1];
	char szSK[16+1];
	char szBcdSK[8+1];
	char szMAC[8+1];
	char szBcdMAC[4+1];
	char szTmp[16+1];

	char szXOR[8+1];
	int nRet ;
	int nLen;
	int nBcdDataLen ;    // BCD码的计算数据长度
	int nItem;			 // 计算BCD码的数据项个数
	int i;
	
	memset(szVector, 0x00,sizeof(szVector));
	memset(szCommand,0x00,sizeof(szCommand));
	memset(szMAC,    0x00,sizeof(szMAC));
	memset(szSK,     0x00,sizeof(szSK));
	memset(szTmp,    0x00,sizeof(szTmp));
	memset(szBcdMAC, 0x00,sizeof(szBcdMAC));
	memset(szXOR,    0x00,sizeof(szXOR));

	nRet = -1;

	// 检查传入参数，的长度是否合法，过程密钥：16个ASCII、初始化向量：16个ASCII；计算数据：16必须是16的倍数
	if( (strlen(Key) != 16) || (strlen(Vector) != 16) || (strlen(Data) % 16 != 0))
	{
		return -1;
	}

	memset(szBcdVector,0x00,sizeof(szBcdVector));
	memset(szBcdSK,0x00,sizeof(szBcdSK));


	// 设置初始化向量
	sprintf(szVector,Vector);

	// 设置SK
	sprintf(szSK,Key);
	
	nLen =(int) strlen(Data);

	nBcdDataLen = nLen / 2;				// BCD码的数据项长度

	nItem = nBcdDataLen / 8;			// 要进行拆分的数据项个数

	char *szBcdData;

	szBcdData = (char *) malloc(nBcdDataLen +1 );
	if(szBcdData == NULL)
	{
		return -2;
	}

	memset(szBcdData,0x00,nBcdDataLen +1);

	// 将ASCII的数据项都转换成BCD码的数据项
	ftAtoh(szSK,szBcdSK,8);
	ftAtoh(szVector,szBcdVector,8);
	ftAtoh(Data,szBcdData,nBcdDataLen);
	

	for(i= 0;i< nItem;i++)
	{
		// 1.对数据进行XOR运行
		memset(szTmp,0x00,sizeof(szTmp));
		
		memcpy(szTmp,szBcdData+i*8,8);
		// 对数据进行异或运行
		ftCalXOR(szBcdVector,szTmp,8,szXOR);
		memset(szTmp,0x00,sizeof(szTmp));
		// 对异或值进行DES加密
		CDES::RunDes(0,szXOR,8,szBcdSK,8,szTmp);
		memcpy(szBcdVector,szTmp,8);
	}

	free(szBcdData);
	szBcdData = NULL;

	// szBcdVector 前四字节为MAC值
	memcpy(szBcdMAC,szBcdVector,4);		
 	ftHtoa(szBcdMAC,szMAC,4);

	memcpy(AscMAC,szMAC,8);
	memcpy(AscMAC+8,"\x00",1);
 
    return 0;


}



/******************************************************************************************
 *******	函数名称：CalMacFor3Des   -序号：24-									*******
 *******	函数功能：无特殊情况的MAC计算函数										*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：char *Key     :计算MAC的密钥，32个字符的ASCII					*******
 *******			  char *InitData:计算MAC的初始化向量，16个字符的ASCII			*******
 *******			  char *Data    :计算MAC的数据,8的倍数,ASCII		          	*******
 *******			  char *Out     :(输出)计算返回的MAC值， ASCII					*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；失败返回<0	;错误代码如下：						*******
 *******								-1：输入密钥错误或输入数据错误				*******
 *******                                -2：计算过程密钥错误						*******
 ******************************************************************************************/
int ftCalMacFor3Des(char *Key,char *InitData,char *Data,char *Out)
{
    char szKey[16+1];			// BCD码的密钥				16字节
    char szKeyL[8+1];			// BCD码的密钥前半部分		8字节
    char szInitData[8+1];		// BCD码的初始值			8字节
	char *pData;                // BCD码的计算数据      
	char szItem[8+1];			// BCD码的当前处理数据项	8字节 
	char szTmp[8+1];			// 进行XOR计算的数据项		8字节
	char szXORVal[8+1];			// XOR结果值				8字节
    int  nItem = 0;				// 分解成8字节一组的数据项总数
	int  nDataLen;				// BCD码的数据长度
	char szMac[8+1];
	int  i;


	memset(szKey     , 0x00, sizeof(szKey));
	memset(szInitData, 0x00, sizeof(szInitData));
    memset(szItem    , 0x00, sizeof(szItem));
	memset(szTmp     , 0x00, sizeof(szTmp));

	// 传入数据合法性检查
	if( strlen(Key) != 32 || strlen(InitData) != 16 )
	{
		
		return -1;
	}

	// 检查传入数据的合法性，BCD码长度必须是8的倍数
	nDataLen = (int)strlen(Data);
	if (nDataLen % 2 != 0)
	{
		
		return -1;
	}

	// 取传入数据的BCD码长度
	nDataLen = nDataLen / 2;			

	// 检查传入的数据是否为8的倍数
	if( nDataLen % 8 != 0)
	{
	
		return -1;
	}

	// 按8字节拆分的数据总项数
	nItem = nDataLen / 8;			

	pData = (char *)malloc( nDataLen + 1);
	if( pData == NULL)
	{

		return -1;
	}
	memset(pData,0x00,nDataLen + 1);


	// 将传入数据的ASCII码转换成BCD码
	// 数据由ACII转换成BCD码
	ftAtoh(Data,pData,nDataLen);
	
	// 计算密钥
	ftAtoh(Key,szKey,16);	

	// 密钥的左半部分(即前8字节)
	memcpy(szKeyL,szKey,8);			 

	// MAC计算的初始化数据 4 字节随机数 + 4字节 0x00 
	ftAtoh(InitData,szInitData,8);

	// 计算MAC
	for(i = 0; i < nItem ; i++)
	{
		// 处理的数据项
		memcpy(szItem,pData+i*8,8);
		ftCalXOR(szItem,szInitData,8,szXORVal);
		if((i+1) != nItem)
		{	
			// DES加密
			CDES::RunDes(0,szXORVal,8,szKeyL,8,szTmp);
		}else {
			// 3DES加密
			CDES::RunDes(0,szXORVal,8,szKey,16,szTmp);
		}
		memcpy(szInitData,szTmp,8);
	}

	// 计算的MAC返回
	memset(szMac,0x00,sizeof(szMac));
	ftHtoa(szTmp,szMac,4);
	
	memcpy(Out,szMac,8);

	// 释放指针
	free(pData);
	pData = NULL;

    return 0;
}





/******************************************************************************************
 *******	函数名称：ftGetProtocol	   -序号：15-									*******
 *******	函数功能：根据上电复位信息[BCD码值]计算IC卡的通信协议,					*******
 *******----------------------------------------------------------------------------******* 
 *******	函数参数：unsigned char *pATR：要计算通信协议的ATR字节流[BCD码]			*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：IC卡的通信协议；0=通信协议为：T=0;1=通信协议为T=1				******* 
 ******************************************************************************************/
int ftGetProtocolForATR(unsigned char *pATR)
{    
	int nTDFlag = 0;
	int nTCFlag = 0;
	int nTBFlag = 0;
	int nTAFlag = 0;
	int nPos;
	
	int nProtocol = 0;		// IC卡通信协议，缺省为：T=0
	unsigned char nT0;
	
	// 获取ATR字节流中 T0 [ATR第2字节] 值
	nT0 = (unsigned char )pATR[1] & 0xF0 ;
	
	// 检测TA1是否存在
	if( (nT0 & 0x10 ) != 0)
	{
		nTAFlag = 1;
	}
	
	// 检测TB1是否存在
	if( (nT0 & 0x20 ) != 0)
	{
		nTBFlag = 1;
	}
	
	// 检测TC1是否存在
	if( ( nT0 & 0x40 ) != 0)
	{
		nTCFlag = 1;
	}
	
	// 检测TD1是否存在
	if( ( nT0 & 0x80 ) != 0)
	{
		nTDFlag = 1;
	}
	
	// 无TD1 则通信协议为：T= 0
	if (nTDFlag == 0)
	{
		nProtocol = 0;
	}else 
	{
		// 计算TD1 值的位置
		nPos = 0;
		if( nTAFlag) 
			nPos +=1;
		if( nTBFlag)
			nPos +=1;
		if(nTCFlag)
			nPos +=1;
		
		nT0 = pATR[nPos + 2] ;
		
		nProtocol = nT0 & 0x01;
	}
	
	return nProtocol;
	
}



/******************************************************************************************
 *******	函数名称：ftWriteLog  -序号：26-										*******
 *******	函数功能：日志文件记录函数												*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：char *FileName   :日志文件名称，相对路径或绝对路径			*******
 *******			  char *pData      :日志写入数据							 	*******
 *******			  int   pDataLen   :写入的日志数据长度					 		*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；													*******
 ******************************************************************************************/
#ifdef MANA
void ftWriteLog(char *pFileName,char *format,...)
{
	SYSTEMTIME st;		
	CString strTime;
	CString strDate;
	CString strFileName;	
	CString strLogBuf;
	CString strCh;
	char szLogBuf[2048+1];
	va_list valist;
	FILE *hLogFile;
	int     nPos;
	int     nRet;


	// 获取当前系统时间
	GetLocalTime(&st);

	// 获取当前时间的年-月-日 - ：时：分：秒：微秒
	//strTime.Format(" %04d-%02d-%02d - %02d:%02d:%02d:%03d",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);	
	strTime.Format("%02d:%02d:%02d:%03d",st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);	
	// 获取：YYYYMMDD 格式的日期
	strDate.Format("%04d%02d%02d",st.wYear,st.wMonth,st.wDay);										
	
	// 日志文件名保存到CString对象
	strFileName.Format("%s",pFileName);

	// 获取日志文件扩张名位置
	nPos = 	strFileName.Find('.');
	if(nPos>0)
	{
		// 在日志文件的扩张名前增加字符:_
		strFileName.Insert(nPos,'_');
	}

	nPos = 	strFileName.Find('.');
	if(nPos>0)
	{
		// 在日志文件的扩展名前增加日期[YYYYMMDD]
		strFileName.Insert(nPos,(LPCTSTR)strDate);
	}
	
	memset(szLogBuf,0x00,sizeof(szLogBuf));

	// 处理传入的格式化数据
	va_start( valist, format );
	_vsnprintf_s( szLogBuf  , 2048 , _TRUNCATE, format, valist );
	va_end( valist );

	strLogBuf = szLogBuf;
	strCh = "";
	// 处理字符串前的换行字符
	while(true)
	{
			nPos=strLogBuf.Find('\n');	
			if(nPos!=0) break;			
			strCh+='\n';
			strLogBuf.Delete(0);
	}

	// 已添加方式打开日志文件
	nRet = fopen_s(&hLogFile,(LPCTSTR)strFileName,"a+");
	if( nRet == 0)
	{
		// 写入日志文件内容
		fprintf(hLogFile,"%s%s-->%s\n",(LPCTSTR)strCh,(LPCTSTR)strTime,(LPCTSTR)strLogBuf);		// 加入时间显示:2008-3-17
		fclose(hLogFile);
	}

}
#endif
/******************************************************************************************
 *******	函数名称：ftSplitStr -序号：27-											*******
 *******	函数功能：按指定字符拆分字符串											*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：char *FileName   :日志文件名称，相对路径或绝对路径			*******
 *******			  char *pData      :日志写入数据							 	*******
 *******			  int   pDataLen   :写入的日志数据长度					 		*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；													*******
 ******************************************************************************************/
int ftSplitStr(char *pStr,int pStrLen,char pSplitChar,char pData[][81])
{
	int nStartPos;
	int nCurrPos;
	int nLen;
	int nNum ;
	char szData[81];

	nNum = 0;
	nStartPos = 0;
	nCurrPos  = 0;
	while(1)
	{
		if(pStr[nCurrPos] == pSplitChar)
		{
			// 字符串长度
			nLen = nCurrPos - nStartPos;
			if(nLen == 0)
			{
				memcpy(pData[nNum],"\0",1);
			}else 
			{
				// 拆分数据项最大长度为80字节
				if(nLen >80) nLen = 80;				
				memset(szData,0x00,sizeof(szData));

				memcpy(pData[nNum],pStr+nStartPos,nLen);
			}
			
			nNum++;
			nStartPos = nCurrPos + 1;
		}
		nCurrPos ++;

		if( nCurrPos >= pStrLen)
		{
			break;
		}
	}

	return nNum;
}



// 计算BCD码字符串的PCK值

/******************************************************************************************
 *******	函数名称：ftCalPCK -序号：28-											*******
 *******	函数功能：计算指定BCD码数据的PCK值(即数据异或计算)						*******
 *******----------------------------------------------------------------------------*******
 *******	函数参数：unsigned char *Str:要计算的BCD码数据							*******
 *******			  int  Len			:要计算的数据长度						 	*******
 *******			  unsigned int *PCK :(输出)计算返回的PCK值				 		*******
 *******----------------------------------------------------------------------------*******
 *******	返 回 值：成功返回：0；													*******
 ******************************************************************************************/
int ftCalPCK(unsigned char *Str,int Len,unsigned int *PCK)
{
	
	unsigned char nCH;
	int i ;
	
	if( Len <= 0)
	{
		return -1;
	}
	
	
	if( Len == 1)
	{
		*PCK = (unsigned int)Str[0];
		return 0;
	}
	
	nCH = Str[0];
	
	// TCK 
	for(i = 1; i< Len  ;i++)
	{
		nCH = (unsigned char )Str[i] ^ (unsigned char)nCH;
	}
	
	*PCK = nCH;
	
	return 0;
	
}
