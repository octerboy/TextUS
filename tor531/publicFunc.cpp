/******************************************************************************************
 ** �ļ����ƣ�publicFunc.cpp                                                             **
 ** �ļ���������������                                                                   **
 **--------------------------------------------------------------------------------------**
 ** �� �� �ˣ����ܿ�ҵ��                                                               **
 ** �������ڣ�2012-6-18                                                                  **
 **--------------------------------------------------------------------------------------**
 ** �� �� �ˣ�                                                                           **
 ** �޸����ڣ�                                                                           **
 **--------------------------------------------------------------------------------------**
 ** �� �� �ţ�                                                                           **
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
 *******	�������ƣ�ftAtoh		-��ţ�1-										*******
 *******	�������ܣ�ASCII ��ת���� BCD�� ��16������								*******
 *******----------------------------------------------------------------------------*******
 *******	����������																*******
 *******			 char *ascstr:��ת����ASCII�ַ���								*******
 *******			 char *bcdstr:ת�����ɵ�BCD��									*******
 *******			 int  bcdlen:���ɵ�BCD�볤��									*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ�����ɵ�BCD��													*******
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
 *******	�������ƣ�ftHtoa		-��ţ�2-										*******
 *******	�������ܣ�BCD��(��16����)ת����ASCII									*******
 *******----------------------------------------------------------------------------******* 
 *******	����������																*******
 *******			char *hexstr:��ת����BCD��										*******
 *******			char *ascstr:���ɵ�ASCII��										*******
 *******			int  length:��Ҫת����BCD��ĳ���								*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����������ɵ�ASCII���ȣ�ʧ�ܷ��أ�0						*******
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
 *******	�������ƣ�ftStringToUpper	   -��ţ�3-								*******
 *******	�������ܣ����ַ����е���ĸת���ɴ�д									*******
 *******----------------------------------------------------------------------------******* 
 *******	����������char *Str:(����/���)��Ҫ����ת�����ַ�						*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����������ɵ�ASCII���ȣ�ʧ�ܷ��أ�0						*******
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
 *******	�������ƣ�ftStringToLower	   -��ţ�4-								*******
 *******	�������ܣ����ַ����е���ĸת����Сд									*******
 *******----------------------------------------------------------------------------******* 
 *******	����������char *Str:(����/���)��Ҫ����ת�����ַ�						*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����������ɵ�ASCII���ȣ�ʧ�ܷ��أ�0						*******
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
 *******	�������ƣ�ftCalXOR	   -��ţ�5-										*******
 *******	�������ܣ������㺯��													*******
 *******----------------------------------------------------------------------------******* 
 *******	����������																******* 
 *******			 char *Param1��Ҫ����������ĵ�һ������						*******
 *******			 char *Param2��Ҫ����������ĵڶ�������						******* 
 *******			 int   Len��   Ҫ��������������ݳ���							*******
 *******			 char *Out��   (���)�����㷵��ֵ								*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����ɹ����أ�0��ʧ�ܷ��أ�-1								*******
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
 *******	�������ƣ�ftHexToLong	   -��ţ�6-									*******
 *******	�������ܣ���16���ƵĽ��ת���ɳ�����									*******
 *******----------------------------------------------------------------------------******* 
 *******	����������																*******
 *******			char *hexstr:ʮ�����ƽ��ֵ�ַ���(�磺"65F")					*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ���ʧ�ܷ��أ�-1���ɹ����أ������͵Ľ��ֵ					*******
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
 *******	�������ƣ�ftLongToHex	   -��ţ�7-									*******
 *******	�������ܣ�ʮ���ƽ��ֵת����16���ƽ���ַ���							*******
 *******----------------------------------------------------------------------------******* 
 *******	����������																*******
  *******			long Val��Ҫת����ʮ���ƽ��ֵ									*******
 *******			char *hexstr:(���)ת�����ɵ�16���ƽ��ֵ�ַ���					*******
 *******			int Flag�����ֵ������ͱ�־��ȱʡֵΪ1��BCD�룻0��16�����ַ���	*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ���ʧ�ܷ��أ�-1���ɹ����أ����ɵ�16���ƽ��ֵ�ַ�������	*******
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

	nLen = i;			// ASCII�����ݳ���

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

	// Flag ����Ϊ��0 ���صĽ��Ϊ16���Ƶ�ASCII�ַ������
	if( Flag == 0)
	{
		memcpy(HexStr,szAscStr,nLen);
		return nLen;
	}

	// Flag ����Ϊ��1�����صĽ��ΪBCD��Ľ�� 
	nLen = nLen/2;
	memset(szHexStr,0x00,sizeof(szHexStr));
    
	ftAtoh(szAscStr,szHexStr,nLen);
	memcpy(HexStr,szHexStr,nLen);

	return nLen;
}


/******************************************************************************************
 *******	�������ƣ�ftLongToAmount   -��ţ�7A-									*******
 *******	�������ܣ�ʮ���ƽ��ֵת����16���ƽ���ַ���(8��ASCII�ַ�ǰ��0)			*******
 *******----------------------------------------------------------------------------******* 
 *******	����������																*******
  *******			long Val��Ҫת����ʮ���ƽ��ֵ									*******
 *******			char *hexstr:(���)ת�����ɵ�16���ƽ��ֵ�ַ���					*******
 *******			int Flag�����ֵ������ͱ�־��ȱʡֵΪ1��BCD�룻0��16�����ַ���	*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ���ʧ�ܷ��أ�-1���ɹ����أ����ɵ�16���ƽ��ֵ�ַ�������	*******
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

	// ����ǰ��Ҫ����ģ�0
	memcpy(szAmount,"00000000",nLen2);
	memcpy(szAmount+nLen2,szVal,nLen);

	memcpy(HexStr,szAmount,8);

	return 0;

}

/******************************************************************************************
 *******	�������ƣ�ftFillAmount	   -��ţ�8-									*******
 *******	�������ܣ���BCD��Ľ���ַ�����䵽4�ֽڳ���(ǰ��0x00)					*******
 *******----------------------------------------------------------------------------******* 
 *******	����������																*******
 *******			char *Str��(����/���)Ҫ��������16���ƽ���ַ���				*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ���ʧ�ܷ��أ�-1���ɹ����أ����أ�0��8						*******
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

	// ����Ҫ�����ַ���
	nFillLen = 8 - nLen;
	//strncpy(sAmount,sFill,nFillLen);
	memcpy(sAmount,sFill,nFillLen);
	memcpy(sAmount+nFillLen,Str,nLen);

	memcpy(Str,sAmount,8);
	return 8;
}


/******************************************************************************************
 *******	�������ƣ�ftFillString	   -��ţ�9-									*******
 *******	�������ܣ�MAC/TAC������ַ�����亯��(���80��8000..��8�ı�������)		*******
 *******----------------------------------------------------------------------------******* 
 *******	����������																*******
 *******			char *Str��(����/���)Ҫ��������16�����ַ���					*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ���ʧ�ܷ��أ�-1���ɹ����أ������ASCII���ݳ���			*******
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


	// ����ַ���
	memset(sFill,0x00,sizeof(sFill));
	memcpy(sFill,"8000000000000000",16);	
	
	memcpy(Str+nLen,sFill,nFillLen);

	memcpy(Str+nLen+nFillLen,"\x00",1);

	return nLen + nFillLen;

}


/******************************************************************************************
 *******	�������ƣ�ftFillString	   -��ţ�10-									*******
 *******	�������ܣ����ַ������ָ���ַ���ָ������,���ڴ˳������ȡ���˳���		*******
 *******----------------------------------------------------------------------------******* 
 *******	����������																*******
 *******			char *Str��(����/���)Ҫ���������ַ���						*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ���ʧ�ܷ��أ�-1���ɹ����أ�0								*******
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
		memcpy(Str,pData,Len+1);   // �� 0x00 �ַ�����	
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
	memcpy(Str,pData,Len+1);           // ����Ӧ������ʱ��������һ��'\0' �ַ��������ַ�
	
	free(sFill);
	sFill = NULL;
	
	free(pData);
	pData = NULL;

	return 0;
}



 
/******************************************************************************************
 *******	�������ƣ�ftCalLRC	   -��ţ�11-										*******
 *******	�������ܣ��������ͼ��㺯��											*******
 *******----------------------------------------------------------------------------******* 
 *******	����������char *DataBuf��(�������)Ҫ�������ͼ���Ļ�����				*******
 *******              int   Len:���ݳ���											*******
 *******			  int Flag�����ݸ�ʽ��׼��1��BCD�룻0:ASCII(ȱʡֵ)	;			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�-1		          						******* 
 ******************************************************************************************/
int  ftCalLRC(char *DataBuf,int Len,int Flag)
{
	char *pBuf;
	char  szHexVal[4+1];	// ���ͼ���ֵBCD��
	char cVal;

	
	int i;
	int nBcdLen = 0;
	
	// �����ַ�ָ��
	pBuf = (char *)malloc(Len+2);
	memset(pBuf,0x00,Len+2);



	if(Flag == 0)		//  �����������ΪASCII����
	{
		nBcdLen = Len / 2;
		
		ftAtoh(DataBuf,pBuf,nBcdLen);			
	}else				// Ϊ���������ΪBCD����
	{
		nBcdLen = Len;
		memcpy(pBuf,DataBuf,Len);

	}

	cVal = pBuf[0];

	for(i=1; i < nBcdLen ; i++)
	{
		cVal = cVal ^ pBuf[i];
	}


	// ���������������׷�ӵ������������
	if(Flag == 0)		// ��������ΪASCII����
	{
		memset(szHexVal,0x00,sizeof(szHexVal));
		sprintf(szHexVal,"%02.2X",(unsigned char)cVal);
		memcpy(DataBuf+Len,szHexVal,2);
	}else				// ��������ΪBCD������
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

// Flag ȱʡֵΪ��= 0��
int  ftCalLRC1(char *DataBuf,int Len,char *RetVal,int Flag)
{
	char *pBuf;
	char  szHexVal[4+1];	// ���ͼ���ֵBCD��
	char cVal;

	
	int i;
	int nBcdLen = 0;
	
	// �����ַ�ָ��
	pBuf = (char *)malloc(Len+2);
	memset(pBuf,0x00,Len+2);



	if(Flag == 0)		//  ���������ΪASCII����
	{
		nBcdLen = Len / 2;
		
		ftAtoh(DataBuf,pBuf,nBcdLen);			
	}else				// ���������ΪBCD����
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
 *******	�������ƣ�ftCharToBitString	   -��ţ�12-								*******
 *******	�������ܣ����ַ�ת����BIT�ַ���1�ֽ�ת����8��0��1���ַ�����				*******
 *******----------------------------------------------------------------------------******* 
 *******	����������unsigned char DataBuf��Ҫ����ת�����ַ�						*******
 *******              int   Len:���ݳ���											*******
 *******			  char *OutBuf��ת�����ɵ�BIT�ַ�����							*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�-1		          						******* 
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
 *******	�������ƣ�ftDataToBitString	   -��ţ�13-								*******
 *******	�������ܣ���BCD������ת����BIT�ַ���1�ֽ�ת����8��0��1���ַ�����		*******
 *******----------------------------------------------------------------------------******* 
 *******	����������char *InData��Ҫ����ת��������ָ��							*******
 *******              int   Len:���ݳ���											*******
 *******			  char *OutBuf��ת�����ɵ�BIT�ַ�����							*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�-1		          						******* 
 ******************************************************************************************/
int ftDataToBitString(char *InData,int Len,char *OutBuf)
{
	char szCharStr[8+1];		 

	char *pOutBuf;
	int nOutLen;

	nOutLen = 8 * Len +1;

	//  �������ָ��
	pOutBuf = (char *)malloc(nOutLen);

	memset(pOutBuf,0x00,nOutLen);

	for(int i = 0;i< Len;i++)
	{
		memset(szCharStr,0x00,sizeof(szCharStr));

		ftCharToBitString((unsigned char)InData[i],szCharStr);

		memcpy(pOutBuf+i*8,szCharStr,8);

	}

	// �����������
	memcpy(OutBuf,pOutBuf,8*Len);

	free(pOutBuf);
	pOutBuf = NULL;

	return 8*Len;
}




/******************************************************************************************
 *******	�������ƣ�ftBitStringToVal	   -��ţ�14-								*******
 *******	�������ܣ���BIT����ת��������ֵ											*******
 *******----------------------------------------------------------------------------*******
 *******	����������char *BitString��Ҫ����ת����BITָ��							*******
 *******              int   Len:BITָ�볤��											*******
 *******			  int  *Val��(���)	Bitת�����ɵ�INTֵ							*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�-1		          						*******
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
 *******	�������ƣ�ftDataEnc	   -��ţ�15-										*******
 *******	�������ܣ����ݼ��ܺ���(��������ļ򵥼���)								*******
 *******----------------------------------------------------------------------------*******
 *******	����������char *pIn:Ҫ���ܵ�����										*******
 *******              int   Len:Ҫ���ܵ����ݳ���									*******
 *******			  char *pOut:(���)	������������								*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�-1		          						*******
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
	
	// ���ü�������
	memcpy(szKey1,"a1g4d8k7i0f6m2ue3r5yo9bxcztvlrhv",32);

	for(int i = 0;i< pLen ;i++ )
	{
		szData[i] = pIn[i] ^ szKey1[i];	
	}

	memcpy(pOut,szData,pLen);
	
	return 0;

}



/******************************************************************************************
 *******	�������ƣ�ftDataDec	   -��ţ�16-										*******
 *******	�������ܣ����ݽ��ܺ���(��������ļ򵥽���)								*******
 *******----------------------------------------------------------------------------*******
 *******	����������char *pIn:Ҫ���ܵ�����										*******
 *******              int   pLen:Ҫ���ܵ����ݳ���									*******
 *******			  int  *pOut��(���)������������								*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�-1		          						*******
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
	
	// ���ü�������
	memcpy(szKey,"a1g4d8k7i0f6m2ue3r5yo9bxcztvlrhv",32);
	for(int i = 0;i< pLen ;i++ )
	{
		szData[i] = pIn[i] ^ szKey[i];	
	}

	memcpy(pOut,szData,pLen);
	
	return 0;
}


/******************************************************************************************
 *******	�������ƣ�ftDiversify   -��ţ�17-										*******
 *******	�������ܣ���Կ��ɢ����													*******
 *******----------------------------------------------------------------------------*******
 *******	����������char *Data��  ��ɢ���ӣ�ͨ��Ϊ8�ֽڵĿ���	    ASCII��ֵ		*******
 *******			  char *KEY��   Ҫ���з�ɢ��KEY				    ASCII��ֵ		*******
 *******			  char *Out��   ��Կ��ɢ�������(��������ԿSK)	ASCII��ֵ		*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ��أ�-1		          						*******
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


	// ��鴫�����ݵĺϷ��ԣ���ɢ����Ϊ16��ASCII�ַ�
	if(strlen(Data) != 16)
	{
         return -1;
	}

	// ��ɢ������ԿΪ32��ASCII�ַ�
	if(strlen(Key) != 32 )
	{
		return -2;
	}

	// ����������ת����16���Ƶ�BCD��
	ftAtoh(Data,szHexData,8);
	ftAtoh(Key,szHexKey,16);

	// �Դ����ɢ����ȡ������ӵ����ݺ�������16�ֽڵļ�������
	for(i = 0;i < 8;i++)
	{
		szHexData[8+i] =~((unsigned char )szHexData[i]);	
	}

	// �Դ����ķ�ɢ���ӽ���3DES����,���ɹ�����ԿSK
	CDES::RunDes(0,szHexData,16,szHexKey,16,szHexOut);
	
	// ��16���ƵĹ�����Կת����ASCII��Ĺ�����Կ
	ftHtoa(szHexOut,szOut,16);
	
	// ��������Կ����ĺ����������
	memcpy(Out,szOut,32);

	return 0;
}

/******************************************************************************************
 *******	�������ƣ�ftDesEnc   -��ţ�18-											*******
 *******	�������ܣ�DES���ݼ���													*******
 *******----------------------------------------------------------------------------*******
 *******	����������char *pKey   :Ҫ����DES���ܵ���Կ,ASCII��16�ֽڣ�BCDΪ8�ֽ�	*******
 *******			  int   pKeyLen:��Կ����,ֵΪ8:BCD��;16:ASCII��					*******
 *******			  char *pIn    :�������ݣ�ASCIIΪ16�ֽڡ�BCD��Ϊ8�ֽڵı��� 	*******
 *******			  char *pOut   :(���)�������ɵ�����							*******
 *******              int   pLen   :Ҫ���ܵ����ݳ��ȱ�����8�ı���					*******
 *******              int   pFlag  :���ݸ�ʽ��־��ȱʡΪ��0��BCD�룻����ֵΪASCII   *******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ���<0	;����������£�						*******
 *******								-1��������Կ���������������ݴ���			*******
 *******								-2���ڲ�����ָ��ʧ��						*******
 ******************************************************************************************/
int ftDesEnc(char *pKey,int pKeyLen,char *pIn,char *pOut,int Len,int pFlag)
{
	unsigned char szHexKey[8+1];		// ʮ�����Ƶ�KEY
	unsigned char *pHexData;
	unsigned char szTmpInData[8+1];		// �������������
	unsigned char szTmpOutData[8+1];	// ������ʱ�������
	unsigned char *pOutData;			// ��ʱ�����ASCII����
	unsigned char *pHexOutData;			// ��ʱ�����ʮ����������

	int   nHexDataLen;					// ʮ�����Ƶ����ݳ���
	int   nDataItem;					// ���������
	int   i;

	memset(szHexKey,0x00,sizeof(szHexKey));

	// ���ݴ�����������ͼ�鴫�����ݵĺϷ���
	if(pFlag == 0)						// ��������ΪBCD��
	{
		if((pKeyLen != 8)  || ((Len % 8) != 0) || (Len == 0)  )
		{
			return -1;
		}
		
		memcpy(szHexKey,pKey,8);		// ������Կ
		nHexDataLen = Len;
		
		// ����ָ��
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
	}else								// ��������ΪASCII��
	{
		if( (pKeyLen != 16) || ((Len % 16 ) != 0)  || ( Len == 0))
		{
			return -1;
		}
		
		// ��ASCII�����Կת����BCD�����Կ
		ftAtoh(pKey,(char *)szHexKey,8);

		// ��ASCII��ļ�������ת����BCD������� 
		nHexDataLen  = Len / 2;

		// ����ָ��
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

		// �������ASCII������ת����
		ftAtoh(pIn,(char *)pHexData,nHexDataLen);
		
	}
 
	// �����ý��м���Ĵ���
	nDataItem = nHexDataLen / 8;

	for(i = 0;i< nDataItem ;i++)
	{
		memset(szTmpInData,0x00,sizeof(szTmpInData));
		memset(szTmpOutData,0x00,sizeof(szTmpOutData));

		//memcpy(pHexData + i*8,szTmpInData,8);
		memcpy(szTmpInData,pHexData + i*8,8);
		// DES���ݼ���
		CDES::RunDes(0,(char *)szTmpInData,8,(char *)szHexKey,8,(char *)szTmpOutData);

		//������ܵ�����
		memcpy(pHexOutData+i*8,szTmpOutData,8);		
	}

	if( pFlag == 0)						// ��������ΪBCD��
	{
		memcpy(pOut,pHexOutData,nHexDataLen);

	}else 
	{
		// �����ܵ�16��������ת����ASCII
		ftHtoa((char *)pHexOutData,(char *)pOutData,nHexDataLen);
		memcpy(pOut,pOutData,nHexDataLen*2);		
	}

	// �ͷ�ָ��
	free(pHexData);
	free(pHexOutData);
	free(pOutData);
	
	pHexData    = NULL;
	pHexOutData = NULL;
	pOutData    = NULL;

	return 0;
}

/******************************************************************************************
 *******	�������ƣ�ft3DesEnc   -��ţ�19-										*******
 *******	�������ܣ�3DES���ݼ���													*******
 *******----------------------------------------------------------------------------*******
 *******	����������char *pKey   :Ҫ����3DES���ܵ���Կ,ASCII��32�ֽڣ�BCDΪ16�ֽ�	*******
 *******			  int   pKeyLen:��Կ����,ֵΪ16:BCD��;32:ASCII��				*******
 *******			  char *pIn    :�������ݣ�ASCIIΪ16�ֽڡ�BCD��Ϊ8�ֽڵı��� 	*******
 *******			  char *pOut   :(���)�������ɵ�����							*******
 *******              int   pLen   :Ҫ���ܵ����ݳ��ȱ�����8�ı���					*******
 *******              int   pFlag  :���ݸ�ʽ��־��ȱʡΪ��0��BCD�룻����ֵΪASCII   *******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ���<0	;����������£�						*******
 *******								-1��������Կ���������������ݴ���			*******
 *******								-2���ڲ�����ָ��ʧ��						*******
 ******************************************************************************************/
int ft3DesEnc(char *pKey,int pKeyLen,char *pIn,char *pOut,int Len,int pFlag)
{
	unsigned char szHexKey[16+1];		// ʮ�����Ƶ�KEY
	unsigned char *pHexData;
	unsigned char szTmpInData[8+1];		// �������������
	unsigned char szTmpOutData[8+1];	// ������ʱ�������
	unsigned char *pOutData;			// ��ʱ�����ASCII����
	unsigned char *pHexOutData;			// ��ʱ�����ʮ����������

	int   nHexDataLen;					// ʮ�����Ƶ����ݳ���
	int   nDataItem;					// ���������
	int   i;

	memset(szHexKey,0x00,sizeof(szHexKey));

	// ���ݴ�����������ͼ�鴫�����ݵĺϷ���
	if(pFlag == 0)						// ��������ΪBCD��
	{
		if((pKeyLen != 16)  || ((Len % 8) != 0) || (Len == 0)  )
		{
			return -1;
		}
		
		memcpy(szHexKey,pKey,16);		// ������Կ
		nHexDataLen = Len;
		
		// ����ָ��
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
	}else								// ��������ΪASCII��
	{
		if( (pKeyLen != 32) || ((Len % 16 ) != 0)  || ( Len == 0))
		{
			return -1;
		}
		
		// ��ASCII�����Կת����BCD�����Կ
		ftAtoh(pKey,(char *)szHexKey,16);

		// ��ASCII��ļ�������ת����BCD������� 
		nHexDataLen  = Len / 2;

		// ����ָ��
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

		// �������ASCII������ת����
		ftAtoh(pIn,(char *)pHexData,nHexDataLen);
		
	}
 
	// �����ý��м���Ĵ���
	nDataItem = nHexDataLen / 8;

	for(i = 0;i< nDataItem ;i++)
	{
		memset(szTmpInData,0x00,sizeof(szTmpInData));
		memset(szTmpOutData,0x00,sizeof(szTmpOutData));

		//memcpy(pHexData + i*8,szTmpInData,8);
		memcpy(szTmpInData,pHexData + i*8,8);
		// DES���ݼ���
		CDES::RunDes(0,(char *)szTmpInData,8,(char *)szHexKey,16,(char *)szTmpOutData);

		//������ܵ�����
		memcpy(pHexOutData+i*8,szTmpOutData,8);		
	}

	if( pFlag == 0)						// ��������ΪBCD��
	{
		memcpy(pOut,pHexOutData,nHexDataLen);

	}else 
	{
		// �����ܵ�16��������ת����ASCII
		ftHtoa((char *)pHexOutData,(char *)pOutData,nHexDataLen);
		memcpy(pOut,pOutData,nHexDataLen*2);		
	}

	// �ͷ�ָ��
	free(pHexData);
	free(pHexOutData);
	free(pOutData);
	
	pHexData    = NULL;
	pHexOutData = NULL;
	pOutData    = NULL;

	return 0;
}


/******************************************************************************************
 *******	�������ƣ�ftDesDec   -��ţ�20-											*******
 *******	�������ܣ�DES���ݽ���													*******
 *******----------------------------------------------------------------------------*******
 *******	����������char *pKey   :Ҫ����DES���ܵ���Կ,ASCII��16�ֽڣ�BCDΪ8�ֽ�	*******
 *******			  int   pKeyLen:��Կ����,ֵΪ8:BCD��;16:ASCII��					*******
 *******			  char *pIn    :�������ݣ�ASCIIΪ16�ֽڡ�BCD��Ϊ8�ֽڵı��� 	*******
 *******			  char *pOut   :(���)�������ɵ�����							*******
 *******              int   pLen   :Ҫ���ܵ����ݳ��ȱ�����8�ı���					*******
 *******              int   pFlag  :���ݸ�ʽ��־��ȱʡΪ��0��BCD�룻����ֵΪASCII   *******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ���<0	;����������£�						*******
 *******								-1��������Կ���������������ݴ���			*******
 *******								-2���ڲ�����ָ��ʧ��						*******
 ******************************************************************************************/
int ftDesDec(char *pKey,int pKeyLen,char *pIn,char *pOut,int Len,int pFlag)
{
	unsigned char szHexKey[8+1];		// ʮ�����Ƶ�KEY
	unsigned char *pHexData;
	unsigned char szTmpInData[8+1];		// �������������
	unsigned char szTmpOutData[8+1];	// ������ʱ�������
	unsigned char *pOutData;			// ��ʱ�����ASCII����
	unsigned char *pHexOutData;			// ��ʱ�����ʮ����������

	int   nHexDataLen;					// ʮ�����Ƶ����ݳ���
	int   nDataItem;					// ���������
	int   i;

	memset(szHexKey,0x00,sizeof(szHexKey));

	// ���ݴ�����������ͼ�鴫�����ݵĺϷ���
	if(pFlag == 0)						// ��������ΪBCD��
	{
		if((pKeyLen != 8)  || ((Len % 8) != 0) || (Len == 0)  )
		{
			return -1;
		}
		
		memcpy(szHexKey,pKey,8);		// ������Կ
		nHexDataLen = Len;
		
		// ����ָ��
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
	}else								// ��������ΪASCII��
	{
		if( (pKeyLen != 16) || ((Len % 16 ) != 0)  || ( Len == 0))
		{
			return -1;
		}
		
		// ��ASCII�����Կת����BCD�����Կ
		ftAtoh(pKey,(char *)szHexKey,8);

		// ��ASCII��ļ�������ת����BCD������� 
		nHexDataLen  = Len / 2;

		// ����ָ��
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

		// �������ASCII������ת����
		ftAtoh(pIn,(char *)pHexData,nHexDataLen);
		
	}
 
	// �����ý��м���Ĵ���
	nDataItem = nHexDataLen / 8;

	for(i = 0;i< nDataItem ;i++)
	{
		memset(szTmpInData,0x00,sizeof(szTmpInData));
		memset(szTmpOutData,0x00,sizeof(szTmpOutData));

		//memcpy(pHexData + i*8,szTmpInData,8);
		memcpy(szTmpInData,pHexData + i*8,8);
		// DES���ݼ���
		CDES::RunDes(1,(char *)szTmpInData,8,(char *)szHexKey,8,(char *)szTmpOutData);

		//������ܵ�����
		memcpy(pHexOutData+i*8,szTmpOutData,8);		
	}

	if( pFlag == 0)						// ��������ΪBCD��
	{
		memcpy(pOut,pHexOutData,nHexDataLen);

	}else 
	{
		// �����ܵ�16��������ת����ASCII
		ftHtoa((char *)pHexOutData,(char *)pOutData,nHexDataLen);
		memcpy(pOut,pOutData,nHexDataLen*2);		
	}

	// �ͷ�ָ��
	free(pHexData);
	free(pHexOutData);
	free(pOutData);
	
	pHexData    = NULL;
	pHexOutData = NULL;
	pOutData    = NULL;

	return 0;
}


/******************************************************************************************
 *******	�������ƣ�ft3DesDec   -��ţ�21-										*******
 *******	�������ܣ�3DES���ݽ���													*******
 *******----------------------------------------------------------------------------*******
 *******	����������char *pKey   :Ҫ����3DES���ܵ���Կ,ASCII��32�ֽڣ�BCDΪ16�ֽ�	*******
 *******			  int   pKeyLen:��Կ����,ֵΪ16:BCD��;32:ASCII��				*******
 *******			  char *pIn    :�������ݣ�ASCIIΪ16�ֽڡ�BCD��Ϊ8�ֽڵı��� 	*******
 *******			  char *pOut   :(���)�������ɵ�����							*******
 *******              int   pLen   :Ҫ���ܵ����ݳ��ȱ�����8�ı���					*******
 *******              int   pFlag  :���ݸ�ʽ��־��ȱʡΪ��0��BCD�룻����ֵΪASCII   *******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ���<0	;����������£�						*******
 *******								-1��������Կ���������������ݴ���			*******
 *******								-2���ڲ�����ָ��ʧ��						*******
 ******************************************************************************************/
int ft3DesDec(char *pKey,int pKeyLen,char *pIn,char *pOut,int Len,int pFlag)
{
	unsigned char szHexKey[16+1];		// ʮ�����Ƶ�KEY
	unsigned char *pHexData;
	unsigned char szTmpInData[8+1];		// �������������
	unsigned char szTmpOutData[8+1];	// ������ʱ�������
	unsigned char *pOutData;			// ��ʱ�����ASCII����
	unsigned char *pHexOutData;			// ��ʱ�����ʮ����������

	int   nHexDataLen;					// ʮ�����Ƶ����ݳ���
	int   nDataItem;					// ���������
	int   i;

	memset(szHexKey,0x00,sizeof(szHexKey));

	// ���ݴ�����������ͼ�鴫�����ݵĺϷ���
	if(pFlag == 0)						// ��������ΪBCD��
	{
		if((pKeyLen != 16)  || ((Len % 8) != 0) || (Len == 0)  )
		{
			return -1;
		}
		
		memcpy(szHexKey,pKey,16);		// ������Կ
		nHexDataLen = Len;
		
		// ����ָ��
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
	}else								// ��������ΪASCII��
	{
		if( (pKeyLen != 32) || ((Len % 16 ) != 0)  || ( Len == 0))
		{
			return -1;
		}
		
		// ��ASCII�����Կת����BCD�����Կ
		ftAtoh(pKey,(char *)szHexKey,16);

		// ��ASCII��ļ�������ת����BCD������� 
		nHexDataLen  = Len / 2;

		// ����ָ��
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

		// �������ASCII������ת����
		ftAtoh(pIn,(char *)pHexData,nHexDataLen);
		
	}
 
	// �����ý��м���Ĵ���
	nDataItem = nHexDataLen / 8;

	for(i = 0;i< nDataItem ;i++)
	{
		memset(szTmpInData,0x00,sizeof(szTmpInData));
		memset(szTmpOutData,0x00,sizeof(szTmpOutData));

		//memcpy(pHexData + i*8,szTmpInData,8);
		memcpy(szTmpInData,pHexData + i*8,8);
		// DES���ݼ���
		CDES::RunDes(1,(char *)szTmpInData,8,(char *)szHexKey,16,(char *)szTmpOutData);

		//������ܵ�����
		memcpy(pHexOutData+i*8,szTmpOutData,8);		
	}

	if( pFlag == 0)						// ��������ΪBCD��
	{
		memcpy(pOut,pHexOutData,nHexDataLen);

	}else 
	{
		// �����ܵ�16��������ת����ASCII
		ftHtoa((char *)pHexOutData,(char *)pOutData,nHexDataLen);
		memcpy(pOut,pOutData,nHexDataLen*2);		
	}

	// �ͷ�ָ��
	free(pHexData);
	free(pHexOutData);
	free(pOutData);
	
	pHexData    = NULL;
	pHexOutData = NULL;
	pOutData    = NULL;

	return 0;
}


/******************************************************************************************
 *******	�������ƣ�ftCalSK   -��ţ�22-											*******
 *******	�������ܣ����ڽ��׹�����Կ(SK)���㺯��[Ȧ�桢Ȧ�ᡢȡ�֡����ѵ�]		*******
 *******----------------------------------------------------------------------------*******
 *******	����������char *pKey   :Ҫ����SK�������Կ,ASCII��32�ֽڣ�BCDΪ16�ֽ�	*******
 *******			  int   pKeyLen:��Կ����,ֵΪ16:BCD��;32:ASCII��				*******
 *******			  char *pIn    :���������Կ���ݣ�ASCIIΪ16�ֽڡ�BCD��Ϊ8�ֽ� 	*******
 *******			  int   pLen   :�������ݳ��ȣ�ASCIIΪ16�ֽڡ�BCD��Ϊ8�ֽ� 		*******
 *******			  char *pOut   :(���)�������ɵĹ�����Կ						*******
 *******              int   pFlag  :���ݸ�ʽ��־��ȱʡΪ��0��BCD�룻����ֵΪASCII   *******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ���<0	;����������£�						*******
 *******								-1��������Կ������������ݴ���				*******
 *******                                -2�����������Կ����						*******
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


	if(pFlag == 0)				// �����������ΪBCD��
	{
		// ������ݺϷ���
		if(( pKeyLen != 16)  || (pLen != 8))
		{
			return -1;
		}

		// ����������
		memcpy(szKey,pKey,pKeyLen);
		memcpy(szHexIn,pIn,pLen);

	}else						// �����������ΪASCII�� 
	{
		// ������ݺϷ���
		if( ( (int)strlen(pKey) != pKeyLen ) || (pKeyLen != 32) ||
			( (int)strlen(pIn)  != pLen )    || ( pLen != 16 ) )
		{
			return -1;
		}

		// ����������
		ftAtoh(pKey,szKey,16);
		ftAtoh(pIn,szHexIn,8);
	}

	// ʹ��3DES�������ɹ�����Կ
	nRet = ft3DesEnc(szKey,16,szHexIn,szHexOut,8,0);
	if(nRet != 0)
	{
		return -2;
	}


	if(pFlag == 0)				// �����������ΪBCD��
	{
		memcpy(pOut,szHexOut,8);
	}else						// �����������ΪASCII��
	{
		ftHtoa(szHexOut,szOut,8);
		memcpy(pOut,szOut,16);
	}

	return 0;
}



/******************************************************************************************
 *******	�������ƣ�CalMac   -��ţ�23-											*******
 *******	�������ܣ����ڽ���MAC���㺯��[Ȧ�桢Ȧ�ᡢȡ�֡����ѵ�]					*******
 *******----------------------------------------------------------------------------*******
 *******	����������char *Key    :����MAC�Ĺ�����Կ��8�ֽ�,BCD��					*******
 *******			  char *Vector :����MAC�ĳ�ʼ��������8�ֽڣ�BCD��,ͨ��Ϊ��\x00	*******
 *******			  char *Data   :����MAC������, BCD��,Ϊ8�ֽڵı���          	*******
 *******			  int   pLen   :����MAC�����ݳ��ȣ�								*******
 *******			  char *pOut   :(���)���㷵�ص�MACֵ��BCD�룬����Ϊ4�ֽ�		*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ���<0	;����������£�						*******
 *******								-1��������Կ������������ݴ���				*******
 *******                                -2�����������Կ����						*******
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
	int nBcdDataLen ;    // BCD��ļ������ݳ���
	int nItem;			 // ����BCD������������
	int i;
	
	memset(szVector, 0x00,sizeof(szVector));
	memset(szCommand,0x00,sizeof(szCommand));
	memset(szMAC,    0x00,sizeof(szMAC));
	memset(szSK,     0x00,sizeof(szSK));
	memset(szTmp,    0x00,sizeof(szTmp));
	memset(szBcdMAC, 0x00,sizeof(szBcdMAC));
	memset(szXOR,    0x00,sizeof(szXOR));

	nRet = -1;

	// ��鴫��������ĳ����Ƿ�Ϸ���������Կ��16��ASCII����ʼ��������16��ASCII���������ݣ�16������16�ı���
	if( (strlen(Key) != 16) || (strlen(Vector) != 16) || (strlen(Data) % 16 != 0))
	{
		return -1;
	}

	memset(szBcdVector,0x00,sizeof(szBcdVector));
	memset(szBcdSK,0x00,sizeof(szBcdSK));


	// ���ó�ʼ������
	sprintf(szVector,Vector);

	// ����SK
	sprintf(szSK,Key);
	
	nLen =(int) strlen(Data);

	nBcdDataLen = nLen / 2;				// BCD����������

	nItem = nBcdDataLen / 8;			// Ҫ���в�ֵ����������

	char *szBcdData;

	szBcdData = (char *) malloc(nBcdDataLen +1 );
	if(szBcdData == NULL)
	{
		return -2;
	}

	memset(szBcdData,0x00,nBcdDataLen +1);

	// ��ASCII�������ת����BCD���������
	ftAtoh(szSK,szBcdSK,8);
	ftAtoh(szVector,szBcdVector,8);
	ftAtoh(Data,szBcdData,nBcdDataLen);
	

	for(i= 0;i< nItem;i++)
	{
		// 1.�����ݽ���XOR����
		memset(szTmp,0x00,sizeof(szTmp));
		
		memcpy(szTmp,szBcdData+i*8,8);
		// �����ݽ����������
		ftCalXOR(szBcdVector,szTmp,8,szXOR);
		memset(szTmp,0x00,sizeof(szTmp));
		// �����ֵ����DES����
		CDES::RunDes(0,szXOR,8,szBcdSK,8,szTmp);
		memcpy(szBcdVector,szTmp,8);
	}

	free(szBcdData);
	szBcdData = NULL;

	// szBcdVector ǰ���ֽ�ΪMACֵ
	memcpy(szBcdMAC,szBcdVector,4);		
 	ftHtoa(szBcdMAC,szMAC,4);

	memcpy(AscMAC,szMAC,8);
	memcpy(AscMAC+8,"\x00",1);
 
    return 0;


}



/******************************************************************************************
 *******	�������ƣ�CalMacFor3Des   -��ţ�24-									*******
 *******	�������ܣ������������MAC���㺯��										*******
 *******----------------------------------------------------------------------------*******
 *******	����������char *Key     :����MAC����Կ��32���ַ���ASCII					*******
 *******			  char *InitData:����MAC�ĳ�ʼ��������16���ַ���ASCII			*******
 *******			  char *Data    :����MAC������,8�ı���,ASCII		          	*******
 *******			  char *Out     :(���)���㷵�ص�MACֵ�� ASCII					*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��ʧ�ܷ���<0	;����������£�						*******
 *******								-1��������Կ������������ݴ���				*******
 *******                                -2�����������Կ����						*******
 ******************************************************************************************/
int ftCalMacFor3Des(char *Key,char *InitData,char *Data,char *Out)
{
    char szKey[16+1];			// BCD�����Կ				16�ֽ�
    char szKeyL[8+1];			// BCD�����Կǰ�벿��		8�ֽ�
    char szInitData[8+1];		// BCD��ĳ�ʼֵ			8�ֽ�
	char *pData;                // BCD��ļ�������      
	char szItem[8+1];			// BCD��ĵ�ǰ����������	8�ֽ� 
	char szTmp[8+1];			// ����XOR�����������		8�ֽ�
	char szXORVal[8+1];			// XOR���ֵ				8�ֽ�
    int  nItem = 0;				// �ֽ��8�ֽ�һ�������������
	int  nDataLen;				// BCD������ݳ���
	char szMac[8+1];
	int  i;


	memset(szKey     , 0x00, sizeof(szKey));
	memset(szInitData, 0x00, sizeof(szInitData));
    memset(szItem    , 0x00, sizeof(szItem));
	memset(szTmp     , 0x00, sizeof(szTmp));

	// �������ݺϷ��Լ��
	if( strlen(Key) != 32 || strlen(InitData) != 16 )
	{
		
		return -1;
	}

	// ��鴫�����ݵĺϷ��ԣ�BCD�볤�ȱ�����8�ı���
	nDataLen = (int)strlen(Data);
	if (nDataLen % 2 != 0)
	{
		
		return -1;
	}

	// ȡ�������ݵ�BCD�볤��
	nDataLen = nDataLen / 2;			

	// ��鴫��������Ƿ�Ϊ8�ı���
	if( nDataLen % 8 != 0)
	{
	
		return -1;
	}

	// ��8�ֽڲ�ֵ�����������
	nItem = nDataLen / 8;			

	pData = (char *)malloc( nDataLen + 1);
	if( pData == NULL)
	{

		return -1;
	}
	memset(pData,0x00,nDataLen + 1);


	// ���������ݵ�ASCII��ת����BCD��
	// ������ACIIת����BCD��
	ftAtoh(Data,pData,nDataLen);
	
	// ������Կ
	ftAtoh(Key,szKey,16);	

	// ��Կ����벿��(��ǰ8�ֽ�)
	memcpy(szKeyL,szKey,8);			 

	// MAC����ĳ�ʼ������ 4 �ֽ������ + 4�ֽ� 0x00 
	ftAtoh(InitData,szInitData,8);

	// ����MAC
	for(i = 0; i < nItem ; i++)
	{
		// �����������
		memcpy(szItem,pData+i*8,8);
		ftCalXOR(szItem,szInitData,8,szXORVal);
		if((i+1) != nItem)
		{	
			// DES����
			CDES::RunDes(0,szXORVal,8,szKeyL,8,szTmp);
		}else {
			// 3DES����
			CDES::RunDes(0,szXORVal,8,szKey,16,szTmp);
		}
		memcpy(szInitData,szTmp,8);
	}

	// �����MAC����
	memset(szMac,0x00,sizeof(szMac));
	ftHtoa(szTmp,szMac,4);
	
	memcpy(Out,szMac,8);

	// �ͷ�ָ��
	free(pData);
	pData = NULL;

    return 0;
}





/******************************************************************************************
 *******	�������ƣ�ftGetProtocol	   -��ţ�15-									*******
 *******	�������ܣ������ϵ縴λ��Ϣ[BCD��ֵ]����IC����ͨ��Э��,					*******
 *******----------------------------------------------------------------------------******* 
 *******	����������unsigned char *pATR��Ҫ����ͨ��Э���ATR�ֽ���[BCD��]			*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ��IC����ͨ��Э�飻0=ͨ��Э��Ϊ��T=0;1=ͨ��Э��ΪT=1				******* 
 ******************************************************************************************/
int ftGetProtocolForATR(unsigned char *pATR)
{    
	int nTDFlag = 0;
	int nTCFlag = 0;
	int nTBFlag = 0;
	int nTAFlag = 0;
	int nPos;
	
	int nProtocol = 0;		// IC��ͨ��Э�飬ȱʡΪ��T=0
	unsigned char nT0;
	
	// ��ȡATR�ֽ����� T0 [ATR��2�ֽ�] ֵ
	nT0 = (unsigned char )pATR[1] & 0xF0 ;
	
	// ���TA1�Ƿ����
	if( (nT0 & 0x10 ) != 0)
	{
		nTAFlag = 1;
	}
	
	// ���TB1�Ƿ����
	if( (nT0 & 0x20 ) != 0)
	{
		nTBFlag = 1;
	}
	
	// ���TC1�Ƿ����
	if( ( nT0 & 0x40 ) != 0)
	{
		nTCFlag = 1;
	}
	
	// ���TD1�Ƿ����
	if( ( nT0 & 0x80 ) != 0)
	{
		nTDFlag = 1;
	}
	
	// ��TD1 ��ͨ��Э��Ϊ��T= 0
	if (nTDFlag == 0)
	{
		nProtocol = 0;
	}else 
	{
		// ����TD1 ֵ��λ��
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
 *******	�������ƣ�ftWriteLog  -��ţ�26-										*******
 *******	�������ܣ���־�ļ���¼����												*******
 *******----------------------------------------------------------------------------*******
 *******	����������char *FileName   :��־�ļ����ƣ����·�������·��			*******
 *******			  char *pData      :��־д������							 	*******
 *******			  int   pDataLen   :д�����־���ݳ���					 		*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��													*******
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


	// ��ȡ��ǰϵͳʱ��
	GetLocalTime(&st);

	// ��ȡ��ǰʱ�����-��-�� - ��ʱ���֣��룺΢��
	//strTime.Format(" %04d-%02d-%02d - %02d:%02d:%02d:%03d",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);	
	strTime.Format("%02d:%02d:%02d:%03d",st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);	
	// ��ȡ��YYYYMMDD ��ʽ������
	strDate.Format("%04d%02d%02d",st.wYear,st.wMonth,st.wDay);										
	
	// ��־�ļ������浽CString����
	strFileName.Format("%s",pFileName);

	// ��ȡ��־�ļ�������λ��
	nPos = 	strFileName.Find('.');
	if(nPos>0)
	{
		// ����־�ļ���������ǰ�����ַ�:_
		strFileName.Insert(nPos,'_');
	}

	nPos = 	strFileName.Find('.');
	if(nPos>0)
	{
		// ����־�ļ�����չ��ǰ��������[YYYYMMDD]
		strFileName.Insert(nPos,(LPCTSTR)strDate);
	}
	
	memset(szLogBuf,0x00,sizeof(szLogBuf));

	// ������ĸ�ʽ������
	va_start( valist, format );
	_vsnprintf_s( szLogBuf  , 2048 , _TRUNCATE, format, valist );
	va_end( valist );

	strLogBuf = szLogBuf;
	strCh = "";
	// �����ַ���ǰ�Ļ����ַ�
	while(true)
	{
			nPos=strLogBuf.Find('\n');	
			if(nPos!=0) break;			
			strCh+='\n';
			strLogBuf.Delete(0);
	}

	// ����ӷ�ʽ����־�ļ�
	nRet = fopen_s(&hLogFile,(LPCTSTR)strFileName,"a+");
	if( nRet == 0)
	{
		// д����־�ļ�����
		fprintf(hLogFile,"%s%s-->%s\n",(LPCTSTR)strCh,(LPCTSTR)strTime,(LPCTSTR)strLogBuf);		// ����ʱ����ʾ:2008-3-17
		fclose(hLogFile);
	}

}
#endif
/******************************************************************************************
 *******	�������ƣ�ftSplitStr -��ţ�27-											*******
 *******	�������ܣ���ָ���ַ�����ַ���											*******
 *******----------------------------------------------------------------------------*******
 *******	����������char *FileName   :��־�ļ����ƣ����·�������·��			*******
 *******			  char *pData      :��־д������							 	*******
 *******			  int   pDataLen   :д�����־���ݳ���					 		*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��													*******
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
			// �ַ�������
			nLen = nCurrPos - nStartPos;
			if(nLen == 0)
			{
				memcpy(pData[nNum],"\0",1);
			}else 
			{
				// �����������󳤶�Ϊ80�ֽ�
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



// ����BCD���ַ�����PCKֵ

/******************************************************************************************
 *******	�������ƣ�ftCalPCK -��ţ�28-											*******
 *******	�������ܣ�����ָ��BCD�����ݵ�PCKֵ(������������)						*******
 *******----------------------------------------------------------------------------*******
 *******	����������unsigned char *Str:Ҫ�����BCD������							*******
 *******			  int  Len			:Ҫ��������ݳ���						 	*******
 *******			  unsigned int *PCK :(���)���㷵�ص�PCKֵ				 		*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�0��													*******
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
